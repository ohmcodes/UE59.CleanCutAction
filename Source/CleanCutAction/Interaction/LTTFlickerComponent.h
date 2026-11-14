// Clean! Cut! Action! - Subtle LED / neon flicker: the owner's glowing material and nearby lights mostly hold steady
// with a faint shimmer, and every few seconds dip briefly a few times. Used by BP_ShopSign_02_LED.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTFlickerComponent.generated.h"

class ALight;
class UMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTFlickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTFlickerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	bool bFlickerEnabled = true;

	/** Scalar parameters on the owner's mesh materials that make it glow (scaled by the flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	TArray<FName> EmissiveParameters = { TEXT("Emissive Intensity") };

	/** Lights that flicker with the glow (must be Movable). Empty = the nearest light within AutoFindRadius. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Flicker")
	TArray<TObjectPtr<ALight>> FlickerLights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker", meta = (ClampMin = "0"))
	float AutoFindRadius = 150.f;

	/** Constant faint wobble (0.03 = +-3%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker", meta = (ClampMin = "0", ClampMax = "0.5"))
	float Shimmer = 0.03f;

	/** Seconds between flicker bursts (random in range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FVector2D BurstInterval = FVector2D(2.5, 7.0);

	/** Quick dips per burst (random in range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FIntPoint DipsPerBurst = FIntPoint(1, 3);

	/** Brightness during a dip, as a fraction of normal (random in range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FVector2D DipBrightness = FVector2D(0.55, 0.85);

	/** How long each dip lasts, seconds (random in range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FVector2D DipDuration = FVector2D(0.04, 0.1);

	/** Gap between dips inside a burst, seconds (random in range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FVector2D DipGap = FVector2D(0.05, 0.15);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Apply(float Brightness);
	float RandIn(const FVector2D& Range) const { return FMath::FRandRange(FMath::Min(Range.X, Range.Y), FMath::Max(Range.X, Range.Y)); }

	struct FGlowParam
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> MID;
		FName Name;
		float BaseValue = 0.f;
	};
	TArray<FGlowParam> GlowParams;
	TArray<float> BaseLightIntensity;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowMIDs;

	float Clock = 0.f;
	float NextEventTime = 0.f;
	int32 DipsLeft = 0;
	bool bInDip = false;
	float DipLevel = 1.f;
	float NoiseSeed = 0.f;
};
