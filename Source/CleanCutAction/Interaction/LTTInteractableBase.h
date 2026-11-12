// Clean! Cut! Action! - Base class for every prop the player can pick up.
// Blueprint master: /Game/_CleanCutAction/Blueprints/Interactables/BP_MasterInteractables

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTInteractable.h"
#include "LTTInteractableBase.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTInteractableBase : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTInteractableBase();

	/** The prop mesh: the root Static Mesh component added in BP_MasterInteractables. Set the mesh asset in each child. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Interactable")
	TObjectPtr<UStaticMeshComponent> Mesh;

	virtual void PostInitializeComponents() override;

	/** Name shown in the HUD prompt, e.g. "Coffee Cup". Empty uses the actor label. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	FText DisplayName;

	/** Verb shown in the prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	FText PromptVerb = NSLOCTEXT("LTT", "PickUp", "Pick up");

	/** If false the prop is ignored by the interaction trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	bool bCanBePickedUp = true;

	/** Simulate physics from BeginPlay (props that rest on something). Wall/ceiling-mounted props stay put until grabbed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Physics")
	bool bSimulatePhysicsOnBeginPlay = true;

	/** Mass in kg while simulating. 0 keeps the mass computed from the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Physics", meta = (ClampMin = "0"))
	float MassOverrideKg = 0.f;

	/** Multiplies the carry component's throw speed (heavy props go less far). 0 = automatic from size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Physics", meta = (ClampMin = "0"))
	float ThrowMultiplier = 0.f;

	/** Distance in front of the camera while held. 0 = automatic from the prop's size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold", meta = (ClampMin = "0"))
	float HoldDistance = 0.f;

	/** Extra offset from the automatic hold position, in camera space (X fwd, Y right, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold")
	FVector HoldOffset = FVector::ZeroVector;

	/** Extra rotation applied while held, relative to the camera-facing hold rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold")
	FRotator HoldRotationOffset = FRotator::ZeroRotator;

	/** Use both hands (IK) while held. Auto turns on for props wider than TwoHandedRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold")
	bool bAutoTwoHanded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold", meta = (EditCondition = "!bAutoTwoHanded"))
	bool bTwoHanded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Hold", meta = (EditCondition = "bAutoTwoHanded"))
	float TwoHandedRadius = 22.f;

	/** True while a carry component holds this prop. */
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsHeld() const { return Holder != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Interactable")
	AActor* GetHolder() const { return Holder; }

	/** Bounding sphere radius of the mesh in world scale. */
	UFUNCTION(BlueprintPure, Category = "Interactable")
	float GetPropRadius() const;

	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool UsesTwoHands() const;

	UFUNCTION(BlueprintPure, Category = "Interactable")
	float GetEffectiveThrowMultiplier() const;

	/** Called by ULTTCarryComponent. Turns off physics/collision and switches to first-person rendering. */
	virtual void BeginHold(AActor* InHolder);

	/** Called by ULTTCarryComponent. Restores collision and turns physics on. */
	virtual void EndHold(const FVector& LinearVelocity, const FVector& AngularVelocityDeg);

	/** Blueprint hooks for sounds/VFX. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable", meta = (DisplayName = "On Picked Up"))
	void ReceivePickedUp(AActor* ByActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable", meta = (DisplayName = "On Dropped"))
	void ReceiveDropped(AActor* ByActor, bool bWasThrown);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable", meta = (DisplayName = "On Focus Changed"))
	void ReceiveFocusChanged(bool bFocused);

	// ILTTInteractable
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Pickup; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override {}

	/** Set when EndHold is called with bThrown so the BP event can tell. */
	bool bLastReleaseWasThrow = false;

protected:
	virtual void BeginPlay() override;

	void ApplyPhysicsSettings();

	/** True if something is directly under the prop (table, floor, shelf). False for wall/ceiling-mounted props. */
	bool HasSupportBelow() const;

	/** Turns on physics for props touching this one, so things stacked on it fall when it's picked up. */
	void WakeTouchingProps();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Holder;

	FName SavedCollisionProfile;
};
