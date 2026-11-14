// Clean! Cut! Action! - Dev button that runs the LTT.PlaceAll cheat

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractable.h"
#include "LTTCheatButton.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;

/** A small button to drop in the level for testing: press it (Interact) or step on it to put every cleanup prop
 *  back except LeaveCount, same as the LTT.PlaceAll console command and the iPoNet coffee cup in the pause menu. */
UCLASS(Blueprintable, meta = (DisplayName = "LTT Cheat Button (Place All)"))
class CLEANCUTACTION_API ALTTCheatButton : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTCheatButton();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cheat")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cheat")
	TObjectPtr<UStaticMeshComponent> Cap;

	/** Walk-on trigger (used when bTriggerOnStep). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cheat")
	TObjectPtr<UBoxComponent> StepTrigger;

	/** Props left for the player to place by hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat", meta = (ClampMin = "0"))
	int32 LeaveCount = 5;

	/** Also fire when the player walks onto the button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat")
	bool bTriggerOnStep = false;

	/** Off = the button removes itself in Shipping builds, so a leftover button can't ship to players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat")
	bool bAllowInShipping = true;

	/** Seconds before it can be pressed again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat", meta = (ClampMin = "0"))
	float Cooldown = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat")
	FLinearColor ButtonColor = FLinearColor(0.9f, 0.08f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cheat")
	FLinearColor PressedColor = FLinearColor(0.1f, 0.8f, 0.15f);

	/** Runs the cheat. Returns how many props were placed (-1 = no mess director / cooling down). */
	UFUNCTION(BlueprintCallable, Category = "Cheat")
	int32 Press();

	/** Blueprint hook for a click sound etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cheat", meta = (DisplayName = "On Pressed"))
	void ReceivePressed(int32 PlacedCount);

	// ILTTInteractable
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Press; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleStepOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetCapColor(const FLinearColor& Color);
	void ReleaseCap();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CapMaterial;

	float ReadyTime = 0.f;
	FVector CapRestLocation = FVector::ZeroVector;
	FTimerHandle ReleaseTimer;
};
