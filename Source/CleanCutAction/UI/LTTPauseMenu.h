// Clean! Cut! Action! - Pause menu: Resume, Music on/off, Quit. Opened and closed by
// ACleanCutActionPlayerController (P / Escape / gamepad Start); the game is paused while it's up.
// Builds its own layout in C++; a Widget Blueprint child can restyle it by naming widgets like the ones below.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTPauseMenu.generated.h"

class UTextBlock;
class UButton;
class UWidget;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTPauseMenu : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Closes the menu and unpauses. */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void Resume();

	/** Mutes / unmutes the background music. */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void ToggleMusic();

	/** Quits the game. */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void QuitGame();

	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> MusicButton;

	/** Shows "Music: On" / "Music: Off". */
	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MusicLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	/** The menu card (fades and drops in). */
	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Card;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void BuildDefaultLayout();
	UTextBlock* MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const;
	UButton* MakeButton(FName Name, const FText& Label, const FLinearColor& Color, UTextBlock** OutLabel = nullptr) const;
	void RefreshMusicLabel();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleMusicClicked();

	UFUNCTION()
	void HandleQuitClicked();

	float AnimTime = 0.f;
};
