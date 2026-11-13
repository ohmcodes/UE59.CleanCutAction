// Clean! Cut! Action!

#include "UI/LTTBriefingScreen.h"
#include "UI/LTTUITheme.h"
#include "Interaction/LTTMessDirector.h"
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
}

void ULTTBriefingScreen::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.f;
	ApplyInputVariant();
	ApplyTexts();
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
	Width->SetWidthOverride(1080.f);
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

	// ---- Two columns: the story and rules | controls and scoring ----
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Columns"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Columns))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
	}

	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumn"));
	if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(Left))
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = 1.15f;
		S->SetSize(Size);
		S->SetPadding(FMargin(0.f, 0.f, 28.f, 0.f));
	}
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightColumn"));
	if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(Right))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// Marlowe's note, on a cream "paper" panel.
	AddHeader(Left, LOCTEXT("NoteHeader", "A NOTE FROM THE DIRECTOR"), 0.f, TEXT("megaphone"));
	UBorder* Note = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotePaper"));
	LTTTheme::StyleInset(Note);
	Note->SetPadding(FMargin(14.f, 10.f));
	StoryText = MakeText(TEXT("StoryText"), 16, NoteCream, true);
	Note->SetContent(StoryText);
	Left->AddChildToVerticalBox(Note);

	AddHeader(Left, LOCTEXT("HowHeader", "HOW TO PLAY"), 14.f, TEXT("checklist"));
	HowToPlayText = MakeText(TEXT("HowToPlayText"), 15, SoftWhite, true);
	Left->AddChildToVerticalBox(HowToPlayText);

	AddHeader(Right, LOCTEXT("ControlsHeader", "CONTROLS"), 0.f, TEXT("card"));
	// Both control lists are built (and named) so a WBP_ child can lay out each; ApplyInputVariant shows one.
	UVerticalBox* ControlsDesktop = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsDesktop"));
	UVerticalBox* ControlsTouch = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsTouch"));
	Right->AddChildToVerticalBox(ControlsDesktop);
	Right->AddChildToVerticalBox(ControlsTouch);
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
	AddControlRow(ControlsDesktop, LOCTEXT("ActionHeader", "Action"), LOCTEXT("KbHeader", "Keyboard"), LOCTEXT("PadHeader", "Gamepad"));
	AddControlRow(ControlsDesktop, LOCTEXT("Move", "Move"), LOCTEXT("MoveKb", "W A S D / Arrows"), LOCTEXT("MovePad", "Left stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Look", "Look"), LOCTEXT("LookKb", "Mouse"), LOCTEXT("LookPad", "Right stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Jump", "Jump"), LOCTEXT("JumpKb", "Space"), LOCTEXT("JumpPad", "A"));
	AddControlRow(ControlsDesktop, LOCTEXT("Grab", "Pick up / Place / Drop"), LOCTEXT("GrabKb", "Left click / E"), LOCTEXT("GrabPad", "X"));
	AddControlRow(ControlsDesktop, LOCTEXT("Throw", "Throw (breaks it!)"), LOCTEXT("ThrowKb", "Right click"), LOCTEXT("ThrowPad", "Right trigger"));
	AddControlRow(ControlsDesktop, LOCTEXT("Hint", "Hint"), LOCTEXT("HintKb", "F"), LOCTEXT("HintPad", "Y"));
	AddControlRow(ControlsDesktop, LOCTEXT("Pause", "Pause"), LOCTEXT("PauseKb", "P / Esc"), LOCTEXT("PausePad", "Start"));

	AddHeader(Right, LOCTEXT("ScoringHeader", "SCORING"), 14.f, TEXT("coins"));
	ScorePlacedText = AddIconTextRow(Right, TEXT("ScorePlacedText"), TEXT("coffee"));
	ScoreBrokenText = AddIconTextRow(Right, TEXT("ScoreBrokenText"), TEXT("destroyed"));
	ScoreBonusText = AddIconTextRow(Right, TEXT("ScoreBonusText"), TEXT("timer"));

	// ---- Start: time trial, or no clock ----
	UHorizontalBox* StartRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StartButtons"));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StartRow))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
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
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
