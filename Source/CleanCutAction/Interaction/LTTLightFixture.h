// Clean! Cut! Action! - A fixed lamp (wall / ceiling) the player can switch on and off with Interact.
// Blueprints: /Game/_CleanCutAction/Blueprints/Interactables/BP_Lamp_* (one per lamp mesh).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractable.h"
#include "LTTLightFixture.generated.h"

class UStaticMeshComponent;
class ULTTLightSwitchComponent;

UCLASS(Blueprintable, meta = (DisplayName = "LTT Light Fixture"))
class CLEANCUTACTION_API ALTTLightFixture : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTLightFixture();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	TObjectPtr<UStaticMeshComponent> LampMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	TObjectPtr<ULTTLightSwitchComponent> LightSwitch;

	/** Turns the lamp on or off. */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void ToggleLight();

	/** Blueprint hook for a click sound etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lamp", meta = (DisplayName = "On Toggled"))
	void ReceiveToggled(bool bIsOn);

	// ILTTInteractable
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override { return true; }
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Press; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override { ToggleLight(); }
};
