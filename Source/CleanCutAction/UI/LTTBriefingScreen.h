// Clean! Cut! Action! - Briefing shown over the black screen before the game starts: Marlowe's note (why the
// Rookie is cleaning the cafe), how to play, controls, scoring, and a Start button. Opened by ALTTMessDirector
// (BriefingClass); the mess is made behind it and the countdown starts when the player presses Start.
// Builds its own layout in C++; a Widget Blueprint child can restyle it by naming widgets like the ones below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTBriefingScreen.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;
class ALTTMessDirector;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTBriefingScreen : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Fills in the time limit and point values. Called by the mess director right after creating the widget. */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void Setup(ALTTMessDirector* InDirector);

	/** Closes the briefing and starts the game. */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void StartGame();

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Marlowe's note: the story. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StoryText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HowToPlayText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoringText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	/** The briefing card (fades and rises in). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bWrap = false) const;
	void AddHeader(UVerticalBox* Box, const FText& Text, float TopPadding) const;
	void AddControlRow(UVerticalBox* Box, const FText& Action, const FText& Keyboard, const FText& Gamepad) const;
	/** Touch version of a controls row: the on-screen button's icon (from /Game/_CleanCutAction/UI/Icons), the action, and where/how. */
	void AddTouchRow(UVerticalBox* Box, const TCHAR* IconName, const FText& Action, const FText& How) const;
	/** True when the owning player uses the on-screen touch controls (phone/tablet, or bForceTouchControls). */
	bool IsTouch() const;
	/** Shows ControlsDesktop or ControlsTouch and sets the StartHint text for the current input. */
	void ApplyInputVariant();
	void ApplyTexts();

	UFUNCTION()
	void HandleStartClicked();

	TWeakObjectPtr<ALTTMessDirector> Director;
	float AnimTime = 0.f;
};
