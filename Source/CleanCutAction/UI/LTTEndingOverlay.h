// Clean! Cut! Action! - Overlay for the end of a take: a SKIP button while the ending camera shots play, then a
// confetti burst that keeps falling over the end screen. Created by ALTTMessDirector (EndingOverlayClass).
// Confetti is drawn in NativePaint (no textures, no Niagara) and ticks in real time, so it keeps falling while
// the end screen has the game paused. Builds its own layout in C++; a WBP_ child can restyle the SkipButton.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTEndingOverlay.generated.h"

class UButton;
class UTextBlock;
class ALTTMessDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTEndingOverlay : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	void Setup(ALTTMessDirector* InDirector);

	/** Shows / hides the SKIP button (shown while the ending shots play). */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void SetSkipVisible(bool bVisible);

	/** Fires a burst of confetti from both bottom corners plus a shower from the top. The widget removes itself
	 *  once every piece has fallen off screen. */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void PlayConfetti();

	/** Pieces in one burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Confetti", meta = (ClampMin = "10", ClampMax = "600"))
	int32 ConfettiCount = 220;

	/** Seconds before pieces start fading out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Confetti", meta = (ClampMin = "0.5"))
	float ConfettiLifetime = 4.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Ending", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SkipButton;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void BuildDefaultLayout();

	UFUNCTION()
	void HandleSkipClicked();

	struct FPiece
	{
		FVector2f Pos = FVector2f::ZeroVector;
		FVector2f Vel = FVector2f::ZeroVector;
		FVector2f Size = FVector2f(10.f, 6.f);
		float Angle = 0.f;
		float Spin = 0.f;
		float Flutter = 0.f;   // phase of the side-to-side sway and the "flip" (width pulsing)
		float Age = 0.f;
		FLinearColor Color = FLinearColor::White;
	};
	TArray<FPiece> Pieces;
	FSlateBrush PieceBrush;
	TWeakObjectPtr<ALTTMessDirector> Director;
	FVector2f ScreenSize = FVector2f(1920.f, 1080.f);
	float AnimTime = 0.f;
	float SkipShownAt = -1.f;
	bool bConfettiPending = false;
	bool bConfettiPlayed = false;
};
