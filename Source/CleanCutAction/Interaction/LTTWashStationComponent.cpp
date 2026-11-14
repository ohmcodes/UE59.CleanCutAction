// Clean! Cut! Action!

#include "Interaction/LTTWashStationComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectIterator.h"

ULTTWashStationComponent::ULTTWashStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULTTWashStationComponent::PlayWashEffects()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	TArray<UNiagaraComponent*> Niagaras;
	Owner->GetComponents(Niagaras);
	for (UNiagaraComponent* N : Niagaras)
	{
		if (N && (BubblesComponentName.IsNone() || N->GetFName() == BubblesComponentName))
		{
			N->Activate(true);
		}
	}
	if (WashSound)
	{
		UAudioComponent* Audio = NewObject<UAudioComponent>(Owner);
		Audio->SetupAttachment(this);
		Audio->SetSound(WashSound);
		Audio->SetVolumeMultiplier(WashVolume);
		Audio->bAutoDestroy = true;
		Audio->bAllowSpatialization = true;
		Audio->bOverrideAttenuation = true;
		Audio->AttenuationOverrides.bAttenuate = true;
		Audio->AttenuationOverrides.bSpatialize = true;
		Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
		Audio->AttenuationOverrides.FalloffDistance = 900.f;
		Audio->RegisterComponent();
		Audio->Play();
	}
}

ULTTWashStationComponent* ULTTWashStationComponent::FindNearest(const UObject* WorldContext, const FVector& From)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	ULTTWashStationComponent* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (TObjectIterator<ULTTWashStationComponent> It; It; ++It)
	{
		if (It->GetWorld() != World || !It->IsRegistered())
		{
			continue;
		}
		const double D = FVector::DistSquared(It->GetComponentLocation(), From);
		if (D < BestDistSq)
		{
			BestDistSq = D;
			Best = *It;
		}
	}
	return Best;
}

ULTTWashStationComponent* ULTTWashStationComponent::FindOn(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<ULTTWashStationComponent>() : nullptr;
}
