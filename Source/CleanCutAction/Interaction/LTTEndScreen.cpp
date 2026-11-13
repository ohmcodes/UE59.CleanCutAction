// Clean! Cut! Action!

#include "Interaction/LTTEndScreen.h"
#include "UI/LTTUITheme.h"
#include "Interaction/LTTMessDirector.h"
#include "UI/LTTLeaderboardPanel.h"
#include "UI/LTTLeaderboardSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "LTTEndScreen"

namespace
{
	// Cafe theme (UI/LTTUITheme.h).
	const FLinearColor EndTapeYellow = LTTTheme::ButtonGold();
	const FLinearColor EndSoftWhite = LTTTheme::Ink();
	const FLinearColor EndMuted = LTTTheme::InkMuted();
	const FLinearColor RedTitle = LTTTheme::Hex(0xFF, 0xC2, 0xAE); // salmon letters on the banner for TIME'S UP

	float EaseOutBounce(float T)
	{
		const float N = 7.5625f, D = 2.75f;
		if (T < 1.f / D) { return N * T * T; }
		if (T < 2.f / D) { T -= 1.5f / D; return N * T * T + 0.75f; }
		if (T < 2.5f / D) { T -= 2.25f / D; return N * T * T + 0.9375f; }
		T -= 2.625f / D;
		return N * T * T + 0.984375f;
	}

	float EndEaseOutCubic(float T)
	{
		const float U = 1.f - T;
		return 1.f - U * U * U;
	}

	FText FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(FMath::FloorToInt(Seconds), 0);
		FNumberFormattingOptions TwoDigits;
		TwoDigits.MinimumIntegralDigits = 2;
		return FText::Format(LOCTEXT("Time", "{0}:{1}"), FText::AsNumber(Total / 60), FText::AsNumber(Total % 60, &TwoDigits));
	}
}

void ULTTEndScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (PlayAgainButton)
	{
		PlayAgainButton->OnClicked.AddDynamic(this, &ULTTEndScreen::HandlePlayAgainClicked);
	}
	if (KeepPlayingButton)
	{
		KeepPlayingButton->OnClicked.AddDynamic(this, &ULTTEndScreen::HandleKeepPlayingClicked);
	}
	if (LeaderboardButton)
	{
		LeaderboardButton->OnClicked.AddDynamic(this, &ULTTEndScreen::HandleLeaderboardClicked);
	}
	if (FullLeaderboardButton)
	{
		FullLeaderboardButton->OnClicked.AddDynamic(this, &ULTTEndScreen::HandleFullLeaderboardClicked);
	}
}

UTextBlock* ULTTEndScreen::MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	LTTTheme::StyleText(Text, FontSize, Color);
	return Text;
}

UButton* ULTTEndScreen::MakeButton(FName Name, const FText& Label, const FLinearColor& Color) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	LTTTheme::StyleButton(Button, Color);
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 22, LTTTheme::Ink());
	Text->SetText(Label);
	Button->SetContent(Text);
	return Button;
}

void ULTTEndScreen::AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock* Value) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelText = MakeText(NAME_None, 22, EndMuted);
	LabelText->SetText(Label);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(Value))
	{
		ValueSlot->SetHorizontalAlignment(HAlign_Right);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 4.f));
	}
}

void ULTTEndScreen::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Darken the game behind.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	LTTTheme::StyleBackdrop(Dim, 0.6f);
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	// The call-sheet card.
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
	Width->SetMinDesiredWidth(520.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	// Title on the ribbon banner, with a clapperboard.
	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 60);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	LTTTheme::FillRibbonBanner(WidgetTree, Banner, TitleText, TEXT("clapperboard"), nullptr, 76.f);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	SubtitleText = MakeText(TEXT("SubtitleText"), 20, EndSoftWhite);
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(SubtitleText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 4.f, 0.f, 24.f));
	}

	TimeValue = MakeText(TEXT("TimeValue"), 22, EndSoftWhite);
	ReturnedValue = MakeText(TEXT("ReturnedValue"), 22, EndSoftWhite);
	DestroyedValue = MakeText(TEXT("DestroyedValue"), 22, LTTTheme::Bad());
	HintsValue = MakeText(TEXT("HintsValue"), 22, EndSoftWhite);
	TimeBonusValue = MakeText(TEXT("TimeBonusValue"), 22, LTTTheme::Good());
	PointsValue = MakeText(TEXT("PointsValue"), 36, LTTTheme::Coin());
	AddStatRow(Column, LOCTEXT("RowTime", "Time on set"), TimeValue);
	AddStatRow(Column, LOCTEXT("RowReturned", "Props returned"), ReturnedValue);
	AddStatRow(Column, LOCTEXT("RowDestroyed", "Props destroyed"), DestroyedValue);
	AddStatRow(Column, LOCTEXT("RowHints", "Hints used"), HintsValue);
	AddStatRow(Column, LOCTEXT("RowTimeBonus", "Time bonus"), TimeBonusValue);
	AddStatRow(Column, LOCTEXT("RowPoints", "Points"), PointsValue);

	RankText = MakeText(TEXT("RankText"), 18, LTTTheme::HeaderBrown());
	RankText->SetJustification(ETextJustify::Center);
	RankText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(RankText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	PlayAgainButton = MakeButton(TEXT("PlayAgainButton"), LOCTEXT("PlayAgain", "Play Again"), EndTapeYellow);
	KeepPlayingButton = MakeButton(TEXT("KeepPlayingButton"), LOCTEXT("KeepPlaying", "Keep Playing"), LTTTheme::ButtonCream());
	for (UButton* Button : { PlayAgainButton.Get(), KeepPlayingButton.Get() })
	{
		if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(Button))
		{
			S->SetPadding(FMargin(8.f, 0.f));
		}
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Buttons))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 28.f, 0.f, 0.f));
	}

	// Leaderboard: the in-game top 10, and the website.
	UHorizontalBox* BoardButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LeaderboardButtons"));
	LeaderboardButton = MakeButton(TEXT("LeaderboardButton"), LOCTEXT("Leaderboard", "Leaderboard"), LTTTheme::ButtonCream());
	FullLeaderboardButton = MakeButton(TEXT("FullLeaderboardButton"), LOCTEXT("FullLeaderboard", "Full Leaderboard"), LTTTheme::ButtonCream());
	for (UButton* Button : { LeaderboardButton.Get(), FullLeaderboardButton.Get() })
	{
		if (UHorizontalBoxSlot* S = BoardButtons->AddChildToHorizontalBox(Button))
		{
			S->SetPadding(FMargin(8.f, 0.f));
		}
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(BoardButtons))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
}

void ULTTEndScreen::Setup(ALTTMessDirector* InDirector, bool bInSetClean)
{
	Director = InDirector;
	bSetClean = bInSetClean;
	FinalPoints = InDirector ? InDirector->GetScore() : 0;
	AnimTime = 0.f;
	ApplyTexts();
	SubmitRun();
}

void ULTTEndScreen::SubmitRun()
{
	ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this);
	const ALTTMessDirector* D = Director.Get();
	if (!Board || !D || !bSubmitToLeaderboard || !Board->bEnabled)
	{
		if (RankText)
		{
			RankText->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	Board->OnRunSubmitted.AddUniqueDynamic(this, &ULTTEndScreen::HandleRunSubmitted);
	if (RankText)
	{
		RankText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RankText->SetColorAndOpacity(FSlateColor(LTTTheme::InkMuted()));
		RankText->SetText(FText::Format(LOCTEXT("Sending", "Sending {0}'s take to the leaderboard..."), FText::FromString(Board->GetPlayerName())));
	}
	Board->SubmitRun(FinalPoints, D->GetTimeOnSet(), bSetClean, D->bUseTimer, D->GetPlacedCount(), D->GetTotalCount(), D->GetBrokenCount(), D->GetHintsUsed());
}

void ULTTEndScreen::HandleRunSubmitted(const FLTTRunResult& Result)
{
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OnRunSubmitted.RemoveDynamic(this, &ULTTEndScreen::HandleRunSubmitted);
	}
	if (!RankText)
	{
		return;
	}
	if (!Result.bOk)
	{
		RankText->SetColorAndOpacity(FSlateColor(LTTTheme::Bad()));
		RankText->SetText(LOCTEXT("SendFailed", "Couldn't reach the leaderboard (offline?). Your take wasn't recorded."));
		return;
	}
	FText Best;
	if (Result.bNewBestScore && Result.bNewBestTime)
	{
		Best = LOCTEXT("BestBoth", "  -  New best score and time!");
	}
	else if (Result.bNewBestScore)
	{
		Best = LOCTEXT("BestScore", "  -  New best score!");
	}
	else if (Result.bNewBestTime)
	{
		Best = LOCTEXT("BestTime", "  -  New best time!");
	}
	RankText->SetColorAndOpacity(FSlateColor(Result.bNewBestScore || Result.bNewBestTime ? LTTTheme::Good() : LTTTheme::HeaderBrown()));
	RankText->SetText(FText::Format(LOCTEXT("Rank", "Leaderboard: #{0} of {1}{2}"), FText::AsNumber(Result.You.Rank), FText::AsNumber(Result.TotalPlayers), Best));
}

void ULTTEndScreen::NativeDestruct()
{
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OnRunSubmitted.RemoveDynamic(this, &ULTTEndScreen::HandleRunSubmitted);
	}
	Super::NativeDestruct();
}

void ULTTEndScreen::OpenLeaderboard()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	TSubclassOf<ULTTLeaderboardPanel> PanelClass = LeaderboardPanelClass;
	if (!PanelClass)
	{
		PanelClass = ULTTLeaderboardPanel::StaticClass();
	}
	if (ULTTLeaderboardPanel* Panel = CreateWidget<ULTTLeaderboardPanel>(PC, PanelClass))
	{
		Panel->Open(this);
	}
}

void ULTTEndScreen::HandleLeaderboardClicked()
{
	OpenLeaderboard();
}

void ULTTEndScreen::HandleFullLeaderboardClicked()
{
	if (const ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OpenFullLeaderboard();
	}
}

void ULTTEndScreen::ApplyTexts()
{
	const ALTTMessDirector* D = Director.Get();
	if (TitleText)
	{
		TitleText->SetText(bSetClean ? LOCTEXT("TitleClean", "SET READY!") : LOCTEXT("TitleTimeUp", "TIME'S UP!"));
		TitleText->SetColorAndOpacity(FSlateColor(bSetClean ? LTTTheme::TitleCream() : RedTitle));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(bSetClean
			? LOCTEXT("SubClean", "Every prop is back on its mark. Places, everyone!")
			: LOCTEXT("SubTimeUp", "We're losing the light! The crew will have to shoot around the mess."));
	}
	if (!D)
	{
		return;
	}
	if (TimeValue)
	{
		TimeValue->SetText(FormatTime(D->GetTimeOnSet()));
	}
	if (ReturnedValue)
	{
		ReturnedValue->SetText(FText::Format(LOCTEXT("Returned", "{0} / {1}"), FText::AsNumber(D->GetPlacedCount()), FText::AsNumber(D->GetTotalCount())));
	}
	if (DestroyedValue)
	{
		// "3  (-45 pts)"
		FNumberFormattingOptions Signed;
		Signed.AlwaysSign = true;
		DestroyedValue->SetText(D->GetBrokenCount() > 0 && D->GetBrokenPoints() != 0
			? FText::Format(LOCTEXT("DestroyedPts", "{0}  ({1} pts)"), FText::AsNumber(D->GetBrokenCount()), FText::AsNumber(D->GetBrokenPoints(), &Signed))
			: FText::AsNumber(D->GetBrokenCount()));
	}
	if (HintsValue)
	{
		HintsValue->SetText(FText::AsNumber(D->GetHintsUsed()));
	}
	if (TimeBonusValue)
	{
		FNumberFormattingOptions Signed;
		Signed.AlwaysSign = true;
		TimeBonusValue->SetText(D->GetTimeBonusPoints() > 0
			? FText::Format(LOCTEXT("TimeBonus", "{0}  ({1} left)"), FText::AsNumber(D->GetTimeBonusPoints(), &Signed), FormatTime(D->GetTimeLeftAtFinish()))
			: LOCTEXT("NoTimeBonus", "-"));
	}
	if (KeepPlayingButton)
	{
		// Nothing left to do once the set is clean, but free roam is still nice; keep it either way.
		KeepPlayingButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void ULTTEndScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime; // UMG ticks with real time, so this runs while the game is paused

	// Title slams down (scale 2.5 -> 1, out-bounce, 0.45 s).
	if (TitleText)
	{
		const float T = FMath::Clamp(AnimTime / 0.45f, 0.f, 1.f);
		const float Scale = FMath::Lerp(2.5f, 1.f, EaseOutBounce(T));
		TitleText->SetRenderScale(FVector2D(Scale, Scale));
	}
	// Card fades and slides in from the right (0.2 -> 0.55 s).
	if (Card)
	{
		const float T = FMath::Clamp((AnimTime - 0.2f) / 0.35f, 0.f, 1.f);
		const float E = EndEaseOutCubic(T);
		Card->SetRenderOpacity(FMath::Max(E, 0.35f));
		Card->SetRenderTranslation(FVector2D((1.f - E) * 120.f, 0.f));
	}
	// Points roll up (0.6 -> 1.6 s).
	if (PointsValue)
	{
		const float T = FMath::Clamp((AnimTime - 0.6f) / 1.f, 0.f, 1.f);
		PointsValue->SetText(FText::AsNumber(FMath::RoundToInt(FinalPoints * EndEaseOutCubic(T))));
	}
}

void ULTTEndScreen::HandlePlayAgainClicked()
{
	PlayAgain();
}

void ULTTEndScreen::HandleKeepPlayingClicked()
{
	KeepPlaying();
}

void ULTTEndScreen::PlayAgain()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ULTTEndScreen::KeepPlaying()
{
	if (ALTTMessDirector* D = Director.Get())
	{
		D->CloseEndScreen();
	}
	else
	{
		RemoveFromParent();
	}
}

#undef LOCTEXT_NAMESPACE
