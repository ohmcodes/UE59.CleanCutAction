// Clean! Cut! Action!

#include "Shoot/LTTShootHUD.h"
#include "Shoot/LTTSceneDirector.h"
#include "Shoot/LTTEffectStation.h"
#include "UI/LTTUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "LTTShootHUD"

namespace
{
	const FLinearColor ShootRecRed = LTTTheme::Hex(0xE5, 0x39, 0x35);
	const FLinearColor ShootGreen = LTTTheme::Hex(0x66, 0xC2, 0x4A);
	const FLinearColor ShootDash = LTTTheme::Hex(0x9F, 0xD4, 0xFF);
	const FLinearColor ShootJuniper = LTTTheme::Hex(0xFF, 0xB3, 0xD1);

	FLinearColor ShootGradeColor(ELTTGrade Grade)
	{
		switch (Grade)
		{
		case ELTTGrade::Perfect: return LTTTheme::ButtonGold();
		case ELTTGrade::Good: return ShootGreen;
		case ELTTGrade::Late: return LTTTheme::Hex(0xFF, 0xB0, 0x60);
		default: return ShootRecRed;
		}
	}

	void ShootSetVisible(UWidget* Widget, bool bVisible)
	{
		if (Widget)
		{
			Widget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void ULTTShootHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
}

void ULTTShootHUD::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible); // display only
}

UTextBlock* ULTTShootHUD::MakeText(FName Name, int32 FontSize, bool bOverlay) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	if (bOverlay)
	{
		LTTTheme::StyleOverlayText(Text, FontSize);
	}
	else
	{
		LTTTheme::StyleText(Text, FontSize, LTTTheme::Ink());
	}
	return Text;
}

UTextBlock* ULTTShootHUD::AddCentered(UCanvasPanel* Root, UTextBlock* Text, FVector2D Anchor, FVector2D Offset) const
{
	Text->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(Text))
	{
		TextSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TextSlot->SetPosition(Offset);
		TextSlot->SetAutoSize(true);
	}
	return Text;
}

void ULTTShootHUD::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	const FSlateBrush PaperChip = LTTTheme::Rounded(LTTTheme::Hex(0xFB, 0xF7, 0xEC, 0.93f), 18.f, LTTTheme::GoldTrim(), 3.f);

	// Red tint over everything while in shot (first child = drawn under the rest).
	Vignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Vignette"));
	Vignette->SetBrush(FSlateBrush());
	Vignette->SetBrushColor(FLinearColor(1.f, 0.f, 0.f, 0.f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Vignette))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	// Top left: REC + timecode, take, score.
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RecPanel"));
		LTTTheme::ApplyBrush(Panel, PaperChip);
		Panel->SetPadding(FMargin(20.f, 10.f, 24.f, 10.f));
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecBox"));
		Panel->SetContent(Box);
		RecText = MakeText(TEXT("RecText"), 24, false);
		TakeText = MakeText(TEXT("TakeText"), 18, false);
		ScoreText = MakeText(TEXT("ScoreText"), 20, false);
		ScoreText->SetColorAndOpacity(FSlateColor(LTTTheme::Coin()));
		Box->AddChildToVerticalBox(RecText);
		Box->AddChildToVerticalBox(TakeText);
		Box->AddChildToVerticalBox(ScoreText);
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.f, 0.f));
			S->SetPosition(FVector2D(40.f, 32.f));
			S->SetAutoSize(true);
		}
	}

	// Top centre: take meter.
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MeterPanel"));
		LTTTheme::ApplyBrush(Panel, PaperChip);
		Panel->SetPadding(FMargin(18.f, 8.f, 18.f, 10.f));
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MeterBox"));
		Panel->SetContent(Box);
		UTextBlock* Label = MakeText(TEXT("MeterLabel"), 16, false);
		Label->SetText(LOCTEXT("TakeMeter", "TAKE METER"));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(LTTTheme::HeaderBrown()));
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Label))
		{
			S->SetHorizontalAlignment(HAlign_Center);
		}
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MeterSize"));
		Size->SetWidthOverride(340.f);
		Size->SetHeightOverride(20.f);
		MeterBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("MeterBar"));
		MeterBar->SetPercent(0.6f);
		Size->SetContent(MeterBar);
		Box->AddChildToVerticalBox(Size);
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.5f, 0.f));
			S->SetAlignment(FVector2D(0.5f, 0.f));
			S->SetPosition(FVector2D(0.f, 24.f));
			S->SetAutoSize(true);
		}
	}

	// Top right: cue card.
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CuePanel"));
		LTTTheme::ApplyBrush(Panel, PaperChip);
		Panel->SetPadding(FMargin(22.f, 10.f, 22.f, 12.f));
		CuePanel = Panel;
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CueBox"));
		Panel->SetContent(Box);
		CueTitleText = MakeText(TEXT("CueTitleText"), 30, false);
		CueWhereText = MakeText(TEXT("CueWhereText"), 18, false);
		CueWhereText->SetColorAndOpacity(FSlateColor(LTTTheme::HeaderBrown()));
		Box->AddChildToVerticalBox(CueTitleText);
		Box->AddChildToVerticalBox(CueWhereText);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CueBarSize"));
		Size->SetWidthOverride(280.f);
		Size->SetHeightOverride(14.f);
		CueBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CueBar"));
		Size->SetContent(CueBar);
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Size))
		{
			S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(1.f, 0.f));
			S->SetAlignment(FVector2D(1.f, 0.f));
			S->SetPosition(FVector2D(-40.f, 32.f));
			S->SetAutoSize(true);
		}
	}

	// Centre: warnings, cards, toasts.
	WarningText = AddCentered(Root, MakeText(TEXT("WarningText"), 34, true), FVector2D(0.5f, 0.f), FVector2D(0.f, 150.f));
	WarningText->SetText(LOCTEXT("InShot", "YOU'RE IN THE SHOT!"));
	WarningText->SetColorAndOpacity(FSlateColor(ShootRecRed));

	ActCardText = AddCentered(Root, WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActCardText")), FVector2D(0.5f, 0.38f), FVector2D::ZeroVector);
	LTTTheme::StyleTitle(ActCardText, 64);

	CutText = AddCentered(Root, WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CutText")), FVector2D(0.5f, 0.4f), FVector2D::ZeroVector);
	LTTTheme::StyleTitle(CutText, 96, ShootRecRed);

	ToastText = AddCentered(Root, MakeText(TEXT("ToastText"), 44, true), FVector2D(0.5f, 0.28f), FVector2D::ZeroVector);

	StationHintText = AddCentered(Root, MakeText(TEXT("StationHintText"), 20, true), FVector2D(0.5f, 0.5f), FVector2D(0.f, 70.f));
	StationHintText->SetColorAndOpacity(FSlateColor(ArrowColor));

	InstructionText = AddCentered(Root, MakeText(TEXT("InstructionText"), 22, true), FVector2D(MeterAnchor.X, MeterAnchor.Y), FVector2D(0.f, -52.f));

	// Bottom: Marlowe's megaphone line, then the subtitles.
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BarkPanel"));
		LTTTheme::ApplyBrush(Panel, LTTTheme::Rounded(LTTTheme::Hex(0xF4, 0xC4, 0x5A, 0.95f), -1.f, LTTTheme::WoodDark(), 3.f));
		Panel->SetPadding(FMargin(18.f, 6.f, 26.f, 6.f));
		BarkPanel = Panel;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BarkRow"));
		Panel->SetContent(Row);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, TEXT("BarkIcon"), TEXT("megaphone"), 40.f)))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}
		BarkLabel = MakeText(TEXT("BarkLabel"), 22, false);
		// Fixed wrap width, not auto-wrap: the panel hugs short lines and only long ones wrap.
		BarkLabel->SetWrapTextAt(1000.f);
		USizeBox* Wrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarkWrap"));
		Wrap->SetMaxDesiredWidth(1000.f);
		Wrap->SetContent(BarkLabel);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Wrap))
		{
			S->SetVerticalAlignment(VAlign_Center);
		}
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.5f, 1.f));
			S->SetAlignment(FVector2D(0.5f, 1.f));
			S->SetPosition(FVector2D(0.f, -190.f));
			S->SetAutoSize(true);
		}
	}
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SubtitlePanel"));
		LTTTheme::ApplyBrush(Panel, LTTTheme::Rounded(LTTTheme::Hex(0x2B, 0x1A, 0x0C, 0.78f), 16.f, LTTTheme::GoldTrim(), 2.f));
		Panel->SetPadding(FMargin(28.f, 10.f, 28.f, 14.f));
		SubtitlePanel = Panel;
		USizeBox* Wrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SubtitleWrap"));
		Wrap->SetMinDesiredWidth(1000.f);
		Wrap->SetMaxDesiredWidth(1000.f);
		Panel->SetContent(Wrap);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SubtitleBox"));
		Wrap->SetContent(Box);
		SpeakerText = MakeText(TEXT("SpeakerText"), 18, true);
		SpeakerText->SetJustification(ETextJustify::Center);
		LineText = MakeText(TEXT("LineText"), 24, true);
		LineText->SetJustification(ETextJustify::Center);
		LineText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(SpeakerText))
		{
			S->SetHorizontalAlignment(HAlign_Center);
		}
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(LineText))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
		}
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.5f, 1.f));
			S->SetAlignment(FVector2D(0.5f, 1.f));
			S->SetPosition(FVector2D(0.f, -60.f));
			S->SetAutoSize(true);
		}
	}
}

void ULTTShootHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Pulse += InDeltaTime;
	if (!Director.IsValid())
	{
		Director = ALTTSceneDirector::Get(this);
	}
	const ALTTSceneDirector* D = Director.Get();
	bMeterVisible = false;
	bArrowVisible = false;
	if (!D)
	{
		ShootSetVisible(CuePanel, false);
		ShootSetVisible(WarningText, false);
		ShootSetVisible(SubtitlePanel, false);
		ShootSetVisible(BarkPanel, false);
		ShootSetVisible(ActCardText, false);
		ShootSetVisible(CutText, false);
		ShootSetVisible(ToastText, false);
		ShootSetVisible(InstructionText, false);
		ShootSetVisible(StationHintText, false);
		return;
	}
	UpdateTopBar(*D);
	UpdateCueCard(*D);
	UpdateDialogue(*D);
	UpdateOverlays(*D);
	UpdateStation(*D);
}

void ULTTShootHUD::UpdateTopBar(const ALTTSceneDirector& D)
{
	if (RecText)
	{
		if (D.IsRolling())
		{
			const float Seconds = D.GetRollSeconds();
			const int32 Whole = FMath::FloorToInt(Seconds);
			FNumberFormattingOptions Two;
			Two.MinimumIntegralDigits = 2;
			RecText->SetText(FText::Format(LOCTEXT("Rec", "REC  {0}:{1}:{2}"), FText::AsNumber(Whole / 60, &Two), FText::AsNumber(Whole % 60, &Two),
				FText::AsNumber(FMath::FloorToInt(FMath::Frac(Seconds) * 24.f), &Two)));
			// The REC light blinks once a second.
			RecText->SetColorAndOpacity(FSlateColor(FMath::Frac(Pulse) < 0.6f ? ShootRecRed : LTTTheme::InkMuted()));
		}
		else
		{
			RecText->SetText(D.HasWrapped() ? LOCTEXT("Wrapped", "THAT'S A WRAP") : LOCTEXT("Standby", "STANDBY"));
			RecText->SetColorAndOpacity(FSlateColor(LTTTheme::InkMuted()));
		}
	}
	if (TakeText)
	{
		TakeText->SetText(D.GetCurrentAct().IsNone()
			? FText::Format(LOCTEXT("Take", "Scene 42 - Take {0}"), FText::AsNumber(D.GetTakeNumber()))
			: FText::Format(LOCTEXT("TakeAct", "Scene 42 - Take {0} - Act {1}"), FText::AsNumber(D.GetTakeNumber()), FText::FromName(D.GetCurrentAct())));
	}
	if (ScoreText)
	{
		ScoreText->SetText(FText::Format(LOCTEXT("Score", "{0} points"), FText::AsNumber(D.GetScore())));
	}
	if (MeterBar)
	{
		const float Fraction = D.GetTakeMeter() / 100.f;
		MeterBar->SetPercent(Fraction);
		MeterBar->SetFillColorAndOpacity(Fraction > 0.5f ? ShootGreen : Fraction > 0.25f ? LTTTheme::ButtonGold() : ShootRecRed);
	}
}

void ULTTShootHUD::UpdateCueCard(const ALTTSceneDirector& D)
{
	const ALTTEffectStation* Station = D.GetActiveStation();
	const ELTTCue Upcoming = D.GetUpcomingCue();
	const bool bActive = Station != nullptr;
	if (!bActive)
	{
		Station = Upcoming != ELTTCue::None ? D.FindStation(Upcoming) : nullptr;
	}
	ShootSetVisible(CuePanel, Station != nullptr);
	if (!Station)
	{
		return;
	}
	const ELTTCue Cue = bActive ? D.GetActiveCue() : Upcoming;
	if (CueTitleText)
	{
		CueTitleText->SetText(FText::Format(bActive ? LOCTEXT("CueNow", "CUE: {0}!") : LOCTEXT("CueNext", "NEXT: {0}"), ULTTSceneData::GetCueName(Cue)));
		CueTitleText->SetColorAndOpacity(FSlateColor(bActive ? LTTTheme::Bad() : LTTTheme::InkMuted()));
		const float Scale = bActive ? 1.f + 0.04f * FMath::Sin(Pulse * 8.f) : 1.f;
		CueTitleText->SetRenderScale(FVector2D(Scale, Scale));
	}
	if (CueWhereText)
	{
		CueWhereText->SetText(Station->FloorLabel.IsEmpty() ? Station->DisplayName
			: FText::Format(LOCTEXT("Where", "{0}  ({1})"), Station->DisplayName, Station->FloorLabel));
	}
	if (CueBar)
	{
		ShootSetVisible(CueBar->GetParent(), bActive);
		const float Left = D.GetCueTimeFraction();
		CueBar->SetPercent(Left);
		CueBar->SetFillColorAndOpacity(Left > 0.6f ? ShootGreen : Left > 0.2f ? LTTTheme::ButtonGold() : ShootRecRed);
	}
}

void ULTTShootHUD::UpdateDialogue(const ALTTSceneDirector& D)
{
	FLTTDialogueLine Line;
	const bool bTalking = D.GetCurrentLine(Line);
	ShootSetVisible(SubtitlePanel, bTalking);
	if (bTalking)
	{
		if (SpeakerText)
		{
			SpeakerText->SetText(D.GetSpeakerLabel(Line));
			const FName Speaker = Line.Speaker;
			const bool bDash = Speaker == TEXT("Dash") || Speaker == TEXT("Leo");
			const bool bJuniper = Speaker == TEXT("Juniper") || Speaker == TEXT("Clara");
			SpeakerText->SetColorAndOpacity(FSlateColor(bDash ? ShootDash : bJuniper ? ShootJuniper : LTTTheme::ButtonGold()));
		}
		if (LineText)
		{
			LineText->SetText(Line.Text);
			// Off-script lines read a little dimmer, like an aside.
			LineText->SetColorAndOpacity(FSlateColor(Line.bOffScript ? LTTTheme::Hex(0xE8, 0xDC, 0xC4) : LTTTheme::TitleCream()));
		}
	}

	const FText BarkLine = D.GetBarkText();
	ShootSetVisible(BarkPanel, !BarkLine.IsEmpty());
	if (BarkLabel && !BarkLine.IsEmpty())
	{
		BarkLabel->SetText(BarkLine);
	}
}

void ULTTShootHUD::UpdateOverlays(const ALTTSceneDirector& D)
{
	const bool bInShot = D.IsPlayerInFrame();
	ShootSetVisible(WarningText, bInShot);
	if (Vignette)
	{
		// 2 Hz pulse.
		const float Alpha = bInShot ? 0.10f + 0.08f * (0.5f + 0.5f * FMath::Sin(Pulse * 4.f * PI)) : 0.f;
		Vignette->SetBrushColor(FLinearColor(1.f, 0.f, 0.f, Alpha));
	}

	const FText Card = D.IsCutShowing() ? FText::GetEmpty() : D.GetActCardText();
	ShootSetVisible(ActCardText, !Card.IsEmpty());
	if (ActCardText && !Card.IsEmpty())
	{
		ActCardText->SetText(Card);
	}

	ShootSetVisible(CutText, D.IsCutShowing());
	if (CutText && D.IsCutShowing())
	{
		CutText->SetText(FText::Format(LOCTEXT("Cut", "CUT!\nTAKE {0}"), FText::AsNumber(D.GetTakeNumber())));
	}

	int32 Points = 0;
	const ELTTGrade Grade = D.GetGradeToast(Points);
	ShootSetVisible(ToastText, Grade != ELTTGrade::None && !D.IsCutShowing());
	if (ToastText && Grade != ELTTGrade::None)
	{
		ToastText->SetText(Points > 0 ? FText::Format(LOCTEXT("Toast", "{0}  +{1}"), ULTTSceneData::GetGradeName(Grade), FText::AsNumber(Points))
			: ULTTSceneData::GetGradeName(Grade));
		ToastText->SetColorAndOpacity(FSlateColor(ShootGradeColor(Grade)));
	}
}

void ULTTShootHUD::UpdateStation(const ALTTSceneDirector& D)
{
	const ALTTEffectStation* Station = D.GetActiveStation();
	const bool bUsing = Station && Station->IsOccupied();
	ShootSetVisible(InstructionText, bUsing);
	if (bUsing)
	{
		if (InstructionText)
		{
			InstructionText->SetText(Station->GetInstruction());
		}
		MeterBand = Station->GetBand();
		MeterValue = Station->GetMeter();
		MeterProgress = Station->GetProgress();
		bMeterVisible = Station->Game != ELTTStationGame::Press && Station->Game != ELTTStationGame::Choice;
	}

	// Not at the station yet: say where it is and point at it.
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const bool bGuide = Station && !bUsing && Pawn && PC->PlayerCameraManager;
	ShootSetVisible(StationHintText, bGuide);
	if (!bGuide)
	{
		return;
	}
	const FVector Target = Station->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector PawnLoc = Pawn->GetActorLocation();
	if (StationHintText)
	{
		const int32 Meters = FMath::RoundToInt(static_cast<float>(FVector::Dist(PawnLoc, Target)) / 100.f);
		const float Up = static_cast<float>(Target.Z - PawnLoc.Z);
		const FText Floor = Up > FloorHeight ? LOCTEXT("Upstairs", " (upstairs)") : Up < -FloorHeight ? LOCTEXT("Downstairs", " (downstairs)") : FText::GetEmpty();
		StationHintText->SetText(FText::Format(LOCTEXT("StationHint", "{0} - {1} m{2}"),
			Station->DisplayName.IsEmpty() ? ULTTSceneData::GetCueName(D.GetActiveCue()) : Station->DisplayName, FText::AsNumber(Meters), Floor));
	}
	if (!bShowStationArrow)
	{
		return;
	}
	const FVector Local = PC->PlayerCameraManager->GetCameraRotation().UnrotateVector(Target - PC->PlayerCameraManager->GetCameraLocation());
	const float Dist = static_cast<float>(Local.Size());
	if (Dist < KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(Local.X) / Dist, -1.f, 1.f)));
	if (Angle <= ArrowHideAngle)
	{
		return; // looking at it
	}
	FVector2f Dir(static_cast<float>(Local.Y), static_cast<float>(-Local.Z));
	if (Dir.SizeSquared() < 1e-4f)
	{
		Dir = FVector2f(0.f, 1.f);
	}
	ArrowDir = Dir.GetSafeNormal();
	bArrowVisible = true;
}

int32 ULTTShootHUD::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2f Size(static_cast<float>(LocalSize.X), static_cast<float>(LocalSize.Y));

	if (bMeterVisible)
	{
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FVector2f BarSize(static_cast<float>(MeterSize.X), static_cast<float>(MeterSize.Y));
		const FVector2f Origin(Size.X * static_cast<float>(MeterAnchor.X) - BarSize.X * 0.5f, Size.Y * static_cast<float>(MeterAnchor.Y) - BarSize.Y * 0.5f);
		auto Box = [&](const FVector2f& Pos, const FVector2f& BoxSize, const FLinearColor& Color)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, MaxLayer, AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Color);
		};

		++MaxLayer;
		Box(Origin - FVector2f(4.f, 4.f), BarSize + FVector2f(8.f, 8.f), LTTTheme::WoodDark());
		++MaxLayer;
		Box(Origin, BarSize, LTTTheme::Hex(0x2B, 0x1A, 0x0C));
		if (MeterBand.X <= MeterBand.Y)
		{
			const float Lo = FMath::Clamp(static_cast<float>(MeterBand.X), 0.f, 1.f);
			const float Hi = FMath::Clamp(static_cast<float>(MeterBand.Y), 0.f, 1.f);
			++MaxLayer;
			Box(Origin + FVector2f(BarSize.X * Lo, 0.f), FVector2f(BarSize.X * (Hi - Lo), BarSize.Y), ShootGreen);
		}
		// The needle.
		const float X = BarSize.X * FMath::Clamp(MeterValue, 0.f, 1.f);
		++MaxLayer;
		Box(Origin + FVector2f(X - 4.f, -8.f), FVector2f(8.f, BarSize.Y + 16.f), LTTTheme::WoodDark());
		++MaxLayer;
		Box(Origin + FVector2f(X - 2.f, -6.f), FVector2f(4.f, BarSize.Y + 12.f), LTTTheme::TitleCream());
		// How much of the job is done (Sustain).
		if (MeterProgress >= 0.f)
		{
			const FVector2f ProgressPos = Origin + FVector2f(0.f, BarSize.Y + 14.f);
			++MaxLayer;
			Box(ProgressPos - FVector2f(2.f, 2.f), FVector2f(BarSize.X + 4.f, 14.f), LTTTheme::WoodDark());
			++MaxLayer;
			Box(ProgressPos, FVector2f(BarSize.X * FMath::Clamp(MeterProgress, 0.f, 1.f), 10.f), LTTTheme::ButtonGold());
		}
	}

	if (bArrowVisible)
	{
		const FVector2f Center = Size * 0.5f;
		const float S = 28.f * (1.f + 0.12f * FMath::Sin(Pulse * 6.f));
		const FVector2f Perp(-ArrowDir.Y, ArrowDir.X);
		const FVector2f Tip = Center + ArrowDir * (Size.Y * 0.22f + S);
		const FVector2f Back = Tip - ArrowDir * S;
		const TArray<FVector2f> Chevron = { Back + Perp * S * 0.8f, Tip, Back - Perp * S * 0.8f };
		const FPaintGeometry Geo = AllottedGeometry.ToPaintGeometry();
		++MaxLayer;
		FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, LTTTheme::Hex(0x5A, 0x34, 0x15, 0.85f), true, 10.f);
		++MaxLayer;
		FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, ArrowColor, true, 6.f);
	}
	return MaxLayer;
}

#undef LOCTEXT_NAMESPACE
