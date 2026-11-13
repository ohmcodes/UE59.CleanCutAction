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
	const FLinearColor BriefTapeYellow = LTTTheme::HeaderBrown(); // section headers
	const FLinearColor BriefSoftWhite = LTTTheme::Ink();
	const FLinearColor BriefMuted = LTTTheme::InkMuted();
	const FLinearColor NoteCream = LTTTheme::Ink();

	float BriefEaseOutCubic(float T)
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
		StartHint->SetText(bTouch ? LOCTEXT("StartHintTouch", "Tap to start") : LOCTEXT("StartHint", "Click, or press Enter / gamepad A"));
	}
}

UTextBlock* ULTTBriefingScreen::MakeText(FName Name, int32 FontSize, const FLinearColor& Color, bool bWrap) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	// Headers bold, body text regular weight so the long paragraphs stay light.
	LTTTheme::StyleText(Text, FontSize, Color, FontSize >= 18 || Color == BriefTapeYellow || Color == BriefMuted);
	Text->SetAutoWrapText(bWrap);
	return Text;
}

void ULTTBriefingScreen::AddHeader(UVerticalBox* Box, const FText& Text, float TopPadding) const
{
	UTextBlock* Header = MakeText(NAME_None, 20, BriefTapeYellow);
	Header->SetText(FText::Format(NSLOCTEXT("LTTBriefingScreen", "HeaderFlourish", "~ {0} ~"), Text));
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Header))
	{
		S->SetPadding(FMargin(0.f, TopPadding, 0.f, 6.f));
	}
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
	AddCell(Action, BriefMuted, 1.1f);
	AddCell(Keyboard, BriefSoftWhite, 1.f);
	AddCell(Gamepad, BriefSoftWhite, 0.8f);
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
			Icon->SetBrushFromTexture(Tex);
			Icon->SetDesiredSizeOverride(FVector2D(26.f, 26.f));
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
	AddCell(Action, BriefMuted, 0.8f);
	AddCell(How, BriefSoftWhite, 1.4f);
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

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	LTTTheme::StylePanel(CardBorder);
	CardBorder->SetPadding(FMargin(44.f, 30.f));
	Card = CardBorder;
	if (UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(CardBorder))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Width->SetWidthOverride(1080.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	// ---- Title ----
	// Title painted on a wooden banner.
	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	LTTTheme::StyleBanner(Banner);
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 50);
	TitleText->SetText(LOCTEXT("Title", "CLEAN! CUT! ACTION!"));
	TitleText->SetJustification(ETextJustify::Center);
	Banner->SetContent(TitleText);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	UTextBlock* Subtitle = MakeText(TEXT("SubtitleText"), 16, BriefMuted);
	Subtitle->SetText(LOCTEXT("Subtitle", "CALL SHEET  -  Last day of shooting  -  Scene 42: The Cafe"));
	Subtitle->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Subtitle))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
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
	AddHeader(Left, LOCTEXT("NoteHeader", "A NOTE FROM THE DIRECTOR"), 0.f);
	UBorder* Note = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotePaper"));
	LTTTheme::StyleInset(Note);
	Note->SetPadding(FMargin(14.f, 10.f));
	StoryText = MakeText(TEXT("StoryText"), 16, NoteCream, true);
	Note->SetContent(StoryText);
	Left->AddChildToVerticalBox(Note);

	AddHeader(Left, LOCTEXT("HowHeader", "HOW TO PLAY"), 16.f);
	HowToPlayText = MakeText(TEXT("HowToPlayText"), 15, BriefSoftWhite, true);
	Left->AddChildToVerticalBox(HowToPlayText);

	AddHeader(Right, LOCTEXT("ControlsHeader", "CONTROLS"), 0.f);
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
	AddControlRow(ControlsDesktop, FText::GetEmpty(), LOCTEXT("KbHeader", "Keyboard / Mouse"), LOCTEXT("PadHeader", "Gamepad"));
	AddControlRow(ControlsDesktop, LOCTEXT("Move", "Move"), LOCTEXT("MoveKb", "W A S D / Arrows"), LOCTEXT("MovePad", "Left stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Look", "Look"), LOCTEXT("LookKb", "Mouse"), LOCTEXT("LookPad", "Right stick"));
	AddControlRow(ControlsDesktop, LOCTEXT("Jump", "Jump"), LOCTEXT("JumpKb", "Space"), LOCTEXT("JumpPad", "A"));
	AddControlRow(ControlsDesktop, LOCTEXT("Grab", "Pick up / Place / Drop"), LOCTEXT("GrabKb", "Left click / E"), LOCTEXT("GrabPad", "X"));
	AddControlRow(ControlsDesktop, LOCTEXT("Throw", "Throw (breaks it!)"), LOCTEXT("ThrowKb", "Right click"), LOCTEXT("ThrowPad", "Right trigger"));
	AddControlRow(ControlsDesktop, LOCTEXT("Hint", "Hint"), LOCTEXT("HintKb", "F"), LOCTEXT("HintPad", "Y"));
	AddControlRow(ControlsDesktop, LOCTEXT("Pause", "Pause"), LOCTEXT("PauseKb", "P / Esc"), LOCTEXT("PausePad", "Start"));

	AddHeader(Right, LOCTEXT("ScoringHeader", "SCORING"), 16.f);
	ScoringText = MakeText(TEXT("ScoringText"), 15, BriefSoftWhite, true);
	Right->AddChildToVerticalBox(ScoringText);

	// ---- Start ----
	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
	LTTTheme::StyleButton(StartButton, LTTTheme::ButtonGold());
	UTextBlock* StartLabel = MakeText(TEXT("StartButton_Label"), 26, LTTTheme::Ink());
	StartLabel->SetText(LOCTEXT("Start", "START CLEANING!"));
	StartButton->SetContent(StartLabel);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StartButton))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}
	UTextBlock* StartHint = MakeText(TEXT("StartHint"), 13, BriefMuted);
	StartHint->SetText(LOCTEXT("StartHint", "Click, or press Enter / gamepad A"));
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
	const bool bTimer = D ? D->bUseTimer : true;
	const int32 Minutes = FMath::Max(FMath::RoundToInt((D ? D->TimeLimitSeconds : 1800.f) / 60.f), 1);
	const FText HintKey = D ? D->HintKey.GetDisplayName() : LOCTEXT("F", "F");

	if (StoryText)
	{
		const FText Deadline = bTimer
			? FText::Format(LOCTEXT("Deadline", "You have {0} minutes."), FText::AsNumber(Minutes))
			: LOCTEXT("NoDeadline", "Be quick about it.");
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

	if (ScoringText)
	{
		const int32 PerPlaced = D ? D->PointsPerPlaced : 10;
		const int32 PerBroken = D ? D->PointsPerBroken : -15;
		const float PerSecond = D ? D->PointsPerSecondLeft : 1.f;
		FNumberFormattingOptions Signed;
		Signed.AlwaysSign = true;
		FText Scoring = FText::Format(LOCTEXT("ScorePlaced", "{0} for each prop back on its mark\n{1} for each prop you break"),
			FText::AsNumber(PerPlaced, &Signed), FText::AsNumber(PerBroken, &Signed));
		if (bTimer && PerSecond > 0.f)
		{
			Scoring = FText::Format(LOCTEXT("ScoreBonus", "{0}\n{1} bonus for every second left on the clock when the set is clean"),
				Scoring, FText::AsNumber(PerSecond, &Signed));
		}
		ScoringText->SetText(Scoring);
	}
}

void ULTTBriefingScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime;

	// Card fades in and rises into place (0.4 s).
	if (Card)
	{
		const float E = BriefEaseOutCubic(FMath::Clamp(AnimTime / 0.4f, 0.f, 1.f));
		Card->SetRenderOpacity(E);
		Card->SetRenderTranslation(FVector2D(0.f, (1.f - E) * 40.f));
	}
	// Start button breathes so it's obvious what to do.
	if (StartButton)
	{
		const float Pulse = 1.f + 0.04f * FMath::Sin(AnimTime * 4.f);
		StartButton->SetRenderScale(FVector2D(Pulse, Pulse));
	}
}

FReply ULTTBriefingScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (!InKeyEvent.IsRepeat() && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right))
	{
		StartGame();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULTTBriefingScreen::HandleStartClicked()
{
	StartGame();
}

void ULTTBriefingScreen::StartGame()
{
	if (ALTTMessDirector* D = Director.Get())
	{
		D->StartFromBriefing();
	}
	else
	{
		RemoveFromParent();
	}
}

#undef LOCTEXT_NAMESPACE
