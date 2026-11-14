// Clean! Cut! Action!

#include "Shoot/LTTShootEndScreen.h"
#include "Shoot/LTTSceneDirector.h"
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
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "LTTShootEndScreen"

void ULTTShootEndScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (ShootAgainButton)
	{
		ShootAgainButton->OnClicked.AddDynamic(this, &ULTTShootEndScreen::HandleShootAgainClicked);
	}
	if (CleanupButton)
	{
		CleanupButton->OnClicked.AddDynamic(this, &ULTTShootEndScreen::HandleCleanupClicked);
	}
}

UTextBlock* ULTTShootEndScreen::MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	LTTTheme::StyleText(Text, FontSize, Color);
	return Text;
}

UButton* ULTTShootEndScreen::MakeButton(FName Name, const FText& Label, const FLinearColor& Color) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	LTTTheme::StyleButton(Button, Color);
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 22, LTTTheme::Ink());
	Text->SetText(Label);
	Button->SetContent(Text);
	return Button;
}

void ULTTShootEndScreen::AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock* Value) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelText = MakeText(NAME_None, 22, LTTTheme::InkMuted());
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

void ULTTShootEndScreen::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	LTTTheme::StyleBackdrop(Dim, 0.6f);
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
	Width->SetMinDesiredWidth(560.f);
	Width->SetMaxDesiredWidth(680.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBanner"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	LTTTheme::StyleTitle(TitleText, 56);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	TitleText->SetText(LOCTEXT("Title", "THAT'S A WRAP!"));
	LTTTheme::FillRibbonBanner(WidgetTree, Banner, TitleText, TEXT("clapperboard"), nullptr, 72.f);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Banner))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	StarsText = MakeText(TEXT("StarsText"), 28, LTTTheme::Coin());
	StarsText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(StarsText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	QuoteText = MakeText(TEXT("QuoteText"), 20, LTTTheme::Ink());
	QuoteText->SetJustification(ETextJustify::Center);
	QuoteText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(QuoteText))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 6.f, 0.f, 22.f));
	}

	CuesValue = MakeText(TEXT("CuesValue"), 20, LTTTheme::Ink());
	CutsValue = MakeText(TEXT("CutsValue"), 22, LTTTheme::Bad());
	FrameValue = MakeText(TEXT("FrameValue"), 22, LTTTheme::Ink());
	PointsValue = MakeText(TEXT("PointsValue"), 36, LTTTheme::Coin());
	AddStatRow(Column, LOCTEXT("RowCues", "Cues"), CuesValue);
	AddStatRow(Column, LOCTEXT("RowCuts", "CUTs"), CutsValue);
	AddStatRow(Column, LOCTEXT("RowFrame", "Time in the shot"), FrameValue);
	AddStatRow(Column, LOCTEXT("RowPoints", "Points"), PointsValue);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	ShootAgainButton = MakeButton(TEXT("ShootAgainButton"), LOCTEXT("ShootAgain", "Shoot Again"), LTTTheme::ButtonGold());
	CleanupButton = MakeButton(TEXT("CleanupButton"), LOCTEXT("Cleanup", "Back to Cleanup"), LTTTheme::ButtonCream());
	for (UButton* Button : { ShootAgainButton.Get(), CleanupButton.Get() })
	{
		if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(Button))
		{
			S->SetPadding(FMargin(8.f, 0.f));
		}
	}
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Buttons))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 26.f, 0.f, 0.f));
	}

	UTextBlock* Copyright = MakeText(TEXT("CopyrightText"), 13, LTTTheme::InkMuted());
	Copyright->SetText(FText::FromString(TEXT("Copyright © 2026 QLE OPC. All Rights Reserved.")));
	Copyright->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Copyright))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
}

void ULTTShootEndScreen::Setup(ALTTSceneDirector* InDirector)
{
	Director = InDirector;
	AnimTime = 0.f;
	const ALTTSceneDirector* D = InDirector;
	if (!D)
	{
		return;
	}
	FinalPoints = D->GetScore();
	const int32 Stars = D->GetStars();
	if (StarsText)
	{
		StarsText->SetText(FText::Format(Stars >= 3 ? LOCTEXT("Stars3", "DIRECTOR'S CUT  -  {0} / 3 STARS") : LOCTEXT("Stars", "{0} / 3 STARS"), FText::AsNumber(Stars)));
	}
	if (QuoteText)
	{
		QuoteText->SetText(FText::Format(LOCTEXT("Quote", "{0}  - Marlowe"), D->GetWrapQuote()));
	}
	if (CuesValue)
	{
		CuesValue->SetText(FText::Format(LOCTEXT("Cues", "{0} perfect, {1} good, {2} late, {3} missed"),
			FText::AsNumber(D->GetGradeCount(ELTTGrade::Perfect)), FText::AsNumber(D->GetGradeCount(ELTTGrade::Good)),
			FText::AsNumber(D->GetGradeCount(ELTTGrade::Late)), FText::AsNumber(D->GetGradeCount(ELTTGrade::Miss))));
	}
	if (CutsValue)
	{
		CutsValue->SetText(FText::AsNumber(D->GetCutCount()));
		CutsValue->SetColorAndOpacity(FSlateColor(D->GetCutCount() > 0 ? LTTTheme::Bad() : LTTTheme::Good()));
	}
	if (FrameValue)
	{
		FNumberFormattingOptions OneDecimal;
		OneDecimal.MinimumFractionalDigits = 1;
		OneDecimal.MaximumFractionalDigits = 1;
		FrameValue->SetText(FText::Format(LOCTEXT("Frame", "{0} s"), FText::AsNumber(D->GetSecondsInFrame(), &OneDecimal)));
	}
	if (CleanupButton)
	{
		CleanupButton->SetVisibility(D->CleanupLevelName.IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void ULTTShootEndScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimTime += InDeltaTime; // UMG ticks with real time, so this runs while the game is paused

	// Title slams down, card fades in, points roll up (same beats as the Phase 1 end screen).
	if (TitleText)
	{
		const float T = FMath::Clamp(AnimTime / 0.45f, 0.f, 1.f);
		const float Scale = FMath::Lerp(2.5f, 1.f, 1.f - FMath::Pow(1.f - T, 3.f));
		TitleText->SetRenderScale(FVector2D(Scale, Scale));
	}
	if (Card)
	{
		const float T = FMath::Clamp((AnimTime - 0.2f) / 0.35f, 0.f, 1.f);
		const float E = 1.f - FMath::Pow(1.f - T, 3.f);
		Card->SetRenderOpacity(FMath::Max(E, 0.35f));
		Card->SetRenderTranslation(FVector2D((1.f - E) * 120.f, 0.f));
	}
	if (PointsValue)
	{
		const float T = FMath::Clamp((AnimTime - 0.6f) / 1.f, 0.f, 1.f);
		PointsValue->SetText(FText::AsNumber(FMath::RoundToInt(FinalPoints * (1.f - FMath::Pow(1.f - T, 3.f)))));
	}
}

void ULTTShootEndScreen::HandleShootAgainClicked()
{
	ShootAgain();
}

void ULTTShootEndScreen::HandleCleanupClicked()
{
	BackToCleanup();
}

void ULTTShootEndScreen::ShootAgain()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ULTTShootEndScreen::BackToCleanup()
{
	const ALTTSceneDirector* D = Director.Get();
	if (!D || D->CleanupLevelName.IsNone())
	{
		return;
	}
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, D->CleanupLevelName);
}

#undef LOCTEXT_NAMESPACE
