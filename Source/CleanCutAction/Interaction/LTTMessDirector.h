// Clean! Cut! Action! - Phase 1 cleanup flow.
// The level is authored clean. At start this actor records every prop's home, finds what each prop
// rests on, then throws everything around (the "earthquake"). The player puts each prop back on its
// home silhouette; when every cleanup prop is placed (or broken), OnAllPlaced fires.
// Also keeps the score and the optional countdown, and adds the cleanup HUD (ULTTCleanupHUD).
// Blueprint: /Game/_CleanCutAction/Blueprints/Core/BP_MessDirector (place one in the level).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTMessDirector.generated.h"

class ALTTInteractableBase;
class UCameraShakeBase;
class UUserWidget;

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

	/** Points for each cleanup prop destroyed before it was put back (use a negative number for a penalty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 PointsPerBroken = 0;

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const { return PlacedCount * PointsPerPlaced + BrokenCount * PointsPerBroken; }

	// ---- Countdown ----

	/** Show and run the countdown. Can be switched at runtime with SetTimerEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown")
	bool bUseTimer = true;

	/** Countdown length in seconds. It starts with the earthquake (or right away if there is none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown", meta = (ClampMin = "1", EditCondition = "bUseTimer"))
	float TimeLimitSeconds = 900.f;

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

	// ---- HUD ----

	/** Adds the cleanup HUD (counts, points, countdown, home arrow) to the player's screen at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	bool bCreateHUD = true;

	/** ULTTCleanupHUD or a Widget Blueprint based on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (EditCondition = "bCreateHUD"))
	TSubclassOf<UUserWidget> HUDClass;

	UFUNCTION(BlueprintPure, Category = "HUD")
	UUserWidget* GetHUDWidget() const { return HUDWidget; }

	/** Called by a prop when it locks into its home. */
	void NotifyPlaced(ALTTInteractableBase* Prop);

protected:
	virtual void BeginPlay() override;

	void SetupTasks();

	UFUNCTION()
	void HandlePropDestroyed(AActor* DestroyedActor);

	void CheckComplete();
	void HandleTimeUp();
	void CreateHUD();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALTTInteractableBase>> Tasks;

	int32 PlacedCount = 0;
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
};
