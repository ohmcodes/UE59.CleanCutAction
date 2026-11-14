// Clean! Cut! Action!

#include "Shoot/LTTFilmCamera.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALTTFilmCamera::ALTTFilmCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only while travelling between acts

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);

	ConeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConeMesh"));
	ConeMesh->SetupAttachment(Root);
	ConeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConeMesh->SetCastShadow(false);
	ConeMesh->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeAsset.Succeeded())
	{
		ConeMesh->SetStaticMesh(ConeAsset.Object);
	}

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->ArrowSize = 2.f;

	// Already cooked for Phase 1 (+MapsToCook in Config/DefaultGame.ini).
	ConeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/_CleanCutAction/Materials/M_GhostSilhouette.M_GhostSilhouette")));
}

ALTTFilmCamera* ALTTFilmCamera::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (World)
	{
		for (TActorIterator<ALTTFilmCamera> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ALTTFilmCamera::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateConeShape();
}

void ALTTFilmCamera::UpdateConeShape()
{
	if (!ConeMesh)
	{
		return;
	}
	// The engine cone is 100 tall along Z with its tip up and a base of radius 50. Pitched 90 degrees its tip points
	// back at the camera, its local X is world up and its local Y is world right.
	const float TanH = FMath::Tan(FMath::DegreesToRadians(HorizontalFOV * 0.5f));
	const float TanV = TanH / FMath::Max(AspectRatio, 0.1f);
	ConeMesh->SetRelativeLocation(FVector(MaxDist * 0.5f, 0.f, 0.f));
	ConeMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	ConeMesh->SetRelativeScale3D(FVector(MaxDist * TanV / 50.f, MaxDist * TanH / 50.f, MaxDist / 100.f));
}

void ALTTFilmCamera::BeginPlay()
{
	Super::BeginPlay();
	UpdateConeShape();
	if (UMaterialInterface* Base = ConeMaterial.LoadSynchronous())
	{
		ConeMID = UMaterialInstanceDynamic::Create(Base, this);
		for (int32 i = 0; i < ConeMesh->GetNumMaterials(); ++i)
		{
			ConeMesh->SetMaterial(i, ConeMID);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: cone material %s didn't load; the frame cone is hidden."), *GetName(), *ConeMaterial.ToString());
		bShowCone = false;
	}
	SetConeLook(ELTTConeLook::Hidden);
}

void ALTTFilmCamera::SetConeLook(ELTTConeLook Look)
{
	if (!ConeMesh)
	{
		return;
	}
	ConeMesh->SetVisibility(bShowCone && Look != ELTTConeLook::Hidden);
	if (ConeMID)
	{
		ConeMID->SetVectorParameterValue(TEXT("Color"),
			Look == ELTTConeLook::Alert ? AlertColor : Look == ELTTConeLook::Allowed ? AllowedColor : IdleColor);
	}
}

bool ALTTFilmCamera::IsPointInFrame(const FVector& Point, const AActor* IgnoreActor) const
{
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Point);
	if (Local.X < NearDist || Local.X > MaxDist)
	{
		return false;
	}
	const float HalfH = FMath::DegreesToRadians(HorizontalFOV * 0.5f);
	const float HalfV = FMath::Atan(FMath::Tan(HalfH) / FMath::Max(AspectRatio, 0.1f));
	if (FMath::Abs(FMath::Atan2(Local.Y, Local.X)) > HalfH || FMath::Abs(FMath::Atan2(Local.Z, Local.X)) > HalfV)
	{
		return false;
	}
	if (bCheckOcclusion && GetWorld())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTFrameCheck), false, this);
		if (IgnoreActor)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Point, OcclusionChannel, Params))
		{
			// Only solid set pieces hide the player; props and other characters don't.
			const AActor* Blocker = Hit.GetActor();
			if (!Blocker || !Blocker->IsRootComponentMovable())
			{
				return false;
			}
		}
	}
	return true;
}

bool ALTTFilmCamera::IsPawnInFrame(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}
	return IsPointInFrame(Pawn->GetPawnViewLocation(), Pawn) || IsPointInFrame(Pawn->GetActorLocation(), Pawn);
}

float ALTTFilmCamera::MoveToAct(FName Act, bool bInstant)
{
	const TObjectPtr<AActor>* Anchor = ActAnchors.Find(Act);
	if (!Anchor || !IsValid(*Anchor))
	{
		return 0.f; // no anchor for this act: the camera stays where it is
	}
	MoveTarget = FTransform((*Anchor)->GetActorQuat(), (*Anchor)->GetActorLocation());
	if (bInstant || MoveTime <= 0.f || MoveTarget.Equals(FTransform(GetActorQuat(), GetActorLocation()), 1.f))
	{
		SetActorLocationAndRotation(MoveTarget.GetLocation(), MoveTarget.GetRotation());
		bMoving = false;
		SetActorTickEnabled(false);
		return 0.f;
	}
	MoveFrom = FTransform(GetActorQuat(), GetActorLocation());
	MoveElapsed = 0.f;
	bMoving = true;
	SetActorTickEnabled(true);
	return MoveTime;
}

void ALTTFilmCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bMoving)
	{
		SetActorTickEnabled(false);
		return;
	}
	MoveElapsed += DeltaSeconds;
	const float A = FMath::Clamp(MoveElapsed / FMath::Max(MoveTime, 0.01f), 0.f, 1.f);
	const float Ease = 0.5f - 0.5f * FMath::Cos(A * PI); // InOutSine
	SetActorLocationAndRotation(
		FMath::Lerp(MoveFrom.GetLocation(), MoveTarget.GetLocation(), Ease),
		FQuat::Slerp(MoveFrom.GetRotation(), MoveTarget.GetRotation(), Ease));
	if (A >= 1.f)
	{
		bMoving = false;
		SetActorTickEnabled(false);
	}
}
