// Clean! Cut! Action! - Phase 1 end screen: "SET READY!" when every prop is back, "TIME'S UP!" when the
// countdown runs out. Shows time on set, props returned / destroyed, hints and points, with Play Again and
// Keep Playing. Opened by ALTTMessDirector (EndScreenClass); the game is paused while it's up.
// Builds its own layout in C++; a Widget Blueprint child can restyle it by naming widgets like the ones below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTEndScreen.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;
class ALTTMessDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTEndScreen : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Fills in the results. Called by the mess director right after creating the widget. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void Setup(ALTTMessDirector* InDirector, bool bInSetClean);

	UFUNCTION(BlueprintPure, Category = "End Screen")
	bool WasSetClean() const { return bSetClean; }

	/** Reloads the level. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void PlayAgain();

	/** Closes the screen and unpauses. After time's up the countdown is switched off and play continues. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void KeepPlaying();

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReturnedValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DestroyedValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintsValue;

	/** "+N" for the seconds left on the clock (set clean only). */
	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeBonusValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointsValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UButton> PlayAgainButton;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UButton> KeepPlayingButton;

	/** The results card (fades and slides in). */
	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const;
	UButton* MakeButton(FName Name, const FText& Label, const FLinearColor& Color) const;
	void AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock* Value) const;
	void ApplyTexts();

	UFUNCTION()
	void HandlePlayAgainClicked();

	UFUNCTION()
	void HandleKeepPlayingClicked();

	TWeakObjectPtr<ALTTMessDirector> Director;
	bool bSetClean = true;
	int32 FinalPoints = 0;
	float AnimTime = 0.f;
};
