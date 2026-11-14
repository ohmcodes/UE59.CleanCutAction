// Clean! Cut! Action!

#include "Interaction/LTTCoffeeMachineComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine primitives are 100 cm across / tall with the pivot in the middle.
	constexpr float PrimSize = 100.f;
}

ULTTCoffeeMachineComponent::ULTTCoffeeMachineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // ticks only while brewing
	IdlePrompt = NSLOCTEXT("LTT", "CoffeeMake", "Make coffee");
}

UStaticMeshComponent* ULTTCoffeeMachineComponent::MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Mesh || !Root)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner, Name);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetCanEverAffectNavigation(false);
	if (Material)
	{
		for (int32 i = 0; i < C->GetNumMaterials(); ++i)
		{
			C->SetMaterial(i, Material);
		}
	}
	C->SetupAttachment(Root);
	C->RegisterComponent();
	C->SetVisibility(false);
	return C;
}

void ULTTCoffeeMachineComponent::BeginPlay()
{
	Super::BeginPlay();
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Portafilter = MakePiece(TEXT("CoffeePortafilter"), PortafilterMesh, nullptr);
	if (Portafilter)
	{
		Portafilter->SetRelativeLocationAndRotation(GroupHead, PortafilterRotation);
	}
	Grounds = MakePiece(TEXT("CoffeeGrounds"), Cylinder, GroundsMaterial);
	Stream = MakePiece(TEXT("CoffeeStream"), Cylinder, StreamMaterial);
	Coffee = MakePiece(TEXT("CoffeeLevel"), Cylinder, CoffeeMaterial);

	Cup = MakePiece(TEXT("CoffeeMug"), CupMesh, nullptr);
	if (Cup && CupMesh)
	{
		// Sit the mug's bottom on the drip tray, centred on the spout.
		const FBoxSphereBounds B = CupMesh->GetBounds();
		const FVector Centre = CupRotation.RotateVector(B.Origin);
		const float Bottom = static_cast<float>(B.Origin.Z - B.BoxExtent.Z);
		Cup->SetRelativeLocationAndRotation(FVector(CupSpot.X - Centre.X, CupSpot.Y - Centre.Y, CupSpot.Z - Bottom), CupRotation);
		CupBottomZ = CupSpot.Z;
		const float Height = static_cast<float>(B.BoxExtent.Z * 2.f);
		CoffeeLowZ = CupBottomZ + Height * 0.15f;
		CoffeeHighZ = CupBottomZ + Height * 0.8f;
		CoffeeRadius = static_cast<float>(FMath::Min(B.BoxExtent.X, B.BoxExtent.Y)) * 0.72f;
		MeasureCup();
	}
}

void ULTTCoffeeMachineComponent::MeasureCup()
{
	// Find the cup's real inside (cups can sit on a saucer): trace straight down at growing distances from the
	// centre. The floor is the first hit at the centre, the rim the highest hit, the inner wall where hits jump up.
	if (!Cup)
	{
		return;
	}
	Cup->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Cup->SetCollisionResponseToAllChannels(ECR_Ignore);
	Cup->RecreatePhysicsState();
	const FTransform Machine = GetOwner()->GetActorTransform();
	const float TopZ = CupSpot.Z + 40.f;
	auto HitZ = [&](float R) -> float
	{
		const FVector Start = Machine.TransformPosition(FVector(CupSpot.X + R, CupSpot.Y, TopZ));
		const FVector End = Machine.TransformPosition(FVector(CupSpot.X + R, CupSpot.Y, CupSpot.Z - 2.f));
		FHitResult Hit;
		if (Cup->LineTraceComponent(Hit, Start, End, FCollisionQueryParams(SCENE_QUERY_STAT(LTTCoffeeCup), true)))
		{
			return static_cast<float>(Machine.InverseTransformPosition(Hit.ImpactPoint).Z);
		}
		return -1.f;
	};
	const float Floor = HitZ(0.f);
	float Rim = Floor;
	float Wall = -1.f;
	for (float R = 0.5f; R <= 12.f; R += 0.25f)
	{
		const float Z = HitZ(R);
		if (Z > Rim)
		{
			Rim = Z;
		}
		if (Wall < 0.f && Floor > 0.f && Z > Floor + 2.f)
		{
			Wall = R; // first point clearly above the floor = inside of the wall
		}
	}
	Cup->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Floor > 0.f && Wall > 0.f && Rim > Floor + 1.f)
	{
		CupBottomZ = Floor - 0.2f;
		CoffeeLowZ = Floor + 0.3f;
		CoffeeHighZ = Floor + (Rim - Floor) * 0.78f;
		CoffeeRadius = FMath::Max(Wall - 0.35f, 0.5f);
	}
}

void ULTTCoffeeMachineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLoop();
	Super::EndPlay(EndPlayReason);
}

void ULTTCoffeeMachineComponent::StopLoop()
{
	if (LoopAudio)
	{
		LoopAudio->Stop();
		LoopAudio->DestroyComponent();
		LoopAudio = nullptr;
	}
}

void ULTTCoffeeMachineComponent::SetGrounds(float Alpha)
{
	if (!Grounds)
	{
		return;
	}
	// A dark disc filling the basket from the bottom up.
	const float H = FMath::Max(BasketDepth * Alpha, 0.05f);
	Grounds->SetRelativeLocation(GroupHead - FVector(0.f, 0.f, BasketDepth - H * 0.5f));
	Grounds->SetRelativeScale3D(FVector(BasketRadius * 2.f / PrimSize, BasketRadius * 2.f / PrimSize, H / PrimSize));
	Grounds->SetVisibility(Alpha > 0.f);
}

void ULTTCoffeeMachineComponent::SetCoffeeLevel(float Alpha)
{
	if (!Coffee)
	{
		return;
	}
	const float Top = FMath::Lerp(CoffeeLowZ, CoffeeHighZ, Alpha);
	const float H = FMath::Max(Top - CupBottomZ, 0.1f);
	Coffee->SetRelativeLocation(FVector(CupSpot.X, CupSpot.Y, CupBottomZ + H * 0.5f));
	Coffee->SetRelativeScale3D(FVector(CoffeeRadius * 2.f / PrimSize, CoffeeRadius * 2.f / PrimSize, H / PrimSize));
	Coffee->SetVisibility(Alpha > 0.f);

	if (Stream)
	{
		// From the bottom of the basket down to the coffee surface.
		const float From = GroupHead.Z - BasketDepth;
		const float Len = FMath::Max(From - Top, 0.5f);
		Stream->SetRelativeLocation(FVector(CupSpot.X, CupSpot.Y, Top + Len * 0.5f));
		Stream->SetRelativeScale3D(FVector(StreamWidth / PrimSize, StreamWidth / PrimSize, Len / PrimSize));
	}
}

bool ULTTCoffeeMachineComponent::Use()
{
	if (IsBusy())
	{
		return false;
	}
	// Fresh brew: empty mug away, portafilter in, grinding.
	if (Cup) { Cup->SetVisibility(false); }
	SetCoffeeLevel(0.f);
	if (Stream) { Stream->SetVisibility(false); }
	if (Portafilter) { Portafilter->SetVisibility(true); }
	SetGrounds(0.f);
	StopLoop();
	LoopAudio = PlayOwnerSound(GrindSound, true);
	Phase = ELTTCoffeePhase::Grinding;
	PhaseTime = 0.f;
	SetComponentTickEnabled(true);
	return true;
}

void ULTTCoffeeMachineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PhaseTime += DeltaTime;

	switch (Phase)
	{
	case ELTTCoffeePhase::Grinding:
		SetGrounds(FMath::Clamp(PhaseTime / GrindTime, 0.f, 1.f));
		if (PhaseTime >= GrindTime)
		{
			StopLoop();
			Phase = ELTTCoffeePhase::MugIn;
			PhaseTime = 0.f;
			if (Cup)
			{
				Cup->SetVisibility(true);
			}
		}
		break;

	case ELTTCoffeePhase::MugIn:
	{
		// The mug pops in (slight overshoot), then the pour starts.
		constexpr float PopTime = 0.25f;
		const float A = FMath::Clamp(PhaseTime / PopTime, 0.f, 1.f);
		const float Pop = A < 1.f ? FMath::Sin(A * PI * 0.5f) * (1.f + 0.15f * FMath::Sin(A * PI)) : 1.f;
		if (Cup)
		{
			Cup->SetRelativeScale3D(FVector(FMath::Max(Pop, 0.01f)));
		}
		if (PhaseTime >= PopTime + 0.15f)
		{
			Phase = ELTTCoffeePhase::Filling;
			PhaseTime = 0.f;
			if (Stream)
			{
				Stream->SetVisibility(true);
			}
			SetCoffeeLevel(0.f);
			LoopAudio = PlayOwnerSound(FillSound, true);
		}
		break;
	}

	case ELTTCoffeePhase::Filling:
		SetCoffeeLevel(FMath::Clamp(PhaseTime / FillTime, 0.f, 1.f));
		if (PhaseTime >= FillTime)
		{
			StopLoop();
			if (Stream)
			{
				Stream->SetVisibility(false);
			}
			PlayOwnerSound(DoneSound, false);
			Phase = ELTTCoffeePhase::Idle;
			SetComponentTickEnabled(false);
		}
		break;

	default:
		SetComponentTickEnabled(false);
		break;
	}
}
