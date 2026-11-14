// Clean! Cut! Action! - Round music on/off button for the touch HUD, placed just left of UI_TouchSimple's pause
// button (top right). Created by ACleanCutActionPlayerController next to the touch controls. Shows a speaker
// with sound waves (T_Icon_MusicOn) or crossed out (T_Icon_MusicOff), following ALTTMusicPlayer's mute state
// (so it stays in step with the pause menu's music button). Builds its own layout in C++.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTMusicToggle.generated.h"

class UButton;
class UTexture2D;
class UImage;

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTMusicToggle : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	UFUNCTION(BlueprintCallable, Category = "Music")
	void ToggleMusic();

	UPROPERTY(BlueprintReadOnly, Category = "Music", meta = (BindWidgetOptional))
	TObjectPtr<UButton> MusicButton;

	UPROPERTY(BlueprintReadOnly, Category = "Music", meta = (BindWidgetOptional))
	TObjectPtr<UImage> MusicIcon;

	/** Same size and top margin as UI_TouchSimple's pause button; RightOffset puts it to the pause button's left. */
	/** Icon while music plays / while it's muted. Empty: T_Icon_MusicOn / T_Icon_MusicOff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TObjectPtr<UTexture2D> MusicOnIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TObjectPtr<UTexture2D> MusicOffIcon;

	/** Icon opacity while muted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0", ClampMax = "1"))
	float MutedOpacity = 0.7f;

	/** Code layout only (the bare C++ class); in WBP_MusicToggle move the button in the designer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float ButtonSize = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float TopOffset = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float RightOffset = 150.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildDefaultLayout();
	void RefreshIcon();

	UFUNCTION()
	void HandleClicked();

	int32 ShownState = -1; // 0 = on, 1 = muted
};
