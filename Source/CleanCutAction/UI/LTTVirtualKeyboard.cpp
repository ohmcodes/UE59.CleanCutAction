// Clean! Cut! Action!

#include "UI/LTTVirtualKeyboard.h"
#include "UI/LTTUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "LTTVirtualKeyboard"

namespace LTTVirtualKeyboardPrivate // unique names: unity builds merge widget .cpps
{
	const TCHAR* KeyShift = TEXT("{SHIFT}");
	const TCHAR* KeyBack = TEXT("{BACK}");
	const TCHAR* KeySpace = TEXT("{SPACE}");
	const TCHAR* KeyDone = TEXT("{DONE}");
	const TCHAR* KeyClear = TEXT("{CLEAR}");

	constexpr float KbKeyHeight = 62.f;

	float KbEaseOutCubic(float T)
	{
		const float U = 1.f - T;
		return 1.f - U * U * U;
	}

	FButtonStyle KbKeyStyle(const FLinearColor& Fill)
	{
		FButtonStyle Style = LTTTheme::ButtonStyle(Fill);
		const FLinearColor Hover = FMath::Lerp(Fill, FLinearColor::White, 0.25f);
		FLinearColor Press = Fill * 0.8f;
		Press.A = Fill.A;
		Style.SetNormal(LTTTheme::Rounded(Fill, 12.f, LTTTheme::ButtonEdge(), 2.f));
		Style.SetHovered(LTTTheme::Rounded(Hover, 12.f, LTTTheme::WoodDark(), 2.f));
		Style.SetPressed(LTTTheme::Rounded(Press, 12.f, LTTTheme::WoodDark(), 2.f));
		Style.SetNormalPadding(FMargin(2.f, 2.f, 2.f, 4.f));
		Style.SetPressedPadding(FMargin(2.f, 4.f, 2.f, 2.f));
		return Style;
	}
}

using namespace LTTVirtualKeyboardPrivate;

// ---- Key ----

ULTTVirtualKey::ULTTVirtualKey(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitIsFocusable(false); // focus stays on the keyboard so physical keys keep working
}

void ULTTVirtualKey::InitKey(ULTTVirtualKeyboard* InOwner, const FString& InValue)
{
	Owner = InOwner;
	Value = InValue;
	if (!Label)
	{
		Label = Cast<UTextBlock>(GetContent());
	}
	SetTouchMethod(EButtonTouchMethod::DownAndUp);
	OnClicked.AddUniqueDynamic(this, &ULTTVirtualKey::HandleKeyClicked);
}

void ULTTVirtualKey::SetActive(bool bActive)
{
	if (!bUseActiveStyle || bActive == bShownActive)
	{
		return;
	}
	if (!bHasInactiveStyle)
	{
		InactiveStyle = GetStyle();
		bHasInactiveStyle = true;
	}
	bShownActive = bActive;
	SetStyle(bActive ? ActiveStyle : InactiveStyle);
}

void ULTTVirtualKey::HandleKeyClicked()
{
	if (ULTTVirtualKeyboard* Keyboard = Owner.Get())
	{
		Keyboard->PressKey(Value);
	}
}

// ---- Keyboard ----

ULTTVirtualKeyboard* ULTTVirtualKeyboard::Open(APlayerController* PC, UEditableTextBox* InTarget, int32 InMaxLength, const FText& Title,
	TSubclassOf<ULTTVirtualKeyboard> Class)
{
	if (!PC)
	{
		return nullptr;
	}
	Class = LTTTheme::PreferWBP<ULTTVirtualKeyboard>(Class, TEXT("WBP_VirtualKeyboard"));
	ULTTVirtualKeyboard* Keyboard = CreateWidget<ULTTVirtualKeyboard>(PC, Class);
	if (!Keyboard)
	{
		return nullptr;
	}
	Keyboard->Target = InTarget;
	Keyboard->MaxLength = FMath::Max(InMaxLength, 1);
	Keyboard->Text = InTarget ? InTarget->GetText().ToString() : FString();
	Keyboard->bShift = Keyboard->Text.IsEmpty();
	if (Keyboard->TitleText)
	{
		Keyboard->TitleText->SetText(Title);
	}
	Keyboard->RefreshKeyCaps();
	Keyboard->RefreshPreview();
	Keyboard->AddToViewport(200); // above the leaderboard (100) and every menu

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Keyboard->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	return Keyboard;
}

void ULTTVirtualKeyboard::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	GatherKeys();
	SetIsFocusable(true);
}

void ULTTVirtualKeyboard::GatherKeys()
{
	// Every key in the tree, code-built or placed in the designer.
	Keys.Reset();
	if (!WidgetTree)
	{
		return;
	}
	WidgetTree->ForEachWidget([this](UWidget* W)
	{
		if (ULTTVirtualKey* Key = Cast<ULTTVirtualKey>(W))
		{
			Key->InitKey(this, Key->Value);
			Keys.Add(Key);
		}
	});
}

ULTTVirtualKey* ULTTVirtualKeyboard::MakeKey(const FString& Value, const FText& Caption, const FLinearColor& Fill, int32 FontSize, FName Name)
{
	ULTTVirtualKey* Key = WidgetTree->ConstructWidget<ULTTVirtualKey>(ULTTVirtualKey::StaticClass(), Name);
	Key->SetStyle(KbKeyStyle(Fill));
	Key->SetBackgroundColor(FLinearColor::White);
	Key->SetColorAndOpacity(FLinearColor::White);
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
	LTTTheme::StyleText(Label, FontSize, LTTTheme::Ink());
	Label->SetJustification(ETextJustify::Center);
	Label->SetText(Caption);
	Key->SetContent(Label);
	Key->Label = Label;
	Key->Value = Value;
	return Key;
}

void ULTTVirtualKeyboard::AddKeyRow(UVerticalBox* Box, const TArray<FString>& Row)
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (const FString& Value : Row)
	{
		float Weight = 1.f;
		FLinearColor Fill = LTTTheme::Hex(0xFF, 0xFD, 0xF6);
		FText Caption = FText::FromString(Value);
		int32 FontSize = 24;
		if (Value == KeyShift)
		{
			Caption = LOCTEXT("Shift", "SHIFT");
			Fill = LTTTheme::ButtonCream();
			Weight = 1.5f;
			FontSize = 16;
		}
		else if (Value == KeyBack)
		{
			Caption = LOCTEXT("Back", "DEL");
			Fill = LTTTheme::ButtonRed();
			Weight = 1.5f;
			FontSize = 18;
		}
		else if (Value == KeySpace)
		{
			Caption = LOCTEXT("Space", "SPACE");
			Fill = LTTTheme::ButtonCream();
			Weight = 4.f;
			FontSize = 16;
		}
		else if (Value == KeyDone)
		{
			Caption = LOCTEXT("Done", "DONE");
			Fill = LTTTheme::ButtonGold();
			Weight = 2.2f;
			FontSize = 20;
		}
		else if (Value == KeyClear)
		{
			Caption = LOCTEXT("Clear", "CLEAR");
			Fill = LTTTheme::ButtonCream();
			Weight = 1.5f;
			FontSize = 16;
		}
		FString Name = TEXT("Key_");
		if (Value == KeyShift) Name += TEXT("Shift");
		else if (Value == KeyBack) Name += TEXT("Delete");
		else if (Value == KeySpace) Name += TEXT("Space");
		else if (Value == KeyDone) Name += TEXT("Done");
		else if (Value == KeyClear) Name += TEXT("Clear");
		else if (Value == TEXT("-")) Name += TEXT("Dash");
		else if (Value == TEXT("_")) Name += TEXT("Underscore");
		else if (Value == TEXT(".")) Name += TEXT("Period");
		else if (Value == TEXT("!")) Name += TEXT("Exclaim");
		else Name += Value.ToUpper();
		ULTTVirtualKey* Key = MakeKey(Value, Caption, Fill, FontSize, *Name);
		if (Value == KeyShift)
		{
			Key->ActiveStyle = KbKeyStyle(LTTTheme::ButtonGold()); // lit while the next letter is a capital
			Key->bUseActiveStyle = true;
		}
		if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Key))
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Weight;
			S->SetSize(Size);
			S->SetPadding(FMargin(3.f));
			S->SetVerticalAlignment(VAlign_Fill);
		}
	}
	USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("KeyRow%d"), Box->GetChildrenCount() ));
	Height->SetHeightOverride(KbKeyHeight);
	Height->SetContent(Line);
	Box->AddChildToVerticalBox(Height);
}

void ULTTVirtualKeyboard::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Light dim: the briefing stays readable behind. Tapping it closes the keyboard (NativeOnMouseButtonDown).
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	LTTTheme::StyleBackdrop(Dim, 0.35f);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Dim))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("KeyboardPanel"));
	LTTTheme::StylePanel(PanelBorder, 22.f);
	PanelBorder->SetPadding(FMargin(14.f, 12.f, 14.f, 14.f));
	KeyboardPanel = PanelBorder;
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(PanelBorder))
	{
		// Full width along the bottom, like a phone keyboard.
		S->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetAutoSize(true);
		S->SetOffsets(FMargin(12.f, -10.f, 12.f, 0.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("KeyColumn"));
	PanelBorder->SetContent(Column);

	// Preview: the keys may cover the text box, so the text being typed shows here too.
	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PreviewRow"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleText(TitleText, 18, LTTTheme::HeaderBrown());
	TitleText->SetText(LOCTEXT("Title", "YOUR NAME"));
	if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(TitleText))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(6.f, 0.f, 14.f, 0.f));
	}
	UBorder* Field = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PreviewField"));
	LTTTheme::ApplyBrush(Field, LTTTheme::Rounded(FLinearColor::White, -1.f, LTTTheme::WoodDark(), 3.f));
	Field->SetPadding(FMargin(20.f, 8.f));
	PreviewText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviewText"));
	LTTTheme::StyleText(PreviewText, 24, LTTTheme::Ink());
	Field->SetContent(PreviewText);
	if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(Field))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Top))
	{
		S->SetPadding(FMargin(3.f, 0.f, 3.f, 8.f));
	}

	AddKeyRow(Column, { TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("0") });
	AddKeyRow(Column, { TEXT("q"), TEXT("w"), TEXT("e"), TEXT("r"), TEXT("t"), TEXT("y"), TEXT("u"), TEXT("i"), TEXT("o"), TEXT("p") });
	AddKeyRow(Column, { TEXT("a"), TEXT("s"), TEXT("d"), TEXT("f"), TEXT("g"), TEXT("h"), TEXT("j"), TEXT("k"), TEXT("l"), KeyBack });
	AddKeyRow(Column, { KeyShift, TEXT("z"), TEXT("x"), TEXT("c"), TEXT("v"), TEXT("b"), TEXT("n"), TEXT("m"), TEXT("-"), TEXT("_") });
	AddKeyRow(Column, { KeyClear, TEXT("."), KeySpace, TEXT("!"), KeyDone });
}

void ULTTVirtualKeyboard::RefreshKeyCaps()
{
	for (ULTTVirtualKey* Key : Keys)
	{
		if (!Key || !Key->Label)
		{
			continue;
		}
		if (Key->Value.Len() == 1 && FChar::IsAlpha(Key->Value[0]))
		{
			Key->Label->SetText(FText::FromString(bShift ? Key->Value.ToUpper() : Key->Value.ToLower()));
		}
		else if (Key->Value == KeyShift)
		{
			Key->SetActive(bShift); // lit while the next letter will be a capital
		}
	}
}

void ULTTVirtualKeyboard::RefreshPreview()
{
	if (PreviewText)
	{
		const float Period = FMath::Max(CaretBlinkPeriod, 0.1f);
		const bool bCaret = FMath::Fmod(AnimTime, Period) < Period * 0.55f;
		PreviewText->SetText(FText::FromString(Text + (bCaret ? TEXT("|") : TEXT(" "))));
	}
}

void ULTTVirtualKeyboard::SetText(const FString& NewText)
{
	Text = NewText.Left(MaxLength);
	if (Target)
	{
		Target->SetText(FText::FromString(Text));
	}
	RefreshPreview();
}

void ULTTVirtualKeyboard::PressKey(const FString& Value)
{
	if (bClosed)
	{
		return;
	}
	if (Value == KeyDone)
	{
		Finish();
	}
	else if (Value == KeyBack)
	{
		SetText(Text.LeftChop(1));
		if (Text.IsEmpty() && !bShift)
		{
			bShift = true;
			RefreshKeyCaps();
		}
	}
	else if (Value == KeyClear)
	{
		SetText(FString());
		bShift = true;
		RefreshKeyCaps();
	}
	else if (Value == KeyShift)
	{
		bShift = !bShift;
		RefreshKeyCaps();
	}
	else if (Value == KeySpace)
	{
		if (!Text.IsEmpty() && !Text.EndsWith(TEXT(" ")))
		{
			SetText(Text + TEXT(" "));
			bShift = true; // capital after a space, like "Rookie Joe"
			RefreshKeyCaps();
		}
	}
	else if (Text.Len() < MaxLength)
	{
		const bool bLetter = Value.Len() == 1 && FChar::IsAlpha(Value[0]);
		SetText(Text + (bLetter && bShift ? Value.ToUpper() : Value));
		if (bLetter && bShift)
		{
			bShift = false; // one capital, then lower case
			RefreshKeyCaps();
		}
	}
}

void ULTTVirtualKeyboard::Finish()
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;
	Text.TrimStartAndEndInline();
	if (Target)
	{
		Target->SetText(FText::FromString(Text));
	}
	APlayerController* PC = GetOwningPlayer();
	RemoveFromParent();
	if (UUserWidget* Back = ReturnFocus.Get())
	{
		if (PC && Back->IsInViewport())
		{
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(Back->TakeWidget());
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
		}
	}
	OnClosed.Broadcast(Text);
}

void ULTTVirtualKeyboard::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime;
	RefreshPreview();
	// Slides up from below its spot.
	if (KeyboardPanel)
	{
		const float Time = FMath::Max(SlideTime, 0.01f);
		const float E = KbEaseOutCubic(FMath::Clamp(AnimTime / Time, 0.f, 1.f));
		if (AnimTime < Time + 0.05f)
		{
			KeyboardPanel->SetRenderTranslation(FVector2D(0.f, (1.f - E) * SlideDistance));
		}
	}
}

FReply ULTTVirtualKeyboard::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		Finish();
		return FReply::Handled();
	}
	if (Key == EKeys::BackSpace)
	{
		PressKey(KeyBack);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply ULTTVirtualKeyboard::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharEvent)
{
	const TCHAR Char = InCharEvent.GetCharacter();
	if (Char >= 32 && Char != 127 && Text.Len() < MaxLength)
	{
		SetText(Text + FString::Chr(Char));
		return FReply::Handled();
	}
	return Super::NativeOnKeyChar(InGeometry, InCharEvent);
}

FReply ULTTVirtualKeyboard::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A tap that no key took: outside the panel closes the keyboard, inside it is swallowed.
	if (KeyboardPanel && !KeyboardPanel->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		Finish();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
