// Clean! Cut! Action! - On-screen keyboard for phones and tablets. The Wasm build has no platform virtual
// keyboard (tapping a Slate text box in the browser never brings up the phone's keyboard), so text entry on
// touch goes through this widget instead. Opened by ULTTBriefingScreen when the name box gets focus on touch.
// It types into a target UEditableTextBox live and shows its own preview line above the keys (the keys can
// cover the text box). Done / tapping outside closes it; physical keys work too (tablets with keyboards).
// Builds its own layout in C++ when used bare; WBP_VirtualKeyboard is the baked, designer-editable version
// (any ULTTVirtualKey in its tree works: set the key's Value in Details, the keyboard finds keys at runtime).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LTTVirtualKeyboard.generated.h"

class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
class ULTTVirtualKeyboard;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnKeyboardClosed, const FString&, FinalText);

/** One key: remembers what it types and reports clicks to its keyboard. */
UCLASS(meta = (DisplayName = "Virtual Key"))
class CLEANCUTACTION_API ULTTVirtualKey : public UButton
{
	GENERATED_BODY()

public:
	ULTTVirtualKey(const FObjectInitializer& ObjectInitializer);

	void InitKey(ULTTVirtualKeyboard* InOwner, const FString& InValue);

	/** What the key types. Special keys: {SHIFT} {BACK} {SPACE} {DONE} {CLEAR}. A single letter shows upper or lower
	 *  case on its label with Shift. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Key")
	FString Value;

	/** Style while the key is "on" (Shift with the next letter a capital). Used only if bUseActiveStyle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Key", meta = (EditCondition = "bUseActiveStyle"))
	FButtonStyle ActiveStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Key", meta = (InlineEditConditionToggle))
	bool bUseActiveStyle = false;

	/** Shows ActiveStyle (on) or the key's own style (off). */
	void SetActive(bool bActive);

	/** The key's caption: its content, if that's a text block. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

private:
	UFUNCTION()
	void HandleKeyClicked();

	TWeakObjectPtr<ULTTVirtualKeyboard> Owner;
	FButtonStyle InactiveStyle;
	bool bHasInactiveStyle = false;
	bool bShownActive = false;
};

UCLASS(Blueprintable)
class CLEANCUTACTION_API ULTTVirtualKeyboard : public UUserWidget
{
	GENERATED_BODY()
	friend class ULTTWidgetBaker; // editor tool: bakes BuildDefaultLayout into a WBP_ child

public:
	/** Opens a keyboard for Target over everything else and gives it UI focus. Class defaults to WBP_VirtualKeyboard,
	 *  or the bare C++ layout if that asset is missing. */
	static ULTTVirtualKeyboard* Open(APlayerController* PC, UEditableTextBox* Target, int32 MaxLength, const FText& Title,
		TSubclassOf<ULTTVirtualKeyboard> Class = nullptr);

	/** Code-built layout, used when the widget has no designer tree (and by the widget baker). */
	void BuildDefaultLayout();

	/** Closes and fires OnClosed with the text. Focus goes back to ReturnFocus if set. */
	UFUNCTION(BlueprintCallable, Category = "Keyboard")
	void Finish();

	UFUNCTION(BlueprintPure, Category = "Keyboard")
	const FString& GetText() const { return Text; }

	UPROPERTY(BlueprintAssignable, Category = "Keyboard")
	FLTTOnKeyboardClosed OnClosed;

	/** Widget that gets UI focus back when the keyboard closes. */
	TWeakObjectPtr<UUserWidget> ReturnFocus;

	/** Called by the keys. */
	void PressKey(const FString& Value);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void GatherKeys();
	void AddKeyRow(UVerticalBox* Box, const TArray<FString>& Keys);
	ULTTVirtualKey* MakeKey(const FString& Value, const FText& Caption, const FLinearColor& Fill, int32 FontSize, FName Name);
	void SetText(const FString& NewText);
	void RefreshKeyCaps();
	void RefreshPreview();

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> Target;

	/** The text being typed, with a blinking caret. */
	UPROPERTY(BlueprintReadOnly, Category = "Keyboard", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PreviewText;

	/** Set from Open's Title ("YOUR NAME"). */
	UPROPERTY(BlueprintReadOnly, Category = "Keyboard", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** The key panel: slides up on open; a tap outside it closes the keyboard. */
	UPROPERTY(BlueprintReadOnly, Category = "Keyboard", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> KeyboardPanel;

	/** How far below its spot the panel starts its slide-up, and how long the slide takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyboard")
	float SlideDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyboard")
	float SlideTime = 0.2f;

	/** Caret blink period in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyboard")
	float CaretBlinkPeriod = 1.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULTTVirtualKey>> Keys;

	FString Text;
	int32 MaxLength = 24;
	bool bShift = true; // first letter capitalised
	bool bClosed = false;
	float AnimTime = 0.f;
};
