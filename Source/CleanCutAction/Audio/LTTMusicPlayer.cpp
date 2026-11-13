// Clean! Cut! Action!

#include "Audio/LTTMusicPlayer.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"

bool ALTTMusicPlayer::bMusicMuted = false;

namespace
{
	/** Used when a track has no fixed length (looping cue, MetaSound). */
	constexpr float FallbackTrackLength = 180.f;
	/** Sounds with no fixed length report INDEFINITELY_LOOPING_DURATION (10000 s). */
	constexpr float NoFixedLength = 9000.f;
}

ALTTMusicPlayer::ALTTMusicPlayer()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	auto MakeAudio = [this](FName Name)
	{
		UAudioComponent* Audio = CreateDefaultSubobject<UAudioComponent>(Name);
		Audio->SetupAttachment(GetRootComponent());
		Audio->bAutoActivate = false;
		Audio->bAllowSpatialization = false;
		Audio->bIsMusic = true;
		Audio->bIsUISound = true; // keeps playing while the game is paused (pause menu, end screen)
		return Audio;
	};
	AudioA = MakeAudio(TEXT("MusicA"));
	AudioB = MakeAudio(TEXT("MusicB"));
	// No default tracks: set them on the placed actor (L_NYC_Cafe uses /Game/_CleanCutAction/MusicFX).
}

void ALTTMusicPlayer::GatherFolderTracks()
{
	if (MusicFolders.Num() == 0)
	{
		return;
	}
	FARFilter Filter;
	for (const FDirectoryPath& Folder : MusicFolders)
	{
		if (!Folder.Path.IsEmpty())
		{
			Filter.PackagePaths.Add(FName(*Folder.Path));
		}
	}
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	IAssetRegistry::GetChecked().GetAssets(Filter, Assets);
	const int32 Before = Tracks.Num();
	for (const FAssetData& Asset : Assets)
	{
		Tracks.AddUnique(TSoftObjectPtr<USoundBase>(Asset.GetSoftObjectPath()));
	}
	UE_LOG(LogTemp, Log, TEXT("MusicPlayer: %d tracks from %d folders (%d total)."), Tracks.Num() - Before, MusicFolders.Num(), Tracks.Num());
}

void ALTTMusicPlayer::BeginPlay()
{
	Super::BeginPlay();
	GatherFolderTracks();
	if (bPlayOnBeginPlay)
	{
		Play();
	}
}

void ALTTMusicPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NextTrackTimer);
	if (LoadHandle.IsValid())
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ALTTMusicPlayer::Play()
{
	if (bPlaying || Tracks.Num() == 0)
	{
		return;
	}
	if (bMusicMuted)
	{
		bResumeOnUnmute = true; // starts when the player unmutes
		return;
	}
	bPlaying = true;
	bFirstTrack = true;
	PlayNext();
}

void ALTTMusicPlayer::Stop(float FadeOutTime)
{
	bPlaying = false;
	GetWorldTimerManager().ClearTimer(NextTrackTimer);
	for (UAudioComponent* Audio : { AudioA.Get(), AudioB.Get() })
	{
		if (Audio && Audio->IsPlaying())
		{
			if (FadeOutTime > 0.f)
			{
				Audio->FadeOut(FadeOutTime, 0.f);
			}
			else
			{
				Audio->Stop();
			}
		}
	}
}

void ALTTMusicPlayer::Skip()
{
	if (!bPlaying)
	{
		Play();
		return;
	}
	GetWorldTimerManager().ClearTimer(NextTrackTimer);
	PlayNext();
}

void ALTTMusicPlayer::SetMusicVolume(float NewVolume)
{
	Volume = FMath::Max(NewVolume, 0.f);
	// The component started last is the one playing (the other one is fading out or idle).
	UAudioComponent* Active = bUseA ? AudioB : AudioA;
	if (Active && Active->IsPlaying())
	{
		Active->AdjustVolume(0.25f, Volume);
	}
}

void ALTTMusicPlayer::SetMusicMuted(bool bMuted)
{
	bMusicMuted = bMuted;
	if (!GEngine)
	{
		return;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (UWorld* World = Context.World(); World && World->IsGameWorld())
		{
			for (TActorIterator<ALTTMusicPlayer> It(World); It; ++It)
			{
				It->ApplyMute();
			}
		}
	}
}

void ALTTMusicPlayer::ApplyMute()
{
	if (bMusicMuted)
	{
		if (bPlaying || LoadHandle.IsValid())
		{
			bResumeOnUnmute = true;
			Stop(0.4f);
		}
	}
	else if (bResumeOnUnmute)
	{
		bResumeOnUnmute = false;
		Play();
	}
}

void ALTTMusicPlayer::RefillBag()
{
	Bag.Reset();
	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		Bag.Add(i);
	}
	for (int32 i = Bag.Num() - 1; i > 0; --i)
	{
		Bag.Swap(i, FMath::RandRange(0, i));
	}
	// Tracks are popped from the end; don't start the new pass with the track that just played.
	if (Bag.Num() > 1 && Bag.Last() == LastIndex)
	{
		Bag.Swap(0, Bag.Num() - 1);
	}
}

int32 ALTTMusicPlayer::PickNextIndex()
{
	const int32 Num = Tracks.Num();
	if (Num == 0)
	{
		return INDEX_NONE;
	}
	if (!bShuffle)
	{
		ListIndex = (ListIndex + 1) % Num;
		return ListIndex;
	}
	while (Bag.Num() > 0 && !Tracks.IsValidIndex(Bag.Last()))
	{
		Bag.Pop(); // the list got shorter at runtime
	}
	if (Bag.Num() == 0)
	{
		RefillBag();
	}
	return Bag.Pop();
}

void ALTTMusicPlayer::PlayNext()
{
	if (!bPlaying)
	{
		return;
	}

	// Skip empty entries; give up if the whole list is empty.
	int32 Index = INDEX_NONE;
	for (int32 Tries = 0; Tries < Tracks.Num(); ++Tries)
	{
		const int32 Candidate = PickNextIndex();
		if (Tracks.IsValidIndex(Candidate) && !Tracks[Candidate].IsNull())
		{
			Index = Candidate;
			break;
		}
	}
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("MusicPlayer: no tracks to play."));
		bPlaying = false;
		return;
	}
	LastIndex = Index;

	const TSoftObjectPtr<USoundBase> Track = Tracks[Index];
	if (USoundBase* Loaded = Track.Get())
	{
		StartTrack(Loaded);
		return;
	}

	// Load in the background (tracks are large); the current one keeps playing meanwhile.
	LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Track.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(this, [this, Track]()
		{
			LoadHandle.Reset();
			if (USoundBase* Sound = Track.Get())
			{
				StartTrack(Sound);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MusicPlayer: couldn't load %s, skipping."), *Track.ToString());
				GetWorldTimerManager().SetTimer(NextTrackTimer, this, &ALTTMusicPlayer::PlayNext, 0.5f, false);
			}
		}));
}

void ALTTMusicPlayer::StartTrack(USoundBase* Sound)
{
	if (!bPlaying || !Sound)
	{
		return;
	}

	UAudioComponent* Old = bUseA ? AudioB : AudioA; // the one started last time
	UAudioComponent* New = bUseA ? AudioA : AudioB;
	bUseA = !bUseA;

	const float FadeIn = bFirstTrack ? FirstFadeInTime : CrossfadeTime;
	bFirstTrack = false;

	if (Old && Old->IsPlaying())
	{
		Old->FadeOut(FMath::Max(CrossfadeTime, 0.05f), 0.f);
	}
	New->Stop();
	New->SetSound(Sound);
	if (FadeIn > 0.f)
	{
		New->FadeIn(FadeIn, Volume);
	}
	else
	{
		New->SetVolumeMultiplier(Volume);
		New->Play();
	}
	CurrentTrack = Sound;

	// Duration keeps the source length even for looping waves; cues/MetaSounds without one use the fallback.
	float Length = Sound->Duration;
	if (Length <= 0.f || Length >= NoFixedLength)
	{
		Length = FallbackTrackLength;
	}
	GetWorldTimerManager().SetTimer(NextTrackTimer, this, &ALTTMusicPlayer::PlayNext, FMath::Max(Length - CrossfadeTime, 5.f), false);

	UE_LOG(LogTemp, Log, TEXT("MusicPlayer: now playing %s (%.0f s)."), *Sound->GetName(), Length);
	OnTrackStarted.Broadcast(Sound);
}
