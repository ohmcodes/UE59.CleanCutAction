// Clean! Cut! Action! - Shared "cozy cafe" UI theme: cream paper panels with gold trim, a wooden title
// banner, beige pill buttons and warm brown text. Every C++ widget builds its layout with these helpers,
// so the look lives in one place. Colours are authored in sRGB hex (what a paint program shows).

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

namespace LTTTheme
{
	inline FLinearColor Hex(uint8 R, uint8 G, uint8 B, float A = 1.f)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B));
		C.A = A;
		return C;
	}

	// ---- Palette ----
	inline FLinearColor Paper()       { return Hex(0xFB, 0xF7, 0xEC); }       // panel fill
	inline FLinearColor PaperDeep()   { return Hex(0xF2, 0xE9, 0xD6); }       // inset boxes (notes, rows)
	inline FLinearColor GoldTrim()    { return Hex(0xC9, 0xA1, 0x5A); }       // panel outline / ornaments
	inline FLinearColor Wood()        { return Hex(0x9A, 0x66, 0x33); }       // title banner
	inline FLinearColor WoodDark()    { return Hex(0x5A, 0x34, 0x15); }       // banner outline, title text outline
	inline FLinearColor Ink()         { return Hex(0x4A, 0x30, 0x18); }       // body text
	inline FLinearColor InkMuted()    { return Hex(0x8C, 0x73, 0x56); }       // secondary text
	inline FLinearColor TitleCream()  { return Hex(0xFF, 0xF8, 0xE6); }       // title letters
	inline FLinearColor HeaderBrown() { return Hex(0x7A, 0x4E, 0x22); }       // section headers
	inline FLinearColor ButtonCream() { return Hex(0xF5, 0xEA, 0xD3); }       // normal button
	inline FLinearColor ButtonGold()  { return Hex(0xF4, 0xC4, 0x5A); }       // primary button
	inline FLinearColor ButtonRed()   { return Hex(0xE8, 0x8B, 0x72); }       // quit / destructive
	inline FLinearColor ButtonEdge()  { return Hex(0xB0, 0x82, 0x48); }       // button outline
	inline FLinearColor Good()        { return Hex(0x5E, 0x94, 0x3C); }       // positive numbers
	inline FLinearColor Bad()         { return Hex(0xC0, 0x4A, 0x30); }       // broken props, time's up
	inline FLinearColor Coin()        { return Hex(0xC8, 0x8A, 0x10); }       // points
	inline FLinearColor Backdrop(float A = 0.6f) { return Hex(0x2B, 0x1A, 0x0C, A); } // dim behind menus

	// ---- Brushes ----
	/** Rounded box with an outline. Radius < 0 = pill (half-height corners). */
	inline FSlateBrush Rounded(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = OutlineWidth;
		if (Radius < 0.f)
		{
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		}
		else
		{
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		}
		return Brush;
	}

	inline void ApplyBrush(UBorder* Border, const FSlateBrush& Brush)
	{
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor::White); // the brush carries the colour
	}

	/** Cream paper card with a gold edge (menus, end screen). */
	inline void StylePanel(UBorder* Border, float Radius = 26.f)
	{
		ApplyBrush(Border, Rounded(Paper(), Radius, GoldTrim(), 4.f));
	}

	/** Darker paper inset (the director's note, HUD chips). */
	inline void StyleInset(UBorder* Border, float Radius = 14.f, float Alpha = 1.f)
	{
		FLinearColor Fill = PaperDeep();
		Fill.A = Alpha;
		FLinearColor Edge = GoldTrim();
		Edge.A = Alpha;
		ApplyBrush(Border, Rounded(Fill, Radius, Edge, 2.f));
	}

	/** Wooden plank behind titles. */
	inline void StyleBanner(UBorder* Border)
	{
		ApplyBrush(Border, Rounded(Wood(), 16.f, WoodDark(), 4.f));
		Border->SetPadding(FMargin(36.f, 6.f, 36.f, 10.f));
	}

	inline void StyleBackdrop(UBorder* Border, float Alpha)
	{
		Border->SetBrush(FSlateBrush());
		Border->SetBrushColor(Backdrop(Alpha));
	}

	// ---- Text ----
	/** Bold warm-brown text, no drop shadow (it sits on paper). */
	inline void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Font.OutlineSettings.OutlineSize = 0;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowColorAndOpacity(FLinearColor::Transparent);
	}

	/** Big cream letters with a thick brown outline, like a sign painted on the banner. */
	inline void StyleTitle(UTextBlock* Text, int32 Size, const FLinearColor& Fill = TitleCream())
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		Font.OutlineSettings.OutlineSize = FMath::Max(2, Size / 14);
		Font.OutlineSettings.OutlineColor = WoodDark();
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Fill));
		Text->SetShadowOffset(FVector2D(0.f, 3.f));
		Text->SetShadowColorAndOpacity(Hex(0x3A, 0x22, 0x0E, 0.55f));
	}

	/** Text over the 3D game with no panel behind it: cream with a brown outline so it reads anywhere. */
	inline void StyleOverlayText(UTextBlock* Text, int32 Size, const FLinearColor& Fill = TitleCream())
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = WoodDark();
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Fill));
		Text->SetShadowOffset(FVector2D(0.f, 2.f));
		Text->SetShadowColorAndOpacity(Hex(0x2B, 0x1A, 0x0C, 0.5f));
	}

	// ---- Buttons ----
	inline FButtonStyle ButtonStyle(const FLinearColor& Fill)
	{
		const FLinearColor Hover = FMath::Lerp(Fill, FLinearColor::White, 0.25f);
		FLinearColor Press = Fill * 0.82f;
		Press.A = Fill.A;
		FLinearColor Off = FMath::Lerp(Fill, Hex(0xD8, 0xD0, 0xC0), 0.6f);
		Off.A = 0.7f;

		FButtonStyle Style;
		Style.SetNormal(Rounded(Fill, -1.f, ButtonEdge(), 3.f));
		Style.SetHovered(Rounded(Hover, -1.f, WoodDark(), 3.f));
		Style.SetPressed(Rounded(Press, -1.f, WoodDark(), 3.f));
		Style.SetDisabled(Rounded(Off, -1.f, ButtonEdge() * FLinearColor(1.f, 1.f, 1.f, 0.5f), 3.f));
		Style.SetNormalPadding(FMargin(28.f, 10.f, 28.f, 12.f));
		Style.SetPressedPadding(FMargin(28.f, 12.f, 28.f, 10.f));
		Style.SetNormalForeground(FSlateColor(Ink()));
		Style.SetHoveredForeground(FSlateColor(Ink()));
		Style.SetPressedForeground(FSlateColor(Ink()));
		Style.SetDisabledForeground(FSlateColor(InkMuted()));
		return Style;
	}

	inline void StyleButton(UButton* Button, const FLinearColor& Fill = ButtonCream())
	{
		Button->SetStyle(ButtonStyle(Fill));
		Button->SetBackgroundColor(FLinearColor::White);
		Button->SetColorAndOpacity(FLinearColor::White);
	}
}
