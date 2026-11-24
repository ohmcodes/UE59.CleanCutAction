// Clean! Cut! Action!

#include "Interaction/LTTCheatButton.h"
#include "Interaction/LTTMessDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LTTCheatButton"

ALTTCheatButton::ALTTCheatButton()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Engine cylinder is 100 x 100 cm, pivot in the middle.
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	SetRootComponent(Base);
	Base->SetStaticMesh(CylinderMesh.Object);
	Base->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.06f)); // 30 cm wide, 6 cm tall
	Base->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (ShapeMaterial.Object)
	{
		Base->SetMaterial(0, ShapeMaterial.Object);
	}

	Cap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cap"));
	Cap->SetupAttachment(Base);
	Cap->SetStaticMesh(CylinderMesh.Object);
	// Relative to the squashed base: 20 cm wide, 5 cm tall, sitting on top.
	Cap->SetRelativeScale3D(FVector(0.66f, 0.66f, 0.8f));
	Cap->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Cap->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (ShapeMaterial.Object)
	{
		Cap->SetMaterial(0, ShapeMaterial.Object);
	}

	StepTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StepTrigger"));
	StepTrigger->SetupAttachment(Base);
	StepTrigger->SetBoxExtent(FVector(60.f, 60.f, 600.f)); // scaled by the base: ~36 cm square, ~70 cm tall
	StepTrigger->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StepTrigger->SetHiddenInGame(true);
}

void ALTTCheatButton::BeginPlay()
{
	Super::BeginPlay();

#if UE_BUILD_SHIPPING
	if (!bAllowInShipping)
	{
		Destroy();
		return;
	}
#endif

	CapRestLocation = Cap->GetRelativeLocation();
	CapMaterial = Cap->CreateDynamicMaterialInstance(0);
	SetCapColor(ButtonColor);

	if (bTriggerOnStep)
	{
		StepTrigger->SetCollisionProfileName(FName(TEXT("OverlapOnlyPawn")));
		StepTrigger->SetGenerateOverlapEvents(true);
		StepTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALTTCheatButton::HandleStepOverlap);
	}
}

void ALTTCheatButton::SetCapColor(const FLinearColor& Color)
{
	if (CapMaterial)
	{
		CapMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

int32 ALTTCheatButton::Press()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < ReadyTime)
	{
		return -1;
	}
	ALTTMessDirector* Director = ALTTMessDirector::Get(World);
	if (!Director)
	{
		UE_LOG(LogTemp, Warning, TEXT("LTTCheatButton: no mess director in this world."));
		return -1;
	}
	ReadyTime = World->GetTimeSeconds() + Cooldown;

	const int32 Placed = Director->CheatPlaceAll(LeaveCount); // same as LTT.PlaceAll
	UE_LOG(LogTemp, Log, TEXT("LTTCheatButton: placed %d props."), Placed);

	// Push the cap down and flash green.
	Cap->SetRelativeLocation(CapRestLocation - FVector(0.f, 0.f, 40.f));
	SetCapColor(PressedColor);
	World->GetTimerManager().SetTimer(ReleaseTimer, this, &ALTTCheatButton::ReleaseCap, FMath::Max(Cooldown, 0.3f), false);

	ReceivePressed(Placed);
	return Placed;
}

void ALTTCheatButton::ReleaseCap()
{
	Cap->SetRelativeLocation(CapRestLocation);
	SetCapColor(ButtonColor);
}

void ALTTCheatButton::HandleStepOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		Press();
	}
}

bool ALTTCheatButton::CanInteract_Implementation(AActor* InstigatorActor) const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() >= ReadyTime;
}

FText ALTTCheatButton::GetPromptText_Implementation(AActor* InstigatorActor) const
{
	return FText::Format(LOCTEXT("Prompt", "Cheat: place all but {0}"), FText::AsNumber(LeaveCount));
}

void ALTTCheatButton::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	// Stencil 1 = focus outline, same as the props.
	Cap->SetRenderCustomDepth(true);
	Cap->SetCustomDepthStencilValue(1);
}

void ALTTCheatButton::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	Cap->SetRenderCustomDepth(false);
}

void ALTTCheatButton::OnInteract_Implementation(AActor* InstigatorActor)
{
	Press();
}

#undef LOCTEXT_NAMESPACE
