// Clean! Cut! Action!

#include "Interaction/LTTCleanupHUD.h"
#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "LTTCleanupHUD"

void ULTTCleanupHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
}

void ULTTCleanupHUD::NativeConstruct()
{
	Super::NativeConstruct();
	// Never block clicks or focus; this is display only.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UTextBlock* ULTTCleanupHUD::MakeText(FName Name, int32 FontSize) const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Text->SetFont(Font);
	Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	return Text;
}

void ULTTCleanupHUD::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top left: placed / destroyed / points.
	UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stats"));
	if (UCanvasPanelSlot* StatsSlot = Root->AddChildToCanvas(Stats))
	{
		StatsSlot->SetAnchors(FAnchors(0.f, 0.f));
		StatsSlot->SetAlignment(FVector2D(0.f, 0.f));
		StatsSlot->SetPosition(FVector2D(40.f, 32.f));
		StatsSlot->SetAutoSize(true);
	}

	PlacedText = MakeText(TEXT("PlacedText"), 26);
	DestroyedText = MakeText(TEXT("DestroyedText"), 20);
	PointsText = MakeText(TEXT("PointsText"), 20);
	DestroyedText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.55f, 0.45f)));
	PointsText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	for (UTextBlock* Text : { PlacedText.Get(), DestroyedText.Get(), PointsText.Get() })
	{
		if (UVerticalBoxSlot* TextSlot = Stats->AddChildToVerticalBox(Text))
		{
			TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
	}

	// Top centre: countdown.
	TimerText = MakeText(TEXT("TimerText"), 40);
	TimerText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* TimerSlot = Root->AddChildToCanvas(TimerText))
	{
		TimerSlot->SetAnchors(FAnchors(0.5f, 0.f));
		TimerSlot->SetAlignment(FVector2D(0.5f, 0.f));
		TimerSlot->SetPosition(FVector2D(0.f, 24.f));
		TimerSlot->SetAutoSize(true);
	}
}

void ULTTCleanupHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Director.IsValid())
	{
		Director = ALTTMessDirector::Get(this);
	}
	ArrowPulse += InDeltaTime;
	UpdateTexts();
	UpdateArrow();
}

void ULTTCleanupHUD::UpdateTexts()
{
	const ALTTMessDirector* D = Director.Get();
	if (PlacedText)
	{
		PlacedText->SetText(FText::Format(LOCTEXT("Placed", "{0}/{1} placed"),
			FText::AsNumber(D ? D->GetPlacedCount() : 0), FText::AsNumber(D ? D->GetTotalCount() : 0)));
	}
	if (DestroyedText)
	{
		DestroyedText->SetText(FText::Format(LOCTEXT("Destroyed", "{0} destroyed"), FText::AsNumber(D ? D->GetBrokenCount() : 0)));
	}
	if (PointsText)
	{
		PointsText->SetText(FText::Format(LOCTEXT("Points", "{0} points"), FText::AsNumber(D ? D->GetScore() : 0)));
	}
	if (TimerText)
	{
		const bool bShow = D && (D->bUseTimer || D->IsTimeUp());
		TimerText->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShow)
		{
			if (D->IsTimeUp())
			{
				TimerText->SetText(LOCTEXT("TimeUp", "TIME'S UP"));
				TimerText->SetColorAndOpacity(FSlateColor(LowTimeColor));
			}
			else
			{
				const float Remaining = D->GetTimeRemaining();
				const int32 Total = FMath::CeilToInt(Remaining);
				FNumberFormattingOptions TwoDigits;
				TwoDigits.MinimumIntegralDigits = 2;
				TimerText->SetText(FText::Format(LOCTEXT("Timer", "{0}:{1}"),
					FText::AsNumber(Total / 60), FText::AsNumber(Total % 60, &TwoDigits)));
				TimerText->SetColorAndOpacity(FSlateColor(Remaining <= LowTimeSeconds ? LowTimeColor : TimerColor));
			}
		}
	}
}

void ULTTCleanupHUD::UpdateArrow()
{
	bArrowVisible = false;
	if (!bShowHomeArrow)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const ULTTCarryComponent* Carry = Pawn ? Pawn->FindComponentByClass<ULTTCarryComponent>() : nullptr;
	const ALTTInteractableBase* Held = Carry ? Carry->GetHeldItem() : nullptr;
	if (!Held || !Held->HasHome() || Held->IsPlaced() || !PC->PlayerCameraManager)
	{
		return;
	}

	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	const FVector Local = CamRot.UnrotateVector(Held->GetHomeCenter() - CamLoc); // X forward, Y right, Z up
	const float Dist = static_cast<float>(Local.Size());
	if (Dist < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(Local.X) / Dist, -1.f, 1.f)));
	if (AngleDeg <= ArrowHideAngle)
	{
		return; // looking at it
	}

	FVector2f Dir(static_cast<float>(Local.Y), static_cast<float>(-Local.Z));
	if (Dir.SizeSquared() < 1e-4f)
	{
		Dir = FVector2f(0.f, 1.f); // straight behind: point down ("turn around")
	}
	ArrowDir = Dir.GetSafeNormal();
	bArrowVisible = true;
}

int32 ULTTCleanupHUD::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bArrowVisible)
	{
		return MaxLayer;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2f Size(static_cast<float>(LocalSize.X), static_cast<float>(LocalSize.Y));
	const FVector2f Center = Size * 0.5f;
	const float Pulse = 1.f + 0.12f * FMath::Sin(ArrowPulse * 6.f);
	const float S = ArrowSize * Pulse;
	const FVector2f Perp(-ArrowDir.Y, ArrowDir.X);

	// Chevron pointing at the home, on a ring around the crosshair.
	const FVector2f Tip = Center + ArrowDir * (Size.Y * ArrowRadius + S);
	const FVector2f Back = Tip - ArrowDir * S;
	TArray<FVector2f> Chevron = { Back + Perp * S * 0.8f, Tip, Back - Perp * S * 0.8f };
	const FVector2f Shift = -ArrowDir * S * 0.55f;
	TArray<FVector2f> Chevron2 = { Chevron[0] + Shift, Chevron[1] + Shift, Chevron[2] + Shift };

	const FPaintGeometry Geo = AllottedGeometry.ToPaintGeometry();
	const FLinearColor Outline(0.f, 0.f, 0.f, 0.7f * ArrowColor.A);
	FLinearColor Faded = ArrowColor;
	Faded.A *= 0.6f;

	++MaxLayer;
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron2, ESlateDrawEffect::None, Outline, true, ArrowThickness + 4.f);
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, Outline, true, ArrowThickness + 4.f);
	++MaxLayer;
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron2, ESlateDrawEffect::None, Faded, true, ArrowThickness);
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, ArrowColor, true, ArrowThickness);
	return MaxLayer;
}

#undef LOCTEXT_NAMESPACE
