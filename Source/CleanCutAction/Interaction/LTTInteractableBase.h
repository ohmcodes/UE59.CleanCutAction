// Clean! Cut! Action! - Base class for every prop the player can pick up.
// Blueprint master: /Game/_CleanCutAction/Blueprints/Interactables/BP_MasterInteractables

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTInteractable.h"
#include "Interaction/LTTPropSoundSet.h"
#include "LTTInteractableBase.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UGeometryCollection;
class ULTTLightSwitchComponent;
class ULTTUsableComponent;
class ULTTWashStationComponent;

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

	/** Material for a prop that broke but other props rest on (a table under cups): it goes back to its home in
	 *  this look so the props on it can still be placed. Needs a vector parameter "Color". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Break")
	TSoftObjectPtr<UMaterialInterface> BrokenMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Break")
	FLinearColor BrokenColor = FLinearColor(0.8f, 0.04f, 0.02f);

	/** Called instead of destroying the prop when it breaks. A prop other cleanup props rest on returns to its home,
	 *  locked and red (counted as broken); any other prop is destroyed. Also runs for the BP's DestroyActor call. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Break")
	void BreakAway();

	/** True once the prop broke and was put back red at its home. It can't break or be picked up again. */
	UFUNCTION(BlueprintPure, Category = "Interactable|Break")
	bool IsBrokenInPlace() const { return bBrokenInPlace; }

	/** Routes the BP's DestroyActor (BreakFromImpact) through BreakAway. */
	virtual void K2_DestroyActor() override;

	// ---- Cleanup: put the prop back where it was in the clean level ----

	/** The set isn't clean until this prop is back home. Off for props that don't matter (trash, extras). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup")
	bool bIsCleanupTask = true;

	// ---- Sound ----

	/** What it sounds like when picked up, put back and broken (sounds come from SoundSet). None = silent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Sound")
	ELTTPropMaterial SoundMaterial = ELTTPropMaterial::None;

	/** The sound bank (DA_PropSounds by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Sound")
	TSoftObjectPtr<ULTTPropSoundSet> SoundSet;

	/** Plays this prop's SoundMaterial sound for Event at the prop. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Sound")
	void PlayPropSound(ELTTPropSoundEvent Event) const;

	// ---- Use once placed ----

	/** Once placed, Interact switches the background music to the next track (e.g. the laptop on the counter).
	 *  While the music is muted it turns it back on instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Use")
	bool bChangesMusic = false;

	/** Switches the music to the next track (or unmutes it). Returns false if the level has no music player. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Use")
	bool ChangeMusic();

	// ---- Trash ----

	/** Trash: no home spot (no silhouette, never "Place"). It's cleaned up by putting it in a trash bin, which
	 *  counts like putting a prop back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Trash")
	bool bIsTrash = false;

	/** Trash that lands in (or hits) this prop is thrown away. Set on BP_Prop_Bin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Trash")
	bool bIsTrashBin = false;

	/** Seconds a thrown-away prop takes to shrink into the bin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Trash", meta = (ClampMin = "0.05"))
	float TrashTweenTime = 0.3f;

	/** Throws this trash prop into Bin: it shrinks into the bin's opening and disappears, and counts as cleaned up. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Trash")
	void ThrowAwayInto(AActor* Bin);

	UFUNCTION(BlueprintPure, Category = "Interactable|Trash")
	bool IsTrashed() const { return bTrashed; }

	/** Where trash goes in: the top centre of the bin. */
	static FVector GetBinOpening(const AActor* Bin);

	/** The trash bin nearest to From (null if the level has none). */
	static ALTTInteractableBase* FindNearestTrashBin(const UObject* WorldContext, const FVector& From);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable|Trash", meta = (DisplayName = "On Thrown Away"))
	void ReceiveThrownAway(AActor* Bin);

	// ---- Dishes ----

	/** Dish: no home spot (no silhouette, never "Place"). It's cleaned up by putting it in a sink (any actor with a
	 *  ULTTWashStationComponent): it's washed with bubbles and disappears, which counts like putting a prop back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Dishes", meta = (DisplayName = "Can Be Dishwashed"))
	bool bIsDishwashable = false;

	/** Washes this dish in Station: it shrinks into the basin and disappears, bubbles + sound, counts as cleaned up. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Dishes")
	void WashIn(ULTTWashStationComponent* Station);

	UFUNCTION(BlueprintPure, Category = "Interactable|Dishes")
	bool IsWashed() const { return bWashed; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Interactable|Dishes", meta = (DisplayName = "On Washed"))
	void ReceiveWashed(AActor* Sink);

	/** Trash or dish: goes in a bin / sink instead of to a home spot. */
	bool HasNoHomeSpot() const { return bIsTrash || bIsDishwashable; }

	/** Skip the earthquake. A cleanup task starts placed and locked; a prop that isn't a task (set dressing)
	 *  just stays where it is and doesn't count towards the clean set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Cleanup")
	bool bStartsAtHome = false;

	/** Use Custom Home instead of where the prop is placed in the level, e.g. it starts out on the counter but
	 *  belongs in a drawer. Turning it on fills Custom Home with the prop's current transform. */
	UPROPERTY(EditInstanceOnly, Category = "Interactable|Cleanup", meta = (InlineEditConditionToggle))
	bool bUseCustomHome = false;

	/** World transform of the home spot (shown as a silhouette in the editor). Moving the prop doesn't change it. */
	UPROPERTY(EditInstanceOnly, Category = "Interactable|Cleanup", meta = (EditCondition = "bUseCustomHome"))
	FTransform CustomHome;

	/** Editor button: the prop's current position becomes its home. Then move the prop to where it should start. */
	UFUNCTION(CallInEditor, Category = "Interactable|Cleanup")
	void SetCustomHomeHere();

	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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

	/** Like IsTargetingHome, but with the snap radius grown by Margin: true when the aim just misses the silhouette. */
	UFUNCTION(BlueprintPure, Category = "Interactable|Cleanup")
	bool IsNearHome(const FVector& ViewLocation, const FVector& ViewDirection, float MaxDistance, float Margin) const;

	/** The prop breaks (its own BreakCollection, via the BP BreakFromImpact event) the first time it hits something. Used for a sloppy drop right next to
	 *  its home silhouette. Picking it up again cancels it. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Break")
	void ArmBreakOnLanding();

	UFUNCTION(BlueprintPure, Category = "Interactable|Break")
	bool IsBreakOnLandingArmed() const { return bBreakOnLanding; }

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

	/** Moves the home spot (and its silhouette) by Delta, for props whose home is inside something that moves,
	 *  like a drawer: the drawer calls this as it slides. A placed (or sliding-in) prop moves with its home. */
	UFUNCTION(BlueprintCallable, Category = "Interactable|Cleanup")
	void ShiftHome(FVector Delta);

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
	virtual ELTTInteractKind GetInteractKind_Implementation() const override;
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override;

	/** True once placed if the prop has a ULTTLightSwitchComponent (e.g. BP_Prop_ModernLamp): Interact then
	 *  switches its light instead of picking it up. The light is off while the prop is out of place. */
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsSwitchableLamp() const;

	/** True once placed if the prop has a light switch or an appliance component (lamp, microwave): Interact uses
	 *  it instead of picking it up. */
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsUsableWhenPlaced() const;

	/** Set when EndHold is called with bThrown so the BP event can tell. */
	bool bLastReleaseWasThrow = false;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Set by ArmBreakOnLanding; cleared on pickup or when it breaks. */
	bool bBreakOnLanding = false;

	void FinishPlacing();
	/** Puts the prop at its home, kinematic and solid (shared by placing and breaking in place). */
	void LockAtHome();
	void ApplyBrokenLook();

	bool bBrokenInPlace = false;

	/** Shrinks into Target and hides (trash into a bin, dish into a sink); marks the prop done. */
	void StartVanishInto(const FVector& Target);

	/** Thrown away or washed: gone and counted as cleaned up. */
	bool bTrashed = false;
	bool bWashed = false;
	/** Picked up by the player at least once: only then does landing in a bin throw it away (not during the earthquake). */
	bool bEverHeld = false;
	bool bTrashing = false;
	float TrashElapsed = 0.f;
	FTransform TrashFrom;
	FVector TrashTarget = FVector::ZeroVector;

	bool bHasHome = false;
	bool bPlaced = false;
	bool bPlacing = false;
	float PlaceElapsed = 0.f;
	FTransform PlaceFrom;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ghost;

	/** Editor-only silhouette at Custom Home. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> CustomHomePreview;

	void UpdateCustomHomePreview();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	void ApplyPhysicsSettings();

	/** True if something is directly under the prop (table, floor, shelf). False for wall/ceiling-mounted props. */
	bool HasSupportBelow() const;

	/** Turns on physics for props touching this one, so things stacked on it fall when it's picked up. */
	void WakeTouchingProps();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Holder;

	/** Found at PostInitializeComponents (a Blueprint component; not named like this property). */
	UPROPERTY(Transient)
	TObjectPtr<ULTTLightSwitchComponent> CachedLightSwitch;

	/** Found at PostInitializeComponents (a Blueprint component: microwave, espresso machine...). */
	UPROPERTY(Transient)
	TObjectPtr<ULTTUsableComponent> CachedAppliance;

	FName SavedCollisionProfile;
};
