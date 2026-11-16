// Copyright © 2026 QLE OPC. All Rights Reserved.
// Clean! Cut! Action! - Turns the first person character into a VR body when a headset is active (OpenXR).
//
// Does nothing on flat screens. In VR it:
//  - moves the camera onto a floor-level origin driven by the headset, hides the first person arms,
//    keeps the capsule under the head (room scale) and crouches it when the player physically crouches;
//  - adds tracked hands with the runtime's controller models, a laser on the right hand, and aims
//    interaction (focus, place, throw, toss stations) along that laser;
//  - holds props in the right hand, with grip-to-grab and throw-by-flick;
//  - moves every viewport widget (HUDs, menus, end screens) onto a floating screen the laser can click;
//  - drives smooth locomotion, snap/smooth turn, comfort vignette and VR-friendly render settings.
//
// Controls (Quest Touch names; Index / WMR / Vive equivalents are in IMC_VR):
//  Left stick move | Right stick snap turn | Right trigger interact / click UI | Right grip grab, release to drop or flick to throw
//  A throw | B back (leave a station) | Right stick click crouch | X jump | Y emote | Menu pause | Left stick click recenter screen

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LTTVRComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UMotionControllerComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCameraComponent;
class ULTTVRScreen;
struct FInputActionValue;

/** A right hand position in tracking space and when it was there. */
struct FLTTVRHandSample
{
	FVector Location = FVector::ZeroVector;
	double Time = 0.0;
};

UCLASS(ClassGroup = (LTT), meta = (BlueprintSpawnableComponent), config = Game)
class CLEANCUTACTION_API ULTTVRComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULTTVRComponent();

	UFUNCTION(BlueprintPure, Category = "VR")
	bool IsVRActive() const { return bVRActive; }

	/** The right hand's laser (aim pose), world space. False when not in VR or the hand isn't tracked. */
	bool GetAimRay(FVector& OutLocation, FRotator& OutRotation) const;

	/** The laser is over a clickable widget on the VR screen. */
	bool IsPointingAtInteractiveUI() const { return bPointingAtUI; }

	// ---- Polled by the effect stations (they read raw keys rather than input actions) ----

	/** Either trigger is held. */
	bool IsTriggerDown() const { return bTriggerDown; }
	/** B (or the WMR / Vive right trackpad) is held. */
	bool IsBackDown() const { return bBackDown; }
	FVector2D GetMoveStick() const { return MoveStick; }
	FVector2D GetTurnStick() const { return TurnStick; }
	/** -1 / +1 on the frame either stick is pushed left / right past half way, 0 otherwise. */
	int32 GetStickFlick() const { return StickFlick; }
	/** -1 / +1 while the right hand moves quickly left / right (shake mini-game), 0 otherwise. */
	int32 GetHandShakeSign() const;

	/** Brings the floating screen back in front of the player. */
	UFUNCTION(BlueprintCallable, Category = "VR")
	void RecenterScreen() { bRecenterScreen = true; }

	// ---- Comfort ----

	/** Right stick turns in steps (on) or smoothly (off). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort")
	bool bSnapTurn = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort", meta = (ClampMin = "10", ClampMax = "90"))
	float SnapTurnDegrees = 45.f;

	/** Smooth turn speed at full stick, degrees per second. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort", meta = (ClampMin = "30", ClampMax = "360"))
	float SmoothTurnRate = 120.f;

	/** Stick walking speed as a fraction of the flat-screen speed (slower is easier on the stomach). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort", meta = (ClampMin = "0.2", ClampMax = "1.5"))
	float WalkSpeedScale = 0.75f;

	/** The stick walks where the head looks (on) or where the left hand points (off). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort")
	bool bHeadRelativeMovement = true;

	/** Darkens the edges of the view while moving or turning with the sticks. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort")
	bool bComfortVignette = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Comfort", meta = (ClampMin = "0", ClampMax = "2"))
	float ComfortVignetteIntensity = 1.1f;

	// ---- Body ----

	/** Crouching in real life crouches the capsule (to get under tables). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Body")
	bool bPhysicalCrouch = true;

	/** Eye height (cm above the floor) under which the capsule crouches. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Body", meta = (ClampMin = "60", ClampMax = "160"))
	float PhysicalCrouchEyeHeight = 115.f;

	/** Raises the view (cm), e.g. 40-50 for seated play. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Body", meta = (ClampMin = "-50", ClampMax = "100"))
	float ViewHeightOffset = 0.f;

	// ---- Throwing ----

	/** Letting go of the grip with the hand moving faster than this (cm/s) throws the prop instead of dropping it. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Throw", meta = (ClampMin = "50"))
	float FlickThrowSpeed = 200.f;

	/** Hand speed to throw speed. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Throw", meta = (ClampMin = "0.5", ClampMax = "4"))
	float HandThrowScale = 1.5f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Throw", meta = (ClampMin = "300"))
	float MaxHandThrowSpeed = 2200.f;

	// ---- Floating screen (HUDs and menus) ----

	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Screen", meta = (ClampMin = "80"))
	float ScreenDistance = 200.f;

	/** Width of the screen in the world (cm). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Screen", meta = (ClampMin = "50"))
	float ScreenWidth = 210.f;

	/** The screen's pixel size, the resolution the widgets lay out at. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Screen")
	FIntPoint ScreenResolution = FIntPoint(1920, 1080);

	/** The screen follows the head once the player looks this far away from it (degrees). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Screen", meta = (ClampMin = "10", ClampMax = "90"))
	float ScreenFollowAngle = 40.f;

	/** How often the screen redraws (per second). Lower is cheaper. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Screen", meta = (ClampMin = "10", ClampMax = "120"))
	float ScreenRedrawRate = 45.f;

	// ---- Performance ----

	/** Switches to VR-friendly scalability and render settings when VR starts. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Performance")
	bool bApplyVRRenderSettings = true;

	/** Lumen in stereo at 72-90 fps is too heavy for most headsets; turns it off in VR (lighting gets flatter). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "VR|Performance")
	bool bDisableLumenInVR = true;

	// ---- Visuals ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Visuals")
	TObjectPtr<UStaticMesh> LaserMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Visuals")
	TObjectPtr<UMaterialInterface> LaserMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Visuals", meta = (ClampMin = "10"))
	float LaserLength = 300.f;

	// ---- Input (IMC_VR is also a default mapping context in DefaultInput.ini, which OpenXR needs) ----

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputMappingContext> VRMappingContext;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> GrabAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> BackAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> UIClickAction;

	UPROPERTY(EditAnywhere, Category = "VR|Input")
	TObjectPtr<UInputAction> RecenterAction;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ActivateVR();
	void TryBindInput();
	void ApplyRenderSettings();

	void UpdateBody(float DeltaTime);
	void UpdateHands(float DeltaTime);
	void UpdateScreen(float DeltaTime);
	void CaptureViewportWidgets();
	void UpdateVignette(float DeltaTime);

	void Turn(float Degrees);
	FVector GetHandVelocity() const;

	void OnMove(const FInputActionValue& Value);
	void OnMoveDone(const FInputActionValue& Value);
	void OnTurn(const FInputActionValue& Value);
	void OnTurnDone(const FInputActionValue& Value);
	void OnGrabStarted(const FInputActionValue& Value);
	void OnGrabCompleted(const FInputActionValue& Value);
	void OnCrouch(const FInputActionValue& Value);
	void OnBack(const FInputActionValue& Value);
	void OnBackDone(const FInputActionValue& Value);
	void OnPause(const FInputActionValue& Value);
	void OnUIClickStarted(const FInputActionValue& Value);
	void OnUIClickCompleted(const FInputActionValue& Value);
	void OnRecenter(const FInputActionValue& Value);

	bool bVRActive = false;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> LeftGrip;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightGrip;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightAim;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Laser;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserMID;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Screen;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetInteractionComponent> Pointer;

	UPROPERTY(Transient)
	TObjectPtr<ULTTVRScreen> ScreenWidget;

	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<UInputComponent> BoundInputComponent;

	// Input state
	FVector2D MoveStick = FVector2D::ZeroVector;
	FVector2D TurnStick = FVector2D::ZeroVector;
	int32 StickFlick = 0;
	float PrevFlickX = 0.f;
	bool bSnapLatched = false;
	bool bTriggerDown = false;
	bool bBackDown = false;
	bool bUIPressed = false;
	bool bHeldByGrip = false;
	float SmoothTurnInput = 0.f;

	// Body
	bool bCrouchFromButton = false;
	bool bCrouchFromBody = false;
	float CrouchDrop = 0.f;
	float StandingWalkSpeed = 0.f;

	// Hands: recent right hand positions in VROrigin space, for throw and shake velocity.
	TArray<FLTTVRHandSample> HandSamples;
	bool bPointingAtUI = false;

	// Screen
	bool bScreenPlaced = false;
	bool bRecenterScreen = true;
	float ScreenYaw = 0.f;
	float ScreenTargetYaw = 0.f;
	float ScreenZ = 0.f;
	FVector LastScreenHead = FVector::ZeroVector;
	bool bScreenHadMenu = false;

	float Vignette = 0.f;
};
