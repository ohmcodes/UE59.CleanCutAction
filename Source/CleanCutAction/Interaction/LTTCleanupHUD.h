// Clean! Cut! Action! - Phase 1 cleanup HUD: placed / destroyed counts, points, the countdown, and an
// arrow toward the held prop's home (or the hinted prop) when the player isn't looking at it.
// Created by ALTTMessDirector (HUDClass). Works on its own: if no Widget Blueprint layout is given it
// builds a default one. A Widget Blueprint child can restyle it by naming its Text Blocks PlacedText,
// DestroyedText, PointsText, TimerText, HintKeyText and HintText, and its Images PlacedIcon, DestroyedIcon,
// PointsIcon, HintIcon and TimerIcon (the timer icon pulses with each second of the countdown).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "LTTCleanupHUD.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;
class UImage;
class UTexture2D;
class ALTTMessDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTCleanupHUD : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlacedText;

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DestroyedText;

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointsText;

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

	/** "[F] Hint", greyed out with the seconds left while the hint is cooling down. */
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintKeyText;

	/** Under the crosshair while a hint is up: prop name, distance, and floor. */
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	// ---- Icons (top left stats and the countdown) ----

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlacedIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> DestroyedIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PointsIcon;

	/** Light bulb next to the "[F] Hint" prompt; faded while the hint cools down. */
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> HintIcon;

	/** Grows and shrinks once per second while the countdown runs (twice, and bigger, when time is low). */
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> TimerIcon;

	/** How much the timer icon grows on each beat (0.15 = 15% bigger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown", meta = (ClampMin = "0"))
	float TimerPulseAmount = 0.15f;

	/** Beat size in the last LowTimeSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown", meta = (ClampMin = "0"))
	float LowTimePulseAmount = 0.3f;

	// ---- Countdown look ----

	/** The countdown turns this colour when this many seconds or fewer are left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	float LowTimeSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	FLinearColor TimerColor = FLinearColor(0.0685f, 0.0296f, 0.0091f); // theme Ink (#4A3018), on the paper pill

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	FLinearColor LowTimeColor = FLinearColor(0.5271f, 0.0685f, 0.0296f); // theme Bad (#C04A30)

	// ---- Home arrow ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	bool bShowHomeArrow = true;

	/** The arrow hides when the home is within this many degrees of where the camera points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow", meta = (ClampMin = "1", ClampMax = "90"))
	float ArrowHideAngle = 15.f;

	/** Distance of the arrow from the screen centre, as a fraction of the screen height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float ArrowRadius = 0.22f;

	/** Arrow length in pixels (at 1080p DPI scale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	float ArrowSize = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	float ArrowThickness = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	FLinearColor ArrowColor = FLinearColor(1.f, 0.78f, 0.1f);

	/** Arrow colour when it points at the hinted prop (not holding anything). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	FLinearColor HintArrowColor = FLinearColor(0.2f, 0.85f, 1.f);

	/** Height difference (cm) above which the hint says "upstairs" / "downstairs". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Home Arrow")
	float FloorHeight = 220.f;

	// ---- Controls legend (PC / web only; touch has its own buttons) ----

	/** Bottom-left card listing the keys. Built at runtime if the Widget Blueprint has no widget with this name. */
	UPROPERTY(BlueprintReadOnly, Category = "Cleanup HUD|Controls", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ControlsLegend;

	/** Shows / hides the key list (the card folds to a one-line "[H] Controls" tab). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Controls")
	FKey ToggleControlsKey = EKeys::H;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Controls")
	bool bControlsLegendStartsOpen = true;

	UFUNCTION(BlueprintCallable, Category = "Cleanup HUD|Controls")
	void SetControlsLegendOpen(bool bOpen);

	// ---- Nearby pickup ring ----

	/** Draws a ring (a circle with a hole) over the nearest prop the player can pick up while empty-handed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring")
	bool bShowPickupRing = true;

	/** Only props within this distance of the camera get the ring (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring", meta = (ClampMin = "50"))
	float PickupRingDistance = 350.f;

	/** Only props within this many degrees of where the camera points get the ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring", meta = (ClampMin = "5", ClampMax = "89"))
	float PickupRingMaxAngle = 40.f;

	/** Ring radius in pixels (at 1080p DPI scale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring", meta = (ClampMin = "4"))
	float PickupRingRadius = 26.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring", meta = (ClampMin = "1"))
	float PickupRingThickness = 5.f;

	/** Ring colour while the prop is near but not under the crosshair. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring")
	FLinearColor PickupRingColor = FLinearColor(1.f, 0.85f, 0.25f);

	/** Ring colour when the crosshair is on the prop (pressing Interact picks it up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Pickup Ring")
	FLinearColor PickupRingFocusColor = FLinearColor(0.3f, 1.f, 0.4f);

	/** True while the arrow is drawn. */
	UFUNCTION(BlueprintPure, Category = "Cleanup HUD|Home Arrow")
	bool IsHomeArrowVisible() const { return bArrowVisible; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** Builds the default layout when there is no Widget Blueprint layout. */
	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize) const;
	/** Image showing /Game/_CleanCutAction/UI/Widget/<TextureName> at Size x Size. */
	UImage* MakeIcon(FName Name, const TCHAR* TextureName, float Size) const;
	void UpdateTimerIcon();

	void BuildControlsLegend();
	void UpdateControlsLegend();
	/** Small dark key cap with cream letters ("E", "Space"). */
	UWidget* MakeKeyCap(const FText& Key) const;
	/** Key cap holding a mouse-button icon (T_Icon_MouseLeft / Right). */
	UWidget* MakeMouseCap(const TCHAR* IconName) const;
	/** Caps, then the action name. */
	void AddLegendRow(UVerticalBox* Box, const TArray<UWidget*>& Caps, const FText& Action) const;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ControlsList;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ControlsToggleText;

	bool bLegendOpen = true;

	void UpdateTexts();
	void UpdateArrow();
	/** Points the arrow at Target unless it's within ArrowHideAngle of the view. */
	void AimArrowAt(const FVector& Target, const FVector& CamLoc, const FRotator& CamRot, const FLinearColor& Color);

	TWeakObjectPtr<ALTTMessDirector> Director;

	bool bArrowVisible = false;
	/** Screen-space direction to the home (X right, Y down), unit length. */
	FVector2f ArrowDir = FVector2f(0.f, 1.f);
	FLinearColor ArrowDrawColor = FLinearColor::White;
	float ArrowPulse = 0.f;

	/** Picks the ring's prop (a few times a second) and projects it to the screen (every frame). */
	void UpdatePickupRing(float DeltaTime);
	/** Nearest visible pickable prop in range and in view, or null. */
	class ALTTInteractableBase* FindPickupRingTarget(const FVector& CamLoc, const FVector& CamDir, const AActor* IgnoreActor) const;

	TWeakObjectPtr<class ALTTInteractableBase> RingTarget;
	float RingSearchTimer = 0.f;
	/** Ring centre in widget space. Kept while the ring fades out. */
	FVector2f RingPos = FVector2f::ZeroVector;
	/** 0..1, fades in and out. */
	float RingAlpha = 0.f;
	bool bRingFocused = false;
};
