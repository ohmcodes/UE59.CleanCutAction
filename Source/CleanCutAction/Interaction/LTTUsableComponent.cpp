// Clean! Cut! Action!

#include "Interaction/LTTUsableComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"

UAudioComponent* ULTTUsableComponent::PlayOwnerSound(USoundBase* Sound, bool bKeep)
{
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Sound || !Root)
	{
		return nullptr;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(Owner);
	Audio->SetupAttachment(Root);
	Audio->SetSound(Sound);
	Audio->SetVolumeMultiplier(Volume);
	Audio->bAutoDestroy = !bKeep;
	Audio->bAllowSpatialization = true;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bAttenuate = true;
	Audio->AttenuationOverrides.bSpatialize = true;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
	Audio->AttenuationOverrides.FalloffDistance = SoundFalloff;
	Audio->RegisterComponent();
	Audio->Play();
	return Audio;
}
