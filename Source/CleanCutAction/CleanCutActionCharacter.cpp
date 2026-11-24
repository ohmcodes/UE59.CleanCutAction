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
#include "Animation/AnimSequenceBase.h"
#include "TimerManager.h"
#include "Audio/LTTMusicPlayer.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "VR/LTTVRComponent.h"

/** Emote animation and input action, loaded by path (both are in MapsToCook). */
static const TCHAR* LTTEmoteAnimPath = TEXT("/Game/_CleanCutAction/Character/Rookie/Anims/A_Emote_Rookie.A_Emote_Rookie");
static const TCHAR* LTTEmoteActionPath = TEXT("/Game/_CleanCutAction/Input/IA_Emote.IA_Emote");

/** The dance song while it plays (single player). */
static TWeakObjectPtr<UAudioComponent> LTTEmoteMusic;

/** Starts the dance song and pauses the background music. Respects the music on/off toggle. */
static void LTTStartEmoteMusic(UWorld* World)
{
	if (!World || ALTTMusicPlayer::IsMusicMuted())
	{
		return;
	}
	// Song for the dance, loaded by path (in MapsToCook). Kept out of MusicFX so the background music player doesn't shuffle it in.
	// Its loudness is the Volume setting on the paroparo Sound Wave asset.
	const TCHAR* SongPath = TEXT("/Game/_CleanCutAction/EmoteMusic/paroparo.paroparo");
	USoundBase* Song = LoadObject<USoundBase>(nullptr, SongPath);
	if (!Song)
	{
		UE_LOG(LogCleanCutAction, Warning, TEXT("Emote music not found: %s"), SongPath);
		return;
	}
	for (TActorIterator<ALTTMusicPlayer> It(World); It; ++It)
	{
		It->SetMusicPaused(true);
	}
	if (UAudioComponent* Old = LTTEmoteMusic.Get())
	{
		Old->Stop();
	}
	// Not a UI sound, so it pauses with the game like the dance does.
	LTTEmoteMusic = UGameplayStatics::SpawnSound2D(World, Song);
}

/** Fades the dance song out and resumes the background music where it was. */
static void LTTStopEmoteMusic(UWorld* World)
{
	UAudioComponent* Song = LTTEmoteMusic.Get();
	LTTEmoteMusic.Reset();
	if (!Song)
	{
		return; // music was off when the dance started, so nothing was paused
	}
	Song->FadeOut(0.5f, 0.f);
	if (World)
	{
		for (TActorIterator<ALTTMusicPlayer> It(World); It; ++It)
		{
			It->SetMusicPaused(false);
		}
	}
}

/** Plays the emote once on the body mesh (the one mirrors and other cameras see), then hands the mesh back to its anim blueprint.
 *  The mesh goes to single-node mode for the length of the clip, so this doesn't depend on the anim blueprint having a montage slot. */
static void LTTPlayEmote(ACleanCutActionCharacter& Character)
{
	USkeletalMeshComponent* Body = Character.GetMesh();
	if (!Body)
	{
		return;
	}
	if (Body->GetAnimationMode() == EAnimationMode::AnimationSingleNode)
	{
		// Already emoting: the key cancels it.
		Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		LTTStopEmoteMusic(Character.GetWorld());
		return;
	}
	UAnimSequenceBase* Emote = LoadObject<UAnimSequenceBase>(nullptr, LTTEmoteAnimPath);
	if (!Emote)
	{
		UE_LOG(LogCleanCutAction, Warning, TEXT("Emote animation not found: %s"), LTTEmoteAnimPath);
		return;
	}
	Body->PlayAnimation(Emote, false);
	LTTStartEmoteMusic(Character.GetWorld());
	FTimerHandle Handle;
	Character.GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(&Character, [WeakBody = TWeakObjectPtr<USkeletalMeshComponent>(Body)]()
	{
		// Still playing means this emote was cancelled and a newer one started; that one has its own timer.
		if (WeakBody.IsValid() && WeakBody->GetAnimationMode() == EAnimationMode::AnimationSingleNode && !WeakBody->IsPlaying())
		{
			WeakBody->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			LTTStopEmoteMusic(WeakBody->GetWorld());
		}
	}), Emote->GetPlayLength() + 0.1f, false);
}

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

	VRComponent = CreateDefaultSubobject<ULTTVRComponent>(TEXT("VR"));

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

	VRComponent = CreateDefaultSubobject<ULTTVRComponent>(TEXT("VR"));
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
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACleanCutActionCharacter::GamepadLookInput);
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

		// Emote (G / d-pad up / UI_TouchSimple's Btn_Emote, which injects IA_Emote): the action asset is loaded by path,
		// the key mapping is a runtime context like crouch's. Kept out of the header so it can go in through Live Coding.
		if (UInputAction* EmoteAction = LoadObject<UInputAction>(nullptr, LTTEmoteActionPath))
		{
			APlayerController* PC = Cast<APlayerController>(GetController());
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr)
			{
				UInputMappingContext* EmoteContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Emote_Runtime"));
				EmoteContext->MapKey(EmoteAction, EKeys::G);
				EmoteContext->MapKey(EmoteAction, EKeys::Gamepad_DPad_Up);
				Subsystem->AddMappingContext(EmoteContext, 1);
			}
			EnhancedInputComponent->BindActionValueLambda(EmoteAction, ETriggerEvent::Started,
				[WeakThis = TWeakObjectPtr<ACleanCutActionCharacter>(this)](const FInputActionValue&)
				{
					if (WeakThis.IsValid())
					{
						LTTPlayEmote(*WeakThis);
					}
				});
		}
		else
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("Emote input action not found: %s"), LTTEmoteActionPath);
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

void ACleanCutActionCharacter::GamepadLookInput(const FInputActionValue& Value)
{
	FVector2D Stick = Value.Get<FVector2D>();
	const float Tilt = FMath::Min(Stick.Size(), 1.f);
	if (Tilt <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Curve on the tilt, not per axis, so diagonals keep their direction.
	Stick = Stick / Stick.Size() * FMath::Pow(Tilt, GamepadLookExponent);

	const UWorld* World = GetWorld();
	const float DeltaSeconds = World ? World->GetDeltaSeconds() : 0.f;
	// NOTE(5.9): DefaultInput.ini keeps bEnableLegacyInputScales, so the controller multiplies yaw by 2.5 and pitch by -2.5;
	// divide that back out so the rates above are real degrees per second.
	constexpr float LegacyInputScale = 2.5f;
	DoAim(Stick.X * GamepadYawRate * DeltaSeconds / LegacyInputScale, Stick.Y * GamepadPitchRate * DeltaSeconds / LegacyInputScale);
}

void ACleanCutActionCharacter::DoAim(float Yaw, float Pitch)
{
	// In VR the headset aims and the right stick turns; a bumped mouse or stick mustn't swing the view.
	if (VRComponent && VRComponent->IsVRActive())
	{
		return;
	}
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
		// VR: the stick moves where the head faces, not where the body (snap turn) faces.
		if (VRComponent && VRComponent->IsVRActive() && FirstPersonCameraComponent)
		{
			const FRotator HeadYaw(0.f, FirstPersonCameraComponent->GetComponentRotation().Yaw, 0.f);
			const FRotationMatrix Basis(HeadYaw);
			AddMovementInput(Basis.GetUnitAxis(EAxis::Y), Right);
			AddMovementInput(Basis.GetUnitAxis(EAxis::X), Forward);
			return;
		}
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
