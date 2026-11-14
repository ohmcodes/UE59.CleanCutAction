// Clean! Cut! Action!

#include "Interaction/LTTPowerBox.h"
#include "Interaction/LTTPowerSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "LTTPowerBox"

ALTTPowerBox::ALTTPowerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetMobility(EComponentMobility::Static);
	BoxMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = BoxMesh;

	IndicatorDot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndicatorDot"));
	IndicatorDot->SetupAttachment(BoxMesh);
	IndicatorDot->SetMobility(EComponentMobility::Movable);
	IndicatorDot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IndicatorDot->SetCastShadow(false);

	IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
	IndicatorLight->SetupAttachment(BoxMesh);
	IndicatorLight->SetMobility(EComponentMobility::Movable);
	IndicatorLight->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	IndicatorLight->SetIntensityUnits(ELightUnits::Candelas);
	IndicatorLight->SetIntensity(1.5f);
	IndicatorLight->SetAttenuationRadius(90.f);
	IndicatorLight->SetCastShadows(false);

	Sparks = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Sparks"));
	Sparks->SetupAttachment(BoxMesh);
	Sparks->SetAutoActivate(false);
}

bool ALTTPowerBox::IsPowerOn() const
{
	return ULTTPowerSubsystem::IsPowerOnIn(this);
}

void ALTTPowerBox::TogglePower()
{
	UWorld* World = GetWorld();
	ULTTPowerSubsystem* Power = World ? World->GetSubsystem<ULTTPowerSubsystem>() : nullptr;
	if (!Power || World->GetTimeSeconds() < ReadyTime)
	{
		return;
	}
	ReadyTime = World->GetTimeSeconds() + Cooldown;
	Power->SetPowerOn(!Power->IsPowerOn());

	Sparks->Activate(true);
	PlayBoxSound(ClickSound);
	PlayBoxSound(SparkSound);
	ReceivePowerSwitched(Power->IsPowerOn());
}

void ALTTPowerBox::PlayBoxSound(USoundBase* Sound)
{
	if (!Sound)
	{
		return;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(this);
	Audio->SetupAttachment(BoxMesh);
	Audio->SetRelativeLocation(Sparks->GetRelativeLocation());
	Audio->SetSound(Sound);
	Audio->SetVolumeMultiplier(Volume);
	Audio->bAutoDestroy = true;
	Audio->bAllowSpatialization = true;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bAttenuate = true;
	Audio->AttenuationOverrides.bSpatialize = true;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
	Audio->AttenuationOverrides.FalloffDistance = SoundFalloff;
	Audio->RegisterComponent();
	Audio->Play();
}

FText ALTTPowerBox::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	return IsPowerOn() ? LOCTEXT("PowerOff", "Turn power off") : LOCTEXT("PowerOn", "Turn power on");
}

void ALTTPowerBox::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	// Stencil 1 = focus outline, same as the props.
	BoxMesh->SetRenderCustomDepth(true);
	BoxMesh->SetCustomDepthStencilValue(1);
}

void ALTTPowerBox::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	BoxMesh->SetRenderCustomDepth(false);
}

#undef LOCTEXT_NAMESPACE
