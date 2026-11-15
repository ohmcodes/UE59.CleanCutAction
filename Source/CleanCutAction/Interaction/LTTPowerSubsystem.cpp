// Clean! Cut! Action!

#include "Interaction/LTTPowerSubsystem.h"
#include "Interaction/LTTLightSwitchComponent.h"
#include "Interaction/LTTPowerBox.h"
#include "Components/LightComponentBase.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
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
		// A full blackout: every light in the world (lamps, the sun, the sky light) and the sky, fog and clouds, so the
		// set is pitch black. Only the power box's own red indicator stays on. Remember each one to restore it later.
		SavedVisibility.Reset();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->IsA<ALTTPowerBox>())
			{
				continue;
			}
			for (UActorComponent* Comp : It->GetComponents())
			{
				USceneComponent* Scene = Cast<USceneComponent>(Comp);
				if (Scene && (Scene->IsA<ULightComponentBase>() || Scene->IsA<USkyAtmosphereComponent>()
					|| Scene->IsA<UExponentialHeightFogComponent>() || Scene->IsA<UVolumetricCloudComponent>()))
				{
					SavedVisibility.Add(Scene, Scene->IsVisible());
					Scene->SetVisibility(false);
				}
			}
		}
	}
	else
	{
		for (const TPair<TWeakObjectPtr<USceneComponent>, bool>& Saved : SavedVisibility)
		{
			if (USceneComponent* Scene = Saved.Key.Get())
			{
				Scene->SetVisibility(Saved.Value);
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
