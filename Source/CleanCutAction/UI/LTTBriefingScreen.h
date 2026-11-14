// Clean! Cut! Action! - Briefing shown over the black screen before the game starts: a three-page carousel
// (Marlowe's note / how to play / controls and scoring, flipped with the arrows, Left/Right, the d-pad or a swipe),
// the leaderboard name, a Leaderboard button, and the Start buttons. Opened by ALTTMessDirector
// (BriefingClass); the mess is made behind it. Two start buttons: StartButton plays a time trial (the countdown
// starts), NoTimerButton plays without a clock (the director's timer is switched off).
// Builds its own layout in C++; a Widget Blueprint child can restyle it by naming widgets like the ones below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTBriefingScreen.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;
class UEditableTextBox;
class UWidgetSwitcher;
class UHorizontalBox;
class ALTTMessDirector;
class ULTTVirtualKeyboard;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTBriefingScreen : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Fills in the time limit and point values. Called by the mess director right after creating the widget. */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void Setup(ALTTMessDirector* InDirector);

	/** Closes the briefing and starts the game, with the countdown (time trial) or without it. */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void StartGame(bool bTimeTrial = true);

	/** On-screen keyboard for the name box on touch (WBP_VirtualKeyboard). Empty: loaded by path, else the C++ layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing")
	TSubclassOf<ULTTVirtualKeyboard> KeyboardClass;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Marlowe's note: the story. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StoryText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HowToPlayText;

	/** Scoring rules, one per row next to an icon (coffee / broken cup / stopwatch). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScorePlacedText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreBrokenText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreBonusText;

	/** Starts a time trial (Enter / gamepad A). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	/** Starts without the countdown (gamepad Y / N key). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NoTimerButton;

	/** The player's name on the leaderboard (starts as a random name; the player can type their own). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NameInput;

	/** Rolls a new random name. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RandomNameButton;

	/** Saves whatever is in NameInput as the leaderboard name (empty = a new random name). */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void CommitName();

	// ---- Carousel ----

	/** The three pages: 0 = director's note, 1 = how to play, 2 = controls and scoring. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> Pages;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrevPageButton;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextPageButton;

	/** "2 / 3  -  HOW TO PLAY" */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PageLabel;

	/** One dot per page (made at runtime); the current one is gold. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> PageDots;

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void ShowPage(int32 Index);

	UFUNCTION(BlueprintPure, Category = "Briefing")
	int32 GetPageIndex() const { return PageIndex; }

	/** Opens the leaderboard panel over the briefing. Added to NameRow at runtime if the WBP has none. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LeaderboardButton;

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void OpenLeaderboard();

	/** Opens the on-screen keyboard for the name (touch: the browser never shows the phone's keyboard). */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void OpenNameKeyboard();

	/** The briefing card with its corner decorations (fades and rises in). */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bWrap = false) const;
	/** Section header: a cartoon icon (/Game/_CleanCutAction/UI/Widget/<IconName>) and the title. */
	void AddHeader(UVerticalBox* Box, const FText& Text, float TopPadding, const TCHAR* IconName) const;
	/** Icon + wrapping text row (scoring rules). */
	UTextBlock* AddIconTextRow(UVerticalBox* Box, FName TextName, const TCHAR* IconName) const;
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

	UFUNCTION()
	void HandleNoTimerClicked();

	UFUNCTION()
	void HandleRandomNameClicked();

	UFUNCTION()
	void HandleNameChanged(const FText& Text);

	UFUNCTION()
	void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Name row (label, text box, dice button) for the C++ layout. */
	void AddNameRow(UVerticalBox* Box);

	/** Carousel pages and the arrows / dots row under them, for the C++ layout. */
	void AddPages(UVerticalBox* Box);
	void AddPageNav(UVerticalBox* Box);
	UButton* MakeLeaderboardButton();
	void RefreshPageNav();
	FText GetPageTitle(int32 Index) const;

	UFUNCTION()
	void HandlePrevPageClicked();

	UFUNCTION()
	void HandleNextPageClicked();

	UFUNCTION()
	void HandleLeaderboardClicked();

	UFUNCTION()
	void HandleKeyboardClosed(const FString& FinalText);

	int32 PageIndex = 0;
	float PageChangedAt = -1.f;
	float PageSlideDir = 1.f;
	FVector2D SwipeStart = FVector2D::ZeroVector;
	bool bSwipeTracking = false;
	float KeyboardClosedAt = -10.f;

	UPROPERTY(Transient)
	TObjectPtr<ULTTVirtualKeyboard> NameKeyboard;

	TWeakObjectPtr<ALTTMessDirector> Director;
	float AnimTime = 0.f;
};
