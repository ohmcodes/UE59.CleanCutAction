// Clean! Cut! Action! - Phase 1 cleanup flow.
// The level is authored clean. At start this actor records every prop's home, finds what each prop
// rests on, then throws everything around (the "earthquake"). The player puts each prop back on its
// home silhouette; when every cleanup prop is placed (or broken), OnAllPlaced fires.
// Also keeps the score and the optional countdown, and adds the cleanup HUD (ULTTCleanupHUD).
// Blueprint: /Game/_CleanCutAction/Blueprints/Core/BP_MessDirector (place one in the level).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "LTTMessDirector.generated.h"

class ALTTInteractableBase;
class UCameraShakeBase;
class UUserWidget;
class UInputAction;
class UInputMappingContext;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class ACameraActor;
class ULTTEndingOverlay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnCleanProgress, int32, Placed, int32, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLTTOnAllPlaced);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLTTOnTimeUp);

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTMessDirector : public AActor
{
	GENERATED_BODY()

public:
	ALTTMessDirector();

	/** First mess director in the world, or null. */
	static ALTTMessDirector* Get(const UObject* WorldContext);

	// ---- Earthquake ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	bool bEarthquakeOnStart = true;

	/** Seconds after BeginPlay before the shake (lets props settle and the player look around). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake", meta = (ClampMin = "0"))
	float EarthquakeDelay = 1.0f;

	/** Horizontal launch speed range for small props (cm/s). Big props get less. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	FVector2D HorizontalSpeed = FVector2D(200.f, 550.f);

	/** Upward launch speed range (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	FVector2D UpSpeed = FVector2D(150.f, 400.f);

	/** Max random spin (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	float MaxSpin = 400.f;

	/** Camera shake played on the player during the earthquake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	TSubclassOf<UCameraShakeBase> EarthquakeShake;

	/** 0 = different mess every play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
	int32 RandomSeed = 0;

	UFUNCTION(BlueprintCallable, Category = "Earthquake")
	void TriggerEarthquake();

	// ---- Progress ----

	/** Cleanup props still in play (placed + not yet placed). Broken props are removed. */
	UFUNCTION(BlueprintPure, Category = "Cleanup")
	int32 GetTotalCount() const { return Tasks.Num(); }

	UFUNCTION(BlueprintPure, Category = "Cleanup")
	int32 GetPlacedCount() const { return PlacedCount; }

	/** Cleanup props destroyed before they were placed (shattered or disappeared). */
	UFUNCTION(BlueprintPure, Category = "Cleanup")
	int32 GetBrokenCount() const { return BrokenCount; }

	UFUNCTION(BlueprintPure, Category = "Cleanup")
	bool IsSetClean() const { return bAllPlaced; }

	UPROPERTY(BlueprintAssignable, Category = "Cleanup")
	FLTTOnCleanProgress OnProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Cleanup")
	FLTTOnAllPlaced OnAllPlaced;

	/** Blueprint hook for the end screen. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cleanup", meta = (DisplayName = "On All Placed"))
	void ReceiveAllPlaced();

	// ---- Score ----

	/** Points for each prop put back home. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsPerPlaced = 10;

	/** Points for each cleanup prop destroyed before it was put back (negative = penalty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsPerBroken = -15;

	/** Bonus points for every second left on the countdown when the set is clean. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (ClampMin = "0"))
	float PointsPerSecondLeft = 1.f;

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const { return GetPropPoints() + GetBrokenPoints() + HintsUsed * PointsPerHint + TimeBonusPoints; }

	/** Points from props put back. */
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetPropPoints() const { return PlacedCount * PointsPerPlaced; }

	/** Points lost (or won) for broken props. */
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetBrokenPoints() const { return BrokenCount * PointsPerBroken; }

	/** Bonus for time left on the clock; set when the set is clean (0 before, or after time ran out). */
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetTimeBonusPoints() const { return TimeBonusPoints; }

	/** Seconds that were left on the clock when the set was clean. */
	UFUNCTION(BlueprintPure, Category = "Score")
	float GetTimeLeftAtFinish() const { return TimeLeftAtFinish; }

	// ---- Countdown ----

	/** Show and run the countdown. Can be switched at runtime with SetTimerEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown")
	bool bUseTimer = true;

	/** Countdown length in seconds. It starts with the earthquake (or right away if there is none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown", meta = (ClampMin = "1", EditCondition = "bUseTimer"))
	float TimeLimitSeconds = 1800.f;

	/** Starts (or restarts) the countdown from Seconds; <= 0 uses TimeLimitSeconds. */
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StartTimer(float Seconds = 0.f);

	/** Stops the countdown without firing OnTimeUp. */
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StopTimer();

	/** Turns the countdown on (starts it if the mess has begun) or off (stops and hides it). */
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void SetTimerEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void AddTime(float Seconds);

	UFUNCTION(BlueprintPure, Category = "Countdown")
	bool IsTimerRunning() const;

	UFUNCTION(BlueprintPure, Category = "Countdown")
	bool IsTimeUp() const { return bTimeUp; }

	/** Seconds left; TimeLimitSeconds before it starts, 0 once time is up. */
	UFUNCTION(BlueprintPure, Category = "Countdown")
	float GetTimeRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Countdown")
	FLTTOnTimeUp OnTimeUp;

	UFUNCTION(BlueprintImplementableEvent, Category = "Countdown", meta = (DisplayName = "On Time Up"))
	void ReceiveTimeUp();

	// ---- Hint ----

	/** Input action for the hint. Empty = the director makes its own, bound to HintKey and HintGamepadKey. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	TObjectPtr<UInputAction> HintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	FKey HintKey = EKeys::F;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	FKey HintGamepadKey = EKeys::Gamepad_FaceButton_Top;

	/** Seconds the hint (outline + arrow) stays up. It also ends when the prop is picked up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint", meta = (ClampMin = "0.5"))
	float HintDuration = 8.f;

	/** Seconds after a hint before the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint", meta = (ClampMin = "0"))
	float HintCooldown = 3.f;

	/** Points for each hint used (negative = cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	int32 PointsPerHint = 0;

	/** Outlines the nearest prop that still needs putting back (one whose home support is ready first) and
	 *  makes the HUD point at it. Returns false if nothing is left, the cooldown is running, or time is up. */
	UFUNCTION(BlueprintCallable, Category = "Hint")
	bool ShowHint();

	/** When a hint lands on the same prop as the previous hint, that prop is probably clipped into something or
	 *  under the map: move it to the middle of its floor (or HintRescueSpot) before outlining it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	bool bMoveRepeatHintToMiddle = true;

	/** Optional: where repeat-hinted props go (a Target Point etc.). Empty = middle of the props' home spots
	 *  on the prop's own floor. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Hint", meta = (EditCondition = "bMoveRepeatHintToMiddle"))
	TObjectPtr<AActor> HintRescueSpot;

	/** Home spots within this height of the prop's home count as the same floor when finding the middle (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint", meta = (ClampMin = "50", EditCondition = "bMoveRepeatHintToMiddle"))
	float HintFloorTolerance = 200.f;

	UFUNCTION(BlueprintCallable, Category = "Hint")
	void ClearHint();

	/** The prop the hint points at, or null. */
	UFUNCTION(BlueprintPure, Category = "Hint")
	ALTTInteractableBase* GetHintTarget() const { return HintTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Hint")
	float GetHintCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Hint")
	int32 GetHintsUsed() const { return HintsUsed; }

	/** Blueprint hook for a hint sound / Marlowe line. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hint", meta = (DisplayName = "On Hint Shown"))
	void ReceiveHintShown(ALTTInteractableBase* Target);

	// ---- HUD ----

	/** Adds the cleanup HUD (counts, points, countdown, home arrow) to the player's screen at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	bool bCreateHUD = true;

	/** ULTTCleanupHUD or a Widget Blueprint based on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (EditCondition = "bCreateHUD"))
	TSubclassOf<UUserWidget> HUDClass;

	UFUNCTION(BlueprintPure, Category = "HUD")
	UUserWidget* GetHUDWidget() const { return HUDWidget; }

	// ---- Briefing: story, controls and rules before the game starts ----

	/** Show the briefing (call sheet + controls + Start button) over the black screen before the game fades in.
	 *  The mess is made behind it; the countdown starts when the player presses Start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing")
	bool bShowBriefing = true;

	/** ULTTBriefingScreen or a Widget Blueprint based on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing", meta = (EditCondition = "bShowBriefing"))
	TSubclassOf<UUserWidget> BriefingClass;

	UFUNCTION(BlueprintPure, Category = "Briefing")
	bool IsBriefingOpen() const { return BriefingWidget != nullptr; }

	/** Closes the briefing and starts the game (fade in + countdown). Called by the briefing's Start button. */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void StartFromBriefing();

	// ---- Intro: hide the mess being made ----

	/** Start on a black screen, throw the props around, and fade in once they've landed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	bool bFadeInOnStart = true;

	/** Seconds to stay black after the earthquake while the props settle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro", meta = (ClampMin = "0", EditCondition = "bFadeInOnStart"))
	float SettleTime = 2.5f;

	/** Fade from black to the game. The countdown starts when this fade starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro", meta = (ClampMin = "0", EditCondition = "bFadeInOnStart"))
	float FadeInTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	FLinearColor FadeColor = FLinearColor::Black;

	// ---- Ending: show off the clean set before the end screen ----

	/** Level Sequence played when the set is clean (camera cuts around the cafe). It holds its last frame
	 *  under the end screen. Takes priority over EndTourCameras. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	TSoftObjectPtr<ULevelSequence> EndSequence;

	/** No sequence? Cameras placed in the level are visited in order instead (blend between them).
	 *  Empty + bAutoFindTourCameras = pick them automatically. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Ending")
	TArray<TObjectPtr<ACameraActor>> EndTourCameras;

	/** Outliner labels of the level cameras to visit, in order (used when EndTourCameras is empty).
	 *  Labels only exist in the editor, so they're resolved to the actual cameras whenever the level is saved
	 *  or cooked (ResolvedLabelCameras); in Play In Editor they're looked up live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	TArray<FString> TourCameraLabels = { TEXT("Interior_15"), TEXT("Interior_3"), TEXT("Interior_14"), TEXT("Interior_17"), TEXT("Interior_6") };

	/** TourCameraLabels resolved at save/cook time, for the packaged game. */
	UPROPERTY(VisibleInstanceOnly, Category = "Ending")
	TArray<TObjectPtr<ACameraActor>> ResolvedLabelCameras;

	/** With no sequence, no EndTourCameras and no labels found: use camera actors already in the level that stand inside the cafe
	 *  (inside the area spanned by the props' home spots), picked at random. Exterior shots are skipped.
	 *  Works in the packaged game (Outliner folders don't exist there). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bAutoFindTourCameras = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "1", EditCondition = "bAutoFindTourCameras"))
	int32 MaxTourCameras = 5;

	/** How far outside the props' area a camera may stand and still count as inside the cafe (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "0", EditCondition = "bAutoFindTourCameras"))
	float TourCameraMargin = 150.f;

	/** Seconds each tour camera is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "0.5"))
	float TourShotTime = 2.5f;

	/** Each tour shot fades in from black and out to black over this long (the last shot stays up under the end screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "0"))
	float TourFadeTime = 0.6f;

	/** Quick fade to black and back when cutting from gameplay to the ending shots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "0"))
	float EndingFadeTime = 0.5f;

	/** Also play the ending shots when time runs out (off: straight to the end screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bPlayEndingOnTimeUp = false;

	/** Plays the ending shots (if any), then the end screen. Normally called automatically. */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void PlayEnding(bool bSetClean);

	/** Show a SKIP button (Space / Enter / gamepad A) while the ending shots play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bAllowSkipEnding = true;

	/** Confetti bursts over the screen when the set is clean, just before the end screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bConfettiOnSetClean = true;

	/** Seconds of confetti before the end screen comes up (it keeps falling over the end screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending", meta = (ClampMin = "0", EditCondition = "bConfettiOnSetClean"))
	float ConfettiLeadTime = 0.9f;

	/** ULTTEndingOverlay or a Widget Blueprint based on it (skip button + confetti). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	TSubclassOf<ULTTEndingOverlay> EndingOverlayClass;

	/** Cuts the ending shots short: jumps to the last shot and goes on to the confetti / end screen. */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void SkipEnding();

	UFUNCTION(BlueprintPure, Category = "Ending")
	bool IsEndingPlaying() const { return bEndingPlaying; }

	// ---- End screen ----

	/** Show the end screen when every prop is back ("SET READY!") or the countdown runs out ("TIME'S UP!"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "End Screen")
	bool bShowEndScreen = true;

	/** ULTTEndScreen or a Widget Blueprint based on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "End Screen", meta = (EditCondition = "bShowEndScreen"))
	TSubclassOf<UUserWidget> EndScreenClass;

	/** Seconds between the last prop locking in (or time running out) and the end screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "End Screen", meta = (ClampMin = "0"))
	float EndScreenDelay = 1.5f;

	/** Pauses the game and opens the end screen. Normally called automatically. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void ShowEndScreen(bool bSetClean);

	/** Closes the end screen and resumes play. After time's up the countdown is switched off so the player can finish. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void CloseEndScreen();

	/** Seconds from the mess starting to the set being clean / time running out (or until now). */
	UFUNCTION(BlueprintPure, Category = "End Screen")
	float GetTimeOnSet() const;

	// ---- Stuck props: under the floor, inside furniture or walls ----

	/** Every RescueInterval seconds, props that fell under the floor, out of the cafe or got stuck inside other
	 *  geometry are moved back to a free spot on the floor near their home. Also runs once while the screen is
	 *  still black after the earthquake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stuck Props")
	bool bRescueStuckProps = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stuck Props", meta = (ClampMin = "0.5", EditCondition = "bRescueStuckProps"))
	float RescueInterval = 2.f;

	/** A prop whose top is this far below the lowest home spot (the ground floor) counts as under the map (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stuck Props", meta = (ClampMin = "0", EditCondition = "bRescueStuckProps"))
	float UnderFloorMargin = 30.f;

	/** How far from home (horizontally) a prop may end up before it counts as lost outside the set (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stuck Props", meta = (ClampMin = "100", EditCondition = "bRescueStuckProps"))
	float MaxDistanceFromSet = 2500.f;

	/** Checks every unplaced prop now and moves the stuck ones. Returns how many were moved. */
	UFUNCTION(BlueprintCallable, Category = "Stuck Props")
	int32 RescueStuckProps();

	// ---- Cheats (console: LTT.PlaceAll [LeaveCount]) ----

	/** Puts every cleanup prop back except LeaveCount of them (props nothing else rests on), so the last few can
	 *  be placed by hand to test the ending. Returns how many were placed. */
	UFUNCTION(BlueprintCallable, Category = "Cheat")
	int32 CheatPlaceAll(int32 LeaveCount = 5);

	/** Called by a prop when it locks into its home. */
	void NotifyPlaced(ALTTInteractableBase* Prop);

	/** True if another cleanup prop still in play rests on Prop at home (cups on a table). */
	bool HasDependents(const ALTTInteractableBase* Prop) const;

	/** Called by a prop that broke but went back to its home (red) because others rest on it: counts it as broken
	 *  and takes it off the to-do list. */
	void NotifyBrokenInPlace(ALTTInteractableBase* Prop);

protected:
	virtual void BeginPlay() override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

	void SetupTasks();

	UFUNCTION()
	void HandlePropDestroyed(AActor* DestroyedActor);

	void CheckComplete();
	void ShowBriefing();
	void HandleTimeUp();
	void CreateHUD();
	void SetupHintInput();
	void MarkMessStarted();
	void BlackOut();
	void FinishIntro();
	void SetPlayerInputBlocked(bool bBlocked);
	void StartEndingShots();
	void NextTourShot();
	bool HasEndingShots() const;
	/** Fills TourCameras from EndTourCameras, or finds cameras inside the cafe. */
	void GatherTourCameras();
	void FadeOutTourShot();
	/** Finds TourCameraLabels among this level's cameras (editor only: labels are editor data). */
	void ResolveLabelCameras(TArray<TObjectPtr<ACameraActor>>& Out) const;

	UFUNCTION()
	void HandleEndingFinished();
	/** After the shots (or with none): confetti if the set is clean, then the end screen. */
	void FinishEnding();
	void ShowEndingOverlay(bool bWithSkip);
	/** True if Prop is under the floor, far outside the set or its middle is inside other geometry. */
	bool IsPropStuck(const ALTTInteractableBase* Prop) const;
	/** A free spot on the floor near Prop's home that it can be dropped on; false if none was found. */
	bool FindRescueSpot(const ALTTInteractableBase* Prop, FVector& OutLocation) const;
	/** Same search as FindRescueSpot, but around Anchor instead of Prop's home. */
	bool FindFreeSpotNear(const ALTTInteractableBase* Prop, const FVector& Anchor, FVector& OutLocation) const;
	/** Middle of the props' home spots on Prop's floor (or HintRescueSpot). */
	FVector GetHintRescueAnchor(const ALTTInteractableBase* Prop) const;
	/** Moves a repeat-hinted prop to the middle of its floor. */
	void MoveHintedPropToMiddle(ALTTInteractableBase* Prop);
	void RescueTick();
	void MoveStuckProp(ALTTInteractableBase* Prop);
	void ScheduleEndScreen(bool bSetClean);
	void HandleHintInput();
	void UpdateHint();
	bool IsHintCandidate(const ALTTInteractableBase* Prop) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALTTInteractableBase>> Tasks;

	int32 PlacedCount = 0;
	int32 TimeBonusPoints = 0;
	float TimeLeftAtFinish = 0.f;
	/** The intro fade / mess start was due while the briefing was up; run it when Start is pressed. */
	bool bFinishIntroPending = false;
	bool bMessStartPending = false;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> BriefingWidget;
	int32 BrokenCount = 0;
	bool bAllPlaced = false;
	bool bTimeUp = false;
	bool bMessStarted = false;
	/** Seconds left while the countdown is stopped; < 0 = TimeLimitSeconds. */
	float StoppedRemaining = -1.f;
	FTimerHandle EarthquakeTimer;
	FTimerHandle CountdownTimer;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	TWeakObjectPtr<ALTTInteractableBase> HintTarget;
	/** The prop the previous hint pointed at (kept after the hint ends; reset when it's moved to the middle). */
	TWeakObjectPtr<ALTTInteractableBase> LastHinted;
	float HintEndTime = 0.f;
	float HintReadyTime = 0.f;
	int32 HintsUsed = 0;
	bool bHintInputBound = false;
	FTimerHandle HintTimer;
	FTimerHandle EndScreenTimer;
	FTimerHandle IntroTimer;
	FTimerHandle EndingTimer;
	bool bIntroDone = false;
	bool bPlayerInputBlocked = false;
	bool bEndingSetClean = true;
	int32 TourIndex = INDEX_NONE;

	/** The cameras the tour visits this game. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACameraActor>> TourCameras;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> EndingPlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> EndingActor;
	float MessStartTime = -1.f;
	float MessEndTime = -1.f;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> EndScreenWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULTTEndingOverlay> EndingOverlay;
	bool bEndingPlaying = false;
	bool bEndingFinished = false;

	FTimerHandle RescueTimer;
	/** Lowest home spot of any prop (the ground floor), for the under-the-map test. */
	float LowestHomeZ = 0.f;
	/** Props seen stuck on the last check: moved only if they're still stuck on the next one (not just passing through). */
	TSet<TWeakObjectPtr<ALTTInteractableBase>> SuspectProps;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeHintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeHintContext;
};
