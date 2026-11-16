// Copyright © 2026 QLE OPC. All Rights Reserved.

#include "VR/LTTVRComponent.h"
#include "VR/LTTVR.h"
#include "VR/LTTVRScreen.h"
#include "CleanCutActionCharacter.h"
#include "CleanCutActionPlayerController.h"
#include "CleanCutAction.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "Interaction/LTTInteractionComponent.h"
#include "UI/LTTInputDevice.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "IXRTrackingSystem.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "Scalability.h"
#include "UObject/ConstructorHelpers.h"
#if LTT_WITH_XRBASE
#include "XRDeviceVisualizationComponent.h"
#endif

namespace
{
	/** Eye height used when the runtime only offers a seated (non floor-level) tracking space, cm. */
	constexpr float LTTLocalSpaceEyeHeight = 165.f;

	template <typename T>
	T* LTTFindAsset(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	void LTTSetCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByCode);
		}
	}
}

ULTTVRComponent::ULTTVRComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Menus pause the game; the hands, laser and screen still have to work.
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Hard references so they're cooked with the character.
	VRMappingContext = LTTFindAsset<UInputMappingContext>(TEXT("/Game/_CleanCutAction/Input/VR/IMC_VR.IMC_VR"));
	MoveAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Move.IA_VR_Move"));
	TurnAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Turn.IA_VR_Turn"));
	GrabAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Grab.IA_VR_Grab"));
	CrouchAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Crouch.IA_VR_Crouch"));
	BackAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Back.IA_VR_Back"));
	PauseAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Pause.IA_VR_Pause"));
	UIClickAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_UIClick.IA_VR_UIClick"));
	RecenterAction = LTTFindAsset<UInputAction>(TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Recenter.IA_VR_Recenter"));
	LaserMesh = LTTFindAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	LaserMaterial = LTTFindAsset<UMaterialInterface>(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

void ULTTVRComponent::BeginPlay()
{
	Super::BeginPlay();
	if (LTTVR::IsActive())
	{
		ActivateVR();
	}
}

bool ULTTVRComponent::GetAimRay(FVector& OutLocation, FRotator& OutRotation) const
{
	if (!bVRActive || !RightAim || !RightAim->IsTracked())
	{
		return false;
	}
	OutLocation = RightAim->GetComponentLocation();
	OutRotation = RightAim->GetComponentRotation();
	return true;
}

void ULTTVRComponent::ActivateVR()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Char ? Cast<APlayerController>(Char->GetController()) : nullptr;
	UCameraComponent* Cam = Char ? Char->FindComponentByClass<UCameraComponent>() : nullptr;
	if (bVRActive || !Char || !PC || !PC->IsLocalController() || !Cam)
	{
		return; // tried again next tick (the controller may not have possessed yet)
	}
	bVRActive = true;
	Camera = Cam;
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: headset '%s' active, switching %s to VR."), GEngine->XRSystem.IsValid() ? *GEngine->XRSystem->GetSystemName().ToString() : TEXT("?"), *GetNameSafe(Char));

	if (GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
	}

	// Floor-level tracking origin under the capsule; the headset moves the camera on it.
	UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
	VROrigin = NewObject<USceneComponent>(Char, TEXT("VR_Origin"));
	VROrigin->SetupAttachment(Capsule);
	VROrigin->SetRelativeLocation(FVector(0.f, 0.f, -Capsule->GetScaledCapsuleHalfHeight()));
	VROrigin->RegisterComponent();

	Cam->AttachToComponent(VROrigin, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Cam->SetRelativeTransform(FTransform::Identity);
	Cam->bUsePawnControlRotation = false;
	Cam->bLockToHmd = true;
	Cam->bEnableFirstPersonFieldOfView = false;
	Cam->bEnableFirstPersonScale = false;

	// No arms in VR: the controllers are the hands. The body mesh stays (mirrors and other cameras see it).
	if (ACleanCutActionCharacter* CCChar = Cast<ACleanCutActionCharacter>(Char))
	{
		if (USkeletalMeshComponent* Arms = CCChar->GetFirstPersonMesh())
		{
			Arms->SetHiddenInGame(true, false);
		}
	}

	// Head pitch comes from the headset; the controller keeps only the body's yaw.
	PC->SetControlRotation(FRotator(0.f, Char->GetActorRotation().Yaw, 0.f));

	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		StandingWalkSpeed = Move->MaxWalkSpeed;
		Move->MaxWalkSpeed *= WalkSpeedScale;
		Move->MaxWalkSpeedCrouched *= WalkSpeedScale;
	}

	// ---- Hands ----
	auto MakeController = [this, Char](const TCHAR* Name, FName Source, bool bShowModel) -> UMotionControllerComponent*
	{
		UMotionControllerComponent* Controller = NewObject<UMotionControllerComponent>(Char, Name);
		Controller->SetTrackingMotionSource(Source);
		Controller->SetupAttachment(VROrigin);
		Controller->SetTickableWhenPaused(true);
		Controller->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Controller->RegisterComponent();
#if LTT_WITH_XRBASE
		if (bShowModel)
		{
			// The runtime's own controller model (OpenXR render model).
			UXRDeviceVisualizationComponent* Model = NewObject<UXRDeviceVisualizationComponent>(Char, *FString::Printf(TEXT("%s_Model"), Name));
			Model->SetupAttachment(Controller);
			Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Model->SetCastShadow(false);
			Model->SetIsVisualizationActive(true);
			Model->RegisterComponent();
		}
#endif
		return Controller;
	};
	LeftGrip = MakeController(TEXT("VR_LeftGrip"), FName(TEXT("Left")), true);
	RightGrip = MakeController(TEXT("VR_RightGrip"), FName(TEXT("Right")), true);
	RightAim = MakeController(TEXT("VR_RightAim"), FName(TEXT("RightAim")), false);

	// Laser: a thin cylinder along the aim (the mesh is 100 cm tall along Z, centred).
	Laser = NewObject<UStaticMeshComponent>(Char, TEXT("VR_Laser"));
	Laser->SetStaticMesh(LaserMesh);
	Laser->SetupAttachment(RightAim);
	Laser->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Laser->SetCastShadow(false);
	Laser->SetTickableWhenPaused(true);
	Laser->RegisterComponent();
	if (LaserMaterial)
	{
		LaserMID = UMaterialInstanceDynamic::Create(LaserMaterial, this);
		Laser->SetMaterial(0, LaserMID);
	}

	// Props are held in the right hand instead of in front of the camera.
	if (ULTTCarryComponent* Carry = Char->FindComponentByClass<ULTTCarryComponent>())
	{
		Carry->SetHoldParent(RightGrip);
		Carry->bHoldInHand = true;
	}

	// ---- Floating screen for HUDs and menus ----
	ScreenWidget = CreateWidget<ULTTVRScreen>(PC, ULTTVRScreen::StaticClass());
	Screen = NewObject<UWidgetComponent>(Char, TEXT("VR_Screen"));
	Screen->SetupAttachment(VROrigin);
	Screen->SetUsingAbsoluteLocation(true);
	Screen->SetUsingAbsoluteRotation(true);
	Screen->SetUsingAbsoluteScale(true);
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetDrawSize(FVector2D(ScreenResolution.X, ScreenResolution.Y));
	Screen->SetPivot(FVector2D(0.5f, 0.5f));
	Screen->SetBlendMode(EWidgetBlendMode::Transparent);
	Screen->SetTwoSided(true);
	Screen->SetBackgroundColor(FLinearColor::Transparent);
	Screen->SetTickWhenOffscreen(true);
	Screen->SetRedrawTime(1.f / FMath::Max(ScreenRedrawRate, 1.f));
	Screen->SetWindowFocusable(false);
	Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Screen->SetCastShadow(false);
	Screen->SetTickableWhenPaused(true);
	Screen->SetOwnerPlayer(PC->GetLocalPlayer());
	Screen->RegisterComponent();
	Screen->SetWidget(ScreenWidget);
	Screen->SetVisibility(false);

	// The laser clicks the screen. Its hit is computed here (Custom source), so the screen needs no collision
	// and never blocks the interaction trace.
	Pointer = NewObject<UWidgetInteractionComponent>(Char, TEXT("VR_Pointer"));
	Pointer->SetupAttachment(RightAim);
	Pointer->InteractionSource = EWidgetInteractionSource::Custom;
	Pointer->bShowDebug = false;
	Pointer->SetTickableWhenPaused(true);
	Pointer->RegisterComponent();

	// Prompts: VR buttons count as gamepad keys, so the gamepad layout (relabelled for VR) is used.
	ULTTInputDeviceSubsystem::SetCurrent(ELTTInputDevice::Gamepad);

	if (bApplyVRRenderSettings)
	{
		ApplyRenderSettings();
	}
	TryBindInput();
}

void ULTTVRComponent::ApplyRenderSettings()
{
	// Scalability: cap the costly groups. Stereo renders every frame twice, at 72-120 fps.
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.ShadowQuality = FMath::Min(Levels.ShadowQuality, 2);
	Levels.PostProcessQuality = FMath::Min(Levels.PostProcessQuality, 1);
	Levels.EffectsQuality = FMath::Min(Levels.EffectsQuality, 2);
	Levels.FoliageQuality = FMath::Min(Levels.FoliageQuality, 2);
	Levels.ShadingQuality = FMath::Min(Levels.ShadingQuality, 2);
	Levels.ViewDistanceQuality = FMath::Min(Levels.ViewDistanceQuality, 2);
	Levels.ReflectionQuality = FMath::Min(Levels.ReflectionQuality, bDisableLumenInVR ? 1 : 2);
	Levels.GlobalIlluminationQuality = FMath::Min(Levels.GlobalIlluminationQuality, bDisableLumenInVR ? 1 : 2);
	Scalability::SetQualityLevels(Levels);

	// Effects that are uncomfortable or wasted in a headset.
	LTTSetCVar(TEXT("r.MotionBlurQuality"), 0);
	LTTSetCVar(TEXT("r.DepthOfFieldQuality"), 0);
	LTTSetCVar(TEXT("r.LensFlareQuality"), 0);
	LTTSetCVar(TEXT("r.SceneColorFringeQuality"), 0);
	LTTSetCVar(TEXT("r.Tonemapper.GrainQuantization"), 0);
	LTTSetCVar(TEXT("r.VSync"), 0); // the headset paces frames
	LTTSetCVar(TEXT("t.MaxFPS"), 0);
#if !PLATFORM_ANDROID
	// Desktop (deferred): virtual shadow maps cost a lot per eye; cascaded shadow maps are cheaper.
	LTTSetCVar(TEXT("r.Shadow.Virtual.Enable"), 0);
	// The project has no AA (MSAA only works with forward shading); aliasing shimmers badly in a headset.
	if (IConsoleVariable* AA = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		if (AA->GetInt() == 0)
		{
			AA->Set(2, ECVF_SetByCode); // TAA
		}
	}
#endif
}

void ULTTVRComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UEnhancedInputComponent* EIC = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!bVRActive || !EIC || BoundInputComponent.Get() == EIC)
	{
		return;
	}
	BoundInputComponent = EIC;

	// IMC_VR is a default mapping context (OpenXR builds its actions from those); add it anyway in case that was turned off.
	if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (VRMappingContext && !Subsystem->HasMappingContext(VRMappingContext))
			{
				Subsystem->AddMappingContext(VRMappingContext, 1);
			}
		}
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ULTTVRComponent::OnMove);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ULTTVRComponent::OnMoveDone);
	}
	if (TurnAction)
	{
		EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ULTTVRComponent::OnTurn);
		EIC->BindAction(TurnAction, ETriggerEvent::Completed, this, &ULTTVRComponent::OnTurnDone);
	}
	if (GrabAction)
	{
		EIC->BindAction(GrabAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnGrabStarted);
		EIC->BindAction(GrabAction, ETriggerEvent::Completed, this, &ULTTVRComponent::OnGrabCompleted);
	}
	if (CrouchAction)
	{
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnCrouch);
	}
	if (BackAction)
	{
		EIC->BindAction(BackAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnBack);
		EIC->BindAction(BackAction, ETriggerEvent::Completed, this, &ULTTVRComponent::OnBackDone);
	}
	if (PauseAction)
	{
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnPause);
	}
	if (UIClickAction)
	{
		EIC->BindAction(UIClickAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnUIClickStarted);
		EIC->BindAction(UIClickAction, ETriggerEvent::Completed, this, &ULTTVRComponent::OnUIClickCompleted);
	}
	if (RecenterAction)
	{
		EIC->BindAction(RecenterAction, ETriggerEvent::Started, this, &ULTTVRComponent::OnRecenter);
	}
}

void ULTTVRComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bVRActive)
	{
		// The headset can come on after BeginPlay (or the controller possesses later).
		if (LTTVR::IsActive())
		{
			ActivateVR();
		}
		if (!bVRActive)
		{
			return;
		}
	}
	if (!Camera.IsValid())
	{
		return;
	}

	TryBindInput();

	// Menus switch to UI-only input, which makes the viewport drop every key, VR buttons included.
	// Slate still gets them first, so keep the viewport listening: the laser and the Menu button must keep working.
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		if (Viewport->IgnoreInput())
		{
			Viewport->SetIgnoreInput(false);
		}
	}

	// Stick flicks (station choices): an edge past half way on either stick.
	const float FlickX = FMath::Abs(MoveStick.X) > FMath::Abs(TurnStick.X) ? MoveStick.X : TurnStick.X;
	StickFlick = 0;
	if (FlickX > 0.6f && PrevFlickX <= 0.6f)
	{
		StickFlick = 1;
	}
	else if (FlickX < -0.6f && PrevFlickX >= -0.6f)
	{
		StickFlick = -1;
	}
	PrevFlickX = FlickX;

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const bool bPaused = PC && PC->IsPaused();
	if (!bPaused)
	{
		if (!bSnapTurn && !FMath::IsNearlyZero(SmoothTurnInput) && PC && !PC->IsLookInputIgnored())
		{
			Turn(SmoothTurnInput * SmoothTurnRate * DeltaTime);
		}
		UpdateBody(DeltaTime);
	}
	UpdateHands(DeltaTime);
	UpdateScreen(DeltaTime);
	UpdateVignette(DeltaTime);
}

void ULTTVRComponent::UpdateBody(float DeltaTime)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UCameraComponent* Cam = Camera.Get();
	if (!Char || !Cam || !VROrigin)
	{
		return;
	}
	UCapsuleComponent* Capsule = Char->GetCapsuleComponent();

	// Room scale: walking in real life moves the capsule under the head (walls still stop it).
	FVector Delta = Cam->GetComponentLocation() - Char->GetActorLocation();
	Delta.Z = 0.0;
	if (Delta.SizeSquared() > FMath::Square(2.0))
	{
		const FVector Before = Char->GetActorLocation();
		Char->AddActorWorldOffset(Delta, true);
		FVector Moved = Char->GetActorLocation() - Before;
		Moved.Z = 0.0;
		VROrigin->AddWorldOffset(-Moved);
	}

	// Eye height above the floor. A seated (local) tracking space starts at the head, so give it a standing height.
	const bool bFloorSpace = !GEngine->XRSystem.IsValid() || GEngine->XRSystem->GetTrackingOrigin() != EHMDTrackingOrigin::Local;
	const float BaseHeight = (bFloorSpace ? 0.f : LTTLocalSpaceEyeHeight) + ViewHeightOffset;
	const float Eye = static_cast<float>(Cam->GetRelativeLocation().Z) + BaseHeight;

	// Physical crouch: duck in real life to fit under tables.
	if (bPhysicalCrouch && !bCrouchFromButton)
	{
		if (!bCrouchFromBody && Eye > 1.f && Eye < PhysicalCrouchEyeHeight)
		{
			Char->Crouch();
			bCrouchFromBody = true;
		}
		else if (bCrouchFromBody && Eye > PhysicalCrouchEyeHeight + 15.f)
		{
			Char->UnCrouch(); // stays down until there's headroom
			bCrouchFromBody = false;
		}
	}

	// Crouched with the button (or kept down by a low ceiling): lower the view to fit the shorter capsule.
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	float TargetDrop = 0.f;
	if (Char->bIsCrouched && !bCrouchFromBody)
	{
		TargetDrop = FMath::Min(0.f, (2.f * HalfHeight - 12.f) - Eye);
	}
	CrouchDrop = FMath::FInterpTo(CrouchDrop, TargetDrop, DeltaTime, 8.f);

	FVector Rel = VROrigin->GetRelativeLocation();
	Rel.Z = -HalfHeight + BaseHeight + CrouchDrop;
	VROrigin->SetRelativeLocation(Rel);
}

void ULTTVRComponent::Turn(float Degrees)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	UCameraComponent* Cam = Camera.Get();
	if (!Controller || !Cam || !VROrigin)
	{
		return;
	}
	// Turn around the head, not the capsule: the player may stand away from its centre.
	const FVector OldHead = Cam->GetComponentLocation();
	FRotator Rot = Controller->GetControlRotation();
	Rot.Yaw = FRotator::NormalizeAxis(Rot.Yaw + Degrees);
	Rot.Pitch = 0.f;
	Rot.Roll = 0.f;
	Controller->SetControlRotation(Rot);
	Pawn->SetActorRotation(FRotator(0.f, Rot.Yaw, 0.f));
	FVector Fix = OldHead - Cam->GetComponentLocation();
	Fix.Z = 0.0;
	VROrigin->AddWorldOffset(Fix);
	if (bScreenPlaced)
	{
		// The screen turns with the player.
		ScreenYaw += Degrees;
		ScreenTargetYaw += Degrees;
	}
}

FVector ULTTVRComponent::GetHandVelocity() const
{
	if (HandSamples.Num() < 2 || !VROrigin)
	{
		return FVector::ZeroVector;
	}
	const FLTTVRHandSample& First = HandSamples[0];
	const FLTTVRHandSample& Last = HandSamples.Last();
	const double Dt = Last.Time - First.Time;
	if (Dt <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}
	// Measured in tracking space, so turning and walking with the sticks don't count as hand speed.
	return VROrigin->GetComponentTransform().TransformVectorNoScale((Last.Location - First.Location) / Dt);
}

int32 ULTTVRComponent::GetHandShakeSign() const
{
	const UCameraComponent* Cam = Camera.Get();
	if (!Cam)
	{
		return 0;
	}
	const FVector Right = FRotator(0.f, Cam->GetComponentRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
	const double Side = FVector::DotProduct(GetHandVelocity(), Right);
	return Side > 120.0 ? 1 : (Side < -120.0 ? -1 : 0);
}

void ULTTVRComponent::UpdateHands(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !VROrigin)
	{
		return;
	}

	// Recent right hand positions (about 0.1 s) for throw speed.
	if (RightGrip && RightGrip->IsTracked())
	{
		const double Now = World->GetRealTimeSeconds();
		FLTTVRHandSample Sample;
		Sample.Location = VROrigin->GetComponentTransform().InverseTransformPositionNoScale(RightGrip->GetComponentLocation());
		Sample.Time = Now;
		HandSamples.Add(Sample);
		while (HandSamples.Num() > 2 && Now - HandSamples[0].Time > 0.1)
		{
			HandSamples.RemoveAt(0, EAllowShrinking::No);
		}
	}
	else
	{
		HandSamples.Reset();
	}

	FVector AimLoc;
	FRotator AimRot;
	const bool bAim = GetAimRay(AimLoc, AimRot);
	const FVector AimDir = AimRot.Vector();

	// Does the laser hit the screen? (A ray against its plane; the screen has no collision.)
	bool bOnScreen = false;
	float LaserLen = LaserLength;
	if (bAim && Screen && Screen->IsVisible())
	{
		const FTransform& T = Screen->GetComponentTransform();
		const FVector P = T.InverseTransformPosition(AimLoc);
		const FVector D = T.InverseTransformVector(AimDir);
		if (FMath::Abs(D.X) > KINDA_SMALL_NUMBER)
		{
			const double Dist = -P.X / D.X; // world cm, since AimDir is a unit vector
			const FVector Local = P + D * Dist;
			const FVector2D Half = Screen->GetDrawSize() * 0.5f;
			if (Dist > 0.0 && Dist < 1000.0 && FMath::Abs(Local.Y) <= Half.X && FMath::Abs(Local.Z) <= Half.Y)
			{
				bOnScreen = true;
				LaserLen = static_cast<float>(Dist);
				FHitResult Hit;
				Hit.bBlockingHit = true;
				Hit.Component = Screen.Get();
				Hit.HitObjectHandle = FActorInstanceHandle(GetOwner());
				Hit.TraceStart = AimLoc;
				Hit.TraceEnd = AimLoc + AimDir * LaserLength;
				Hit.Location = Hit.ImpactPoint = AimLoc + AimDir * Dist;
				Hit.Normal = Hit.ImpactNormal = -AimDir;
				Hit.Distance = static_cast<float>(Dist);
				if (Pointer)
				{
					Pointer->SetCustomHitResult(Hit);
				}
			}
		}
	}
	if (!bOnScreen && Pointer)
	{
		Pointer->SetCustomHitResult(FHitResult());
	}
	bPointingAtUI = bOnScreen && Pointer && Pointer->IsOverInteractableWidget();
	if (!bOnScreen && bUIPressed && Pointer)
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
		bUIPressed = false;
	}

	// Laser: to the screen, or to whatever it hits in the world.
	const ULTTInteractionComponent* Interaction = GetOwner()->FindComponentByClass<ULTTInteractionComponent>();
	const ULTTCarryComponent* Carry = GetOwner()->FindComponentByClass<ULTTCarryComponent>();
	FLinearColor Color(1.f, 1.f, 1.f);
	if (bAim && !bOnScreen)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTVRLaser), false, GetOwner());
		if (Carry && Carry->GetHeldItem())
		{
			Params.AddIgnoredActor(Carry->GetHeldItem());
		}
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, AimLoc, AimLoc + AimDir * LaserLength, ECC_Visibility, Params))
		{
			LaserLen = Hit.Distance;
		}
		if (Interaction && (Interaction->GetFocusedActor() || Interaction->GetPlaceHint() == ELTTPlaceHint::Place
			|| Interaction->GetPlaceHint() == ELTTPlaceHint::Trash || Interaction->GetPlaceHint() == ELTTPlaceHint::Wash
			|| Interaction->GetPlaceHint() == ELTTPlaceHint::Mop))
		{
			Color = FLinearColor(0.2f, 1.f, 0.35f); // something to grab, use or put back
		}
	}
	else if (bOnScreen)
	{
		Color = bPointingAtUI ? FLinearColor(0.2f, 0.75f, 1.f) : FLinearColor(0.6f, 0.6f, 0.7f);
	}
	if (Laser)
	{
		Laser->SetVisibility(bAim);
		LaserLen = FMath::Max(LaserLen, 1.f);
		Laser->SetRelativeLocation(FVector(LaserLen * 0.5f, 0.f, 0.f));
		Laser->SetRelativeScale3D(FVector(0.005f, 0.005f, LaserLen / 100.f));
		if (LaserMID)
		{
			LaserMID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

void ULTTVRComponent::CaptureViewportWidgets()
{
	UWorld* World = GetWorld();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!World || !ScreenWidget)
	{
		return;
	}
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UUserWidget::StaticClass(), true);
	UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(World);
	for (UUserWidget* Widget : Found)
	{
		if (!Widget || Widget == ScreenWidget || !Widget->IsInViewport())
		{
			continue;
		}
		if (Widget->GetOwningPlayer() && Widget->GetOwningPlayer() != PC)
		{
			continue;
		}
		const int32 ZOrder = ViewportSubsystem ? ViewportSubsystem->GetWidgetSlot(Widget).ZOrder : 0;
		// Hold its Slate widget while it moves, so it isn't destroyed and rebuilt (that would re-run its Construct).
		const TSharedRef<SWidget> Keep = Widget->TakeWidget();
		Widget->RemoveFromParent();
		ScreenWidget->AddLayer(Widget, ZOrder);
	}
}

void ULTTVRComponent::UpdateScreen(float DeltaTime)
{
	UCameraComponent* Cam = Camera.Get();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Screen || !ScreenWidget || !Cam || !PC)
	{
		return;
	}
	CaptureViewportWidgets();

	const bool bVisible = ScreenWidget->HasVisibleLayer();
	if (Screen->IsVisible() != bVisible)
	{
		Screen->SetVisibility(bVisible);
	}
	if (!bVisible)
	{
		return;
	}

	// A menu (pause, end screen, briefing) pops up straight ahead.
	const bool bMenu = PC->IsPaused() || PC->ShouldShowMouseCursor();
	if (bMenu && !bScreenHadMenu)
	{
		bRecenterScreen = true;
	}
	bScreenHadMenu = bMenu;

	// The view, not the pawn's camera: the briefing and the end-of-level tour look through other cameras.
	FVector Head = Cam->GetComponentLocation();
	float HeadYaw = static_cast<float>(Cam->GetComponentRotation().Yaw);
	if (const APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
	{
		Head = CameraManager->GetCameraLocation();
		HeadYaw = static_cast<float>(CameraManager->GetCameraRotation().Yaw);
	}
	// The view cut to another camera (or teleported): put the screen straight in front again, no drift.
	if (FVector::DistSquared(Head, LastScreenHead) > FMath::Square(150.0))
	{
		bScreenPlaced = false;
	}
	LastScreenHead = Head;
	if (!bScreenPlaced)
	{
		ScreenYaw = ScreenTargetYaw = HeadYaw;
		ScreenZ = static_cast<float>(Head.Z) - 15.f;
		bScreenPlaced = true;
		bRecenterScreen = false;
	}
	else if (bRecenterScreen || FMath::Abs(FRotator::NormalizeAxis(HeadYaw - ScreenTargetYaw)) > ScreenFollowAngle)
	{
		// Lazy follow: stays put while the player looks around it, catches up when they turn away.
		ScreenTargetYaw = HeadYaw;
		bRecenterScreen = false;
	}
	ScreenYaw = static_cast<float>(FMath::RInterpTo(FRotator(0.f, ScreenYaw, 0.f), FRotator(0.f, ScreenTargetYaw, 0.f), DeltaTime, 5.f).Yaw);
	ScreenZ = FMath::FInterpTo(ScreenZ, static_cast<float>(Head.Z) - 15.f, DeltaTime, 3.f);

	// Closer (and smaller, same apparent size) when a wall is in the way, so it isn't buried in it.
	const FVector Dir = FRotator(0.f, ScreenYaw, 0.f).Vector();
	float Distance = ScreenDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LTTVRScreen), false, GetOwner());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Head, Head + Dir * ScreenDistance, ECC_Visibility, Params))
	{
		Distance = FMath::Max(Hit.Distance - 10.f, 60.f);
	}
	const float Scale = (ScreenWidth / FMath::Max(ScreenResolution.X, 1)) * (Distance / ScreenDistance);
	Screen->SetWorldScale3D(FVector(Scale));
	// The widget's right is +Y, so its X axis points away from the viewer.
	Screen->SetWorldLocationAndRotation(FVector(Head.X, Head.Y, ScreenZ) + Dir * Distance, FRotator(0.f, ScreenYaw, 0.f));
}

void ULTTVRComponent::UpdateVignette(float DeltaTime)
{
	UCameraComponent* Cam = Camera.Get();
	if (!Cam)
	{
		return;
	}
	const bool bMoving = MoveStick.SizeSquared() > 0.01f || (!bSnapTurn && FMath::Abs(SmoothTurnInput) > 0.1f);
	const float Target = bComfortVignette && bMoving ? ComfortVignetteIntensity : 0.f;
	if (FMath::IsNearlyEqual(Vignette, Target, 0.001f) && Cam->PostProcessSettings.bOverride_VignetteIntensity)
	{
		return;
	}
	Vignette = FMath::FInterpTo(Vignette, Target, DeltaTime, 6.f);
	Cam->PostProcessSettings.bOverride_VignetteIntensity = true;
	Cam->PostProcessSettings.VignetteIntensity = Vignette;
}

// ---- Input ----

void ULTTVRComponent::OnMove(const FInputActionValue& Value)
{
	MoveStick = Value.Get<FVector2D>();
	APawn* Pawn = Cast<APawn>(GetOwner());
	const UCameraComponent* Cam = Camera.Get();
	if (!Pawn || !Cam)
	{
		return;
	}
	const USceneComponent* Basis = (!bHeadRelativeMovement && LeftGrip && LeftGrip->IsTracked()) ? static_cast<const USceneComponent*>(LeftGrip) : Cam;
	const FRotator Yaw(0.f, Basis->GetComponentRotation().Yaw, 0.f);
	Pawn->AddMovementInput(Yaw.RotateVector(FVector::ForwardVector), MoveStick.Y);
	Pawn->AddMovementInput(Yaw.RotateVector(FVector::RightVector), MoveStick.X);
}

void ULTTVRComponent::OnMoveDone(const FInputActionValue& Value)
{
	MoveStick = FVector2D::ZeroVector;
}

void ULTTVRComponent::OnTurn(const FInputActionValue& Value)
{
	TurnStick = Value.Get<FVector2D>();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || PC->IsLookInputIgnored() || PC->IsPaused())
	{
		SmoothTurnInput = 0.f;
		return;
	}
	if (bSnapTurn)
	{
		SmoothTurnInput = 0.f;
		if (!bSnapLatched && FMath::Abs(TurnStick.X) > 0.7f)
		{
			Turn(FMath::Sign(TurnStick.X) * SnapTurnDegrees);
			bSnapLatched = true;
		}
		else if (FMath::Abs(TurnStick.X) < 0.35f)
		{
			bSnapLatched = false;
		}
	}
	else
	{
		SmoothTurnInput = TurnStick.X;
	}
}

void ULTTVRComponent::OnTurnDone(const FInputActionValue& Value)
{
	TurnStick = FVector2D::ZeroVector;
	SmoothTurnInput = 0.f;
	bSnapLatched = false;
}

void ULTTVRComponent::OnGrabStarted(const FInputActionValue& Value)
{
	ULTTInteractionComponent* Interaction = GetOwner()->FindComponentByClass<ULTTInteractionComponent>();
	ULTTCarryComponent* Carry = GetOwner()->FindComponentByClass<ULTTCarryComponent>();
	if (bPointingAtUI || !Interaction || !Carry || Carry->IsHolding())
	{
		return;
	}
	// Same as the trigger: picks up the prop under the laser (or uses a switch, a station...).
	Interaction->Interact();
	bHeldByGrip = Carry->IsHolding();
}

void ULTTVRComponent::OnGrabCompleted(const FInputActionValue& Value)
{
	if (!bHeldByGrip)
	{
		return;
	}
	bHeldByGrip = false;
	ULTTInteractionComponent* Interaction = GetOwner()->FindComponentByClass<ULTTInteractionComponent>();
	ULTTCarryComponent* Carry = GetOwner()->FindComponentByClass<ULTTCarryComponent>();
	if (!Interaction || !Carry || !Carry->IsHolding())
	{
		return;
	}
	const FVector HandVelocity = GetHandVelocity();
	if (HandVelocity.Size() >= FlickThrowSpeed)
	{
		Carry->ThrowWithVelocity((HandVelocity * HandThrowScale).GetClampedToMaxSize(MaxHandThrowSpeed));
	}
	else
	{
		// Let go: put it back if aimed at its home, bin it, wash it... or just drop it.
		Interaction->ReleaseHeld();
	}
}

void ULTTVRComponent::OnCrouch(const FInputActionValue& Value)
{
	if (ACleanCutActionCharacter* Char = Cast<ACleanCutActionCharacter>(GetOwner()))
	{
		if (bCrouchFromBody)
		{
			return; // already down in real life
		}
		Char->DoCrouchToggle();
		bCrouchFromButton = Char->IsCrouchingOrWantsTo();
	}
}

void ULTTVRComponent::OnBack(const FInputActionValue& Value)
{
	bBackDown = true;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (ACleanCutActionPlayerController* PC = Pawn ? Cast<ACleanCutActionPlayerController>(Pawn->GetController()) : nullptr)
	{
		if (PC->IsPauseMenuOpen())
		{
			PC->ClosePauseMenu();
		}
	}
}

void ULTTVRComponent::OnBackDone(const FInputActionValue& Value)
{
	bBackDown = false;
}

void ULTTVRComponent::OnPause(const FInputActionValue& Value)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (ACleanCutActionPlayerController* PC = Pawn ? Cast<ACleanCutActionPlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->TogglePauseMenu();
	}
}

void ULTTVRComponent::OnUIClickStarted(const FInputActionValue& Value)
{
	bTriggerDown = true;
	if (Pointer && Screen && Screen->IsVisible() && Pointer->GetHoveredWidgetComponent() == Screen)
	{
		Pointer->PressPointerKey(EKeys::LeftMouseButton);
		bUIPressed = true;
	}
}

void ULTTVRComponent::OnUIClickCompleted(const FInputActionValue& Value)
{
	bTriggerDown = false;
	if (bUIPressed && Pointer)
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	bUIPressed = false;
}

void ULTTVRComponent::OnRecenter(const FInputActionValue& Value)
{
	RecenterScreen();
}
