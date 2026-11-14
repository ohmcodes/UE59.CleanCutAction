// Clean! Cut! Action! - Mains power for the level (the electric box): off = every local light (point / spot / rect)
// goes dark and every lamp's bulb dims; on = everything back the way it was. Lamp switches flipped during an outage
// are remembered and take effect when the power returns. Sky and directional lights are never touched.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LTTPowerSubsystem.generated.h"

class ULightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnPowerChanged, bool, bPowerOn);

UCLASS()
class CLEANCUTACTION_API ULTTPowerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Power")
	void SetPowerOn(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Power")
	bool IsPowerOn() const { return bPowerOn; }

	/** Power state of WorldContext's world (true if there's no world / subsystem). */
	static bool IsPowerOnIn(const UObject* WorldContext);

	UPROPERTY(BlueprintAssignable, Category = "Power")
	FLTTOnPowerChanged OnPowerChanged;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	bool bPowerOn = true;

	/** Visibility of each local light when the power went off, restored when it comes back. */
	TMap<TWeakObjectPtr<ULightComponent>, bool> SavedVisibility;
};
