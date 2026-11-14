// Clean! Cut! Action! - A small appliance routine for a prop once it's placed (microwave, oven...):
// Use opens the door, closes it, plays the running sound for RunTime, then the done sound.
// ALTTInteractableBase makes a placed prop with this component usable (Interact = Use).

#pragma once

#include "CoreMinimal.h"
#include "Interaction/LTTUsableComponent.h"
#include "LTTApplianceComponent.generated.h"

class USceneComponent;
class USoundBase;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLTTOnApplianceFinished);

UENUM()
enum class ELTTAppliancePhase : uint8
{
	Idle,
	Opening,
	HoldOpen,
	Closing,
	Running
};

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTApplianceComponent : public ULTTUsableComponent
{
	GENERATED_BODY()

public:
	ULTTApplianceComponent();

	/** Name of the owner's component that swings open (the microwave's "Lid"). None = no door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	FName DoorComponentName = TEXT("Lid");

	/** How far the door swings open, relative to its closed rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	FRotator DoorOpenRotation = FRotator(0.f, 100.f, 0.f);

	/** Seconds to swing open (and again to swing shut). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance", meta = (ClampMin = "0.05"))
	float DoorTime = 0.35f;

	/** Seconds the door stays open before closing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance", meta = (ClampMin = "0"))
	float HoldOpenTime = 0.4f;

	/** Seconds it runs (the running sound loops this long). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance", meta = (ClampMin = "0"))
	float RunTime = 4.f;

	/** Plays (looping) while it runs, e.g. the microwave timer ticking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	TObjectPtr<USoundBase> RunningSound;

	/** Plays once when it's done, e.g. the ding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	TObjectPtr<USoundBase> DoneSound;

	/** Optional: plays when the door shuts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	TObjectPtr<USoundBase> DoorCloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	FText IdlePrompt = NSLOCTEXT("LTT", "ApplianceUse", "Use microwave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appliance")
	FText BusyPrompt = NSLOCTEXT("LTT", "ApplianceBusy", "Heating...");

	// ULTTUsableComponent
	virtual bool Use() override;
	virtual bool IsBusy() const override { return Phase != ELTTAppliancePhase::Idle; }
	virtual FText GetPrompt() const override { return IsBusy() ? BusyPrompt : IdlePrompt; }

	/** Fires after the done sound starts. */
	UPROPERTY(BlueprintAssignable, Category = "Appliance")
	FLTTOnApplianceFinished OnFinished;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetDoorAlpha(float Alpha);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Door;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RunningAudio;

	FRotator DoorClosedRotation = FRotator::ZeroRotator;
	ELTTAppliancePhase Phase = ELTTAppliancePhase::Idle;
	float PhaseTime = 0.f;
};
