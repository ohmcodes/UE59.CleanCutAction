// Clean! Cut! Action! - Phase 2 effect station (Docs/04 §3). One class covers every station: pick the cues it
// answers and the mini-game it plays. The player can only use it while ALTTSceneDirector has one of its cues
// active; it then takes over the mouse until the mini-game ends (E or right mouse button leaves early).
// Input is read straight from the keys, so no Input Action assets are needed:
//   left mouse = action, W = push (Sustain), A/D or mouse left/right = shake, 1-3 / mouse wheel = pick a gel.
//
// Suggested setup per cue:
//   Slate -> Press | Door -> Timing | Steam -> HoldRelease | Traffic, Wind, Boom, Rain -> Sustain
//   Thunder -> Shake | Sunset + Moonlight (same station) -> Choice | Napkins, Umbrella, Roses -> Toss

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractable.h"
#include "Shoot/LTTSceneData.h"
#include "LTTEffectStation.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;
class USoundBase;
class ALight;
class ALTTSceneDirector;

UENUM(BlueprintType)
enum class ELTTStationGame : uint8
{
	/** One click. (Slate) */
	Press,
	/** Click when the marker crosses the target. (Door bell) */
	Timing,
	/** Hold to build pressure, release inside the band. (Espresso steam) */
	HoldRelease,
	/** Hold/release to keep the meter inside the band for a while. (Car rail, wind fan, boom mic, rain rig) */
	Sustain,
	/** Shake left/right to fill, then click on the flash. (Thunder sheet) */
	Shake,
	/** Pick one of the options and click. (Light gels) */
	Choice,
	/** Aim at the actor, hold to charge, release to throw. (Prop tables) */
	Toss
};

USTRUCT(BlueprintType)
struct FLTTStationChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	FText Name;

	/** Colour given to KeyLight when this option is slid in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	FLinearColor Color = FLinearColor::White;

	/** The cue this option is right for (None = always wrong, like the green gel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	ELTTCue CorrectFor = ELTTCue::None;
};

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTEffectStation : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTEffectStation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station")
	TObjectPtr<USceneComponent> Root;

	/** What the player aims at to use the station (blocks the interaction trace only). Resize it to cover the prop. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station")
	TObjectPtr<UBoxComponent> InteractBox;

	/** The thrown prop (Toss). Hidden until thrown. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station")
	TObjectPtr<UStaticMeshComponent> TossProp;

	// ---- Setup ----

	/** The cues this station answers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	TArray<ELTTCue> Cues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	ELTTStationGame Game = ELTTStationGame::Press;

	/** Shown in the prompt and on the cue card, e.g. "Rain Rig". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	FText DisplayName;

	/** Shown on the cue card so the player knows where to run: "1F", "2F". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	FText FloorLabel;

	/** Level actors that make up the station (meshes placed in the level). Outlined with the station while its cue is active. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Station")
	TArray<TObjectPtr<AActor>> HighlightActors;

	/** Custom depth stencil used for the outline while the cue is active (1 = the focus outline of M_PP_Outline). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station")
	int32 HighlightStencil = 1;

	// ---- Effect: what happens in the world when the cue lands ----

	/** Level actors (Niagara systems, lights, meshes) hidden at start and switched on when the effect plays. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Effect")
	TArray<TObjectPtr<AActor>> EffectActors;

	/** Seconds the effect actors stay on. 0 = until the take is reset (rain). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0"))
	float EffectDuration = 3.f;

	/** Also switch the effect actors on when the cue was missed (the "sad drizzle"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bShowEffectOnMiss = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<USoundBase> EffectSound;

	/** The weak, funny version for a missed cue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<USoundBase> MissSound;

	// ---- Timing (Door bell) ----

	/** Seconds the marker takes to cross the bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Timing", meta = (ClampMin = "0.5", EditCondition = "Game == ELTTStationGame::Timing"))
	float TimingTravelTime = 2.2f;

	/** Where on the bar (0-1) the target is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Timing", meta = (ClampMin = "0.1", ClampMax = "0.95", EditCondition = "Game == ELTTStationGame::Timing"))
	float TimingTarget = 0.75f;

	// ---- Hold & release (Steam) ----

	/** Seconds of holding to reach full pressure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Hold", meta = (ClampMin = "0.3", EditCondition = "Game == ELTTStationGame::HoldRelease"))
	float HoldFillTime = 2.2f;

	/** Releasing inside this band is Perfect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Hold", meta = (EditCondition = "Game == ELTTStationGame::HoldRelease"))
	FVector2D HoldPerfectBand = FVector2D(0.62f, 0.78f);

	/** This far outside the band is still Good. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Hold", meta = (ClampMin = "0", EditCondition = "Game == ELTTStationGame::HoldRelease"))
	float HoldGoodMargin = 0.11f;

	// ---- Sustain (Traffic, Wind, Boom, Rain) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (EditCondition = "Game == ELTTStationGame::Sustain"))
	FVector2D SustainBand = FVector2D(0.55f, 0.75f);

	/** Seconds the meter has to spend inside the band. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (ClampMin = "0.5", EditCondition = "Game == ELTTStationGame::Sustain"))
	float SustainTime = 3.f;

	/** Meter gained per second while holding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (ClampMin = "0.05", EditCondition = "Game == ELTTStationGame::Sustain"))
	float SustainRise = 0.7f;

	/** Meter lost per second while not holding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (ClampMin = "0.05", EditCondition = "Game == ELTTStationGame::Sustain"))
	float SustainFall = 0.5f;

	/** Random sway added to the meter (the boom pole wobble). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (ClampMin = "0", ClampMax = "0.3", EditCondition = "Game == ELTTStationGame::Sustain"))
	float SustainWobble = 0.f;

	/** Time above this costs quality (over-wind, boom in shot). 1 or more = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Sustain", meta = (EditCondition = "Game == ELTTStationGame::Sustain"))
	float SustainOverLevel = 0.9f;

	// ---- Shake (Thunder) ----

	/** Rumble added by each change of direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Shake", meta = (ClampMin = "0.01", EditCondition = "Game == ELTTStationGame::Shake"))
	float ShakeGain = 0.12f;

	/** Rumble lost per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Shake", meta = (ClampMin = "0", EditCondition = "Game == ELTTStationGame::Shake"))
	float ShakeDecay = 0.25f;

	// ---- Choice (Light gels) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Choice", meta = (EditCondition = "Game == ELTTStationGame::Choice"))
	TArray<FLTTStationChoice> Choices;

	/** Movable light in the level that takes the gel's colour. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Game|Choice", meta = (EditCondition = "Game == ELTTStationGame::Choice"))
	TObjectPtr<ALight> KeyLight;

	// ---- Toss (Prop tables) ----

	/** Film actor the prop is thrown to ("Dash" or "Juniper"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Toss", meta = (EditCondition = "Game == ELTTStationGame::Toss"))
	FName TossTargetName = TEXT("Dash");

	/** Mesh thrown for each cue. Missing = a small box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Toss", meta = (EditCondition = "Game == ELTTStationGame::Toss"))
	TMap<ELTTCue, TObjectPtr<UStaticMesh>> TossMeshes;

	/** Distance a full-power throw covers (cm). The right power is the target's distance over this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Toss", meta = (ClampMin = "200", EditCondition = "Game == ELTTStationGame::Toss"))
	float TossMaxRange = 1400.f;

	/** Seconds for the power bar to go from empty to full (it then comes back down). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Toss", meta = (ClampMin = "0.3", EditCondition = "Game == ELTTStationGame::Toss"))
	float TossChargeTime = 1.1f;

	// ---- State (read by the HUD) ----

	UFUNCTION(BlueprintPure, Category = "Station")
	bool HandlesCue(ELTTCue Cue) const { return Cues.Contains(Cue); }

	UFUNCTION(BlueprintPure, Category = "Station")
	bool IsCueActive() const { return ActiveCue != ELTTCue::None; }

	UFUNCTION(BlueprintPure, Category = "Station")
	bool IsOccupied() const { return User.IsValid(); }

	/** What to do right now, e.g. "Hold the mouse button, release in the green". */
	UFUNCTION(BlueprintPure, Category = "Station")
	FText GetInstruction() const;

	/** The mini-game's meter, 0-1. */
	UFUNCTION(BlueprintPure, Category = "Station")
	float GetMeter() const { return Meter; }

	/** The green band on the meter (X = min, Y = max). X > Y = no band. */
	UFUNCTION(BlueprintPure, Category = "Station")
	FVector2D GetBand() const;

	/** Progress toward finishing, 0-1 (Sustain). Negative = none. */
	UFUNCTION(BlueprintPure, Category = "Station")
	float GetProgress() const;

	// ---- Called by the scene director ----

	void SetCueActive(ELTTCue Cue, bool bActive);

	/** The window ran out. Ends the mini-game and returns what the player earned so far. */
	ELTTGrade HandleCueTimeout();

	/** Shows the effect for a resolved cue. */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	void PlayEffect(ELTTCue Cue, ELTTGrade Grade);

	/** Back to the start-of-take state: effects off, light colour restored. */
	UFUNCTION(BlueprintCallable, Category = "Station")
	void ResetStation();

	/** Kicks the player out of the mini-game. */
	UFUNCTION(BlueprintCallable, Category = "Station")
	void ExitStation();

	// ---- Blueprint hooks ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Station", meta = (DisplayName = "On Cue Active Changed"))
	void ReceiveCueActiveChanged(ELTTCue Cue, bool bActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Station", meta = (DisplayName = "On Station Entered"))
	void ReceiveEntered(APawn* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Station", meta = (DisplayName = "On Station Exited"))
	void ReceiveExited();

	/** The cue was resolved: play VFX/SFX for it (Grade Miss = the weak version). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effect", meta = (DisplayName = "On Effect"))
	void ReceiveEffect(ELTTCue Cue, ELTTGrade Grade);

	UFUNCTION(BlueprintImplementableEvent, Category = "Effect", meta = (DisplayName = "On Effect Ended"))
	void ReceiveEffectEnded();

	// ILTTInteractable
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Station; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override {}
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override {}
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	void EnterStation(APawn* Player);
	void LeaveStation(bool bBailed);
	void Finish(ELTTGrade Grade);
	void SetHighlighted(bool bOn);
	void SetEffectActorsOn(bool bOn);
	void EndEffect();
	void ApplyChoice(int32 Index);
	int32 FindCorrectChoice() const;
	void TickToss(class APlayerController* PC, float Dt, bool bAction);
	void TickFlight(float Dt);
	static ELTTGrade GradeFromQuality(float Quality);

	TWeakObjectPtr<APawn> User;
	TWeakObjectPtr<ALTTSceneDirector> Director;
	ELTTCue ActiveCue = ELTTCue::None;

	bool bLookLocked = false;
	bool bArmed = false;
	bool bPrevAction = false;
	bool bPrevExit = false;
	float TimeInStation = 0.f;

	// Mini-game state
	float Meter = 0.f;
	float RawMeter = 0.f;
	bool bHolding = false;
	float HoldElapsed = 0.f;
	float InBandTime = 0.f;
	float ActiveTime = 0.f;
	float OverTime = 0.f;
	int32 LastShakeSign = 0;
	bool bFlashPhase = false;
	float FlashElapsed = 0.f;
	int32 SelectedChoice = 0;
	bool bHadWrongChoice = false;
	float ChargePhase = 0.f;
	float TossIdeal = 0.5f;
	bool bOnTarget = false;

	// Toss flight
	bool bInFlight = false;
	float FlightElapsed = 0.f;
	float FlightTime = 0.7f;
	FVector FlightFrom = FVector::ZeroVector;
	FVector FlightTo = FVector::ZeroVector;
	float FlightArc = 80.f;
	ELTTGrade FlightGrade = ELTTGrade::None;

	bool bKeyLightColorSaved = false;
	FLinearColor SavedKeyLightColor = FLinearColor::White;
	FTimerHandle EffectTimer;
	FTimerHandle TossHideTimer;
};
