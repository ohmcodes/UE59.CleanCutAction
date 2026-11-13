// Clean! Cut! Action! - Online leaderboard client (server: Leaderboard/ in the repo, hosted at cca.ohmcodes.com).
// The player is identified by a browser id: a GUID made on first run and kept in the browser's localStorage on the
// web build (a save game slot elsewhere), together with the name typed on the briefing screen.
// Every finished game is sent with SubmitRun; the server keeps each player's best score and best clean-set time.
//
// HTTP: the web build's engine HTTP module is a null stub (Wasm Docs/Networking.md), so on Wasm requests go through
// the browser's fetch() on the main thread and are polled from the game thread. Elsewhere FHttpModule is used.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "Containers/Ticker.h"
#include "LTTLeaderboardSubsystem.generated.h"

/** Name and browser id, for platforms without localStorage (and as a fallback on the web). */
UCLASS()
class CLEANCUTACTION_API ULTTPlayerProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString PlayerId;

	UPROPERTY()
	FString PlayerName;
};

USTRUCT(BlueprintType)
struct FLTTLeaderboardEntry
{
	GENERATED_BODY()

	/** 1-based place; 0 = not on the board yet (no finished game). */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 BestScore = 0;

	/** Fastest time on set of a run that cleaned the whole set; < 0 = none yet. */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	float BestTime = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 Runs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bIsYou = false;
};

USTRUCT(BlueprintType)
struct FLTTLeaderboardResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString Error;

	/** Best first. */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	TArray<FLTTLeaderboardEntry> Top;

	/** The local player's own row (Rank 0 = known name, no finished game yet). */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bHasYou = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FLTTLeaderboardEntry You;

	/** Players on the board. */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 TotalPlayers = 0;
};

USTRUCT(BlueprintType)
struct FLTTRunResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bNewBestScore = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bNewBestTime = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FLTTLeaderboardEntry You;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 TotalPlayers = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnLeaderboardFetched, const FLTTLeaderboardResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnRunSubmitted, const FLTTRunResult&, Result);

UCLASS(Config = Game)
class CLEANCUTACTION_API ULTTLeaderboardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static ULTTLeaderboardSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Longest name the server keeps. */
	static constexpr int32 MaxNameLength = 24;

	/** "Sleepy Barista 42" and friends. */
	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	static FString MakeRandomName();

	/** Trims, drops control characters and angle brackets, caps the length. */
	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	static FString SanitizeName(const FString& InName);

	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	const FString& GetPlayerId() const { return PlayerId; }

	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	const FString& GetPlayerName() const { return PlayerName; }

	/** Saves the name (an empty one becomes a random name) and tells the server if it changed. Returns the name kept. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	FString SetPlayerName(const FString& NewName);

	/** Picks, saves and returns a new random name. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	FString RandomizePlayerName();

	/** Sends a finished game. The server keeps the best score, and the best time when bSetClean. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void SubmitRun(int32 Score, float TimeSeconds, bool bSetClean, bool bTimeTrial, int32 Placed, int32 Total, int32 Broken, int32 Hints);

	/** Asks for the top Limit players and the local player's place. Waits for a run still being sent. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void FetchLeaderboard(int32 Limit = 10);

	/** The website, with ?player=<id> so the page highlights this player. */
	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	FString GetFullLeaderboardUrl() const;

	/** Opens the website in a new browser tab. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void OpenFullLeaderboard() const;

	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	bool IsSubmitting() const { return bSubmitInFlight; }

	/** The last answer to SubmitRun this session (bOk false before any). */
	UFUNCTION(BlueprintPure, Category = "Leaderboard")
	const FLTTRunResult& GetLastRunResult() const { return LastRunResult; }

	UPROPERTY(BlueprintAssignable, Category = "Leaderboard")
	FLTTOnLeaderboardFetched OnLeaderboardFetched;

	UPROPERTY(BlueprintAssignable, Category = "Leaderboard")
	FLTTOnRunSubmitted OnRunSubmitted;

	/** Server root. Override in DefaultGame.ini ([/Script/CleanCutAction.LTTLeaderboardSubsystem] BaseUrl=...)
	 *  or on the command line with -LeaderboardUrl=http://localhost:8787 */
	UPROPERTY(Config, EditAnywhere, Category = "Leaderboard")
	FString BaseUrl = TEXT("https://cca.ohmcodes.com");

	/** Path of the public page under BaseUrl. */
	UPROPERTY(Config, EditAnywhere, Category = "Leaderboard")
	FString LeaderboardPagePath = TEXT("/leaderboard");

	/** Off = nothing is sent (offline builds). */
	UPROPERTY(Config, EditAnywhere, Category = "Leaderboard")
	bool bEnabled = true;

private:
	using FResponseFn = TFunction<void(int32 /*Status*/, const FString& /*Body*/)>;

	void LoadProfile();
	void SaveProfile() const;
	void SendRequest(const FString& Verb, const FString& Path, const FString& JsonBody, FResponseFn Done);
	void DoFetch(int32 Limit);
	void PushNameToServer();
	FString ApiUrl(const FString& Path) const;

	static bool ParseEntry(const TSharedPtr<class FJsonObject>& Obj, FLTTLeaderboardEntry& Out);

	FString PlayerId;
	FString PlayerName;
	FString ResolvedBaseUrl;

	bool bSubmitInFlight = false;
	int32 QueuedFetchLimit = 0; // > 0: a fetch waits for the submit to finish
	FLTTRunResult LastRunResult;

#if PLATFORM_WASM
	bool TickWebRequests(float DeltaTime);
	TMap<int32, FResponseFn> PendingWeb;
	int32 NextWebRequestId = 1;
	FTSTicker::FDelegateHandle WebTicker;
#endif
};
