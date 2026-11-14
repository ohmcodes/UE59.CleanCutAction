// Clean! Cut! Action! - Phase 2 scene director (Docs/04 §2, §5, §7). Place one in the shoot level.
// Runs the scene beat by beat: the actors' lines, Marlowe's cue call, the wait for the player to work the
// matching ALTTEffectStation, then the lines for the grade they got. Keeps the score and the take meter,
// docks the player for standing in the film camera's frame, calls CUT when the meter is empty (the act
// restarts), and opens the end screen after the wrap. Also adds the shoot HUD (ULTTShootHUD).
//
// Level checklist: one ALTTSceneDirector, one ALTTFilmCamera (+ an anchor actor per act), two ALTTFilmActor
// Blueprints (Dash, Juniper), an ALTTActorMark per mark name, and an ALTTEffectStation per cue. Anything
// missing is skipped with a log warning, so the scene can be tested while the level is still being built.
// Console: LTT.SkipCue [Perfect|Good|Late|Miss], LTT.JumpToAct <A|B|C>, LTT.Cut.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shoot/LTTSceneData.h"
#include "LTTSceneDirector.generated.h"

class ALTTEffectStation;
class ALTTFilmActor;
class ALTTActorMark;
class ALTTFilmCamera;
class UUserWidget;
class UMaterialParameterCollection;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnActStarted, FName, Act);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnCueCalled, ELTTCue, Cue, float, Window);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLTTOnCueResolved, ELTTCue, Cue, ELTTGrade, Grade, int32, Points);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnCut, int32, TakeNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnSceneWrapped, int32, Score, int32, Stars);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnLineSpoken, const FLTTDialogueLine&, Line);

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTSceneDirector : public AActor
{
	GENERATED_BODY()

public:
	ALTTSceneDirector();

	/** First scene director in the world, or null. */
	static ALTTSceneDirector* Get(const UObject* WorldContext);

	// ---- Scene ----

	/** The script. Empty = the built-in Scene 42 (Docs/05 §2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TObjectPtr<ULTTSceneData> SceneData;

	/** Leave out the beats marked optional (Door, Wind, Moonlight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	bool bSkipOptionalBeats = false;

	/** Seconds after BeginPlay before Marlowe starts talking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene", meta = (ClampMin = "0"))
	float StartDelay = 1.5f;

	/** Play Marlowe's pre-roll speech and the slate tutorial. Off = straight to "ACTION!". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	bool bPlayPreRoll = true;

	/** Phase 1 props (ALTTInteractableBase) are frozen in place and can't be picked up during the shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	bool bLockSetProps = true;

	// ---- Dialogue pacing (lines without a voice-over) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (ClampMin = "0.05"))
	float SecondsPerWord = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (ClampMin = "0.5"))
	float MinLineSeconds = 1.6f;

	/** Seconds the act title card stays up when there is no camera move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (ClampMin = "0"))
	float ActCardSeconds = 2.5f;

	// ---- Cue window ----

	/** Player run speed used to add travel time to a cue window (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cues", meta = (ClampMin = "100"))
	float TravelSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cues", meta = (ClampMin = "0"))
	float MaxTravelBonus = 12.f;

	/** Multiplies every cue window (easier > 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cues", meta = (ClampMin = "0.25"))
	float WindowScale = 1.f;

	// ---- Scoring (Docs/04 §5) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsPerfect = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsGood = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsLate = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsPerCut = -50;

	/** Points lost per second in the camera's frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	float PointsPerSecondInFrame = -10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterStart = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterPerfect = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterGood = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterLate = -3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterMiss = -15.f;

	/** Take meter lost per second in frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	float MeterPerSecondInFrame = -4.f;

	/** Seconds in frame before it starts to cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter", meta = (ClampMin = "0"))
	float FrameGrace = 0.25f;

	/** Off = the meter can't trigger a CUT (story mode / testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take Meter")
	bool bAllowCut = true;

	// ---- World ----

	/** Optional MPC_World. The director drives its scalars RainIntensity, Wetness, LightningFlash, WindStrength and
	 *  BackdropTime (0 day, 1 sunset, 2 night) when cues land. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TObjectPtr<UMaterialParameterCollection> WorldMPC;

	// ---- Sounds (all optional) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> CueCallSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> CueGoodSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> CueMissSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> CutSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> WrapSound;

	// ---- HUD / end screen ----

	/** ULTTShootHUD or a Widget Blueprint based on it. Left as the C++ class, WBP_ShootHUD is used (edit that in the UMG designer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	TSubclassOf<UUserWidget> HUDClass;

	/** ULTTShootEndScreen or a Widget Blueprint based on it. Left as the C++ class, WBP_ShootEndScreen is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	TSubclassOf<UUserWidget> EndScreenClass;

	/** Level the end screen's "Back to Cleanup" button opens. None hides the button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FName CleanupLevelName = TEXT("/Game/StylizedNewYorkCity/Level/L_NYC_Cafe");

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnActStarted OnActStarted;

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnCueCalled OnCueCalled;

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnCueResolved OnCueResolved;

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnCut OnCut;

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnSceneWrapped OnSceneWrapped;

	UPROPERTY(BlueprintAssignable, Category = "Scene")
	FLTTOnLineSpoken OnLineSpoken;

	/** Blueprint hook: a cue landed. Drive weather, lights and VFX that aren't a station's effect actors. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (DisplayName = "On Cue Effect"))
	void ReceiveCueEffect(ELTTCue Cue, ELTTGrade Grade);

	/** Blueprint hook: the take was reset (CUT or act jump). Put the weather back. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (DisplayName = "On Take Reset"))
	void ReceiveTakeReset(FName Act);

	// ---- State (read by the HUD, stations and end screen) ----

	/** The camera is rolling: from "ACTION!" to the wrap. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	bool IsRolling() const { return bRolling; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	float GetRollSeconds() const { return RollSeconds; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	FName GetCurrentAct() const { return CurrentAct; }

	/** 0-100. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	float GetTakeMeter() const { return TakeMeter; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetScore() const { return FMath::Max(Score, 0); }

	/** 100 x the number of scored cues in this run of the scene. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetMaxScore() const { return MaxScore; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetCutCount() const { return CutCount; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetTakeNumber() const { return CutCount + 1; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	float GetSecondsInFrame() const { return SecondsInFrame; }

	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetGradeCount(ELTTGrade Grade) const;

	/** 1-3, valid once the scene wrapped. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	int32 GetStars() const;

	UFUNCTION(BlueprintPure, Category = "Scene")
	bool IsPlayerInFrame() const { return bPlayerInFrame; }

	UFUNCTION(BlueprintPure, Category = "Cues")
	bool IsCueActive() const { return ActiveStation != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Cues")
	ELTTCue GetActiveCue() const { return ActiveSpec.Cue; }

	UFUNCTION(BlueprintPure, Category = "Cues")
	ALTTEffectStation* GetActiveStation() const { return ActiveStation; }

	/** 1 when the cue is called, 0 when it's missed. Always 1 for the slate. */
	UFUNCTION(BlueprintPure, Category = "Cues")
	float GetCueTimeFraction() const;

	/** The cue of the beat being played, before it's called ("NEXT: RAIN"). None if there is none. */
	UFUNCTION(BlueprintPure, Category = "Cues")
	ELTTCue GetUpcomingCue() const { return UpcomingCue; }

	UFUNCTION(BlueprintPure, Category = "Cues")
	ALTTEffectStation* FindStation(ELTTCue Cue) const;

	UFUNCTION(BlueprintPure, Category = "Scene")
	ALTTFilmActor* FindFilmActor(FName Name) const;

	UFUNCTION(BlueprintPure, Category = "Scene")
	ALTTActorMark* FindMark(FName MarkName) const;

	/** The subtitle on screen. False if nobody is talking. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool GetCurrentLine(FLTTDialogueLine& OutLine) const;

	/** "LEO (Dash)", "Dash (off-script)", "MARLOWE". */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetSpeakerLabel(const FLTTDialogueLine& Line) const;

	/** Marlowe's megaphone line (cue calls, heckles). Empty when none is up. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetBarkText() const { return BarkSecondsLeft > 0.f ? BarkText : FText::GetEmpty(); }

	/** "ACT II - THE TERRACE" while the title card is up. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	FText GetActCardText() const { return ActCardSecondsLeft > 0.f ? ActCardText : FText::GetEmpty(); }

	/** True while the CUT interstitial is up. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	bool IsCutShowing() const { return bCutShowing; }

	/** The last cue result, for the "PERFECT! +100" toast. Grade None once it has faded. */
	UFUNCTION(BlueprintPure, Category = "Cues")
	ELTTGrade GetGradeToast(int32& OutPoints) const;

	UFUNCTION(BlueprintPure, Category = "Scene")
	bool HasWrapped() const { return bWrapped; }

	/** Marlowe's end-screen quote for the stars earned. */
	UFUNCTION(BlueprintPure, Category = "Scene")
	FText GetWrapQuote() const;

	// ---- Actions ----

	/** Shows a megaphone line from Marlowe over the HUD without interrupting the scene. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Bark(const FText& Text, float Seconds = 3.5f);

	/** Called by the active station when its mini-game ends. */
	void ReportCueResult(ALTTEffectStation* Station, ELTTGrade Grade);

	/** Stops the take and restarts the current act. */
	UFUNCTION(BlueprintCallable, Category = "Scene")
	void CallCut();

	/** Cheat / debug: resolves the active cue with this grade. */
	UFUNCTION(BlueprintCallable, Category = "Cheat")
	void CheatSkipCue(ELTTGrade Grade = ELTTGrade::Good);

	/** Cheat / debug: restarts from the first beat of an act ("A", "B", "C"). */
	UFUNCTION(BlueprintCallable, Category = "Cheat")
	void CheatJumpToAct(FName Act);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowEndScreen();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	enum class EStage : uint8 { Idle, Lines, Wait, Cue, Wrapped };

	void GatherLevel();
	void LockSetProps();
	void CreateHUD();
	void StartScene();
	void StartSlate();
	void StartRolling();
	void StartBeat(int32 Index);
	void PlayBeatFrom(int32 Index);
	void EnterAct(const FLTTBeat& Beat, bool bInstantCamera);
	void CallCue();
	void ResolveCue(ELTTGrade StationGrade);
	void StartFinale();
	void Wrap();
	void ResetToCheckpoint(int32 CheckpointIndex, bool bPenalty);
	int32 FindCheckpoint(int32 FromIndex) const;
	void PlaceActorsForBeat(const FLTTBeat& Beat);
	void StopEverything();

	void PlayLines(const TArray<FLTTDialogueLine>& Lines, TFunction<void()> Then);
	void ShowNextLine();
	float GetLineSeconds(const FLTTDialogueLine& Line) const;
	void WaitThen(float Seconds, TFunction<void()> Then);

	void TickFrameCheck(float Dt);
	void ApplyCueToWorld(ELTTCue Cue, ELTTGrade Grade);
	void SetWorldParam(FName Name, float Target, float Seconds);
	void TickWorldParams(float Dt);
	void SetPlayerInputBlocked(bool bBlocked);
	ELTTCue FindUpcomingCue(int32 BeatIndex) const;
	bool ShouldSkipBeat(const FLTTBeat& Beat) const;
	static FText GetResultBark(ELTTCue Cue, ELTTGrade Grade);

	UPROPERTY(Transient)
	TObjectPtr<ULTTSceneData> Scene;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALTTEffectStation>> Stations;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALTTFilmActor>> FilmActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALTTActorMark>> Marks;

	UPROPERTY(Transient)
	TObjectPtr<ALTTFilmCamera> FilmCamera;

	UPROPERTY(Transient)
	TObjectPtr<ALTTEffectStation> ActiveStation;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> EndScreenWidget;

	EStage Stage = EStage::Idle;
	TFunction<void()> StageThen;

	// Lines
	TArray<FLTTDialogueLine> LineQueue;
	int32 LineIndex = INDEX_NONE;
	float LineSecondsLeft = 0.f;
	bool bLineShowing = false;

	float WaitSecondsLeft = 0.f;

	// Beats
	int32 BeatIndex = INDEX_NONE;
	FName CurrentAct;
	ELTTCue UpcomingCue = ELTTCue::None;

	// Cue
	FLTTCueSpec ActiveSpec;
	float CueWindow = 0.f;
	float CueElapsed = 0.f;
	bool bSlateActive = false;

	// Take
	bool bRolling = false;
	bool bFrameChecksPaused = false;
	bool bWrapped = false;
	bool bCutShowing = false;
	bool bPlayerInFrame = false;
	bool bPlayerInputBlocked = false;
	float RollSeconds = 0.f;
	float TakeMeter = 60.f;
	int32 Score = 0;
	float FramePenaltyRemainder = 0.f;
	int32 MaxScore = 0;
	int32 CutCount = 0;
	float SecondsInFrame = 0.f;
	float FrameStreak = 0.f;
	float FrameBarkCooldown = 0.f;
	int32 GradeCounts[5] = { 0, 0, 0, 0, 0 };
	/** Score and grade counts when the current act started; a CUT goes back to them. */
	int32 CheckpointScore = 0;
	int32 CheckpointCounts[5] = { 0, 0, 0, 0, 0 };
	bool bFinale = false;
	uint8 LastConeLook = 255;

	// HUD bits
	FText BarkText;
	float BarkSecondsLeft = 0.f;
	FText ActCardText;
	float ActCardSecondsLeft = 0.f;
	ELTTGrade ToastGrade = ELTTGrade::None;
	int32 ToastPoints = 0;
	float ToastSecondsLeft = 0.f;

	struct FWorldParamTween
	{
		FName Name;
		float From = 0.f;
		float To = 0.f;
		float Seconds = 0.f;
		float Elapsed = 0.f;
	};
	TArray<FWorldParamTween> ParamTweens;
	TMap<FName, float> ParamValues;
	float FlashSecondsLeft = 0.f;
};
