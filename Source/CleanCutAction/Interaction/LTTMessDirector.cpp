// Clean! Cut! Action!

#include "Interaction/LTTMessDirector.h"
#include "UI/LTTUITheme.h"
#include "CleanCutActionPlayerController.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTCleanupHUD.h"
#include "Interaction/LTTEndScreen.h"
#include "UI/LTTBriefingScreen.h"
#include "UI/LTTEndingOverlay.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ObjectSaveContext.h"
#include "Engine/Level.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Interaction/LTTInteractionComponent.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/** Game input after a menu closes; the project's controller keeps Game+UI on touch so the touch buttons still work. */
	void LTTRestoreGameInput(APlayerController* PC)
	{
		if (ACleanCutActionPlayerController* CCPC = Cast<ACleanCutActionPlayerController>(PC))
		{
			CCPC->RestoreGameInput();
			return;
		}
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

ALTTMessDirector::ALTTMessDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	HUDClass = ULTTCleanupHUD::StaticClass();
	EndScreenClass = ULTTEndScreen::StaticClass();
	BriefingClass = ULTTBriefingScreen::StaticClass();
	EndingOverlayClass = ULTTEndingOverlay::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> NextPhase(TEXT("/Game/_CleanCutAction/SFX/SW_next_phase.SW_next_phase"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Miss(TEXT("/Game/_CleanCutAction/SFX/SW_miss.SW_miss"));
	SetReadySound = NextPhase.Object;
	TimeUpSound = Miss.Object;
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
	BlackOut();
	// Every prop records its home in its own BeginPlay; wait one tick so all of them have.
	GetWorldTimerManager().SetTimerForNextTick(this, &ALTTMessDirector::SetupTasks);
}

void ALTTMessDirector::SetupTasks()
{
	CreateHUD(); // in case the player controller didn't exist yet at BeginPlay
	BlackOut();
	SetupHintInput();
	ShowBriefing();
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

	// The ground floor, for the "fell under the map" test: the lowest bottom of any prop at home.
	LowestHomeZ = TNumericLimits<float>::Max();
	for (const ALTTInteractableBase* Prop : All)
	{
		const FBoxSphereBounds& B = Prop->Mesh->Bounds;
		LowestHomeZ = FMath::Min(LowestHomeZ, static_cast<float>(B.Origin.Z - B.BoxExtent.Z));
	}
	if (All.Num() == 0)
	{
		LowestHomeZ = GetActorLocation().Z;
	}
	if (bRescueStuckProps)
	{
		const float FirstCheck = (bEarthquakeOnStart ? EarthquakeDelay : 0.f) + SettleTime + RescueInterval;
		GetWorldTimerManager().SetTimer(RescueTimer, this, &ALTTMessDirector::RescueTick, RescueInterval, true, FirstCheck);
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
	else if (bFadeInOnStart)
	{
		GetWorldTimerManager().SetTimer(IntroTimer, this, &ALTTMessDirector::FinishIntro, 0.2f, false);
	}
	else
	{
		MarkMessStarted();
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

// ---- Hint ----

void ALTTMessDirector::SetupHintInput()
{
	if (bHintInputBound)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UEnhancedInputComponent* EIC = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
	if (!PC || !PC->IsLocalController() || !EIC)
	{
		// The controller's input component can show up a little later; try again next tick.
		GetWorldTimerManager().SetTimerForNextTick(this, &ALTTMessDirector::SetupHintInput);
		return;
	}

	const UInputAction* Action = HintAction;
	if (!Action)
	{
		// No asset given: make the action and a small mapping context at runtime.
		RuntimeHintAction = NewObject<UInputAction>(this, TEXT("IA_Hint_Runtime"));
		RuntimeHintContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Hint_Runtime"));
		if (HintKey.IsValid())
		{
			RuntimeHintContext->MapKey(RuntimeHintAction, HintKey);
		}
		if (HintGamepadKey.IsValid())
		{
			RuntimeHintContext->MapKey(RuntimeHintAction, HintGamepadKey);
		}
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->AddMappingContext(RuntimeHintContext, 1);
		}
		Action = RuntimeHintAction;
	}

	EIC->BindAction(Action, ETriggerEvent::Started, this, &ALTTMessDirector::HandleHintInput);
	bHintInputBound = true;
}

void ALTTMessDirector::HandleHintInput()
{
	ShowHint();
}

bool ALTTMessDirector::IsHintCandidate(const ALTTInteractableBase* Prop) const
{
	return IsValid(Prop) && Prop->Mesh && Prop->bIsCleanupTask && Prop->HasHome() && !Prop->IsPlaced() && !Prop->IsHeld();
}

float ALTTMessDirector::GetHintCooldownRemaining() const
{
	return FMath::Max(HintReadyTime - GetWorld()->GetTimeSeconds(), 0.f);
}

bool ALTTMessDirector::ShowHint()
{
	if (!bMessStarted || bAllPlaced || bTimeUp || GetHintCooldownRemaining() > 0.f)
	{
		return false;
	}

	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector From = Pawn ? Pawn->GetActorLocation() : GetActorLocation();
	const ALTTInteractableBase* Current = HintTarget.Get();

	// Nearest prop that can be put back right now; props waiting on their support (cup before its table) come last.
	// Pressing again while a hint is up moves on to a different prop.
	ALTTInteractableBase* Best = nullptr;
	double BestScore = TNumericLimits<double>::Max();
	for (ALTTInteractableBase* Prop : Tasks)
	{
		if (!IsHintCandidate(Prop) || Prop == Current)
		{
			continue;
		}
		double Score = FVector::DistSquared(From, Prop->Mesh->Bounds.Origin);
		if (!Prop->IsHomeSupportReady())
		{
			Score += 1.0e12;
		}
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Prop;
		}
	}
	if (!Best)
	{
		Best = IsHintCandidate(Current) ? const_cast<ALTTInteractableBase*>(Current) : nullptr;
	}
	if (!Best)
	{
		return false;
	}

	ClearHint();

	// Pointed at this one last time too and it's still not back: it's likely clipped into something or under
	// the map, so bring it to the middle of its floor where the player can reach it.
	if (bMoveRepeatHintToMiddle && LastHinted.Get() == Best)
	{
		MoveHintedPropToMiddle(Best);
		LastHinted.Reset();
	}
	else
	{
		LastHinted = Best;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	HintTarget = Best;
	HintEndTime = Now + HintDuration;
	HintReadyTime = Now + HintCooldown;
	++HintsUsed;

	// Same outline as focus (stencil 1, M_PP_Outline).
	Best->Mesh->SetRenderCustomDepth(true);
	Best->Mesh->SetCustomDepthStencilValue(1);
	GetWorldTimerManager().SetTimer(HintTimer, this, &ALTTMessDirector::UpdateHint, 0.1f, true);

	UE_LOG(LogTemp, Log, TEXT("MessDirector: hint -> %s"), *Best->GetPropName().ToString());
	ReceiveHintShown(Best);
	return true;
}

void ALTTMessDirector::UpdateHint()
{
	ALTTInteractableBase* Target = HintTarget.Get();
	if (!IsHintCandidate(Target) || GetWorld()->GetTimeSeconds() >= HintEndTime || bAllPlaced || bTimeUp)
	{
		ClearHint();
		return;
	}
	// Losing focus turns the outline off; keep it on while the hint lasts.
	if (!Target->Mesh->bRenderCustomDepth)
	{
		Target->Mesh->SetRenderCustomDepth(true);
		Target->Mesh->SetCustomDepthStencilValue(1);
	}
}

void ALTTMessDirector::ClearHint()
{
	GetWorldTimerManager().ClearTimer(HintTimer);
	ALTTInteractableBase* Target = HintTarget.Get();
	HintTarget.Reset();
	if (IsValid(Target) && Target->IsHeld() && LastHinted.Get() == Target)
	{
		LastHinted.Reset(); // the player found it, so it isn't stuck
	}
	if (!IsValid(Target) || !Target->Mesh || Target->IsHeld())
	{
		return;
	}
	// Leave the outline if the player is aiming at it.
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const ULTTInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<ULTTInteractionComponent>() : nullptr;
	if (!Interaction || Interaction->GetFocusedActor() != Target)
	{
		Target->Mesh->SetRenderCustomDepth(false);
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
	MessEndTime = GetWorld()->GetTimeSeconds();
	ClearHint();
	UE_LOG(LogTemp, Log, TEXT("MessDirector: time up (%d/%d placed, %d broken, %d points)."), PlacedCount, Tasks.Num(), BrokenCount, GetScore());
	OnTimeUp.Broadcast();
	ReceiveTimeUp();
	ScheduleEndScreen(false);
}

void ALTTMessDirector::TriggerEarthquake()
{
const bool bShouldStartHidden = bFadeInOnStart && !bIntroDone;
	if (bShouldStartHidden)
	{
		// Behind the black screen: let everything land, then fade in (FinishIntro starts the clock).
		GetWorldTimerManager().SetTimer(IntroTimer, this, &ALTTMessDirector::FinishIntro, FMath::Max(SettleTime, 0.05f), false);
	}
	else
	{
		MarkMessStarted();
	}

	FRandomStream Rng(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		ALTTInteractableBase* Prop = *It;
		UStaticMeshComponent* M = Prop ? Prop->Mesh.Get() : nullptr;
		// Set dressing (not a task, starts at home) stays put; it isn't part of the mess.
		if (!M || Prop->IsPlaced() || Prop->IsHeld() || (Prop->bStartsAtHome && !Prop->bIsCleanupTask))
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

	if (EarthquakeShake && !bShouldStartHidden)
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

bool ALTTMessDirector::HasDependents(const ALTTInteractableBase* Prop) const
{
	if (!Prop)
	{
		return false;
	}
	for (const ALTTInteractableBase* T : Tasks)
	{
		if (IsValid(T) && T != Prop && T->HomeSupport == Prop)
		{
			return true;
		}
	}
	return false;
}

void ALTTMessDirector::NotifyBrokenInPlace(ALTTInteractableBase* Prop)
{
	if (!Prop || !Tasks.Contains(Prop))
	{
		return;
	}
	Tasks.Remove(Prop);
	Prop->OnDestroyed.RemoveDynamic(this, &ALTTMessDirector::HandlePropDestroyed);
	++BrokenCount;
	if (HintTarget.Get() == Prop)
	{
		ClearHint();
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: %s broke; back on its mark (red) for the props that rest on it."), *Prop->GetPropName().ToString());
	NotifyPlaced(nullptr); // recounts placed props, broadcasts progress, checks for a clean set
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
	// Finishing early pays: bonus for every second still on the clock.
	if (bUseTimer && bMessStarted && !bTimeUp)
	{
		TimeLeftAtFinish = GetTimeRemaining();
		TimeBonusPoints = FMath::FloorToInt(TimeLeftAtFinish * FMath::Max(PointsPerSecondLeft, 0.f));
	}
	StopTimer();
	ClearHint();
	UE_LOG(LogTemp, Log, TEXT("MessDirector: set is clean (%d placed, %d broken)."), PlacedCount, BrokenCount);
	MessEndTime = GetWorld()->GetTimeSeconds();
	OnAllPlaced.Broadcast();
	ReceiveAllPlaced();
	ScheduleEndScreen(true);
}

// ---- Mess start / end screen ----

void ALTTMessDirector::MarkMessStarted()
{
	if (bMessStarted)
	{
		return;
	}
	if (BriefingWidget)
	{
		bMessStartPending = true; // the clock waits for the Start button
		return;
	}
	bMessStarted = true;
	MessStartTime = GetWorld()->GetTimeSeconds();
	if (bUseTimer)
	{
		StartTimer();
	}
}

float ALTTMessDirector::GetTimeOnSet() const
{
	if (MessStartTime < 0.f)
	{
		return 0.f;
	}
	const float End = MessEndTime >= 0.f ? MessEndTime : GetWorld()->GetTimeSeconds();
	return FMath::Max(End - MessStartTime, 0.f);
}

void ALTTMessDirector::ScheduleEndScreen(bool bSetClean)
{
	if (!bShowEndScreen)
	{
		return;
	}
	const FTimerDelegate Show = FTimerDelegate::CreateUObject(this, &ALTTMessDirector::PlayEnding, bSetClean);
	if (EndScreenDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(EndScreenTimer, Show, EndScreenDelay, false);
	}
	else
	{
		PlayEnding(bSetClean);
	}
}

void ALTTMessDirector::ShowEndScreen(bool bSetClean)
{
	if (EndScreenWidget || !EndScreenClass)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	EndScreenWidget = CreateWidget<UUserWidget>(PC, EndScreenClass);
	if (!EndScreenWidget)
	{
		return;
	}
	if (ULTTEndScreen* Screen = Cast<ULTTEndScreen>(EndScreenWidget))
	{
		Screen->Setup(this, bSetClean);
	}
	EndScreenWidget->AddToViewport(10);
	if (USoundBase* Sting = bSetClean ? SetReadySound.Get() : TimeUpSound.Get())
	{
		UGameplayStatics::PlaySound2D(this, Sting);
	}
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// The level is done but the player isn't frozen: hand the camera back from the ending shots, give movement
	// back and keep the game running behind the end screen (cursor up for its buttons).
	GetWorldTimerManager().ClearTimer(EndingTimer);
	bEndingPlaying = false;
	if (EndingPlayer)
	{
		EndingPlayer->Stop(); // also lifts the sequence's movement / look lock
		EndingPlayer = nullptr;
	}
	if (EndingActor)
	{
		EndingActor->Destroy();
		EndingActor = nullptr;
	}
	if (PC->GetPawn() && PC->GetViewTarget() != PC->GetPawn())
	{
		PC->SetViewTargetWithBlend(PC->GetPawn(), 0.6f, VTBlend_Cubic);
	}
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StopCameraFade();
	}
	SetPlayerInputBlocked(false);
	ACleanCutActionPlayerController::KeepGameInputWithCursor(PC);
}

void ALTTMessDirector::CloseEndScreen()
{
	GetWorldTimerManager().ClearTimer(EndScreenTimer);
	bEndingPlaying = false;
	if (EndScreenWidget)
	{
		EndScreenWidget->RemoveFromParent();
		EndScreenWidget = nullptr;
	}
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (bTimeUp && !bAllPlaced)
	{
		// Keep going without a clock; the set-clean screen still shows when the last prop goes back.
		bTimeUp = false;
		bUseTimer = false;
		MessEndTime = -1.f;
	}

	UGameplayStatics::SetGamePaused(this, false);
	GetWorldTimerManager().ClearTimer(EndingTimer);
	if (EndingPlayer)
	{
		EndingPlayer->Stop(); // hands the camera back
		EndingPlayer = nullptr;
	}
	if (EndingActor)
	{
		EndingActor->Destroy();
		EndingActor = nullptr;
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->GetPawn() && PC->GetViewTarget() != PC->GetPawn())
		{
			PC->SetViewTarget(PC->GetPawn());
		}
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopCameraFade();
		}
		LTTRestoreGameInput(PC);
	}
	SetPlayerInputBlocked(false);
}

void ALTTMessDirector::LoadNextLevel()
{
	if (NextLevelName.IsNone())
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: set is clean, opening %s."), *NextLevelName.ToString());
	// Input mode lives on the viewport and survives OpenLevel; don't carry the end screen's into Phase 2.
	ACleanCutActionPlayerController::ResetGameInput(UGameplayStatics::GetPlayerController(this, 0));
	UGameplayStatics::OpenLevel(this, NextLevelName);
}

// ---- Briefing ----

void ALTTMessDirector::ShowBriefing()
{
	if (!bShowBriefing || !BriefingClass || BriefingWidget)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	BriefingWidget = CreateWidget<UUserWidget>(PC, BriefingClass);
	if (!BriefingWidget)
	{
		return;
	}
	if (ULTTBriefingScreen* Screen = Cast<ULTTBriefingScreen>(BriefingWidget))
	{
		Screen->Setup(this);
	}
	BriefingWidget->AddToViewport(20);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(BriefingWidget->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
}

void ALTTMessDirector::StartFromBriefing()
{
	if (!BriefingWidget)
	{
		return;
	}
	BriefingWidget->RemoveFromParent();
	BriefingWidget = nullptr;
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		LTTRestoreGameInput(PC);
	}
	// Whatever was waiting on the briefing: the fade in (which starts the clock) or just the clock.
	if (bFinishIntroPending)
	{
		bFinishIntroPending = false;
		bMessStartPending = false;
		FinishIntro();
	}
	else if (bMessStartPending)
	{
		bMessStartPending = false;
		MarkMessStarted();
	}
}

// ---- Intro fade ----

void ALTTMessDirector::BlackOut()
{
	if (!bFadeInOnStart || bIntroDone)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->SetManualCameraFade(1.f, FadeColor, false);
		SetPlayerInputBlocked(true);
	}
}

void ALTTMessDirector::FinishIntro()
{
	if (bIntroDone)
	{
		return;
	}
	if (BriefingWidget)
	{
		bFinishIntroPending = true; // stay black until the player presses Start
		return;
	}
	bIntroDone = true;
	if (bRescueStuckProps)
	{
		RescueStuckProps(); // still black: anything the earthquake pushed through the floor or into furniture comes back unseen
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FMath::Max(FadeInTime, 0.01f), FadeColor);
	}
	SetPlayerInputBlocked(false);
	MarkMessStarted();
}

void ALTTMessDirector::SetPlayerInputBlocked(bool bBlocked)
{
	if (bBlocked == bPlayerInputBlocked)
	{
		return;
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		// Both are counters in the engine, so always set and clear them in pairs.
		PC->SetIgnoreMoveInput(bBlocked);
		PC->SetIgnoreLookInput(bBlocked);
		bPlayerInputBlocked = bBlocked;
	}
}

// ---- Ending shots ----

bool ALTTMessDirector::HasEndingShots() const
{
	if (!EndSequence.IsNull())
	{
		return true;
	}
	for (const ACameraActor* Cam : TourCameras)
	{
		if (IsValid(Cam))
		{
			return true;
		}
	}
	return false;
}

void ALTTMessDirector::GatherTourCameras()
{
	TourCameras.Reset();
	for (ACameraActor* Cam : EndTourCameras)
	{
		if (IsValid(Cam))
		{
			TourCameras.Add(Cam);
		}
	}

	if (TourCameras.Num() == 0 && TourCameraLabels.Num() > 0)
	{
#if WITH_EDITOR
		ResolveLabelCameras(TourCameras); // live lookup in Play In Editor
#endif
		if (TourCameras.Num() == 0)
		{
			for (ACameraActor* Cam : ResolvedLabelCameras) // saved/cooked lookup in the packaged game
			{
				if (IsValid(Cam))
				{
					TourCameras.Add(Cam);
				}
			}
		}
		if (TourCameras.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("MessDirector: ending tour uses %d labelled cameras."), TourCameras.Num());
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("MessDirector: none of the TourCameraLabels were found; picking cameras automatically."));
	}

	if (TourCameras.Num() > 0 || !bAutoFindTourCameras)
	{
		return;
	}

	// The cafe = the box around every prop's home spot (both floors), plus a margin.
	FBox Cafe(ForceInit);
	for (const ALTTInteractableBase* Prop : Tasks)
	{
		if (IsValid(Prop) && Prop->HasHome())
		{
			Cafe += Prop->HomeTransform.GetLocation();
		}
	}
	if (!Cafe.IsValid)
	{
		return;
	}
	Cafe = Cafe.ExpandBy(TourCameraMargin);

	TArray<ACameraActor*> Inside;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && Cafe.IsInsideOrOn(It->GetActorLocation()))
		{
			Inside.Add(*It);
		}
	}
	for (int32 i = Inside.Num() - 1; i > 0; --i)
	{
		Inside.Swap(i, FMath::RandRange(0, i));
	}
	for (int32 i = 0; i < FMath::Min(MaxTourCameras, Inside.Num()); ++i)
	{
		TourCameras.Add(Inside[i]);
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: ending tour uses %d of %d cameras inside the cafe."), TourCameras.Num(), Inside.Num());
	for (const ACameraActor* Cam : TourCameras)
	{
		UE_LOG(LogTemp, Log, TEXT("  tour camera: %s"), *Cam->GetName());
	}
}

void ALTTMessDirector::ResolveLabelCameras(TArray<TObjectPtr<ACameraActor>>& Out) const
{
	Out.Reset();
#if WITH_EDITOR
	const ULevel* Level = GetLevel();
	if (!Level)
	{
		return;
	}
	for (const FString& Label : TourCameraLabels)
	{
		for (AActor* Actor : Level->Actors)
		{
			ACameraActor* Cam = Cast<ACameraActor>(Actor);
			if (Cam && Cam->GetActorLabel() == Label)
			{
				Out.Add(Cam);
				break;
			}
		}
	}
#endif
}

void ALTTMessDirector::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
#if WITH_EDITOR
	// Bake the labelled cameras into a real reference so the packaged game (no labels) can find them.
	ResolveLabelCameras(ResolvedLabelCameras);
#endif
}

void ALTTMessDirector::PlayEnding(bool bSetClean)
{
	bEndingSetClean = bSetClean;
	bEndingFinished = false;
	const bool bWantShots = bSetClean || bPlayEndingOnTimeUp;
	if (bWantShots && EndSequence.IsNull())
	{
		GatherTourCameras();
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!bWantShots || !HasEndingShots() || !PC || !PC->PlayerCameraManager)
	{
		FinishEnding();
		return;
	}

	ClearHint();
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetPlayerInputBlocked(true);
	bEndingPlaying = true;
	ShowEndingOverlay(bAllowSkipEnding);

	if (EndingFadeTime > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, EndingFadeTime, FadeColor, false, true);
		GetWorldTimerManager().SetTimer(EndingTimer, this, &ALTTMessDirector::StartEndingShots, EndingFadeTime, false);
	}
	else
	{
		StartEndingShots();
	}
}

void ALTTMessDirector::StartEndingShots()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (ULevelSequence* Sequence = EndSequence.LoadSynchronous())
	{
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FMath::Max(EndingFadeTime, 0.01f), FadeColor);
		}
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bPauseAtEnd = true;          // hold the last shot under the end screen
		Settings.bDisableMovementInput = true;
		Settings.bDisableLookAtInput = true;
		Settings.bHidePlayer = true;
		ALevelSequenceActor* OutActor = nullptr;
		EndingPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Sequence, Settings, OutActor);
		EndingActor = OutActor;
		if (EndingPlayer)
		{
			EndingPlayer->OnFinished.AddDynamic(this, &ALTTMessDirector::HandleEndingFinished);
			EndingPlayer->Play();
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("MessDirector: couldn't play the end sequence, using tour cameras."));
	}

	TourIndex = INDEX_NONE;
	NextTourShot();
}

void ALTTMessDirector::NextTourShot()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	ACameraActor* Cam = nullptr;
	while (!Cam && ++TourIndex < TourCameras.Num())
	{
		Cam = IsValid(TourCameras[TourIndex]) ? TourCameras[TourIndex].Get() : nullptr;
	}
	if (!Cam || !PC)
	{
		HandleEndingFinished();
		return;
	}

	// Cut while the screen is black, then fade in.
	PC->SetViewTarget(Cam);
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FMath::Max(TourFadeTime, 0.01f), FadeColor);
	}

	bool bLast = true;
	for (int32 i = TourIndex + 1; i < TourCameras.Num(); ++i)
	{
		if (IsValid(TourCameras[i]))
		{
			bLast = false;
			break;
		}
	}
	if (bLast)
	{
		// The last shot stays up under the end screen.
		GetWorldTimerManager().SetTimer(EndingTimer, this, &ALTTMessDirector::HandleEndingFinished, TourFadeTime + TourShotTime, false);
	}
	else
	{
		GetWorldTimerManager().SetTimer(EndingTimer, this, &ALTTMessDirector::FadeOutTourShot, TourFadeTime + TourShotTime, false);
	}
}

void ALTTMessDirector::FadeOutTourShot()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FMath::Max(TourFadeTime, 0.01f), FadeColor, false, true);
	}
	GetWorldTimerManager().SetTimer(EndingTimer, this, &ALTTMessDirector::NextTourShot, FMath::Max(TourFadeTime, 0.01f), false);
}

void ALTTMessDirector::HandleEndingFinished()
{
	GetWorldTimerManager().ClearTimer(EndingTimer);
	FinishEnding();
}

void ALTTMessDirector::ShowEndingOverlay(bool bWithSkip)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController() || !EndingOverlayClass)
	{
		return;
	}
	if (!EndingOverlay || !EndingOverlay->IsInViewport())
	{
		EndingOverlay = CreateWidget<ULTTEndingOverlay>(PC, LTTTheme::PreferWBP<ULTTEndingOverlay>(EndingOverlayClass, TEXT("WBP_EndingOverlay")));
		if (!EndingOverlay)
		{
			return;
		}
		EndingOverlay->Setup(this);
		EndingOverlay->AddToViewport(30); // over the end screen (10), under the leaderboard (100)
	}
	EndingOverlay->SetSkipVisible(bWithSkip);
	if (bWithSkip)
	{
		// The shots take no game input anyway; UI focus lets Space / Enter / gamepad A skip, and the cursor tap SKIP.
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(EndingOverlay->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
}

void ALTTMessDirector::SkipEnding()
{
	if (!bEndingPlaying || bEndingFinished)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(EndingTimer);
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (EndingPlayer)
	{
		EndingPlayer->Pause(); // hold where it is under the end screen
	}
	else if (PC)
	{
		// Straight to the last tour shot (it stays up under the end screen).
		for (int32 i = TourCameras.Num() - 1; i >= 0; --i)
		{
			if (IsValid(TourCameras[i]))
			{
				PC->SetViewTarget(TourCameras[i]);
				TourIndex = i;
				break;
			}
		}
	}
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StopCameraFade(); // no black frame left over from a cut
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: ending skipped."));
	FinishEnding();
}

void ALTTMessDirector::FinishEnding()
{
	if (bEndingFinished)
	{
		return;
	}
	bEndingFinished = true;
	bEndingPlaying = false;
	GetWorldTimerManager().ClearTimer(EndingTimer);

	if (bEndingSetClean && bConfettiOnSetClean)
	{
		// Confetti first, then the end screen comes up under the falling pieces.
		ShowEndingOverlay(false);
		if (EndingOverlay)
		{
			EndingOverlay->PlayConfetti();
		}
		if (ConfettiLeadTime > 0.f)
		{
			const FTimerDelegate Show = FTimerDelegate::CreateUObject(this, &ALTTMessDirector::ShowEndScreen, bEndingSetClean);
			GetWorldTimerManager().SetTimer(EndScreenTimer, Show, ConfettiLeadTime, false);
			return;
		}
	}
	else if (EndingOverlay)
	{
		EndingOverlay->SetSkipVisible(false);
	}
	ShowEndScreen(bEndingSetClean);
}

// ---- Stuck props ----

namespace
{
	/** The prop's mesh bounds in its own space (scaled), and the world centre of that box. */
	void LTTLocalBox(const ALTTInteractableBase* Prop, FVector& OutExtent, FVector& OutWorldCentre)
	{
		const UStaticMeshComponent* M = Prop->Mesh;
		const FBoxSphereBounds Local = M->GetStaticMesh() ? M->GetStaticMesh()->GetBounds() : FBoxSphereBounds(FVector::ZeroVector, FVector(10.f), 10.f);
		OutExtent = Local.BoxExtent * M->GetComponentScale().GetAbs();
		OutWorldCentre = M->GetComponentTransform().TransformPosition(Local.Origin);
	}

	FCollisionObjectQueryParams LTTSolidObjects()
	{
		FCollisionObjectQueryParams Types;
		Types.AddObjectTypesToQuery(ECC_WorldStatic);
		Types.AddObjectTypesToQuery(ECC_WorldDynamic);
		Types.AddObjectTypesToQuery(ECC_PhysicsBody);
		return Types;
	}
}

bool ALTTMessDirector::IsPropStuck(const ALTTInteractableBase* Prop) const
{
	if (!IsValid(Prop) || !Prop->Mesh || !Prop->HasHome() || Prop->IsPlaced() || Prop->IsHeld() || Prop->IsBrokenInPlace())
	{
		return false;
	}
	const UStaticMeshComponent* M = Prop->Mesh;
	const FBoxSphereBounds& B = M->Bounds;

	// Under the map: its top is below the ground floor.
	if (B.Origin.Z + B.BoxExtent.Z < LowestHomeZ - UnderFloorMargin)
	{
		return true;
	}
	// Flew out of the set (out a window, through a wall).
	if (FVector::Dist2D(B.Origin, Prop->HomeTransform.GetLocation()) > MaxDistanceFromSet)
	{
		return true;
	}
	// Still moving: physics may yet push it out of whatever it's in.
	if (M->IsSimulatingPhysics() && M->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(40.f))
	{
		return false;
	}

	// Inside something: a small sphere at the middle of the prop (a third of its thinnest side, so a plate lying on
	// a table doesn't count) overlaps a wall, the floor or another prop's collision.
	// Resting where it belongs (a pencil on its home table) is never "inside" anything.
	if (FVector::Dist(Prop->GetActorLocation(), Prop->HomeTransform.GetLocation()) < 15.f)
	{
		return false;
	}
	FVector Extent, Centre;
	LTTLocalBox(Prop, Extent, Centre);
	// Too thin to tell resting contact from being inside (a pencil lying on the floor touches it at its middle).
	if (Extent.GetMin() < 2.f)
	{
		return false;
	}
	const float Radius = FMath::Clamp(Extent.GetMin() * 0.35f, 1.f, 15.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTStuckProp), false, Prop);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Centre, FQuat::Identity, LTTSolidObjects(), FCollisionShape::MakeSphere(Radius), Params);
	const ECollisionChannel MyType = M->GetCollisionObjectType();
	for (const FOverlapResult& O : Overlaps)
	{
		const UPrimitiveComponent* Other = O.GetComponent();
		if (!Other || Other->GetOwner() == Prop || Other->GetCollisionResponseToChannel(MyType) != ECR_Block)
		{
			continue;
		}
		if (const ALTTInteractableBase* OtherProp = Cast<ALTTInteractableBase>(Other->GetOwner()))
		{
			if (OtherProp->IsHeld())
			{
				continue; // the player is carrying it through
			}
			if (OtherProp->GetPropRadius() < Prop->GetPropRadius())
			{
				continue; // a cup sitting on a chair's seat: the small one is the stuck one, if anything
			}
		}
		return true;
	}
	return false;
}

bool ALTTMessDirector::FindRescueSpot(const ALTTInteractableBase* Prop, FVector& OutLocation) const
{
	return FindFreeSpotNear(Prop, Prop->HomeTransform.GetLocation(), OutLocation);
}

FVector ALTTMessDirector::GetHintRescueAnchor(const ALTTInteractableBase* Prop) const
{
	if (IsValid(HintRescueSpot))
	{
		return HintRescueSpot->GetActorLocation() + FVector(0.f, 0.f, 100.f); // search from about chest height
	}
	// Middle of the area spanned by the home spots on the prop's floor, at that floor's height (lowest home there).
	const float HomeZ = Prop->HomeTransform.GetLocation().Z;
	FBox Area(ForceInit);
	for (const ALTTInteractableBase* Other : Tasks)
	{
		if (IsValid(Other) && Other->HasHome())
		{
			const FVector L = Other->HomeTransform.GetLocation();
			if (FMath::Abs(L.Z - HomeZ) <= HintFloorTolerance)
			{
				Area += L;
			}
		}
	}
	if (!Area.IsValid)
	{
		return Prop->HomeTransform.GetLocation();
	}
	const FVector Centre = Area.GetCenter();
	return FVector(Centre.X, Centre.Y, Area.Min.Z + 100.f); // about chest height, above chairs
}

void ALTTMessDirector::MoveHintedPropToMiddle(ALTTInteractableBase* Prop)
{
	const FVector Anchor = GetHintRescueAnchor(Prop);
	FVector Location;
	if (!FindFreeSpotNear(Prop, Anchor, Location) && !FindRescueSpot(Prop, Location))
	{
		// Nowhere free: drop it from above the middle.
		Location = Anchor + FVector(0.f, 0.f, Prop->GetPropRadius() + 60.f);
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: hinted %s twice, moved from %s to %s."), *Prop->GetName(), *Prop->GetActorLocation().ToCompactString(), *Location.ToCompactString());
	Prop->SetActorLocationAndRotation(Location, Prop->HomeTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (UStaticMeshComponent* M = Prop->Mesh)
	{
		if (M->IsSimulatingPhysics())
		{
			M->SetPhysicsLinearVelocity(FVector::ZeroVector);
			M->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			M->WakeAllRigidBodies();
		}
	}
	SuspectProps.Remove(Prop);
}

bool ALTTMessDirector::FindFreeSpotNear(const ALTTInteractableBase* Prop, const FVector& Anchor, FVector& OutLocation) const
{
	const UStaticMeshComponent* M = Prop->Mesh;
	const FTransform& Home = Prop->HomeTransform;
	const FBoxSphereBounds Local = M->GetStaticMesh() ? M->GetStaticMesh()->GetBounds() : FBoxSphereBounds(FVector::ZeroVector, FVector(10.f), 10.f);
	// The prop goes back the way it stands at home: its box at the home rotation.
	const FTransform HomeRotScale(Home.GetRotation(), FVector::ZeroVector, Home.GetScale3D());
	const FBox Rotated = FBox(Local.Origin - Local.BoxExtent, Local.Origin + Local.BoxExtent).TransformBy(HomeRotScale);
	const FVector HalfSize = Rotated.GetExtent();
	const FVector CentreOffset = Rotated.GetCenter(); // actor origin -> box centre

	const FVector HomeLoc = Anchor;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTRescueSpot), false, Prop);
	static const float Rings[] = { 0.f, 70.f, 140.f, 220.f, 320.f };
	for (const float Ring : Rings)
	{
		const int32 Steps = Ring <= 0.f ? 1 : 10;
		for (int32 i = 0; i < Steps; ++i)
		{
			const float Angle = 2.f * PI * i / Steps;
			const FVector Spot = HomeLoc + CentreOffset + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Ring;
			// Same room as the home spot: nothing solid in between.
			const FVector EyeHome = HomeLoc + CentreOffset + FVector(0.f, 0.f, 30.f);
			if (Ring > 0.f && GetWorld()->LineTraceTestByChannel(EyeHome, FVector(Spot.X, Spot.Y, EyeHome.Z), ECC_Visibility, Params))
			{
				continue;
			}
			// Whatever is underneath (floor, table, counter).
			FHitResult Hit;
			const FVector Start(Spot.X, Spot.Y, EyeHome.Z);
			const FVector End(Spot.X, Spot.Y, HomeLoc.Z - 800.f);
			if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
			{
				continue;
			}
			const FVector Centre(Spot.X, Spot.Y, Hit.ImpactPoint.Z + HalfSize.Z + 4.f);
			if (Centre.Z - HalfSize.Z < LowestHomeZ - UnderFloorMargin)
			{
				continue; // found a hole, not a floor
			}
			if (GetWorld()->OverlapAnyTestByObjectType(Centre, FQuat::Identity, LTTSolidObjects(), FCollisionShape::MakeBox(HalfSize + FVector(1.f)), Params))
			{
				continue; // something's already there
			}
			OutLocation = Centre - CentreOffset;
			return true;
		}
	}
	return false;
}

void ALTTMessDirector::MoveStuckProp(ALTTInteractableBase* Prop)
{
	FVector Location;
	if (!FindRescueSpot(Prop, Location))
	{
		// Nowhere free nearby: drop it from just above home, like a prop that fell out of the world.
		Location = Prop->HomeTransform.GetLocation() + FVector(0.f, 0.f, Prop->GetPropRadius() + 40.f);
	}
	UE_LOG(LogTemp, Log, TEXT("MessDirector: %s was stuck at %s, moved to %s."), *Prop->GetName(), *Prop->GetActorLocation().ToCompactString(), *Location.ToCompactString());
	Prop->SetActorLocationAndRotation(Location, Prop->HomeTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (UStaticMeshComponent* M = Prop->Mesh)
	{
		if (M->IsSimulatingPhysics())
		{
			M->SetPhysicsLinearVelocity(FVector::ZeroVector);
			M->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			M->WakeAllRigidBodies();
		}
	}
	SuspectProps.Remove(Prop);
}

int32 ALTTMessDirector::RescueStuckProps()
{
	TArray<ALTTInteractableBase*> Stuck;
	for (ALTTInteractableBase* Prop : Tasks)
	{
		if (IsPropStuck(Prop))
		{
			Stuck.Add(Prop);
		}
	}
	for (ALTTInteractableBase* Prop : Stuck)
	{
		MoveStuckProp(Prop);
	}
	return Stuck.Num();
}

namespace LTTRescueCountsPrivate
{
	// How often each prop was moved during play. Keyed weakly, so props from an earlier PIE session just go stale.
	static TMap<TWeakObjectPtr<ALTTInteractableBase>, int32> Counts;
	static constexpr int32 MaxPerProp = 3;
}

void ALTTMessDirector::RescueTick()
{
	if (!bRescueStuckProps || bAllPlaced)
	{
		return;
	}
	// Moved only when stuck on two checks in a row, so a prop just tumbling through a doorway is left alone.
	TSet<TWeakObjectPtr<ALTTInteractableBase>> NewSuspects;
	TArray<ALTTInteractableBase*> Confirmed;
	for (ALTTInteractableBase* Prop : Tasks)
	{
		if (IsPropStuck(Prop))
		{
			if (SuspectProps.Contains(Prop))
			{
				Confirmed.Add(Prop);
			}
			else
			{
				NewSuspects.Add(Prop);
			}
		}
	}
	SuspectProps = MoveTemp(NewSuspects);
	for (ALTTInteractableBase* Prop : Confirmed)
	{
		// A prop that keeps coming back stuck is a false positive (odd collision), not a real one: stop moving it.
		int32& Count = LTTRescueCountsPrivate::Counts.FindOrAdd(Prop);
		if (++Count > LTTRescueCountsPrivate::MaxPerProp)
		{
			continue;
		}
		MoveStuckProp(Prop);
	}
}


// ---- Cheat ----

namespace
{
	int32 StackDepth(const ALTTInteractableBase* Prop)
	{
		int32 Depth = 0;
		for (const ALTTInteractableBase* S = Prop ? Prop->HomeSupport.Get() : nullptr; IsValid(S) && Depth < 16; S = S->HomeSupport)
		{
			++Depth;
		}
		return Depth;
	}
}

int32 ALTTMessDirector::CheatPlaceAll(int32 LeaveCount)
{
	TArray<ALTTInteractableBase*> Open;
	for (ALTTInteractableBase* Prop : Tasks)
	{
		if (IsValid(Prop) && Prop->HasHome() && !Prop->IsPlaced() && !Prop->IsHeld())
		{
			Open.Add(Prop);
		}
	}

	// Leave props nothing else rests on (a cup, not the table under it), picked at random.
	TSet<const ALTTInteractableBase*> Supports;
	for (const ALTTInteractableBase* Prop : Open)
	{
		if (IsValid(Prop->HomeSupport))
		{
			Supports.Add(Prop->HomeSupport.Get());
		}
	}
	TArray<ALTTInteractableBase*> Leaves;
	TArray<ALTTInteractableBase*> Others;
	for (ALTTInteractableBase* Prop : Open)
	{
		(Supports.Contains(Prop) ? Others : Leaves).Add(Prop);
	}
	auto Shuffle = [](TArray<ALTTInteractableBase*>& A)
	{
		for (int32 i = A.Num() - 1; i > 0; --i)
		{
			A.Swap(i, FMath::RandRange(0, i));
		}
	};
	Shuffle(Leaves);
	Shuffle(Others);
	Leaves.Append(Others); // fall back to supports only if there aren't enough leaves

	TSet<ALTTInteractableBase*> Keep;
	for (int32 i = 0; i < FMath::Min(FMath::Max(LeaveCount, 0), Leaves.Num()); ++i)
	{
		Keep.Add(Leaves[i]);
	}

	// Place bottom-up so tables are home before the plates on them.
	TArray<ALTTInteractableBase*> ToPlace;
	for (ALTTInteractableBase* Prop : Open)
	{
		if (!Keep.Contains(Prop))
		{
			ToPlace.Add(Prop);
		}
	}
	ToPlace.Sort([](const ALTTInteractableBase& A, const ALTTInteractableBase& B) { return StackDepth(&A) < StackDepth(&B); });

	ClearHint();
	for (ALTTInteractableBase* Prop : ToPlace)
	{
		Prop->SnapToHome(); // calls NotifyPlaced (and the end screen if nothing was left)
	}

	UE_LOG(LogTemp, Log, TEXT("MessDirector: cheat placed %d props, %d left to place."), ToPlace.Num(), Keep.Num());
	for (const ALTTInteractableBase* Prop : Keep)
	{
		UE_LOG(LogTemp, Log, TEXT("  left: %s"), *Prop->GetPropName().ToString());
	}
	return ToPlace.Num();
}

static FAutoConsoleCommandWithWorldAndArgs GLTTPlaceAllCommand(
	TEXT("LTT.PlaceAll"),
	TEXT("LTT.PlaceAll [LeaveCount=5] - puts every cleanup prop back except LeaveCount, to test the ending."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const int32 Leave = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 5;
		if (ALTTMessDirector* Director = ALTTMessDirector::Get(World))
		{
			Director->CheatPlaceAll(Leave);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LTT.PlaceAll: no mess director in this world."));
		}
	}));
