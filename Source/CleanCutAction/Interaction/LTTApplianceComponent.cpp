// Clean! Cut! Action!

#include "Interaction/LTTApplianceComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"

ULTTApplianceComponent::ULTTApplianceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // ticks only while it runs
}

void ULTTApplianceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner(); Owner && !DoorComponentName.IsNone())
	{
		TArray<USceneComponent*> Scene;
		Owner->GetComponents(Scene);
		for (USceneComponent* C : Scene)
		{
			if (C && C->GetFName() == DoorComponentName)
			{
				Door = C;
				break;
			}
		}
	}
	if (Door)
	{
		Door->SetMobility(EComponentMobility::Movable);
		DoorClosedRotation = Door->GetRelativeRotation();
	}
}

void ULTTApplianceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RunningAudio)
	{
		RunningAudio->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

bool ULTTApplianceComponent::Use()
{
	if (IsBusy())
	{
		return false;
	}
	Phase = Door ? ELTTAppliancePhase::Opening : ELTTAppliancePhase::Running;
	PhaseTime = 0.f;
	if (Phase == ELTTAppliancePhase::Running)
	{
		RunningAudio = PlayOwnerSound(RunningSound, true);
	}
	SetComponentTickEnabled(true);
	return true;
}

void ULTTApplianceComponent::SetDoorAlpha(float Alpha)
{
	if (Door)
	{
		const float E = Alpha * Alpha * (3.f - 2.f * Alpha); // smoothstep
		Door->SetRelativeRotation(DoorClosedRotation + DoorOpenRotation * E);
	}
}

void ULTTApplianceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PhaseTime += DeltaTime;

	switch (Phase)
	{
	case ELTTAppliancePhase::Opening:
		SetDoorAlpha(FMath::Clamp(PhaseTime / DoorTime, 0.f, 1.f));
		if (PhaseTime >= DoorTime)
		{
			Phase = ELTTAppliancePhase::HoldOpen;
			PhaseTime = 0.f;
		}
		break;

	case ELTTAppliancePhase::HoldOpen:
		if (PhaseTime >= HoldOpenTime)
		{
			Phase = ELTTAppliancePhase::Closing;
			PhaseTime = 0.f;
		}
		break;

	case ELTTAppliancePhase::Closing:
		SetDoorAlpha(1.f - FMath::Clamp(PhaseTime / DoorTime, 0.f, 1.f));
		if (PhaseTime >= DoorTime)
		{
			SetDoorAlpha(0.f);
			PlayOwnerSound(DoorCloseSound, false);
			RunningAudio = PlayOwnerSound(RunningSound, true);
			Phase = ELTTAppliancePhase::Running;
			PhaseTime = 0.f;
		}
		break;

	case ELTTAppliancePhase::Running:
		if (PhaseTime >= RunTime)
		{
			if (RunningAudio)
			{
				RunningAudio->Stop();
				RunningAudio->DestroyComponent();
				RunningAudio = nullptr;
			}
			PlayOwnerSound(DoneSound, false);
			Phase = ELTTAppliancePhase::Idle;
			SetComponentTickEnabled(false);
			OnFinished.Broadcast();
		}
		break;

	default:
		SetComponentTickEnabled(false);
		break;
	}
}
