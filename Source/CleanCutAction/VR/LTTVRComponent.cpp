// Clean! Cut! Action!

#include "VR/LTTVRComponent.h"
#include "CleanCutAction.h"
#include "IXRTrackingSystem.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StereoLayerComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Interaction/LTTCleanupHUD.h"
#include "IHeadMountedDisplay.h"
#include "ISpectatorScreenController.h"
#include "HeadMountedDisplayTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

/** VR-only input actions, loaded by path (in MapsToCook). IMC_VR maps them, and it's an Enhanced Input default
 *  mapping context so the OpenXR runtime sees its bindings. */
static const TCHAR* LTTVRTurnActionPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Turn.IA_VR_Turn");
static const TCHAR* LTTVRUIClickActionPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_UIClick.IA_VR_UIClick");

int32 ULTTVRComponent::ActiveCount = 0;

namespace
{
	/** Headset is on and rendering in stereo (VR Preview, -vr, Start in VR). */
	bool LTTIsHMDRendering()
	{
		return GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed();
	}

	/** Draw size of the UI panel; the viewport widgets are laid out as if on a 1080p screen. */
	const FVector2D LTTPanelDrawSize(1920.f, 1080.f);

	const FName LTTUILayerName(TEXT("VR_UILayer"));

	/** 1: show the UI through an OpenXR quad stereo layer (always on top). 0: draw the widget panel in the scene. */
	TAutoConsoleVariable<int32> CVarLTTVRUILayer(
		TEXT("ltt.VR.UILayer"), 0,
		TEXT("VR UI panel: 1 = OpenXR quad stereo layer (composited on top), 0 = world-space widget drawn in the scene."));

	/** Share of the headset's horizontal field of view the UI panel covers. */
	TAutoConsoleVariable<float> CVarLTTVRUIScreenFill(
		TEXT("ltt.VR.UIScreenFill"), 0.85f,
		TEXT("VR UI panel width as a share of the headset's horizontal field of view (0.5 - 1)."));

	/** 1: the UI panel is glued to the view like a screen. 0: it lazily follows the head (world-space menu). */
	TAutoConsoleVariable<int32> CVarLTTVRUIHeadLocked(
		TEXT("ltt.VR.UIHeadLocked"), 1,
		TEXT("VR UI panel: 1 = head-locked (moves with the view), 0 = lazy follow (stays in the world, re-centres when you look away)."));

	/** Ray against the UI panel plane (the panel faces back along its +X). True and fills Hit when it lands on the panel. */
	bool LTTTracePanel(UWidgetComponent* Panel, const FVector& Start, const FVector& Dir, FHitResult& Hit)
	{
		const FVector Normal = Panel->GetForwardVector();
		const double Denom = FVector::DotProduct(Dir, Normal);
		if (Denom >= -KINDA_SMALL_NUMBER)
		{
			return false;
		}
		const double T = FVector::DotProduct(Panel->GetComponentLocation() - Start, Normal) / Denom;
		if (T <= 0.0 || T >= 1000.0)
		{
			return false;
		}
		const FVector Point = Start + Dir * T;
		const FVector Local = Panel->GetComponentTransform().InverseTransformPosition(Point);
		const FVector2D Half = LTTPanelDrawSize * 0.5f;
		if (FMath::Abs(Local.Y) > Half.X || FMath::Abs(Local.Z) > Half.Y)
		{
			return false;
		}
		Hit = FHitResult();
		Hit.bBlockingHit = true;
		Hit.Component = Panel;
		Hit.Location = Hit.ImpactPoint = Point;
		Hit.Normal = Hit.ImpactNormal = Normal;
		Hit.TraceStart = Start;
		Hit.TraceEnd = Point;
		Hit.Distance = T;
		return true;
	}

	/** Reaches the protected SimulatePointerMovement, to re-aim the pointer and click in the same frame. */
	struct FLTTPointerAccess : public UWidgetInteractionComponent
	{
		using UWidgetInteractionComponent::SimulatePointerMovement;
	};

	/** The VR component whose current click is aimed by the head instead of the controller (held until release). */
	TWeakObjectPtr<const ULTTVRComponent> GLTTGazeClicker;

	/** Flat-screen HUD parts that don't belong in VR: the first-person crosshair, and the pickup ring, which is placed
	 *  with the flat screen's projection and can't line up on the VR panel. */
	void LTTAdaptWidgetForVR(UUserWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		if (UWidget* Crosshair = Widget->GetWidgetFromName(TEXT("Crosshair")))
		{
			Crosshair->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ULTTCleanupHUD* CleanupHUD = Cast<ULTTCleanupHUD>(Widget))
		{
			CleanupHUD->bShowPickupRing = false;
		}
	}

	/**
	 * Keeps the character's body with the headset:
	 * - The capsule sweeps to wherever the head moved (walking in the room), stopping at walls, and the VR origin
	 *   shifts back so the view stays inside the capsule: the head can't leave the body or go through walls.
	 * - Both meshes turn to face where the head looks, and slide so their head bone sits under the eyes: the
	 *   first person mesh (seen only by the player: arms and hands) exactly at the camera, the world body (seen in
	 *   mirrors and by others) under it horizontally. The arms' hands follow the controllers in
	 *   ULTTFirstPersonAnimInstance.
	 */
	void LTTUpdateVRBody(ACharacter* Character, UCameraComponent* Camera, USceneComponent* VROrigin)
	{
		if (!Character || !Camera || !VROrigin)
		{
			return;
		}

		// 1. Capsule follows the head (horizontal only).
		const FVector CapsuleBefore = Character->GetActorLocation();
		FVector Delta = Camera->GetComponentLocation() - CapsuleBefore;
		Delta.Z = 0.f;
		if (Delta.SizeSquared() > 0.25f)
		{
			Character->AddActorWorldOffset(Delta, /*bSweep*/ true);
			// Whatever the sweep allowed, the view goes back to the capsule's centre.
			VROrigin->AddWorldOffset(-Delta);
		}

		const float HeadYaw = Camera->GetComponentRotation().Yaw;
		const FRotator MeshFacing(0.f, HeadYaw - 90.f, 0.f); // the Rookie mesh faces +Y
		const FVector Forward = FRotator(0.f, HeadYaw, 0.f).Vector();
		const FName HeadBone(TEXT("head"));

		TArray<USkeletalMeshComponent*> Meshes;
		Character->GetComponents(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (!Mesh || !Mesh->DoesSocketExist(HeadBone))
			{
				continue;
			}
			Mesh->SetWorldRotation(MeshFacing);
			const bool bFirstPerson = Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson;
			// The eyes are inside the first person mesh's head: hide its neck and head (the arms hang off the spine,
			// so they stay). The world body keeps its head for mirrors.
			if (bFirstPerson)
			{
				const FName Neck = Mesh->DoesSocketExist(TEXT("neck_01")) ? FName(TEXT("neck_01")) : HeadBone;
				if (!Mesh->IsBoneHiddenByName(Neck))
				{
					Mesh->HideBoneByName(Neck, PBO_None);
				}
			}
			// Eyes sit a little in front of the head bone.
			const FVector WantHead = Camera->GetComponentLocation() - Forward * 8.f - FVector(0.f, 0.f, 6.f);
			FVector Offset = WantHead - Mesh->GetSocketLocation(HeadBone);
			if (!bFirstPerson)
			{
				Offset.Z = 0.f; // the world body keeps its feet on the floor
			}
			if (Offset.SizeSquared() > 0.01f)
			{
				// Ease in so animation sway doesn't jitter the mesh.
				Mesh->AddWorldOffset(Offset * 0.5f);
			}
		}
	}

	/** Console variables changed for VR, with their previous values. */
	TMap<FString, FString> GLTTSavedVRCVars;

	void LTTSetVRCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (!GLTTSavedVRCVars.Contains(Name))
			{
				GLTTSavedVRCVars.Add(Name, Var->GetString());
			}
			Var->Set(Value, ECVF_SetByCode);
		}
	}

	void LTTRestoreVRCVars()
	{
		for (const TPair<FString, FString>& Pair : GLTTSavedVRCVars)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*Pair.Key))
			{
				Var->Set(*Pair.Value, ECVF_SetByCode);
			}
		}
		GLTTSavedVRCVars.Reset();
	}

	/** UI panel pitch (single local player), kept here so the header doesn't change. */
	float GLTTPanelPitch = 0.f;

	/** The stereo layer that shows the UI panel's render target (found by name, so it needs no header member). */
	UStereoLayerComponent* LTTFindUILayer(const AActor* Owner)
	{
		TArray<UStereoLayerComponent*> Layers;
		if (Owner)
		{
			Owner->GetComponents(Layers);
		}
		for (UStereoLayerComponent* Layer : Layers)
		{
			if (Layer && Layer->GetFName() == LTTUILayerName)
			{
				return Layer;
			}
		}
		return nullptr;
	}
}

void ULTTVRUIHost::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// Pointer dots, on top of every captured menu (captured widgets are inserted before this canvas).
		UCanvasPanel* DotLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VR_DotLayer"));
		DotLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* LayerSlot = Root->AddChildToOverlay(DotLayer))
		{
			LayerSlot->SetHorizontalAlignment(HAlign_Fill);
			LayerSlot->SetVerticalAlignment(VAlign_Fill);
		}
		auto MakeDot = [this, DotLayer](FName Name, const FLinearColor& Fill)
		{
			UImage* Dot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = FSlateColor(Fill);
			Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(0.f), FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.85f)), 3.f);
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			Dot->SetBrush(Brush);
			Dot->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* DotSlot = DotLayer->AddChildToCanvas(Dot))
			{
				DotSlot->SetAutoSize(false);
				DotSlot->SetSize(FVector2D(24.f));
			}
		};
		MakeDot(TEXT("VR_DotGaze"), FLinearColor(0.2f, 0.9f, 1.f, 0.9f));
		MakeDot(TEXT("VR_DotPointer"), FLinearColor(1.f, 0.85f, 0.1f, 1.f));
	}
	else if (WidgetTree)
	{
		Root = Cast<UOverlay>(WidgetTree->RootWidget);
	}
}

ULTTVRComponent::ULTTVRComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Menus (pause, end screen) are used while the game is paused.
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	// After the camera and motion controllers have this frame's pose.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	// Hard references so the cook keeps the engine shapes.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	SphereMesh = Sphere.Object;
	CylinderMesh = Cylinder.Object;
}

bool ULTTVRComponent::IsHeadsetActive()
{
	return ActiveCount > 0;
}

void ULTTVRComponent::BeginPlay()
{
	Super::BeginPlay();
	TurnAction = LoadObject<UInputAction>(nullptr, LTTVRTurnActionPath);
	UIClickAction = LoadObject<UInputAction>(nullptr, LTTVRUIClickActionPath);
}

void ULTTVRComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bVRActive)
	{
		bVRActive = false;
		ActiveCount = FMath::Max(0, ActiveCount - 1);
		LTTRestoreVRCVars();
	}
	Super::EndPlay(EndPlayReason);
}

void ULTTVRComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const bool bLocal = Pawn && Pawn->IsLocallyControlled() && Cast<APlayerController>(Pawn->GetController());
	const bool bWantVR = bLocal && LTTIsHMDRendering();
	if (bWantVR != bVRActive)
	{
		bWantVR ? EnterVR() : ExitVR();
	}
	if (!bVRActive)
	{
		return;
	}

	TryBindInput();
	LTTUpdateVRBody(Cast<ACharacter>(GetOwner()), GetOwner()->FindComponentByClass<UCameraComponent>(), VROrigin);

	CaptureTimer -= DeltaTime;
	if (CaptureTimer <= 0.f)
	{
		CaptureTimer = 0.1f;
		CaptureViewportWidgets();
	}
	UpdatePanel(DeltaTime);
	UpdatePointer();
}

UMotionControllerComponent* ULTTVRComponent::MakeController(FName Name, FName Source, bool bVisible)
{
	UMotionControllerComponent* Controller = NewObject<UMotionControllerComponent>(GetOwner(), Name);
	Controller->SetupAttachment(VROrigin);
	Controller->SetTrackingMotionSource(Source);
	// Keeps tracking while the pause menu is up, so the laser can still point at it.
	Controller->SetTickableWhenPaused(true);
	Controller->RegisterComponent();

	if (bVisible && SphereMesh)
	{
		UStaticMeshComponent* Marker = NewObject<UStaticMeshComponent>(GetOwner(), *(Name.ToString() + TEXT("_Marker")));
		Marker->SetupAttachment(Controller);
		Marker->SetStaticMesh(SphereMesh);
		Marker->SetRelativeScale3D(FVector(0.06f)); // the basic sphere is 100 cm across
		Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Marker->SetCastShadow(false);
		Marker->RegisterComponent();
	}
	return Controller;
}

void ULTTVRComponent::EnterVR()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCameraComponent* Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Camera || !PC)
	{
		return;
	}
	bVRActive = true;
	++ActiveCount;
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: headset active, switching %s to VR mode"), *GetNameSafe(Character));

	// Stereo needs 72+ fps or the runtime (XR Simulator, Quest) treats the app as unresponsive and drops its input.
	// Lumen GI and reflections, rendered twice, cost the most and don't exist on Quest anyway: switch them off in VR
	// and put the previous values back in ExitVR.
	LTTSetVRCVar(TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.ReflectionMethod"), TEXT("2"));

	// The desktop window mirrors one eye. The default crops it to fill the window, and the eye's field of view isn't
	// centred, so the UI looked cut off and shifted there. Show the whole eye, letterboxed, so the window matches
	// what the headset (or the XR Simulator) shows.
	if (GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
	{
		if (ISpectatorScreenController* Spectator = GEngine->XRSystem->GetHMDDevice()->GetSpectatorScreenController())
		{
			Spectator->SetSpectatorScreenMode(ESpectatorScreenMode::SingleEyeLetterboxed);
		}
	}

	// Seated / standing-stationary: the origin is wherever the head starts, placed at standing eye height on the capsule,
	// so a seated player sees the set from the same height as a standing one.
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Local);
		GEngine->XRSystem->ResetOrientationAndPosition(0.f);
	}

	if (!VROrigin)
	{
		VROrigin = NewObject<USceneComponent>(Character, TEXT("VROrigin"));
		VROrigin->SetupAttachment(Character->GetRootComponent());
		VROrigin->RegisterComponent();
	}
	VROrigin->SetRelativeLocation(FVector(0.f, 0.f, Character->BaseEyeHeight));

	// Camera: off the arms' head bone (animated, would bob the view) and onto the origin; the HMD drives it from there.
	FlatCameraParent = Camera->GetAttachParent();
	FlatCameraSocket = Camera->GetAttachSocketName();
	FlatCameraRelative = Camera->GetRelativeTransform();
	Camera->AttachToComponent(VROrigin, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Camera->SetRelativeTransform(FTransform::Identity);
	Camera->SetWorldScale3D(FVector::OneVector);
	Camera->bUsePawnControlRotation = false;
	Camera->bLockToHmd = true;
	Camera->SetEnableFirstPersonFieldOfView(false);
	Camera->SetEnableFirstPersonScale(false);

	// The first person arms follow the flat camera; hide them (the controllers have their own markers).
	TArray<USkeletalMeshComponent*> Meshes;
	Character->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson)
		{
			// Hidden in VR: seen from just behind the eyes, a full first-person body (torso, arms reaching to the
			// controllers) fills the view. Its hand IK on the controllers still runs, ready for a hands-only look.
			Mesh->SetHiddenInGame(true, false);
		}
	}

	// Level the view: pitch would only tilt movement and the flat aim, the head does the looking now.
	FRotator Control = PC->GetControlRotation();
	Control.Pitch = 0.f;
	Control.Roll = 0.f;
	PC->SetControlRotation(Control);

	if (!LeftController)
	{
		LeftController = MakeController(TEXT("VR_LeftGrip"), FName(TEXT("Left")), false);
		RightController = MakeController(TEXT("VR_RightGrip"), FName(TEXT("Right")), false);
		RightAim = MakeController(TEXT("VR_RightAim"), FName(TEXT("RightAim")), false);

		Laser = NewObject<UStaticMeshComponent>(Character, TEXT("VR_Laser"));
		Laser->SetupAttachment(RightAim);
		Laser->SetStaticMesh(CylinderMesh);
		Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Laser->SetCastShadow(false);
		Laser->SetHiddenInGame(true);
		Laser->RegisterComponent();

		Pointer = NewObject<UWidgetInteractionComponent>(Character, TEXT("VR_Pointer"));
		Pointer->SetupAttachment(RightAim);
		Pointer->InteractionSource = EWidgetInteractionSource::Custom;
		Pointer->bEnableHitTesting = true;
		Pointer->VirtualUserIndex = 0;
		Pointer->PointerIndex = 0;
		Pointer->SetTickableWhenPaused(true);
		Pointer->RegisterComponent();
	}

	if (!UIPanel)
	{
		UIHost = CreateWidget<ULTTVRUIHost>(PC, ULTTVRUIHost::StaticClass());
		UIPanel = NewObject<UWidgetComponent>(Character, TEXT("VR_UIPanel"));
		UIPanel->SetupAttachment(Character->GetRootComponent());
		UIPanel->SetAbsolute(true, true, true);
		UIPanel->SetWidgetSpace(EWidgetSpace::World);
		UIPanel->SetDrawSize(LTTPanelDrawSize);
		UIPanel->SetPivot(FVector2D(0.5f, 0.5f));
		UIPanel->SetBlendMode(EWidgetBlendMode::Transparent);
		UIPanel->SetTwoSided(false);
		UIPanel->SetTickWhenOffscreen(true);
		UIPanel->SetTickableWhenPaused(true);
		UIPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UIPanel->SetCastShadow(false);
		UIPanel->SetOwnerPlayer(PC->GetLocalPlayer());
		UIPanel->SetWidget(UIHost);
		UIPanel->SetWorldScale3D(FVector(PanelWidth / LTTPanelDrawSize.X));
		// Not drawn in the scene: camera fades (the intro holds the screen black behind the briefing) and walls would
		// cover it. It still renders its widget to a texture, which the stereo layer below shows, and the laser still
		// aims at its plane.
		UIPanel->SetRenderInMainPass(false);
		UIPanel->SetRenderInDepthPass(false);
		// Always on top: a copy of the engine's translucent widget material with depth testing off, so walls and
		// props right in front of the player don't cut through the HUD. Loaded by path (in MapsToCook).
		if (UMaterialInterface* OnTop = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_CleanCutAction/Materials/VR/M_VRWidget_OnTop.M_VRWidget_OnTop")))
		{
			UIPanel->SetMaterial(0, OnTop);
		}
		else
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("VR: M_VRWidget_OnTop not found, the UI panel can clip into walls"));
		}
		UIPanel->SetTranslucentSortPriority(10000);
		UIPanel->RegisterComponent();
	}

	if (!LTTFindUILayer(Character))
	{
		// World-locked quad layer: composited by the XR runtime after the game's frame, so nothing in the scene
		// (fades, post-process, geometry) can hide the UI.
		UStereoLayerComponent* Layer = NewObject<UStereoLayerComponent>(Character, LTTUILayerName);
		Layer->SetupAttachment(Character->GetRootComponent());
		Layer->SetAbsolute(true, true, true);
		if (FByteProperty* TypeProp = FindFProperty<FByteProperty>(UStereoLayerComponent::StaticClass(), TEXT("StereoLayerType")))
		{
			TypeProp->SetPropertyValue_InContainer(Layer, static_cast<uint8>(SLT_WorldLocked));
		}
		else if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(UStereoLayerComponent::StaticClass(), TEXT("StereoLayerType")))
		{
			EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(Layer), static_cast<int64>(SLT_WorldLocked));
		}
		Layer->bLiveTexture = true;
		Layer->bSupportsDepth = false;
		Layer->bQuadPreserveTextureRatio = false;
		Layer->SetQuadSize(FVector2D(PanelWidth, PanelWidth * LTTPanelDrawSize.Y / LTTPanelDrawSize.X));
		Layer->SetPriority(10);
		Layer->SetTickableWhenPaused(true);
		Layer->RegisterComponent();
	}
	bPanelPlaced = false;
	CaptureTimer = 0.f;
}

void ULTTVRComponent::ExitVR()
{
	if (!bVRActive)
	{
		return;
	}
	bVRActive = false;
	ActiveCount = FMath::Max(0, ActiveCount - 1);
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: headset gone, back to flat mode"));
	LTTRestoreVRCVars();

	ReleaseCapturedWidgets();
	if (UIPanel)
	{
		UIPanel->SetHiddenInGame(true);
	}
	if (UStereoLayerComponent* Layer = LTTFindUILayer(GetOwner()))
	{
		Layer->SetVisibility(false);
	}
	if (Laser)
	{
		Laser->SetHiddenInGame(true);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCameraComponent* Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
	if (Camera && FlatCameraParent.IsValid())
	{
		Camera->AttachToComponent(FlatCameraParent.Get(), FAttachmentTransformRules::KeepRelativeTransform, FlatCameraSocket);
		Camera->SetRelativeTransform(FlatCameraRelative);
		Camera->bUsePawnControlRotation = true;
		Camera->SetEnableFirstPersonFieldOfView(true);
		Camera->SetEnableFirstPersonScale(true);
	}
	if (Character)
	{
		TArray<USkeletalMeshComponent*> Meshes;
		Character->GetComponents(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson)
			{
				Mesh->SetHiddenInGame(false, false);
			}
		}
	}
}

void ULTTVRComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UEnhancedInputComponent* EIC = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!EIC || BoundInputComponent.Get() == EIC)
	{
		return;
	}
	if (TurnAction)
	{
		EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ULTTVRComponent::HandleTurn);
		EIC->BindAction(TurnAction, ETriggerEvent::Completed, this, &ULTTVRComponent::HandleTurnReleased);
	}
	else
	{
		UE_LOG(LogCleanCutAction, Warning, TEXT("VR: turn action not found: %s"), LTTVRTurnActionPath);
	}
	if (UIClickAction)
	{
		EIC->BindAction(UIClickAction, ETriggerEvent::Started, this, &ULTTVRComponent::HandleUIClickPressed);
		EIC->BindAction(UIClickAction, ETriggerEvent::Completed, this, &ULTTVRComponent::HandleUIClickReleased);
	}
	else
	{
		UE_LOG(LogCleanCutAction, Warning, TEXT("VR: UI click action not found: %s"), LTTVRUIClickActionPath);
	}
	BoundInputComponent = EIC;
}

void ULTTVRComponent::HandleTurn(const FInputActionValue& Value)
{
	const float X = Value.Get<float>();
	if (!bVRActive)
	{
		return;
	}
	if (FMath::Abs(X) < 0.3f)
	{
		bTurnArmed = true;
		return;
	}
	if (!bTurnArmed || FMath::Abs(X) < 0.6f)
	{
		return;
	}
	bTurnArmed = false;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
	{
		if (PC->IsPaused())
		{
			return;
		}
		FRotator Control = PC->GetControlRotation();
		Control.Yaw += FMath::Sign(X) * SnapTurnDegrees;
		Control.Pitch = 0.f;
		PC->SetControlRotation(Control);
	}
}

void ULTTVRComponent::HandleTurnReleased()
{
	bTurnArmed = true;
}

void ULTTVRComponent::HandleUIClickPressed()
{
	if (!bVRActive || !Pointer)
	{
		return;
	}
	// The controller ray isn't on a button (in the XR Simulator the hands sit off to the side): try where the head looks.
	bool bGaze = false;
	if (!Pointer->IsOverInteractableWidget() && UIPanel && !UIPanel->bHiddenInGame)
	{
		const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
		FHitResult GazeHit;
		if (Camera && LTTTracePanel(UIPanel, Camera->GetComponentLocation(), Camera->GetForwardVector(), GazeHit))
		{
			const FHitResult ControllerHit = Pointer->GetLastHitResult();
			Pointer->SetCustomHitResult(GazeHit);
			(Pointer.Get()->*(&FLTTPointerAccess::SimulatePointerMovement))();
			if (Pointer->IsOverInteractableWidget())
			{
				bGaze = true;
				GLTTGazeClicker = this; // keep aiming by gaze until release, so press and release hit the same button
			}
			else
			{
				Pointer->SetCustomHitResult(ControllerHit);
				(Pointer.Get()->*(&FLTTPointerAccess::SimulatePointerMovement))();
			}
		}
	}
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: UI click (over panel=%d, over button=%d, aimed by %s)"),
		Pointer->GetHoveredWidgetComponent() != nullptr, Pointer->IsOverInteractableWidget(), bGaze ? TEXT("gaze") : TEXT("controller"));
	Pointer->PressPointerKey(EKeys::LeftMouseButton);
}

void ULTTVRComponent::HandleUIClickReleased()
{
	if (bVRActive && Pointer)
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	if (GLTTGazeClicker.Get() == this)
	{
		GLTTGazeClicker.Reset();
	}
}

void ULTTVRComponent::CaptureViewportWidgets()
{
	UOverlay* Root = UIHost ? UIHost->GetRoot() : nullptr;
	if (!Root)
	{
		return;
	}

	// Forget widgets the game has removed from the panel (RemoveFromParent works the same in the overlay).
	for (auto It = CapturedZOrder.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Key()) || It.Key()->GetParent() != Root)
		{
			It.RemoveCurrent();
		}
	}

	bool bCapturedNew = false;
	UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get();
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, UUserWidget::StaticClass(), true);
	for (UUserWidget* Widget : Found)
	{
		if (!Widget || Widget == UIHost || !Widget->IsInViewport())
		{
			continue;
		}
		const int32 ZOrder = Viewport ? Viewport->GetWidgetSlot(Widget).ZOrder : 0;
		Widget->RemoveFromParent();

		// Keep the screen's stacking: insert after every captured widget with a lower or equal Z order.
		int32 Index = 0;
		for (int32 i = 0; i < Root->GetChildrenCount(); ++i)
		{
			const int32* ChildZ = CapturedZOrder.Find(Cast<UUserWidget>(Root->GetChildAt(i)));
			if (ChildZ && *ChildZ <= ZOrder)
			{
				Index = i + 1;
			}
		}
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Root->InsertChildAt(Index, Widget)))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		CapturedZOrder.Add(Widget, ZOrder);
		LTTAdaptWidgetForVR(Widget);
		bCapturedNew = true;
		UE_LOG(LogCleanCutAction, Log, TEXT("VR: moved %s (Z %d) onto the UI panel"), *GetNameSafe(Widget), ZOrder);
	}

	// Menus switch to UI-only input when they open. In VR nothing is on the flat screen to take that input, and the
	// laser / gaze click comes in as game input, so keep game input flowing.
	if (bCapturedNew)
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
		{
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(true);
			// Keyboard focus to the top focusable menu, so its own shortcuts (Enter, N, arrows) still work.
			UUserWidget* Top = nullptr;
			int32 TopZ = MIN_int32;
			for (const TPair<TObjectPtr<UUserWidget>, int32>& Pair : CapturedZOrder)
			{
				if (Pair.Key && Pair.Key->IsFocusable() && Pair.Key->IsVisible() && Pair.Value >= TopZ)
				{
					Top = Pair.Key;
					TopZ = Pair.Value;
				}
			}
			if (Top)
			{
				Mode.SetWidgetToFocus(Top->TakeWidget());
			}
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(false);
		}
	}
}

void ULTTVRComponent::ReleaseCapturedWidgets()
{
	for (const TPair<TObjectPtr<UUserWidget>, int32>& Pair : CapturedZOrder)
	{
		if (UUserWidget* Widget = Pair.Key)
		{
			Widget->RemoveFromParent();
			Widget->AddToViewport(Pair.Value);
		}
	}
	CapturedZOrder.Reset();
}

void ULTTVRComponent::UpdatePanel(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCameraComponent* Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!UIPanel || !Camera)
	{
		return;
	}
	UIPanel->SetHiddenInGame(CapturedZOrder.Num() == 0);

	const FVector Head = Camera->GetComponentLocation();
	const float HeadYaw = Camera->GetComponentRotation().Yaw;
	// Pitch follows too (clamped), so the menu comes back to the middle of the view when looking up or down.
	const float HeadPitch = FMath::Clamp(FRotator::NormalizeAxis(Camera->GetComponentRotation().Pitch), -40.f, 40.f);
	if (!bPanelPlaced)
	{
		PanelYaw = HeadYaw;
		GLTTPanelPitch = HeadPitch;
		bPanelPlaced = true;
	}
	const bool bHeadLocked = CVarLTTVRUIHeadLocked.GetValueOnGameThread() != 0;
	if (bHeadLocked)
	{
		// Screen-style UI: glued to the view (level, no roll), so it never drifts behind when looking around.
		PanelYaw = HeadYaw;
		GLTTPanelPitch = FMath::Clamp(FRotator::NormalizeAxis(Camera->GetComponentRotation().Pitch), -89.f, 89.f);
	}
	else
	{
		// Lazy follow: stays put for small head turns, glides back in front once the head looks well away.
		const float Delta = FRotator::NormalizeAxis(HeadYaw - PanelYaw);
		const float PitchDelta = HeadPitch - GLTTPanelPitch;
		if (FMath::Abs(Delta) > PanelFollowAngle || FMath::Abs(PitchDelta) > PanelFollowAngle * 0.75f)
		{
			const float Alpha = FMath::Clamp(DeltaTime * 4.f, 0.f, 1.f);
			PanelYaw = FRotator::NormalizeAxis(PanelYaw + Delta * Alpha);
			GLTTPanelPitch += PitchDelta * Alpha;
		}
	}
	// Size the panel to the headset's view (like a screen), 85% of the horizontal field of view; the 16:9 height
	// follows. Falls back to PanelWidth when the device doesn't report a field of view.
	float Width = PanelWidth;
	if (GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
	{
		float HFov = 0.f, VFov = 0.f;
		GEngine->XRSystem->GetHMDDevice()->GetFieldOfView(HFov, VFov);
		if (HFov > 10.f && HFov < 170.f)
		{
			Width = 2.f * PanelDistance * FMath::Tan(FMath::DegreesToRadians(HFov * 0.5f)) * CVarLTTVRUIScreenFill.GetValueOnGameThread();
		}
	}
	UIPanel->SetWorldScale3D(FVector(Width / LTTPanelDrawSize.X));
	const FRotator Facing(GLTTPanelPitch, PanelYaw, 0.f);
	const FVector Location = Head + Facing.Vector() * PanelDistance + (bHeadLocked ? FVector::ZeroVector : FVector(0.f, 0.f, -10.f));
	// A widget component shows its face along +X, so turn it back toward the eyes (the reverse of Facing).
	UIPanel->SetWorldLocationAndRotation(Location, FRotator(-GLTTPanelPitch, PanelYaw + 180.f, 0.f));

	const bool bUseLayer = CVarLTTVRUILayer.GetValueOnGameThread() != 0;
	if (UIPanel->bRenderInMainPass == bUseLayer)
	{
		UIPanel->SetRenderInMainPass(!bUseLayer);
		UIPanel->SetRenderInDepthPass(!bUseLayer);
	}
	if (UStereoLayerComponent* Layer = LTTFindUILayer(GetOwner()))
	{
		// A quad layer faces back along -X, toward a viewer looking down +X.
		Layer->SetWorldLocationAndRotation(Location, Facing);
		UTextureRenderTarget2D* Target = UIPanel->GetRenderTarget();
		if (Target && Layer->GetTexture() != Target)
		{
			Layer->SetTexture(Target);
		}
		Layer->SetVisibility(bUseLayer && !UIPanel->bHiddenInGame && Target != nullptr);
	}
}

void ULTTVRComponent::UpdatePointer()
{
	if (!Pointer || !RightAim || !UIPanel)
	{
		return;
	}

	FHitResult Hit;
	bool bOnPanel = false;
	bool bFromController = false;
	const bool bGazeHeld = GLTTGazeClicker.Get() == this;
	if (!UIPanel->bHiddenInGame)
	{
		// The right controller first; without one (keyboard and mouse, XR Simulator) the head's gaze aims instead.
		// A click that started on the gaze keeps the gaze until it's released.
		if (!bGazeHeld && RightAim->IsTracked() && LTTTracePanel(UIPanel, RightAim->GetComponentLocation(), RightAim->GetForwardVector(), Hit))
		{
			bOnPanel = bFromController = true;
		}
		else if (const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>())
		{
			bOnPanel = LTTTracePanel(UIPanel, Camera->GetComponentLocation(), Camera->GetForwardVector(), Hit);
		}
	}
	Pointer->SetCustomHitResult(Hit);

	// Dots drawn on the panel itself (always visible, even in a dark set): the pointer, and the head's gaze while the
	// controller has the pointer. A click that misses a button with the controller falls back to the gaze.
	FHitResult GazeHit;
	bool bGazeOnPanel = false;
	if (bFromController && !UIPanel->bHiddenInGame)
	{
		if (const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>())
		{
			bGazeOnPanel = LTTTracePanel(UIPanel, Camera->GetComponentLocation(), Camera->GetForwardVector(), GazeHit);
		}
	}
	auto PlaceDot = [this](FName Name, bool bShow, const FVector& WorldPoint, float Size)
	{
		UImage* Dot = (UIHost && UIHost->WidgetTree) ? Cast<UImage>(UIHost->WidgetTree->FindWidget(Name)) : nullptr;
		if (!Dot)
		{
			return;
		}
		Dot->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bShow)
		{
			return;
		}
		FVector2D Pixel;
		UIPanel->GetLocalHitLocation(WorldPoint, Pixel);
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Dot->Slot))
		{
			Slot->SetSize(FVector2D(Size));
			Slot->SetPosition(Pixel - FVector2D(Size * 0.5f));
		}
	};
	// Only while a menu that takes clicks is open (briefing, pause, end screen...): during play the HUD has nothing to
	// click, and a dot in the middle of the view reads as a first-person crosshair.
	bool bMenuOpen = false;
	for (const TPair<TObjectPtr<UUserWidget>, int32>& Pair : CapturedZOrder)
	{
		if (Pair.Key && Pair.Key->IsFocusable() && Pair.Key->IsVisible())
		{
			bMenuOpen = true;
			break;
		}
	}
	const bool bOverButton = Pointer->IsOverInteractableWidget();
	PlaceDot(TEXT("VR_DotPointer"), bOnPanel && (bMenuOpen || bOverButton), Hit.ImpactPoint, bOverButton ? 40.f : 26.f);
	PlaceDot(TEXT("VR_DotGaze"), bGazeOnPanel && bMenuOpen, GazeHit.ImpactPoint, 18.f);

	// Laser whenever the controller points at the panel.
	const bool bShowLaser = bOnPanel && bFromController && (bMenuOpen || bOverButton);
	if (Laser)
	{
		Laser->SetHiddenInGame(!bShowLaser);
		if (bShowLaser)
		{
			// The basic cylinder is 100 cm tall along Z, centred: lay it along +X and stretch it to the hit.
			const float Length = Hit.Distance;
			Laser->SetRelativeLocationAndRotation(FVector(Length * 0.5f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f));
			Laser->SetRelativeScale3D(FVector(0.005f, 0.005f, Length / 100.f));
		}
	}
}
