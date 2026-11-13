// Clean! Cut! Action! - In-game leaderboard: the top 10 Rookies, and the player's own place underneath when they
// are not in the top 10. Opened from the end screen (ULTTEndScreen::LeaderboardPanelClass) over the paused game.
// "Full Leaderboard" opens cca.ohmcodes.com/leaderboard in a new browser tab.
// Builds its own layout in C++; a Widget Blueprint child can restyle it by naming widgets like the ones below.
// Rows are made at runtime inside the "Rows" and "YouBox" vertical boxes.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/LTTLeaderboardSubsystem.h"
#include "LTTLeaderboardPanel.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTLeaderboardPanel : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Adds the panel to the viewport above everything, takes UI focus and loads the board. When it closes, focus
	 *  goes back to ReturnFocusTo (the end screen). */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void Open(UUserWidget* ReturnFocusTo);

	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void Close();

	/** Asks the server again. */
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void Refresh();

	/** Players shown in the list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaderboard", meta = (ClampMin = "1", ClampMax = "25"))
	int32 RowCount = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** "Loading...", "Top 10 of N Rookies", or the error. */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** The top rows go in here. */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Rows;

	/** The player's own place when they are not in the list (or how to get on it). */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> YouBox;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UButton> FullLeaderboardButton;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** The card (fades and rises in). */
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bBold = true) const;
	UButton* MakeButton(FName Name, const FText& Label, const FLinearColor& Fill, const TCHAR* Icon) const;
	/** One leaderboard line: rank chip, name, score, best time. bHeader = column titles. */
	UWidget* MakeRow(const FLTTLeaderboardEntry& Entry, bool bHeader) const;
	/** A plain message line inside the list area (empty board, not ranked yet). */
	UWidget* MakeMessageRow(const FText& Message) const;
	void ShowLoading();

	UFUNCTION()
	void HandleFetched(const FLTTLeaderboardResult& Result);

	UFUNCTION()
	void HandleFullClicked();

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleCloseClicked();

	TWeakObjectPtr<UUserWidget> ReturnFocus;
	/** Rows slide in one after another from RowsShownAt. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> AnimatedRows;
	float AnimTime = 0.f;
	float RowsShownAt = -1.f;
};
