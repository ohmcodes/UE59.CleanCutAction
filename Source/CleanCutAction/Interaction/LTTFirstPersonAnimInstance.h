// Clean! Cut! Action! - Parent class for the first person arms Anim Blueprint (ABP_FP_Copy).
// Reads hand IK targets from the owner's ULTTCarryComponent so the hands grab the held prop.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LTTFirstPersonAnimInstance.generated.h"

UCLASS()
class CLEANCUTACTION_API ULTTFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	float RightHandIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FVector RightHandIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FVector RightElbowTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FRotator RightHandIKRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	float LeftHandIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FVector LeftElbowTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
	FRotator LeftHandIKRotation = FRotator::ZeroRotator;

	/** Hand bone rotation relative to the camera while gripping (depends on the skeleton's bone axes). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand IK")
	FRotator RightHandRotationOffset = FRotator(0.f, -90.f, -90.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand IK")
	FRotator LeftHandRotationOffset = FRotator(0.f, 90.f, 90.f);

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
