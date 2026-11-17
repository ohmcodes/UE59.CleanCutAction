// Clean! Cut! Action!

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "LTTVRComponent.generated.h"

class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UMotionControllerComponent;
class UOverlay;
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

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

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

	static int32 ActiveCount;
};
