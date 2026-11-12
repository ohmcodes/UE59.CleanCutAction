// Clean! Cut! Action!

#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTInteractable.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

ULTTCarryComponent::ULTTCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Run after the camera has taken this frame's control rotation.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void ULTTCarryComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveHoldParent();
}

USceneComponent* ULTTCarryComponent::ResolveHoldParent()
{
	if (!HoldParent && GetOwner())
	{
		HoldParent = GetOwner()->FindComponentByClass<UCameraComponent>();
	}
	return HoldParent;
}

bool ULTTCarryComponent::PickUp(ALTTInteractableBase* Item)
{
	if (!IsValid(Item) || HeldItem || !ILTTInteractable::Execute_CanInteract(Item, GetOwner()))
	{
		return false;
	}

	USceneComponent* Parent = ResolveHoldParent();
	if (!Parent)
	{
		return false;
	}

	Item->BeginHold(GetOwner());

	HeldRelativeYaw = FRotator::NormalizeAxis(Item->GetActorRotation().Yaw - Parent->GetComponentRotation().Yaw);

	Item->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);
	BlendStartLocation = Item->GetRootComponent()->GetRelativeLocation();
	BlendStartRotation = Item->GetRootComponent()->GetRelativeRotation().Quaternion();
	BlendElapsed = 0.f;

	HeldItem = Item;
	OnPickedUp.Broadcast(Item);
	return true;
}

void ULTTCarryComponent::Drop()
{
	Release(false);
}

void ULTTCarryComponent::Throw()
{
	Release(true);
}

bool ULTTCarryComponent::PlaceHeld()
{
	ALTTInteractableBase* Item = HeldItem;
	if (!IsValid(Item) || !Item->HasHome() || Item->IsPlaced())
	{
		return false;
	}
	Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeldItem = nullptr;
	Item->PlaceAtHome();
	OnReleased.Broadcast(Item, false);
	return true;
}

void ULTTCarryComponent::Release(bool bThrow)
{
	ALTTInteractableBase* Item = HeldItem;
	if (!IsValid(Item) || !HoldParent)
	{
		HeldItem = nullptr;
		return;
	}

	const FVector CamLoc = HoldParent->GetComponentLocation();
	const FVector Fwd = HoldParent->GetForwardVector();
	const FVector Up = HoldParent->GetUpVector();
	const float Radius = Item->GetPropRadius();

	// The held prop has no collision, so it may be inside a wall. Sweep from the eye to its centre and pull it back.
	const FVector Center = Item->Mesh->Bounds.Origin;
	const FVector ToCenter = Center - CamLoc;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTCarryRelease), false, GetOwner());
	Params.AddIgnoredActor(Item);
	FHitResult Hit;
	const float SweepRadius = FMath::Clamp(Radius * 0.5f, 3.f, 25.f);
	if (GetWorld()->SweepSingleByChannel(Hit, CamLoc, Center, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(SweepRadius), Params))
	{
		const FVector SafeCenter = Hit.Location - ToCenter.GetSafeNormal() * FMath::Max(Radius - SweepRadius, 0.f) * 0.5f;
		Item->AddActorWorldOffset(SafeCenter - Center);
	}

	Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	const FVector OwnerVelocity = GetOwner() ? GetOwner()->GetVelocity() : FVector::ZeroVector;
	FVector LinearVelocity;
	FVector AngularVelocity = FVector::ZeroVector;
	if (bThrow)
	{
		const FVector Dir = (Fwd + Up * FMath::Tan(FMath::DegreesToRadians(ThrowUpAngle))).GetSafeNormal();
		LinearVelocity = OwnerVelocity + Dir * ThrowSpeed * Item->GetEffectiveThrowMultiplier();
		AngularVelocity = FMath::VRand() * 240.f;
	}
	else
	{
		LinearVelocity = OwnerVelocity + Fwd * DropForwardSpeed;
	}

	Item->bLastReleaseWasThrow = bThrow;
	Item->EndHold(LinearVelocity, AngularVelocity);

	HeldItem = nullptr;
	OnReleased.Broadcast(Item, bThrow);
}

void ULTTCarryComponent::ComputeHoldTarget(FVector& OutRelLocation, FRotator& OutRelRotation) const
{
	const float Radius = HeldItem->GetPropRadius();
	const bool bTwoHands = HeldItem->UsesTwoHands();

	const float Dist = HeldItem->HoldDistance > 0.f ? HeldItem->HoldDistance : FMath::Max(BaseHoldDistance, Radius * 1.1f + 18.f);
	const FVector CenterRel = FVector(Dist, bTwoHands ? 0.f : OneHandSideOffset, -(HoldDownOffset + Radius * 0.5f)) + HeldItem->HoldOffset;

	const FQuat RelQuat = FRotator(0.f, HeldRelativeYaw, 0.f).Quaternion() * HeldItem->HoldRotationOffset.Quaternion();

	// Hold by the bounds centre, not the pivot (most props pivot at their base).
	FVector LocalCenter = FVector::ZeroVector;
	if (const UStaticMesh* SM = HeldItem->Mesh->GetStaticMesh())
	{
		LocalCenter = SM->GetBounds().Origin * HeldItem->Mesh->GetComponentScale();
	}

	OutRelLocation = CenterRel - RelQuat.RotateVector(LocalCenter);
	OutRelRotation = RelQuat.Rotator();
}

void ULTTCarryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (HeldItem && !IsValid(HeldItem))
	{
		HeldItem = nullptr;
	}

	if (HeldItem)
	{
		FVector TargetLoc;
		FRotator TargetRot;
		ComputeHoldTarget(TargetLoc, TargetRot);

		BlendElapsed += DeltaTime;
		const float A = PickupBlendTime > 0.f ? FMath::Clamp(BlendElapsed / PickupBlendTime, 0.f, 1.f) : 1.f;
		const float Ease = 1.f - FMath::Pow(1.f - A, 3.f); // OutCubic

		USceneComponent* Root = HeldItem->GetRootComponent();
		Root->SetRelativeLocationAndRotation(
			FMath::Lerp(BlendStartLocation, TargetLoc, Ease),
			FQuat::Slerp(BlendStartRotation, TargetRot.Quaternion(), Ease));
	}

	UpdateHandIK(DeltaTime);
}

void ULTTCarryComponent::UpdateHandIK(float DeltaTime)
{
	const float Speed = 1.f / FMath::Max(HandIKBlendTime, 0.01f);
	const bool bReady = HeldItem && BlendElapsed >= PickupBlendTime * 0.5f;
	const float TargetR = bReady ? 1.f : 0.f;
	const float TargetL = (bReady && HeldItem->UsesTwoHands()) ? 1.f : 0.f;
	RightHand.Alpha = FMath::FInterpConstantTo(RightHand.Alpha, TargetR, DeltaTime, Speed);
	LeftHand.Alpha = FMath::FInterpConstantTo(LeftHand.Alpha, TargetL, DeltaTime, Speed);

	if (!HeldItem || !HoldParent)
	{
		return; // keep last targets while blending out
	}

	const FTransform Cam = HoldParent->GetComponentTransform();
	const FVector Fwd = Cam.GetUnitAxis(EAxis::X);
	const FVector Right = Cam.GetUnitAxis(EAxis::Y);
	const FVector Up = Cam.GetUnitAxis(EAxis::Z);

	const FBoxSphereBounds& B = HeldItem->Mesh->Bounds;
	const FVector E = B.BoxExtent;
	const float ExtRight = FMath::Abs(Right.X) * E.X + FMath::Abs(Right.Y) * E.Y + FMath::Abs(Right.Z) * E.Z;

	const FVector Base = B.Origin + Fwd * GripOffset.X + Up * GripOffset.Z;
	RightHand.Location = Base + Right * (ExtRight + GripOffset.Y);
	LeftHand.Location = Base - Right * (ExtRight + GripOffset.Y);

	RightHand.JointTarget = Cam.TransformPosition(ElbowPoleOffset);
	LeftHand.JointTarget = Cam.TransformPosition(FVector(ElbowPoleOffset.X, -ElbowPoleOffset.Y, ElbowPoleOffset.Z));

	RightHand.Rotation = Cam.Rotator();
	LeftHand.Rotation = Cam.Rotator();
}
