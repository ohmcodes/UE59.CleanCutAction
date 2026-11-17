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
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "UI/LTTUITheme.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Interaction/LTTCleanupHUD.h"
#include "IHeadMountedDisplay.h"
#include "IHandTracker.h"
#include "Features/IModularFeatures.h"
#include "Components/PoseableMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedPlayerInput.h"
#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractionComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "HAL/PlatformTime.h"
#include "Components/LocalLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "UObject/UObjectIterator.h"
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

/** Gameplay actions the hand gestures and the wrist menu fire (the same assets IMC_VR maps to the controllers). */
static const TCHAR* LTTVRMoveActionPath = TEXT("/Game/Input/Actions/IA_Move.IA_Move");
static const TCHAR* LTTVRJumpActionPath = TEXT("/Game/Input/Actions/IA_Jump.IA_Jump");
static const TCHAR* LTTVRInteractActionPath = TEXT("/Game/_CleanCutAction/Input/IA_Interact.IA_Interact");
static const TCHAR* LTTVRThrowActionPath = TEXT("/Game/_CleanCutAction/Input/IA_Throw.IA_Throw");
static const TCHAR* LTTVREmoteActionPath = TEXT("/Game/_CleanCutAction/Input/IA_Emote.IA_Emote");
static const TCHAR* LTTVRHintActionPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Hint.IA_VR_Hint");
static const TCHAR* LTTVRPauseActionPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Pause.IA_VR_Pause");

/** Controller finger poses for the template hand meshes (IMC_VR; values read every frame). */
static const TCHAR* LTTVRGripLPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandGrip_L.IA_VR_HandGrip_L");
static const TCHAR* LTTVRGripRPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandGrip_R.IA_VR_HandGrip_R");
static const TCHAR* LTTVRTriggerLPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandTrigger_L.IA_VR_HandTrigger_L");
static const TCHAR* LTTVRTriggerRPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandTrigger_R.IA_VR_HandTrigger_R");
static const TCHAR* LTTVRIndexTouchLPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandIndexTouch_L.IA_VR_HandIndexTouch_L");
static const TCHAR* LTTVRIndexTouchRPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandIndexTouch_R.IA_VR_HandIndexTouch_R");
static const TCHAR* LTTVRThumbTouchLPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandThumbTouch_L.IA_VR_HandThumbTouch_L");
static const TCHAR* LTTVRThumbTouchRPath = TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_HandThumbTouch_R.IA_VR_HandThumbTouch_R");

/** VR Template hands (XRMannequins) and the unlit glow for hands, laser and dots (all in MapsToCook). */
static const TCHAR* LTTVRHandLeftMeshPath = TEXT("/Game/XRMannequins/Meshes/SKM_MannyXR_left.SKM_MannyXR_left");
static const TCHAR* LTTVRHandRightMeshPath = TEXT("/Game/XRMannequins/Meshes/SKM_MannyXR_right.SKM_MannyXR_right");
static const TCHAR* LTTVRHandComponentClassPath = TEXT("/Game/XRMannequins/BP_MannequinsXR.BP_MannequinsXR_C");
static const TCHAR* LTTVRGlowMaterialPath = TEXT("/Game/_CleanCutAction/Materials/VR/M_VRGlow.M_VRGlow");

const FVector2D ULTTVRHandMenu::DrawSize(1040.f, 560.f);

int32 ULTTVRComponent::ActiveCount = 0;
int32 ULTTVRComponent::HandsCount = 0;

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
		TEXT("ltt.VR.UIScreenFill"), 0.62f,
		TEXT("VR UI panel width as a share of the headset's horizontal field of view (0.5 - 1)."));

	/** 1: the UI panel is glued to the view like a screen. 0: it lazily follows the head (world-space menu). */
	TAutoConsoleVariable<int32> CVarLTTVRUIHeadLocked(
		TEXT("ltt.VR.UIHeadLocked"), 1,
		TEXT("VR UI panel: 1 = head-locked (moves with the view), 0 = lazy follow (stays in the world, re-centres when you look away)."));

	/** Render resolution per eye in VR, percent of the headset's (read when VR starts). */
	TAutoConsoleVariable<float> CVarLTTVRScreenPercentage(
		TEXT("ltt.VR.ScreenPercentage"), 100.f,
		TEXT("VR render resolution per eye, percent of the headset's recommended size (50 - 150). Applied when VR starts."));

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
		const FVector2D Half = Panel->GetDrawSize() * 0.5f;
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

	// ---- Quest render budget (see Docs/10_VR_Meta_Quest.md section 9.6) ----
	// The cafe has 37 movable local lights, and on Quest's mobile forward renderer each one adds per-pixel work to
	// everything it reaches. Measured on a Quest 2 before this: 10 - 18 fps, GPU 98 %. Applied when VR starts on Android
	// (or anywhere with ltt.Quest.Budget 2), and again by the console command ltt.Quest.ApplyBudget after a change.
	TAutoConsoleVariable<int32> CVarLTTQuestBudget(TEXT("ltt.Quest.Budget"),
		1, TEXT("Quest render budget: 0 = off, 1 = on Android only (default), 2 = everywhere (to preview it on PC)."));
	TAutoConsoleVariable<float> CVarLTTQuestLightRadius(TEXT("ltt.Quest.MaxLightRadius"),
		450.f, TEXT("Quest: largest attenuation radius of a point / spot / rect light, cm (0 = leave them)."));
	TAutoConsoleVariable<int32> CVarLTTQuestSunShadows(TEXT("ltt.Quest.SunShadows"),
		0, TEXT("Quest: 1 = keep the sun's dynamic shadow, 0 = no sun shadow (saves its shadow depth pass)."));
	TAutoConsoleVariable<int32> CVarLTTQuestRectLights(TEXT("ltt.Quest.RectLights"),
		1, TEXT("Quest: 1 = keep rect lights, 0 = take them out of the renderer (the costliest local light type on mobile)."));

	bool LTTQuestBudgetActive()
	{
		const int32 Mode = CVarLTTQuestBudget.GetValueOnGameThread();
		return Mode >= 2 || (Mode == 1 && PLATFORM_ANDROID);
	}

	void LTTApplyQuestRenderBudget(UWorld* World)
	{
		if (!World || !LTTQuestBudgetActive())
		{
			return;
		}
		const float MaxRadius = CVarLTTQuestLightRadius.GetValueOnGameThread();
		const bool bSunShadows = CVarLTTQuestSunShadows.GetValueOnGameThread() != 0;
		const bool bRectLights = CVarLTTQuestRectLights.GetValueOnGameThread() != 0;
		int32 Local = 0;
		int32 Rect = 0;
		for (TObjectIterator<ULightComponent> It; It; ++It)
		{
			ULightComponent* Light = *It;
			if (!Light || Light->IsTemplate() || Light->GetWorld() != World)
			{
				continue;
			}
			if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(Light))
			{
				Sun->SetCastShadows(bSunShadows);
				continue;
			}
			ULocalLightComponent* LocalLight = Cast<ULocalLightComponent>(Light);
			if (!LocalLight)
			{
				continue;
			}
			// Light switches and the power box toggle visibility, so these per-light settings stay in force.
			LocalLight->SetCastShadows(false);
			LocalLight->SetAffectTranslucentLighting(false);
			LocalLight->SetVolumetricScatteringIntensity(0.f);
			if (MaxRadius > 0.f && LocalLight->AttenuationRadius > MaxRadius)
			{
				LocalLight->SetAttenuationRadius(MaxRadius);
			}
			if (Cast<URectLightComponent>(LocalLight))
			{
				++Rect;
				if (!bRectLights)
				{
					// Out of the renderer entirely (visibility toggles from switches don't bring it back).
					LocalLight->bAffectsWorld = false;
					LocalLight->MarkRenderStateDirty();
				}
			}
			++Local;
		}
		UE_LOG(LogCleanCutAction, Log, TEXT("VR: Quest render budget: %d local lights (%d rect%s), radius <= %.0f, sun shadow %s"),
			Local, Rect, bRectLights ? TEXT("") : TEXT(", off"), MaxRadius, bSunShadows ? TEXT("on") : TEXT("off"));
	}

	FAutoConsoleCommandWithWorld GLTTQuestApplyBudgetCmd(TEXT("ltt.Quest.ApplyBudget"),
		TEXT("Re-applies the Quest render budget (ltt.Quest.*) to the current level."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&LTTApplyQuestRenderBudget));

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

void ULTTVRHandMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HandMenuPanel"));
	LTTTheme::StylePanel(Panel, 22.f);
	Panel->SetPadding(FMargin(18.f, 14.f));
	WidgetTree->RootWidget = Panel;

	// Left column: the score (moved here from the top-left HUD while playing with hands) and every gesture.
	// Right column: buttons for the actions.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HandMenuColumns"));
	Panel->SetContent(Columns);
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HandMenuInfo"));
	if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(Info))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}
	auto AddText = [this](UVerticalBox* Box, FName Name, const FText& Value, int32 Size, const FLinearColor& Color, float Gap = 2.f) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		LTTTheme::StyleText(Text, Size, Color);
		Text->SetText(Value);
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Text))
		{
			S->SetPadding(FMargin(0.f, Gap, 0.f, 0.f));
		}
		return Text;
	};
	UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HandMenuStats"));
	Info->AddChildToVerticalBox(Stats);
	AddText(Stats, TEXT("HandStatPlaced"), FText::GetEmpty(), 30, LTTTheme::Ink(), 0.f);
	AddText(Stats, TEXT("HandStatDestroyed"), FText::GetEmpty(), 22, LTTTheme::Bad());
	AddText(Stats, TEXT("HandStatPoints"), FText::GetEmpty(), 22, LTTTheme::Coin());
	AddText(Stats, TEXT("HandStatHint"), FText::GetEmpty(), 20, LTTTheme::HeaderBrown());

	const FLinearColor Head = LTTTheme::HeaderBrown();
	const FLinearColor Body = LTTTheme::Ink();
	// Controllers: the button list, shown instead of the hand gestures while no hand is tracked.
	UVerticalBox* PadLegend = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HandMenuPadLegend"));
	Info->AddChildToVerticalBox(PadLegend);
	AddText(PadLegend, NAME_None, NSLOCTEXT("LTTVR", "PadLegendHead", "CONTROLLERS"), 20, Head, 12.f);
	AddText(PadLegend, NAME_None, NSLOCTEXT("LTTVR", "PadLegend0", "L stick: move   R stick: turn"), 18, Body);
	AddText(PadLegend, NAME_None, NSLOCTEXT("LTTVR", "PadLegend1", "Trigger: pick up / place, press buttons"), 18, Body);
	AddText(PadLegend, NAME_None, NSLOCTEXT("LTTVR", "PadLegend2", "A: jump   B: throw   Y: dance"), 18, Body);
	AddText(PadLegend, NAME_None, NSLOCTEXT("LTTVR", "PadLegend3", "X: this menu   Menu: pause"), 18, Body);
	UVerticalBox* HandLegend = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HandMenuHandLegend"));
	Info->AddChildToVerticalBox(HandLegend);
	Info = HandLegend; // the hand gesture lines below go into their own group
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendLeft", "LEFT HAND"), 20, Head, 12.f);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendL0", "Index pinch (hold): walk where you look"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendL1", "Middle pinch: turn left"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendL2", "Ring pinch: jump"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendL3", "Pinky pinch: hint"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendRight", "RIGHT HAND"), 20, Head, 10.f);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendR0", "Index pinch: pick up / place, press buttons"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendR1", "Middle pinch: turn right"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendR2", "Flick forward: throw (or ring pinch)"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendR3", "Pinky pinch: dance   Scrub: mop"), 18, Body);
	AddText(Info, NAME_None, NSLOCTEXT("LTTVR", "HandLegendMore", "Left palm: this menu   Right palm (hold): pause"), 16, LTTTheme::InkMuted(), 10.f);

	USizeBox* GridBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	GridBox->SetWidthOverride(380.f);
	if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(GridBox))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		S->SetVerticalAlignment(VAlign_Fill);
	}
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("HandMenuGrid"));
	Grid->SetSlotPadding(FMargin(6.f));
	GridBox->SetContent(Grid);

	auto AddButton = [this, Grid](const TCHAR* Name, const FText& Label, int32 Row, int32 Col, const FLinearColor& Fill) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		LTTTheme::StyleButton(Button, Fill);
		// Pointer clicks only; never takes focus from the menus on the main panel. NOTE(5.9): no SetIsFocusable; the
		// protected init setter has to run before the Slate widget exists (it does: the tree is being built here).
		struct FLTTButtonAccess : public UButton
		{
			using UButton::InitIsFocusable;
		};
		(Button->*(&FLTTButtonAccess::InitIsFocusable))(false);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LTTTheme::StyleText(Text, 30, LTTTheme::Ink());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Button->SetContent(Text);
		if (UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Button, Row, Col))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
		return Button;
	};
	const FLinearColor Cream = LTTTheme::ButtonCream();
	AddButton(TEXT("Btn_Hint"), NSLOCTEXT("LTTVR", "HandHint", "Hint"), 0, 0, LTTTheme::ButtonGold())->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnHint);
	AddButton(TEXT("Btn_Pause"), NSLOCTEXT("LTTVR", "HandPause", "Pause"), 0, 1, LTTTheme::ButtonRed())->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnPause);
	AddButton(TEXT("Btn_Jump"), NSLOCTEXT("LTTVR", "HandJump", "Jump"), 1, 0, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnJump);
	AddButton(TEXT("Btn_Throw"), NSLOCTEXT("LTTVR", "HandThrow", "Throw"), 1, 1, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnThrow);
	AddButton(TEXT("Btn_TurnLeft"), NSLOCTEXT("LTTVR", "HandTurnLeft", "< Turn"), 2, 0, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnTurnLeft);
	AddButton(TEXT("Btn_TurnRight"), NSLOCTEXT("LTTVR", "HandTurnRight", "Turn >"), 2, 1, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnTurnRight);
	AddButton(TEXT("Btn_Dance"), NSLOCTEXT("LTTVR", "HandDance", "Dance"), 3, 0, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnDance);
	AddButton(TEXT("Btn_Recenter"), NSLOCTEXT("LTTVR", "HandRecenter", "Recenter"), 3, 1, Cream)->OnClicked.AddDynamic(this, &ULTTVRHandMenu::OnRecenter);
}

void ULTTVRHandMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Gesture list with hands, button list with controllers.
	const bool bHands = ULTTVRComponent::IsUsingHands();
	if (UWidget* HandLegend = GetWidgetFromName(TEXT("HandMenuHandLegend")))
	{
		HandLegend->SetVisibility(bHands ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (UWidget* PadLegend = GetWidgetFromName(TEXT("HandMenuPadLegend")))
	{
		PadLegend->SetVisibility(bHands ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	// The same numbers as the cleanup HUD's top-left panel (hidden while playing with hands). Phase 2 has no mess
	// director, so the block folds away there.
	const ALTTMessDirector* D = ALTTMessDirector::Get(this);
	if (UWidget* Stats = GetWidgetFromName(TEXT("HandMenuStats")))
	{
		Stats->SetVisibility(D ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!D)
	{
		return;
	}
	auto Set = [this](const TCHAR* Name, const FText& Value)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(Name)))
		{
			Text->SetText(Value);
		}
	};
	Set(TEXT("HandStatPlaced"), FText::Format(NSLOCTEXT("LTTVR", "HandPlaced", "{0}/{1} placed"), FText::AsNumber(D->GetPlacedCount()), FText::AsNumber(D->GetTotalCount())));
	Set(TEXT("HandStatDestroyed"), FText::Format(NSLOCTEXT("LTTVR", "HandDestroyed", "{0} destroyed"), FText::AsNumber(D->GetBrokenCount())));
	Set(TEXT("HandStatPoints"), FText::Format(NSLOCTEXT("LTTVR", "HandPoints", "{0} points"), FText::AsNumber(D->GetScore())));
	const float Cooldown = D->GetHintCooldownRemaining();
	Set(TEXT("HandStatHint"), Cooldown > 0.f
		? FText::Format(NSLOCTEXT("LTTVR", "HandHintCooldown", "Hint ready in {0}s"), FText::AsNumber(FMath::CeilToInt(Cooldown)))
		: NSLOCTEXT("LTTVR", "HandHintReady", "Hint ready"));
}

void ULTTVRHandMenu::Fire(ELTTVRHandAction Action)
{
	if (ULTTVRComponent* VR = OwnerVR.Get())
	{
		VR->DoHandAction(Action);
	}
}

void ULTTVRHandMenu::OnJump() { Fire(ELTTVRHandAction::Jump); }
void ULTTVRHandMenu::OnThrow() { Fire(ELTTVRHandAction::Throw); }
void ULTTVRHandMenu::OnHint() { Fire(ELTTVRHandAction::Hint); }
void ULTTVRHandMenu::OnDance() { Fire(ELTTVRHandAction::Dance); }
void ULTTVRHandMenu::OnTurnLeft() { Fire(ELTTVRHandAction::TurnLeft); }
void ULTTVRHandMenu::OnTurnRight() { Fire(ELTTVRHandAction::TurnRight); }
void ULTTVRHandMenu::OnRecenter() { Fire(ELTTVRHandAction::Recenter); }
void ULTTVRHandMenu::OnPause() { Fire(ELTTVRHandAction::Pause); }

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

bool ULTTVRComponent::IsUsingHands()
{
	return ActiveCount > 0 && HandsCount > 0;
}

bool ULTTVRComponent::ToggleControllerMenu()
{
	if (ActiveCount <= 0 || IsUsingHands())
	{
		return false;
	}
	for (TObjectIterator<ULTTVRComponent> It; It; ++It)
	{
		if (It->bVRActive && !It->IsTemplate())
		{
			It->bControllerMenuOpen = !It->bControllerMenuOpen;
			UE_LOG(LogCleanCutAction, Log, TEXT("VR: controller info menu %s"), It->bControllerMenuOpen ? TEXT("open") : TEXT("closed"));
			return true;
		}
	}
	return false;
}

bool ULTTVRComponent::IsPointerOnUI()
{
	if (ActiveCount <= 0)
	{
		return false;
	}
	for (TObjectIterator<ULTTVRComponent> It; It; ++It)
	{
		if (It->bVRActive && !It->IsTemplate() && It->Pointer && It->Pointer->IsOverInteractableWidget())
		{
			return true;
		}
	}
	return false;
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
	if (bHandsActive)
	{
		bHandsActive = false;
		HandsCount = FMath::Max(0, HandsCount - 1);
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
	UpdateHands(DeltaTime);
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
	LTTSetVRCVar(TEXT("r.ReflectionMethod"), TEXT("0"));
	// A post-process volume can switch Lumen back on per view; these turn it off whatever the volume says.
	LTTSetVRCVar(TEXT("r.Lumen.DiffuseIndirect.Allow"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.Lumen.Reflections.Allow"), TEXT("0"));
	// Virtual shadow maps are the next biggest cost in stereo: plain cascaded shadows, two cascades, smaller maps.
	LTTSetVRCVar(TEXT("r.Shadow.Virtual.Enable"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.Shadow.CSM.MaxCascades"), TEXT("2"));
	LTTSetVRCVar(TEXT("r.Shadow.MaxResolution"), TEXT("1024"));
	LTTSetVRCVar(TEXT("r.Shadow.DistanceScale"), TEXT("0.6"));
	LTTSetVRCVar(TEXT("r.DistanceFieldShadowing"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.DistanceFieldAO"), TEXT("0"));
	// Screen effects that are wasted (or uncomfortable) in a headset.
	LTTSetVRCVar(TEXT("r.VolumetricFog"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.SSR.Quality"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.AmbientOcclusionLevels"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.MotionBlurQuality"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.DepthOfFieldQuality"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.LensFlareQuality"), TEXT("0"));
	LTTSetVRCVar(TEXT("r.SceneColorFringeQuality"), TEXT("0"));
	// The street outside the cafe is mostly seen through windows: draw less of it far away.
	LTTSetVRCVar(TEXT("r.ViewDistanceScale"), TEXT("0.6"));
	// Quest: light radii, no light shadows, optional sun shadow / rect lights (ltt.Quest.*).
	LTTApplyQuestRenderBudget(GetWorld());
	// Optional per-eye resolution (ltt.VR.ScreenPercentage; 100 = the headset's own and leaves it alone). Below 100 the
	// engine recreates the headset swapchain at the smaller size, which left the Quest over Link showing nothing.
	const float ScreenPercentage = FMath::Clamp(CVarLTTVRScreenPercentage.GetValueOnGameThread(), 50.f, 150.f);
	if (!FMath::IsNearlyEqual(ScreenPercentage, 100.f))
	{
		LTTSetVRCVar(TEXT("r.ScreenPercentage"), *FString::SanitizeFloat(ScreenPercentage));
	}

	// Hand tracking needs the runtime to support it (Quest over Link: Developer Runtime Features on in the Link app).
	{
		bool bHandTracker = false;
		for (IHandTracker* Tracker : IModularFeatures::Get().GetModularFeatureImplementations<IHandTracker>(IHandTracker::GetModularFeatureName()))
		{
			bHandTracker |= Tracker && Tracker->IsHandTrackingStateValid();
		}
		UE_LOG(LogCleanCutAction, Log, TEXT("VR: runtime hand tracking %s"), bHandTracker
			? TEXT("available (put the controllers down to use hands)")
			: TEXT("NOT available (Quest over Link: Meta Horizon Link app > Settings > Beta > Developer Runtime Features on, and hand tracking on in the headset)"));
	}

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
		else if (Mesh)
		{
			// The world body is "owner no see", so it never counts as rendered for the player, and characters
			// default to refreshing bones only when rendered: mirrors (scene captures) showed it frozen, dance
			// included. Refresh the pose every frame instead.
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
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
#if PLATFORM_ANDROID
		// Quest: redrawing the 1920 x 1080 widget tree into its texture every frame costs game thread and GPU time.
		// 36 Hz (every other frame at 72 Hz) is plenty for menus and the HUD.
		UIPanel->SetRedrawTime(1.f / 36.f);
#endif
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
	if (!LeftHandDots && SphereMesh)
	{
		auto MakeHandDots = [this, Character](const TCHAR* Name)
		{
			UInstancedStaticMeshComponent* Dots = NewObject<UInstancedStaticMeshComponent>(Character, Name);
			Dots->SetupAttachment(Character->GetRootComponent());
			Dots->SetAbsolute(true, true, true); // instances are placed in world space
			Dots->SetStaticMesh(SphereMesh);
			Dots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Dots->SetCastShadow(false);
			Dots->SetTickableWhenPaused(true);
			Dots->SetHiddenInGame(true);
			Dots->RegisterComponent();
			Dots->SetWorldTransform(FTransform::Identity);
			for (int32 i = 0; i < EHandKeypointCount; ++i)
			{
				Dots->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.01f)), /*bWorldSpace*/ true);
			}
			return Dots;
		};
		LeftHandDots = MakeHandDots(TEXT("VR_LeftHandDots"));
		RightHandDots = MakeHandDots(TEXT("VR_RightHandDots"));
	}

	if (!HandMenuPanel)
	{
		HandMenu = CreateWidget<ULTTVRHandMenu>(PC, ULTTVRHandMenu::StaticClass());
		HandMenu->OwnerVR = this;
		HandMenuPanel = NewObject<UWidgetComponent>(Character, TEXT("VR_HandMenu"));
		HandMenuPanel->SetupAttachment(Character->GetRootComponent());
		HandMenuPanel->SetAbsolute(true, true, true);
		HandMenuPanel->SetWidgetSpace(EWidgetSpace::World);
		HandMenuPanel->SetDrawSize(ULTTVRHandMenu::DrawSize);
		HandMenuPanel->SetPivot(FVector2D(0.5f, 0.5f));
		HandMenuPanel->SetBlendMode(EWidgetBlendMode::Transparent);
		HandMenuPanel->SetTwoSided(false);
		HandMenuPanel->SetTickWhenOffscreen(true);
		HandMenuPanel->SetTickableWhenPaused(true);
		HandMenuPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HandMenuPanel->SetCastShadow(false);
		HandMenuPanel->SetOwnerPlayer(PC->GetLocalPlayer());
		HandMenuPanel->SetWidget(HandMenu);
		HandMenuPanel->SetWorldScale3D(FVector(HandMenuWidth / ULTTVRHandMenu::DrawSize.X));
		if (UMaterialInterface* OnTop = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_CleanCutAction/Materials/VR/M_VRWidget_OnTop.M_VRWidget_OnTop")))
		{
			HandMenuPanel->SetMaterial(0, OnTop);
		}
		HandMenuPanel->SetTranslucentSortPriority(10001); // over the HUD panel
		HandMenuPanel->SetHiddenInGame(true);
		HandMenuPanel->RegisterComponent();
	}

	// Glow for the laser and the joint dots: unlit, so they show when the breaker kills the lights.
	if (!GlowMID)
	{
		if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, LTTVRGlowMaterialPath))
		{
			GlowMID = UMaterialInstanceDynamic::Create(Glow, this);
			LaserMID = UMaterialInstanceDynamic::Create(Glow, this);
			LaserMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.25f));
			LaserMID->SetScalarParameterValue(TEXT("Base"), 1.f);
			LaserMID->SetScalarParameterValue(TEXT("Opacity"), 0.85f);
			GlowMID->SetScalarParameterValue(TEXT("Base"), 0.35f);
			GlowMID->SetScalarParameterValue(TEXT("Opacity"), 0.55f);
		}
		else
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("VR: M_VRGlow not found, hands and laser are dark in a dark set"));
		}
	}
	if (Laser && LaserMID)
	{
		Laser->SetMaterial(0, LaserMID);
	}
	for (UInstancedStaticMeshComponent* Dots : { LeftHandDots.Get(), RightHandDots.Get() })
	{
		if (Dots && LaserMID)
		{
			Dots->SetMaterial(0, LaserMID);
		}
	}
	CreateHandMeshes(Character);

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
	for (UPrimitiveComponent* HandPart : TArray<UPrimitiveComponent*>{ LeftHandDots, RightHandDots, HandMenuPanel,
		LeftTrackedHand, RightTrackedHand, LeftControllerHand, RightControllerHand })
	{
		if (HandPart)
		{
			HandPart->SetHiddenInGame(true);
		}
	}
	bHandMenuShown = bRightPinchInteract = false;
	LeftPinchFinger = RightPinchFinger = -1;
	PausePalmTime = 0.f;
	bPausePalmFired = bMopAnchorValid = false;
	RightHandSamples.Reset();
	if (bHandsActive)
	{
		bHandsActive = false;
		HandsCount = FMath::Max(0, HandsCount - 1);
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
	SnapTurn(FMath::Sign(X));
}

void ULTTVRComponent::SnapTurn(float Sign)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
	{
		if (PC->IsPaused())
		{
			return;
		}
		FRotator Control = PC->GetControlRotation();
		Control.Yaw += Sign * SnapTurnDegrees;
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
	bool bOnHandMenu = false;
	bool bFromController = false;
	const bool bGazeHeld = GLTTGazeClicker.Get() == this;
	// The right hand or controller first (the nearest of the wrist menu and the UI panel); without either (keyboard and
	// mouse, XR Simulator) the head's gaze aims at the UI panel instead. A click that started on the gaze keeps the
	// gaze until it's released.
	FVector AimStart = FVector::ZeroVector;
	FVector AimDir = FVector::ForwardVector;
	if (!bGazeHeld && GetAimRay(AimStart, AimDir))
	{
		FHitResult MenuHit;
		if (HandMenuPanel && !HandMenuPanel->bHiddenInGame && LTTTracePanel(HandMenuPanel, AimStart, AimDir, MenuHit))
		{
			Hit = MenuHit;
			bOnHandMenu = bFromController = true;
		}
		FHitResult PanelHit;
		if (!UIPanel->bHiddenInGame && LTTTracePanel(UIPanel, AimStart, AimDir, PanelHit) && (!bOnHandMenu || PanelHit.Distance < Hit.Distance))
		{
			Hit = PanelHit;
			bOnHandMenu = false;
			bOnPanel = bFromController = true;
		}
	}
	if (!bFromController && !UIPanel->bHiddenInGame)
	{
		if (const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>())
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
	const bool bMenuOpen = IsMenuOpen();
	const bool bOverButton = Pointer->IsOverInteractableWidget();
	PlaceDot(TEXT("VR_DotPointer"), bOnPanel && !bOnHandMenu && (bMenuOpen || bOverButton), Hit.ImpactPoint, bOverButton ? 40.f : 26.f);
	PlaceDot(TEXT("VR_DotGaze"), bGazeOnPanel && bMenuOpen, GazeHit.ImpactPoint, 18.f);

	// Laser whenever the hand / controller points at a menu.
	const bool bShowLaser = bFromController && (bOnHandMenu || (bOnPanel && (bMenuOpen || bOverButton)));
	if (Laser)
	{
		Laser->SetHiddenInGame(!bShowLaser);
		if (bShowLaser)
		{
			// The basic cylinder is 100 cm tall along Z, centred: lay it along the ray and stretch it to the hit.
			const float Length = Hit.Distance;
			Laser->SetWorldLocationAndRotation(AimStart + AimDir * (Length * 0.5f), FRotationMatrix::MakeFromZ(AimDir).Rotator());
			Laser->SetWorldScale3D(FVector(0.005f, 0.005f, Length / 100.f));
		}
	}
}

namespace
{
	bool LTTHandTracked(const FXRHandTrackingState& Hand)
	{
		return Hand.bValid && Hand.TrackingStatus == ETrackingStatus::Tracked && Hand.HandKeyLocations.Num() == EHandKeypointCount
			&& Hand.HandKeyRotations.Num() == EHandKeypointCount;
	}

	const FVector& LTTJoint(const FXRHandTrackingState& Hand, EHandKeypoint Joint)
	{
		return Hand.HandKeyLocations[static_cast<int32>(Joint)];
	}

	/** Joint radius in cm, or a typical one when the runtime doesn't report radii. */
	float LTTJointRadius(const FXRHandTrackingState& Hand, EHandKeypoint Joint)
	{
		const int32 Index = static_cast<int32>(Joint);
		const float Radius = Hand.HandKeyRadii.IsValidIndex(Index) ? Hand.HandKeyRadii[Index] : 0.f;
		return Radius > 0.1f ? FMath::Clamp(Radius, 0.4f, 1.5f) : 0.8f;
	}

	/** Fingertips that pinch against the thumb: 0 index, 1 middle, 2 ring, 3 little. */
	const EHandKeypoint LTTFingerTips[4] = { EHandKeypoint::IndexTip, EHandKeypoint::MiddleTip, EHandKeypoint::RingTip, EHandKeypoint::LittleTip };

	/** Gap between the thumb tip and a fingertip's surfaces, cm. */
	float LTTPinchGap(const FXRHandTrackingState& Hand, int32 Finger)
	{
		const EHandKeypoint Tip = LTTFingerTips[FMath::Clamp(Finger, 0, 3)];
		return FVector::Dist(LTTJoint(Hand, EHandKeypoint::ThumbTip), LTTJoint(Hand, Tip))
			- LTTJointRadius(Hand, EHandKeypoint::ThumbTip) - LTTJointRadius(Hand, Tip);
	}

	/** Out of the palm (OpenXR joints: +Y is the back of the hand, which is +Z in Unreal). */
	FVector LTTPalmNormal(const FXRHandTrackingState& Hand)
	{
		return -Hand.HandKeyRotations[static_cast<int32>(EHandKeypoint::Palm)].GetUpVector();
	}
}

bool ULTTVRComponent::IsMenuOpen() const
{
	for (const TPair<TObjectPtr<UUserWidget>, int32>& Pair : CapturedZOrder)
	{
		if (Pair.Key && Pair.Key->IsFocusable() && Pair.Key->IsVisible())
		{
			return true;
		}
	}
	return false;
}

bool ULTTVRComponent::GetAimRay(FVector& OutStart, FVector& OutDir) const
{
	if (bHandAimValid)
	{
		OutStart = HandAimStart;
		OutDir = HandAimDir;
		return true;
	}
	if (RightAim && RightAim->IsTracked())
	{
		OutStart = RightAim->GetComponentLocation();
		OutDir = RightAim->GetForwardVector();
		return true;
	}
	return false;
}

void ULTTVRComponent::InjectAction(const TCHAR* Path, const FInputActionValue& Value)
{
	TObjectPtr<UInputAction>& Action = InjectedActions.FindOrAdd(Path);
	if (!Action)
	{
		Action = LoadObject<UInputAction>(nullptr, Path);
		if (!Action)
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("VR: hand input action not found: %s"), Path);
			return;
		}
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Input = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr)
	{
		Input->InjectInputForAction(Action, Value, TArray<UInputModifier*>(), TArray<UInputTrigger*>());
	}
}

void ULTTVRComponent::DoHandAction(ELTTVRHandAction Action)
{
	if (!bVRActive)
	{
		return;
	}
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: wrist menu action %d"), static_cast<int32>(Action));
	switch (Action)
	{
	case ELTTVRHandAction::Jump:      InjectAction(LTTVRJumpActionPath, FInputActionValue(true)); break;
	case ELTTVRHandAction::Throw:     InjectAction(LTTVRThrowActionPath, FInputActionValue(true)); break;
	case ELTTVRHandAction::Hint:      InjectAction(LTTVRHintActionPath, FInputActionValue(true)); break;
	case ELTTVRHandAction::Dance:     InjectAction(LTTVREmoteActionPath, FInputActionValue(true)); break;
	case ELTTVRHandAction::Pause:     InjectAction(LTTVRPauseActionPath, FInputActionValue(true)); break;
	case ELTTVRHandAction::TurnLeft:  SnapTurn(-1.f); break;
	case ELTTVRHandAction::TurnRight: SnapTurn(1.f); break;
	case ELTTVRHandAction::Recenter:
		if (GEngine && GEngine->XRSystem.IsValid())
		{
			GEngine->XRSystem->ResetOrientationAndPosition(0.f);
		}
		bPanelPlaced = false;
		break;
	}
}

void ULTTVRComponent::UpdateHands(float DeltaTime)
{
	FXRHandTrackingState Left;
	FXRHandTrackingState Right;
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->GetHandTrackingState(this, EXRSpaceType::UnrealWorldSpace, EControllerHand::Left, Left);
		GEngine->XRSystem->GetHandTrackingState(this, EXRSpaceType::UnrealWorldSpace, EControllerHand::Right, Right);
	}
	const bool bLeftOK = LTTHandTracked(Left);
	const bool bRightOK = LTTHandTracked(Right);

	// Hands mode (HUD shows gestures) while a hand is tracked, and for a moment after: hands drop out of view a lot.
	HandsLostTime = (bLeftOK || bRightOK) ? 0.f : HandsLostTime + DeltaTime;
	const bool bWantHands = bLeftOK || bRightOK || (bHandsActive && HandsLostTime < 1.5f);
	if (bWantHands != bHandsActive)
	{
		bHandsActive = bWantHands;
		HandsCount = FMath::Max(0, HandsCount + (bWantHands ? 1 : -1));
		UE_LOG(LogCleanCutAction, Log, TEXT("VR: %s"), bWantHands ? TEXT("hand tracking active") : TEXT("hand tracking lost, controllers"));
	}

	// Pinch: the thumb tip touches one fingertip (the closest one, so a middle pinch isn't read as an index pinch).
	// Released again a little further apart, so it doesn't flicker. Out: the finger released and the finger pressed
	// this frame (-1 for none).
	auto UpdatePinch = [this](bool bTracked, const FXRHandTrackingState& Hand, int32& Finger, int32& OutReleased, int32& OutPressed)
	{
		OutReleased = OutPressed = -1;
		if (!bTracked)
		{
			OutReleased = Finger;
			Finger = -1;
			return;
		}
		if (Finger >= 0 && LTTPinchGap(Hand, Finger) >= PinchDistance * 2.f)
		{
			OutReleased = Finger;
			Finger = -1;
		}
		if (Finger < 0)
		{
			int32 Best = -1;
			float BestGap = PinchDistance;
			for (int32 i = 0; i < 4; ++i)
			{
				const float Gap = LTTPinchGap(Hand, i);
				if (Gap < BestGap)
				{
					BestGap = Gap;
					Best = i;
				}
			}
			if (Best >= 0 && Best != OutReleased)
			{
				Finger = OutPressed = Best;
			}
		}
	};
	int32 LeftReleased, LeftPressed, RightReleased, RightPressed;
	UpdatePinch(bLeftOK, Left, LeftPinchFinger, LeftReleased, LeftPressed);
	UpdatePinch(bRightOK, Right, RightPinchFinger, RightReleased, RightPressed);

	// Template hand meshes follow the joints; the joint spheres are the fallback when the meshes didn't load.
	UpdateHandMeshes(Left, Right, bLeftOK, bRightOK);
	auto UpdateDots = [](UInstancedStaticMeshComponent* Dots, const FXRHandTrackingState& Hand, bool bTracked, int32 Finger)
	{
		if (!Dots)
		{
			return;
		}
		Dots->SetHiddenInGame(!bTracked);
		if (!bTracked || Dots->GetInstanceCount() != EHandKeypointCount)
		{
			return;
		}
		for (int32 i = 0; i < EHandKeypointCount; ++i)
		{
			const EHandKeypoint Joint = static_cast<EHandKeypoint>(i);
			float Diameter = LTTJointRadius(Hand, Joint) * 2.f;
			if (Joint == EHandKeypoint::Palm)
			{
				Diameter = 0.f; // the palm point floats inside the hand
			}
			else if (Finger >= 0 && (Joint == EHandKeypoint::ThumbTip || Joint == LTTFingerTips[Finger]))
			{
				Diameter *= 1.6f;
			}
			Dots->UpdateInstanceTransform(i, FTransform(FQuat::Identity, LTTJoint(Hand, Joint), FVector(Diameter / 100.f)),
				/*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ i == EHandKeypointCount - 1, /*bTeleport*/ true);
		}
	};
	UpdateDots(LeftHandDots, Left, bLeftOK && !LeftTrackedHand, LeftPinchFinger);
	UpdateDots(RightHandDots, Right, bRightOK && !RightTrackedHand, RightPinchFinger);

	const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}
	const FVector Head = Camera->GetComponentLocation();

	// Right hand ray, like the Quest system pointer: from an estimated shoulder through the knuckles, smoothed. The
	// knuckles barely move during a pinch, so clicking doesn't knock the ray off the button.
	if (bRightOK)
	{
		const FVector Knuckles = (LTTJoint(Right, EHandKeypoint::IndexProximal) + LTTJoint(Right, EHandKeypoint::ThumbProximal)) * 0.5f;
		const FRotator HeadYaw(0.f, Camera->GetComponentRotation().Yaw, 0.f);
		const FVector Shoulder = Head + FRotationMatrix(HeadYaw).GetUnitAxis(EAxis::Y) * 18.f - FVector(0.f, 0.f, 20.f);
		const FVector Dir = (Knuckles - Shoulder).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			const float Alpha = bHandAimValid ? 1.f - FMath::Exp(-DeltaTime * 18.f) : 1.f;
			HandAimDir = (HandAimDir + (Dir - HandAimDir) * Alpha).GetSafeNormal();
			HandAimStart = Knuckles;
			bHandAimValid = !HandAimDir.IsNearlyZero();
		}
	}
	else
	{
		bHandAimValid = false;
	}

	UpdateHandMenu(Left, Head);
	const bool bRightPalmFacing = UpdatePausePalm(Right, bRightOK, Head, DeltaTime);
	UpdateHandMotion(Right, bRightOK, bRightPalmFacing, DeltaTime);

	if (RightReleased >= 0)
	{
		HandlePinch(true, RightReleased, false);
	}
	// A right pinch with the palm toward the face is the headset's own gesture (Meta button), not ours.
	if (RightPressed >= 0 && !bRightPalmFacing)
	{
		HandlePinch(true, RightPressed, true);
	}
	if (LeftReleased >= 0)
	{
		HandlePinch(false, LeftReleased, false);
	}
	if (LeftPressed >= 0)
	{
		HandlePinch(false, LeftPressed, true);
	}

	// Left index pinch held = walk where the head looks (the character moves head-relative in VR), at a walking pace.
	// Not while the left palm shows the wrist menu (that pinch is the headset's own menu gesture).
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const bool bPaused = PC && PC->IsPaused();
	if (LeftPinchFinger == 0 && !bHandMenuShown && !IsMenuOpen() && !bPaused)
	{
		InjectAction(LTTVRMoveActionPath, FInputActionValue(FVector2D(0.f, FMath::Clamp(PinchWalkSpeed, 0.1f, 1.f))));
	}
}

void ULTTVRComponent::HandlePinch(bool bRightHand, int32 Finger, bool bPressed)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const bool bPaused = PC && PC->IsPaused();
	const bool bPlaying = !IsMenuOpen() && !bPaused;

	if (bRightHand)
	{
		switch (Finger)
		{
		case 0:
			// Index = the controller trigger: clicks the menu button under the ray (or the gaze), and otherwise picks
			// up / places what the player looks at.
			if (bPressed)
			{
				bRightPinchInteract = !(Pointer && Pointer->IsOverInteractableWidget()) && bPlaying;
				HandleUIClickPressed();
				if (bRightPinchInteract)
				{
					InjectAction(LTTVRInteractActionPath, FInputActionValue(true));
				}
			}
			else
			{
				HandleUIClickReleased();
				bRightPinchInteract = false;
			}
			break;
		case 1:
			if (bPressed && bPlaying)
			{
				DoHandAction(ELTTVRHandAction::TurnRight);
			}
			break;
		case 2:
			// Button throw, for when the real throw (a flick of the hand, UpdateHandMotion) isn't wanted.
			if (bPressed && bPlaying)
			{
				DoHandAction(ELTTVRHandAction::Throw);
			}
			break;
		case 3:
			// Fires on the pinch; holding it longer is left to the headset.
			if (bPressed && bPlaying)
			{
				DoHandAction(ELTTVRHandAction::Dance);
			}
			break;
		default:
			break;
		}
		if (Finger != 0)
		{
			HandThrowBlockedUntil = FPlatformTime::Seconds() + HandThrowGrace;
		}
		return;
	}

	// Left hand. Nothing while its palm faces the player: that's the wrist menu and the headset's menu gesture.
	if (!bPressed || bHandMenuShown || !bPlaying)
	{
		return;
	}
	switch (Finger)
	{
	case 1: DoHandAction(ELTTVRHandAction::TurnLeft); break;
	case 2: DoHandAction(ELTTVRHandAction::Jump); break;
	case 3: DoHandAction(ELTTVRHandAction::Hint); break;
	default: break; // index: walking, handled every frame in UpdateHands
	}
}

bool ULTTVRComponent::UpdatePausePalm(const FXRHandTrackingState& Right, bool bRightOK, const FVector& Head, float DeltaTime)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const bool bPaused = PC && PC->IsPaused();

	float Facing = -1.f;
	if (bRightOK)
	{
		const FVector Palm = LTTJoint(Right, EHandKeypoint::Palm);
		Facing = FVector::DotProduct(LTTPalmNormal(Right), (Head - Palm).GetSafeNormal());
	}
	const bool bFacing = Facing > 0.7f;
	// Re-arms once the palm turns well away, so holding it there doesn't toggle the menu again.
	if (Facing < 0.3f)
	{
		bPausePalmFired = false;
	}
	// Only from play: once paused, the menu's own Resume button closes it. Not while the right hand works the
	// wrist menu or pinches (palm toward you + index pinch is the headset's own Meta button).
	if (bFacing && !bPausePalmFired && !bPaused && !IsMenuOpen() && !bHandMenuShown && RightPinchFinger < 0)
	{
		PausePalmTime += DeltaTime;
		if (PausePalmTime >= PausePalmHoldTime)
		{
			bPausePalmFired = true;
			PausePalmTime = 0.f;
			UE_LOG(LogCleanCutAction, Log, TEXT("VR: right palm held, pause menu"));
			DoHandAction(ELTTVRHandAction::Pause);
		}
	}
	else
	{
		PausePalmTime = 0.f;
	}
	return Facing > 0.5f;
}

void ULTTVRComponent::UpdateHandMotion(const FXRHandTrackingState& Right, bool bRightOK, bool bPalmFacing, float DeltaTime)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	ULTTCarryComponent* Carry = Character ? Character->FindComponentByClass<ULTTCarryComponent>() : nullptr;
	ULTTInteractionComponent* Interaction = Character ? Character->FindComponentByClass<ULTTInteractionComponent>() : nullptr;
	ALTTInteractableBase* Held = Carry && Carry->IsHolding() ? Carry->GetHeldItem() : nullptr;
	const double Now = FPlatformTime::Seconds();

	// A new prop in the hand: the pickup pinch moved the hand, so wait a moment before reading a throw.
	if (Held != LastHeldItem.Get())
	{
		LastHeldItem = Held;
		HandThrowBlockedUntil = Now + HandThrowGrace;
		bMopAnchorValid = false;
	}

	const bool bPlaying = PC && !PC->IsPaused() && !IsMenuOpen() && !bHandMenuShown;
	if (!bRightOK || !VROrigin || !Held || !bPlaying)
	{
		RightHandSamples.Reset();
		bMopAnchorValid = false;
		return;
	}

	// Palm centre in VR origin space: walking and snap turns move the origin, not the hand inside it.
	const FTransform Origin = VROrigin->GetComponentTransform();
	const FVector Local = Origin.InverseTransformPositionNoScale(LTTJoint(Right, EHandKeypoint::Palm));
	RightHandSamples.Add({ Now, Local });
	while (RightHandSamples.Num() > 2 && Now - RightHandSamples[0].Time > 0.2)
	{
		RightHandSamples.RemoveAt(0, EAllowShrinking::No);
	}

	// ---- Mop: scrubbing over a puddle. Every MopStrokeDistance of horizontal travel is one stroke; the puddle
	// itself ignores strokes while the last one is still playing, so scrubbing fast can't skip ahead. ----
	if (Held->bIsMop)
	{
		if (!bHandMop || !Interaction)
		{
			return;
		}
		if (!bMopAnchorValid)
		{
			MopAnchor = Local;
			bMopAnchorValid = true;
		}
		const FVector Travel = FVector(Local.X - MopAnchor.X, Local.Y - MopAnchor.Y, 0.f);
		if (Travel.Size() >= MopStrokeDistance && RightPinchFinger < 0)
		{
			if (Interaction->TryMopStroke())
			{
				MopAnchor = Local;
			}
		}
		return; // the mop is never flick-thrown (it swings while mopping)
	}

	// ---- Real throw: a fast flick forward. ----
	if (!bHandThrow || bPalmFacing || RightPinchFinger >= 0 || Now < HandThrowBlockedUntil || RightHandSamples.Num() < 3)
	{
		return;
	}
	// Speed over the last ~70 ms (one sample's jitter would read as a flick).
	const FLTTHandSample& Newest = RightHandSamples.Last();
	int32 From = RightHandSamples.Num() - 2;
	while (From > 0 && Newest.Time - RightHandSamples[From].Time < 0.07)
	{
		--From;
	}
	const double Span = Newest.Time - RightHandSamples[From].Time;
	if (Span < 0.03)
	{
		return;
	}
	const FVector LocalVelocity = (Newest.Pos - RightHandSamples[From].Pos) / Span;
	const FVector Velocity = Origin.GetRotation().RotateVector(LocalVelocity);
	const float Speed = Velocity.Size();
	if (Speed < HandThrowMinSpeed)
	{
		return;
	}
	// Forward, away from the body (a hand pulled back or dropped to the lap isn't a throw).
	const UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	const FVector Forward = Camera ? FRotator(0.f, Camera->GetComponentRotation().Yaw, 0.f).Vector() : Character->GetActorForwardVector();
	const FVector Dir = Velocity / Speed;
	if (FVector::DotProduct(Dir, Forward) < 0.35f || Dir.Z < -0.6f)
	{
		return;
	}
	const float OutSpeed = FMath::Clamp(Speed * HandThrowBoost * Held->GetEffectiveThrowMultiplier(), 300.f, HandThrowMaxSpeed);
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: hand throw, hand %.0f cm/s, prop %.0f cm/s"), Speed, OutSpeed);
	Carry->ThrowWithVelocity(Dir * OutSpeed);
	RightHandSamples.Reset();
	HandThrowBlockedUntil = Now + HandThrowGrace;
}

void ULTTVRComponent::UpdateHandMenu(const FXRHandTrackingState& Left, const FVector& Head)
{
	if (!HandMenuPanel)
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;

	bool bShow = false;
	FVector Palm = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	const bool bBlocked = IsMenuOpen() || (PC && PC->IsPaused());
	if (bBlocked || IsUsingHands())
	{
		bControllerMenuOpen = false; // a pause / end menu takes over; X opens it again afterwards
	}
	if (!LTTHandTracked(Left) && bControllerMenuOpen && LeftController && LeftController->IsTracked())
	{
		// Controllers: the info menu (X) floats just above the left controller, turned to the eyes.
		Palm = LeftController->GetComponentLocation();
		Normal = (Head - Palm).GetSafeNormal2D();
		bShow = true;
	}
	else if (LTTHandTracked(Left) && !bBlocked)
	{
		Palm = LTTJoint(Left, EHandKeypoint::Palm);
		Normal = LTTPalmNormal(Left);
		const float Facing = FVector::DotProduct(Normal, (Head - Palm).GetSafeNormal());
		// Opens when the palm turns to the face; stays while it roughly does, or while the right hand aims at it.
		const bool bAimedAt = bHandMenuShown && Pointer && Pointer->GetHoveredWidgetComponent() == HandMenuPanel;
		bShow = Facing > (bHandMenuShown ? 0.3f : 0.6f) || (bAimedAt && Facing > 0.f);
	}
	if (bShow != bHandMenuShown)
	{
		bHandMenuShown = bShow;
		HandMenuPanel->SetHiddenInGame(!bShow);
	}
	if (!bShow)
	{
		return;
	}
	// Just above the open hand, turned to the eyes (a widget component shows its face along +X).
	const float MenuHeight = HandMenuWidth * ULTTVRHandMenu::DrawSize.Y / ULTTVRHandMenu::DrawSize.X;
	const FVector Location = Palm + Normal * 4.f + FVector(0.f, 0.f, MenuHeight * 0.5f + 6.f);
	const FVector ToEyes = (Head - Location).GetSafeNormal();
	HandMenuPanel->SetWorldScale3D(FVector(HandMenuWidth / HandMenuPanel->GetDrawSize().X));
	HandMenuPanel->SetWorldLocationAndRotation(Location, FRotationMatrix::MakeFromXZ(ToEyes, FVector::UpVector).Rotator());
}

namespace
{
	/** Builds the bone list for a VR Template hand mesh: which OpenXR joints pose each bone, and the rotation offset
	 *  between a frame made of the bone's direction and the back of the hand, and the bone itself (reference pose).
	 *  Nothing here assumes the skeleton's bone axes, so any hand with these bone names works. */
	bool LTTBuildHandBones(const USkeletalMesh* Mesh, bool bRight, TArray<FLTTHandBone>& Out)
	{
		Out.Reset();
		if (!Mesh)
		{
			return false;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const TArray<FTransform>& Local = Ref.GetRefBonePose();
		TArray<FTransform> Component;
		Component.SetNum(Local.Num());
		for (int32 i = 0; i < Local.Num(); ++i)
		{
			const int32 Parent = Ref.GetParentIndex(i);
			Component[i] = Parent != INDEX_NONE ? Local[i] * Component[Parent] : Local[i];
		}
		const TCHAR* Side = bRight ? TEXT("_r") : TEXT("_l");
		auto Index = [&Ref, Side](const TCHAR* Base) { return Ref.FindBoneIndex(FName(*FString::Printf(TEXT("%s%s"), Base, Side))); };
		const int32 Hand = Index(TEXT("hand"));
		const int32 Index01 = Index(TEXT("index_01"));
		const int32 Pinky01 = Index(TEXT("pinky_01"));
		if (Hand == INDEX_NONE || Index01 == INDEX_NONE || Pinky01 == INDEX_NONE)
		{
			return false;
		}
		// Back of the hand in the reference pose: the knuckle plane's normal, flipped for the left hand.
		const FVector ToIndex = Component[Index01].GetLocation() - Component[Hand].GetLocation();
		const FVector ToPinky = Component[Pinky01].GetLocation() - Component[Hand].GetLocation();
		const FVector RefUp = FVector::CrossProduct(ToIndex, ToPinky).GetSafeNormal() * (bRight ? 1.f : -1.f);

		// Bone, joint at its start, joint it points at, the bone it points at in the mesh (none for the last bone of a
		// finger, which keeps its parent's direction), thumb twist from the whole hand.
		struct FRow { const TCHAR* Bone; EHandKeypoint Joint; EHandKeypoint Toward; const TCHAR* TowardBone; bool bHandUp; };
		const FRow Rows[] = {
			{ TEXT("hand"), EHandKeypoint::Wrist, EHandKeypoint::MiddleProximal, TEXT("middle_01"), false },
			{ TEXT("thumb_01"), EHandKeypoint::ThumbMetacarpal, EHandKeypoint::ThumbProximal, TEXT("thumb_02"), true },
			{ TEXT("thumb_02"), EHandKeypoint::ThumbProximal, EHandKeypoint::ThumbDistal, TEXT("thumb_03"), true },
			{ TEXT("thumb_03"), EHandKeypoint::ThumbDistal, EHandKeypoint::ThumbTip, nullptr, true },
			{ TEXT("index_metacarpal"), EHandKeypoint::IndexMetacarpal, EHandKeypoint::IndexProximal, TEXT("index_01"), false },
			{ TEXT("index_01"), EHandKeypoint::IndexProximal, EHandKeypoint::IndexIntermediate, TEXT("index_02"), false },
			{ TEXT("index_02"), EHandKeypoint::IndexIntermediate, EHandKeypoint::IndexDistal, TEXT("index_03"), false },
			{ TEXT("index_03"), EHandKeypoint::IndexDistal, EHandKeypoint::IndexTip, nullptr, false },
			{ TEXT("middle_metacarpal"), EHandKeypoint::MiddleMetacarpal, EHandKeypoint::MiddleProximal, TEXT("middle_01"), false },
			{ TEXT("middle_01"), EHandKeypoint::MiddleProximal, EHandKeypoint::MiddleIntermediate, TEXT("middle_02"), false },
			{ TEXT("middle_02"), EHandKeypoint::MiddleIntermediate, EHandKeypoint::MiddleDistal, TEXT("middle_03"), false },
			{ TEXT("middle_03"), EHandKeypoint::MiddleDistal, EHandKeypoint::MiddleTip, nullptr, false },
			{ TEXT("ring_metacarpal"), EHandKeypoint::RingMetacarpal, EHandKeypoint::RingProximal, TEXT("ring_01"), false },
			{ TEXT("ring_01"), EHandKeypoint::RingProximal, EHandKeypoint::RingIntermediate, TEXT("ring_02"), false },
			{ TEXT("ring_02"), EHandKeypoint::RingIntermediate, EHandKeypoint::RingDistal, TEXT("ring_03"), false },
			{ TEXT("ring_03"), EHandKeypoint::RingDistal, EHandKeypoint::RingTip, nullptr, false },
			{ TEXT("pinky_metacarpal"), EHandKeypoint::LittleMetacarpal, EHandKeypoint::LittleProximal, TEXT("pinky_01"), false },
			{ TEXT("pinky_01"), EHandKeypoint::LittleProximal, EHandKeypoint::LittleIntermediate, TEXT("pinky_02"), false },
			{ TEXT("pinky_02"), EHandKeypoint::LittleIntermediate, EHandKeypoint::LittleDistal, TEXT("pinky_03"), false },
			{ TEXT("pinky_03"), EHandKeypoint::LittleDistal, EHandKeypoint::LittleTip, nullptr, false },
		};
		for (const FRow& Row : Rows)
		{
			const int32 Bone = Index(Row.Bone);
			if (Bone == INDEX_NONE)
			{
				continue;
			}
			FVector RefDir = FVector::ZeroVector;
			float Length = 0.f;
			if (Row.TowardBone)
			{
				const int32 Toward = Index(Row.TowardBone);
				if (Toward == INDEX_NONE)
				{
					continue;
				}
				RefDir = Component[Toward].GetLocation() - Component[Bone].GetLocation();
				Length = RefDir.Size();
			}
			else
			{
				const int32 Parent = Ref.GetParentIndex(Bone);
				RefDir = Parent != INDEX_NONE ? Component[Bone].GetLocation() - Component[Parent].GetLocation() : FVector::ZeroVector;
			}
			if (!RefDir.Normalize())
			{
				continue;
			}
			FLTTHandBone& Entry = Out.AddDefaulted_GetRef();
			Entry.Bone = Ref.GetBoneName(Bone);
			Entry.Joint = static_cast<uint8>(Row.Joint);
			Entry.Toward = static_cast<uint8>(Row.Toward);
			Entry.bHandUp = Row.bHandUp;
			const FQuat Frame = FRotationMatrix::MakeFromXZ(RefDir, RefUp).ToQuat();
			Entry.Offset = Frame.Inverse() * Component[Bone].GetRotation();
			Entry.RefLength = Length;
			Entry.RefComponent = Component[Bone];
		}
		return Out.Num() > 0 && Out[0].Bone == Ref.GetBoneName(Hand);
	}

	/** Sets a float or double variable on an anim instance by name (the template anim blueprint's pose alphas). */
	void LTTSetAnimAlpha(UAnimInstance* Anim, FName Name, float Value)
	{
		FProperty* Property = Anim ? Anim->GetClass()->FindPropertyByName(Name) : nullptr;
		if (FDoubleProperty* Double = CastField<FDoubleProperty>(Property))
		{
			Double->SetPropertyValue_InContainer(Anim, Value);
		}
		else if (FFloatProperty* Float = CastField<FFloatProperty>(Property))
		{
			Float->SetPropertyValue_InContainer(Anim, Value);
		}
	}
}

void ULTTVRComponent::CreateHandMeshes(ACharacter* Character)
{
	if (!Character || LeftTrackedHand || LeftControllerHand)
	{
		return;
	}
	USkeletalMesh* MeshL = LoadObject<USkeletalMesh>(nullptr, LTTVRHandLeftMeshPath);
	USkeletalMesh* MeshR = LoadObject<USkeletalMesh>(nullptr, LTTVRHandRightMeshPath);
	if (!MeshL || !MeshR)
	{
		UE_LOG(LogCleanCutAction, Warning, TEXT("VR: template hand meshes not found (/Game/XRMannequins), hands show as joint dots"));
		return;
	}

	// Hand tracking: a poseable copy of the template hand, bone rotations set from the tracked joints every frame.
	auto MakeTracked = [this, Character](const TCHAR* Name, USkeletalMesh* Mesh, bool bRight, TArray<FLTTHandBone>& Bones) -> UPoseableMeshComponent*
	{
		if (!LTTBuildHandBones(Mesh, bRight, Bones))
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("VR: %s has no hand bones, that tracked hand stays as dots"), *GetNameSafe(Mesh));
			return nullptr;
		}
		UPoseableMeshComponent* Hand = NewObject<UPoseableMeshComponent>(Character, Name);
		Hand->SetupAttachment(Character->GetRootComponent());
		Hand->SetAbsolute(true, true, true);
		Hand->SetSkinnedAssetAndUpdate(Mesh);
		Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hand->SetCastShadow(false);
		Hand->SetTickableWhenPaused(true);
		Hand->SetHiddenInGame(true);
		Hand->SetOverlayMaterial(GlowMID);
		Hand->RegisterComponent();
		return Hand;
	};
	LeftTrackedHand = MakeTracked(TEXT("VR_LeftTrackedHand"), MeshL, false, LeftHandBones);
	RightTrackedHand = MakeTracked(TEXT("VR_RightTrackedHand"), MeshR, true, RightHandBones);

	// Controllers: the template's hand component (BP_MannequinsXR + ABP_MannequinsXR, which mirrors the right-hand
	// animations for the left), on the grip pose with the template's offsets.
	UClass* HandClass = LoadClass<USkeletalMeshComponent>(nullptr, LTTVRHandComponentClassPath);
	auto MakeControllerHand = [this, Character, HandClass](const TCHAR* Name, USkeletalMesh* Mesh, UMotionControllerComponent* Grip, bool bRight) -> USkeletalMeshComponent*
	{
		if (!Grip)
		{
			return nullptr;
		}
		USkeletalMeshComponent* Hand = NewObject<USkeletalMeshComponent>(Character, HandClass ? HandClass : USkeletalMeshComponent::StaticClass(), Name);
		Hand->SetupAttachment(Grip);
		Hand->SetSkeletalMeshAsset(Mesh);
		if (FBoolProperty* Mirror = FindFProperty<FBoolProperty>(Hand->GetClass(), TEXT("bMirror")))
		{
			Mirror->SetPropertyValue_InContainer(Hand, !bRight);
		}
		Hand->SetRelativeLocationAndRotation(FVector(-2.98f, bRight ? 3.5f : -3.5f, 4.56f),
			bRight ? FRotator(25.f, 0.f, 90.f) : FRotator(-25.f, -180.f, 90.f));
		Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hand->SetCastShadow(false);
		Hand->SetTickableWhenPaused(true);
		Hand->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		Hand->SetHiddenInGame(true);
		Hand->SetOverlayMaterial(GlowMID);
		Hand->RegisterComponent();
		if (!Hand->GetAnimInstance())
		{
			UE_LOG(LogCleanCutAction, Warning, TEXT("VR: %s has no anim blueprint, that controller hand won't animate"), Name);
		}
		return Hand;
	};
	LeftControllerHand = MakeControllerHand(TEXT("VR_LeftControllerHand"), MeshL, LeftController, false);
	RightControllerHand = MakeControllerHand(TEXT("VR_RightControllerHand"), MeshR, RightController, true);
	UE_LOG(LogCleanCutAction, Log, TEXT("VR: template hands ready (tracked %d/%d, controller %d/%d)"),
		LeftTrackedHand != nullptr, RightTrackedHand != nullptr, LeftControllerHand != nullptr, RightControllerHand != nullptr);
}

void ULTTVRComponent::UpdateHandMeshes(const FXRHandTrackingState& Left, const FXRHandTrackingState& Right, bool bLeftOK, bool bRightOK)
{
	if (LeftTrackedHand)
	{
		LeftTrackedHand->SetHiddenInGame(!bLeftOK);
		if (bLeftOK)
		{
			UpdateTrackedHandMesh(LeftTrackedHand, LeftHandBones, Left);
		}
	}
	if (RightTrackedHand)
	{
		RightTrackedHand->SetHiddenInGame(!bRightOK);
		if (bRightOK)
		{
			UpdateTrackedHandMesh(RightTrackedHand, RightHandBones, Right);
		}
	}
	// Controller hands only while playing with controllers (with hand tracking the runtime may still report a grip pose).
	const bool bLeftCtrl = !bHandsActive && LeftController && LeftController->IsTracked();
	const bool bRightCtrl = !bHandsActive && RightController && RightController->IsTracked();
	if (LeftControllerHand)
	{
		LeftControllerHand->SetHiddenInGame(!bLeftCtrl);
		if (bLeftCtrl)
		{
			UpdateControllerHandMesh(LeftControllerHand, false);
		}
	}
	if (RightControllerHand)
	{
		RightControllerHand->SetHiddenInGame(!bRightCtrl);
		if (bRightCtrl)
		{
			UpdateControllerHandMesh(RightControllerHand, true);
		}
	}
}

void ULTTVRComponent::UpdateTrackedHandMesh(UPoseableMeshComponent* Mesh, TArray<FLTTHandBone>& Bones, const FXRHandTrackingState& Hand)
{
	if (!Mesh || Bones.Num() == 0)
	{
		return;
	}
	auto Joint = [&Hand](uint8 Index) { return Hand.HandKeyLocations[Index]; };
	const FVector HandUp = Hand.HandKeyRotations[static_cast<int32>(EHandKeypoint::Palm)].GetUpVector(); // back of the hand
	auto BoneRotation = [&](const FLTTHandBone& Bone, FQuat& Out) -> bool
	{
		const FVector Dir = (Joint(Bone.Toward) - Joint(Bone.Joint)).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return false;
		}
		const FVector Up = Bone.bHandUp ? HandUp : Hand.HandKeyRotations[Bone.Joint].GetUpVector();
		Out = FRotationMatrix::MakeFromXZ(Dir, Up).ToQuat() * Bone.Offset;
		return true;
	};

	// Place the whole mesh so its hand bone sits on the wrist joint, scaled to the player's hand.
	const FLTTHandBone& Root = Bones[0];
	FQuat HandRotation;
	if (!BoneRotation(Root, HandRotation))
	{
		return;
	}
	const float Measured = FVector::Dist(Joint(Root.Joint), Joint(Root.Toward));
	const float Scale = Root.RefLength > 1.f ? FMath::Clamp(Measured / Root.RefLength, 0.75f, 1.3f) : 1.f;
	const FQuat ComponentRotation = HandRotation * Root.RefComponent.GetRotation().Inverse();
	const FVector ComponentLocation = Joint(Root.Joint) - ComponentRotation.RotateVector(Root.RefComponent.GetLocation() * Scale);
	Mesh->SetWorldTransform(FTransform(ComponentRotation, ComponentLocation, FVector(Scale)));

	// Then each finger bone, parents first (the list is in that order), in world space.
	for (int32 i = 1; i < Bones.Num(); ++i)
	{
		FQuat Rotation;
		if (BoneRotation(Bones[i], Rotation))
		{
			Mesh->SetBoneRotationByName(Bones[i].Bone, Rotation.Rotator(), EBoneSpaces::WorldSpace);
		}
	}
}

float ULTTVRComponent::GetHandActionValue(const TCHAR* Path)
{
	TObjectPtr<UInputAction>& Action = InjectedActions.FindOrAdd(Path);
	if (!Action)
	{
		Action = LoadObject<UInputAction>(nullptr, Path);
		if (!Action)
		{
			return 0.f;
		}
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* Input = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	const UEnhancedPlayerInput* PlayerInput = Input ? Input->GetPlayerInput() : nullptr;
	return PlayerInput ? PlayerInput->GetActionValue(Action).Get<float>() : 0.f;
}

void ULTTVRComponent::UpdateControllerHandMesh(USkeletalMeshComponent* Mesh, bool bRightHand)
{
	UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		return;
	}
	// Same inputs as the VR Template's pawn: grip closes the hand, the trigger curls the index, a finger off the
	// trigger points, a thumb off the buttons gives a thumbs up.
	const float Grip = GetHandActionValue(bRightHand ? LTTVRGripRPath : LTTVRGripLPath);
	const float Trigger = GetHandActionValue(bRightHand ? LTTVRTriggerRPath : LTTVRTriggerLPath);
	const float IndexTouch = GetHandActionValue(bRightHand ? LTTVRIndexTouchRPath : LTTVRIndexTouchLPath);
	const float ThumbTouch = GetHandActionValue(bRightHand ? LTTVRThumbTouchRPath : LTTVRThumbTouchLPath);
	LTTSetAnimAlpha(Anim, TEXT("PoseAlphaGrasp"), FMath::Clamp(Grip, 0.f, 1.f));
	LTTSetAnimAlpha(Anim, TEXT("PoseAlphaIndexCurl"), FMath::Clamp(Trigger, 0.f, 1.f));
	LTTSetAnimAlpha(Anim, TEXT("PoseAlphaPoint"), IndexTouch > 0.5f || Trigger > 0.05f ? 0.f : 1.f);
	LTTSetAnimAlpha(Anim, TEXT("PoseAlphaThumbUp"), ThumbTouch > 0.5f ? 0.f : 1.f);
}
