// Clean! Cut! Action! - Phase 2 end screen (Docs/06 §9.2): "THAT'S A WRAP!", stars, Marlowe's quote, score,
// cue grades, CUTs and time in shot, with Shoot Again and Back to Cleanup. Opened by ALTTSceneDirector
// (EndScreenClass); the game is paused while it's up. Builds its own layout in C++; a Widget Blueprint child can
// restyle it by naming widgets like the ones below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTShootEndScreen.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;
class ALTTSceneDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTShootEndScreen : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Fills in the results. Called by the scene director right after creating the widget. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void Setup(ALTTSceneDirector* InDirector);

	/** Reloads the shoot level. */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void ShootAgain();

	/** Opens the director's CleanupLevelName (Phase 1). */
	UFUNCTION(BlueprintCallable, Category = "End Screen")
	void BackToCleanup();

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** "DIRECTOR'S CUT - 3 / 3 STARS". */
	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StarsText;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuoteText;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CuesValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CutsValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointsValue;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ShootAgainButton;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CleanupButton;

	UPROPERTY(BlueprintReadOnly, Category = "End Screen", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const;
	UButton* MakeButton(FName Name, const FText& Label, const FLinearColor& Color) const;
	void AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock* Value) const;

	UFUNCTION()
	void HandleShootAgainClicked();

	UFUNCTION()
	void HandleCleanupClicked();

	TWeakObjectPtr<ALTTSceneDirector> Director;
	int32 FinalPoints = 0;
	float AnimTime = 0.f;
};
