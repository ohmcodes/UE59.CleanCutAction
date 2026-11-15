// Clean! Cut! Action! - Marlowe heckling the player through the megaphone while they clean (Docs/05 §1,
// Docs/03 §7). Place one in the Phase 1 level next to the mess director. It listens to the mess director
// (progress, broken props, hints, countdown, set clean) and the player's carry component (pick up, drop,
// throw) and picks a line for each event, with chance rolls, cooldowns and no repeats until a pool runs dry.
// Lines show in ULTTBarkWidget (bottom centre, the Phase 2 bark's gold pill).
//
// Every new line cuts in on the one on screen (after MinShowBeforeInterrupt). A cut-off tutorial / story /
// milestone line (priority 2-3) is said again afterwards and the rest of its chain carries on; a cut-off reaction
// (priority 0-1) is dropped. Lines raised in the same moment play one after another. Only one milestone waits in
// the queue (the newest), and reactions are at least ReactionGap seconds apart. Once the player has placed
// TutorialSkipAfterPlaced props, the next interrupted tutorial line ends the tutorial for the session.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractableBase.h"
#include "LTTCleanupBarker.generated.h"

class ALTTMessDirector;
class ALTTInteractableBase;
class ULTTCarryComponent;
class ULTTBarkWidget;
class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class ELTTBarkSpeaker : uint8
{
	Marlowe,
	WalkieDash,
	WalkieJuniper
};

/** Gameplay moments props and the interaction component report (they have no delegate of their own). */
UENUM(BlueprintType)
enum class ELTTBarkEvent : uint8
{
	Crash,         // a prop the player let go of slammed into something
	TrashMiss,     // thrown trash hit something that isn't a bin
	BinWrongItem,  // a set piece landed in a bin (and was spat out)
	WrongOrder     // tried to place a prop before the one it rests on
};

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTCleanupBarker : public AActor
{
	GENERATED_BODY()

public:
	ALTTCleanupBarker();

	/** First barker in the world, or null. */
	static ALTTCleanupBarker* Get(const UObject* WorldContext);

	/** Reports a moment to the level's barker, if there is one. */
	static void NotifyEvent(const UObject* WorldContext, ELTTBarkEvent Event, ALTTInteractableBase* Prop);

	void HandleEvent(ELTTBarkEvent Event, ALTTInteractableBase* Prop);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks")
	bool bEnabled = true;

	/** Multiplies every reaction's chance (2 = twice as chatty, 0 = only story, tutorials and milestones). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks", meta = (ClampMin = "0"))
	float ChattinessScale = 1.f;

	/** Minimum seconds between two reactions (priority 0-1), so quick events don't keep cutting each other off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks", meta = (ClampMin = "0"))
	float ReactionGap = 4.f;

	/** A new line cuts in on the one on screen, but not before it has been up this long (so it can be read). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks", meta = (ClampMin = "0"))
	float MinShowBeforeInterrupt = 1.f;

	/** Once the player has placed this many props, the next time a tutorial line (the intro or a "first time you..."
	 *  line) gets interrupted the tutorial stops for the session: they know the game. 0 = never stop it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks", meta = (ClampMin = "0"))
	int32 TutorialSkipAfterPlaced = 10;

	/** Seconds after the screen fades in before the intro starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks", meta = (ClampMin = "0"))
	float IntroDelay = 1.2f;

	// ---- Line timing ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Timing", meta = (ClampMin = "0.05"))
	float SecondsPerWord = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Timing", meta = (ClampMin = "0.5"))
	float MinLineSeconds = 2.4f;

	/** Added to every line so the end can be read. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Timing", meta = (ClampMin = "0"))
	float LinePadding = 0.9f;

	/** Pause between queued lines (chains). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Timing", meta = (ClampMin = "0"))
	float ChainGap = 0.35f;

	// ---- Triggers ----

	/** Seconds without progress or carrying anything before Marlowe nags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Triggers", meta = (ClampMin = "5"))
	float IdleSeconds = 40.f;

	/** Seconds between nags while still idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Triggers", meta = (ClampMin = "5"))
	float IdleRepeatSeconds = 60.f;

	/** A placement within this many seconds of the last one adds to the streak (Docs/03 §3.14). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Triggers", meta = (ClampMin = "1"))
	float StreakWindow = 6.f;

	/** Seconds after a plain drop before checking whether the prop landed somewhere it doesn't belong. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Triggers", meta = (ClampMin = "0.2"))
	float WrongDropCheckDelay = 1.2f;

	// ---- Look and sound ----

	/** ULTTBarkWidget or a Widget Blueprint based on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Look")
	TSubclassOf<ULTTBarkWidget> WidgetClass;

	/** Megaphone icon (a hard reference so it is cooked with the level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Look")
	TObjectPtr<UTexture2D> BarkIcon;

	/** Optional sound when Marlowe starts a line (a megaphone click / blip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Look")
	TObjectPtr<USoundBase> LineSound;

	/** Optional sound when a walkie line starts (squelch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barks|Look")
	TObjectPtr<USoundBase> WalkieSound;

	// ---- API ----

	/** Shows a line now (priority 3: interrupts anything lower, queues behind other story lines). */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void Bark(const FText& Text, ELTTBarkSpeaker Speaker = ELTTBarkSpeaker::Marlowe, int32 Priority = 3);

	/** The line on screen. False if nobody is talking. */
	bool GetCurrentLine(FText& OutText, FText& OutSpeaker, bool& bOutWalkie) const;

	/** Goes up by one for each new line (the widget pops in when it changes). */
	int32 GetLineSerial() const { return LineSerial; }

	UFUNCTION(BlueprintPure, Category = "Barks")
	bool IsTalking() const { return bTalking; }

	/** Turns barking on or off. Turning it off also drops every queued line and the one on screen.
	 *  Console: LTT.DisableBarks [0|1] (1 = disable, the default; 0 = enable again). */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void SetBarksEnabled(bool bInEnabled);

	/** Drops every queued line and hides the one on screen. */
	UFUNCTION(BlueprintCallable, Category = "Barks")
	void ClearQueuedBarks();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	struct FLine
	{
		FText Text;
		ELTTBarkSpeaker Speaker = ELTTBarkSpeaker::Marlowe;
		int32 Priority = 1;
		/** Part of the intro or a "first time you..." line. */
		bool bTutorial = false;
	};

	struct FPool
	{
		TArray<FLine> Lines;
		/** Shuffle bag of line indices still to play. */
		TArray<int32> Bag;
		float Chance = 1.f;
		float Cooldown = 0.f;
		int32 Priority = 1;
		bool bOneShot = false;
		bool bChain = false;
		bool bTutorial = false;
		float ReadyTime = 0.f;
		bool bUsed = false;
	};

	void BuildPools();
	FPool& AddPool(FName Id, int32 Priority, float Chance, float Cooldown, std::initializer_list<FText> Lines,
		ELTTBarkSpeaker Speaker = ELTTBarkSpeaker::Marlowe);

	/** Rolls the pool's chance and cooldown and says one of its lines (all of them, in order, for a chain).
	 *  Args fill {0}, {1} in the line. Returns true if something was said or queued. */
	bool Trigger(FName Id, const FFormatOrderedArguments& Args = FFormatOrderedArguments(), bool bIgnoreChance = false);

	void Say(TArray<FLine> Lines);
	void StartLine(const FLine& Line);
	float GetLineSeconds(const FLine& Line) const;
	float Now() const;

	void TryBind();
	void CreateWidgetIfNeeded();

	UFUNCTION()
	void HandleProgress(int32 Placed, int32 Total);

	UFUNCTION()
	void HandleAllPlaced();

	UFUNCTION()
	void HandleTimeUp();

	UFUNCTION()
	void HandleHint(ALTTInteractableBase* Target);

	UFUNCTION()
	void HandlePickedUp(ALTTInteractableBase* Item);

	UFUNCTION()
	void HandleReleased(ALTTInteractableBase* Item, bool bThrown);

	UFUNCTION()
	void HandleFocusChanged(AActor* NewFocus, FText PromptText);

	UFUNCTION()
	void HandlePowerChanged(bool bPowerOn);

	UFUNCTION()
	void HandleLightToggled(bool bIsOn);

	/** Kind-specific lines when a prop is cleaned up (light on, socket in, camera back, shelf / rack done...). */
	bool OnKindCleaned(ALTTInteractableBase* Item);
	/** True if every cleanup prop of this kind still in play is back. */
	bool IsKindDone(ELTTBarkPropKind Kind) const;
	/** True if no trash is left lying around. */
	bool IsTrashDone() const;

	void CheckWrongDrop(TWeakObjectPtr<ALTTInteractableBase> Item);
	void OnCleanedOne();
	void CheckMilestones(int32 Placed, int32 Total);
	void TickTriggers();

	TMap<FName, FPool> Pools;

	FLine Current;
	bool bTalking = false;
	float LineSecondsLeft = 0.f;
	float CurrentDuration = 0.f;
	float CurrentStartTime = 0.f;
	/** The line on screen was cut by another and is already queued to be said again. */
	bool bCurrentRequeued = false;
	/** Where lines said in the same moment as the current one go in the queue (keeps their order). */
	int32 BatchInsertIndex = 0;
	float LastReactionTime = -100.f;
	float QuietSince = -100.f;
	float ChainWait = 0.f;
	int32 LineSerial = 0;
	TArray<FLine> Queue;

	TWeakObjectPtr<ALTTMessDirector> Director;
	TWeakObjectPtr<ULTTCarryComponent> Carry;
	TWeakObjectPtr<ALTTInteractableBase> LastReleased;
	/** The bin or a wrong-order line already reacted to this drop; skip the "wrong spot" check for it. */
	TWeakObjectPtr<ALTTInteractableBase> NoWrongDropFor;

	UPROPERTY(Transient)
	TObjectPtr<ULTTBarkWidget> Widget;

	bool bDirectorBound = false;
	bool bFocusBound = false;
	bool bPowerBound = false;
	bool bLightsBound = false;
	/** Mains power was switched off at some point (so switching it on is a "power restored"). */
	bool bPowerWasOff = false;
	TSet<ELTTBarkPropKind> KindsDone;
	bool bTrashDone = false;
	bool bStarted = false;
	bool bIntroDone = false;
	bool bFinished = false;
	float StartTime = 0.f;
	float LastActivityTime = 0.f;
	float NextIdleTime = 0.f;
	float LastPlaceTime = -100.f;
	int32 Streak = 0;
	int32 LastPlaced = 0;
	/** Placed count when the mess started (props already home don't count as the player's). */
	int32 StartPlaced = 0;
	int32 PlacedByPlayer = 0;
	/** The player showed they know the game: no more intro / "first time" lines this session. */
	bool bTutorialsOff = false;
	int32 LastBroken = 0;
	TSet<int32> MilestonesHit;
	TSet<int32> StoriesHit;
	bool bWarned5Min = false;
	bool bWarned1Min = false;
};
