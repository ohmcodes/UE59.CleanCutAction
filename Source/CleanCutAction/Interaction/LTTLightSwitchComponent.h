// Clean! Cut! Action! - Turns level lights (and the owner's glowing bulb) on and off.
// Used by ALTTLightFixture (wall / ceiling lamps) and by pickup props such as BP_Prop_ModernLamp
// (ALTTInteractableBase makes a placed prop with this component switchable).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTLightSwitchComponent.generated.h"

class ALight;
class UMeshComponent;
class UMaterialInstanceDynamic;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnLightToggled, bool, bIsOn);

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTLightSwitchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTLightSwitchComponent();

	/** Level lights this switch controls (they must be Movable). Empty = the nearest light within AutoFindRadius
	 *  of the owner's mesh, found at BeginPlay (before anything is moved). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Light Switch")
	TArray<TObjectPtr<ALight>> ControlledLights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch", meta = (ClampMin = "0"))
	float AutoFindRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch")
	bool bStartsOn = true;

	/** Scalar parameters on the owner's mesh materials that make the bulb glow. Set to OffEmissiveScale x their
	 *  value while off (parameters a material doesn't have are ignored). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch")
	TArray<FName> EmissiveParameters = { TEXT("EmissiveBright"), TEXT("Emissive Intensity"), TEXT("Emissive Tint Intensity") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch", meta = (ClampMin = "0", ClampMax = "1"))
	float OffEmissiveScale = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Light Switch")
	void SetLightOn(bool bOn);

	/** The player flips the switch: toggles the light and plays ToggleSound. */
	UFUNCTION(BlueprintCallable, Category = "Light Switch")
	void Toggle();

	/** Click played at the lamp when the player switches it (not on start-up, placing or power cuts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch")
	TObjectPtr<USoundBase> ToggleSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Switch", meta = (ClampMin = "0"))
	float ToggleVolume = 0.8f;

	UFUNCTION(BlueprintPure, Category = "Light Switch")
	bool IsLightOn() const { return bLightOn; }

	/** Fires after every change (for click sounds etc.). */
	UPROPERTY(BlueprintAssignable, Category = "Light Switch")
	FLTTOnLightToggled OnLightToggled;

	/** "Turn off light" / "Turn on light". */
	FText GetToggleText() const;

	/** Re-applies the switch with the current mains power (ULTTPowerSubsystem): lit = switched on AND power on. */
	void ApplyPower();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ResolveLights();
	void CacheEmissive();
	UMeshComponent* GetBulbMesh() const;

	/** What the switch is set to (kept through power cuts). */
	bool bLightOn = true;
	/** What's actually showing: bLightOn && mains power. The level starts with lights and bulbs on. */
	bool bShownOn = true;
	bool bEmissiveCached = false;

	void ApplyShown();

	struct FEmissiveValue
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> MID;
		FName Name;
		float OnValue = 0.f;
	};
	TArray<FEmissiveValue> EmissiveValues;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BulbMIDs;
};
