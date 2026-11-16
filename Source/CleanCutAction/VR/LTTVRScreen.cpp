// Copyright © 2026 QLE OPC. All Rights Reserved.

#include "VR/LTTVRScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

void ULTTVRScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Layers"));
		WidgetTree->RootWidget = Layers;
	}
	else if (WidgetTree)
	{
		Layers = Cast<UOverlay>(WidgetTree->RootWidget);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void ULTTVRScreen::AddLayer(UUserWidget* Widget, int32 ZOrder)
{
	if (!Widget || !Layers)
	{
		return;
	}
	// Overlay children paint in order: insert after every layer with a Z order at or below this one.
	int32 Index = 0;
	const int32 Count = Layers->GetChildrenCount();
	for (; Index < Count; ++Index)
	{
		const int32* Existing = LayerZOrders.Find(Layers->GetChildAt(Index));
		if (Existing && *Existing > ZOrder)
		{
			break;
		}
	}
	LayerZOrders.Add(Widget, ZOrder);
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Layers->InsertChildAt(Index, Widget)))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	// Forget layers that were removed since.
	for (auto It = LayerZOrders.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid() || It->Key->GetParent() != Layers)
		{
			It.RemoveCurrent();
		}
	}
}

bool ULTTVRScreen::HasVisibleLayer() const
{
	if (!Layers)
	{
		return false;
	}
	for (int32 i = 0; i < Layers->GetChildrenCount(); ++i)
	{
		if (const UWidget* Child = Layers->GetChildAt(i))
		{
			if (Child->IsVisible())
			{
				return true;
			}
		}
	}
	return false;
}
