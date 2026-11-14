// Clean! Cut! Action! - Phase 2 scene script data (Docs/04 §2.1, Docs/05 §2).
// A scene is a list of beats: lines before the cue, the cue Marlowe calls, and the lines after it for each grade.
// ALTTSceneDirector uses the built-in Scene 42 when no ULTTSceneData asset is assigned.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LTTSceneData.generated.h"

class USoundBase;

/** The effects Marlowe can call for. Each ALTTEffectStation lists the cues it handles. */
UENUM(BlueprintType)
enum class ELTTCue : uint8
{
	None,
	Slate,
	Traffic,
	Door,
	Steam,
	Napkins,
	Thunder,
	Sunset,
	Wind,
	Boom,
	Rain,
	Umbrella,
	Roses,
	Moonlight
};

UENUM(BlueprintType)
enum class ELTTGrade : uint8
{
	None,
	Perfect,
	Good,
	Late,
	Miss
};

USTRUCT(BlueprintType)
struct FLTTDialogueLine
{
	GENERATED_BODY()

	/** Marlowe, Dash, Juniper, Leo (Dash in character) or Clara (Juniper in character). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	FName Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line", meta = (MultiLine = true))
	FText Text;

	/** Optional voice-over. The line then lasts as long as the sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	TSoftObjectPtr<USoundBase> Voice;

	/** Key into the speaking actor's Montages map (e.g. "Laugh"). None = a random talk montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	FName Montage;

	/** ALTTActorMark name to walk to when the line starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	FName MoveToMark;

	/** Who walks to MoveToMark. None = the speaker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	FName MoveActor;

	/** Extra seconds to hold the line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	float ExtraDelay = 0.f;

	/** The actors dropped the script: subtitle shows the actor's own name with "(off-script)". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	bool bOffScript = false;
};

USTRUCT(BlueprintType)
struct FLTTCueSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue")
	ELTTCue Cue = ELTTCue::None;

	/** Seconds after the call before the cue is missed. Travel time to the station is added on top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue", meta = (ClampMin = "1"))
	float BaseWindow = 8.f;

	/** Also grade by how fast it was done (tosses, gels): the worse of the station grade and the time grade counts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue")
	bool bGradeBySpeed = false;

	/** Done within this fraction of the window = Perfect (speed-graded cues). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue", meta = (EditCondition = "bGradeBySpeed"))
	float PerfectFrac = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue", meta = (EditCondition = "bGradeBySpeed"))
	float GoodFrac = 0.8f;

	/** No timer, no score (the slate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue")
	bool bTutorial = false;

	/** What Marlowe shouts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cue", meta = (MultiLine = true))
	FText CallLine;
};

USTRUCT(BlueprintType)
struct FLTTBeat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	FName BeatID;

	/** "A", "B" or "C": which film camera anchor and act title this beat belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	FName Act;

	/** First beat of an act: the camera moves here, and a CUT restarts from this beat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	bool bActCheckpoint = false;

	/** Where the actors stand when the act (re)starts. Checkpoint beats only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat", meta = (EditCondition = "bActCheckpoint"))
	FName DashStartMark;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat", meta = (EditCondition = "bActCheckpoint"))
	FName JuniperStartMark;

	/** Effects from earlier acts that are still on when this act restarts after a CUT (rain in Act C). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat", meta = (EditCondition = "bActCheckpoint"))
	TArray<ELTTCue> RestoreEffects;

	/** Skipped when the director's bSkipOptionalBeats is on, or when no station handles its cue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	bool bOptional = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PreDialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	bool bHasCue = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat", meta = (EditCondition = "bHasCue"))
	FLTTCueSpec CueSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PostPerfect;

	/** Empty = PostPerfect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PostGood;

	/** Empty = PostGood. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PostLate;

	/** Empty = PostLate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PostMiss;

	/** Played after the post lines whatever the grade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beat")
	TArray<FLTTDialogueLine> PostAlways;

	const TArray<FLTTDialogueLine>& GetPost(ELTTGrade Grade) const;
};

UCLASS(BlueprintType)
class CLEANCUTACTION_API ULTTSceneData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Marlowe's speech before the slate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TArray<FLTTDialogueLine> PreRoll;

	/** Said once the slate is clapped, right before the first beat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TArray<FLTTDialogueLine> SlateDone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TArray<FLTTBeat> Beats;

	/** Finale and epilogue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TArray<FLTTDialogueLine> Wrap;

	/** "A" -> "ACT I - THE ARRIVAL". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TMap<FName, FText> ActTitles;

	/** Marlowe's line when an act restarts after a CUT. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	TMap<FName, FText> RestartLines;

	/** Replaces everything in this asset with Scene 42 from Docs/05 §2 (button in the Details panel). */
	UFUNCTION(CallInEditor, Category = "Scene")
	void FillWithScene42();

	/** A transient asset holding Scene 42. */
	static ULTTSceneData* MakeScene42(UObject* Outer);

	static FText GetCueName(ELTTCue Cue);
	static FText GetGradeName(ELTTGrade Grade);
};
