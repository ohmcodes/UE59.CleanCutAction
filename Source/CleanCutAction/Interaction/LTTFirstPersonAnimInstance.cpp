// Clean! Cut! Action!

#include "Interaction/LTTFirstPersonAnimInstance.h"
#include "Interaction/LTTCarryComponent.h"
#include "VR/LTTVRComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"

namespace
{
	/** VR: a hand not busy carrying follows its controller (grip pose). False if there's no tracked controller. */
	bool LTTControllerHandIK(const AActor* Owner, FName ControllerName, float Side, FLTTHandIK& Out)
	{
		const ULTTVRComponent* VR = Owner ? Owner->FindComponentByClass<ULTTVRComponent>() : nullptr;
		if (!VR || !VR->IsVRActive())
		{
			return false;
		}
		TArray<UMotionControllerComponent*> Controllers;
		Owner->GetComponents(Controllers);
		const UMotionControllerComponent* Controller = nullptr;
		for (const UMotionControllerComponent* C : Controllers)
		{
			if (C && C->GetFName() == ControllerName)
			{
				Controller = C;
				break;
			}
		}
		const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
		if (!Controller || !Controller->IsTracked() || !Camera)
		{
			return false;
		}
		const FTransform Grip = Controller->GetComponentTransform();
		Out.Alpha = 1.f;
		// The grip pose sits in the palm; the IK target is the wrist, a little behind it.
		Out.Location = Grip.GetLocation() - Grip.GetRotation().GetForwardVector() * 8.f;
		Out.Rotation = Grip.Rotator();
		// Elbow hint: below and out to the side of the shoulder, so the arms bend naturally.
		const FTransform Head = Camera->GetComponentTransform();
		Out.JointTarget = Head.TransformPosition(FVector(-10.f, 45.f * Side, -70.f));
		return true;
	}
}

void ULTTFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AActor* Owner = GetOwningActor();
	const ULTTCarryComponent* Carry = Owner ? Owner->FindComponentByClass<ULTTCarryComponent>() : nullptr;

	FLTTHandIK R;
	FLTTHandIK L;
	if (Carry)
	{
		R = Carry->GetRightHandIK();
		L = Carry->GetLeftHandIK();
	}
	// VR: hands that aren't holding anything go to the controllers.
	if (R.Alpha <= KINDA_SMALL_NUMBER)
	{
		LTTControllerHandIK(Owner, TEXT("VR_RightGrip"), 1.f, R);
	}
	if (L.Alpha <= KINDA_SMALL_NUMBER)
	{
		LTTControllerHandIK(Owner, TEXT("VR_LeftGrip"), -1.f, L);
	}

	RightHandIKAlpha = R.Alpha;
	RightHandIKLocation = R.Location;
	RightElbowTarget = R.JointTarget;
	RightHandIKRotation = (R.Rotation.Quaternion() * RightHandRotationOffset.Quaternion()).Rotator();

	LeftHandIKAlpha = L.Alpha;
	LeftHandIKLocation = L.Location;
	LeftElbowTarget = L.JointTarget;
	LeftHandIKRotation = (L.Rotation.Quaternion() * LeftHandRotationOffset.Quaternion()).Rotator();
}
