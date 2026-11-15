// Clean! Cut! Action!

#include "UI/LTTBarkWidget.h"
#include "UI/LTTUITheme.h"
#include "Interaction/LTTCleanupBarker.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
	// Marlowe: the Phase 2 bark's gold pill. Walkie: dark radio-brown so the actors read as someone else.
	FSlateBrush LTTBarkBrush(bool bWalkie)
	{
		return bWalkie
			? LTTTheme::Rounded(LTTTheme::Hex(0x3A, 0x26, 0x14, 0.92f), -1.f, LTTTheme::GoldTrim(), 3.f)
			: LTTTheme::Rounded(LTTTheme::Hex(0xF4, 0xC4, 0x5A, 0.95f), -1.f, LTTTheme::WoodDark(), 3.f);
	}
}

void ULTTBarkWidget::Setup(ALTTCleanupBarker* InBarker, UTexture2D* Icon)
{
	Barker = InBarker;
	IconTexture = Icon;
	if (BarkIcon && Icon)
	{
		FSlateBrush Brush = BarkIcon->GetBrush();
		Brush.SetResourceObject(Icon);
		BarkIcon->SetBrush(Brush);
	}
}

void ULTTBarkWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (BarkPanel)
	{
		BarkPanel->SetVisibility(ESlateVisibility::Collapsed);
		BarkPanel->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
	}
}

void ULTTBarkWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible); // display only
}

void ULTTBarkWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BarkPanel"));
	LTTTheme::ApplyBrush(Panel, LTTBarkBrush(false));
	Panel->SetPadding(FMargin(18.f, 6.f, 28.f, 8.f));
	BarkPanel = Panel;
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
	{
		S->SetAnchors(FAnchors(0.5f, 1.f));
		S->SetAlignment(FVector2D(0.5f, 1.f));
		S->SetPosition(FVector2D(0.f, -BottomOffset));
		S->SetAutoSize(true);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BarkRow"));
	Panel->SetContent(Row);

	BarkIcon = LTTTheme::MakeArtIcon(WidgetTree, TEXT("BarkIcon"), TEXT("megaphone"), 44.f);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(BarkIcon))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BarkColumn"));
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Column))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}

	SpeakerLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerLabel"));
	LTTTheme::StyleText(SpeakerLabel, 13, LTTTheme::WoodDark());
	Column->AddChildToVerticalBox(SpeakerLabel);

	BarkLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BarkLabel"));
	LTTTheme::StyleText(BarkLabel, 22, LTTTheme::Ink());
	// Fixed wrap width, not auto-wrap: the panel hugs short lines and only long ones wrap (as in the shoot HUD).
	BarkLabel->SetWrapTextAt(900.f);
	USizeBox* Wrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarkWrap"));
	Wrap->SetMaxDesiredWidth(900.f);
	Wrap->SetContent(BarkLabel);
	Column->AddChildToVerticalBox(Wrap);
}

void ULTTBarkWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!BarkPanel)
	{
		return;
	}

	FText Text;
	FText Speaker;
	bool bLineWalkie = false;
	const ALTTCleanupBarker* B = Barker.Get();
	const bool bHasLine = B && B->GetCurrentLine(Text, Speaker, bLineWalkie);

	if (bHasLine && B->GetLineSerial() != ShownSerial)
	{
		ShownSerial = B->GetLineSerial();
		PopTime = 0.f;
		if (BarkLabel)
		{
			BarkLabel->SetText(Text);
			BarkLabel->SetColorAndOpacity(FSlateColor(bLineWalkie ? LTTTheme::TitleCream() : LTTTheme::Ink()));
		}
		if (SpeakerLabel)
		{
			SpeakerLabel->SetText(Speaker);
			SpeakerLabel->SetColorAndOpacity(FSlateColor(bLineWalkie ? LTTTheme::GoldTrim() : LTTTheme::WoodDark()));
		}
		if (bLineWalkie != bWalkie)
		{
			bWalkie = bLineWalkie;
			if (UBorder* Border = Cast<UBorder>(BarkPanel))
			{
				LTTTheme::ApplyBrush(Border, LTTBarkBrush(bWalkie));
			}
		}
	}

	Alpha = FMath::FInterpConstantTo(Alpha, bHasLine ? 1.f : 0.f, InDeltaTime, bHasLine ? 8.f : 4.f);
	PopTime += InDeltaTime;

	// Pop in (OutBack) on each new line.
	const float T = FMath::Clamp(PopTime / 0.22f, 0.f, 1.f);
	constexpr float C1 = 1.70158f;
	constexpr float C3 = C1 + 1.f;
	const float Back = 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	const float Scale = FMath::Lerp(0.85f, 1.f, Back);

	BarkPanel->SetRenderScale(FVector2D(Scale, Scale));
	BarkPanel->SetRenderOpacity(Alpha);
	BarkPanel->SetVisibility(Alpha > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
