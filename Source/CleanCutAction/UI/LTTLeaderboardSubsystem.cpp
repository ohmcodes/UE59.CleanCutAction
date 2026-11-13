// Clean! Cut! Action!

#include "UI/LTTLeaderboardSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"

#if PLATFORM_WASM
#include <emscripten/em_asm.h>
#include <emscripten/threading.h>
#include <stdlib.h>
#else
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLTTLeaderboard, Log, All);

namespace LTTLeaderboardPrivate
{
	const TCHAR* SaveSlot = TEXT("LTTPlayerProfile");
	const TCHAR* IdKey = TEXT("cca_player_id");
	const TCHAR* NameKey = TEXT("cca_player_name");

#if PLATFORM_WASM
	// The game thread is a pthread Worker (PROXY_TO_PTHREAD); localStorage and the page's fetch live on the browser's
	// main thread, so these run there. Strings come back as a malloc'd UTF-8 buffer address (a double, like the
	// engine's WasmStartup helpers) that the caller frees.
	FString TakeJsString(double Address)
	{
		if (Address == 0.0)
		{
			return FString();
		}
		char* Ptr = reinterpret_cast<char*>(static_cast<uintptr_t>(Address));
		FString Result = UTF8_TO_TCHAR(Ptr);
		free(Ptr);
		return Result;
	}

	FString LocalStorageGet(const TCHAR* Key)
	{
		const double Address = MAIN_THREAD_EM_ASM_DOUBLE({
			var Value = null;
			try { Value = window.localStorage.getItem(UTF8ToString($0)); } catch (e) { Value = null; }
			return (Value === null || Value === undefined) ? 0 : stringToNewUTF8(Value);
		}, TCHAR_TO_UTF8(Key));
		return TakeJsString(Address);
	}

	void LocalStorageSet(const TCHAR* Key, const FString& Value)
	{
		FTCHARToUTF8 KeyUtf8(Key);
		FTCHARToUTF8 ValueUtf8(*Value);
		MAIN_THREAD_EM_ASM({
			try { window.localStorage.setItem(UTF8ToString($0), UTF8ToString($1)); } catch (e) {}
		}, KeyUtf8.Get(), ValueUtf8.Get());
	}

	/** Starts a fetch on the main thread; the answer waits in globalThis.__lttHttp[Id] until PollFetch takes it. */
	void StartFetch(int32 Id, const FString& Url, const FString& Verb, const FString& Body)
	{
		FTCHARToUTF8 UrlUtf8(*Url);
		FTCHARToUTF8 VerbUtf8(*Verb);
		FTCHARToUTF8 BodyUtf8(*Body);
		// Synchronous proxy: returns as soon as the fetch has started, while the UTF-8 buffers are still alive.
		MAIN_THREAD_EM_ASM({
			var Id = $0;
			var Url = UTF8ToString($1);
			var Verb = UTF8ToString($2);
			var Body = UTF8ToString($3);
			var Store = globalThis.__lttHttp || (globalThis.__lttHttp = {});
			Store[Id] = null;
			// No commas outside parentheses in here: EM_ASM's macro would split the code at them.
			var Options = {};
			Options.method = Verb;
			Options.mode = "cors";
			Options.credentials = "omit";
			Options.cache = "no-store";
			Options.headers = {};
			Options.headers["Accept"] = "application/json";
			if (Verb !== "GET") { Options.headers["Content-Type"] = "application/json"; Options.body = Body; }
			if (typeof AbortController !== "undefined") {
				var Controller = new AbortController();
				Options.signal = Controller.signal;
				setTimeout(function () { Controller.abort(); }, 15000);
			}
			fetch(Url, Options)
				.then(function (Response) { return Response.text().then(function (Text) { Store[Id] = { s: Response.status, b: Text }; }); })
				.catch(function (Error) { Store[Id] = { s: 0, b: String(Error) }; });
		}, Id, UrlUtf8.Get(), VerbUtf8.Get(), BodyUtf8.Get());
	}

	/** True when the request finished: Status 0 = network error. */
	bool PollFetch(int32 Id, int32& OutStatus, FString& OutBody)
	{
		const double Address = MAIN_THREAD_EM_ASM_DOUBLE({
			var Store = globalThis.__lttHttp;
			if (!Store) { return 0; }
			var Result = Store[$0];
			if (!Result) { return 0; }
			delete Store[$0];
			return stringToNewUTF8(String(Result.s) + String.fromCharCode(10) + Result.b);
		}, Id);
		if (Address == 0.0)
		{
			return false;
		}
		const FString Text = TakeJsString(Address);
		FString StatusText;
		if (!Text.Split(TEXT("\n"), &StatusText, &OutBody))
		{
			StatusText = Text;
			OutBody.Reset();
		}
		OutStatus = FCString::Atoi(*StatusText);
		return true;
	}
#endif

	TSharedPtr<FJsonObject> ParseJson(const FString& Body)
	{
		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Obj))
		{
			return nullptr;
		}
		return Obj;
	}

	FString WriteJson(const TSharedRef<FJsonObject>& Obj)
	{
		FString Out;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Obj, Writer);
		return Out;
	}

	FString ErrorFrom(int32 Status, const TSharedPtr<FJsonObject>& Obj)
	{
		FString Error;
		if (Obj.IsValid() && Obj->TryGetStringField(TEXT("error"), Error))
		{
			return Error;
		}
		return Status == 0 ? FString(TEXT("offline")) : FString::Printf(TEXT("HTTP %d"), Status);
	}
}

ULTTLeaderboardSubsystem* ULTTLeaderboardSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<ULTTLeaderboardSubsystem>() : nullptr;
}

void ULTTLeaderboardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ResolvedBaseUrl = BaseUrl;
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("LeaderboardUrl="), Override) && !Override.IsEmpty())
	{
		ResolvedBaseUrl = Override;
	}
	ResolvedBaseUrl.RemoveFromEnd(TEXT("/"));

	LoadProfile();

#if PLATFORM_WASM
	WebTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ULTTLeaderboardSubsystem::TickWebRequests), 0.1f);
#endif
	UE_LOG(LogLTTLeaderboard, Log, TEXT("Leaderboard %s as '%s' (%s)"), *ResolvedBaseUrl, *PlayerName, *PlayerId);
}

void ULTTLeaderboardSubsystem::Deinitialize()
{
#if PLATFORM_WASM
	FTSTicker::RemoveTicker(WebTicker);
	PendingWeb.Empty();
#endif
	Super::Deinitialize();
}

// ---------------------------------------------------------------- names and profile

FString ULTTLeaderboardSubsystem::MakeRandomName()
{
	static const TCHAR* Adjectives[] = {
		TEXT("Sleepy"), TEXT("Speedy"), TEXT("Clumsy"), TEXT("Tidy"), TEXT("Dusty"), TEXT("Jittery"), TEXT("Fearless"),
		TEXT("Caffeinated"), TEXT("Sparkly"), TEXT("Grumpy"), TEXT("Lucky"), TEXT("Sneaky"), TEXT("Brave"), TEXT("Fancy"),
		TEXT("Wobbly"), TEXT("Zippy"), TEXT("Frothy"), TEXT("Spotless"), TEXT("Dramatic"), TEXT("Nervous"), TEXT("Cozy"),
		TEXT("Hasty"), TEXT("Steady"), TEXT("Golden"),
	};
	static const TCHAR* Nouns[] = {
		TEXT("Barista"), TEXT("Rookie"), TEXT("Gaffer"), TEXT("Grip"), TEXT("Extra"), TEXT("Stuntman"), TEXT("Croissant"),
		TEXT("Espresso"), TEXT("Muffin"), TEXT("Latte"), TEXT("Boom Op"), TEXT("Clapper"), TEXT("Mopper"), TEXT("Busser"),
		TEXT("Teacup"), TEXT("Runner"), TEXT("Scone"), TEXT("Stagehand"), TEXT("Cappuccino"), TEXT("Donut"), TEXT("Intern"),
		TEXT("Propmaster"),
	};
	const TCHAR* Adjective = Adjectives[FMath::RandRange(0, UE_ARRAY_COUNT(Adjectives) - 1)];
	const TCHAR* Noun = Nouns[FMath::RandRange(0, UE_ARRAY_COUNT(Nouns) - 1)];
	return SanitizeName(FString::Printf(TEXT("%s %s %d"), Adjective, Noun, FMath::RandRange(10, 99)));
}

FString ULTTLeaderboardSubsystem::SanitizeName(const FString& InName)
{
	FString Out;
	Out.Reserve(InName.Len());
	bool bLastSpace = true; // also drops leading spaces
	for (const TCHAR C : InName)
	{
		if (C < 32 || C == 127 || C == TEXT('<') || C == TEXT('>'))
		{
			continue;
		}
		const bool bSpace = FChar::IsWhitespace(C);
		if (bSpace && bLastSpace)
		{
			continue;
		}
		Out.AppendChar(bSpace ? TEXT(' ') : C);
		bLastSpace = bSpace;
	}
	Out.TrimEndInline();
	if (Out.Len() > MaxNameLength)
	{
		Out.LeftInline(MaxNameLength);
		Out.TrimEndInline();
	}
	return Out;
}

void ULTTLeaderboardSubsystem::LoadProfile()
{
	using namespace LTTLeaderboardPrivate;
#if PLATFORM_WASM
	PlayerId = LocalStorageGet(IdKey);
	PlayerName = LocalStorageGet(NameKey);
#endif
	if (PlayerId.IsEmpty() || PlayerName.IsEmpty())
	{
		if (const ULTTPlayerProfileSave* Save = Cast<ULTTPlayerProfileSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0)))
		{
			if (PlayerId.IsEmpty())
			{
				PlayerId = Save->PlayerId;
			}
			if (PlayerName.IsEmpty())
			{
				PlayerName = Save->PlayerName;
			}
		}
	}

	bool bChanged = false;
	if (PlayerId.Len() < 8)
	{
		PlayerId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
		bChanged = true;
	}
	const FString Clean = SanitizeName(PlayerName);
	if (Clean.IsEmpty() || Clean != PlayerName)
	{
		PlayerName = Clean.IsEmpty() ? MakeRandomName() : Clean;
		bChanged = true;
	}
	if (bChanged)
	{
		SaveProfile();
	}
}

void ULTTLeaderboardSubsystem::SaveProfile() const
{
	using namespace LTTLeaderboardPrivate;
#if PLATFORM_WASM
	LocalStorageSet(IdKey, PlayerId);
	LocalStorageSet(NameKey, PlayerName);
#endif
	if (ULTTPlayerProfileSave* Save = Cast<ULTTPlayerProfileSave>(UGameplayStatics::CreateSaveGameObject(ULTTPlayerProfileSave::StaticClass())))
	{
		Save->PlayerId = PlayerId;
		Save->PlayerName = PlayerName;
		UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
	}
}

FString ULTTLeaderboardSubsystem::SetPlayerName(const FString& NewName)
{
	FString Clean = SanitizeName(NewName);
	if (Clean.IsEmpty())
	{
		Clean = MakeRandomName();
	}
	if (Clean != PlayerName)
	{
		PlayerName = Clean;
		SaveProfile();
		// A run always carries the current name; only a rename after this session's run needs its own request.
		if (LastRunResult.bOk)
		{
			PushNameToServer();
		}
	}
	return PlayerName;
}

FString ULTTLeaderboardSubsystem::RandomizePlayerName()
{
	return SetPlayerName(MakeRandomName());
}

void ULTTLeaderboardSubsystem::PushNameToServer()
{
	if (!bEnabled)
	{
		return;
	}
	const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), PlayerName);
	SendRequest(TEXT("PUT"), FString::Printf(TEXT("/api/players/%s"), *PlayerId), LTTLeaderboardPrivate::WriteJson(Obj),
		[](int32 Status, const FString& Body)
		{
			UE_CLOG(Status != 200, LogLTTLeaderboard, Warning, TEXT("Rename failed (%d): %s"), Status, *Body.Left(200));
		});
}

// ---------------------------------------------------------------- requests

FString ULTTLeaderboardSubsystem::ApiUrl(const FString& Path) const
{
	return ResolvedBaseUrl + Path;
}

FString ULTTLeaderboardSubsystem::GetFullLeaderboardUrl() const
{
	return FString::Printf(TEXT("%s%s?player=%s"), *ResolvedBaseUrl, *LeaderboardPagePath, *PlayerId);
}

void ULTTLeaderboardSubsystem::OpenFullLeaderboard() const
{
	FString Error;
	FPlatformProcess::LaunchURL(*GetFullLeaderboardUrl(), nullptr, &Error);
	UE_CLOG(!Error.IsEmpty(), LogLTTLeaderboard, Warning, TEXT("Could not open the leaderboard page: %s"), *Error);
}

void ULTTLeaderboardSubsystem::SendRequest(const FString& Verb, const FString& Path, const FString& JsonBody, FResponseFn Done)
{
	const FString Url = ApiUrl(Path);
#if PLATFORM_WASM
	const int32 Id = NextWebRequestId++;
	PendingWeb.Add(Id, MoveTemp(Done));
	LTTLeaderboardPrivate::StartFetch(Id, Url, Verb, JsonBody);
#else
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	if (Verb != TEXT("GET"))
	{
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(JsonBody);
	}
	Request->SetTimeout(15.f);
	TWeakObjectPtr<ULTTLeaderboardSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, Done = MoveTemp(Done)](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			const int32 Status = (bConnected && Response.IsValid()) ? Response->GetResponseCode() : 0;
			Done(Status, Response.IsValid() ? Response->GetContentAsString() : FString());
		});
	Request->ProcessRequest();
#endif
}

#if PLATFORM_WASM
bool ULTTLeaderboardSubsystem::TickWebRequests(float DeltaTime)
{
	if (PendingWeb.IsEmpty())
	{
		return true;
	}
	TArray<int32> Ids;
	PendingWeb.GetKeys(Ids);
	for (const int32 Id : Ids)
	{
		int32 Status = 0;
		FString Body;
		if (LTTLeaderboardPrivate::PollFetch(Id, Status, Body))
		{
			FResponseFn Done;
			PendingWeb.RemoveAndCopyValue(Id, Done);
			if (Done)
			{
				Done(Status, Body);
			}
		}
	}
	return true;
}
#endif

bool ULTTLeaderboardSubsystem::ParseEntry(const TSharedPtr<FJsonObject>& Obj, FLTTLeaderboardEntry& Out)
{
	if (!Obj.IsValid())
	{
		return false;
	}
	Out.Rank = Obj->GetIntegerField(TEXT("rank"));
	Out.Name = Obj->GetStringField(TEXT("name"));
	Out.BestScore = Obj->GetIntegerField(TEXT("bestScore"));
	double Time = -1.0;
	Out.BestTime = Obj->TryGetNumberField(TEXT("bestTime"), Time) ? static_cast<float>(Time) : -1.f;
	Out.Runs = Obj->GetIntegerField(TEXT("runs"));
	Obj->TryGetBoolField(TEXT("isYou"), Out.bIsYou);
	return true;
}

void ULTTLeaderboardSubsystem::SubmitRun(int32 Score, float TimeSeconds, bool bSetClean, bool bTimeTrial, int32 Placed, int32 Total, int32 Broken, int32 Hints)
{
	if (!bEnabled)
	{
		return;
	}
	const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("playerId"), PlayerId);
	Obj->SetStringField(TEXT("name"), PlayerName);
	Obj->SetNumberField(TEXT("score"), Score);
	Obj->SetNumberField(TEXT("timeSeconds"), FMath::RoundToDouble(FMath::Max(TimeSeconds, 0.f) * 100.0) / 100.0);
	Obj->SetBoolField(TEXT("setClean"), bSetClean);
	Obj->SetBoolField(TEXT("timeTrial"), bTimeTrial);
	Obj->SetNumberField(TEXT("placed"), FMath::Max(Placed, 0));
	Obj->SetNumberField(TEXT("total"), FMath::Max(Total, 0));
	Obj->SetNumberField(TEXT("broken"), FMath::Max(Broken, 0));
	Obj->SetNumberField(TEXT("hints"), FMath::Max(Hints, 0));

	bSubmitInFlight = true;
	TWeakObjectPtr<ULTTLeaderboardSubsystem> WeakThis(this);
	SendRequest(TEXT("POST"), TEXT("/api/runs"), LTTLeaderboardPrivate::WriteJson(Obj), [WeakThis](int32 Status, const FString& Body)
	{
		ULTTLeaderboardSubsystem* Self = WeakThis.Get();
		if (!Self)
		{
			return;
		}
		Self->bSubmitInFlight = false;

		FLTTRunResult Result;
		const TSharedPtr<FJsonObject> Json = LTTLeaderboardPrivate::ParseJson(Body);
		bool bOk = false;
		if (Status == 200 && Json.IsValid() && Json->TryGetBoolField(TEXT("ok"), bOk) && bOk)
		{
			Result.bOk = true;
			Json->TryGetBoolField(TEXT("newBestScore"), Result.bNewBestScore);
			Json->TryGetBoolField(TEXT("newBestTime"), Result.bNewBestTime);
			const TSharedPtr<FJsonObject>* Player = nullptr;
			if (Json->TryGetObjectField(TEXT("player"), Player))
			{
				ParseEntry(*Player, Result.You);
			}
			Result.TotalPlayers = Json->GetIntegerField(TEXT("total"));
		}
		else
		{
			Result.Error = LTTLeaderboardPrivate::ErrorFrom(Status, Json);
			UE_LOG(LogLTTLeaderboard, Warning, TEXT("Submit failed: %s"), *Result.Error);
		}
		Self->LastRunResult = Result;
		Self->OnRunSubmitted.Broadcast(Result);

		if (Self->QueuedFetchLimit > 0)
		{
			const int32 Limit = Self->QueuedFetchLimit;
			Self->QueuedFetchLimit = 0;
			Self->DoFetch(Limit);
		}
	});
}

void ULTTLeaderboardSubsystem::FetchLeaderboard(int32 Limit)
{
	Limit = FMath::Clamp(Limit, 1, 100);
	if (!bEnabled)
	{
		FLTTLeaderboardResult Result;
		Result.Error = TEXT("disabled");
		OnLeaderboardFetched.Broadcast(Result);
		return;
	}
	if (bSubmitInFlight)
	{
		QueuedFetchLimit = Limit; // show the board with this run on it
		return;
	}
	DoFetch(Limit);
}

void ULTTLeaderboardSubsystem::DoFetch(int32 Limit)
{
	TWeakObjectPtr<ULTTLeaderboardSubsystem> WeakThis(this);
	SendRequest(TEXT("GET"), FString::Printf(TEXT("/api/leaderboard?limit=%d&player=%s"), Limit, *PlayerId), FString(),
		[WeakThis](int32 Status, const FString& Body)
		{
			ULTTLeaderboardSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			FLTTLeaderboardResult Result;
			const TSharedPtr<FJsonObject> Json = LTTLeaderboardPrivate::ParseJson(Body);
			bool bOk = false;
			if (Status == 200 && Json.IsValid() && Json->TryGetBoolField(TEXT("ok"), bOk) && bOk)
			{
				Result.bOk = true;
				Result.TotalPlayers = Json->GetIntegerField(TEXT("total"));
				const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
				if (Json->TryGetArrayField(TEXT("entries"), Entries))
				{
					for (const TSharedPtr<FJsonValue>& Value : *Entries)
					{
						FLTTLeaderboardEntry Entry;
						if (ParseEntry(Value->AsObject(), Entry))
						{
							Result.Top.Add(Entry);
						}
					}
				}
				const TSharedPtr<FJsonObject>* You = nullptr;
				if (Json->TryGetObjectField(TEXT("you"), You) && You && You->IsValid())
				{
					Result.bHasYou = ParseEntry(*You, Result.You);
				}
			}
			else
			{
				Result.Error = LTTLeaderboardPrivate::ErrorFrom(Status, Json);
				UE_LOG(LogLTTLeaderboard, Warning, TEXT("Fetch failed: %s"), *Result.Error);
			}
			Self->OnLeaderboardFetched.Broadcast(Result);
		});
}
