// Clean! Cut! Action!

#include "Shoot/LTTSceneDirector.h"
#include "CleanCutActionPlayerController.h"
#include "Shoot/LTTEffectStation.h"
#include "Shoot/LTTFilmActor.h"
#include "Shoot/LTTFilmCamera.h"
#include "Shoot/LTTShootHUD.h"
#include "Shoot/LTTShootEndScreen.h"
#include "Interaction/LTTInteractableBase.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "LTTSceneDirector"

namespace
{
	const FName NAME_Marlowe(TEXT("Marlowe"));
	const FName NAME_Dash(TEXT("Dash"));
	const FName NAME_Juniper(TEXT("Juniper"));
	const FName NAME_StormOff(TEXT("StormOff"));
	const FName Param_Rain(TEXT("RainIntensity"));
	const FName Param_Wetness(TEXT("Wetness"));
	const FName Param_Flash(TEXT("LightningFlash"));
	const FName Param_Wind(TEXT("WindStrength"));
	const FName Param_Backdrop(TEXT("BackdropTime"));
	constexpr float FlashSeconds = 0.3f;
}

ALTTSceneDirector::ALTTSceneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	HUDClass = ULTTShootHUD::StaticClass();
	EndScreenClass = ULTTShootEndScreen::StaticClass();
}

ALTTSceneDirector* ALTTSceneDirector::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (World)
	{
		for (TActorIterator<ALTTSceneDirector> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ALTTSceneDirector::BeginPlay()
{
	Super::BeginPlay();
	// Arriving from Phase 1's end screen leaves the viewport in UI-only input with the cursor up.
	ACleanCutActionPlayerController::ResetGameInput(UGameplayStatics::GetPlayerController(this, 0));
	Scene = SceneData ? SceneData.Get() : ULTTSceneData::MakeScene42(this);
	TakeMeter = MeterStart;
	CreateHUD();
	// Stations, actors and props finish their own BeginPlay first.
	GetWorldTimerManager().SetTimerForNextTick(this, &ALTTSceneDirector::GatherLevel);
}

void ALTTSceneDirector::GatherLevel()
{
	UWorld* World = GetWorld();
	for (TActorIterator<ALTTEffectStation> It(World); It; ++It)
	{
		Stations.Add(*It);
	}
	for (TActorIterator<ALTTFilmActor> It(World); It; ++It)
	{
		FilmActors.Add(*It);
	}
	for (TActorIterator<ALTTActorMark> It(World); It; ++It)
	{
		Marks.Add(*It);
	}
	FilmCamera = ALTTFilmCamera::Get(this);
	CreateHUD();
	if (bLockSetProps)
	{
		LockSetProps();
	}

	MaxScore = 0;
	for (const FLTTBeat& Beat : Scene->Beats)
	{
		if (!ShouldSkipBeat(Beat) && Beat.bHasCue && FindStation(Beat.CueSpec.Cue))
		{
			MaxScore += PointsPerfect;
		}
		else if (Beat.bHasCue && !FindStation(Beat.CueSpec.Cue))
		{
			UE_LOG(LogTemp, Warning, TEXT("SceneDirector: no station handles cue %s (beat %s); %s."), *ULTTSceneData::GetCueName(Beat.CueSpec.Cue).ToString(),
				*Beat.BeatID.ToString(), Beat.bOptional ? TEXT("the beat is skipped") : TEXT("its lines play without a cue"));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("SceneDirector: %d beats, %d stations, %d film actors, %d marks, camera %s, max score %d."),
		Scene->Beats.Num(), Stations.Num(), FilmActors.Num(), Marks.Num(), FilmCamera ? *FilmCamera->GetName() : TEXT("MISSING"), MaxScore);

	if (Scene->Beats.Num() > 0)
	{
		const FLTTBeat& First = Scene->Beats[0];
		if (FilmCamera)
		{
			FilmCamera->MoveToAct(First.Act, true);
			FilmCamera->SetConeLook(ELTTConeLook::Idle);
		}
		PlaceActorsForBeat(First);
	}
	WaitThen(StartDelay, [this]() { StartScene(); });
}

void ALTTSceneDirector::LockSetProps()
{
	for (TActorIterator<ALTTInteractableBase> It(GetWorld()); It; ++It)
	{
		It->bCanBePickedUp = false;
		if (It->Mesh)
		{
			It->Mesh->SetSimulatePhysics(false);
		}
	}
}

void ALTTSceneDirector::CreateHUD()
{
	if (!HUDClass || HUDWidget)
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

// ---- Lookups ----

ALTTEffectStation* ALTTSceneDirector::FindStation(ELTTCue Cue) const
{
	for (ALTTEffectStation* Station : Stations)
	{
		if (IsValid(Station) && Station->HandlesCue(Cue))
		{
			return Station;
		}
	}
	return nullptr;
}

ALTTFilmActor* ALTTSceneDirector::FindFilmActor(FName Name) const
{
	for (ALTTFilmActor* Actor : FilmActors)
	{
		if (IsValid(Actor) && Actor->IsNamed(Name))
		{
			return Actor;
		}
	}
	return nullptr;
}

ALTTActorMark* ALTTSceneDirector::FindMark(FName MarkName) const
{
	for (ALTTActorMark* Mark : Marks)
	{
		if (IsValid(Mark) && Mark->MarkName == MarkName)
		{
			return Mark;
		}
	}
	return nullptr;
}

bool ALTTSceneDirector::ShouldSkipBeat(const FLTTBeat& Beat) const
{
	return Beat.bOptional && (bSkipOptionalBeats || !Beat.bHasCue || !FindStation(Beat.CueSpec.Cue));
}

ELTTCue ALTTSceneDirector::FindUpcomingCue(int32 Index) const
{
	if (Scene && Scene->Beats.IsValidIndex(Index))
	{
		const FLTTBeat& Beat = Scene->Beats[Index];
		if (Beat.bHasCue && FindStation(Beat.CueSpec.Cue))
		{
			return Beat.CueSpec.Cue;
		}
	}
	return ELTTCue::None;
}

int32 ALTTSceneDirector::FindCheckpoint(int32 FromIndex) const
{
	for (int32 i = FMath::Min(FromIndex, Scene->Beats.Num() - 1); i > 0; --i)
	{
		if (Scene->Beats[i].bActCheckpoint)
		{
			return i;
		}
	}
	return 0;
}

int32 ALTTSceneDirector::GetGradeCount(ELTTGrade Grade) const
{
	return GradeCounts[FMath::Clamp(static_cast<int32>(Grade), 0, 4)];
}

int32 ALTTSceneDirector::GetStars() const
{
	const float Fraction = MaxScore > 0 ? static_cast<float>(GetScore()) / MaxScore : 0.f;
	if (Fraction >= 0.8f && CutCount == 0 && SecondsInFrame < 6.f)
	{
		return 3;
	}
	if (Fraction >= 0.55f && CutCount <= 1)
	{
		return 2;
	}
	return 1;
}

FText ALTTSceneDirector::GetWrapQuote() const
{
	switch (GetStars())
	{
	case 3: return LOCTEXT("Quote3", "\"That's the Director's Cut. Frame it. Frame YOU.\"");
	case 2: return LOCTEXT("Quote2", "\"That's a scene, Rookie. That's a real scene.\"");
	default: return LOCTEXT("Quote1", "\"We got it. Somehow. Cinema is a miracle.\"");
	}
}

float ALTTSceneDirector::GetCueTimeFraction() const
{
	if (!ActiveStation || ActiveSpec.bTutorial || CueWindow <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(1.f - CueElapsed / CueWindow, 0.f, 1.f);
}

bool ALTTSceneDirector::GetCurrentLine(FLTTDialogueLine& OutLine) const
{
	if (bLineShowing && LineQueue.IsValidIndex(LineIndex))
	{
		OutLine = LineQueue[LineIndex];
		return true;
	}
	return false;
}

FText ALTTSceneDirector::GetSpeakerLabel(const FLTTDialogueLine& Line) const
{
	if (Line.Speaker.IsNone())
	{
		return FText::GetEmpty();
	}
	FName ActorName = Line.Speaker;
	FName PartName = NAME_None;
	if (const ALTTFilmActor* Actor = FindFilmActor(Line.Speaker))
	{
		ActorName = Actor->ActorName;
		PartName = Actor->CharacterName;
	}
	else if (Line.Speaker == TEXT("Leo"))
	{
		ActorName = NAME_Dash;
		PartName = Line.Speaker;
	}
	else if (Line.Speaker == TEXT("Clara"))
	{
		ActorName = NAME_Juniper;
		PartName = Line.Speaker;
	}
	if (Line.bOffScript)
	{
		return FText::Format(LOCTEXT("OffScript", "{0} (off-script)"), FText::FromName(ActorName));
	}
	if (Line.Speaker == PartName && PartName != ActorName)
	{
		return FText::Format(LOCTEXT("InCharacter", "{0} ({1})"), FText::FromName(PartName).ToUpper(), FText::FromName(ActorName));
	}
	return FText::FromName(Line.Speaker).ToUpper();
}

ELTTGrade ALTTSceneDirector::GetGradeToast(int32& OutPoints) const
{
	OutPoints = ToastPoints;
	return ToastSecondsLeft > 0.f ? ToastGrade : ELTTGrade::None;
}

void ALTTSceneDirector::Bark(const FText& Text, float Seconds)
{
	BarkText = Text;
	BarkSecondsLeft = Seconds;
}

// ---- Stage helpers ----

float ALTTSceneDirector::GetLineSeconds(const FLTTDialogueLine& Line) const
{
	TArray<FString> Words;
	Line.Text.ToString().ParseIntoArrayWS(Words);
	return FMath::Max(Words.Num() * SecondsPerWord + 0.6f, MinLineSeconds) + Line.ExtraDelay;
}

void ALTTSceneDirector::PlayLines(const TArray<FLTTDialogueLine>& Lines, TFunction<void()> Then)
{
	LineQueue = Lines;
	LineIndex = INDEX_NONE;
	StageThen = MoveTemp(Then);
	Stage = EStage::Lines;
	ShowNextLine();
}

void ALTTSceneDirector::ShowNextLine()
{
	++LineIndex;
	if (!LineQueue.IsValidIndex(LineIndex))
	{
		bLineShowing = false;
		Stage = EStage::Idle;
		TFunction<void()> Then = MoveTemp(StageThen);
		StageThen = nullptr;
		if (Then)
		{
			Then();
		}
		return;
	}

	const FLTTDialogueLine& Line = LineQueue[LineIndex];
	bLineShowing = true;
	LineSecondsLeft = GetLineSeconds(Line);
	if (USoundBase* Voice = Line.Voice.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Voice);
		const float Duration = Voice->GetDuration();
		if (Duration > 0.f && Duration < 60.f)
		{
			LineSecondsLeft = Duration + 0.25f + Line.ExtraDelay;
		}
	}
	if (ALTTFilmActor* Speaker = FindFilmActor(Line.Speaker))
	{
		Speaker->Speak(Line);
	}
	if (!Line.MoveToMark.IsNone())
	{
		ALTTFilmActor* Mover = FindFilmActor(Line.MoveActor.IsNone() ? Line.Speaker : Line.MoveActor);
		ALTTActorMark* Mark = FindMark(Line.MoveToMark);
		if (Mover && Mark)
		{
			Mover->MoveToMark(Mark, Line.Montage == NAME_StormOff);
		}
		else if (Mover)
		{
			UE_LOG(LogTemp, Warning, TEXT("SceneDirector: no ALTTActorMark named %s in the level."), *Line.MoveToMark.ToString());
		}
	}
	OnLineSpoken.Broadcast(Line);
}

void ALTTSceneDirector::WaitThen(float Seconds, TFunction<void()> Then)
{
	Stage = EStage::Wait;
	WaitSecondsLeft = Seconds;
	StageThen = MoveTemp(Then);
}

void ALTTSceneDirector::StopEverything()
{
	if (ActiveStation)
	{
		ActiveStation->SetCueActive(ActiveSpec.Cue, false);
		ActiveStation = nullptr;
	}
	bSlateActive = false;
	LineQueue.Reset();
	LineIndex = INDEX_NONE;
	bLineShowing = false;
	Stage = EStage::Idle;
	StageThen = nullptr;
	UpcomingCue = ELTTCue::None;
}

void ALTTSceneDirector::SetPlayerInputBlocked(bool bBlocked)
{
	if (bBlocked == bPlayerInputBlocked)
	{
		return;
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetIgnoreMoveInput(bBlocked);
		PC->SetIgnoreLookInput(bBlocked);
		bPlayerInputBlocked = bBlocked;
	}
}

// ---- Scene flow ----

void ALTTSceneDirector::StartScene()
{
	if (bPlayPreRoll && Scene->PreRoll.Num() > 0)
	{
		PlayLines(Scene->PreRoll, [this]() { StartSlate(); });
	}
	else
	{
		StartRolling();
	}
}

void ALTTSceneDirector::StartSlate()
{
	ALTTEffectStation* Slate = FindStation(ELTTCue::Slate);
	if (!Slate)
	{
		PlayLines(Scene->SlateDone, [this]() { StartRolling(); });
		return;
	}
	ActiveSpec = FLTTCueSpec();
	ActiveSpec.Cue = ELTTCue::Slate;
	ActiveSpec.bTutorial = true;
	ActiveStation = Slate;
	bSlateActive = true;
	CueElapsed = 0.f;
	CueWindow = 0.f;
	Slate->SetCueActive(ELTTCue::Slate, true);
	Stage = EStage::Cue;
	OnCueCalled.Broadcast(ELTTCue::Slate, 0.f);
}

void ALTTSceneDirector::StartRolling()
{
	bRolling = true;
	RollSeconds = 0.f;
	CheckpointScore = Score;
	StartBeat(0);
}

void ALTTSceneDirector::StartBeat(int32 Index)
{
	while (Scene->Beats.IsValidIndex(Index) && ShouldSkipBeat(Scene->Beats[Index]))
	{
		++Index;
	}
	if (!Scene->Beats.IsValidIndex(Index))
	{
		StartFinale();
		return;
	}
	BeatIndex = Index;
	const FLTTBeat& Beat = Scene->Beats[Index];
	if (Beat.bActCheckpoint && Beat.Act != CurrentAct)
	{
		EnterAct(Beat, false);
		return;
	}
	PlayBeatFrom(Index);
}

void ALTTSceneDirector::EnterAct(const FLTTBeat& Beat, bool bInstantCamera)
{
	CurrentAct = Beat.Act;
	CheckpointScore = Score;
	FMemory::Memcpy(CheckpointCounts, GradeCounts, sizeof(GradeCounts));

	// No penalties while the camera travels and the title card is up.
	bFrameChecksPaused = true;
	const float MoveSeconds = FilmCamera ? FilmCamera->MoveToAct(Beat.Act, bInstantCamera) : 0.f;
	const float Hold = FMath::Max(MoveSeconds, ActCardSeconds);
	if (const FText* Title = Scene->ActTitles.Find(Beat.Act))
	{
		ActCardText = *Title;
		ActCardSecondsLeft = Hold;
	}
	OnActStarted.Broadcast(Beat.Act);
	const int32 Index = BeatIndex;
	WaitThen(Hold, [this, Index]() { PlayBeatFrom(Index); });
}

void ALTTSceneDirector::PlayBeatFrom(int32 Index)
{
	bFrameChecksPaused = false;
	BeatIndex = Index;
	UpcomingCue = FindUpcomingCue(Index);
	PlayLines(Scene->Beats[Index].PreDialogue, [this]() { CallCue(); });
}

void ALTTSceneDirector::CallCue()
{
	const FLTTBeat& Beat = Scene->Beats[BeatIndex];
	ALTTEffectStation* Station = Beat.bHasCue ? FindStation(Beat.CueSpec.Cue) : nullptr;
	UpcomingCue = ELTTCue::None;
	const int32 Next = BeatIndex + 1;
	if (!Station)
	{
		// Nothing to operate: the scene carries on as if it went fine.
		TArray<FLTTDialogueLine> Lines = Beat.GetPost(ELTTGrade::Good);
		Lines.Append(Beat.PostAlways);
		PlayLines(Lines, [this, Next]() { StartBeat(Next); });
		return;
	}

	ActiveSpec = Beat.CueSpec;
	ActiveStation = Station;
	CueElapsed = 0.f;
	float Travel = 0.f;
	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		// Straight-line distance, with height counted double for the stairs.
		const FVector Delta = Station->GetActorLocation() - Pawn->GetActorLocation();
		const float Distance = static_cast<float>(Delta.Size2D() + FMath::Abs(Delta.Z) * 2.0);
		Travel = FMath::Clamp(Distance / FMath::Max(TravelSpeed, 1.f) * 1.15f, 0.f, MaxTravelBonus);
	}
	CueWindow = (ActiveSpec.BaseWindow + Travel) * WindowScale;
	Station->SetCueActive(ActiveSpec.Cue, true);
	Bark(ActiveSpec.CallLine, 4.5f);
	if (CueCallSound)
	{
		UGameplayStatics::PlaySound2D(this, CueCallSound);
	}
	Stage = EStage::Cue;
	OnCueCalled.Broadcast(ActiveSpec.Cue, CueWindow);
}

void ALTTSceneDirector::ReportCueResult(ALTTEffectStation* Station, ELTTGrade Grade)
{
	if (Stage == EStage::Cue && Station && Station == ActiveStation)
	{
		ResolveCue(Grade);
	}
}

void ALTTSceneDirector::ResolveCue(ELTTGrade StationGrade)
{
	ALTTEffectStation* Station = ActiveStation;
	const FLTTCueSpec Spec = ActiveSpec;
	ActiveStation = nullptr;
	Stage = EStage::Idle;
	if (Station)
	{
		Station->SetCueActive(Spec.Cue, false);
	}

	if (Spec.bTutorial)
	{
		bSlateActive = false;
		if (Station)
		{
			Station->PlayEffect(Spec.Cue, ELTTGrade::Perfect);
		}
		PlayLines(Scene->SlateDone, [this]() { StartRolling(); });
		return;
	}

	ELTTGrade Grade = StationGrade == ELTTGrade::None ? ELTTGrade::Miss : StationGrade;
	if (Spec.bGradeBySpeed && Grade != ELTTGrade::Miss && CueWindow > 0.f)
	{
		const float Fraction = CueElapsed / CueWindow;
		const ELTTGrade TimeGrade = Fraction <= Spec.PerfectFrac ? ELTTGrade::Perfect : Fraction <= Spec.GoodFrac ? ELTTGrade::Good : ELTTGrade::Late;
		Grade = static_cast<ELTTGrade>(FMath::Max(static_cast<uint8>(Grade), static_cast<uint8>(TimeGrade))); // the worse of the two
	}

	int32 Points = 0;
	float MeterDelta = MeterMiss;
	switch (Grade)
	{
	case ELTTGrade::Perfect: Points = PointsPerfect; MeterDelta = MeterPerfect; break;
	case ELTTGrade::Good: Points = PointsGood; MeterDelta = MeterGood; break;
	case ELTTGrade::Late: Points = PointsLate; MeterDelta = MeterLate; break;
	default: break;
	}
	Score += Points;
	TakeMeter = FMath::Clamp(TakeMeter + MeterDelta, 0.f, 100.f);
	++GradeCounts[static_cast<int32>(Grade)];
	ToastGrade = Grade;
	ToastPoints = Points;
	ToastSecondsLeft = 2.2f;

	if (Station)
	{
		Station->PlayEffect(Spec.Cue, Grade);
	}
	ApplyCueToWorld(Spec.Cue, Grade);
	ReceiveCueEffect(Spec.Cue, Grade);
	for (ALTTFilmActor* Actor : FilmActors)
	{
		if (IsValid(Actor))
		{
			Actor->ReceiveCueResult(Spec.Cue, Grade);
		}
	}
	if (USoundBase* Sound = Grade == ELTTGrade::Miss ? CueMissSound.Get() : CueGoodSound.Get())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
	OnCueResolved.Broadcast(Spec.Cue, Grade, Points);
	UE_LOG(LogTemp, Log, TEXT("SceneDirector: %s -> %s (+%d, meter %.0f)."), *ULTTSceneData::GetCueName(Spec.Cue).ToString(),
		*ULTTSceneData::GetGradeName(Grade).ToString(), Points, TakeMeter);

	if (TakeMeter <= 0.f && bAllowCut)
	{
		CallCut();
		return;
	}

	const FLTTBeat& Beat = Scene->Beats[BeatIndex];
	TArray<FLTTDialogueLine> Lines = Beat.GetPost(Grade);
	Lines.Append(Beat.PostAlways);
	// Marlowe reacts only when the scripted lines don't already have him talking (and never over the last kiss).
	const bool bMarloweScripted = Lines.ContainsByPredicate([](const FLTTDialogueLine& L) { return L.Speaker == NAME_Marlowe; });
	if (!bMarloweScripted && BeatIndex < Scene->Beats.Num() - 1)
	{
		const FText Reaction = GetResultBark(Spec.Cue, Grade);
		if (!Reaction.IsEmpty())
		{
			Bark(Reaction);
		}
	}
	const int32 Next = BeatIndex + 1;
	PlayLines(Lines, [this, Next]() { StartBeat(Next); });
}

void ALTTSceneDirector::StartFinale()
{
	bFrameChecksPaused = true;
	bFinale = true;
	UpcomingCue = ELTTCue::None;
	// The player keeps moving through the wrap; frame checks are already off, so nothing can cost points now.
	if (WrapSound)
	{
		UGameplayStatics::PlaySound2D(this, WrapSound);
	}
	PlayLines(Scene->Wrap, [this]() { Wrap(); });
}

void ALTTSceneDirector::Wrap()
{
	bRolling = false;
	bWrapped = true;
	Stage = EStage::Wrapped;
	OnSceneWrapped.Broadcast(GetScore(), GetStars());
	UE_LOG(LogTemp, Log, TEXT("SceneDirector: wrap. Score %d / %d, %d cuts, %.1f s in frame, %d stars."), GetScore(), MaxScore, CutCount, SecondsInFrame, GetStars());
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &ALTTSceneDirector::ShowEndScreen, 1.f, false);
}

void ALTTSceneDirector::ShowEndScreen()
{
	SetPlayerInputBlocked(false);
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
	if (ULTTShootEndScreen* Screen = Cast<ULTTShootEndScreen>(EndScreenWidget))
	{
		Screen->Setup(this);
	}
	EndScreenWidget->AddToViewport(10);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	// Not paused and no UI-only focus: the player can keep walking the set behind the end screen.
	ACleanCutActionPlayerController::KeepGameInputWithCursor(PC);
}

// ---- CUT / retake ----

void ALTTSceneDirector::CallCut()
{
	if (!bRolling || bCutShowing || bWrapped)
	{
		return;
	}
	StopEverything();
	++CutCount;
	bCutShowing = true;
	bFrameChecksPaused = true;
	bPlayerInFrame = false;
	const FText CutLines[] = {
		LOCTEXT("Cut1", "CUT! CUT CUT CUT!"),
		LOCTEXT("Cut2", "CUT! That was a disaster! A beautiful disaster, but a disaster!"),
		LOCTEXT("Cut3", "CUT! Reset! Somebody get Dash a towel and me a sedative!") };
	Bark(CutLines[FMath::RandRange(0, 2)], 2.6f);
	if (CutSound)
	{
		UGameplayStatics::PlaySound2D(this, CutSound);
	}
	for (ALTTFilmActor* Actor : FilmActors)
	{
		if (IsValid(Actor))
		{
			Actor->SetFrozen(true);
		}
	}
	OnCut.Broadcast(GetTakeNumber());
	UE_LOG(LogTemp, Log, TEXT("SceneDirector: CUT. Take %d restarts act %s."), GetTakeNumber(), *CurrentAct.ToString());
	const int32 Checkpoint = FindCheckpoint(BeatIndex);
	WaitThen(2.5f, [this, Checkpoint]() { ResetToCheckpoint(Checkpoint, true); });
}

void ALTTSceneDirector::ResetToCheckpoint(int32 CheckpointIndex, bool bPenalty)
{
	if (!Scene->Beats.IsValidIndex(CheckpointIndex))
	{
		return;
	}
	const FLTTBeat& Beat = Scene->Beats[CheckpointIndex];
	bCutShowing = false;
	BeatIndex = CheckpointIndex;
	CurrentAct = Beat.Act;
	TakeMeter = MeterStart;
	FrameStreak = 0.f;

	// Points earned in the act being redone are handed back; each CUT costs its penalty.
	Score = CheckpointScore + (bPenalty ? PointsPerCut : 0);
	CheckpointScore = Score;
	FMemory::Memcpy(GradeCounts, CheckpointCounts, sizeof(GradeCounts));

	for (ALTTFilmActor* Actor : FilmActors)
	{
		if (IsValid(Actor))
		{
			Actor->SetFrozen(false);
		}
	}
	for (ALTTEffectStation* Station : Stations)
	{
		if (IsValid(Station))
		{
			Station->ResetStation();
		}
	}
	ParamTweens.Reset();
	for (const FName Param : { Param_Rain, Param_Wetness, Param_Flash, Param_Wind, Param_Backdrop })
	{
		SetWorldParam(Param, 0.f, 0.f);
	}
	ReceiveTakeReset(Beat.Act);
	// What was already on when this act first started (the rain in Act C).
	for (const ELTTCue Cue : Beat.RestoreEffects)
	{
		if (ALTTEffectStation* Station = FindStation(Cue))
		{
			Station->PlayEffect(Cue, ELTTGrade::Good);
		}
		ApplyCueToWorld(Cue, ELTTGrade::Good);
		ReceiveCueEffect(Cue, ELTTGrade::Good);
	}

	if (FilmCamera)
	{
		FilmCamera->MoveToAct(Beat.Act, true);
	}
	PlaceActorsForBeat(Beat);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor::Black);
		}
	}

	TArray<FLTTDialogueLine> Restart;
	if (const FText* RestartText = Scene->RestartLines.Find(Beat.Act))
	{
		FLTTDialogueLine& Line = Restart.AddDefaulted_GetRef();
		Line.Speaker = NAME_Marlowe;
		Line.Text = *RestartText;
	}
	PlayLines(Restart, [this, CheckpointIndex]() { PlayBeatFrom(CheckpointIndex); });
}

void ALTTSceneDirector::PlaceActorsForBeat(const FLTTBeat& Beat)
{
	auto Place = [this](FName ActorName, FName MarkName)
	{
		ALTTFilmActor* Actor = FindFilmActor(ActorName);
		ALTTActorMark* Mark = MarkName.IsNone() ? nullptr : FindMark(MarkName);
		if (Actor && Mark)
		{
			Actor->TeleportToMark(Mark);
		}
		else if (Actor && !MarkName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("SceneDirector: no ALTTActorMark named %s in the level."), *MarkName.ToString());
		}
	};
	Place(NAME_Dash, Beat.DashStartMark);
	Place(NAME_Juniper, Beat.JuniperStartMark);
}

void ALTTSceneDirector::CheatSkipCue(ELTTGrade Grade)
{
	if (Stage == EStage::Cue && ActiveStation)
	{
		ActiveStation->ExitStation();
		ResolveCue(Grade);
	}
}

void ALTTSceneDirector::CheatJumpToAct(FName Act)
{
	const int32 Index = Scene ? Scene->Beats.IndexOfByPredicate([Act](const FLTTBeat& B) { return B.bActCheckpoint && B.Act == Act; }) : INDEX_NONE;
	if (Index == INDEX_NONE || bWrapped)
	{
		UE_LOG(LogTemp, Warning, TEXT("SceneDirector: no act %s to jump to."), *Act.ToString());
		return;
	}
	StopEverything();
	bRolling = true;
	bFinale = false;
	SetPlayerInputBlocked(false);
	ResetToCheckpoint(Index, false);
}

// ---- Tick ----

void ALTTSceneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRolling && !bCutShowing)
	{
		RollSeconds += DeltaSeconds;
	}
	BarkSecondsLeft = FMath::Max(BarkSecondsLeft - DeltaSeconds, 0.f);
	ActCardSecondsLeft = FMath::Max(ActCardSecondsLeft - DeltaSeconds, 0.f);
	ToastSecondsLeft = FMath::Max(ToastSecondsLeft - DeltaSeconds, 0.f);
	FrameBarkCooldown = FMath::Max(FrameBarkCooldown - DeltaSeconds, 0.f);
	TickWorldParams(DeltaSeconds);

	switch (Stage)
	{
	case EStage::Lines:
		LineSecondsLeft -= DeltaSeconds;
		if (LineSecondsLeft <= 0.f)
		{
			ShowNextLine();
		}
		break;

	case EStage::Wait:
		WaitSecondsLeft -= DeltaSeconds;
		if (WaitSecondsLeft <= 0.f)
		{
			Stage = EStage::Idle;
			TFunction<void()> Then = MoveTemp(StageThen);
			StageThen = nullptr;
			if (Then)
			{
				Then();
			}
		}
		break;

	case EStage::Cue:
		if (!ActiveSpec.bTutorial)
		{
			CueElapsed += DeltaSeconds;
			if (CueElapsed >= CueWindow)
			{
				ResolveCue(ActiveStation ? ActiveStation->HandleCueTimeout() : ELTTGrade::Miss);
			}
		}
		break;

	default:
		break;
	}

	TickFrameCheck(DeltaSeconds);
}

void ALTTSceneDirector::TickFrameCheck(float Dt)
{
	const bool bChecking = bRolling && !bFrameChecksPaused && !bSlateActive && !bCutShowing && FilmCamera && !FilmCamera->IsMoving();
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	bPlayerInFrame = bChecking && Pawn && FilmCamera->IsPawnInFrame(Pawn);

	if (FilmCamera)
	{
		// The cone is up from the pre-roll (Marlowe points it out) until the finale.
		const ELTTConeLook Look = (bWrapped || bFinale) ? ELTTConeLook::Hidden
			: bSlateActive ? ELTTConeLook::Allowed
			: bPlayerInFrame ? ELTTConeLook::Alert : ELTTConeLook::Idle;
		if (static_cast<uint8>(Look) != LastConeLook)
		{
			LastConeLook = static_cast<uint8>(Look);
			FilmCamera->SetConeLook(Look);
		}
	}

	if (!bPlayerInFrame)
	{
		FrameStreak = 0.f;
		return;
	}
	FrameStreak += Dt;
	if (FrameStreak < FrameGrace)
	{
		return;
	}
	SecondsInFrame += Dt;
	TakeMeter = FMath::Clamp(TakeMeter + MeterPerSecondInFrame * Dt, 0.f, 100.f);
	FramePenaltyRemainder += PointsPerSecondInFrame * Dt;
	const int32 Whole = FMath::TruncToInt(FramePenaltyRemainder);
	Score += Whole;
	FramePenaltyRemainder -= Whole;

	if (FrameBarkCooldown <= 0.f)
	{
		FrameBarkCooldown = 8.f;
		const FText Lines[] = {
			LOCTEXT("InFrame1", "Rookie, you're IN THE SHOT!"),
			LOCTEXT("InFrame2", "Get out of frame! You're not in this movie!"),
			LOCTEXT("InFrame3", "Why is there a Rookie in my romance?!"),
			LOCTEXT("InFrame4", "Duck! DUCK! Not a real duck! Get DOWN!"),
			LOCTEXT("InFrame5", "The audience will think you're the love interest! Move!") };
		Bark(Lines[FMath::RandRange(0, 4)]);
	}
	if (TakeMeter <= 0.f && bAllowCut)
	{
		CallCut();
	}
}

// ---- World (MPC_World) ----

void ALTTSceneDirector::SetWorldParam(FName Name, float Target, float Seconds)
{
	ParamTweens.RemoveAll([Name](const FWorldParamTween& T) { return T.Name == Name; });
	if (Seconds <= 0.f)
	{
		ParamValues.Add(Name, Target);
		if (WorldMPC)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(this, WorldMPC, Name, Target);
		}
		return;
	}
	FWorldParamTween& Tween = ParamTweens.AddDefaulted_GetRef();
	Tween.Name = Name;
	Tween.From = ParamValues.FindRef(Name);
	Tween.To = Target;
	Tween.Seconds = Seconds;
}

void ALTTSceneDirector::TickWorldParams(float Dt)
{
	for (int32 i = ParamTweens.Num() - 1; i >= 0; --i)
	{
		FWorldParamTween& Tween = ParamTweens[i];
		Tween.Elapsed += Dt;
		const float A = FMath::Clamp(Tween.Elapsed / Tween.Seconds, 0.f, 1.f);
		const float Value = FMath::Lerp(Tween.From, Tween.To, A);
		ParamValues.Add(Tween.Name, Value);
		if (WorldMPC)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(this, WorldMPC, Tween.Name, Value);
		}
		if (A >= 1.f)
		{
			ParamTweens.RemoveAtSwap(i);
		}
	}
	if (FlashSecondsLeft > 0.f)
	{
		FlashSecondsLeft = FMath::Max(FlashSecondsLeft - Dt, 0.f);
		// Double flicker: on, dip, on, off.
		const float A = 1.f - FlashSecondsLeft / FlashSeconds;
		const float Value = FlashSecondsLeft <= 0.f ? 0.f : (A < 0.3f || (A > 0.45f && A < 0.8f)) ? 1.f : 0.2f;
		ParamValues.Add(Param_Flash, Value);
		if (WorldMPC)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(this, WorldMPC, Param_Flash, Value);
		}
	}
}

void ALTTSceneDirector::ApplyCueToWorld(ELTTCue Cue, ELTTGrade Grade)
{
	const bool bMissed = Grade == ELTTGrade::Miss;
	switch (Cue)
	{
	case ELTTCue::Rain:
		SetWorldParam(Param_Rain, bMissed ? 0.2f : 1.f, 2.f);
		SetWorldParam(Param_Wetness, bMissed ? 0.2f : 1.f, 6.f); // lags behind the rain, never dries in-scene
		break;
	case ELTTCue::Thunder:
		if (!bMissed)
		{
			FlashSecondsLeft = FlashSeconds;
		}
		break;
	case ELTTCue::Sunset:
		if (!bMissed)
		{
			SetWorldParam(Param_Backdrop, 1.f, 2.f);
		}
		break;
	case ELTTCue::Moonlight:
		if (!bMissed)
		{
			SetWorldParam(Param_Backdrop, 2.f, 2.f);
		}
		break;
	case ELTTCue::Wind:
		SetWorldParam(Param_Wind, bMissed ? 0.f : 0.65f, 1.f);
		break;
	default:
		break;
	}
}

// ---- Marlowe's reactions (Docs/05 §2.4) ----

FText ALTTSceneDirector::GetResultBark(ELTTCue Cue, ELTTGrade Grade)
{
	const int32 G = static_cast<int32>(Grade) - 1; // Perfect, Good, Late, Miss
	if (G < 0 || G > 3)
	{
		return FText::GetEmpty();
	}
	const TCHAR* Lines[4] = { nullptr, nullptr, nullptr, nullptr };
	auto Set = [&Lines](const TCHAR* P, const TCHAR* Go, const TCHAR* L, const TCHAR* M) { Lines[0] = P; Lines[1] = Go; Lines[2] = L; Lines[3] = M; };
	switch (Cue)
	{
	case ELTTCue::Traffic: Set(TEXT("Gorgeous traffic! I believe in this city!"), TEXT("Nice. A little jerky, like real traffic."), TEXT("Late traffic. Also realistic."), TEXT("Where's my CAR?")); break;
	case ELTTCue::Door: Set(TEXT("Ding! Perfect timing!"), TEXT("Good ding."), TEXT("Late ding. Still a ding."), TEXT("That was a ding for a ghost.")); break;
	case ELTTCue::Steam: Set(TEXT("Look at that steam! I can smell it from here!"), TEXT("Nice puff."), TEXT("A late puff."), TEXT("No steam?!")); break;
	case ELTTCue::Napkins: Set(TEXT("Perfect toss! He even caught it!"), TEXT("Caught! Barely! Good!"), TEXT("He had to bend down. Undignified, but fine."), TEXT("Not his FACE!")); break;
	case ELTTCue::Thunder: Set(TEXT("THAT'S thunder!"), TEXT("Good rumble!"), TEXT("A little late, like a real storm."), TEXT("That was a burp. That was a weather burp.")); break;
	case ELTTCue::Sunset: Set(TEXT("Magic hour! I'm weeping!"), TEXT("Lovely orange."), TEXT("The sun took its time."), TEXT("Where's my sunset?!")); break;
	case ELTTCue::Wind: Set(TEXT("Shampoo commercial! Perfect!"), TEXT("Nice breeze."), TEXT("Breeze arrived late."), TEXT("Her hair is not moving!")); break;
	case ELTTCue::Boom: Set(TEXT("Clean audio! Clean frame! I love you!"), TEXT("Good sound."), TEXT("We'll fix it in post."), TEXT("BOOM IN SHOT!")); break;
	case ELTTCue::Rain: Set(TEXT("LOOK AT THAT RAIN! I'm a GENIUS! You're a genius! We're all wet!"), TEXT("Rain! Real-looking rain!"), TEXT("Late rain! Still rain!"), TEXT("That's a drizzle! I asked for drama, not humidity!")); break;
	case ELTTCue::Umbrella: Set(TEXT("Perfect throw!"), TEXT("Good catch, Dash!"), TEXT("Close enough!"), TEXT("Stop hitting my lead actor!")); break;
	case ELTTCue::Roses: Set(TEXT("Rose delivery! Romantic!"), TEXT("Nice!"), TEXT("Slightly crushed. Like love."), TEXT("They're in the street!")); break;
	case ELTTCue::Moonlight: Set(TEXT("Moonlight! Perfect!"), TEXT("Nice blue."), TEXT("The moon rose slowly."), TEXT("Is the moon SICK?")); break;
	default: break;
	}
	return Lines[G] ? FText::FromString(Lines[G]) : FText::GetEmpty();
}

// ---- Console ----

static FAutoConsoleCommandWithWorldAndArgs GLTTSkipCueCommand(
	TEXT("LTT.SkipCue"),
	TEXT("LTT.SkipCue [Perfect|Good|Late|Miss] - resolves the active Phase 2 cue (default Good)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ELTTGrade Grade = ELTTGrade::Good;
		if (Args.Num() > 0)
		{
			Grade = Args[0].Equals(TEXT("Perfect"), ESearchCase::IgnoreCase) ? ELTTGrade::Perfect
				: Args[0].Equals(TEXT("Late"), ESearchCase::IgnoreCase) ? ELTTGrade::Late
				: Args[0].Equals(TEXT("Miss"), ESearchCase::IgnoreCase) ? ELTTGrade::Miss : ELTTGrade::Good;
		}
		if (ALTTSceneDirector* Director = ALTTSceneDirector::Get(World))
		{
			Director->CheatSkipCue(Grade);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GLTTJumpToActCommand(
	TEXT("LTT.JumpToAct"),
	TEXT("LTT.JumpToAct <A|B|C> - restarts the Phase 2 scene from the first beat of an act."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ALTTSceneDirector* Director = ALTTSceneDirector::Get(World);
		if (Director && Args.Num() > 0)
		{
			Director->CheatJumpToAct(FName(*Args[0].ToUpper()));
		}
	}));

static FAutoConsoleCommandWithWorld GLTTCutCommand(
	TEXT("LTT.Cut"),
	TEXT("LTT.Cut - calls CUT in the Phase 2 scene (the current act restarts)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ALTTSceneDirector* Director = ALTTSceneDirector::Get(World))
		{
			Director->CallCut();
		}
	}));

#undef LOCTEXT_NAMESPACE
