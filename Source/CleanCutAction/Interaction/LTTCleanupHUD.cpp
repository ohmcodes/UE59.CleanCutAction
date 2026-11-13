// Clean! Cut! Action!

#include "Interaction/LTTCleanupHUD.h"
#include "UI/LTTUITheme.h"
#include "Components/Border.h"
#include "CleanCutActionPlayerController.h"
#include "Interaction/LTTMessDirector.h"
#include "Interaction/LTTCarryComponent.h"
#include "Interaction/LTTInteractableBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/StaticMeshComponent.h"
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
	LTTTheme::StyleText(Text, FontSize, LTTTheme::Ink()); // sits on a paper panel
	return Text;
}

UImage* ULTTCleanupHUD::MakeIcon(FName Name, const TCHAR* TextureName, float Size) const
{
	return LTTTheme::MakeArtIcon(WidgetTree, Name, TextureName, Size);
}

void ULTTCleanupHUD::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top left: placed / destroyed / points.
	UBorder* StatsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatsPanel"));
	LTTTheme::ApplyBrush(StatsPanel, LTTTheme::Rounded(LTTTheme::Hex(0xFB, 0xF7, 0xEC, 0.93f), 18.f, LTTTheme::GoldTrim(), 3.f));
	StatsPanel->SetPadding(FMargin(20.f, 12.f, 24.f, 10.f));
	UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stats"));
	StatsPanel->SetContent(Stats);
	if (UCanvasPanelSlot* StatsSlot = Root->AddChildToCanvas(StatsPanel))
	{
		StatsSlot->SetAnchors(FAnchors(0.f, 0.f));
		StatsSlot->SetAlignment(FVector2D(0.f, 0.f));
		StatsSlot->SetPosition(FVector2D(40.f, 32.f));
		StatsSlot->SetAutoSize(true);
	}

	PlacedText = MakeText(TEXT("PlacedText"), 26);
	DestroyedText = MakeText(TEXT("DestroyedText"), 20);
	PointsText = MakeText(TEXT("PointsText"), 20);
	HintKeyText = MakeText(TEXT("HintKeyText"), 18);
	DestroyedText->SetColorAndOpacity(FSlateColor(LTTTheme::Bad()));
	PointsText->SetColorAndOpacity(FSlateColor(LTTTheme::Coin()));
	PlacedIcon = MakeIcon(TEXT("PlacedIcon"), TEXT("coffee"), 36.f);
	DestroyedIcon = MakeIcon(TEXT("DestroyedIcon"), TEXT("destroyed"), 36.f);
	PointsIcon = MakeIcon(TEXT("PointsIcon"), TEXT("diamond"), 36.f);
	HintIcon = MakeIcon(TEXT("HintIcon"), TEXT("lightbulb"), 36.f);

	// One row per stat: icon, then the count.
	auto AddRow = [this, Stats](FName RowName, UImage* Icon, UTextBlock* Text)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UVerticalBoxSlot* RowSlot = Stats->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
	};
	AddRow(TEXT("PlacedRow"), PlacedIcon, PlacedText);
	AddRow(TEXT("DestroyedRow"), DestroyedIcon, DestroyedText);
	AddRow(TEXT("PointsRow"), PointsIcon, PointsText);
	AddRow(TEXT("HintRow"), HintIcon, HintKeyText);

	// Top centre: countdown.
	TimerText = MakeText(TEXT("TimerText"), 40);
	TimerText->SetJustification(ETextJustify::Center);
	UBorder* TimerPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TimerPanel"));
	LTTTheme::ApplyBrush(TimerPanel, LTTTheme::Rounded(LTTTheme::Hex(0xFB, 0xF7, 0xEC, 0.93f), -1.f, LTTTheme::GoldTrim(), 3.f));
	TimerPanel->SetPadding(FMargin(18.f, 4.f, 30.f, 4.f));
	UHorizontalBox* TimerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TimerRow"));
	TimerPanel->SetContent(TimerRow);
	TimerIcon = MakeIcon(TEXT("TimerIcon"), TEXT("timer"), 48.f);
	if (UHorizontalBoxSlot* IconSlot = TimerRow->AddChildToHorizontalBox(TimerIcon))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}
	if (UHorizontalBoxSlot* TextSlot = TimerRow->AddChildToHorizontalBox(TimerText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UCanvasPanelSlot* TimerSlot = Root->AddChildToCanvas(TimerPanel))
	{
		TimerSlot->SetAnchors(FAnchors(0.5f, 0.f));
		TimerSlot->SetAlignment(FVector2D(0.5f, 0.f));
		TimerSlot->SetPosition(FVector2D(0.f, 24.f));
		TimerSlot->SetAutoSize(true);
	}

	// Under the crosshair: what the hint points at.
	HintText = MakeText(TEXT("HintText"), 20);
	HintText->SetJustification(ETextJustify::Center);
	LTTTheme::StyleOverlayText(HintText, 20, HintArrowColor);
	if (UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(HintText))
	{
		HintSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		HintSlot->SetAlignment(FVector2D(0.5f, 0.f));
		HintSlot->SetPosition(FVector2D(0.f, 60.f));
		HintSlot->SetAutoSize(true);
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
	UpdateTimerIcon();
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
	if (HintKeyText)
	{
		const float Cooldown = D ? D->GetHintCooldownRemaining() : 0.f;
		const FText Key = D ? D->HintKey.GetDisplayName() : FText::GetEmpty();
		HintKeyText->SetText(Cooldown > 0.f
			? FText::Format(LOCTEXT("HintCooldown", "[{0}] Hint ({1}s)"), Key, FText::AsNumber(FMath::CeilToInt(Cooldown)))
			: FText::Format(LOCTEXT("HintReady", "[{0}] Hint"), Key));
		HintKeyText->SetColorAndOpacity(FSlateColor(Cooldown > 0.f ? LTTTheme::InkMuted() : LTTTheme::HeaderBrown()));
		// Touch has its own Hint button (UI_TouchSimple), so the "[F] Hint" key prompt is desktop only.
		const ACleanCutActionPlayerController* CCPC = Cast<ACleanCutActionPlayerController>(GetOwningPlayer());
		const bool bTouch = CCPC && CCPC->ShouldUseTouchControls();
		const ESlateVisibility HintVis = D && !bTouch && !D->IsSetClean() && !D->IsTimeUp() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		HintKeyText->SetVisibility(HintVis);
		if (HintIcon)
		{
			HintIcon->SetVisibility(HintVis);
			HintIcon->SetRenderOpacity(Cooldown > 0.f ? 0.4f : 1.f);
		}
	}
	if (TimerText)
	{
		const bool bShow = D && (D->bUseTimer || D->IsTimeUp());
		TimerText->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UWidget* TimerPanel = GetWidgetFromName(TEXT("TimerPanel"));
		if (!TimerPanel)
		{
			TimerPanel = TimerText->GetParent();
		}
		if (TimerPanel)
		{
			TimerPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
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

void ULTTCleanupHUD::UpdateTimerIcon()
{
	if (!TimerIcon)
	{
		return;
	}
	const ALTTMessDirector* D = Director.Get();
	float Scale = 1.f;
	float Angle = 0.f;
	if (D && D->IsTimerRunning() && !D->IsTimeUp())
	{
		// Beat in step with the clock: the icon swells and settles back each time a second ticks off,
		// twice a second (bigger, with a little wobble) once time is low.
		const float Remaining = D->GetTimeRemaining();
		const bool bLow = Remaining <= LowTimeSeconds;
		const float Beats = bLow ? 2.f : 1.f;
		const float Phase = FMath::Frac(-Remaining * Beats); // 0 right on the tick, rising to 1 by the next
		const float Swell = FMath::Sin(Phase * PI);
		Scale = 1.f + (bLow ? LowTimePulseAmount : TimerPulseAmount) * Swell * Swell;
		Angle = bLow ? 8.f * FMath::Sin(Phase * 2.f * PI) : 0.f;
	}
	TimerIcon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	TimerIcon->SetRenderScale(FVector2D(Scale, Scale));
	TimerIcon->SetRenderTransformAngle(Angle);
	// Reddish tint (white is the icon's own colours) when time is low or up, matching the countdown text.
	TimerIcon->SetColorAndOpacity(D && (D->IsTimeUp() || (D->IsTimerRunning() && D->GetTimeRemaining() <= LowTimeSeconds))
		? FLinearColor(1.f, 0.6f, 0.55f) : FLinearColor::White);
}

void ULTTCleanupHUD::UpdateArrow()
{
	bArrowVisible = false;

	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const ULTTCarryComponent* Carry = Pawn ? Pawn->FindComponentByClass<ULTTCarryComponent>() : nullptr;
	const ALTTInteractableBase* Held = Carry ? Carry->GetHeldItem() : nullptr;
	const ALTTInteractableBase* Hinted = (Pawn && !Held && Director.IsValid()) ? Director->GetHintTarget() : nullptr;

	if (HintText)
	{
		if (Hinted && Hinted->Mesh)
		{
			const FVector Target = Hinted->Mesh->Bounds.Origin;
			const FVector PawnLoc = Pawn->GetActorLocation();
			const int32 Meters = FMath::RoundToInt(static_cast<float>(FVector::Dist(PawnLoc, Target)) / 100.f);
			const float Up = static_cast<float>(Target.Z - PawnLoc.Z);
			const FText Floor = Up > FloorHeight ? LOCTEXT("Upstairs", " (upstairs)")
				: Up < -FloorHeight ? LOCTEXT("Downstairs", " (downstairs)") : FText::GetEmpty();
			HintText->SetText(FText::Format(LOCTEXT("HintTarget", "{0} - {1} m{2}"), Hinted->GetPropName(), FText::AsNumber(Meters), Floor));
			HintText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			HintText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!bShowHomeArrow || !PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();

	if (Held)
	{
		if (Held->HasHome() && !Held->IsPlaced())
		{
			AimArrowAt(Held->GetHomeCenter(), CamLoc, CamRot, ArrowColor);
		}
	}
	else if (Hinted && Hinted->Mesh)
	{
		AimArrowAt(Hinted->Mesh->Bounds.Origin, CamLoc, CamRot, HintArrowColor);
	}
}

void ULTTCleanupHUD::AimArrowAt(const FVector& Target, const FVector& CamLoc, const FRotator& CamRot, const FLinearColor& Color)
{
	const FVector Local = CamRot.UnrotateVector(Target - CamLoc); // X forward, Y right, Z up
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
	ArrowDrawColor = Color;
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
	const FLinearColor Outline = LTTTheme::Hex(0x5A, 0x34, 0x15, 0.85f * ArrowDrawColor.A);
	FLinearColor Faded = ArrowDrawColor;
	Faded.A *= 0.6f;

	++MaxLayer;
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron2, ESlateDrawEffect::None, Outline, true, ArrowThickness + 4.f);
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, Outline, true, ArrowThickness + 4.f);
	++MaxLayer;
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron2, ESlateDrawEffect::None, Faded, true, ArrowThickness);
	FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer, Geo, Chevron, ESlateDrawEffect::None, ArrowDrawColor, true, ArrowThickness);
	return MaxLayer;
}

#undef LOCTEXT_NAMESPACE
