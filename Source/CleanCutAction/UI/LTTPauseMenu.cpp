// Clean! Cut! Action!

#include "UI/LTTPauseMenu.h"
#include "CleanCutActionPlayerController.h"
#include "Audio/LTTMusicPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "LTTPauseMenu"

namespace
{
	const FLinearColor TapeYellow(1.f, 0.78f, 0.1f);
	const FLinearColor SoftWhite(0.95f, 0.95f, 0.92f);
	const FLinearColor Muted(0.7f, 0.7f, 0.68f);
	const FLinearColor ButtonGrey(0.75f, 0.75f, 0.75f);
	const FLinearColor QuitRed(1.f, 0.42f, 0.32f);

	float EaseOutCubic(float T)
	{
		const float U = 1.f - T;
		return 1.f - U * U * U;
	}
}

void ULTTPauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	SetIsFocusable(true); // receives P / Escape while input is UI-only
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &ULTTPauseMenu::HandleResumeClicked);
	}
	if (MusicButton)
	{
		MusicButton->OnClicked.AddDynamic(this, &ULTTPauseMenu::HandleMusicClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &ULTTPauseMenu::HandleQuitClicked);
	}
}

void ULTTPauseMenu::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.f;
	RefreshMusicLabel();
}

UTextBlock* ULTTPauseMenu::MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(2.f, 2.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	return Text;
}

UButton* ULTTPauseMenu::MakeButton(FName Name, const FText& Label, const FLinearColor& Color, UTextBlock** OutLabel) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(Color);
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 24, FLinearColor(0.08f, 0.08f, 0.08f));
	Text->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Text->SetJustification(ETextJustify::Center);
	Text->SetText(Label);
	Button->SetContent(Text);
	if (OutLabel)
	{
		*OutLabel = Text;
	}
	return Button;
}

void ULTTPauseMenu::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	CardBorder->SetBrushColor(FLinearColor(0.06f, 0.06f, 0.07f, 0.92f));
	CardBorder->SetPadding(FMargin(48.f, 36.f));
	Card = CardBorder;
	if (UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(CardBorder))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Width->SetMinDesiredWidth(360.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	TitleText = MakeText(TEXT("TitleText"), 56, TapeYellow);
	TitleText->SetText(LOCTEXT("Title", "CUT!"));
	TitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(TitleText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	UTextBlock* Subtitle = MakeText(TEXT("SubtitleText"), 18, Muted);
	Subtitle->SetText(LOCTEXT("Subtitle", "Game paused"));
	Subtitle->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Subtitle))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	ResumeButton = MakeButton(TEXT("ResumeButton"), LOCTEXT("Resume", "Resume"), TapeYellow);
	UTextBlock* MusicText = nullptr;
	MusicButton = MakeButton(TEXT("MusicButton"), FText::GetEmpty(), ButtonGrey, &MusicText);
	MusicLabel = MusicText;
	QuitButton = MakeButton(TEXT("QuitButton"), LOCTEXT("Quit", "Quit"), QuitRed);
	for (UButton* Button : { ResumeButton.Get(), MusicButton.Get(), QuitButton.Get() })
	{
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Button))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetPadding(FMargin(0.f, 6.f));
		}
	}

	UTextBlock* KeysHint = MakeText(TEXT("KeysHint"), 14, Muted);
	KeysHint->SetText(LOCTEXT("KeysHint", "P / Esc to resume"));
	KeysHint->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(KeysHint))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
}

void ULTTPauseMenu::RefreshMusicLabel()
{
	if (MusicLabel)
	{
		MusicLabel->SetText(ALTTMusicPlayer::IsMusicMuted() ? LOCTEXT("MusicOff", "Music: Off") : LOCTEXT("MusicOn", "Music: On"));
	}
}

void ULTTPauseMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime; // UMG ticks with real time, so this runs while paused

	// Card fades in and drops into place (0.25 s).
	if (Card)
	{
		const float E = EaseOutCubic(FMath::Clamp(AnimTime / 0.25f, 0.f, 1.f));
		Card->SetRenderOpacity(E);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - E) * -40.f));
	}
}

FReply ULTTPauseMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (!InKeyEvent.IsRepeat() && (Key == EKeys::P || Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right))
	{
		Resume();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULTTPauseMenu::HandleResumeClicked()
{
	Resume();
}

void ULTTPauseMenu::HandleMusicClicked()
{
	ToggleMusic();
}

void ULTTPauseMenu::HandleQuitClicked()
{
	QuitGame();
}

void ULTTPauseMenu::Resume()
{
	if (ACleanCutActionPlayerController* PC = GetOwningPlayer<ACleanCutActionPlayerController>())
	{
		PC->ClosePauseMenu();
	}
	else
	{
		RemoveFromParent();
	}
}

void ULTTPauseMenu::ToggleMusic()
{
	ALTTMusicPlayer::SetMusicMuted(!ALTTMusicPlayer::IsMusicMuted());
	RefreshMusicLabel();
}

void ULTTPauseMenu::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
