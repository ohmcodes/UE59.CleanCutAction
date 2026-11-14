// Copyright Epic Games, Inc. All Rights Reserved.

#include "CleanCutActionCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CleanCutAction.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

ACleanCutActionCharacter::ACleanCutActionCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Crouch (C / Ctrl / right stick click / touch button): a shorter capsule for getting under tables and into tight spots.
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(48.f);
	GetCharacterMovement()->MaxWalkSpeedCrouched = 250.f;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
}

void ACleanCutActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACleanCutActionCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACleanCutActionCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACleanCutActionCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACleanCutActionCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACleanCutActionCharacter::LookInput);

		// Crouch: a runtime action + context for CrouchKeys unless an asset action is set.
		const UInputAction* Crouch = CrouchAction;
		if (!Crouch)
		{
			APlayerController* PC = Cast<APlayerController>(GetController());
			UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
			if (Subsystem)
			{
				if (!RuntimeCrouchAction)
				{
					RuntimeCrouchAction = NewObject<UInputAction>(this, TEXT("IA_Crouch_Runtime"));
					RuntimeCrouchContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Crouch_Runtime"));
					for (const FKey& Key : CrouchKeys)
					{
						if (Key.IsValid())
						{
							RuntimeCrouchContext->MapKey(RuntimeCrouchAction, Key);
						}
					}
				}
				Subsystem->AddMappingContext(RuntimeCrouchContext, 1);
				Crouch = RuntimeCrouchAction;
			}
		}
		if (Crouch)
		{
			EnhancedInputComponent->BindAction(Crouch, ETriggerEvent::Started, this, &ACleanCutActionCharacter::DoCrouchToggle);
		}
	}
	else
	{
		UE_LOG(LogCleanCutAction, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ACleanCutActionCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ACleanCutActionCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ACleanCutActionCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACleanCutActionCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ACleanCutActionCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ACleanCutActionCharacter::BeginPlay()
{
	Super::BeginPlay();
	FirstPersonMeshBaseLocation = FirstPersonMesh->GetRelativeLocation();
}

void ACleanCutActionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!FMath::IsNearlyEqual(CrouchViewOffset, CrouchViewTarget))
	{
		CrouchViewOffset = FMath::FInterpConstantTo(CrouchViewOffset, CrouchViewTarget, DeltaSeconds, CrouchViewSpeed);
		// The first person mesh is parented to the body mesh, which may be scaled; keep the offset in world cm.
		const float ParentScaleZ = FMath::Max(KINDA_SMALL_NUMBER, GetMesh()->GetComponentScale().Z);
		FirstPersonMesh->SetRelativeLocation(FirstPersonMeshBaseLocation + FVector(0.f, 0.f, CrouchViewOffset / ParentScaleZ));
	}
}

void ACleanCutActionCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// The capsule top came down by twice the half-height change; bring the eyes down with it.
	CrouchViewTarget = -2.f * ScaledHalfHeightAdjust;
}

void ACleanCutActionCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchViewTarget = 0.f;
}

void ACleanCutActionCharacter::DoCrouchToggle()
{
	if (IsCrouchingOrWantsTo())
	{
		UnCrouch(); // movement keeps us down until there's room to stand
	}
	else
	{
		Crouch();
	}
}

bool ACleanCutActionCharacter::IsCrouchingOrWantsTo() const
{
	return bIsCrouched || (GetCharacterMovement() && GetCharacterMovement()->bWantsToCrouch);
}

void ACleanCutActionCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
