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
	if (GetWorld()->SweepSingleByChannel(Hit, ViewLoc, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<ULTTInteractable>() && ILTTInteractable::Execute_CanInteract(HitActor, GetOwner()))
		{
			NewFocus = HitActor;
		}
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
		Carry->Drop();
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
