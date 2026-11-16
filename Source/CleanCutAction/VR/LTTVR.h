// Copyright © 2026 QLE OPC. All Rights Reserved.
// Clean! Cut! Action! - VR helpers shared by gameplay code (aim ray, screen-widget checks, VR on/off).

#pragma once

#include "CoreMinimal.h"

class AActor;
class AController;
class UUserWidget;
class ULTTVRComponent;

namespace LTTVR
{
	/** A headset is connected and the game renders in stereo. */
	CLEANCUTACTION_API bool IsActive();

	/** The VR component of the pawn (or of the controller's pawn), if it has switched to VR. */
	CLEANCUTACTION_API ULTTVRComponent* FindActive(const AActor* PawnOrController);

	/** Where the player aims from: the right hand's laser in VR, the camera otherwise. */
	CLEANCUTACTION_API void GetAimViewPoint(const AController* Controller, FVector& OutLocation, FRotator& OutRotation);

	/** On screen: in the viewport, or moved onto the VR screen (where IsInViewport() is false). */
	CLEANCUTACTION_API bool IsOnScreen(const UUserWidget* Widget);

	/** The player's laser is over a button on the VR screen, so the trigger clicks it instead of interacting. */
	CLEANCUTACTION_API bool IsPointingAtUI(const AActor* PawnOrController);
}
