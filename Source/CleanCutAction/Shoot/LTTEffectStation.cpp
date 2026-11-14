// Clean! Cut! Action!

#include "Shoot/LTTEffectStation.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Shoot/LTTSceneDirector.h"
#include "Shoot/LTTFilmActor.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Light.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LTTEffectStation"

ALTTEffectStation::ALTTEffectStation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only while someone is using it or a prop is in the air

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetupAttachment(Root);
	InteractBox->SetBoxExtent(FVector(40.f, 40.f, 60.f));
	InteractBox->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // the interaction trace
	InteractBox->SetCanEverAffectNavigation(false);

	TossProp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TossProp"));
	TossProp->SetupAttachment(Root);
	TossProp->SetUsingAbsoluteLocation(true);
	TossProp->SetUsingAbsoluteRotation(true);
	TossProp->SetUsingAbsoluteScale(true);
	TossProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TossProp->SetCanEverAffectNavigation(false);
	TossProp->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		TossProp->SetStaticMesh(CubeAsset.Object);
		TossProp->SetWorldScale3D(FVector(0.2f));
	}

	FLTTStationChoice Orange;
	Orange.Name = LOCTEXT("GelOrange", "Orange");
	Orange.Color = FLinearColor(1.f, 0.45f, 0.1f);
	Orange.CorrectFor = ELTTCue::Sunset;
	FLTTStationChoice Blue;
	Blue.Name = LOCTEXT("GelBlue", "Blue");
	Blue.Color = FLinearColor(0.25f, 0.45f, 1.f);
	Blue.CorrectFor = ELTTCue::Moonlight;
	FLTTStationChoice Green;
	Green.Name = LOCTEXT("GelGreen", "Green");
	Green.Color = FLinearColor(0.1f, 1.f, 0.2f);
	Choices = { Orange, Blue, Green };
}

void ALTTEffectStation::BeginPlay()
{
	Super::BeginPlay();
	Director = ALTTSceneDirector::Get(this);
	SetEffectActorsOn(false);
}

void ALTTEffectStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LeaveStation(false);
	Super::EndPlay(EndPlayReason);
}

// ---- Interaction ----

bool ALTTEffectStation::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return IsCueActive() && !IsOccupied() && !bInFlight;
}

FText ALTTEffectStation::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	return FText::Format(LOCTEXT("Prompt", "Use {0}"), DisplayName.IsEmpty() ? ULTTSceneData::GetCueName(ActiveCue) : DisplayName);
}

void ALTTEffectStation::OnInteract_Implementation(AActor* InstigatorActor)
{
	if (CanInteract_Implementation(InstigatorActor))
	{
		EnterStation(Cast<APawn>(InstigatorActor));
	}
}

void ALTTEffectStation::SetCueActive(ELTTCue Cue, bool bActive)
{
	if (!Director.IsValid())
	{
		Director = ALTTSceneDirector::Get(this);
	}
	const ELTTCue NewCue = bActive ? Cue : ELTTCue::None;
	if (NewCue == ActiveCue)
	{
		return;
	}
	if (!bActive)
	{
		LeaveStation(false);
	}
	else
	{
		bHadWrongChoice = false;
	}
	ActiveCue = NewCue;
	SetHighlighted(bActive);
	ReceiveCueActiveChanged(Cue, bActive);
}

void ALTTEffectStation::SetHighlighted(bool bOn)
{
	auto Apply = [this, bOn](AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}
		TInlineComponentArray<UPrimitiveComponent*> Prims(Actor);
		for (UPrimitiveComponent* Prim : Prims)
		{
			if (Prim != InteractBox && Prim != TossProp)
			{
				Prim->SetRenderCustomDepth(bOn);
				Prim->SetCustomDepthStencilValue(HighlightStencil);
			}
		}
	};
	Apply(this);
	for (AActor* Actor : HighlightActors)
	{
		Apply(Actor);
	}
}

void ALTTEffectStation::EnterStation(APawn* Player)
{
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC || IsOccupied())
	{
		return;
	}
	User = Player;
	PC->SetIgnoreMoveInput(true);
	bLookLocked = Game != ELTTStationGame::Toss; // tossing is aimed with the camera
	if (bLookLocked)
	{
		PC->SetIgnoreLookInput(true);
	}

	bArmed = false;
	bPrevAction = true;
	bPrevExit = true;
	TimeInStation = 0.f;
	Meter = RawMeter = 0.f;
	bHolding = false;
	HoldElapsed = 0.f;
	InBandTime = ActiveTime = OverTime = 0.f;
	LastShakeSign = 0;
	bFlashPhase = false;
	FlashElapsed = 0.f;
	ChargePhase = 0.f;
	bOnTarget = false;

	SetActorTickEnabled(true);
	ReceiveEntered(Player);
}

void ALTTEffectStation::LeaveStation(bool bBailed)
{
	APawn* Player = User.Get();
	if (!Player)
	{
		User.Reset();
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		// Counters in the engine: cleared once for each time they were set.
		PC->SetIgnoreMoveInput(false);
		if (bLookLocked)
		{
			PC->SetIgnoreLookInput(false);
		}
	}
	bLookLocked = false;
	User.Reset();
	if (!bInFlight)
	{
		SetActorTickEnabled(false);
	}
	ReceiveExited();
	if (bBailed && IsCueActive() && Director.IsValid())
	{
		Director->Bark(LOCTEXT("Bail", "Where are you going? The cue isn't done!"));
	}
}

void ALTTEffectStation::ExitStation()
{
	LeaveStation(false);
}

void ALTTEffectStation::Finish(ELTTGrade Grade)
{
	LeaveStation(false);
	if (ALTTSceneDirector* D = Director.Get())
	{
		D->ReportCueResult(this, Grade);
	}
}

ELTTGrade ALTTEffectStation::GradeFromQuality(float Quality)
{
	return Quality >= 0.85f ? ELTTGrade::Perfect : Quality >= 0.6f ? ELTTGrade::Good : Quality >= 0.3f ? ELTTGrade::Late : ELTTGrade::Miss;
}

ELTTGrade ALTTEffectStation::HandleCueTimeout()
{
	ELTTGrade Grade = ELTTGrade::Miss;
	if (bInFlight)
	{
		Grade = FlightGrade == ELTTGrade::Miss ? ELTTGrade::Miss : ELTTGrade::Late; // thrown at the buzzer
	}
	else if (IsOccupied() && Game == ELTTStationGame::Sustain && InBandTime >= SustainTime * 0.6f)
	{
		Grade = ELTTGrade::Late; // most of the way there
	}
	LeaveStation(false);
	return Grade;
}

// ---- Mini-games ----

void ALTTEffectStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInFlight)
	{
		TickFlight(DeltaSeconds);
	}

	APawn* Player = User.Get();
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC)
	{
		if (Player)
		{
			LeaveStation(false);
		}
		else if (!bInFlight)
		{
			SetActorTickEnabled(false);
		}
		return;
	}

	TimeInStation += DeltaSeconds;
	const bool bAction = PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::Gamepad_RightTrigger);
	const bool bExit = PC->IsInputKeyDown(EKeys::E) || PC->IsInputKeyDown(EKeys::RightMouseButton) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right);
	const bool bPressed = bAction && !bPrevAction;
	const bool bExitPressed = bExit && !bPrevExit;
	bPrevAction = bAction;
	bPrevExit = bExit;

	// The click (or E) that opened the station doesn't count as its first input.
	if (!bArmed)
	{
		bArmed = TimeInStation > 0.15f && !bAction && !bExit;
		return;
	}
	if (bExitPressed && !bInFlight)
	{
		LeaveStation(true);
		return;
	}

	switch (Game)
	{
	case ELTTStationGame::Press:
		if (bPressed)
		{
			Finish(ELTTGrade::Perfect);
		}
		break;

	case ELTTStationGame::Timing:
	{
		Meter += DeltaSeconds / FMath::Max(TimingTravelTime, 0.1f);
		if (bPressed)
		{
			const float ErrorSeconds = FMath::Abs(Meter - TimingTarget) * TimingTravelTime;
			Finish(ErrorSeconds <= 0.15f ? ELTTGrade::Perfect : ErrorSeconds <= 0.35f ? ELTTGrade::Good : ErrorSeconds <= 0.7f ? ELTTGrade::Late : ELTTGrade::Miss);
		}
		else if (Meter >= 1.f)
		{
			Finish(ELTTGrade::Miss);
		}
		break;
	}

	case ELTTStationGame::HoldRelease:
	{
		if (bAction)
		{
			bHolding = true;
			HoldElapsed += DeltaSeconds;
			const float A = HoldElapsed / FMath::Max(HoldFillTime, 0.1f);
			Meter = FMath::Clamp(1.f - FMath::Square(1.f - FMath::Min(A, 1.f)), 0.f, 1.f); // fast at first, slowing near the top
			if (A >= 1.2f)
			{
				// Over-pressure: fog everywhere.
				if (Director.IsValid())
				{
					Director->Bark(LOCTEXT("OverSteam", "Too much! It's a cafe, not a sauna!"));
				}
				Finish(ELTTGrade::Miss);
			}
		}
		else if (bHolding)
		{
			const float Lo = static_cast<float>(HoldPerfectBand.X);
			const float Hi = static_cast<float>(HoldPerfectBand.Y);
			ELTTGrade Grade = ELTTGrade::Miss;
			if (Meter >= Lo && Meter <= Hi)
			{
				Grade = ELTTGrade::Perfect;
			}
			else if (Meter >= Lo - HoldGoodMargin && Meter <= Hi + HoldGoodMargin)
			{
				Grade = ELTTGrade::Good;
			}
			else if (Meter >= 0.3f)
			{
				Grade = ELTTGrade::Late;
			}
			Finish(Grade);
		}
		break;
	}

	case ELTTStationGame::Sustain:
	{
		const bool bPush = bAction || PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up);
		RawMeter = FMath::Clamp(RawMeter + (bPush ? SustainRise : -SustainFall) * DeltaSeconds, 0.f, 1.f);
		const float Sway = SustainWobble > 0.f ? FMath::PerlinNoise1D(TimeInStation * 1.7f) * SustainWobble : 0.f;
		Meter = FMath::Clamp(RawMeter + Sway, 0.f, 1.f);

		const bool bInBand = Meter >= SustainBand.X && Meter <= SustainBand.Y;
		if (bInBand)
		{
			InBandTime += DeltaSeconds;
		}
		if (InBandTime > 0.f)
		{
			ActiveTime += DeltaSeconds; // the ramp up to the band is free
		}
		if (SustainOverLevel < 1.f && Meter > SustainOverLevel)
		{
			OverTime += DeltaSeconds;
		}
		if (InBandTime >= SustainTime)
		{
			const float Quality = InBandTime / FMath::Max(ActiveTime, 0.01f) - 0.5f * OverTime / FMath::Max(SustainTime, 0.1f);
			Finish(GradeFromQuality(Quality));
		}
		break;
	}

	case ELTTStationGame::Shake:
	{
		if (!bFlashPhase)
		{
			float MouseX = 0.f, MouseY = 0.f;
			PC->GetInputMouseDelta(MouseX, MouseY);
			int32 Sign = 0;
			if (FMath::Abs(MouseX) > 0.5f)
			{
				Sign = MouseX > 0.f ? 1 : -1;
			}
			if (PC->WasInputKeyJustPressed(EKeys::A))
			{
				Sign = -1;
			}
			else if (PC->WasInputKeyJustPressed(EKeys::D))
			{
				Sign = 1;
			}
			if (Sign != 0 && Sign != LastShakeSign)
			{
				LastShakeSign = Sign;
				Meter += ShakeGain;
			}
			Meter = FMath::Clamp(Meter - ShakeDecay * DeltaSeconds, 0.f, 1.f);
			if (Meter >= 1.f)
			{
				bFlashPhase = true;
				FlashElapsed = 0.f;
			}
		}
		else
		{
			FlashElapsed += DeltaSeconds;
			if (bPressed || PC->WasInputKeyJustPressed(EKeys::F))
			{
				Finish(FlashElapsed <= 0.4f ? ELTTGrade::Perfect : FlashElapsed <= 0.8f ? ELTTGrade::Good : ELTTGrade::Late);
			}
			else if (FlashElapsed > 2.5f)
			{
				Finish(ELTTGrade::Late);
			}
		}
		break;
	}

	case ELTTStationGame::Choice:
	{
		const int32 Num = Choices.Num();
		if (Num == 0)
		{
			Finish(ELTTGrade::Good);
			break;
		}
		const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
		for (int32 i = 0; i < FMath::Min(Num, 5); ++i)
		{
			if (PC->WasInputKeyJustPressed(NumberKeys[i]))
			{
				SelectedChoice = i;
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::D))
		{
			SelectedChoice = (SelectedChoice + 1) % Num;
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::A))
		{
			SelectedChoice = (SelectedChoice + Num - 1) % Num;
		}
		SelectedChoice = FMath::Clamp(SelectedChoice, 0, Num - 1);
		if (bPressed)
		{
			ApplyChoice(SelectedChoice);
			if (Choices[SelectedChoice].CorrectFor == ActiveCue)
			{
				Finish(bHadWrongChoice ? ELTTGrade::Late : ELTTGrade::Perfect); // a swap after a wrong gel is a late save
			}
			else
			{
				bHadWrongChoice = true;
				if (Director.IsValid())
				{
					Director->Bark(FText::Format(LOCTEXT("WrongGel", "{0}?! WRONG GEL! WRONG GEL!"), Choices[SelectedChoice].Name.ToUpper()));
				}
			}
		}
		break;
	}

	case ELTTStationGame::Toss:
		TickToss(PC, DeltaSeconds, bAction);
		break;
	}
}

void ALTTEffectStation::TickToss(APlayerController* PC, float Dt, bool bAction)
{
	if (bInFlight)
	{
		return;
	}
	const ALTTFilmActor* Target = Director.IsValid() ? Director->FindFilmActor(TossTargetName) : nullptr;
	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector ViewDir = ViewRot.Vector();
	// No actor placed yet: a spot in front of the station so the mini-game still works.
	const FVector TargetLoc = Target ? Target->GetCatchLocation() : GetActorLocation() + GetActorForwardVector() * 500.f + FVector(0.f, 0.f, 120.f);
	const FVector ToTarget = TargetLoc - ViewLoc;
	const float Distance = static_cast<float>(ToTarget.Size());
	const float AimDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(ViewDir, ToTarget.GetSafeNormal())), -1.f, 1.f)));
	TossIdeal = FMath::Clamp(Distance / FMath::Max(TossMaxRange, 1.f), 0.2f, 0.92f);
	bOnTarget = AimDegrees <= 8.f;

	if (bAction)
	{
		bHolding = true;
		ChargePhase += Dt / FMath::Max(TossChargeTime, 0.1f);
		const float Saw = FMath::Fmod(ChargePhase, 2.f);
		Meter = Saw <= 1.f ? Saw : 2.f - Saw; // up, then back down
		return;
	}
	if (!bHolding)
	{
		return;
	}

	// Released: throw.
	bHolding = false;
	const float PowerError = FMath::Abs(Meter - TossIdeal);
	ELTTGrade Grade = ELTTGrade::Miss;
	if (PowerError <= 0.07f && AimDegrees <= 4.f)
	{
		Grade = ELTTGrade::Perfect;
	}
	else if (PowerError <= 0.15f && AimDegrees <= 8.f)
	{
		Grade = ELTTGrade::Good;
	}
	else if (PowerError <= 0.28f && AimDegrees <= 16.f)
	{
		Grade = ELTTGrade::Late; // lands at their feet
	}

	FlightGrade = Grade;
	FlightFrom = ViewLoc + ViewDir * 45.f - FVector(0.f, 0.f, 20.f);
	if (Grade == ELTTGrade::Perfect || Grade == ELTTGrade::Good)
	{
		FlightTo = TargetLoc;
	}
	else if (Grade == ELTTGrade::Late)
	{
		FlightTo = TargetLoc + ViewDir.GetSafeNormal2D() * -60.f - FVector(0.f, 0.f, 110.f);
	}
	else if (AimDegrees <= 16.f && Target)
	{
		FlightTo = Target->GetActorLocation() + FVector(0.f, 0.f, 80.f); // bonk
	}
	else
	{
		FlightTo = ViewLoc + ViewDir * FMath::Max(TossMaxRange * Meter, 200.f);
	}
	FlightArc = 50.f + 0.12f * static_cast<float>(FVector::Dist(FlightFrom, FlightTo));
	FlightElapsed = 0.f;
	FlightTime = FMath::Clamp(static_cast<float>(FVector::Dist(FlightFrom, FlightTo)) / 1100.f, 0.35f, 1.1f);
	bInFlight = true;

	GetWorldTimerManager().ClearTimer(TossHideTimer);
	if (const TObjectPtr<UStaticMesh>* Mesh = TossMeshes.Find(ActiveCue); Mesh && *Mesh)
	{
		TossProp->SetStaticMesh(*Mesh);
		TossProp->SetWorldScale3D(FVector(1.f));
	}
	TossProp->SetWorldLocation(FlightFrom);
	TossProp->SetVisibility(true);
}

void ALTTEffectStation::TickFlight(float Dt)
{
	FlightElapsed += Dt;
	const float A = FMath::Clamp(FlightElapsed / FMath::Max(FlightTime, 0.05f), 0.f, 1.f);
	FVector Pos = FMath::Lerp(FlightFrom, FlightTo, A);
	Pos.Z += FlightArc * 4.f * A * (1.f - A);
	TossProp->SetWorldLocation(Pos);
	TossProp->AddWorldRotation(FRotator(360.f * Dt, 200.f * Dt, 0.f));
	if (A < 1.f)
	{
		return;
	}
	bInFlight = false;
	const ELTTGrade Grade = FlightGrade;
	// Caught props leave the air at once; fumbled ones lie there for a moment.
	const float HideDelay = (Grade == ELTTGrade::Perfect || Grade == ELTTGrade::Good) ? 0.05f : 2.f;
	GetWorldTimerManager().SetTimer(TossHideTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { TossProp->SetVisibility(false); }), HideDelay, false);
	if (IsOccupied())
	{
		Finish(Grade);
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

// ---- Choice ----

int32 ALTTEffectStation::FindCorrectChoice() const
{
	return Choices.IndexOfByPredicate([this](const FLTTStationChoice& C) { return C.CorrectFor == ActiveCue; });
}

void ALTTEffectStation::ApplyChoice(int32 Index)
{
	if (!Choices.IsValidIndex(Index) || !IsValid(KeyLight) || !KeyLight->GetLightComponent())
	{
		return;
	}
	ULightComponent* Light = KeyLight->GetLightComponent();
	if (!bKeyLightColorSaved)
	{
		SavedKeyLightColor = Light->GetLightColor();
		bKeyLightColorSaved = true;
	}
	Light->SetLightColor(Choices[Index].Color);
}

// ---- Effect ----

void ALTTEffectStation::SetEffectActorsOn(bool bOn)
{
	for (AActor* Actor : EffectActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		Actor->SetActorHiddenInGame(!bOn);
		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (UActorComponent* Component : Components)
		{
			// Niagara systems and audio restart from the top; lights and meshes just show.
			if (bOn)
			{
				Component->Activate(true);
			}
			else
			{
				Component->Deactivate();
			}
		}
	}
}

namespace
{
	/** Stops and removes the looping effect sounds (rain) a station started; one-shots aren't kept. */
	void StopStationLoops(AActor* Station)
	{
		TInlineComponentArray<UAudioComponent*> Audio(Station);
		for (UAudioComponent* Component : Audio)
		{
			if (Component && Component->ComponentHasTag(TEXT("StationLoop")))
			{
				Component->Stop();
				Component->DestroyComponent();
			}
		}
	}
}

void ALTTEffectStation::PlayEffect(ELTTCue Cue, ELTTGrade Grade)
{
	const bool bMissed = Grade == ELTTGrade::Miss;
	GetWorldTimerManager().ClearTimer(EffectTimer);
	if (!bMissed || bShowEffectOnMiss)
	{
		SetEffectActorsOn(true);
		if (EffectDuration > 0.f)
		{
			GetWorldTimerManager().SetTimer(EffectTimer, this, &ALTTEffectStation::EndEffect, EffectDuration, false);
		}
	}
	StopStationLoops(this);
	if (USoundBase* Sound = bMissed ? MissSound.Get() : EffectSound.Get())
	{
		if (Sound->IsLooping())
		{
			// A looping sound (rain) keeps going until the effect ends or the take resets.
			if (UAudioComponent* Loop = UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent(), NAME_None,
				FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, 1.f, 1.f, 0.f, nullptr, nullptr, false))
			{
				Loop->ComponentTags.Add(TEXT("StationLoop"));
			}
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
		}
	}
	if (Game == ELTTStationGame::Choice && !bMissed)
	{
		// Also covers an act restarting with the gel already in.
		const int32 Index = Choices.IndexOfByPredicate([Cue](const FLTTStationChoice& C) { return C.CorrectFor == Cue; });
		ApplyChoice(Index);
	}
	ReceiveEffect(Cue, Grade);
}

void ALTTEffectStation::EndEffect()
{
	SetEffectActorsOn(false);
	StopStationLoops(this);
	ReceiveEffectEnded();
}

void ALTTEffectStation::ResetStation()
{
	GetWorldTimerManager().ClearTimer(EffectTimer);
	GetWorldTimerManager().ClearTimer(TossHideTimer);
	LeaveStation(false);
	bInFlight = false;
	TossProp->SetVisibility(false);
	SetActorTickEnabled(false);
	SetEffectActorsOn(false);
	StopStationLoops(this);
	if (bKeyLightColorSaved && IsValid(KeyLight) && KeyLight->GetLightComponent())
	{
		KeyLight->GetLightComponent()->SetLightColor(SavedKeyLightColor);
	}
	bHadWrongChoice = false;
	ReceiveEffectEnded();
}

// ---- HUD ----

FVector2D ALTTEffectStation::GetBand() const
{
	switch (Game)
	{
	case ELTTStationGame::Timing:
	{
		const float Half = 0.15f / FMath::Max(TimingTravelTime, 0.1f);
		return FVector2D(TimingTarget - Half, TimingTarget + Half);
	}
	case ELTTStationGame::HoldRelease:
		return HoldPerfectBand;
	case ELTTStationGame::Sustain:
		return SustainBand;
	case ELTTStationGame::Shake:
		return FVector2D(0.97f, 1.f);
	case ELTTStationGame::Toss:
		return FVector2D(TossIdeal - 0.07f, TossIdeal + 0.07f);
	default:
		return FVector2D(1.f, 0.f);
	}
}

float ALTTEffectStation::GetProgress() const
{
	return Game == ELTTStationGame::Sustain ? FMath::Clamp(InBandTime / FMath::Max(SustainTime, 0.1f), 0.f, 1.f) : -1.f;
}

FText ALTTEffectStation::GetInstruction() const
{
	switch (Game)
	{
	case ELTTStationGame::Press:
		return LOCTEXT("InsPress", "Click to clap the slate!");
	case ELTTStationGame::Timing:
		return LOCTEXT("InsTiming", "Click when the marker hits the green!");
	case ELTTStationGame::HoldRelease:
		return LOCTEXT("InsHold", "Hold the mouse button, release in the green. Don't overdo it!");
	case ELTTStationGame::Sustain:
		return LOCTEXT("InsSustain", "Hold and release the mouse button to keep the needle in the green");
	case ELTTStationGame::Shake:
		return bFlashPhase ? LOCTEXT("InsFlash", "FLASH! CLICK NOW!") : LOCTEXT("InsShake", "Shake it! Move the mouse left and right (or tap A / D)");
	case ELTTStationGame::Choice:
	{
		FString Options;
		for (int32 i = 0; i < Choices.Num(); ++i)
		{
			Options += FString::Printf(TEXT("%s[%d] %s%s   "), i == SelectedChoice ? TEXT(">> ") : TEXT(""), i + 1,
				*Choices[i].Name.ToString(), i == SelectedChoice ? TEXT(" <<") : TEXT(""));
		}
		return FText::Format(LOCTEXT("InsChoice", "{0}\nPick with 1-3 or the mouse wheel, click to slide it in"), FText::FromString(Options.TrimEnd()));
	}
	case ELTTStationGame::Toss:
		return bOnTarget ? LOCTEXT("InsTossOn", "ON TARGET - hold to charge, release in the green to throw!")
			: FText::Format(LOCTEXT("InsToss", "Aim at {0}, hold to charge, release in the green to throw"), FText::FromName(TossTargetName));
	default:
		return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
