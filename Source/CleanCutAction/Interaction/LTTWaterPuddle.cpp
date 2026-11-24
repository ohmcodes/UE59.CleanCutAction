// Clean! Cut! Action!

#include "Interaction/LTTWaterPuddle.h"
#include "VR/LTTVRComponent.h"
#include "Interaction/LTTMessDirector.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LTTWaterPuddle"

namespace
{
	/** The engine Plane is 100 x 100 cm. */
	constexpr float PlaneSize = 100.f;
	/** The water in M_WaterPuddle reaches about this far across the plane (the rest is the soft, wobbly edge). */
	constexpr float PlaneFill = 0.9f;
	/** Aim this far outside the water still counts (cm). */
	constexpr float AimMargin = 12.f;

	/** Walk speeds of the characters standing in water. Several puddles can overlap, so they're counted. */
	struct FLTTSlowState
	{
		int32 Count = 0;
		float Walk = 0.f;
		float Crouched = 0.f;
	};
	TMap<TWeakObjectPtr<ACharacter>, FLTTSlowState> GSlowedCharacters;

	void AddSlow(ACharacter* Character, float Factor)
	{
		UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
		if (!Move)
		{
			return;
		}
		for (auto It = GSlowedCharacters.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent(); // left over from an earlier play session
			}
		}
		FLTTSlowState& State = GSlowedCharacters.FindOrAdd(Character);
		if (State.Count++ == 0)
		{
			State.Walk = Move->MaxWalkSpeed;
			State.Crouched = Move->MaxWalkSpeedCrouched;
			Move->MaxWalkSpeed = State.Walk * Factor;
			Move->MaxWalkSpeedCrouched = State.Crouched * Factor;
		}
	}

	void RemoveSlow(ACharacter* Character)
	{
		FLTTSlowState* State = GSlowedCharacters.Find(Character);
		if (!State || --State->Count > 0)
		{
			return;
		}
		if (UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr)
		{
			Move->MaxWalkSpeed = State->Walk;
			Move->MaxWalkSpeedCrouched = State->Crouched;
		}
		GSlowedCharacters.Remove(Character);
	}
}

ALTTWaterPuddle::ALTTWaterPuddle()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	Water->SetupAttachment(RootComponent);
	Water->SetRelativeLocation(FVector(0.f, 0.f, 0.6f)); // just above the floor, no z-fighting
	Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Water->SetCastShadow(false);
	Water->SetCanEverAffectNavigation(false);

	Splash = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Splash"));
	Splash->SetupAttachment(RootComponent);
	Splash->SetAutoActivate(false);

	// All of these are also in +MapsToCook (Config/DefaultGame.ini): C++ defaults aren't seen by the cooker.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMat(TEXT("/Game/_CleanCutAction/Materials/M_WaterPuddle.M_WaterPuddle"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SplashFX(TEXT("/Game/_CleanCutAction/VFX/NS_MopSplash.NS_MopSplash"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Swish(TEXT("/Game/_CleanCutAction/SFX/SW_MopSwish.SW_MopSwish"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Step(TEXT("/Game/_CleanCutAction/SFX/SW_PuddleSplash.SW_PuddleSplash"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Clean(TEXT("/Game/_CleanCutAction/SFX/SW_PuddleClean.SW_PuddleClean"));
	Water->SetStaticMesh(Plane.Object);
	if (WaterMat.Object)
	{
		Water->SetMaterial(0, WaterMat.Object);
	}
	Splash->SetAsset(SplashFX.Object);
	MopSound = Swish.Object;
	SplashSound = Step.Object;
	CleanSound = Clean.Object;

	CurrentRadius = RadiusFrom = RadiusTo = Radius;
}

void ALTTWaterPuddle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyRadius(Radius);
}

void ALTTWaterPuddle::BeginPlay()
{
	Super::BeginPlay();
	ApplyRadius(Radius);
	WaterMID = Water->CreateDynamicMaterialInstance(0);

	if (bCountsAsCleanupTask)
	{
		if (ALTTMessDirector* Director = ALTTMessDirector::Get(this))
		{
			Director->RegisterExtraTask();
			bRegisteredTask = true;
		}
	}
}

void ALTTWaterPuddle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<ACharacter>& Character : Slowed)
	{
		if (Character.IsValid())
		{
			RemoveSlow(Character.Get());
		}
	}
	Slowed.Reset();

	// Removed some other way (not mopped): it's no longer something to clean.
	if (EndPlayReason == EEndPlayReason::Destroyed && bRegisteredTask && !bCleaned)
	{
		if (ALTTMessDirector* Director = ALTTMessDirector::Get(this))
		{
			Director->RemoveExtraTask();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ALTTWaterPuddle::ApplyRadius(float NewRadius)
{
	CurrentRadius = FMath::Max(NewRadius, 0.f);
	if (Water)
	{
		const float Scale = FMath::Max(CurrentRadius * 2.f / (PlaneSize * PlaneFill), 0.001f);
		Water->SetRelativeScale3D(FVector(Scale, Scale, 1.f));
	}
}

bool ALTTWaterPuddle::IsInWater(const ACharacter* Character) const
{
	if (!Character || bCleaned)
	{
		return false;
	}
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	const FVector Local = GetActorTransform().InverseTransformPosition(Feet);
	return Local.SizeSquared2D() <= FMath::Square(CurrentRadius) && FMath::Abs(Local.Z) * GetActorScale3D().Z < 45.f;
}

void ALTTWaterPuddle::UpdateSlow(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	StepTimer -= DeltaSeconds;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			continue;
		}
		const bool bWasIn = Slowed.Contains(Character);
		const bool bIn = IsInWater(Character);
		const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight() - 3.f);
		if (bIn && !bWasIn)
		{
			Slowed.Add(Character);
			AddSlow(Character, SlowFactor);
			FireSplash(Feet);
			PlayPuddleSound(SplashSound, Feet, 1.f, FMath::FRandRange(0.92f, 1.08f));
			StepTimer = StepInterval;
		}
		else if (!bIn && bWasIn)
		{
			Slowed.Remove(Character);
			RemoveSlow(Character);
		}
		else if (bIn && StepTimer <= 0.f)
		{
			// Sloshing through it.
			const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
			if (Move && Move->IsMovingOnGround() && Character->GetVelocity().SizeSquared2D() > FMath::Square(40.f))
			{
				FireSplash(Feet);
				PlayPuddleSound(SplashSound, Feet, 0.55f, FMath::FRandRange(1.05f, 1.3f));
				StepTimer = StepInterval;
			}
		}
	}
}

void ALTTWaterPuddle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (StrokeElapsed >= 0.f)
	{
		StrokeElapsed += DeltaSeconds;
		const float A = FMath::Clamp(StrokeElapsed / FMath::Max(StrokeTime, 0.05f), 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - A, 3.f); // OutCubic
		ApplyRadius(FMath::Lerp(RadiusFrom, RadiusTo, Ease));
		if (A >= 1.f)
		{
			StrokeElapsed = -1.f;
			if (bCleaned)
			{
				FinishCleaning();
			}
		}
	}

	UpdateSlow(DeltaSeconds);
}

bool ALTTWaterPuddle::MopUp(AActor* Mop, FVector At)
{
	if (bCleaned || StrokeElapsed >= 0.f)
	{
		return false;
	}
	++StrokesDone;
	const int32 Needed = FMath::Max(MopsNeeded, 1);
	RadiusFrom = CurrentRadius;
	RadiusTo = StrokesDone >= Needed ? 0.f : Radius * (1.f - static_cast<float>(StrokesDone) / Needed);
	StrokeElapsed = 0.f;

	FireSplash(At);
	PlayPuddleSound(MopSound, At, 1.f, FMath::FRandRange(0.92f, 1.1f));
	ReceiveMopped(GetStrokesLeft());

	if (StrokesDone >= Needed)
	{
		MarkCleaned(); // counted right away; the water is gone when this stroke ends
	}
	return true;
}

void ALTTWaterPuddle::MarkCleaned()
{
	if (bCleaned)
	{
		return;
	}
	bCleaned = true;
	if (bRegisteredTask)
	{
		if (ALTTMessDirector* Director = ALTTMessDirector::Get(this))
		{
			Director->NotifyExtraTaskDone();
		}
	}
}

void ALTTWaterPuddle::FinishCleaning()
{
	Water->SetVisibility(false);
	PlayPuddleSound(CleanSound, GetActorLocation(), 0.8f);
	ReceiveCleaned();
	SetLifeSpan(1.5f); // lets the last splash finish
}

void ALTTWaterPuddle::CleanInstantly()
{
	if (!bCleaned)
	{
		MarkCleaned();
		Destroy();
	}
}

void ALTTWaterPuddle::FireSplash(const FVector& At)
{
	if (Splash && Splash->GetAsset())
	{
		Splash->SetWorldLocation(At + GetActorUpVector() * 2.f);
		Splash->Activate(true);
	}
}

void ALTTWaterPuddle::PlayPuddleSound(USoundBase* Sound, const FVector& At, float VolumeScale, float Pitch)
{
	if (!Sound)
	{
		return;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(this);
	Audio->SetupAttachment(RootComponent);
	Audio->SetSound(Sound);
	Audio->SetVolumeMultiplier(Volume * VolumeScale);
	Audio->SetPitchMultiplier(Pitch);
	Audio->bAutoDestroy = true;
	Audio->bAllowSpatialization = true;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bAttenuate = true;
	Audio->AttenuationOverrides.bSpatialize = true;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
	Audio->AttenuationOverrides.FalloffDistance = SoundFalloff;
	Audio->RegisterComponent();
	Audio->SetWorldLocation(At);
	Audio->Play();
}

ALTTWaterPuddle* ALTTWaterPuddle::FindAimed(const UObject* WorldContext, const FVector& ViewLocation, const FVector& ViewDirection,
	float MaxDistance, FVector& OutPoint, const AActor* IgnoreActor)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	ALTTWaterPuddle* Best = nullptr;
	double BestDist = MaxDistance;
	for (TActorIterator<ALTTWaterPuddle> It(World); It; ++It)
	{
		ALTTWaterPuddle* Puddle = *It;
		if (Puddle->bCleaned || Puddle->IsActorBeingDestroyed())
		{
			continue;
		}
		// Where the view ray meets the water plane (local z = 0), looking down at it from above.
		const FTransform& T = Puddle->GetActorTransform();
		const FVector O = T.InverseTransformPosition(ViewLocation);
		const FVector D = T.InverseTransformVector(ViewDirection);
		if (O.Z <= 0.0 || D.Z >= -UE_KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const FVector Local = O + D * (-O.Z / D.Z);
		if (Local.SizeSquared2D() > FMath::Square(Puddle->CurrentRadius + AimMargin))
		{
			continue;
		}
		const FVector Point = T.TransformPosition(Local);
		const double Dist = FVector::Dist(Point, ViewLocation);
		if (Dist > BestDist)
		{
			continue;
		}
		// Nothing solid in the way (a table over the puddle, a wall). The floor itself is hit at about Dist.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTPuddleAim), false, IgnoreActor);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, ViewLocation, Point, ECC_Visibility, Params) && Hit.Distance < Dist - 15.0)
		{
			continue;
		}
		BestDist = Dist;
		Best = Puddle;
		OutPoint = Point;
	}
	return Best;
}

FText ALTTWaterPuddle::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	return LOCTEXT("NeedsMop", "Needs a mop");
}

void ALTTWaterPuddle::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	if (WaterMID)
	{
		WaterMID->SetScalarParameterValue(TEXT("Highlight"), 1.f);
	}
}

void ALTTWaterPuddle::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	if (WaterMID)
	{
		WaterMID->SetScalarParameterValue(TEXT("Highlight"), 0.f);
	}
}

void ALTTWaterPuddle::OnInteract_Implementation(AActor* InstigatorActor)
{
	// Bare hands: just a little splash. It takes the mop to clean it.
	const UWorld* World = GetWorld();
	if (bCleaned || !World || World->GetTimeSeconds() < PokeReadyTime)
	{
		return;
	}
	PokeReadyTime = World->GetTimeSeconds() + 0.5f;
	FVector At = GetActorLocation();
	if (const APawn* Pawn = Cast<APawn>(InstigatorActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			FVector ViewLoc;
			FRotator ViewRot;
			ULTTVRComponent::GetAimViewPoint(Controller, ViewLoc, ViewRot); // VR: the hand's ray
			FVector Point;
			if (FindAimed(this, ViewLoc, ViewRot.Vector(), 1000.f, Point, InstigatorActor) == this)
			{
				At = Point;
			}
		}
	}
	FireSplash(At);
	PlayPuddleSound(SplashSound, At, 0.6f, 1.25f);
}

#undef LOCTEXT_NAMESPACE
