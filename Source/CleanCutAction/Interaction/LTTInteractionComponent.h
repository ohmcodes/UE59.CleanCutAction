// Clean! Cut! Action! - Focus trace and interact/throw input for the player (Docs/02 §5).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTInteractionComponent.generated.h"

class UInputAction;
class ULTTCarryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnFocusChanged, AActor*, NewFocus, FText, PromptText);

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

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FLTTOnFocusChanged OnFocusChanged;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Pick up the focused prop, drop the held one, or use the focused interactable. */
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
	void SetFocus(AActor* NewFocus);
	ULTTCarryComponent* GetCarry() const;

	TWeakObjectPtr<AActor> FocusedActor;
	TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
