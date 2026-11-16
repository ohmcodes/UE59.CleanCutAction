// Copyright © 2026 QLE OPC. All Rights Reserved.

#include "VR/LTTVR.h"
#include "VR/LTTVRComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "IXRTrackingSystem.h"
#include "StereoRendering.h"

namespace LTTVR
{
	bool IsActive()
	{
		return GEngine
			&& GEngine->XRSystem.IsValid()
			&& GEngine->XRSystem->IsHeadTrackingAllowed()
			&& GEngine->StereoRenderingDevice.IsValid()
			&& GEngine->StereoRenderingDevice->IsStereoEnabled();
	}

	ULTTVRComponent* FindActive(const AActor* PawnOrController)
	{
		const AActor* Pawn = PawnOrController;
		if (const AController* Controller = Cast<AController>(PawnOrController))
		{
			Pawn = Controller->GetPawn();
		}
		ULTTVRComponent* VR = Pawn ? Pawn->FindComponentByClass<ULTTVRComponent>() : nullptr;
		return VR && VR->IsVRActive() ? VR : nullptr;
	}

	void GetAimViewPoint(const AController* Controller, FVector& OutLocation, FRotator& OutRotation)
	{
		if (!Controller)
		{
			OutLocation = FVector::ZeroVector;
			OutRotation = FRotator::ZeroRotator;
			return;
		}
		if (const ULTTVRComponent* VR = FindActive(Controller))
		{
			if (VR->GetAimRay(OutLocation, OutRotation))
			{
				return;
			}
		}
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	}

	bool IsOnScreen(const UUserWidget* Widget)
	{
		return Widget && (Widget->IsInViewport() || Widget->GetParent() != nullptr);
	}

	bool IsPointingAtUI(const AActor* PawnOrController)
	{
		const ULTTVRComponent* VR = FindActive(PawnOrController);
		return VR && VR->IsPointingAtInteractiveUI();
	}
}
