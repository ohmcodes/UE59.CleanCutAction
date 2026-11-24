// Clean! Cut! Action!

#include "Interaction/LTTInteractionComponent.h"
#include "VR/LTTVRComponent.h"
#include "Interaction/LTTCleanupBarker.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractable.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTWashStationComponent.h"
#include "Interaction/LTTWaterPuddle.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

ULTTInteractionComponent::ULTTInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULTTInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	TryBindInput();
}

ULTTCarryComponent* ULTTInteractionComponent::GetCarry() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULTTCarryComponent>() : nullptr;
}

void ULTTInteractionComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UEnhancedInputComponent* EIC = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!EIC || BoundInputComponent.Get() == EIC)
	{
		return;
	}

	if (InteractAction)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ULTTInteractionComponent::Interact);
	}
	if (ThrowAction)
	{
		EIC->BindAction(ThrowAction, ETriggerEvent::Started, this, &ULTTInteractionComponent::ThrowHeld);
	}
	BoundInputComponent = EIC;
}

void ULTTInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The pawn's input component is created on possession, which can come after BeginPlay.
	TryBindInput();
	UpdateFocus();
	UpdatePlaceHint();
}

void ULTTInteractionComponent::UpdatePlaceHint()
{
	PlaceHint = ELTTPlaceHint::None;
	const ULTTCarryComponent* Carry = GetCarry();
	ALTTInteractableBase* Held = Carry ? Carry->GetHeldItem() : nullptr;

	// The prop under the held one's home (table under a cup) isn't back yet: show where it goes, in orange.
	ALTTInteractableBase* NeededSupport = nullptr;
	if (Held && Held->bIsCleanupTask && Held->HasHome())
	{
		ALTTInteractableBase* Blocking = Held->GetBlockingSupport();
		if (Blocking && !Blocking->IsHeld())
		{
			NeededSupport = Blocking;
		}
	}
	SetSupportGhost(NeededSupport);
	TargetPuddle = nullptr;
	ULTTCarryComponent::ClearToolAim();

	const APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Held || !Controller)
	{
		return;
	}

	PlaceHint = ELTTPlaceHint::Drop;
	TargetBin = nullptr;

	FVector ViewLoc;
	FRotator ViewRot;
	ULTTVRComponent::GetAimViewPoint(Controller, ViewLoc, ViewRot); // VR: the hand's ray

	// Trash has no home: it goes in a bin. Aiming at one within reach = "Throw in the bin".
	if (Held->bIsTrash)
	{
		Held->UpdateGhost(false, false, false);
		if (ALTTInteractableBase* Bin = ALTTInteractableBase::FindNearestTrashBin(this, ViewLoc))
		{
			const FVector ToBin = ALTTInteractableBase::GetBinOpening(Bin) - ViewLoc;
			FVector BinOrigin, BinExtent;
			Bin->GetActorBounds(true, BinOrigin, BinExtent);
			const double Dist = ToBin.Size();
			// Within reach, and the aim passes within the bin's radius of its opening.
			const double Off = FVector::CrossProduct(ViewRot.Vector(), ToBin).Size();
			if (Dist <= PlaceDistance && FVector::DotProduct(ViewRot.Vector(), ToBin) > 0.0 && Off <= FMath::Max(BinExtent.X, BinExtent.Y) + 10.0)
			{
				PlaceHint = ELTTPlaceHint::Trash;
				TargetBin = Bin;
			}
		}
		return;
	}

	// Dishes have no home either: they go in a sink. Aiming at the basin within reach = "Wash in the sink".
	TargetStation = nullptr;
	if (Held->bIsDishwashable)
	{
		Held->UpdateGhost(false, false, false);
		if (ULTTWashStationComponent* Station = ULTTWashStationComponent::FindNearest(this, ViewLoc))
		{
			const FVector ToBasin = Station->GetComponentLocation() - ViewLoc;
			const double Off = FVector::CrossProduct(ViewRot.Vector(), ToBasin).Size();
			if (ToBasin.Size() <= PlaceDistance && FVector::DotProduct(ViewRot.Vector(), ToBasin) > 0.0 && Off <= Station->AimRadius)
			{
				PlaceHint = ELTTPlaceHint::Wash;
				TargetStation = Station;
			}
		}
		return;
	}

	// The mop: aiming at a puddle within reach = "Mop up the water". Anywhere else it's dropped like any prop.
	if (Held->bIsMop)
	{
		FVector At;
		if (ALTTWaterPuddle* Puddle = ALTTWaterPuddle::FindAimed(this, ViewLoc, ViewRot.Vector(), MopReach, At, GetOwner()))
		{
			Held->UpdateGhost(false, false, false);
			PlaceHint = ELTTPlaceHint::Mop;
			TargetPuddle = Puddle;
			TargetPuddlePoint = At;
			ULTTCarryComponent::SetToolAim(At); // the mop points at the water
			return;
		}
	}

	if (!Held->bIsCleanupTask || !Held->HasHome())
	{
		return;
	}
	const bool bTargeted = Held->IsTargetingHome(ViewLoc, ViewRot.Vector(), PlaceDistance);
	const bool bReady = Held->IsHomeSupportReady();
	if (bTargeted)
	{
		PlaceHint = bReady ? ELTTPlaceHint::Place : ELTTPlaceHint::NeedsSupport;
	}
	else if (bBreakOnNearMissDrop && Held->IsNearHome(ViewLoc, ViewRot.Vector(), PlaceDistance, NearMissMargin))
	{
		PlaceHint = ELTTPlaceHint::Fumble;
	}
	Held->UpdateGhost(true, bTargeted, bReady);
}

void ULTTInteractionComponent::SetSupportGhost(ALTTInteractableBase* Support)
{
	ALTTInteractableBase* Old = ShownSupportGhost.Get();
	if (Old != Support && IsValid(Old) && !Old->IsHeld())
	{
		Old->UpdateGhost(false, false, false);
	}
	ShownSupportGhost = Support;
	if (Support)
	{
		Support->UpdateGhost(true, false, true, true);
	}
}

FText ULTTInteractionComponent::GetPlaceHintText() const
{
	switch (PlaceHint)
	{
	case ELTTPlaceHint::Place:
		return NSLOCTEXT("LTT", "HintPlace", "Place");
	case ELTTPlaceHint::NeedsSupport:
	{
		const ULTTCarryComponent* Carry = GetCarry();
		const ALTTInteractableBase* Held = Carry ? Carry->GetHeldItem() : nullptr;
		if (const ALTTInteractableBase* Blocking = Held ? Held->GetBlockingSupport() : nullptr)
		{
			return FText::Format(NSLOCTEXT("LTT", "HintSupport", "Put the {0} back first"), Blocking->GetPropName());
		}
		return NSLOCTEXT("LTT", "HintDrop", "Drop");
	}
	case ELTTPlaceHint::Drop:
		return NSLOCTEXT("LTT", "HintDrop", "Drop");
	case ELTTPlaceHint::Fumble:
		return NSLOCTEXT("LTT", "HintFumble", "Missed the mark! Dropping here breaks it");
	case ELTTPlaceHint::Trash:
		return NSLOCTEXT("LTT", "HintTrash", "Throw in the bin");
	case ELTTPlaceHint::Wash:
		return NSLOCTEXT("LTT", "HintWash", "Wash in the sink");
	case ELTTPlaceHint::Mop:
		return NSLOCTEXT("LTT", "HintMop", "Mop up the water");
	default:
		return FText::GetEmpty();
	}
}

void ULTTInteractionComponent::UpdateFocus()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Controller || !Controller->IsLocalController())
	{
		SetFocus(nullptr);
		return;
	}

	const ULTTCarryComponent* Carry = GetCarry();
	if (Carry && Carry->IsHolding())
	{
		SetFocus(nullptr);
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	ULTTVRComponent::GetAimViewPoint(Controller, ViewLoc, ViewRot); // VR: the hand's ray
	AActor* NewFocus = TraceForInteractable(ViewLoc, ViewRot.Vector(), TraceDistance, TraceRadius);
	// Water puddles have no collision (props lying in them stay grabbable): looking at the water focuses it.
	if (!NewFocus)
	{
		FVector At;
		NewFocus = ALTTWaterPuddle::FindAimed(this, ViewLoc, ViewRot.Vector(), TraceDistance, At, GetOwner());
	}
	// Gravity pull: out of reach, only props that can be picked up (a lamp across the room isn't switched from there).
	if (!NewFocus && bGravityPull && PullDistance > TraceDistance)
	{
		AActor* Far = TraceForInteractable(ViewLoc, ViewRot.Vector(), PullDistance, PullTraceRadius);
		if (Far && Cast<ALTTInteractableBase>(Far) && ILTTInteractable::Execute_GetInteractKind(Far) == ELTTInteractKind::Pickup)
		{
			NewFocus = Far;
		}
	}
	SetFocus(NewFocus);
}

AActor* ULTTInteractionComponent::TraceForInteractable(const FVector& ViewLoc, const FVector& Dir, float Distance, float Radius) const
{
	const FVector End = ViewLoc + Dir * Distance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTInteractTrace), false, GetOwner());
	FHitResult Hit;
	AActor* NewFocus = nullptr;
	// Placed props are see-through for the trace, so a cup already put back on a table doesn't block
	// grabbing the plate behind it. Set dressing props are see-through too, unless they can be used (a lamp, an appliance). Walls and static set pieces still block.
	for (int32 Pass = 0; Pass < MaxPlacedPassThrough + 1; ++Pass)
	{
		if (!GetWorld()->SweepSingleByChannel(Hit, ViewLoc, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(Radius), Params))
		{
			break;
		}
		AActor* HitActor = Hit.GetActor();
		const ALTTInteractableBase* HitProp = Cast<ALTTInteractableBase>(HitActor);
		if (HitProp && (HitProp->IsHeld() || ((HitProp->IsPlaced() || HitProp->IsSetDressing()) && !HitProp->IsUsableWhenPlaced())))
		{
			Params.AddIgnoredActor(HitActor);
			continue;
		}
		if (HitActor && HitActor->Implements<ULTTInteractable>() && ILTTInteractable::Execute_CanInteract(HitActor, GetOwner()))
		{
			NewFocus = HitActor;
		}
		break;
	}
	return NewFocus;
}

void ULTTInteractionComponent::SetFocus(AActor* NewFocus)
{
	AActor* Old = FocusedActor.Get();
	if (Old == NewFocus)
	{
		return;
	}

	if (Old && Old->Implements<ULTTInteractable>())
	{
		ILTTInteractable::Execute_OnFocusEnd(Old, GetOwner());
	}

	FocusedActor = NewFocus;
	FText Prompt;
	if (NewFocus)
	{
		ILTTInteractable::Execute_OnFocusBegin(NewFocus, GetOwner());
		Prompt = ILTTInteractable::Execute_GetPromptText(NewFocus, GetOwner());
	}
	OnFocusChanged.Broadcast(NewFocus, Prompt);
}

void ULTTInteractionComponent::Interact()
{
	// VR: the trigger is also the menu click. On a button it clicks; it doesn't grab the prop behind the menu.
	if (ULTTVRComponent::IsPointerOnUI())
	{
		return;
	}
	ULTTCarryComponent* Carry = GetCarry();
	if (Carry && Carry->IsHolding())
	{
		UpdatePlaceHint();
		if (PlaceHint == ELTTPlaceHint::Place && Carry->PlaceHeld())
		{
			PlaceHint = ELTTPlaceHint::None;
			return;
		}
		ALTTInteractableBase* Dropped = Carry->GetHeldItem();
		if (PlaceHint == ELTTPlaceHint::Mop && TargetPuddle.IsValid())
		{
			// One stroke; the mop stays in the hand.
			if (TargetPuddle->MopUp(Dropped, TargetPuddlePoint))
			{
				Carry->PlayUseSwing(TargetPuddle->StrokeTime);
			}
			return;
		}
		if (PlaceHint == ELTTPlaceHint::Trash && TargetBin.IsValid())
		{
			ALTTInteractableBase* Bin = TargetBin.Get();
			Carry->Drop();
			PlaceHint = ELTTPlaceHint::None;
			if (IsValid(Dropped))
			{
				Dropped->ThrowAwayInto(Bin);
			}
			return;
		}
		if (PlaceHint == ELTTPlaceHint::Wash && TargetStation.IsValid())
		{
			ULTTWashStationComponent* Station = TargetStation.Get();
			Carry->Drop();
			PlaceHint = ELTTPlaceHint::None;
			if (IsValid(Dropped))
			{
				Dropped->WashIn(Station);
			}
			return;
		}
		const bool bFumble = PlaceHint == ELTTPlaceHint::Fumble;
		if (PlaceHint == ELTTPlaceHint::NeedsSupport && IsValid(Dropped))
		{
			ALTTCleanupBarker::NotifyEvent(this, ELTTBarkEvent::WrongOrder, Dropped); // its table isn't back yet
		}
		Carry->Drop();
		if (bFumble && IsValid(Dropped))
		{
			// Aimed just beside its silhouette: it slips and shatters on whatever it lands on.
			Dropped->ArmBreakOnLanding();
		}
		return;
	}

	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return;
	}

	if (ILTTInteractable::Execute_GetInteractKind(Target) == ELTTInteractKind::Pickup)
	{
		if (ALTTInteractableBase* Prop = Cast<ALTTInteractableBase>(Target))
		{
			SetFocus(nullptr);
			if (Carry)
			{
				// Out of arm's reach: gravity pull, the prop flies across the room into the hand.
				float Dist = 0.f;
				const APawn* Pawn = Cast<APawn>(GetOwner());
				if (AController* Controller = Pawn ? Pawn->GetController() : nullptr)
				{
					FVector ViewLoc;
					FRotator ViewRot;
					ULTTVRComponent::GetAimViewPoint(Controller, ViewLoc, ViewRot); // VR: the hand's ray
					Dist = FVector::Dist(ViewLoc, Prop->GetActorLocation());
				}
				if (bGravityPull && Dist > TraceDistance + TraceRadius)
				{
					Carry->PullFromAfar(Prop, FMath::Clamp(Dist / FMath::Max(PullSpeed, 100.f), 0.25f, 0.9f));
				}
				else
				{
					Carry->PickUp(Prop);
				}
			}
			return;
		}
	}

	ILTTInteractable::Execute_OnInteract(Target, GetOwner());
}

bool ULTTInteractionComponent::TryMopStroke()
{
	ULTTCarryComponent* Carry = GetCarry();
	if (!Carry || !Carry->IsHolding())
	{
		return false;
	}
	UpdatePlaceHint();
	if (PlaceHint != ELTTPlaceHint::Mop || !TargetPuddle.IsValid())
	{
		return false;
	}
	if (TargetPuddle->MopUp(Carry->GetHeldItem(), TargetPuddlePoint))
	{
		Carry->PlayUseSwing(TargetPuddle->StrokeTime);
		return true;
	}
	return false;
}

void ULTTInteractionComponent::ThrowHeld()
{
	if (ULTTCarryComponent* Carry = GetCarry())
	{
		if (Carry->IsHolding())
		{
			Carry->Throw();
		}
	}
}
