// Clean! Cut! Action!

#include "UI/LTTMusicToggle.h"
#include "UI/LTTUITheme.h"
#include "Audio/LTTMusicPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

void ULTTMusicToggle::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (MusicButton)
	{
		MusicButton->OnClicked.AddDynamic(this, &ULTTMusicToggle::HandleClicked);
	}
	// Full-screen canvas: only the button may take touches, or it would swallow the look stick.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (WidgetTree && WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	RefreshIcon();
}

void ULTTMusicToggle::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Same look as UI_TouchSimple's round buttons: warm cream fill, brown ring, dark brown icon.
	MusicButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MusicButton"));
	const FLinearColor Fill(0.913099f, 0.822786f, 0.651406f, 0.95f);
	const FLinearColor Ring(0.434154f, 0.223228f, 0.064803f, 1.f);
	FButtonStyle Style;
	Style.SetNormal(LTTTheme::Rounded(Fill, -1.f, Ring, 4.f));
	Style.SetHovered(LTTTheme::Rounded(FMath::Lerp(Fill, FLinearColor::White, 0.2f), -1.f, Ring, 4.f));
	FLinearColor Pressed = Fill * 0.8f;
	Pressed.A = Fill.A;
	Style.SetPressed(LTTTheme::Rounded(Pressed, -1.f, Ring, 4.f));
	Style.SetNormalPadding(FMargin(16.f)).SetPressedPadding(FMargin(16.f, 18.f, 16.f, 14.f));
	MusicButton->SetStyle(Style);
	MusicButton->SetBackgroundColor(FLinearColor::White);
	MusicButton->SetToolTipText(NSLOCTEXT("LTTMusicToggle", "Tip", "Music on / off"));

	MusicIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MusicIcon"));
	MusicIcon->SetBrush(LTTTheme::UIIconBrush(TEXT("T_Icon_MusicOn"), 52.f));
	MusicIcon->SetColorAndOpacity(LTTTheme::Ink());
	MusicButton->SetContent(MusicIcon);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MusicSize"));
	Size->SetWidthOverride(ButtonSize);
	Size->SetHeightOverride(ButtonSize);
	Size->SetContent(MusicButton);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Size))
	{
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(-RightOffset, TopOffset));
		S->SetAutoSize(true);
	}
}

void ULTTMusicToggle::RefreshIcon()
{
	const int32 State = ALTTMusicPlayer::IsMusicMuted() ? 1 : 0;
	if (State == ShownState || !MusicIcon)
	{
		return;
	}
	ShownState = State;
	UTexture2D* Tex = State ? MusicOffIcon.Get() : MusicOnIcon.Get();
	if (!Tex)
	{
		Tex = LTTTheme::UIIcon(State ? TEXT("T_Icon_MusicOff") : TEXT("T_Icon_MusicOn"));
	}
	if (Tex)
	{
		MusicIcon->SetBrushResourceObject(Tex);
	}
	// Muted reads as "off": the icon dims a little.
	MusicIcon->SetRenderOpacity(State ? MutedOpacity : 1.f);
}

void ULTTMusicToggle::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshIcon(); // the pause menu can toggle the music too
}

void ULTTMusicToggle::ToggleMusic()
{
	ALTTMusicPlayer::SetMusicMuted(!ALTTMusicPlayer::IsMusicMuted());
	RefreshIcon();
}

void ULTTMusicToggle::HandleClicked()
{
	ToggleMusic();
}
