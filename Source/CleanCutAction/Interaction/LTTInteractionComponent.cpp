// Clean! Cut! Action!

#include "Interaction/LTTInteractionComponent.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractable.h"
#include "Interaction/LTTInteractableBase.h"
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
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

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
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector End = ViewLoc + ViewRot.Vector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTInteractTrace), false, GetOwner());
	FHitResult Hit;
	AActor* NewFocus = nullptr;
	// Placed props are see-through for the trace, so a cup already put back on a table doesn't block
	// grabbing the plate behind it. Walls and static set pieces still block.
	for (int32 Pass = 0; Pass < MaxPlacedPassThrough + 1; ++Pass)
	{
		if (!GetWorld()->SweepSingleByChannel(Hit, ViewLoc, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), Params))
		{
			break;
		}
		AActor* HitActor = Hit.GetActor();
		const ALTTInteractableBase* HitProp = Cast<ALTTInteractableBase>(HitActor);
		if (HitProp && (HitProp->IsHeld() || (HitProp->IsPlaced() && !HitProp->IsUsableWhenPlaced())))
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
	SetFocus(NewFocus);
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
		const bool bFumble = PlaceHint == ELTTPlaceHint::Fumble;
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
				Carry->PickUp(Prop);
			}
			return;
		}
	}

	ILTTInteractable::Execute_OnInteract(Target, GetOwner());
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
