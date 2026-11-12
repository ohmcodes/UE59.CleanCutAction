// Clean! Cut! Action!

#include "Interaction/LTTFirstPersonAnimInstance.h"
#include "Interaction/LTTCarryComponent.h"

void ULTTFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AActor* Owner = GetOwningActor();
	const ULTTCarryComponent* Carry = Owner ? Owner->FindComponentByClass<ULTTCarryComponent>() : nullptr;
	if (!Carry)
	{
		RightHandIKAlpha = 0.f;
		LeftHandIKAlpha = 0.f;
		return;
	}

	const FLTTHandIK& R = Carry->GetRightHandIK();
	RightHandIKAlpha = R.Alpha;
	RightHandIKLocation = R.Location;
	RightElbowTarget = R.JointTarget;
	RightHandIKRotation = (R.Rotation.Quaternion() * RightHandRotationOffset.Quaternion()).Rotator();

	const FLTTHandIK& L = Carry->GetLeftHandIK();
	LeftHandIKAlpha = L.Alpha;
	LeftHandIKLocation = L.Location;
	LeftElbowTarget = L.JointTarget;
	LeftHandIKRotation = (L.Rotation.Quaternion() * LeftHandRotationOffset.Quaternion()).Rotator();
}
