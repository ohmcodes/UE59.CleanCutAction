// Clean! Cut! Action!

#include "Interaction/LTTPowerSubsystem.h"
#include "Interaction/LTTLightSwitchComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/Light.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"

bool ULTTPowerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool ULTTPowerSubsystem::IsPowerOnIn(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const ULTTPowerSubsystem* Power = World ? World->GetSubsystem<ULTTPowerSubsystem>() : nullptr;
	return !Power || Power->IsPowerOn();
}

void ULTTPowerSubsystem::SetPowerOn(bool bOn)
{
	if (bOn == bPowerOn)
	{
		return;
	}
	bPowerOn = bOn;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!bOn)
	{
		// Remember every local light, then switch them all off.
		SavedVisibility.Reset();
		for (TActorIterator<ALight> It(World); It; ++It)
		{
			ULocalLightComponent* L = Cast<ULocalLightComponent>(It->GetLightComponent());
			if (L)
			{
				SavedVisibility.Add(L, L->IsVisible());
				L->SetVisibility(false);
			}
		}
	}
	else
	{
		for (const TPair<TWeakObjectPtr<ULightComponent>, bool>& Saved : SavedVisibility)
		{
			if (ULightComponent* L = Saved.Key.Get())
			{
				L->SetVisibility(Saved.Value);
			}
		}
		SavedVisibility.Reset();
	}

	// Lamps: dim / restore their bulbs, and let their switch decide their own light.
	for (TObjectIterator<ULTTLightSwitchComponent> It; It; ++It)
	{
		if (It->GetWorld() == World)
		{
			It->ApplyPower();
		}
	}
	OnPowerChanged.Broadcast(bPowerOn);
}
