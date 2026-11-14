// Clean! Cut! Action! - The two film actors of the Phase 2 shoot (Dash and Juniper) and the marks they stand on
// (Docs/04 §1.3, §6). Make a Blueprint child per actor for the mesh, Anim Blueprint and montages, place both in
// the level, and place an ALTTActorMark for every mark name the scene script uses.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Shoot/LTTSceneData.h"
#include "LTTFilmActor.generated.h"

class UAnimMontage;
class UArrowComponent;
class UBillboardComponent;

/** A spot an actor walks to. The arrow is the way they face once there. */
UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTActorMark : public AActor
{
	GENERATED_BODY()

public:
	ALTTActorMark();

	/** The name the scene script uses: A_LeoTable, A_ClaraDoor, A_ClaraTable, B_ClaraRail, B_LeoRail, C_Center_L, C_Center_R. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mark")
	FName MarkName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mark")
	TObjectPtr<UArrowComponent> Arrow;
};

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTFilmActor : public ACharacter
{
	GENERATED_BODY()

public:
	ALTTFilmActor();

	/** "Dash" or "Juniper": the name script lines use for the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor")
	FName ActorName = TEXT("Dash");

	/** "Leo" or "Clara": the part they play. Lines by this speaker are theirs too. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor")
	FName CharacterName = TEXT("Leo");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor")
	float WalkSpeed = 160.f;

	/** Used for montages named "StormOff". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor")
	float RunSpeed = 300.f;

	/** If the actor hasn't reached a mark after this long (no NavMesh, blocked), they're put on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor", meta = (ClampMin = "1"))
	float MoveTimeout = 9.f;

	/** Montages by the name script lines ask for: Laugh, Angry, StormOff, Catch, Startled, LeanIn... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor|Animation")
	TMap<FName, TObjectPtr<UAnimMontage>> Montages;

	/** One is picked at random for lines that don't name a montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Film Actor|Animation")
	TArray<TObjectPtr<UAnimMontage>> TalkMontages;

	/** Where tossed props are aimed (in front of the chest). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Film Actor")
	TObjectPtr<USceneComponent> CatchPoint;

	UFUNCTION(BlueprintPure, Category = "Film Actor")
	bool IsNamed(FName Name) const { return Name == ActorName || Name == CharacterName; }

	UFUNCTION(BlueprintPure, Category = "Film Actor")
	FVector GetCatchLocation() const;

	/** Walks to the mark (NavMesh path), then turns to face its arrow. */
	UFUNCTION(BlueprintCallable, Category = "Film Actor")
	void MoveToMark(ALTTActorMark* Mark, bool bRun = false);

	UFUNCTION(BlueprintCallable, Category = "Film Actor")
	void TeleportToMark(ALTTActorMark* Mark);

	/** Plays the line's montage (or a random talk montage) and calls the Blueprint event. */
	void Speak(const FLTTDialogueLine& Line);

	UFUNCTION(BlueprintCallable, Category = "Film Actor")
	bool PlayNamedMontage(FName Name);

	/** Freezes or resumes the actor's animation and movement (CUT interstitial). */
	UFUNCTION(BlueprintCallable, Category = "Film Actor")
	void SetFrozen(bool bFrozen);

	/** Blueprint hook: voice blips, look-at, glance at the camera on off-script lines. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Film Actor", meta = (DisplayName = "On Line Spoken"))
	void ReceiveLineSpoken(const FLTTDialogueLine& Line);

	/** Blueprint hook: reaction to a cue result (smile on Perfect, glance at the camera on Miss). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Film Actor", meta = (DisplayName = "On Cue Result"))
	void ReceiveCueResult(ELTTCue Cue, ELTTGrade Grade);

protected:
	virtual void Tick(float DeltaSeconds) override;

	void FinishMove();

	TWeakObjectPtr<ALTTActorMark> TargetMark;
	bool bMovingToMark = false;
	bool bFacingMark = false;
	float MoveElapsed = 0.f;
};
