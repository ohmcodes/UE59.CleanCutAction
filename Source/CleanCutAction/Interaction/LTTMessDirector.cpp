// Clean! Cut! Action!

#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTCleanupHUD.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

ALTTMessDirector::ALTTMessDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	HUDClass = ULTTCleanupHUD::StaticClass();
}

ALTTMessDirector* ALTTMessDirector::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ALTTMessDirector> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ALTTMessDirector::BeginPlay()
{
	Super::BeginPlay();
	CreateHUD();
	// Every prop records its home in its own BeginPlay; wait one tick so all of them have.
	GetWorldTimerManager().SetTimerForNextTick(this, &ALTTMessDirector::SetupTasks);
}

void ALTTMessDirector::SetupTasks()
{
	CreateHUD(); // in case the player controller didn't exist yet at BeginPlay
	TArray<ALTTInteractableBase*> All;
	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		ALTTInteractableBase* Prop = *It;
		if (IsValid(Prop) && Prop->Mesh)
		{
			Prop->RecordHome();
			All.Add(Prop);
		}
	}

	// Supports are found while everything is still at home.
	for (ALTTInteractableBase* Prop : All)
	{
		if (Prop->bIsCleanupTask)
		{
			Prop->FindHomeSupport();
			Tasks.Add(Prop);
			Prop->OnDestroyed.AddDynamic(this, &ALTTMessDirector::HandlePropDestroyed);
		}
	}

	for (ALTTInteractableBase* Prop : All)
	{
		if (Prop->bIsCleanupTask && Prop->bStartsAtHome)
		{
			Prop->SnapToHome(); // calls NotifyPlaced
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MessDirector: %d cleanup props."), Tasks.Num());
	OnProgressChanged.Broadcast(PlacedCount, Tasks.Num());

	if (bEarthquakeOnStart)
	{
		if (EarthquakeDelay > 0.f)
		{
			GetWorldTimerManager().SetTimer(EarthquakeTimer, this, &ALTTMessDirector::TriggerEarthquake, EarthquakeDelay, false);
		}
		else
		{
			TriggerEarthquake();
		}
	}
	else
	{
		bMessStarted = true;
		if (bUseTimer)
		{
			StartTimer();
		}
	}
}

void ALTTMessDirector::CreateHUD()
{
	if (!bCreateHUD || !HUDClass || HUDWidget)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->IsLocalController())
	{
		HUDWidget = CreateWidget<UUserWidget>(PC, HUDClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(1);
		}
	}
}

// ---- Countdown ----

void ALTTMessDirector::StartTimer(float Seconds)
{
	if (bAllPlaced)
	{
		return;
	}
	const float Length = Seconds > 0.f ? Seconds : TimeLimitSeconds;
	bTimeUp = false;
	StoppedRemaining = -1.f;
	GetWorldTimerManager().SetTimer(CountdownTimer, this, &ALTTMessDirector::HandleTimeUp, FMath::Max(Length, 0.01f), false);
}

void ALTTMessDirector::StopTimer()
{
	if (IsTimerRunning())
	{
		StoppedRemaining = GetWorldTimerManager().GetTimerRemaining(CountdownTimer);
	}
	GetWorldTimerManager().ClearTimer(CountdownTimer);
}

void ALTTMessDirector::SetTimerEnabled(bool bEnabled)
{
	bUseTimer = bEnabled;
	if (!bEnabled)
	{
		StopTimer();
	}
	else if (bMessStarted && !IsTimerRunning() && !bTimeUp)
	{
		StartTimer(StoppedRemaining); // resumes where it stopped
	}
}

void ALTTMessDirector::AddTime(float Seconds)
{
	if (bTimeUp)
	{
		return;
	}
	const float NewRemaining = FMath::Max(GetTimeRemaining() + Seconds, 0.01f);
	if (IsTimerRunning())
	{
		StartTimer(NewRemaining);
	}
	else
	{
		StoppedRemaining = NewRemaining;
	}
}

bool ALTTMessDirector::IsTimerRunning() const
{
	return GetWorldTimerManager().IsTimerActive(CountdownTimer);
}

float ALTTMessDirector::GetTimeRemaining() const
{
	if (bTimeUp)
	{
		return 0.f;
	}
	if (IsTimerRunning())
	{
		return GetWorldTimerManager().GetTimerRemaining(CountdownTimer);
	}
	return StoppedRemaining >= 0.f ? StoppedRemaining : TimeLimitSeconds;
}

void ALTTMessDirector::HandleTimeUp()
{
	bTimeUp = true;
	UE_LOG(LogTemp, Log, TEXT("MessDirector: time up (%d/%d placed, %d broken, %d points)."), PlacedCount, Tasks.Num(), BrokenCount, GetScore());
	OnTimeUp.Broadcast();
	ReceiveTimeUp();
}

void ALTTMessDirector::TriggerEarthquake()
{
	if (!bMessStarted)
	{
		bMessStarted = true;
		if (bUseTimer)
		{
			StartTimer();
		}
	}

	FRandomStream Rng(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		ALTTInteractableBase* Prop = *It;
		UStaticMeshComponent* M = Prop ? Prop->Mesh.Get() : nullptr;
		if (!M || Prop->IsPlaced() || Prop->IsHeld())
		{
			continue;
		}

		// Small props fly, chairs and tables mostly slide and tip.
		const float Scale = FMath::Clamp(25.f / FMath::Max(Prop->GetPropRadius(), 1.f), 0.35f, 1.f);
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Linear = Dir * Rng.FRandRange(HorizontalSpeed.X, HorizontalSpeed.Y) * Scale
			+ FVector::UpVector * Rng.FRandRange(UpSpeed.X, UpSpeed.Y) * Scale;
		const FVector Spin = FVector(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f)) * MaxSpin * Scale;

		M->SetSimulatePhysics(true);
		M->WakeAllRigidBodies();
		M->SetPhysicsLinearVelocity(Linear);
		M->SetPhysicsAngularVelocityInDegrees(Spin);
	}

	if (EarthquakeShake)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->ClientStartCameraShake(EarthquakeShake);
		}
	}
}

void ALTTMessDirector::NotifyPlaced(ALTTInteractableBase* Prop)
{
	PlacedCount = 0;
	for (const ALTTInteractableBase* T : Tasks)
	{
		if (IsValid(T) && T->IsPlaced())
		{
			++PlacedCount;
		}
	}
	OnProgressChanged.Broadcast(PlacedCount, Tasks.Num());
	CheckComplete();
}

void ALTTMessDirector::HandlePropDestroyed(AActor* DestroyedActor)
{
	ALTTInteractableBase* Prop = Cast<ALTTInteractableBase>(DestroyedActor);
	if (!Prop || !Tasks.Contains(Prop))
	{
		return;
	}
	Tasks.Remove(Prop);
	if (Prop->IsPlaced())
	{
		--PlacedCount;
	}
	else
	{
		++BrokenCount; // shattered or disappeared before it was put back
	}
	OnProgressChanged.Broadcast(PlacedCount, Tasks.Num());
	CheckComplete();
}

void ALTTMessDirector::CheckComplete()
{
	if (bAllPlaced || Tasks.Num() == 0 || PlacedCount < Tasks.Num())
	{
		return;
	}
	bAllPlaced = true;
	StopTimer();
	UE_LOG(LogTemp, Log, TEXT("MessDirector: set is clean (%d placed, %d broken)."), PlacedCount, BrokenCount);
	OnAllPlaced.Broadcast();
	ReceiveAllPlaced();
}
