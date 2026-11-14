// Clean! Cut! Action!

#include "UI/LTTBriefingScreen.h"
#include "UI/LTTUITheme.h"
#include "Interaction/LTTMessDirector.h"
#include "UI/LTTLeaderboardSubsystem.h"
#include "Components/EditableTextBox.h"
#include "CleanCutActionPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "UI/LTTLeaderboardPanel.h"
#include "UI/LTTVirtualKeyboard.h"

#define LOCTEXT_NAMESPACE "LTTBriefingScreen"

namespace
{
	// Cafe theme (UI/LTTUITheme.h).
	const FLinearColor TapeYellow = LTTTheme::HeaderBrown(); // section headers
	const FLinearColor SoftWhite = LTTTheme::Ink();
	const FLinearColor Muted = LTTTheme::InkMuted();
	const FLinearColor NoteCream = LTTTheme::Ink();

	float EaseOutCubic(float T)
	{
		const float U = 1.f - T;
		return 1.f - U * U * U;
	}

	// Screen fit: room kept around the card (the corner props hang ~50 over its edges, plus phone notches),
	// and the smallest the card may shrink to.
	const float BriefingFitMargin = 56.f;
	const float BriefingMinFitScale = 0.3f;

	constexpr int32 BriefingPageCount = 3;
	constexpr float BriefingSwipeDistance = 70.f; // px (local) for a swipe to flip the page
}

void ULTTBriefingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	SetIsFocusable(true); // gets Enter / Space / gamepad A while input is UI-only
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandleStartClicked);
	}
	if (NoTimerButton)
	{
		NoTimerButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandleNoTimerClicked);
	}
	if (RandomNameButton)
	{
		RandomNameButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandleRandomNameClicked);
	}
	if (NameInput)
	{
		NameInput->OnTextChanged.AddDynamic(this, &ULTTBriefingScreen::HandleNameChanged);
		NameInput->OnTextCommitted.AddDynamic(this, &ULTTBriefingScreen::HandleNameCommitted);
	}
	if (!LeaderboardButton)
	{
		// WBP_Briefing was made before this existed: put it at the end of the name row.
		if (UHorizontalBox* NameRow = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("NameRow"))))
		{
			LeaderboardButton = MakeLeaderboardButton();
			if (UHorizontalBoxSlot* S = NameRow->AddChildToHorizontalBox(LeaderboardButton))
			{
				S->SetVerticalAlignment(VAlign_Center);
				S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			}
		}
	}
	if (LeaderboardButton)
	{
		LeaderboardButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandleLeaderboardClicked);
	}
	if (PrevPageButton)
	{
		PrevPageButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandlePrevPageClicked);
	}
	if (NextPageButton)
	{
		NextPageButton->OnClicked.AddDynamic(this, &ULTTBriefingScreen::HandleNextPageClicked);
	}
	// Dots: one per page, made here so a WBP_ layout only needs the empty PageDots box.
	if (PageDots && PageDots->GetChildrenCount() == 0 && Pages)
	{
		for (int32 i = 0; i < Pages->GetNumWidgets(); ++i)
		{
			UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			DotSize->SetWidthOverride(14.f);
			DotSize->SetHeightOverride(14.f);
			DotSize->SetContent(Dot);
			if (UHorizontalBoxSlot* S = PageDots->AddChildToHorizontalBox(DotSize))
			{
				S->SetVerticalAlignment(VAlign_Center);
				S->SetPadding(FMargin(5.f, 0.f));
			}
		}
	}
}

void ULTTBriefingScreen::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.f;
	ApplyInputVariant();
	ApplyTexts();
	ShowPage(0);
	PageChangedAt = -1.f; // no slide on the first page
	if (NameInput)
	{
		const ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this);
		NameInput->SetText(FText::FromString(Board ? Board->GetPlayerName() : ULTTLeaderboardSubsystem::MakeRandomName()));
		NameInput->SetHintText(LOCTEXT("NameHint", "Type your name"));
	}
}

void ULTTBriefingScreen::ApplyInputVariant()
{
	// Works for the C++ layout and for a WBP_ child, as long as the widgets keep these names.
	const bool bTouch = IsTouch();
	if (UWidget* Desktop = GetWidgetFromName(TEXT("ControlsDesktop")))
	{
		Desktop->SetVisibility(bTouch ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (UWidget* Touch = GetWidgetFromName(TEXT("ControlsTouch")))
	{
		Touch->SetVisibility(bTouch ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (UTextBlock* StartHint = Cast<UTextBlock>(GetWidgetFromName(TEXT("StartHint"))))
	{
		StartHint->SetText(bTouch ? LOCTEXT("StartHintTouch", "Tap a button to start")
			: LOCTEXT("StartHint", "Time trial: Enter / gamepad A     No timer: N / gamepad Y"));
	}
}

UTextBlock* ULTTBriefingScreen::MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bWrap) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	// Headers bold, body text regular weight so the long paragraphs stay light.
	LTTTheme::StyleText(Text, FontSize, Color, FontSize >= 18 || Color == TapeYellow || Color == Muted);
	Text->SetAutoWrapText(bWrap);
	return Text;
}

void ULTTBriefingScreen::AddHeader(UVerticalBox* Box, const FText& Text, float TopPadding, const TCHAR* IconName) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, NAME_None, IconName, 40.f)))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	UTextBlock* Header = MakeText(NAME_None, 20, TapeYellow);
	Header->SetText(Text);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Header))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
	{
		S->SetPadding(FMargin(0.f, TopPadding, 0.f, 6.f));
	}
}

UTextBlock* ULTTBriefingScreen::AddIconTextRow(UVerticalBox* Box, FName TextName, const TCHAR* IconName) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, NAME_None, IconName, 32.f)))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(4.f, 0.f, 12.f, 0.f));
	}
	UTextBlock* Text = MakeText(TextName, 15, SoftWhite, true);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Text))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
	{
		S->SetPadding(FMargin(0.f, 2.f));
	}
	return Text;
}

void ULTTBriefingScreen::AddControlRow(UVerticalBox* Box, const FText& Action, const FText& Keyboard, const FText& Gamepad) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto AddCell = [this, Row](const FText& Text, const FLinearColor& Color, float Fill)
	{
		UTextBlock* Cell = MakeText(NAME_None, 15, Color);
		Cell->SetText(Text);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Cell))
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Fill;
			S->SetSize(Size);
			S->SetVerticalAlignment(VAlign_Center);
		}
	};
	AddCell(Action, Muted, 1.35f);
	AddCell(Keyboard, SoftWhite, 1.f);
	AddCell(Gamepad, SoftWhite, 0.7f);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
	{
		S->SetPadding(FMargin(0.f, 2.f));
	}
}

bool ULTTBriefingScreen::IsTouch() const
{
	if (!GetWorld())
	{
		return false; // e.g. the editor baker's temporary instance
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}
	const ACleanCutActionPlayerController* CCPC = Cast<ACleanCutActionPlayerController>(PC);
	return CCPC && CCPC->ShouldUseTouchControls();
}

void ULTTBriefingScreen::AddTouchRow(UVerticalBox* Box, const TCHAR* IconName, const FText& Action, const FText& How) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// Icon in a small round chip, like the on-screen button. The textures are cooked via UI_TouchSimple's hard refs.
	UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LTTTheme::ApplyBrush(Chip, LTTTheme::Rounded(LTTTheme::ButtonCream(), -1.f, LTTTheme::ButtonEdge(), 2.f));
	Chip->SetPadding(FMargin(6.f));
	Chip->SetHorizontalAlignment(HAlign_Center);
	Chip->SetVerticalAlignment(VAlign_Center);
	if (IconName)
	{
		const FString Path = FString::Printf(TEXT("/Game/_CleanCutAction/UI/Icons/%s.%s"), IconName, IconName);
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path))
		{
			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			FSlateBrush Brush;
			Brush.SetResourceObject(Tex);
			Brush.ImageSize = FVector2D(26.f, 26.f); // on the brush so a WBP_ bake keeps it
			Icon->SetBrush(Brush);
			Icon->SetColorAndOpacity(LTTTheme::Ink()); // white icon art, tinted brown on the cream chip
			Chip->SetContent(Icon);
		}
	}
	USizeBox* ChipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ChipSize->SetWidthOverride(38.f);
	ChipSize->SetHeightOverride(38.f);
	ChipSize->SetContent(Chip);
	if (!IconName)
	{
		ChipSize->SetVisibility(ESlateVisibility::Hidden); // keeps the text columns aligned
	}
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(ChipSize))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	auto AddCell = [this, Row](const FText& Text, const FLinearColor& Color, float Fill)
	{
		UTextBlock* Cell = MakeText(NAME_None, 15, Color, true);
		Cell->SetText(Text);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Cell))
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Fill;
			S->SetSize(Size);
			S->SetVerticalAlignment(VAlign_Center);
		}
	};
	AddCell(Action, Muted, 0.8f);
	AddCell(How, SoftWhite, 1.4f);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
	{
		S->SetPadding(FMargin(0.f, 3.f));
	}
}

void ULTTBriefingScreen::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// The game is black behind this anyway (intro fade); a solid backdrop keeps it readable either way.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	LTTTheme::StyleBackdrop(Dim, 0.85f);
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	// The card (paper panel) inside an overlay that also holds the film-set props peeking over its corners.
	UOverlay* CardFrame = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
	Card = CardFrame;
	if (UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(CardFrame))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}
	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardPanel"));
	LTTTheme::StylePanel(CardBorder);
	CardBorder->SetPadding(FMargin(44.f, 24.f, 44.f, 26.f));
	CardFrame->AddChildToOverlay(CardBorder);
	auto AddCornerArt = [this, CardFrame](FName Name, const TCHAR* Texture, float Size, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Offset, float Angle)
	{
		UImage* Art = LTTTheme::MakeArtIcon(WidgetTree, Name, Texture, Size);
		Art->SetRenderTransformAngle(Angle);
		Art->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* S = CardFrame->AddChildToOverlay(Art))
		{
			S->SetHorizontalAlignment(H);
			S->SetVerticalAlignment(V);
			S->SetPadding(Offset); // negative = hangs over the card edge
		}
	};
	AddCornerArt(TEXT("CornerLight"), TEXT("rig_light"), 120.f, HAlign_Left, VAlign_Top, FMargin(-54.f, -46.f, 0.f, 0.f), -12.f);
	AddCornerArt(TEXT("CornerCamera"), TEXT("camera_rig"), 120.f, HAlign_Right, VAlign_Top, FMargin(0.f, -46.f, -54.f, 0.f), 8.f);
	AddCornerArt(TEXT("CornerFilmRoll"), TEXT("film_roll"), 110.f, HAlign_Left, VAlign_Bottom, FMargin(-50.f, 0.f, 0.f, -40.f), 10.f);
	AddCornerArt(TEXT("CornerChair"), TEXT("director_chair"), 120.f, HAlign_Right, VAlign_Bottom, FMargin(0.f, 0.f, -50.f, -44.f), -6.f);

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Width->SetWidthOverride(900.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	// ---- Title: on the ribbon banner, clapperboard and megaphone either side ----
	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 48);
	TitleText->SetText(LOCTEXT("Title", "CLEAN! CUT! ACTION!"));
	TitleText->SetJustification(ETextJustify::Center);
	LTTTheme::FillRibbonBanner(WidgetTree, Banner, TitleText, TEXT("clapperboard"), TEXT("megaphone"), 64.f);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
	}
	UHorizontalBox* SubtitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SubtitleRow"));
	UTextBlock* Subtitle = MakeText(TEXT("SubtitleText"), 16, Muted);
	Subtitle->SetText(LOCTEXT("Subtitle", "CALL SHEET  -  Last day of shooting  -  Scene 42: The Cafe"));
	Subtitle->SetJustification(ETextJustify::Center);
	for (int32 i = 0; i < 3; ++i)
	{
		UWidget* Cell = i == 1 ? static_cast<UWidget*>(Subtitle) : LTTTheme::MakeArtIcon(WidgetTree, NAME_None, TEXT("film"), 34.f);
		if (UHorizontalBoxSlot* S = SubtitleRow->AddChildToHorizontalBox(Cell))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(10.f, 0.f));
		}
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(SubtitleRow))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	}

	// ---- Carousel: note | how to play | controls and scoring ----
	AddPages(Column);
	AddPageNav(Column);

	// ---- Leaderboard name ----
	AddNameRow(Column);

	// ---- Start: time trial, or no clock ----
	UHorizontalBox* StartRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StartButtons"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StartRow))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
	auto AddStartButton = [this, StartRow](FName Name, const FLinearColor& Fill, const TCHAR* Icon, float IconSize, const FText& Label, int32 FontSize, float LeftPad)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		LTTTheme::StyleButton(Button, Fill);
		UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, *(Name.ToString() + TEXT("_Icon")), Icon, IconSize)))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}
		UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), FontSize, LTTTheme::Ink());
		Text->SetText(Label);
		if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(Text))
		{
			S->SetVerticalAlignment(VAlign_Center);
		}
		Button->SetContent(Content);
		if (UHorizontalBoxSlot* S = StartRow->AddChildToHorizontalBox(Button))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(LeftPad, 0.f, 0.f, 0.f));
		}
		return Button;
	};
	StartButton = AddStartButton(TEXT("StartButton"), LTTTheme::ButtonGold(), TEXT("timer"), 44.f, LOCTEXT("Start", "START TIME TRIAL!"), 26, 0.f);
	NoTimerButton = AddStartButton(TEXT("NoTimerButton"), LTTTheme::ButtonCream(), TEXT("coffee"), 40.f, LOCTEXT("StartNoTimer", "JUST CLEAN (NO TIMER)"), 20, 28.f);
	UTextBlock* StartHint = MakeText(TEXT("StartHint"), 13, Muted);
	StartHint->SetText(LOCTEXT("StartHint", "Time trial: Enter / gamepad A     No timer: N / gamepad Y"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StartHint))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
}

void ULTTBriefingScreen::AddPages(UVerticalBox* Box)
{
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Pages))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
	}

	// Page 1: Marlowe's note, on a darker "paper" panel.
	UVerticalBox* PageNote = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PageNote"));
	Pages->AddChild(PageNote);
	AddHeader(PageNote, LOCTEXT("NoteHeader", "A NOTE FROM THE DIRECTOR"), 0.f, TEXT("megaphone"));
	UBorder* Note = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotePaper"));
	LTTTheme::StyleInset(Note);
	Note->SetPadding(FMargin(18.f, 14.f));
	StoryText = MakeText(TEXT("StoryText"), 18, NoteCream, true);
	Note->SetContent(StoryText);
	PageNote->AddChildToVerticalBox(Note);

	// Page 2: how to play.
	UVerticalBox* PageHowTo = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PageHowTo"));
	Pages->AddChild(PageHowTo);
	AddHeader(PageHowTo, LOCTEXT("HowHeader", "HOW TO PLAY"), 0.f, TEXT("checklist"));
	UBorder* HowPaper = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HowToPaper"));
	LTTTheme::StyleInset(HowPaper);
	HowPaper->SetPadding(FMargin(18.f, 14.f));
	HowToPlayText = MakeText(TEXT("HowToPlayText"), 17, SoftWhite, true);
	HowToPlayText->SetLineHeightPercentage(1.15f);
	HowPaper->SetContent(HowToPlayText);
	PageHowTo->AddChildToVerticalBox(HowPaper);

	// Page 3: controls | scoring, side by side.
	UHorizontalBox* PageRules = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PageRules"));
	Pages->AddChild(PageRules);
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsColumn"));
	if (UHorizontalBoxSlot* S = PageRules->AddChildToHorizontalBox(Left))
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = 1.25f;
		S->SetSize(Size);
		S->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
	}
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ScoringColumn"));
	if (UHorizontalBoxSlot* S = PageRules->AddChildToHorizontalBox(Right))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	AddHeader(Left, LOCTEXT("ControlsHeader", "CONTROLS"), 0.f, TEXT("card"));
	// Both control lists are built (and named) so a WBP_ child can lay out each; ApplyInputVariant shows one.
	UVerticalBox* ControlsDesktop = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsDesktop"));
	UVerticalBox* ControlsTouch = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsTouch"));
	Left->AddChildToVerticalBox(ControlsDesktop);
	Left->AddChildToVerticalBox(ControlsTouch);
	ControlsTouch->SetVisibility(ESlateVisibility::Collapsed);
	// Phone / tablet: the on-screen buttons from UI_TouchSimple.
	AddTouchRow(ControlsTouch, nullptr, LOCTEXT("TouchMove", "Move"), LOCTEXT("TouchMoveHow", "Left stick (bottom left)"));
	AddTouchRow(ControlsTouch, nullptr, LOCTEXT("TouchLook", "Look / aim"), LOCTEXT("TouchLookHow", "Right stick (bottom right)"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_HandOpen"), LOCTEXT("TouchGrab", "Pick up"), LOCTEXT("TouchGrabHow", "Aim at a prop, tap the hand"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_HandClosed"), LOCTEXT("TouchPlace", "Place / Drop"), LOCTEXT("TouchPlaceHow", "Tap the hand again: green = PLACE, red = it will break"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_Throw"), LOCTEXT("TouchThrow", "Throw (breaks it!)"), LOCTEXT("TouchThrowHow", "Bottle button, shows while holding"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_Jump"), LOCTEXT("TouchJump", "Jump"), LOCTEXT("TouchJumpHow", "Jump button"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_Hint"), LOCTEXT("TouchHint", "Hint"), LOCTEXT("TouchHintHow", "Light bulb button"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_Pause"), LOCTEXT("TouchPause", "Pause"), LOCTEXT("TouchPauseHow", "Top right corner"));
	AddTouchRow(ControlsTouch, TEXT("T_Icon_MusicOn"), LOCTEXT("TouchMusic", "Music on / off"), LOCTEXT("TouchMusicHow", "Speaker, next to pause"));
	AddControlRow(ControlsDesktop, LOCTEXT("ActionHeader", "Action"), LOCTEXT("KbHeader", "Keyboard"), LOCTEXT("PadHeader", "Gamepad"));
	AddControlRow(ControlsDesktop, LOCTEXT("Move", "Move"), LOCTEXT("MoveKb", "W A S D / Arrows"), LOCTEXT("MovePad", "Left stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Look", "Look"), LOCTEXT("LookKb", "Mouse"), LOCTEXT("LookPad", "Right stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Jump", "Jump"), LOCTEXT("JumpKb", "Space"), LOCTEXT("JumpPad", "A"));
	AddControlRow(ControlsDesktop, LOCTEXT("Grab", "Pick up / Place / Drop"), LOCTEXT("GrabKb", "Left click / E"), LOCTEXT("GrabPad", "X"));
	AddControlRow(ControlsDesktop, LOCTEXT("Throw", "Throw (breaks it!)"), LOCTEXT("ThrowKb", "Right click"), LOCTEXT("ThrowPad", "Right trigger"));
	AddControlRow(ControlsDesktop, LOCTEXT("Hint", "Hint"), LOCTEXT("HintKb", "F"), LOCTEXT("HintPad", "Y"));
	AddControlRow(ControlsDesktop, LOCTEXT("Pause", "Pause"), LOCTEXT("PauseKb", "P / Esc"), LOCTEXT("PausePad", "Start"));

	AddHeader(Right, LOCTEXT("ScoringHeader", "SCORING"), 0.f, TEXT("coins"));
	ScorePlacedText = AddIconTextRow(Right, TEXT("ScorePlacedText"), TEXT("coffee"));
	ScoreBrokenText = AddIconTextRow(Right, TEXT("ScoreBrokenText"), TEXT("destroyed"));
	ScoreBonusText = AddIconTextRow(Right, TEXT("ScoreBonusText"), TEXT("timer"));
}

void ULTTBriefingScreen::AddPageNav(UVerticalBox* Box)
{
	UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PageNav"));
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Nav))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	auto MakeNavButton = [this](FName Name, const FText& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		LTTTheme::StyleButton(Button, LTTTheme::ButtonCream());
		UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 20, LTTTheme::Ink());
		Text->SetText(Label);
		Button->SetContent(Text);
		return Button;
	};
	PrevPageButton = MakeNavButton(TEXT("PrevPageButton"), LOCTEXT("PrevPage", "<  BACK"));
	NextPageButton = MakeNavButton(TEXT("NextPageButton"), LOCTEXT("NextPage", "NEXT  >"));

	UVerticalBox* Middle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PageIndicator"));
	PageDots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PageDots"));
	if (UVerticalBoxSlot* S = Middle->AddChildToVerticalBox(PageDots))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}
	PageLabel = MakeText(TEXT("PageLabel"), 14, Muted);
	PageLabel->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Middle->AddChildToVerticalBox(PageLabel))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	if (UHorizontalBoxSlot* S = Nav->AddChildToHorizontalBox(PrevPageButton))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* S = Nav->AddChildToHorizontalBox(Middle))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* S = Nav->AddChildToHorizontalBox(NextPageButton))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
}

UButton* ULTTBriefingScreen::MakeLeaderboardButton()
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LeaderboardButton"));
	LTTTheme::StyleButton(Button, LTTTheme::ButtonCream());
	Button->SetToolTipText(LOCTEXT("LeaderboardTip", "See the top Rookies"));
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, TEXT("LeaderboardButton_Icon"), TEXT("diamond"), 28.f)))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	UTextBlock* Label = MakeText(TEXT("LeaderboardButton_Label"), 16, LTTTheme::Ink());
	Label->SetText(LOCTEXT("Leaderboard", "Leaderboard"));
	if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(Label))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	Button->SetContent(Content);
	return Button;
}

FText ULTTBriefingScreen::GetPageTitle(int32 Index) const
{
	switch (Index)
	{
	case 0: return LOCTEXT("PageNote", "DIRECTOR'S NOTE");
	case 1: return LOCTEXT("PageHowTo", "HOW TO PLAY");
	default: return LOCTEXT("PageRules", "CONTROLS & SCORING");
	}
}

void ULTTBriefingScreen::ShowPage(int32 Index)
{
	if (!Pages)
	{
		return;
	}
	const int32 Count = FMath::Max(Pages->GetNumWidgets(), 1);
	const int32 NewIndex = FMath::Clamp(Index, 0, Count - 1);
	if (NewIndex != PageIndex)
	{
		PageSlideDir = NewIndex > PageIndex ? 1.f : -1.f;
		PageChangedAt = AnimTime;
	}
	PageIndex = NewIndex;
	Pages->SetActiveWidgetIndex(PageIndex);
	RefreshPageNav();
}

void ULTTBriefingScreen::RefreshPageNav()
{
	const int32 Count = Pages ? FMath::Max(Pages->GetNumWidgets(), 1) : BriefingPageCount;
	if (PrevPageButton)
	{
		PrevPageButton->SetIsEnabled(PageIndex > 0);
	}
	if (NextPageButton)
	{
		NextPageButton->SetIsEnabled(PageIndex < Count - 1);
	}
	if (PageLabel)
	{
		PageLabel->SetText(FText::Format(LOCTEXT("PageLabelFmt", "{0} / {1}  -  {2}"), FText::AsNumber(PageIndex + 1), FText::AsNumber(Count), GetPageTitle(PageIndex)));
	}
	if (PageDots)
	{
		for (int32 i = 0; i < PageDots->GetChildrenCount(); ++i)
		{
			USizeBox* DotSize = Cast<USizeBox>(PageDots->GetChildAt(i));
			UBorder* Dot = DotSize ? Cast<UBorder>(DotSize->GetContent()) : Cast<UBorder>(PageDots->GetChildAt(i));
			if (!Dot)
			{
				continue;
			}
			const bool bOn = i == PageIndex;
			LTTTheme::ApplyBrush(Dot, LTTTheme::Rounded(bOn ? LTTTheme::ButtonGold() : LTTTheme::PaperDeep(), -1.f, bOn ? LTTTheme::WoodDark() : LTTTheme::ButtonEdge(), 2.f));
			if (DotSize)
			{
				DotSize->SetWidthOverride(bOn ? 30.f : 14.f); // the current page is a wider pill
			}
		}
	}
}

void ULTTBriefingScreen::HandlePrevPageClicked()
{
	ShowPage(PageIndex - 1);
}

void ULTTBriefingScreen::HandleNextPageClicked()
{
	ShowPage(PageIndex + 1);
}

void ULTTBriefingScreen::OpenLeaderboard()
{
	CommitName(); // the panel marks "YOU" by player id; the name shown should be the one typed
	ULTTLeaderboardPanel::ShowPanel(this);
}

void ULTTBriefingScreen::HandleLeaderboardClicked()
{
	OpenLeaderboard();
}

void ULTTBriefingScreen::OpenNameKeyboard()
{
	if (NameKeyboard || !NameInput)
	{
		return;
	}
	NameKeyboard = ULTTVirtualKeyboard::Open(GetOwningPlayer(), NameInput, ULTTLeaderboardSubsystem::MaxNameLength, LOCTEXT("KeyboardTitle", "YOUR NAME"), KeyboardClass);
	if (NameKeyboard)
	{
		NameKeyboard->ReturnFocus = this;
		NameKeyboard->OnClosed.AddDynamic(this, &ULTTBriefingScreen::HandleKeyboardClosed);
	}
}

void ULTTBriefingScreen::HandleKeyboardClosed(const FString& FinalText)
{
	NameKeyboard = nullptr;
	KeyboardClosedAt = AnimTime;
	CommitName(); // empty = a new random name
	SetKeyboardFocus();
}

void ULTTBriefingScreen::AddNameRow(UVerticalBox* Box)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NameRow"));
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, TEXT("NameIcon"), TEXT("director_chair"), 40.f)))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	UTextBlock* Label = MakeText(TEXT("NameLabel"), 18, TapeYellow);
	Label->SetText(LOCTEXT("NameLabel", "YOUR NAME"));
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Label))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}
	NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInput"));
	LTTTheme::StyleTextBox(NameInput, 18);
	NameInput->SetJustification(ETextJustify::Center);
	USizeBox* InputWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NameInputWidth"));
	InputWidth->SetWidthOverride(280.f);
	InputWidth->SetContent(NameInput);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(InputWidth))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	RandomNameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RandomNameButton"));
	LTTTheme::StyleButton(RandomNameButton, LTTTheme::ButtonCream());
	RandomNameButton->SetToolTipText(LOCTEXT("RandomTip", "Pick a random name"));
	UTextBlock* RandomLabel = MakeText(TEXT("RandomNameButton_Label"), 16, LTTTheme::Ink());
	RandomLabel->SetText(LOCTEXT("Random", "Random"));
	RandomNameButton->SetContent(RandomLabel);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(RandomNameButton))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	}
	LeaderboardButton = MakeLeaderboardButton();
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LeaderboardButton))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	}
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
}

void ULTTBriefingScreen::CommitName()
{
	if (!NameInput)
	{
		return;
	}
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		const FString Kept = Board->SetPlayerName(NameInput->GetText().ToString());
		NameInput->SetText(FText::FromString(Kept));
	}
}

void ULTTBriefingScreen::HandleRandomNameClicked()
{
	if (ULTTLeaderboardSubsystem* Board = ULTTLeaderboardSubsystem::Get(this))
	{
		if (NameInput)
		{
			NameInput->SetText(FText::FromString(Board->RandomizePlayerName()));
		}
	}
	SetKeyboardFocus(); // Enter starts again, not the text box
}

void ULTTBriefingScreen::HandleNameChanged(const FText& Text)
{
	// Cap the length while typing (the server keeps 24 characters).
	const FString Str = Text.ToString();
	if (Str.Len() > ULTTLeaderboardSubsystem::MaxNameLength && NameInput)
	{
		NameInput->SetText(FText::FromString(Str.Left(ULTTLeaderboardSubsystem::MaxNameLength)));
	}
}

void ULTTBriefingScreen::HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	CommitName();
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SetKeyboardFocus(); // the next Enter starts the time trial
	}
}

void ULTTBriefingScreen::Setup(ALTTMessDirector* InDirector)
{
	Director = InDirector;
	AnimTime = 0.f;
	ApplyTexts();
}

void ULTTBriefingScreen::ApplyTexts()
{
	const ALTTMessDirector* D = Director.Get();
	// The player picks time trial or not with the start buttons, so the texts describe both.
	const int32 Minutes = FMath::Max(FMath::RoundToInt((D ? D->TimeLimitSeconds : 1800.f) / 60.f), 1);
	const FText HintKey = D ? D->HintKey.GetDisplayName() : LOCTEXT("F", "F");

	if (StoryText)
	{
		const FText Deadline = FText::Format(LOCTEXT("Deadline", "Racing the clock? You get {0} minutes. Otherwise, just get it done."), FText::AsNumber(Minutes));
		StoryText->SetText(FText::Format(LOCTEXT("Story",
			"Rookie!\n\n"
			"Last night's wrap party got out of hand and the whole cafe set got knocked to pieces. "
			"Today Dash and Juniper shoot their big first-date scene right here, and I want every cup, chair and croissant "
			"back on its mark before the cameras roll. {0}\n\n"
			"Break anything and it comes out of your paycheck. Don't make me yell CUT.\n\n"
			"- Marlowe Finch, Director"), Deadline));
	}

	if (HowToPlayText)
	{
		if (IsTouch())
		{
			HowToPlayText->SetText(LOCTEXT("HowToTouch",
				"-  Aim at a scattered prop and tap the hand to pick it up. A glowing silhouette shows where it belongs.\n"
				"-  Blue = its spot.  Yellow = you're aiming at it: tap the hand (it turns green, PLACE) to put it back.\n"
				"-  Orange = put the thing underneath back first (the table before the cup).\n"
				"-  Aim right at the silhouette. If the hand turns red, dropping there breaks it. Thrown props break too.\n"
				"-  Lost? Tap the light bulb to highlight the nearest prop and follow the arrow."));
		}
		else
		{
			HowToPlayText->SetText(FText::Format(LOCTEXT("HowTo",
				"-  Pick up a scattered prop. A glowing silhouette shows where it belongs.\n"
				"-  Blue = its spot.  Yellow = you're aiming at it: click to place it.\n"
				"-  Orange = put the thing underneath back first (the table before the cup).\n"
				"-  Aim right at the silhouette. Drop it just beside the mark and it slips and breaks. Thrown props break too.\n"
				"-  Lost? Press {0} to highlight the nearest prop and follow the arrow."), HintKey));
		}
	}

	const int32 PerPlaced = D ? D->PointsPerPlaced : 10;
	const int32 PerBroken = D ? D->PointsPerBroken : -15;
	const float PerSecond = D ? D->PointsPerSecondLeft : 1.f;
	FNumberFormattingOptions Signed;
	Signed.AlwaysSign = true;
	if (ScorePlacedText)
	{
		ScorePlacedText->SetText(FText::Format(LOCTEXT("ScorePlaced", "{0} for each prop back on its mark"), FText::AsNumber(PerPlaced, &Signed)));
	}
	if (ScoreBrokenText)
	{
		ScoreBrokenText->SetText(FText::Format(LOCTEXT("ScoreBroken", "{0} for each prop you break"), FText::AsNumber(PerBroken, &Signed)));
	}
	if (ScoreBonusText)
	{
		ScoreBonusText->SetText(FText::Format(LOCTEXT("ScoreBonus", "Time trial: {0} for every second left on the clock"), FText::AsNumber(PerSecond, &Signed)));
		if (UWidget* Row = ScoreBonusText->GetParent())
		{
			Row->SetVisibility(PerSecond > 0.f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void ULTTBriefingScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime;

	// Card fades in and rises into place (0.4 s).
	if (Card)
	{
		const float E = EaseOutCubic(FMath::Clamp(AnimTime / 0.4f, 0.f, 1.f));
		Card->SetRenderOpacity(E);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - E) * 40.f));

		// Fit the screen: on a small or short screen (phone, landscape browser) the whole card shrinks around
		// its centre until it fits, so the start buttons are always on screen and tappable (hit testing follows
		// the render scale). Full size whenever there's room. Works for the C++ layout and the WBP_ child.
		const FVector2D Avail = MyGeometry.GetLocalSize() - FVector2D(2.f * BriefingFitMargin);
		const FVector2D Want = Card->GetDesiredSize();
		float Fit = 1.f;
		if (Want.X > 1.f && Want.Y > 1.f && Avail.X > 1.f && Avail.Y > 1.f)
		{
			Fit = FMath::Clamp(FMath::Min(Avail.X / Want.X, Avail.Y / Want.Y), BriefingMinFitScale, 1.f);
		}
		Card->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Card->SetRenderScale(FVector2D(Fit, Fit));
	}
	// Page flip: the new page slides in from the side it came from (0.25 s).
	if (Pages)
	{
		const float T = PageChangedAt >= 0.f ? FMath::Clamp((AnimTime - PageChangedAt) / 0.25f, 0.f, 1.f) : 1.f;
		const float E = EaseOutCubic(T);
		Pages->SetRenderTranslation(FVector2D((1.f - E) * 70.f * PageSlideDir, 0.f));
		Pages->SetRenderOpacity(0.35f + 0.65f * E);
	}
	// Touch: the browser never brings up the phone's keyboard for a Slate text box, so ours opens instead.
	if (NameInput && !NameKeyboard && NameInput->HasKeyboardFocus() && IsTouch() && AnimTime - KeyboardClosedAt > 0.3f)
	{
		OpenNameKeyboard();
	}
	// Start buttons breathe (out of step) so it's obvious what to do.
	if (StartButton)
	{
		const float Pulse = 1.f + 0.04f * FMath::Sin(AnimTime * 4.f);
		StartButton->SetRenderScale(FVector2D(Pulse, Pulse));
	}
	if (NoTimerButton)
	{
		const float Pulse = 1.f + 0.03f * FMath::Sin(AnimTime * 4.f + PI);
		NoTimerButton->SetRenderScale(FVector2D(Pulse, Pulse));
	}
}

FReply ULTTBriefingScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (NameInput && NameInput->HasKeyboardFocus())
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent); // typing a name: N and Space are letters, not shortcuts
	}
	if (!InKeyEvent.IsRepeat() && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right))
	{
		StartGame(true);
		return FReply::Handled();
	}
	if (!InKeyEvent.IsRepeat() && (Key == EKeys::N || Key == EKeys::Gamepad_FaceButton_Top))
	{
		StartGame(false);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_RightShoulder)
	{
		ShowPage(PageIndex + 1);
		return FReply::Handled();
	}
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftShoulder)
	{
		ShowPage(PageIndex - 1);
		return FReply::Handled();
	}
	if (!InKeyEvent.IsRepeat() && (Key == EKeys::L || Key == EKeys::Gamepad_Special_Left))
	{
		OpenLeaderboard();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply ULTTBriefingScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A press that no button or text box took (on the page text, the card...): maybe the start of a swipe.
	// Touches arrive here too: Slate turns an unhandled touch into a mouse press.
	if (Pages)
	{
		SwipeStart = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		bSwipeTracking = true;
		if (TSharedPtr<SWidget> Slate = GetCachedWidget())
		{
			return FReply::Handled().CaptureMouse(Slate.ToSharedRef());
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULTTBriefingScreen::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bSwipeTracking)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
	bSwipeTracking = false;
	const FVector2D Delta = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - SwipeStart;
	if (FMath::Abs(Delta.X) > BriefingSwipeDistance && FMath::Abs(Delta.X) > FMath::Abs(Delta.Y) * 1.5f)
	{
		ShowPage(PageIndex + (Delta.X < 0.f ? 1 : -1)); // swipe left = next page
	}
	return FReply::Handled().ReleaseMouseCapture();
}

void ULTTBriefingScreen::HandleStartClicked()
{
	StartGame(true);
}

void ULTTBriefingScreen::HandleNoTimerClicked()
{
	StartGame(false);
}

void ULTTBriefingScreen::StartGame(bool bTimeTrial)
{
	CommitName(); // keep a typed name even if Enter was never pressed in the box
	if (ALTTMessDirector* D = Director.Get())
	{
		// Before StartFromBriefing: the countdown starts there only if the timer is on.
		D->SetTimerEnabled(bTimeTrial);
		D->StartFromBriefing();
	}
	else
	{
		RemoveFromParent();
	}
}

#undef LOCTEXT_NAMESPACE
