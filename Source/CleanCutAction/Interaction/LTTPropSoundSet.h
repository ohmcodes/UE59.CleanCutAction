// Clean! Cut! Action! - Generic prop sounds by material (wood, metal, paper...): pick up, place, break.
// One asset (/Game/_CleanCutAction/SFX/Props/DA_PropSounds) holds them all; each prop picks its SoundMaterial.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LTTPropSoundSet.generated.h"

class USoundBase;
class USoundAttenuation;

UENUM(BlueprintType)
enum class ELTTPropMaterial : uint8
{
	None,      // silent
	Wood,
	Metal,
	Paper,     // paper, cardboard, books
	Ceramic,   // cups, plates, pots
	Glass,
	Plastic,
	Fabric,
	Food
};

UENUM(BlueprintType)
enum class ELTTPropSoundEvent : uint8
{
	Pickup,
	Place,
	Break
};

USTRUCT(BlueprintType)
struct FLTTPropSounds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop Sounds")
	TObjectPtr<USoundBase> Pickup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop Sounds")
	TObjectPtr<USoundBase> Place;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop Sounds")
	TObjectPtr<USoundBase> Break;
};

UCLASS(BlueprintType)
class CLEANCUTACTION_API ULTTPropSoundSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Sounds")
	TMap<ELTTPropMaterial, FLTTPropSounds> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Sounds", meta = (ClampMin = "0"))
	float Volume = 0.8f;

	/** Break sounds play this much louder (a smash should cut through). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Sounds", meta = (ClampMin = "0"))
	float BreakVolumeScale = 1.2f;

	/** Each play picks a pitch in this range, so repeats don't sound identical. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Sounds")
	FVector2D PitchRange = FVector2D(0.92, 1.08);

	/** 3D falloff for every prop sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Sounds")
	TObjectPtr<USoundAttenuation> Attenuation;

	USoundBase* GetSound(ELTTPropMaterial Material, ELTTPropSoundEvent Event) const;
};
