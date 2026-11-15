// Clean! Cut! Action! - A spill on the floor. Walking through it slows the player down (with splashes); it is
// cleaned by holding the mop (a prop with bIsMop, BP_Prop_Mop), aiming at the water and clicking: each stroke
// shrinks it, the last one removes it. Without the mop, looking at it shows "Needs a mop".
// Counts as a cleanup task for the mess director, so the set isn't clean while a puddle is left.
// Place it from Place Actors ("LTT Water Puddle") or use /Game/_CleanCutAction/Blueprints/Interactables/BP_WaterPuddle.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LTTInteractable.h"
#include "LTTWaterPuddle.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class ACharacter;

UCLASS(Blueprintable, meta = (DisplayName = "LTT Water Puddle"))
class CLEANCUTACTION_API ALTTWaterPuddle : public AActor, public ILTTInteractable
{
	GENERATED_BODY()

public:
	ALTTWaterPuddle();

	/** The water: a flat plane with M_WaterPuddle (round, wobbly edge, ripples). No collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puddle")
	TObjectPtr<UStaticMeshComponent> Water;

	/** Droplet burst fired where the mop hits, and at the player's feet when they step in. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puddle")
	TObjectPtr<UNiagaraComponent> Splash;

	/** Size of the puddle (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle", meta = (ClampMin = "10"))
	float Radius = 80.f;

	/** Walk speed while standing in the water, as a fraction of normal (0.45 = less than half speed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle", meta = (ClampMin = "0.05", ClampMax = "1"))
	float SlowFactor = 0.45f;

	/** Mop strokes (clicks) needed to clean it up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle", meta = (ClampMin = "1"))
	int32 MopsNeeded = 3;

	/** Seconds one stroke takes (the puddle shrinks over this time; the next stroke has to wait for it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle", meta = (ClampMin = "0.05"))
	float StrokeTime = 0.4f;

	/** The set isn't clean until this puddle is mopped up (it's counted with the props in the HUD and the score). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle")
	bool bCountsAsCleanupTask = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound")
	TObjectPtr<USoundBase> MopSound;

	/** Stepping into / walking through the water. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound")
	TObjectPtr<USoundBase> SplashSound;

	/** The last stroke: all clean. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound")
	TObjectPtr<USoundBase> CleanSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound", meta = (ClampMin = "0"))
	float Volume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound", meta = (ClampMin = "100"))
	float SoundFalloff = 1200.f;

	/** Seconds between splash steps while the player walks through the water. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Sound", meta = (ClampMin = "0.1"))
	float StepInterval = 0.42f;

	/** One mop stroke at the world position At. Returns false while the previous stroke is still playing or the
	 *  puddle is already clean. */
	UFUNCTION(BlueprintCallable, Category = "Puddle")
	bool MopUp(AActor* Mop, FVector At);

	/** Removes the puddle right away and counts it as cleaned (Place All cheat). */
	UFUNCTION(BlueprintCallable, Category = "Puddle")
	void CleanInstantly();

	UFUNCTION(BlueprintPure, Category = "Puddle")
	bool IsCleaned() const { return bCleaned; }

	UFUNCTION(BlueprintPure, Category = "Puddle")
	int32 GetStrokesLeft() const { return FMath::Max(MopsNeeded - StrokesDone, 0); }

	/** Radius right now (it shrinks with every stroke), in the actor's local space. */
	UFUNCTION(BlueprintPure, Category = "Puddle")
	float GetCurrentRadius() const { return CurrentRadius; }

	/** The nearest puddle the view ray lands on within MaxDistance, with nothing in the way. OutPoint is where the
	 *  ray meets the water. Used by ULTTInteractionComponent (the puddle has no collision to trace against). */
	static ALTTWaterPuddle* FindAimed(const UObject* WorldContext, const FVector& ViewLocation, const FVector& ViewDirection,
		float MaxDistance, FVector& OutPoint, const AActor* IgnoreActor = nullptr);

	UFUNCTION(BlueprintImplementableEvent, Category = "Puddle", meta = (DisplayName = "On Mopped"))
	void ReceiveMopped(int32 StrokesLeft);

	UFUNCTION(BlueprintImplementableEvent, Category = "Puddle", meta = (DisplayName = "On Cleaned"))
	void ReceiveCleaned();

	// ILTTInteractable (focused without the mop in hand)
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override { return !bCleaned; }
	virtual FText GetPromptText_Implementation(AActor* InstigatorActor) const override;
	virtual ELTTInteractKind GetInteractKind_Implementation() const override { return ELTTInteractKind::Press; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual void OnInteract_Implementation(AActor* InstigatorActor) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ApplyRadius(float NewRadius);
	void UpdateSlow(float DeltaSeconds);
	void FinishCleaning();
	void MarkCleaned();
	void PlayPuddleSound(USoundBase* Sound, const FVector& At, float VolumeScale = 1.f, float Pitch = 1.f);
	void FireSplash(const FVector& At);
	/** True if the character's feet are in the water. */
	bool IsInWater(const ACharacter* Character) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;

	/** Characters this puddle is slowing down right now. */
	TArray<TWeakObjectPtr<ACharacter>> Slowed;

	float CurrentRadius = 80.f;
	float RadiusFrom = 80.f;
	float RadiusTo = 80.f;
	/** < 0: no stroke playing. */
	float StrokeElapsed = -1.f;
	float StepTimer = 0.f;
	float PokeReadyTime = 0.f;
	int32 StrokesDone = 0;
	bool bCleaned = false;
	bool bRegisteredTask = false;
};
