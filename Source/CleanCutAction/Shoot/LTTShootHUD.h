// Clean! Cut! Action! - Phase 2 shoot HUD (Docs/06 §8): REC + timecode, take number, score, take meter, the cue
// card with its countdown, the next cue, the "in shot" warning, subtitles, Marlowe's megaphone line, act title
// cards, the CUT interstitial, the grade toast, the station mini-game meter and an arrow to the active station.
// Created by ALTTSceneDirector (HUDClass). Builds its own layout in C++; a Widget Blueprint child can restyle it
// by naming its widgets like the properties below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTShootHUD.generated.h"

class UTextBlock;
class UProgressBar;
class UBorder;
class UWidget;
class UCanvasPanel;
class ALTTSceneDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTShootHUD : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TakeText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MeterBar;

	/** Top right card: shown while a cue is active or coming up. */
	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CuePanel;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CueTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CueWhereText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CueBar;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarningText;

	/** Full-screen red tint that pulses while the player is in shot. */
	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Vignette;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SubtitlePanel;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LineText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BarkPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BarkLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActCardText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CutText;

	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ToastText;

	/** What to do at the station being used. The meter itself is painted under it. */
	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InstructionText;

	/** Under the crosshair while a cue is active: station name, distance and floor. */
	UPROPERTY(BlueprintReadOnly, Category = "Shoot HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StationHintText;

	// ---- Station meter (painted) ----

	/** Centre of the mini-game meter, as a fraction of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Station Meter")
	FVector2D MeterAnchor = FVector2D(0.5f, 0.66f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Station Meter")
	FVector2D MeterSize = FVector2D(460.f, 30.f);

	// ---- Station arrow ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Arrow")
	bool bShowStationArrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Arrow", meta = (ClampMin = "1", ClampMax = "90"))
	float ArrowHideAngle = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Arrow")
	FLinearColor ArrowColor = FLinearColor(1.f, 0.78f, 0.1f);

	/** Height difference (cm) above which the hint says "upstairs" / "downstairs". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoot HUD|Arrow")
	float FloorHeight = 220.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, bool bOverlay) const;
	UTextBlock* AddCentered(UCanvasPanel* Root, UTextBlock* Text, FVector2D Anchor, FVector2D Offset) const;

	void UpdateTopBar(const ALTTSceneDirector& D);
	void UpdateCueCard(const ALTTSceneDirector& D);
	void UpdateDialogue(const ALTTSceneDirector& D);
	void UpdateOverlays(const ALTTSceneDirector& D);
	void UpdateStation(const ALTTSceneDirector& D);

	TWeakObjectPtr<ALTTSceneDirector> Director;
	float Pulse = 0.f;

	// Painted bits, refreshed each tick.
	bool bMeterVisible = false;
	float MeterValue = 0.f;
	FVector2D MeterBand = FVector2D(1.f, 0.f);
	float MeterProgress = -1.f;
	bool bArrowVisible = false;
	FVector2f ArrowDir = FVector2f(0.f, 1.f);
};
