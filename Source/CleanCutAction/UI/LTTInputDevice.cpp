// Copyright © 2026 Ohmcodes. All Rights Reserved.

#include "UI/LTTInputDevice.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/PanelWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "LTTInputDevice"

namespace
{
	ELTTInputDevice GLTTInputDevice = ELTTInputDevice::KeyboardMouse;
	bool GLTTInputDeviceInitialised = false;
	TArray<TWeakObjectPtr<ULTTInputDeviceSubsystem>> GLTTInputDeviceSubsystems;

	/** Sticks under this don't count as "using the gamepad" (resting drift). */
	constexpr float LTTGamepadAnalogThreshold = 0.4f;

	/** After any gamepad input, mouse clicks and keys don't switch away from the gamepad for this long. Steam Input
	 *  sends the Steam Controller / Deck trackpads and some buttons as mouse clicks or keyboard keys, mixed in with
	 *  real gamepad input; without this the prompts flip back to the keyboard every time. */
	constexpr double LTTGamepadStickySeconds = 1.5;
	double GLTTLastGamepadInputTime = -1000.0;

	/** Switch to keyboard / mouse or touch, unless the gamepad was used a moment ago. */
	void LTTSetNonGamepad(ELTTInputDevice Device, const FKey& Cause)
	{
		if (ULTTInputDeviceSubsystem::GetCurrent() == ELTTInputDevice::Gamepad
			&& FPlatformTime::Seconds() - GLTTLastGamepadInputTime < LTTGamepadStickySeconds)
		{
			return;
		}
		if (ULTTInputDeviceSubsystem::GetCurrent() != Device)
		{
			UE_LOG(LogTemp, Log, TEXT("LTTInputDevice: %s (from %s)"), Device == ELTTInputDevice::Touch ? TEXT("touch") : TEXT("keyboard/mouse"), *Cause.ToString());
		}
		ULTTInputDeviceSubsystem::SetCurrent(Device);
	}

	void LTTSetGamepad(const FKey& Cause)
	{
		GLTTLastGamepadInputTime = FPlatformTime::Seconds();
		if (ULTTInputDeviceSubsystem::GetCurrent() != ELTTInputDevice::Gamepad)
		{
			UE_LOG(LogTemp, Log, TEXT("LTTInputDevice: gamepad (from %s)"), *Cause.ToString());
		}
		ULTTInputDeviceSubsystem::SetCurrent(ELTTInputDevice::Gamepad);
	}

	/** Buttons whose look was swapped to the focus style, with the style to put back. */
	TMap<TWeakObjectPtr<UButton>, FButtonStyle> GLTTFocusStyled;
}

/** Watches every input event before Slate routes it, and never consumes any. */
class FLTTInputDeviceProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key.IsGamepadKey())
		{
			LTTSetGamepad(Key);
		}
		else if (!Key.IsTouch() && !Key.IsMouseButton())
		{
			LTTSetNonGamepad(ELTTInputDevice::KeyboardMouse, Key);
		}
		return false;
	}

	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
	{
		if (InAnalogInputEvent.GetKey().IsGamepadKey() && FMath::Abs(InAnalogInputEvent.GetAnalogValue()) > LTTGamepadAnalogThreshold)
		{
			LTTSetGamepad(InAnalogInputEvent.GetKey());
		}
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		LTTSetNonGamepad(MouseEvent.IsTouchEvent() ? ELTTInputDevice::Touch : ELTTInputDevice::KeyboardMouse, MouseEvent.GetEffectingButton());
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("LTTInputDevice"); }
};

void ULTTInputDeviceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GLTTInputDeviceSubsystems.Add(this);

	if (!GLTTInputDeviceInitialised)
	{
		GLTTInputDeviceInitialised = true;
		// Start on what the player most likely holds: touch on phones/tablets, a gamepad if one is plugged in
		// (Steam Deck, a PC with a controller), keyboard and mouse otherwise. The first input corrects it.
		if (SVirtualJoystick::ShouldDisplayTouchInterface())
		{
			GLTTInputDevice = ELTTInputDevice::Touch;
		}
		else if (FSlateApplication::IsInitialized() && FSlateApplication::Get().IsGamepadAttached())
		{
			GLTTInputDevice = ELTTInputDevice::Gamepad;
		}
	}

	if (FSlateApplication::IsInitialized())
	{
		Processor = MakeShared<FLTTInputDeviceProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(Processor);
	}
}

void ULTTInputDeviceSubsystem::Deinitialize()
{
	if (Processor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(Processor);
	}
	Processor.Reset();
	GLTTInputDeviceSubsystems.Remove(this);
	Super::Deinitialize();
}

ELTTInputDevice ULTTInputDeviceSubsystem::GetCurrent()
{
	return GLTTInputDevice;
}

void ULTTInputDeviceSubsystem::SetCurrent(ELTTInputDevice NewDevice)
{
	if (GLTTInputDevice == NewDevice)
	{
		return;
	}
	GLTTInputDevice = NewDevice;
	// Copy: a listener might create or remove a subsystem.
	const TArray<TWeakObjectPtr<ULTTInputDeviceSubsystem>> Subsystems = GLTTInputDeviceSubsystems;
	for (const TWeakObjectPtr<ULTTInputDeviceSubsystem>& Weak : Subsystems)
	{
		if (ULTTInputDeviceSubsystem* Subsystem = Weak.Get())
		{
			Subsystem->OnInputDeviceChanged.Broadcast(NewDevice);
		}
	}
}

FText ULTTInputDeviceSubsystem::GamepadKeyLabel(const FKey& Key)
{
	// Xbox / Steam Deck face-button names; both put A at the bottom.
	if (Key == EKeys::Gamepad_FaceButton_Bottom) return LOCTEXT("PadA", "A");
	if (Key == EKeys::Gamepad_FaceButton_Right) return LOCTEXT("PadB", "B");
	if (Key == EKeys::Gamepad_FaceButton_Left) return LOCTEXT("PadX", "X");
	if (Key == EKeys::Gamepad_FaceButton_Top) return LOCTEXT("PadY", "Y");
	if (Key == EKeys::Gamepad_LeftShoulder) return LOCTEXT("PadLB", "LB");
	if (Key == EKeys::Gamepad_RightShoulder) return LOCTEXT("PadRB", "RB");
	if (Key == EKeys::Gamepad_LeftTrigger || Key == EKeys::Gamepad_LeftTriggerAxis) return LOCTEXT("PadLT", "LT");
	if (Key == EKeys::Gamepad_RightTrigger || Key == EKeys::Gamepad_RightTriggerAxis) return LOCTEXT("PadRT", "RT");
	if (Key == EKeys::Gamepad_LeftThumbstick) return LOCTEXT("PadL3", "L3");
	if (Key == EKeys::Gamepad_RightThumbstick) return LOCTEXT("PadR3", "R3");
	if (Key == EKeys::Gamepad_DPad_Up) return LOCTEXT("PadUp", "D-pad Up");
	if (Key == EKeys::Gamepad_DPad_Down) return LOCTEXT("PadDown", "D-pad Down");
	if (Key == EKeys::Gamepad_DPad_Left) return LOCTEXT("PadLeft", "D-pad Left");
	if (Key == EKeys::Gamepad_DPad_Right) return LOCTEXT("PadRight", "D-pad Right");
	if (Key == EKeys::Gamepad_Special_Right) return LOCTEXT("PadMenu", "Menu");
	if (Key == EKeys::Gamepad_Special_Left) return LOCTEXT("PadView", "View");
	if (Key == EKeys::Gamepad_Left2D) return LOCTEXT("PadLS", "Left stick");
	if (Key == EKeys::Gamepad_Right2D) return LOCTEXT("PadRS", "Right stick");
	return Key.GetDisplayName(false);
}

FText ULTTInputDeviceSubsystem::KeyLabelFor(const FKey& KeyboardKey, const FKey& GamepadKey)
{
	if (IsGamepad() && GamepadKey.IsValid())
	{
		return GamepadKeyLabel(GamepadKey);
	}
	return KeyboardKey.IsValid() ? KeyboardKey.GetDisplayName() : FText::GetEmpty();
}

namespace
{
	/** Visible on screen: every ancestor is visible and, inside a widget switcher, on the active page. */
	bool LTTIsShown(UWidget* Widget)
	{
		for (UWidget* W = Widget; W; )
		{
			if (!W->IsVisible())
			{
				return false;
			}
			UPanelWidget* Parent = W->GetParent();
			if (UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Parent))
			{
				if (Switcher->GetActiveWidget() != W)
				{
					return false;
				}
			}
			W = Parent;
		}
		return true;
	}

	bool LTTCanFocus(UWidget* Widget)
	{
		if (!Widget || !LTTIsShown(Widget) || !Widget->GetIsEnabled())
		{
			return false;
		}
		const TSharedPtr<SWidget> Slate = Widget->GetCachedWidget();
		return Slate.IsValid() && Slate->SupportsKeyboardFocus() && Slate->IsEnabled();
	}

	void LTTSetFocus(UWidget* Widget, APlayerController* PC)
	{
		if (PC)
		{
			Widget->SetUserFocus(PC);
		}
		else
		{
			Widget->SetKeyboardFocus();
		}
	}

	/** Gold outline + hover fill on the button the gamepad is on, so the player can see where they are. */
	void LTTUpdateFocusLook(UUserWidget* Menu)
	{
		if (!Menu->WidgetTree)
		{
			return;
		}
		const bool bShowFocus = ULTTInputDeviceSubsystem::IsGamepad();
		Menu->WidgetTree->ForEachWidget([bShowFocus](UWidget* Widget)
		{
			UButton* Button = Cast<UButton>(Widget);
			if (!Button)
			{
				return;
			}
			const bool bFocused = bShowFocus && Button->HasKeyboardFocus();
			const TWeakObjectPtr<UButton> Key(Button);
			if (bFocused && !GLTTFocusStyled.Contains(Key))
			{
				const FButtonStyle Original = Button->GetStyle();
				FButtonStyle Focused = Original;
				FSlateBrush Brush = Original.Hovered;
				Brush.OutlineSettings.Color = FSlateColor(FLinearColor(1.f, 0.72f, 0.12f));
				Brush.OutlineSettings.Width = FMath::Max(Brush.OutlineSettings.Width, 4.f);
				Focused.SetNormal(Brush);
				Focused.SetHovered(Brush);
				Button->SetStyle(Focused);
				GLTTFocusStyled.Add(Key, Original);
			}
			else if (!bFocused)
			{
				if (const FButtonStyle* Original = GLTTFocusStyled.Find(Key))
				{
					Button->SetStyle(*Original);
					GLTTFocusStyled.Remove(Key);
				}
			}
		});
		// Drop entries for buttons that are gone.
		for (auto It = GLTTFocusStyled.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
}

UWidget* ULTTInputDeviceSubsystem::FocusFirstButton(UUserWidget* Menu, UWidget* Preferred)
{
	if (!Menu || !Menu->WidgetTree)
	{
		return nullptr;
	}
	APlayerController* PC = Menu->GetOwningPlayer();
	if (LTTCanFocus(Preferred))
	{
		LTTSetFocus(Preferred, PC);
		return Preferred;
	}
	UWidget* Found = nullptr;
	Menu->WidgetTree->ForEachWidget([&Found](UWidget* Widget)
	{
		if (!Found && Cast<UButton>(Widget) && LTTCanFocus(Widget))
		{
			Found = Widget;
		}
	});
	if (Found)
	{
		LTTSetFocus(Found, PC);
	}
	return Found;
}

void ULTTInputDeviceSubsystem::EnsureGamepadFocus(UUserWidget* Menu, UWidget* Preferred)
{
	if (!Menu || !Menu->IsInViewport() || !Menu->IsVisible())
	{
		return;
	}
	LTTUpdateFocusLook(Menu);
	if (!IsGamepad() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	if (Menu->HasFocusedDescendants())
	{
		// Fine unless the focused widget went out of sight (a page flipped, a row collapsed).
		bool bHiddenFocus = false;
		if (Menu->WidgetTree)
		{
			Menu->WidgetTree->ForEachWidget([&bHiddenFocus](UWidget* Widget)
			{
				if (!bHiddenFocus && Widget->HasKeyboardFocus() && !LTTIsShown(Widget))
				{
					bHiddenFocus = true;
				}
			});
		}
		if (bHiddenFocus)
		{
			FocusFirstButton(Menu, Preferred);
		}
		return;
	}

	// Leave focus alone if another menu (a panel or keyboard opened on top) holds it.
	APlayerController* PC = Menu->GetOwningPlayer();
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	TSharedPtr<const FSlateUser> User = LP ? LP->GetSlateUser() : nullptr;
	if (!User.IsValid())
	{
		User = FSlateApplication::Get().GetUser(0);
	}
	const TSharedPtr<SWidget> Focused = User.IsValid() ? User->GetFocusedWidget() : nullptr;
	if (Focused.IsValid())
	{
		const TSharedPtr<SWidget> MenuSlate = Menu->GetCachedWidget();
		const TSharedPtr<SViewport> Viewport = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetGameViewportWidget() : nullptr;
		const bool bOnMenu = MenuSlate.IsValid() && Focused == MenuSlate;
		const bool bOnGame = Viewport.IsValid() && Focused == StaticCastSharedPtr<SWidget>(Viewport);
		if (!bOnMenu && !bOnGame)
		{
			return;
		}
	}
	FocusFirstButton(Menu, Preferred);
}

#undef LOCTEXT_NAMESPACE
