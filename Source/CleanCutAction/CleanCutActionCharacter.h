// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InputCoreTypes.h"
#include "CleanCutActionCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ULTTVRComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract, config=Game)
class ACleanCutActionCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Switches the character to a VR body (headset camera, tracked hands, floating HUD) when a headset is active. Idle otherwise. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULTTVRComponent> VRComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Crouch toggle. If empty, a runtime action is made for CrouchKeys. */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> CrouchAction;

	/** Keys that toggle crouch when no CrouchAction asset is set. */
	UPROPERTY(EditAnywhere, Category ="Input")
	TArray<FKey> CrouchKeys = { EKeys::C, EKeys::LeftControl, EKeys::Gamepad_RightThumbstick };

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeCrouchAction;

	/** Right stick turn speed at full tilt, degrees per second. The stick used to add a fixed angle every frame,
	 *  so it spun far too fast (and faster at higher frame rates). Mouse and trackpad look are unchanged. */
	UPROPERTY(EditAnywhere, Config, Category ="Input|Gamepad Look", meta = (ClampMin = "10", ClampMax = "720"))
	float GamepadYawRate = 140.f;

	UPROPERTY(EditAnywhere, Config, Category ="Input|Gamepad Look", meta = (ClampMin = "10", ClampMax = "720"))
	float GamepadPitchRate = 95.f;

	/** Response curve: 1 = linear, higher = finer aim near the centre of the stick, same top speed. */
	UPROPERTY(EditAnywhere, Config, Category ="Input|Gamepad Look", meta = (ClampMin = "1", ClampMax = "4"))
	float GamepadLookExponent = 1.8f;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeCrouchContext;

	/** How fast the view drops / rises when crouching (cm per second). */
	UPROPERTY(EditAnywhere, Category ="Crouch", meta = (ClampMin = "10"))
	float CrouchViewSpeed = 400.f;

	/** The camera rides the arms' head bone, which the engine keeps at standing height when the capsule shrinks,
	 *  so the arms mesh is lowered by this offset (eases toward CrouchViewTarget). */
	float CrouchViewOffset = 0.f;
	float CrouchViewTarget = 0.f;
	FVector FirstPersonMeshBaseLocation = FVector::ZeroVector;
	
public:
	ACleanCutActionCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Right stick look: scaled to GamepadYawRate / GamepadPitchRate per second with the response curve. */
	void GamepadLookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:
	/** Crouches if standing, stands if crouched (stays down while there's no headroom). Touch buttons can call this too. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoCrouchToggle();

	UFUNCTION(BlueprintPure, Category="Input")
	bool IsCrouchingOrWantsTo() const;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns the VR component (always present; active only with a headset) **/
	ULTTVRComponent* GetVRComponent() const { return VRComponent; }

};

