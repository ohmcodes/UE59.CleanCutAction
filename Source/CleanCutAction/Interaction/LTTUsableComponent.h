// Clean! Cut! Action! - Base for components that make a placed prop usable (Interact = Use instead of pick up):
// ULTTApplianceComponent (microwave), ULTTCoffeeMachineComponent (espresso machine), ...
// ALTTInteractableBase finds the first one on the prop.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTUsableComponent.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS(Abstract, ClassGroup = (LTT))
class CLEANCUTACTION_API ULTTUsableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Starts using it. Returns false if it can't start (already running). */
	UFUNCTION(BlueprintCallable, Category = "Usable")
	virtual bool Use() { return false; }

	UFUNCTION(BlueprintPure, Category = "Usable")
	virtual bool IsBusy() const { return false; }

	/** HUD prompt while the crosshair is on the prop. */
	UFUNCTION(BlueprintPure, Category = "Usable")
	virtual FText GetPrompt() const { return NSLOCTEXT("LTT", "UsableUse", "Use"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usable|Sound", meta = (ClampMin = "0"))
	float Volume = 0.8f;

	/** Sounds fade out with distance; silent beyond this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usable|Sound", meta = (ClampMin = "100"))
	float SoundFalloff = 1200.f;

protected:
	/** Plays Sound in 3D at the owner. bKeep = keep (and return) the audio component, e.g. for a loop to stop later. */
	UAudioComponent* PlayOwnerSound(USoundBase* Sound, bool bKeep);
};
