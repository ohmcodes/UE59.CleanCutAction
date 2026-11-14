// Clean! Cut! Action!

#include "Interaction/LTTLightSwitchComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Interaction/LTTPowerSubsystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LTTLightSwitch"

ULTTLightSwitchComponent::ULTTLightSwitchComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Cooked through a +MapsToCook entry in Config/DefaultGame.ini (a C++ default alone isn't seen by the cooker).
	static ConstructorHelpers::FObjectFinder<USoundBase> Click(TEXT("/Game/_CleanCutAction/SFX/SW_LightClick.SW_LightClick"));
	if (Click.Succeeded())
	{
		ToggleSound = Click.Object;
	}
}

void ULTTLightSwitchComponent::Toggle()
{
	SetLightOn(!bLightOn);
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!ToggleSound || !Root)
	{
		return;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(Owner);
	Audio->SetupAttachment(Root);
	Audio->SetSound(ToggleSound);
	Audio->SetVolumeMultiplier(ToggleVolume);
	Audio->bAutoDestroy = true;
	Audio->bAllowSpatialization = true;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bAttenuate = true;
	Audio->AttenuationOverrides.bSpatialize = true;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(100.f, 0.f, 0.f);
	Audio->AttenuationOverrides.FalloffDistance = 900.f;
	Audio->RegisterComponent();
	Audio->Play();
}

void ULTTLightSwitchComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveLights();
	bLightOn = bStartsOn;
	bShownOn = true; // lights and bulbs are on in the level as authored
	ApplyShown();
}

void ULTTLightSwitchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A smashed lamp shouldn't leave its light glowing in mid-air.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		SetLightOn(false);
	}
	Super::EndPlay(EndPlayReason);
}

UMeshComponent* ULTTLightSwitchComponent::GetBulbMesh() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	if (UMeshComponent* Root = Cast<UMeshComponent>(Owner->GetRootComponent()))
	{
		return Root;
	}
	return Owner->FindComponentByClass<UMeshComponent>();
}

void ULTTLightSwitchComponent::ResolveLights()
{
	ControlledLights.RemoveAll([](const TObjectPtr<ALight>& L) { return !IsValid(L); });
	UWorld* World = GetWorld();
	if (!ControlledLights.IsEmpty() || !World || AutoFindRadius <= 0.f)
	{
		return;
	}
	const UMeshComponent* Bulb = GetBulbMesh();
	const FVector Center = Bulb ? Bulb->Bounds.Origin : GetOwner()->GetActorLocation();
	ALight* Best = nullptr;
	double BestDistSq = FMath::Square(AutoFindRadius);
	for (TActorIterator<ALight> It(World); It; ++It)
	{
		const double DistSq = FVector::DistSquared(It->GetActorLocation(), Center);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	if (Best)
	{
		ControlledLights.Add(Best);
	}
}

void ULTTLightSwitchComponent::CacheEmissive()
{
	bEmissiveCached = true;
	UMeshComponent* Bulb = GetBulbMesh();
	if (!Bulb || EmissiveParameters.IsEmpty())
	{
		return;
	}
	for (int32 i = 0; i < Bulb->GetNumMaterials(); ++i)
	{
		UMaterialInterface* Mat = Bulb->GetMaterial(i);
		if (!Mat)
		{
			continue;
		}
		// Only make a dynamic instance for slots that actually glow.
		TArray<TPair<FName, float>> Found;
		for (const FName& Param : EmissiveParameters)
		{
			float Value = 0.f;
			if (Mat->GetScalarParameterValue(FHashedMaterialParameterInfo(Param), Value) && Value > 0.f)
			{
				Found.Emplace(Param, Value);
			}
		}
		if (Found.IsEmpty())
		{
			continue;
		}
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat);
		if (!MID)
		{
			MID = Bulb->CreateAndSetMaterialInstanceDynamic(i);
		}
		if (!MID)
		{
			continue;
		}
		BulbMIDs.Add(MID);
		for (const TPair<FName, float>& P : Found)
		{
			EmissiveValues.Add({ MID, P.Key, P.Value });
		}
	}
}

void ULTTLightSwitchComponent::SetLightOn(bool bOn)
{
	if (bOn == bLightOn)
	{
		return;
	}
	bLightOn = bOn;
	ApplyShown();
	OnLightToggled.Broadcast(bLightOn);
}

void ULTTLightSwitchComponent::ApplyPower()
{
	ApplyShown();
}

void ULTTLightSwitchComponent::ApplyShown()
{
	// A switch flipped during a power cut is remembered; the light only shows once the power is back.
	const bool bShow = bLightOn && ULTTPowerSubsystem::IsPowerOnIn(this);

	for (ALight* Light : ControlledLights)
	{
		if (IsValid(Light) && Light->GetLightComponent())
		{
			Light->GetLightComponent()->SetVisibility(bShow);
		}
	}

	if (bShow == bShownOn)
	{
		return;
	}
	bShownOn = bShow;
	if (!bEmissiveCached)
	{
		if (bShow)
		{
			return; // never dimmed yet: the materials are already in their "on" state
		}
		CacheEmissive();
	}
	for (const FEmissiveValue& V : EmissiveValues)
	{
		if (UMaterialInstanceDynamic* MID = V.MID.Get())
		{
			MID->SetScalarParameterValue(V.Name, bShow ? V.OnValue : V.OnValue * OffEmissiveScale);
		}
	}
}

FText ULTTLightSwitchComponent::GetToggleText() const
{
	return bLightOn ? LOCTEXT("TurnOff", "Turn off light") : LOCTEXT("TurnOn", "Turn on light");
}

#undef LOCTEXT_NAMESPACE
