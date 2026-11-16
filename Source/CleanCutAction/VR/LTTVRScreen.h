// Copyright © 2026 QLE OPC. All Rights Reserved.
// Clean! Cut! Action! - The floating VR screen's root widget. Viewport widgets don't show in a headset, so the VR
// component moves every widget added to the viewport (HUDs, menus, end screens) in here, keeping their Z order.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LTTVRScreen.generated.h"

class UOverlay;

UCLASS(NotBlueprintable)
class CLEANCUTACTION_API ULTTVRScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Adds a widget taken off the viewport, above the layers with a lower or equal Z order. */
	void AddLayer(UUserWidget* Widget, int32 ZOrder);

	/** Something visible is on the screen (a HUD or a menu). */
	bool HasVisibleLayer() const;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> Layers;

	TMap<TWeakObjectPtr<UWidget>, int32> LayerZOrders;
};
