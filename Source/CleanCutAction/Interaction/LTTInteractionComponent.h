// Clean! Cut! Action! - Focus trace and interact/throw input for the player (Docs/02 §5).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTInteractionComponent.generated.h"

class UInputAction;
class ULTTCarryComponent;
class ALTTInteractableBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnFocusChanged, AActor*, NewFocus, FText, PromptText);

/** What pressing Interact does with the held prop. */
UENUM(BlueprintType)
enum class ELTTPlaceHint : uint8
{
	None,          // not holding anything
	Drop,          // not aiming at its home: drops it
	Place,         // aiming at its home: puts it back and locks it
	NeedsSupport,  // aiming at its home, but the prop under it (e.g. the table) isn't back yet: drops it
	Fumble,        // aim just misses its home: dropping it here breaks it
	Trash          // holding trash, aiming at a trash bin: throws it away
};

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTInteractionComponent();

	/** Pick up / drop / use (LMB, E). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Throw the held prop (RMB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> ThrowAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceRadius = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** How many placed props in a row the focus trace may pass through. */
	static constexpr int32 MaxPlacedPassThrough = 8;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FLTTOnFocusChanged OnFocusChanged;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Max distance from the eye to a home silhouette for placing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float PlaceDistance = 400.f;

	/** Dropping a cleanup prop while the aim only just misses its home silhouette breaks it (a fumbled placement). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Fumble")
	bool bBreakOnNearMissDrop = true;

	/** How far outside the snap radius still counts as a near miss (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Fumble", meta = (ClampMin = "0", EditCondition = "bBreakOnNearMissDrop"))
	float NearMissMargin = 35.f;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	ELTTPlaceHint GetPlaceHint() const { return PlaceHint; }

	/** HUD text for the held prop: "Place", "Drop", or "Put the Table back first". */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetPlaceHintText() const;

	/** Pick up the focused prop, place/drop the held one, or use the focused interactable. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	/** Throw the held prop. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ThrowHeld();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void TryBindInput();
	void UpdateFocus();
	void UpdatePlaceHint();

	ELTTPlaceHint PlaceHint = ELTTPlaceHint::None;
	void SetFocus(AActor* NewFocus);
	ULTTCarryComponent* GetCarry() const;

	TWeakObjectPtr<AActor> FocusedActor;

	/** Support prop whose home ghost is shown (orange) because the held prop needs it first. */
	TWeakObjectPtr<ALTTInteractableBase> ShownSupportGhost;

	/** The bin the held trash is aimed at (PlaceHint Trash). */
	TWeakObjectPtr<ALTTInteractableBase> TargetBin;
	void SetSupportGhost(ALTTInteractableBase* Support);
	TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
