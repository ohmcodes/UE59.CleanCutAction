// Clean! Cut! Action! - Background music: plays a list of tracks in a shuffled loop with crossfades.
// Place one in the level and set Tracks (or MusicFolders) on the instance. Tracks set on the placed actor are
// cooked because the level references them. MusicFolders are found at runtime, so those folders must also be
// cooked (e.g. DirectoriesToAlwaysCook in Config/DefaultGame.ini).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LTTMusicPlayer.generated.h"

class UAudioComponent;
class USoundBase;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnTrackStarted, USoundBase*, Track);

UCLASS(Blueprintable)
class CLEANCUTACTION_API ALTTMusicPlayer : public AActor
{
	GENERATED_BODY()

public:
	ALTTMusicPlayer();

	/** Tracks to play (plus any found in MusicFolders). Each track plays once, then the next one crossfades in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TArray<TSoftObjectPtr<USoundBase>> Tracks;

	/** Optional: every Sound Wave in these folders (and subfolders) is added to Tracks at BeginPlay.
	 *  Folders used in the packaged game must be in Project Settings > Packaging > Additional Asset Directories to Cook
	 *  (that cooks the whole folder, so mind the web build size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (LongPackageName))
	TArray<FDirectoryPath> MusicFolders;

	/** Random order (a shuffled pass through every track, never the same track twice in a row). Off = list order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bShuffle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bPlayOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0", ClampMax = "2"))
	float Volume = 0.35f;

	/** Seconds the old and new tracks overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0"))
	float CrossfadeTime = 4.f;

	/** Fade-in for the very first track. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0"))
	float FirstFadeInTime = 2.f;

	/** Starts music (no-op if already playing). */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void Play();

	/** Fades out and stops. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void Stop(float FadeOutTime = 1.5f);

	/** Crossfades to the next track now. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void Skip();

	UFUNCTION(BlueprintCallable, Category = "Music")
	void SetMusicVolume(float NewVolume);

	/** Mutes or unmutes all music (pause menu): muting fades the music out, unmuting starts the next track. Lasts for the session, so it survives Play Again / level loads. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	static void SetMusicMuted(bool bMuted);

	UFUNCTION(BlueprintPure, Category = "Music")
	static bool IsMusicMuted() { return bMusicMuted; }

	UFUNCTION(BlueprintPure, Category = "Music")
	bool IsPlaying() const { return bPlaying; }

	UFUNCTION(BlueprintPure, Category = "Music")
	USoundBase* GetCurrentTrack() const { return CurrentTrack; }

	UPROPERTY(BlueprintAssignable, Category = "Music")
	FLTTOnTrackStarted OnTrackStarted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Adds every Sound Wave in MusicFolders to Tracks. */
	void GatherFolderTracks();
	int32 PickNextIndex();
	void RefillBag();
	void PlayNext();
	void StartTrack(USoundBase* Sound);
	void ApplyMute();

	static bool bMusicMuted;
	/** Music was playing (or wanted to start) when muted; Play() again on unmute. */
	bool bResumeOnUnmute = false;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> AudioA;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> AudioB;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentTrack;

	/** Track indices left in this shuffled pass. */
	TArray<int32> Bag;
	int32 LastIndex = INDEX_NONE;
	int32 ListIndex = INDEX_NONE;
	bool bUseA = true;
	bool bPlaying = false;
	bool bFirstTrack = true;
	FTimerHandle NextTrackTimer;
	TSharedPtr<FStreamableHandle> LoadHandle;
};
