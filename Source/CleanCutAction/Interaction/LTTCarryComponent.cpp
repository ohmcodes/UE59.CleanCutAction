// Clean! Cut! Action!

#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTInteractable.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
	// ---- Held tool (the mop): gripped on its handle at the lower right, head out in front ----
	// One local player, so the aim lives here rather than on the component.
	bool GToolAimSet = false;
	FVector GToolAimPoint = FVector::ZeroVector;
	/** Smoothed direction from the grip to the mop head, camera space. */
	FVector GToolDir = FVector::ZeroVector;
	/** Smoothed position of the mop head (its pivot), camera space. */
	FVector GToolHead = FVector::ZeroVector;
	/** While mopping the head sits on the floor at most this far in front of the player (cm). */
	constexpr float ToolFloorReach = 150.f;

	/** Where the hand holds the handle, camera space (X fwd, Y right, Z up). */
	const FVector ToolGripOffset(30.f, 19.f, -34.f);
	/** How far up the handle the hand is, from the mop's pivot (the bottom of its head), cm. */
	constexpr float ToolGripHeight = 92.f;
	/** Carried, not mopping: the head points this way from the grip (down and slightly towards the middle). */
	const FRotator ToolIdleAim(-15.f, -8.f, 0.f);
	constexpr float ToolAimSpeed = 10.f;

	/** SwingT: 0..1 during a mop stroke, < 0 otherwise. */
	void ComputeToolTarget(const USceneComponent* Parent, float DeltaTime, float SwingT, FVector& OutRelLocation, FRotator& OutRelRotation)
	{
		const FTransform& Cam = Parent->GetComponentTransform();

		// Carried: head out in front of the grip.
		FVector WantDir = ToolIdleAim.Vector();
		FVector WantHead = ToolGripOffset + WantDir * ToolGripHeight;
		FVector ScrubAxis = WantDir;
		if (GToolAimSet)
		{
			// Mopping: the head rests on the water, on the way from the player to the aimed spot (no further out
			// than ToolFloorReach), and the handle runs from there up to the hand, at about 45 degrees.
			const FVector Grip = Cam.TransformPosition(ToolGripOffset);
			const FVector Base(Grip.X, Grip.Y, GToolAimPoint.Z);
			const FVector Along = GToolAimPoint - Base;
			const float Dist = static_cast<float>(Along.Size());
			if (Dist > 5.f)
			{
				const FVector Flat = Along / Dist;
				const FVector Head = Base + Flat * FMath::Min(Dist, ToolFloorReach);
				WantHead = Cam.InverseTransformPosition(Head);
				WantDir = Cam.InverseTransformVector(Head - Grip).GetSafeNormal();
				ScrubAxis = Cam.InverseTransformVector(Flat);
			}
		}
		if (GToolDir.IsNearlyZero())
		{
			GToolDir = WantDir;
			GToolHead = WantHead;
		}
		else
		{
			GToolDir = FMath::VInterpTo(GToolDir, WantDir, DeltaTime, ToolAimSpeed).GetSafeNormal();
			GToolHead = FMath::VInterpTo(GToolHead, WantHead, DeltaTime, ToolAimSpeed);
		}

		// A stroke: push and pull across the floor (along the handle when just carried), with a small sideways sweep.
		FVector Head = GToolHead;
		if (SwingT >= 0.f)
		{
			Head += ScrubAxis * (16.f * FMath::Sin(2.f * PI * SwingT)) + FVector(0.f, 6.f * FMath::Sin(PI * SwingT), 0.f);
		}

		// The mop's +Z runs from its head up the handle; its head is widest along X (kept level, across the view).
		OutRelRotation = FRotationMatrix::MakeFromZX(-GToolDir, FVector::RightVector).Rotator();
		OutRelLocation = Head;
	}
}

void ULTTCarryComponent::SetToolAim(const FVector& WorldPoint)
{
	GToolAimSet = true;
	GToolAimPoint = WorldPoint;
}

void ULTTCarryComponent::ClearToolAim()
{
	GToolAimSet = false;
}

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
	SwingElapsed = -1.f;
	GToolDir = FVector::ZeroVector;

	HeldItem = Item;
	OnPickedUp.Broadcast(Item);
	return true;
}

void ULTTCarryComponent::PlayUseSwing(float Duration)
{
	if (HeldItem)
	{
		SwingDuration = FMath::Max(Duration, 0.05f);
		SwingElapsed = 0.f;
	}
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
	if (!IsValid(Item) || !Item->HasHome() || Item->IsPlaced() || Item->HasNoHomeSpot())
	{
		return false;
	}
	Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeldItem = nullptr;
	Item->PlaceAtHome();
	OnReleased.Broadcast(Item, false);
	return true;
}

void ULTTCarryComponent::ThrowWithVelocity(const FVector& Velocity)
{
	Release(true, &Velocity);
}

void ULTTCarryComponent::Release(bool bThrow, const FVector* ThrowVelocity)
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
	if (bThrow && ThrowVelocity)
	{
		LinearVelocity = OwnerVelocity + *ThrowVelocity;
		AngularVelocity = FMath::VRand() * 180.f;
	}
	else if (bThrow)
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

	const float Yaw = HeldItem->bKeepPickupYaw ? HeldRelativeYaw : 0.f;
	const FQuat RelQuat = FRotator(0.f, Yaw, 0.f).Quaternion() * HeldItem->HoldRotationOffset.Quaternion();

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
		float SwingT = -1.f;
		if (SwingElapsed >= 0.f)
		{
			SwingElapsed += DeltaTime;
			SwingT = SwingElapsed / SwingDuration;
			if (SwingT >= 1.f)
			{
				SwingElapsed = -1.f;
				SwingT = -1.f;
			}
		}

		if (HeldItem->bIsMop && HoldParent)
		{
			// The mop is held by its handle and points at the puddle being mopped.
			ComputeToolTarget(HoldParent, DeltaTime, SwingT, TargetLoc, TargetRot);
		}
		else
		{
			ComputeHoldTarget(TargetLoc, TargetRot);
			// Use swing: push forward and down, with a sideways sweep, then back.
			if (SwingT >= 0.f)
			{
				const float Push = FMath::Sin(PI * SwingT);
				TargetLoc += FVector(16.f * Push, 9.f * FMath::Sin(2.f * PI * SwingT), -7.f * Push);
			}
		}

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
	// The mop is gripped on its handle (its bounds centre), not beside its head's width.
	const float ExtRight = HeldItem->bIsMop ? 0.f : FMath::Abs(Right.X) * E.X + FMath::Abs(Right.Y) * E.Y + FMath::Abs(Right.Z) * E.Z;

	const FVector Base = B.Origin + Fwd * GripOffset.X + Up * GripOffset.Z;
	RightHand.Location = Base + Right * (ExtRight + GripOffset.Y);
	LeftHand.Location = Base - Right * (ExtRight + GripOffset.Y);

	if (HeldItem->bIsMop)
	{
		// On the handle, where the mop is gripped.
		RightHand.Location = HeldItem->Mesh->GetComponentTransform().TransformPosition(FVector(0.f, 0.f, ToolGripHeight));
	}

	RightHand.JointTarget =Cam.TransformPosition(ElbowPoleOffset);
	LeftHand.JointTarget = Cam.TransformPosition(FVector(ElbowPoleOffset.X, -ElbowPoleOffset.Y, ElbowPoleOffset.Z));

	RightHand.Rotation = Cam.Rotator();
	LeftHand.Rotation = Cam.Rotator();
}
