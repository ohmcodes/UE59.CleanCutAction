// Clean! Cut! Action! - Base class for every prop the player can pick up.
// Blueprint master: /Game/_CleanCutAction/Blueprints/Interactables/BP_MasterInteractables

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTInteractable.h"
#include "LTTInteractableBase.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UGeometryCollection;

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

	// ---- Breaking: one shared pre-fractured cube for every prop (keeps the web build small) ----

	/** Shared fractured cube used by ShatterGeneric. It's scaled to this prop's bounds and given its material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Break")
	TSoftObjectPtr<UGeometryCollection> GenericBreakCollection;

	/** Seconds before the pieces are removed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Break", meta = (ClampMin = "0.5"))
	float DebrisLifeSpan = 4.f;

	/** Outward speed given to the pieces (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Break", meta = (ClampMin = "0"))
	float ShatterImpulse = 250.f;

	/** Breaks the prop into GenericBreakCollection pieces sized and coloured like it. Destroying the prop counts it as
	 *  broken for the mess director. Returns the debris actor, or null if the generic collection is missing. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Break")
	AActor* ShatterGeneric(FVector ImpactPoint, bool bDestroyProp = true);

	// ---- Cleanup: put the prop back where it was in the clean level ----

	/** The set isn't clean until this prop is back home. Off for props that don't matter (trash, extras). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup")
	bool bIsCleanupTask = true;

	/** Skip the earthquake: the prop starts placed and locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup")
	bool bStartsAtHome = false;

	/** How close the aim (or the held prop) must be to the home silhouette to place it. 0 = automatic from size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup", meta = (ClampMin = "0"))
	float SnapRadius = 0.f;

	/** Translucent material for the home silhouette (needs a vector parameter "Color"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup")
	TSoftObjectPtr<UMaterialInterface> GhostMaterial;

	/** Clean-level transform, recorded at BeginPlay before the mess. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Interactable|Cleanup")
	FTransform HomeTransform;

	/** Prop this one rests on at home (e.g. the table under a cup). It has to be placed first. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Interactable|Cleanup")
	TObjectPtr<ALTTInteractableBase> HomeSupport;

	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	bool IsPlaced() const { return bPlaced; }

	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	bool HasHome() const { return bHasHome; }

	/** True if nothing has to be placed before this prop (its home support is placed or static). */
	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	bool IsHomeSupportReady() const;

	/** The lowest prop in this one's stack that isn't back yet (the table under the plate under this croissant),
	 *  i.e. what has to be put back next. Null if nothing blocks this prop. */
	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	ALTTInteractableBase* GetBlockingSupport() const;

	/** World centre of the prop's bounds at home. */
	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	FVector GetHomeCenter() const;

	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	float GetSnapRadius() const;

	/** True if the view ray passes through the home silhouette, or the prop's centre is already inside it. */
	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	bool IsTargetingHome(const FVector& ViewLocation, const FVector& ViewDirection, float MaxDistance) const;

	/** Readable prop name for the HUD, e.g. "Coffee Cup". */
	UFUNCTION(BlueprintPure, Category = "Interactable")
	FText GetPropName() const;

	/** Records the current transform as home. Called at BeginPlay. */
	void RecordHome();

	/** Finds the prop under the home position. Called by the mess director once every home is recorded. */
	void FindHomeSupport();

	/** Slides from the hand to home and locks there. Called by ULTTCarryComponent::PlaceHeld. */
	void PlaceAtHome();

	/** Locks at home immediately (bStartsAtHome). */
	void SnapToHome();

	/** Shows the home silhouette while carried. bTargeted = aiming at it, bReady = its support is placed.
	 *  bAsRequiredSupport = shown (orange) because a held prop needs this one put back first. */
	void UpdateGhost(bool bVisible, bool bTargeted, bool bReady, bool bAsRequiredSupport = false);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable|Cleanup", meta = (DisplayName = "On Placed"))
	void ReceivePlaced();

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
	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	void FinishPlacing();

	bool bHasHome = false;
	bool bPlaced = false;
	bool bPlacing = false;
	float PlaceElapsed = 0.f;
	FTransform PlaceFrom;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ghost;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	void ApplyPhysicsSettings();

	/** True if something is directly under the prop (table, floor, shelf). False for wall/ceiling-mounted props. */
	bool HasSupportBelow() const;

	/** Turns on physics for props touching this one, so things stacked on it fall when it's picked up. */
	void WakeTouchingProps();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Holder;

	FName SavedCollisionProfile;
};
