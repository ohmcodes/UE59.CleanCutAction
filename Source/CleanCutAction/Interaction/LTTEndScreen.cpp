// Clean! Cut! Action!

#include "Interaction/LTTEndScreen.h"
#include "Interaction/LTTMessDirector.h"
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
	const FLinearColor TapeYellow(1.f, 0.78f, 0.1f);
	const FLinearColor SoftWhite(0.95f, 0.95f, 0.92f);
	const FLinearColor Muted(0.7f, 0.7f, 0.68f);
	const FLinearColor RedTitle(1.f, 0.3f, 0.2f);

	float EaseOutBounce(float T)
	{
		const float N = 7.5625f, D = 2.75f;
		if (T < 1.f / D) { return N * T * T; }
		if (T < 2.f / D) { T -= 1.5f / D; return N * T * T + 0.75f; }
		if (T < 2.5f / D) { T -= 2.25f / D; return N * T * T + 0.9375f; }
		T -= 2.625f / D;
		return N * T * T + 0.984375f;
	}

	float EaseOutCubic(float T)
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
}

UTextBlock* ULTTEndScreen::MakeText(FName Name, int32 FontSize, const FLinearColor& Color) const
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

UButton* ULTTEndScreen::MakeButton(FName Name, const FText& Label, const FLinearColor& Color) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(Color);
	UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("_Label")), 22, FLinearColor(0.08f, 0.08f, 0.08f));
	Text->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Text->SetText(Label);
	Button->SetContent(Text);
	return Button;
}

void ULTTEndScreen::AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock* Value) const
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelText = MakeText(NAME_None, 22, Muted);
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
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	// The call-sheet card.
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
	Width->SetMinDesiredWidth(520.f);
	CardBorder->SetContent(Width);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	TitleText = MakeText(TEXT("TitleText"), 64, TapeYellow);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(TitleText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	SubtitleText = MakeText(TEXT("SubtitleText"), 20, SoftWhite);
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(SubtitleText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 4.f, 0.f, 24.f));
	}

	TimeValue = MakeText(TEXT("TimeValue"), 22, SoftWhite);
	ReturnedValue = MakeText(TEXT("ReturnedValue"), 22, SoftWhite);
	DestroyedValue = MakeText(TEXT("DestroyedValue"), 22, FLinearColor(1.f, 0.55f, 0.45f));
	HintsValue = MakeText(TEXT("HintsValue"), 22, SoftWhite);
	PointsValue = MakeText(TEXT("PointsValue"), 36, TapeYellow);
	AddStatRow(Column, LOCTEXT("RowTime", "Time on set"), TimeValue);
	AddStatRow(Column, LOCTEXT("RowReturned", "Props returned"), ReturnedValue);
	AddStatRow(Column, LOCTEXT("RowDestroyed", "Props destroyed"), DestroyedValue);
	AddStatRow(Column, LOCTEXT("RowHints", "Hints used"), HintsValue);
	AddStatRow(Column, LOCTEXT("RowPoints", "Points"), PointsValue);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	PlayAgainButton = MakeButton(TEXT("PlayAgainButton"), LOCTEXT("PlayAgain", "Play Again"), TapeYellow);
	KeepPlayingButton = MakeButton(TEXT("KeepPlayingButton"), LOCTEXT("KeepPlaying", "Keep Playing"), FLinearColor(0.75f, 0.75f, 0.75f));
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
}

void ULTTEndScreen::Setup(ALTTMessDirector* InDirector, bool bInSetClean)
{
	Director = InDirector;
	bSetClean = bInSetClean;
	FinalPoints = InDirector ? InDirector->GetScore() : 0;
	AnimTime = 0.f;
	ApplyTexts();
}

void ULTTEndScreen::ApplyTexts()
{
	const ALTTMessDirector* D = Director.Get();
	if (TitleText)
	{
		TitleText->SetText(bSetClean ? LOCTEXT("TitleClean", "SET READY!") : LOCTEXT("TitleTimeUp", "TIME'S UP!"));
		TitleText->SetColorAndOpacity(FSlateColor(bSetClean ? TapeYellow : RedTitle));
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
		DestroyedValue->SetText(FText::AsNumber(D->GetBrokenCount()));
	}
	if (HintsValue)
	{
		HintsValue->SetText(FText::AsNumber(D->GetHintsUsed()));
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
		const float E = EaseOutCubic(T);
		Card->SetRenderOpacity(FMath::Max(E, 0.35f));
		Card->SetRenderTranslation(FVector2D((1.f - E) * 120.f, 0.f));
	}
	// Points roll up (0.6 -> 1.6 s).
	if (PointsValue)
	{
		const float T = FMath::Clamp((AnimTime - 0.6f) / 1.f, 0.f, 1.f);
		PointsValue->SetText(FText::AsNumber(FMath::RoundToInt(FinalPoints * EaseOutCubic(T))));
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
