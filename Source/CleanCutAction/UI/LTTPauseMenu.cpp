// Clean! Cut! Action!

#include "UI/LTTPauseMenu.h"
#include "UI/LTTUITheme.h"
#include "CleanCutActionPlayerController.h"
#include "Audio/LTTMusicPlayer.h"
#include "Interaction/LTTMessDirector.h"
#include "UI/LTTLeaderboardSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "LTTPauseMenu"

namespace // names prefixed: unity builds can put this file next to other widgets with the same helpers
{
	// Cafe theme (UI/LTTUITheme.h).
	const FLinearColor PauseTapeYellow = LTTTheme::ButtonGold();
	const FLinearColor PauseSoftWhite = LTTTheme::Ink();
	const FLinearColor PauseMuted = LTTTheme::InkMuted();
	const FLinearColor PauseButtonGrey = LTTTheme::ButtonCream();
	const FLinearColor PauseQuitRed = LTTTheme::ButtonRed();

	float PauseEaseOutCubic(float T)
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
	if (!CheatButton)
	{
		AddCheatButton(); // WBP_PauseMenu was baked before this existed
	}
	if (CheatButton)
	{
		CheatButton->OnClicked.AddDynamic(this, &ULTTPauseMenu::HandleCheatClicked);
	}
}

void ULTTPauseMenu::AddCheatButton()
{
	UCanvasPanel* Root = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (!Root)
	{
		return;
	}
	CheatButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CheatButton"));
	FSlateBrush NoDraw;
	NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
	FButtonStyle Style;
	Style.SetNormal(NoDraw).SetHovered(NoDraw).SetPressed(NoDraw).SetDisabled(NoDraw);
	Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
	CheatButton->SetStyle(Style);
	CheatButton->SetContent(LTTTheme::MakeArtIcon(WidgetTree, TEXT("CheatIcon"), TEXT("coffee"), 32.f));
	CheatButton->SetRenderOpacity(0.55f);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(CheatButton))
	{
		S->SetAnchors(FAnchors(1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(-20.f, -20.f));
		S->SetAutoSize(true);
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
	LTTTheme::StyleText(Text, FontSize, Color);
	return Text;
}

UButton* ULTTPauseMenu::MakeButton(FName Name, const FText& Label, const FLinearColor& Color, UTextBlock** OutLabel) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	LTTTheme::StyleButton(Button, Color);
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 24, LTTTheme::Ink());
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
	LTTTheme::StyleBackdrop(Dim, 0.55f);
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	LTTTheme::StylePanel(CardBorder);
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

	// "CUT!" on the ribbon banner, with Marlowe's megaphone.
	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 56);
	TitleText->SetText(LOCTEXT("Title", "CUT!"));
	TitleText->SetJustification(ETextJustify::Center);
	LTTTheme::FillRibbonBanner(WidgetTree, Banner, TitleText, TEXT("megaphone"), nullptr, 72.f);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	UTextBlock* Subtitle = MakeText(TEXT("SubtitleText"), 18, PauseMuted);
	Subtitle->SetText(LOCTEXT("Subtitle", "Game paused"));
	Subtitle->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Subtitle))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	ResumeButton = MakeButton(TEXT("ResumeButton"), LOCTEXT("Resume", "Resume"), PauseTapeYellow);
	UTextBlock* MusicText = nullptr;
	MusicButton = MakeButton(TEXT("MusicButton"), FText::GetEmpty(), PauseButtonGrey, &MusicText);
	MusicText->Rename(TEXT("MusicLabel")); // same name as the property, so a WBP_ child binds it
	MusicLabel = MusicText;
	QuitButton = MakeButton(TEXT("QuitButton"), LOCTEXT("Quit", "Quit"), PauseQuitRed);
	for (UButton* Button : { ResumeButton.Get(), MusicButton.Get(), QuitButton.Get() })
	{
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Button))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetPadding(FMargin(0.f, 6.f));
		}
	}

	UTextBlock* KeysHint = MakeText(TEXT("KeysHint"), 14, PauseMuted);
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
		const float E = PauseEaseOutCubic(FMath::Clamp(AnimTime / 0.25f, 0.f, 1.f));
		Card->SetRenderOpacity(E);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - E) * -40.f));
	}
	if (CheatButton)
	{
		CheatButton->SetRenderOpacity(CheatButton->IsHovered() ? 1.f : 0.55f);
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

void ULTTPauseMenu::HandleCheatClicked()
{
	const ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this);
	if (!Board || !Board->GetPlayerName().Equals(TEXT("iPoNet"), ESearchCase::CaseSensitive))
	{
		return; // just a coffee cup for everyone else
	}
	if (ALTTMessDirector* Director = ALTTMessDirector::Get(GetWorld()))
	{
		Resume(); // unpause first so the end screen (if nothing is left) isn't stuck behind the menu
		Director->CheatPlaceAll(5); // same as LTT.PlaceAll
	}
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
