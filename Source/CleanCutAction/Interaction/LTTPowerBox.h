// Clean! Cut! Action! - The electric box: Interact turns the mains power off / on (ULTTPowerSubsystem), with a spark
// burst, a relay click and a crackle. A small red indicator light stays on either way, so the box can be found
// in the dark. Blueprint: /Game/_CleanCutAction/Blueprints/Interactables/BP_ElectricBox.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractable.h"
#include "LTTPowerBox.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;
class USoundBase;

UCLASS(Blueprintable, meta = (DisplayName = "LTT Power Box"))
class CLEANCUTACTION_API ALTTPowerBox : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTPowerBox();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Box")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	/** Small glowing red dot (always on). Give it an unlit red material. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Box")
	TObjectPtr<UStaticMeshComponent> IndicatorDot;

	/** Faint red light around the dot (always on, not part of the mains). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Box")
	TObjectPtr<UPointLightComponent> IndicatorLight;

	/** Spark burst fired on every switch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Box")
	TObjectPtr<UNiagaraComponent> Sparks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Box|Sound")
	TObjectPtr<USoundBase> ClickSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Box|Sound")
	TObjectPtr<USoundBase> SparkSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Box|Sound", meta = (ClampMin = "0"))
	float Volume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Box|Sound", meta = (ClampMin = "100"))
	float SoundFalloff = 1500.f;

	/** Seconds before it can be switched again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Box", meta = (ClampMin = "0"))
	float Cooldown = 0.6f;

	/** Turns the mains power off / on. */
	UFUNCTION(BlueprintCallable, Category = "Power Box")
	void TogglePower();

	UFUNCTION(BlueprintPure, Category = "Power Box")
	bool IsPowerOn() const;

	/** Blueprint hook for extra effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Power Box", meta = (DisplayName = "On Power Switched"))
	void ReceivePowerSwitched(bool bPowerOn);

	// ILTTInteractable
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override { return true; }
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Press; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override { TogglePower(); }

protected:
	void PlayBoxSound(USoundBase* Sound);

	float ReadyTime = 0.f;
};
