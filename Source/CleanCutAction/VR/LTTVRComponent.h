// Clean! Cut! Action!

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "LTTVRComponent.generated.h"

class UCameraComponent;
class UInstancedStaticMeshComponent;
class ULTTVRComponent;
class UEnhancedInputComponent;
class UInputAction;
class UMotionControllerComponent;
class UOverlay;
class UMaterialInstanceDynamic;
class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
struct FInputActionValue;

/** Full-screen overlay that holds the viewport widgets while in VR (screen-space UI isn't drawn in the headset). */
UCLASS()
class ULTTVRUIHost : public UUserWidget
{
	GENERATED_BODY()

public:
	UOverlay* GetRoot() const { return Root; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> Root;
};

/** What a wrist menu button does (hand tracking has no buttons, so these stand in for A, B, X, Y, stick turn, Menu). */
enum class ELTTVRHandAction : uint8
{
	Jump,
	Throw,
	Hint,
	Dance,
	TurnLeft,
	TurnRight,
	Recenter,
	Pause,
};

/** Wrist menu for hand tracking: appears above the left hand when its palm faces the player. Shows the score, the
 *  gesture list and buttons for every action; point with the right hand and pinch to press one. Built in C++, no
 *  widget asset. */
UCLASS()
class ULTTVRHandMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ULTTVRComponent> OwnerVR;

	/** Draw size of the menu, px (the widget component uses it). */
	static const FVector2D DrawSize;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void OnJump();
	UFUNCTION() void OnThrow();
	UFUNCTION() void OnHint();
	UFUNCTION() void OnDance();
	UFUNCTION() void OnTurnLeft();
	UFUNCTION() void OnTurnRight();
	UFUNCTION() void OnRecenter();
	UFUNCTION() void OnPause();

private:
	void Fire(ELTTVRHandAction Action);
};

/** One bone of a tracked-hand mesh and the OpenXR joints that pose it (see ULTTVRComponent::UpdateTrackedHandMesh). */
struct FLTTHandBone
{
	FName Bone;
	/** Joint at the bone's start and the joint it points at. */
	uint8 Joint = 0;
	uint8 Toward = 0;
	/** Twist from the back of the whole hand (thumb bones) instead of the joint's own. */
	bool bHandUp = false;
	/** Bone rotation relative to a frame built from its direction and the back of the hand (reference pose). */
	FQuat Offset = FQuat::Identity;
	/** Reference-pose length from this bone to the one it points at, cm (the hand bone's sizes the mesh to the player). */
	float RefLength = 0.f;
	/** Component-space reference transform (the hand bone's places the mesh). */
	FTransform RefComponent = FTransform::Identity;
};

/** Right hand position at one moment, in VR origin space (for the real throw's speed). */
struct FLTTHandSample
{
	double Time = 0.0;
	FVector Pos = FVector::ZeroVector;
};

/**
 * Turns the first person character into a VR pawn when a headset is active (PC VR over Link / Air Link, later Quest).
 * Does nothing in flat play, so the desktop, web and Android builds are unchanged.
 *
 * - The camera moves to a VR origin at eye height and follows the headset instead of the control rotation.
 * - Left / right motion controllers get small visible markers; the right aim pose has a laser for menus.
 * - Right stick snap-turns. Left stick moves relative to where the head faces (the character does that part).
 * - Every viewport widget (HUD, briefing, pause, end screen) is moved onto a world-space panel in front of the player,
 *   and the right trigger clicks its buttons.
 *
 * Interaction still aims with the head (GetPlayerViewPoint is the HMD view), so the cleaning code works as is.
 */
UCLASS(ClassGroup = (CleanCutAction), meta = (BlueprintSpawnableComponent))
class CLEANCUTACTION_API ULTTVRComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTVRComponent();

	/** True while a headset is rendering this pawn's view. */
	UFUNCTION(BlueprintPure, Category = "VR")
	bool IsVRActive() const { return bVRActive; }

	/** True when any pawn in the world is in VR mode (for code with no pawn at hand). */
	static bool IsHeadsetActive();

	/** True while the player plays with tracked hands instead of controllers (HUD labels switch to gestures). */
	static bool IsUsingHands();

	/** Controllers: X opens / closes the info menu above the left controller (score, hint, buttons). Returns true if a
	 *  VR pawn took the press (the caller then skips its own X action). */
	static bool ToggleControllerMenu();

	/** True while the VR pointer (controller laser or hand ray) is on a clickable menu button, so a trigger press there
	 *  clicks the button instead of picking something up. */
	static bool IsPointerOnUI();

	/** Runs a wrist menu action (same result as the matching controller button). */
	void DoHandAction(ELTTVRHandAction Action);

	/** Gap between the thumb and a fingertip that counts as a pinch, cm (released again at twice this). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands", meta = (ClampMin = "0.2", ClampMax = "5"))
	float PinchDistance = 1.4f;

	/** Walking speed while the left index pinch is held, as a share of full stick (0.1 - 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands", meta = (ClampMin = "0.1", ClampMax = "1"))
	float PinchWalkSpeed = 0.45f;

	/** Width of the wrist menu in the world, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands", meta = (ClampMin = "10"))
	float HandMenuWidth = 36.f;

	/** How long the right palm has to face the eyes to open the pause menu, seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands", meta = (ClampMin = "0.1", ClampMax = "3"))
	float PausePalmHoldTime = 0.6f;

	/** Real throw: flick the right hand forward while holding a prop to throw it with the hand's speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Throw")
	bool bHandThrow = true;

	/** Hand speed that counts as a throw, cm/s (a relaxed seated flick is about 250 - 450). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Throw", meta = (ClampMin = "50"))
	float HandThrowMinSpeed = 220.f;

	/** Hand speed is multiplied by this (a seated arm is slow; the flat game throws at 1100 cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Throw", meta = (ClampMin = "0.5", ClampMax = "10"))
	float HandThrowBoost = 2.6f;

	/** Fastest a hand throw can leave the hand, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Throw", meta = (ClampMin = "100"))
	float HandThrowMaxSpeed = 1900.f;

	/** No hand throw this long after picking a prop up or pinching, seconds (the pickup pinch moves the hand too). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Throw", meta = (ClampMin = "0"))
	float HandThrowGrace = 0.45f;

	/** Real mopping: holding the mop over a puddle, scrubbing the right hand back and forth makes the strokes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Mop")
	bool bHandMop = true;

	/** Horizontal hand travel that counts as one mop stroke, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Hands|Mop", meta = (ClampMin = "3"))
	float MopStrokeDistance = 14.f;

	/** Snap turn step, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Comfort", meta = (ClampMin = "10", ClampMax = "90"))
	float SnapTurnDegrees = 45.f;

	/** Distance of the UI panel from the eyes, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|UI", meta = (ClampMin = "40"))
	float PanelDistance = 140.f;

	/** Width of the UI panel in the world, cm (height follows the 16:9 draw size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|UI", meta = (ClampMin = "20"))
	float PanelWidth = 150.f;

	/** The panel re-centres in front of the head once the head turns further than this from it, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|UI", meta = (ClampMin = "0", ClampMax = "90"))
	float PanelFollowAngle = 30.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnterVR();
	void ExitVR();
	void TryBindInput();
	void HandleTurn(const FInputActionValue& Value);
	void HandleTurnReleased();
	void HandleUIClickPressed();
	void HandleUIClickReleased();
	void CaptureViewportWidgets();
	void ReleaseCapturedWidgets();
	void UpdatePanel(float DeltaTime);
	void UpdatePointer();
	UMotionControllerComponent* MakeController(FName Name, FName Source, bool bVisible);
	void SnapTurn(float Sign);
	void UpdateHands(float DeltaTime);
	void UpdateHandMenu(const struct FXRHandTrackingState& Left, const FVector& Head);
	/** One gesture fired by a finger pinch (Finger: 0 index, 1 middle, 2 ring, 3 little). */
	void HandlePinch(bool bRightHand, int32 Finger, bool bPressed);
	/** Right palm held toward the face opens the pause menu. Returns true while the palm faces the face. */
	bool UpdatePausePalm(const struct FXRHandTrackingState& Right, bool bRightOK, const FVector& Head, float DeltaTime);
	/** Real throw and mop strokes from the right hand's motion. */
	void UpdateHandMotion(const struct FXRHandTrackingState& Right, bool bRightOK, bool bPalmFacing, float DeltaTime);
	/** Template hand meshes: posed from the tracked joints, or animated from the controller buttons. */
	void CreateHandMeshes(class ACharacter* Character);
	void UpdateHandMeshes(const struct FXRHandTrackingState& Left, const struct FXRHandTrackingState& Right, bool bLeftOK, bool bRightOK);
	void UpdateTrackedHandMesh(UPoseableMeshComponent* Mesh, TArray<FLTTHandBone>& Bones, const struct FXRHandTrackingState& Hand);
	void UpdateControllerHandMesh(USkeletalMeshComponent* Mesh, bool bRightHand);
	float GetHandActionValue(const TCHAR* Path);
	/** World-space pointer ray: the tracked right hand (shoulder through the knuckles), else the right controller. */
	bool GetAimRay(FVector& OutStart, FVector& OutDir) const;
	/** Fires an Enhanced Input action as if its button was pressed this frame (paths are the IMC_VR actions). */
	void InjectAction(const TCHAR* Path, const struct FInputActionValue& Value);
	bool IsMenuOpen() const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> LeftController;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightController;

	/** Right hand aim pose (points along the controller ray); the laser and widget pointer hang off it. */
	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightAim;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Laser;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> UIPanel;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetInteractionComponent> Pointer;

	UPROPERTY(Transient)
	TObjectPtr<ULTTVRUIHost> UIHost;

	/** Viewport Z order of each captured widget, so the panel stacks them like the screen did. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UUserWidget>, int32> CapturedZOrder;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UIClickAction;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;

	/** Joint spheres that show the tracked hands (nothing else draws them). */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> LeftHandDots;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> RightHandDots;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> HandMenuPanel;

	UPROPERTY(Transient)
	TObjectPtr<ULTTVRHandMenu> HandMenu;

	/** Input actions fired by hand gestures, by asset path. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UInputAction>> InjectedActions;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	/** Template hands (XRMannequins) posed from the tracked hand joints. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> LeftTrackedHand;

	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> RightTrackedHand;

	/** Template hands on the controllers, animated by the template's anim blueprint (grip, trigger, thumb). */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> LeftControllerHand;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> RightControllerHand;

	/** Unlit glow (M_VRGlow) so the hands, laser and joint dots stay visible in a dark set. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserMID;

	TArray<FLTTHandBone> LeftHandBones;
	TArray<FLTTHandBone> RightHandBones;

	/** Where the camera was attached in flat mode, to put it back if the headset goes away. */
	TWeakObjectPtr<USceneComponent> FlatCameraParent;
	FName FlatCameraSocket;
	FTransform FlatCameraRelative;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInputComponent;

	bool bVRActive = false;
	bool bTurnArmed = true;
	bool bPanelPlaced = false;
	float PanelYaw = 0.f;
	float CaptureTimer = 0.f;

	// Hand tracking state.
	bool bHandsActive = false;
	float HandsLostTime = 0.f;
	/** Finger pinched against the thumb on each hand (0 index ... 3 little), -1 for none. */
	int32 LeftPinchFinger = -1;
	int32 RightPinchFinger = -1;
	bool bRightPinchInteract = false;
	bool bHandMenuShown = false;
	bool bHandAimValid = false;
	FVector HandAimStart = FVector::ZeroVector;
	FVector HandAimDir = FVector::ForwardVector;

	/** Controllers: the info menu above the left controller is open (toggled with X). */
	bool bControllerMenuOpen = false;

	// Right palm pause.
	float PausePalmTime = 0.f;
	bool bPausePalmFired = false;

	// Right hand motion (throw / mop), in VR origin space so snap turns and walking don't count as hand speed.
	TArray<FLTTHandSample> RightHandSamples;
	TWeakObjectPtr<AActor> LastHeldItem;
	double HandThrowBlockedUntil = 0.0;
	bool bMopAnchorValid = false;
	FVector MopAnchor = FVector::ZeroVector;

	static int32 ActiveCount;
	static int32 HandsCount;
};
