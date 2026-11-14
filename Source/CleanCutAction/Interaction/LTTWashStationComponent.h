// Clean! Cut! Action! - Marks a sink basin: props with bIsDishwashable that are put (or dropped) here are washed:
// they shrink into the basin and disappear (counted as cleaned up), with the owner's bubbles and a wash sound.
// Put it on any sink Blueprint at the basin (BP_Sink has one). Scales to any number of dishes and sinks.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LTTWashStationComponent.generated.h"

class USoundBase;
class ALTTInteractableBase;

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTWashStationComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULTTWashStationComponent();

	/** How close (cm) the aim must pass to the basin to "Wash in the sink". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wash Station", meta = (ClampMin = "5"))
	float AimRadius = 30.f;

	/** Name of the owner's Niagara component fired on every wash (BP_Sink: "Bubbles"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wash Station")
	FName BubblesComponentName = TEXT("Bubbles");

	/** Played once per wash at the basin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wash Station")
	TObjectPtr<USoundBase> WashSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wash Station", meta = (ClampMin = "0"))
	float WashVolume = 0.7f;

	/** Called when a dish goes in: bubbles + sound. */
	UFUNCTION(BlueprintCallable, Category = "Wash Station")
	void PlayWashEffects();

	/** The wash station nearest to From (null if none). */
	static ULTTWashStationComponent* FindNearest(const UObject* WorldContext, const FVector& From);

	/** True if Actor owns a wash station (e.g. a dish hit the sink). */
	static ULTTWashStationComponent* FindOn(const AActor* Actor);
};
