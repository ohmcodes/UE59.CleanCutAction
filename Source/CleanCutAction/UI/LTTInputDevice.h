// Copyright © 2026 Ohmcodes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputCoreTypes.h"
#include "LTTInputDevice.generated.h"

class UUserWidget;
class UWidget;
class APlayerController;

/** What the player last used. HUDs and menus show that device's controls. */
UENUM(BlueprintType)
enum class ELTTInputDevice : uint8
{
	KeyboardMouse,
	Gamepad,
	Touch
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLTTOnInputDeviceChanged, ELTTInputDevice, NewDevice);

/**
 * Tracks the last input device (keyboard/mouse, gamepad or touch) through a Slate input pre-processor, so every
 * screen can show the matching controls. Works the same on Windows, Wasm and the Steam Deck browser.
 *
 * Mouse movement alone doesn't switch to keyboard/mouse: Steam Input's trackpads move the mouse while the rest of
 * the controller is a gamepad, and the prompts shouldn't flicker. A key, a mouse click or a touch does switch.
 */
UCLASS()
class CLEANCUTACTION_API ULTTInputDeviceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The device the player last used (process-wide; the game is single player). */
	static ELTTInputDevice GetCurrent();
	static bool IsGamepad() { return GetCurrent() == ELTTInputDevice::Gamepad; }
	static bool IsTouch() { return GetCurrent() == ELTTInputDevice::Touch; }

	UFUNCTION(BlueprintPure, Category = "Input Device", meta = (DisplayName = "Get Input Device"))
	static ELTTInputDevice GetInputDevice() { return GetCurrent(); }

	UFUNCTION(BlueprintPure, Category = "Input Device", meta = (DisplayName = "Is Using Gamepad"))
	static bool IsUsingGamepad() { return IsGamepad(); }

	UFUNCTION(BlueprintPure, Category = "Input Device", meta = (DisplayName = "Is Using Touch"))
	static bool IsUsingTouch() { return IsTouch(); }

	/** Picks the keyboard or gamepad text for the current device (touch gets the keyboard text). */
	static FText Pick(const FText& KeyboardText, const FText& GamepadText) { return IsGamepad() ? GamepadText : KeyboardText; }

	/** Short label for a gamepad key in Xbox / Steam Deck layout: "A", "RT", "D-pad Up", "Menu". */
	UFUNCTION(BlueprintPure, Category = "Input Device")
	static FText GamepadKeyLabel(const FKey& Key);

	/** Label for whichever of the two keys matches the current device. */
	static FText KeyLabelFor(const FKey& KeyboardKey, const FKey& GamepadKey);

	/** Fires when the player switches device. */
	UPROPERTY(BlueprintAssignable, Category = "Input Device")
	FLTTOnInputDeviceChanged OnInputDeviceChanged;

	/** Sets the device and broadcasts to every subsystem instance (called by the input pre-processor). */
	static void SetCurrent(ELTTInputDevice NewDevice);

	// ---- Gamepad menu navigation ----

	/** Focuses Preferred if it's usable, otherwise the first visible, enabled, focusable button in Menu's tree.
	 *  Returns the widget that got focus (null if none). */
	static UWidget* FocusFirstButton(UUserWidget* Menu, UWidget* Preferred = nullptr);

	/** Call from a menu's NativeTick. While the gamepad is in use and nothing inside Menu (or another menu on top)
	 *  has focus, puts focus on Preferred or the first button so the left stick / d-pad and A work. */
	static void EnsureGamepadFocus(UUserWidget* Menu, UWidget* Preferred = nullptr);

	/** True for keys that mean "back / cancel" in a menu (gamepad B). */
	static bool IsBackKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::OculusTouch_Right_B_Click || Key == EKeys::ValveIndex_Right_B_Click;
	}

private:
	TSharedPtr<class FLTTInputDeviceProcessor> Processor;
};
