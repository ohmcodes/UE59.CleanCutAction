// Copyright Epic Games, Inc. All Rights Reserved.


#include "CleanCutActionPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "UI/LTTPauseMenu.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "CleanCutActionCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "CleanCutAction.h"
#include "Widgets/Input/SVirtualJoystick.h"

ACleanCutActionPlayerController::ACleanCutActionPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ACleanCutActionCameraManager::StaticClass();
	PauseMenuClass = ULTTPauseMenu::StaticClass();
}

void ACleanCutActionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCleanCutAction, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ACleanCutActionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			// Pause: a runtime action + context for PauseKeys unless an asset action is set.
			const UInputAction* Action = PauseAction;
			if (!Action)
			{
				RuntimePauseAction = NewObject<UInputAction>(this, TEXT("IA_Pause_Runtime"));
				RuntimePauseAction->bTriggerWhenPaused = true;
				RuntimePauseContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Pause_Runtime"));
				for (const FKey& Key : PauseKeys)
				{
					if (Key.IsValid())
					{
						RuntimePauseContext->MapKey(RuntimePauseAction, Key);
					}
				}
				Subsystem->AddMappingContext(RuntimePauseContext, 2);
				Action = RuntimePauseAction;
			}
			if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
			{
				EIC->BindAction(Action, ETriggerEvent::Started, this, &ACleanCutActionPlayerController::HandlePauseInput);
			}
		}
	}
}

void ACleanCutActionPlayerController::HandlePauseInput()
{
	TogglePauseMenu();
}

void ACleanCutActionPlayerController::TogglePauseMenu()
{
	if (PauseMenuWidget)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void ACleanCutActionPlayerController::OpenPauseMenu()
{
	// The end screen (or anything else) already paused the game: leave it alone.
	if (PauseMenuWidget || !PauseMenuClass || !IsLocalPlayerController() || IsPaused())
	{
		return;
	}
	PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuClass);
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->AddToViewport(20);
	UGameplayStatics::SetGamePaused(this, true);
	SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

void ACleanCutActionPlayerController::ClosePauseMenu()
{
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;
	UGameplayStatics::SetGamePaused(this, false);
	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());
}

bool ACleanCutActionPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
