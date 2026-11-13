// Clean! Cut! Action!

#include "UI/LTTLeaderboardPanel.h"
#include "UI/LTTUITheme.h"
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

#define LOCTEXT_NAMESPACE "LTTLeaderboardPanel"

namespace LTTLeaderboardPanelPrivate
{
	// Unique names: unity builds merge widget .cpps (see the pause menu's helpers).
	constexpr float RankWidth = 58.f;
	constexpr float ScoreWidth = 130.f;
	constexpr float TimeWidth = 120.f;

	float LbEaseOutCubic(float T)
	{
		const float U = 1.f - T;
		return 1.f - U * U * U;
	}

	FText LbFormatTime(float Seconds)
	{
		if (Seconds < 0.f)
		{
			return LOCTEXT("NoTime", "-");
		}
		const int32 Total = FMath::Max(FMath::FloorToInt(Seconds), 0);
		FNumberFormattingOptions TwoDigits;
		TwoDigits.MinimumIntegralDigits = 2;
		return FText::Format(LOCTEXT("TimeFmt", "{0}:{1}"), FText::AsNumber(Total / 60), FText::AsNumber(Total % 60, &TwoDigits));
	}

	/** Gold, silver and bronze chips for the podium, cream for the rest. */
	FLinearColor RankFill(int32 Rank)
	{
		switch (Rank)
		{
		case 1: return LTTTheme::ButtonGold();
		case 2: return LTTTheme::Hex(0xD9, 0xD4, 0xCB);
		case 3: return LTTTheme::Hex(0xD9, 0x93, 0x5A);
		default: return LTTTheme::ButtonCream();
		}
	}
}

using namespace LTTLeaderboardPanelPrivate;

void ULTTLeaderboardPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	SetIsFocusable(true); // Esc / gamepad B close it
	if (FullLeaderboardButton)
	{
		FullLeaderboardButton->OnClicked.AddDynamic(this, &ULTTLeaderboardPanel::HandleFullClicked);
	}
	if (RetryButton)
	{
		RetryButton->OnClicked.AddDynamic(this, &ULTTLeaderboardPanel::HandleRetryClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &ULTTLeaderboardPanel::HandleCloseClicked);
	}
}

void ULTTLeaderboardPanel::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.f;
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OnLeaderboardFetched.AddUniqueDynamic(this, &ULTTLeaderboardPanel::HandleFetched);
	}
	Refresh();
}

void ULTTLeaderboardPanel::NativeDestruct()
{
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OnLeaderboardFetched.RemoveDynamic(this, &ULTTLeaderboardPanel::HandleFetched);
	}
	Super::NativeDestruct();
}

UTextBlock* ULTTLeaderboardPanel::MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bBold) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	LTTTheme::StyleText(Text, FontSize, Color, bBold);
	return Text;
}

UButton* ULTTLeaderboardPanel::MakeButton(FName Name, const FText& Label, const FLinearColor& Fill, const TCHAR* Icon) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	LTTTheme::StyleButton(Button, Fill);
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Icon)
	{
		if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, *(Name.ToString() + TEXT("_Icon")), Icon, 32.f)))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}
	}
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 20, LTTTheme::Ink());
	Text->SetText(Label);
	if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(Text))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	Button->SetContent(Content);
	return Button;
}

void ULTTLeaderboardPanel::BuildDefaultLayout()
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
	CardBorder->SetPadding(FMargin(40.f, 26.f, 40.f, 28.f));
	Card = CardBorder;
	if (UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(CardBorder))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardWidth"));
	Width->SetWidthOverride(700.f);
	CardBorder->SetContent(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Width->SetContent(Column);

	// Title on the ribbon, coins and a diamond either side.
	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 46);
	TitleText->SetText(LOCTEXT("Title", "LEADERBOARD"));
	TitleText->SetJustification(ETextJustify::Center);
	LTTTheme::FillRibbonBanner(WidgetTree, Banner, TitleText, TEXT("coins"), TEXT("diamond"), 58.f);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	StatusText = MakeText(TEXT("StatusText"), 17, LTTTheme::InkMuted());
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StatusText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 2.f, 0.f, 12.f));
	}

	// Column titles.
	FLTTLeaderboardEntry HeaderEntry;
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(MakeRow(HeaderEntry, true)))
	{
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
	Column->AddChildToVerticalBox(Rows);

	YouBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("YouBox"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(YouBox))
	{
		S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	FullLeaderboardButton = MakeButton(TEXT("FullLeaderboardButton"), LOCTEXT("Full", "Full Leaderboard"), LTTTheme::ButtonGold(), TEXT("film"));
	RetryButton = MakeButton(TEXT("RetryButton"), LOCTEXT("Retry", "Retry"), LTTTheme::ButtonCream(), nullptr);
	CloseButton = MakeButton(TEXT("CloseButton"), LOCTEXT("Back", "Back"), LTTTheme::ButtonCream(), nullptr);
	RetryButton->SetVisibility(ESlateVisibility::Collapsed);
	for (UButton* Button : { FullLeaderboardButton.Get(), RetryButton.Get(), CloseButton.Get() })
	{
		if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(Button))
		{
			S->SetPadding(FMargin(8.f, 0.f));
			S->SetVerticalAlignment(VAlign_Center);
		}
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Buttons))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
	}

	UTextBlock* UrlText = MakeText(TEXT("UrlText"), 13, LTTTheme::InkMuted(), false);
	UrlText->SetText(LOCTEXT("Url", "cca.ohmcodes.com/leaderboard"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(UrlText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
}

UWidget* ULTTLeaderboardPanel::MakeRow(const FLTTLeaderboardEntry& Entry, bool bHeader) const
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const int32 FontSize = bHeader ? 13 : 19;

	auto AddFixed = [this, Line](UWidget* Content, float Width, EHorizontalAlignment Align)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(Width);
		Box->SetContent(Content);
		if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Box))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetHorizontalAlignment(Align);
		}
		return Box;
	};

	// Rank: a round chip (gold / silver / bronze on the podium).
	if (bHeader)
	{
		UTextBlock* Hash = MakeText(NAME_None, FontSize, LTTTheme::InkMuted());
		Hash->SetText(LOCTEXT("HeadRank", "#"));
		Hash->SetJustification(ETextJustify::Center);
		AddFixed(Hash, RankWidth, HAlign_Fill);
	}
	else
	{
		UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		const FLinearColor Edge = Entry.Rank > 0 && Entry.Rank <= 3 ? LTTTheme::WoodDark() : LTTTheme::ButtonEdge();
		LTTTheme::ApplyBrush(Chip, LTTTheme::Rounded(RankFill(Entry.Rank), -1.f, Edge, 2.f));
		Chip->SetHorizontalAlignment(HAlign_Center);
		Chip->SetVerticalAlignment(VAlign_Center);
		Chip->SetPadding(FMargin(6.f, 2.f));
		UTextBlock* RankText = MakeText(NAME_None, Entry.Rank >= 100 ? 14 : 18, LTTTheme::Ink());
		RankText->SetText(Entry.Rank > 0 ? FText::AsNumber(Entry.Rank) : LOCTEXT("NoRank", "-"));
		Chip->SetContent(RankText);
		USizeBox* ChipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ChipSize->SetWidthOverride(44.f);
		ChipSize->SetHeightOverride(40.f);
		ChipSize->SetContent(Chip);
		AddFixed(ChipSize, RankWidth, HAlign_Left);
	}

	// Name (fills), with a YOU tag on the player's own row.
	UHorizontalBox* NameCell = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Name = MakeText(NAME_None, FontSize, bHeader ? LTTTheme::InkMuted() : LTTTheme::Ink());
	Name->SetText(bHeader ? LOCTEXT("HeadName", "ROOKIE") : FText::FromString(Entry.Name));
	Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	if (UHorizontalBoxSlot* S = NameCell->AddChildToHorizontalBox(Name))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	if (!bHeader && Entry.bIsYou)
	{
		UBorder* Tag = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		LTTTheme::ApplyBrush(Tag, LTTTheme::Rounded(LTTTheme::Wood(), -1.f, LTTTheme::WoodDark(), 1.f));
		Tag->SetPadding(FMargin(8.f, 1.f, 8.f, 2.f));
		UTextBlock* TagText = MakeText(NAME_None, 12, LTTTheme::TitleCream());
		TagText->SetText(LOCTEXT("You", "YOU"));
		Tag->SetContent(TagText);
		if (UHorizontalBoxSlot* S = NameCell->AddChildToHorizontalBox(Tag))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
		}
	}
	if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(NameCell))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(6.f, 0.f, 8.f, 0.f));
	}

	UTextBlock* Score = MakeText(NAME_None, FontSize, bHeader ? LTTTheme::InkMuted() : LTTTheme::Coin());
	Score->SetText(bHeader ? LOCTEXT("HeadScore", "BEST SCORE") : FText::AsNumber(Entry.BestScore));
	Score->SetJustification(ETextJustify::Right);
	AddFixed(Score, ScoreWidth, HAlign_Fill);

	UTextBlock* Time = MakeText(NAME_None, FontSize, bHeader || Entry.BestTime < 0.f ? LTTTheme::InkMuted() : LTTTheme::Good());
	Time->SetText(bHeader ? LOCTEXT("HeadTime", "BEST TIME") : LbFormatTime(Entry.BestTime));
	Time->SetJustification(ETextJustify::Right);
	AddFixed(Time, TimeWidth, HAlign_Fill);

	if (bHeader)
	{
		UBorder* Pad = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HeaderRow"));
		Pad->SetBrush(FSlateBrush());
		Pad->SetBrushColor(FLinearColor::Transparent);
		Pad->SetPadding(FMargin(14.f, 0.f));
		Pad->SetContent(Line);
		return Pad;
	}

	// Paper strip; the player's own row is gold.
	UBorder* Strip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (Entry.bIsYou)
	{
		LTTTheme::ApplyBrush(Strip, LTTTheme::Rounded(LTTTheme::Hex(0xFD, 0xE7, 0xAE), 14.f, LTTTheme::ButtonEdge(), 3.f));
	}
	else
	{
		LTTTheme::StyleInset(Strip, 14.f);
	}
	Strip->SetPadding(FMargin(14.f, 5.f));
	Strip->SetContent(Line);
	return Strip;
}

UWidget* ULTTLeaderboardPanel::MakeMessageRow(const FText& Message) const
{
	UBorder* Strip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LTTTheme::StyleInset(Strip, 14.f, 0.6f);
	Strip->SetPadding(FMargin(16.f, 10.f));
	UTextBlock* Text = MakeText(NAME_None, 17, LTTTheme::Ink());
	Text->SetText(Message);
	Text->SetAutoWrapText(true);
	Text->SetJustification(ETextJustify::Center);
	Strip->SetContent(Text);
	return Strip;
}

void ULTTLeaderboardPanel::Open(UUserWidget* ReturnFocusTo)
{
	ReturnFocus = ReturnFocusTo;
	if (!IsInViewport())
	{
		AddToViewport(100); // above the end screen (10) and the briefing (20)
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
}

void ULTTLeaderboardPanel::Close()
{
	APlayerController* PC = GetOwningPlayer();
	UUserWidget* Back = ReturnFocus.Get();
	RemoveFromParent();
	if (PC && Back && Back->IsInViewport())
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Back->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void ULTTLeaderboardPanel::ShowLoading()
{
	if (StatusText)
	{
		StatusText->SetText(LOCTEXT("Loading", "Checking the call sheet..."));
		StatusText->SetColorAndOpacity(FSlateColor(LTTTheme::InkMuted()));
	}
	if (RetryButton)
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Rows)
	{
		Rows->ClearChildren();
	}
	if (YouBox)
	{
		YouBox->ClearChildren();
	}
	AnimatedRows.Reset();
	RowsShownAt = -1.f;
}

void ULTTLeaderboardPanel::Refresh()
{
	ShowLoading();
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->FetchLeaderboard(RowCount);
	}
	else
	{
		FLTTLeaderboardResult Offline;
		Offline.Error = TEXT("offline");
		HandleFetched(Offline);
	}
}

void ULTTLeaderboardPanel::HandleFetched(const FLTTLeaderboardResult& Result)
{
	if (!Rows)
	{
		return;
	}
	Rows->ClearChildren();
	if (YouBox)
	{
		YouBox->ClearChildren();
	}
	AnimatedRows.Reset();

	if (!Result.bOk)
	{
		if (StatusText)
		{
			StatusText->SetText(LOCTEXT("Error", "The leaderboard is on a coffee break. Check your connection and try again."));
			StatusText->SetColorAndOpacity(FSlateColor(LTTTheme::Bad()));
		}
		if (RetryButton)
		{
			RetryButton->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}
	if (RetryButton)
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (StatusText)
	{
		StatusText->SetColorAndOpacity(FSlateColor(LTTTheme::InkMuted()));
		StatusText->SetText(FText::Format(LOCTEXT("Status", "Top {0} of {1} {1}|plural(one=Rookie,other=Rookies)"),
			FText::AsNumber(FMath::Min(Result.Top.Num(), RowCount)), FText::AsNumber(Result.TotalPlayers)));
	}

	bool bYouInList = false;
	for (const FLTTLeaderboardEntry& Entry : Result.Top)
	{
		bYouInList |= Entry.bIsYou;
		UWidget* Row = MakeRow(Entry, false);
		if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(Row))
		{
			S->SetPadding(FMargin(0.f, 2.f));
		}
		AnimatedRows.Add(Row);
	}
	if (Result.Top.IsEmpty())
	{
		UWidget* Row = MakeMessageRow(LOCTEXT("Empty", "Nobody has wrapped a take yet. Be the first on the board!"));
		Rows->AddChildToVerticalBox(Row);
		AnimatedRows.Add(Row);
	}

	// Your place underneath when you're not in the list.
	if (YouBox && !bYouInList)
	{
		UTextBlock* Gap = MakeText(TEXT("YouGap"), 18, LTTTheme::InkMuted());
		Gap->SetText(LOCTEXT("Gap", ". . .   YOUR PLACE   . . ."));
		Gap->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* S = YouBox->AddChildToVerticalBox(Gap))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
		}
		UWidget* Row = nullptr;
		if (Result.bHasYou && Result.You.Rank > 0)
		{
			FLTTLeaderboardEntry You = Result.You;
			You.bIsYou = true;
			Row = MakeRow(You, false);
		}
		else
		{
			const FString Name = ULTTLeaderboardSubsystem::Get(this) ? ULTTLeaderboardSubsystem::Get(this)->GetPlayerName() : FString();
			Row = MakeMessageRow(FText::Format(LOCTEXT("NotRanked", "{0}: not on the board yet. Finish a take to get your place!"), FText::FromString(Name)));
		}
		YouBox->AddChildToVerticalBox(Row);
		AnimatedRows.Add(Row);
	}

	for (UWidget* Row : AnimatedRows)
	{
		Row->SetRenderOpacity(0.f);
	}
	RowsShownAt = AnimTime;
}

void ULTTLeaderboardPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime; // real time: runs while the game is paused

	if (Card)
	{
		const float E = LbEaseOutCubic(FMath::Clamp(AnimTime / 0.35f, 0.f, 1.f));
		Card->SetRenderOpacity(E);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - E) * 40.f));
	}
	// Rows slide in from the right, 40 ms apart.
	if (RowsShownAt >= 0.f)
	{
		for (int32 i = 0; i < AnimatedRows.Num(); ++i)
		{
			if (UWidget* Row = AnimatedRows[i])
			{
				const float E = LbEaseOutCubic(FMath::Clamp((AnimTime - RowsShownAt - i * 0.04f) / 0.25f, 0.f, 1.f));
				Row->SetRenderOpacity(E);
				Row->SetRenderTranslation(FVector2D((1.f - E) * 60.f, 0.f));
			}
		}
	}
}

FReply ULTTLeaderboardPanel::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::L)
	{
		HandleFullClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULTTLeaderboardPanel::HandleFullClicked()
{
	if (const ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		Board->OpenFullLeaderboard();
	}
}

void ULTTLeaderboardPanel::HandleRetryClicked()
{
	Refresh();
}

void ULTTLeaderboardPanel::HandleCloseClicked()
{
	Close();
}

#undef LOCTEXT_NAMESPACE
