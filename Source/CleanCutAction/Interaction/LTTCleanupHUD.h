// Clean! Cut! Action! - Phase 1 cleanup HUD: placed / destroyed counts, points, the countdown, and an
// arrow toward the held prop's home (or the hinted prop) when the player isn't looking at it.
// Created by ALTTMessDirector (HUDClass). Works on its own: if no Widget Blueprint layout is given it
// builds a default one. A Widget Blueprint child can restyle it by naming its Text Blocks PlacedText,
// DestroyedText, PointsText, TimerText, HintKeyText and HintText.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTCleanupHUD.generated.h"

class UTextBlock;
class ALTTMessDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTCleanupHUD : public UUserWidget
{
	GENERATED_BODY()

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

	// ---- Countdown look ----

	/** The countdown turns this colour when this many seconds or fewer are left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	float LowTimeSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	FLinearColor TimerColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup HUD|Countdown")
	FLinearColor LowTimeColor = FLinearColor(1.f, 0.15f, 0.1f);

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
};
