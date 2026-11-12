// Clean! Cut! Action! - Holds one prop in front of the camera, drops/throws it with physics,
// and provides hand IK targets for the first person arms (ULTTFirstPersonAnimInstance).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTCarryComponent.generated.h"

class ALTTInteractableBase;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnPickedUp, ALTTInteractableBase*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLTTOnReleased, ALTTInteractableBase*, Item, bool, bThrown);

/** World-space hand IK data for one hand. */
USTRUCT(BlueprintType)
struct FLTTHandIK
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	float Alpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FVector Location = FVector::ZeroVector;

	/** Elbow pole target. */
	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FVector JointTarget = FVector::ZeroVector;

	/** Camera-relative basis of the grip, world space. */
	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTCarryComponent();

	/** Component the held prop attaches to. Defaults to the owner's first camera. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void SetHoldParent(USceneComponent* InParent) { HoldParent = InParent; }

	UFUNCTION(BlueprintCallable, Category = "Carry")
	bool PickUp(ALTTInteractableBase* Item);

	/** Gentle drop in front of the player with physics. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Drop();

	/** Throw forward with physics. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Throw();

	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsHolding() const { return HeldItem != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Carry")
	ALTTInteractableBase* GetHeldItem() const { return HeldItem; }

	UFUNCTION(BlueprintPure, Category = "Carry|IK")
	const FLTTHandIK& GetRightHandIK() const { return RightHand; }

	UFUNCTION(BlueprintPure, Category = "Carry|IK")
	const FLTTHandIK& GetLeftHandIK() const { return LeftHand; }

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FLTTOnPickedUp OnPickedUp;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FLTTOnReleased OnReleased;

	// ---- Tuning ----

	/** Minimum distance of a held prop's centre from the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Hold")
	float BaseHoldDistance = 45.f;

	/** Sideways offset for one-handed props (they sit in the right hand). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Hold")
	float OneHandSideOffset = 16.f;

	/** How far below the eye line held props sit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Hold")
	float HoldDownOffset = 18.f;

	/** Time for a picked up prop to fly into the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Hold")
	float PickupBlendTime = 0.18f;

	/** Forward speed added on Drop (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Release")
	float DropForwardSpeed = 150.f;

	/** Throw speed for a small prop (cm/s); scaled by the prop's throw multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Release")
	float ThrowSpeed = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Release")
	float ThrowUpAngle = 10.f;

	/** IK blend in/out time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|IK")
	float HandIKBlendTime = 0.15f;

	/** Grip offset from the prop's side, camera space (X fwd, Y outwards, Z up). Mirrored for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|IK")
	FVector GripOffset = FVector(-2.f, 2.f, -2.f);

	/** Elbow pole offset from the camera, camera space. Mirrored for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|IK")
	FVector ElbowPoleOffset = FVector(0.f, 45.f, -70.f);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Release(bool bThrow);
	void ComputeHoldTarget(FVector& OutRelLocation, FRotator& OutRelRotation) const;
	void UpdateHandIK(float DeltaTime);
	USceneComponent* ResolveHoldParent();

	UPROPERTY(Transient)
	TObjectPtr<ALTTInteractableBase> HeldItem;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HoldParent;

	/** Prop yaw relative to the camera at pickup, kept while holding. */
	float HeldRelativeYaw = 0.f;

	float BlendElapsed = 0.f;
	FVector BlendStartLocation = FVector::ZeroVector;
	FQuat BlendStartRotation = FQuat::Identity;

	FLTTHandIK RightHand;
	FLTTHandIK LeftHand;
};
