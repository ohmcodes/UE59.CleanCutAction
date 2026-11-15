// Clean! Cut! Action! - Marlowe's megaphone line for Phase 1 (bottom centre, the same gold pill as the Phase 2
// shoot HUD's bark). Created by ALTTCleanupBarker and fed from it every frame; pops in when a new line starts
// and fades out when the line is over. Builds its own layout; a Widget Blueprint child can restyle it by naming
// a widget BarkPanel and Text Blocks BarkLabel and SpeakerLabel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTBarkWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class ALTTCleanupBarker;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTBarkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Bark", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BarkPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Bark", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BarkLabel;

	/** "MARLOWE", "JUNIPER (walkie)". */
	UPROPERTY(BlueprintReadOnly, Category = "Bark", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Bark", meta = (BindWidgetOptional))
	TObjectPtr<UImage> BarkIcon;

	/** Distance of the panel's bottom edge from the bottom of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	float BottomOffset = 150.f;

	void Setup(ALTTCleanupBarker* InBarker, UTexture2D* Icon);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildDefaultLayout();

	TWeakObjectPtr<ALTTCleanupBarker> Barker;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> IconTexture;

	int32 ShownSerial = INDEX_NONE;
	/** 0..1 fade of the panel. */
	float Alpha = 0.f;
	/** Seconds since the current line popped in. */
	float PopTime = 10.f;
	bool bWalkie = false;
};
