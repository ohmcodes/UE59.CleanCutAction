// Clean! Cut! Action!

#include "UI/LTTEndingOverlay.h"
#include "UI/LTTUITheme.h"
#include "Interaction/LTTMessDirector.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "LTTEndingOverlay"

namespace LTTEndingOverlayPrivate // unique names: unity builds merge widget .cpps
{
	constexpr float ConfettiGravity = 620.f;   // px/s^2 (slow: paper flutters)
	constexpr float ConfettiDrag = 1.6f;       // per second, on velocity
	constexpr float ConfettiFadeTime = 0.8f;

	FLinearColor ConfettiColor(FRandomStream& Rng)
	{
		static const FLinearColor Colors[] = {
			LTTTheme::ButtonGold(), LTTTheme::Hex(0xE8, 0x5D, 0x4A), LTTTheme::Hex(0x5E, 0xB0, 0x4C),
			LTTTheme::Hex(0x4A, 0x9B, 0xE0), LTTTheme::Hex(0xF2, 0x8C, 0xC0), LTTTheme::TitleCream(),
			LTTTheme::Hex(0xA8, 0x6B, 0xE0), LTTTheme::Hex(0xFF, 0x9F, 0x2E) };
		return Colors[Rng.RandRange(0, UE_ARRAY_COUNT(Colors) - 1)];
	}
}

using namespace LTTEndingOverlayPrivate;

void ULTTEndingOverlay::Setup(ALTTMessDirector* InDirector)
{
	Director = InDirector;
}

void ULTTEndingOverlay::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	PieceBrush = FSlateColorBrush(FLinearColor::White);
	SetIsFocusable(true); // Space / Enter / gamepad A skip
	if (SkipButton)
	{
		SkipButton->OnClicked.AddDynamic(this, &ULTTEndingOverlay::HandleSkipClicked);
		SkipButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible); // only the button takes clicks
}

void ULTTEndingOverlay::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	SkipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkipButton"));
	LTTTheme::StyleButton(SkipButton, LTTTheme::ButtonCream());
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(LTTTheme::MakeArtIcon(WidgetTree, TEXT("SkipButton_Icon"), TEXT("clapperboard"), 40.f)))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkipButton_Label"));
	LTTTheme::StyleText(Label, 24, LTTTheme::Ink());
	Label->SetText(LOCTEXT("Skip", "SKIP  >>"));
	if (UHorizontalBoxSlot* S = Content->AddChildToHorizontalBox(Label))
	{
		S->SetVerticalAlignment(VAlign_Center);
	}
	SkipButton->SetContent(Content);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(SkipButton))
	{
		S->SetAnchors(FAnchors(1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(-48.f, -48.f));
		S->SetAutoSize(true);
	}
}

void ULTTEndingOverlay::SetSkipVisible(bool bVisible)
{
	if (SkipButton)
	{
		SkipButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	SkipShownAt = bVisible ? AnimTime : -1.f;
}

void ULTTEndingOverlay::PlayConfetti()
{
	SetSkipVisible(false);
	SetVisibility(ESlateVisibility::HitTestInvisible); // the end screen under it takes the clicks
	bConfettiPending = true; // spawned on the next tick, when the screen size is known
}

void ULTTEndingOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Dt = FMath::Min(InDeltaTime, 0.05f); // real time (UMG ticks while paused); clamp hitches
	AnimTime += Dt;
	const FVector2D Local = MyGeometry.GetLocalSize();
	if (Local.X > 1.f && Local.Y > 1.f)
	{
		ScreenSize = FVector2f(Local);
	}

	// SKIP pops in and gently pulses so it's noticed.
	if (SkipButton && SkipShownAt >= 0.f)
	{
		const float T = FMath::Clamp((AnimTime - SkipShownAt) / 0.3f, 0.f, 1.f);
		const float Pulse = 1.f + 0.035f * FMath::Sin(AnimTime * 4.f);
		SkipButton->SetRenderOpacity(T);
		SkipButton->SetRenderScale(FVector2D(Pulse, Pulse));
	}

	if (bConfettiPending)
	{
		bConfettiPending = false;
		bConfettiPlayed = true;
		FRandomStream Rng(FMath::Rand());
		const float Scale = FMath::Clamp(ScreenSize.Y / 1080.f, 0.5f, 2.f);
		Pieces.Reserve(Pieces.Num() + ConfettiCount);
		for (int32 i = 0; i < ConfettiCount; ++i)
		{
			FPiece P;
			const int32 Source = i % 3; // 0 = left cannon, 1 = right cannon, 2 = shower from the top
			if (Source < 2)
			{
				const bool bLeft = Source == 0;
				P.Pos = FVector2f(bLeft ? -10.f : ScreenSize.X + 10.f, ScreenSize.Y * Rng.FRandRange(0.78f, 0.95f));
				// Up and towards the middle, 50-80 degrees above the horizontal.
				const float Angle = FMath::DegreesToRadians(Rng.FRandRange(50.f, 80.f));
				const float Speed = Rng.FRandRange(900.f, 1700.f) * Scale;
				P.Vel = FVector2f(FMath::Cos(Angle) * Speed * (bLeft ? 1.f : -1.f), -FMath::Sin(Angle) * Speed);
				P.Age = -Rng.FRandRange(0.f, 0.25f); // a short stagger so it's a burst, not a block
			}
			else
			{
				P.Pos = FVector2f(Rng.FRandRange(0.f, ScreenSize.X), -Rng.FRandRange(10.f, ScreenSize.Y * 0.5f));
				P.Vel = FVector2f(Rng.FRandRange(-80.f, 80.f), Rng.FRandRange(60.f, 220.f)) * Scale;
				P.Age = -Rng.FRandRange(0.f, 0.6f);
			}
			P.Size = FVector2f(Rng.FRandRange(9.f, 16.f), Rng.FRandRange(5.f, 9.f)) * Scale;
			P.Angle = Rng.FRandRange(0.f, 2.f * PI);
			P.Spin = Rng.FRandRange(-9.f, 9.f);
			P.Flutter = Rng.FRandRange(0.f, 2.f * PI);
			P.Color = ConfettiColor(Rng);
			Pieces.Add(P);
		}
	}

	for (int32 i = Pieces.Num() - 1; i >= 0; --i)
	{
		FPiece& P = Pieces[i];
		P.Age += Dt;
		if (P.Age < 0.f)
		{
			continue; // not launched yet
		}
		P.Vel.Y += ConfettiGravity * Dt;
		P.Vel *= FMath::Max(0.f, 1.f - ConfettiDrag * Dt);
		P.Flutter += Dt * 6.f;
		P.Pos += P.Vel * Dt + FVector2f(FMath::Sin(P.Flutter) * 40.f * Dt, 0.f);
		P.Angle += P.Spin * Dt;
		if (P.Pos.Y > ScreenSize.Y + 40.f || P.Age > ConfettiLifetime + ConfettiFadeTime)
		{
			Pieces.RemoveAtSwap(i);
		}
	}

	if (bConfettiPlayed && Pieces.Num() == 0)
	{
		RemoveFromParent(); // all fallen
	}
}

int32 ULTTEndingOverlay::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (Pieces.Num() == 0)
	{
		return MaxLayer;
	}
	++MaxLayer;
	for (const FPiece& P : Pieces)
	{
		if (P.Age < 0.f)
		{
			continue;
		}
		// "Flip": the width pulses as if the paper turns over in the air; the back side is a little darker.
		const float Flip = FMath::Cos(P.Flutter * 1.3f);
		const FVector2f Size(FMath::Max(P.Size.X * FMath::Abs(Flip), 1.5f), P.Size.Y);
		FLinearColor Color = Flip < 0.f ? P.Color * 0.78f : P.Color;
		Color.A = 1.f - FMath::Clamp((P.Age - ConfettiLifetime) / ConfettiFadeTime, 0.f, 1.f);
		const FPaintGeometry Geo = AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(P.Pos - Size * 0.5f),
			FSlateRenderTransform(FQuat2f(P.Angle)), FVector2f(0.5f, 0.5f));
		FSlateDrawElement::MakeBox(OutDrawElements, MaxLayer, Geo, &PieceBrush, ESlateDrawEffect::None, Color);
	}
	return MaxLayer;
}

FReply ULTTEndingOverlay::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bSkipKey = Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::Escape
		|| Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right;
	if (!InKeyEvent.IsRepeat() && bSkipKey && SkipButton && SkipButton->IsVisible())
	{
		HandleSkipClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULTTEndingOverlay::HandleSkipClicked()
{
	SetSkipVisible(false);
	if (ALTTMessDirector* D = Director.Get())
	{
		D->SkipEnding();
	}
}

#undef LOCTEXT_NAMESPACE
