// Clean! Cut! Action!

#include "Shoot/LTTFilmActor.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

ALTTActorMark::ALTTActorMark()
{
	PrimaryActorTick.bCanEverTick = false;
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	SetRootComponent(Arrow);
	Arrow->ArrowColor = FColor(255, 200, 40);
	Arrow->ArrowSize = 1.5f;
}

ALTTFilmActor::ALTTFilmActor()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	bUseControllerRotationYaw = false;

	CatchPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CatchPoint"));
	CatchPoint->SetupAttachment(GetCapsuleComponent());
	CatchPoint->SetRelativeLocation(FVector(35.f, 0.f, 35.f));
}

FVector ALTTFilmActor::GetCatchLocation() const
{
	return CatchPoint ? CatchPoint->GetComponentLocation() : GetActorLocation();
}

void ALTTFilmActor::MoveToMark(ALTTActorMark* Mark, bool bRun)
{
	if (!Mark)
	{
		return;
	}
	TargetMark = Mark;
	bFacingMark = false;
	MoveElapsed = 0.f;
	GetCharacterMovement()->MaxWalkSpeed = bRun ? RunSpeed : WalkSpeed;

	if (FVector::DistSquared2D(GetActorLocation(), Mark->GetActorLocation()) < FMath::Square(30.f))
	{
		FinishMove();
		return;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	const bool bStarted = AI && AI->MoveToLocation(Mark->GetActorLocation(), 20.f, false) != EPathFollowingRequestResult::Failed;
	if (!bStarted)
	{
		// No controller or no path (no NavMesh on this floor): just appear there.
		TeleportToMark(Mark);
		return;
	}
	bMovingToMark = true;
}

void ALTTFilmActor::TeleportToMark(ALTTActorMark* Mark)
{
	if (!Mark)
	{
		return;
	}
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	bMovingToMark = false;
	bFacingMark = false;
	TargetMark = Mark;
	// Marks sit on the floor; the capsule centre is half a capsule above it.
	const FVector Location = Mark->GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f);
	SetActorLocationAndRotation(Location, FRotator(0.f, Mark->GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void ALTTFilmActor::FinishMove()
{
	bMovingToMark = false;
	bFacingMark = TargetMark.IsValid();
}

void ALTTFilmActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// No Anim Blueprint needed: with the mesh in "Use Animation Asset" mode playing a 1D speed blend space
	// (idle at 0, walk, run), feed it the ground speed.
	if (UAnimSingleNodeInstance* Single = GetMesh() ? GetMesh()->GetSingleNodeInstance() : nullptr)
	{
		if (Cast<UBlendSpace>(Single->GetAnimationAsset()))
		{
			Single->SetBlendSpacePosition(FVector(GetVelocity().Size2D(), 0.f, 0.f));
		}
	}

	ALTTActorMark* Mark = TargetMark.Get();
	if (bMovingToMark)
	{
		MoveElapsed += DeltaSeconds;
		const AAIController* AI = Cast<AAIController>(GetController());
		const bool bArrived = Mark && FVector::DistSquared2D(GetActorLocation(), Mark->GetActorLocation()) < FMath::Square(45.f);
		const bool bStopped = MoveElapsed > 0.5f && (!AI || AI->GetMoveStatus() == EPathFollowingStatus::Idle);
		if (!Mark || bArrived)
		{
			FinishMove();
		}
		else if (bStopped || MoveElapsed > MoveTimeout)
		{
			TeleportToMark(Mark); // stuck or the path gave out
		}
	}
	else if (bFacingMark && Mark)
	{
		const FRotator Target(0.f, Mark->GetActorRotation().Yaw, 0.f);
		const FRotator New = FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, 8.f);
		SetActorRotation(New);
		if (FMath::Abs(FRotator::NormalizeAxis(New.Yaw - Target.Yaw)) < 1.f)
		{
			bFacingMark = false;
		}
	}
}

void ALTTFilmActor::Speak(const FLTTDialogueLine& Line)
{
	if (Line.Montage.IsNone() || !PlayNamedMontage(Line.Montage))
	{
		if (TalkMontages.Num() > 0)
		{
			if (UAnimMontage* Talk = TalkMontages[FMath::RandRange(0, TalkMontages.Num() - 1)])
			{
				PlayAnimMontage(Talk);
			}
		}
	}
	ReceiveLineSpoken(Line);
}

bool ALTTFilmActor::PlayNamedMontage(FName Name)
{
	const TObjectPtr<UAnimMontage>* Found = Montages.Find(Name);
	if (Found && *Found)
	{
		PlayAnimMontage(*Found);
		return true;
	}
	return false;
}

void ALTTFilmActor::SetFrozen(bool bFrozen)
{
	CustomTimeDilation = bFrozen ? 0.f : 1.f;
}
