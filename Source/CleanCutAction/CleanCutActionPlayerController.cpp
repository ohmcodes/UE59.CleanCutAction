// Copyright Epic Games, Inc. All Rights Reserved.


#include "CleanCutActionPlayerController.h"
#include "VR/LTTVR.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "UI/LTTPauseMenu.h"
#include "UI/LTTMusicToggle.h"
#include "UI/LTTUITheme.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "CleanCutActionCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "CleanCutAction.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "VR/LTTVRComponent.h"

ACleanCutActionPlayerController::ACleanCutActionPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ACleanCutActionCameraManager::StaticClass();
	PauseMenuClass = ULTTPauseMenu::StaticClass();
	MusicToggleClass = ULTTMusicToggle::StaticClass();
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

		// Music on/off next to the touch pause button (the pause menu has one too, but it's two taps away).
		if (MusicToggleClass)
		{
			MusicToggleWidget = CreateWidget<UUserWidget>(this, LTTTheme::PreferWBP<ULTTMusicToggle>(MusicToggleClass, TEXT("WBP_MusicToggle")));
			if (MusicToggleWidget)
			{
				MusicToggleWidget->AddToPlayerScreen(1);
			}
		}

	}

	// Keyboard / gamepad / touch prompts follow the device the player last used.
	if (IsLocalPlayerController())
	{
		if (ULTTInputDeviceSubsystem* Devices = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULTTInputDeviceSubsystem>() : nullptr)
		{
			Devices->OnInputDeviceChanged.AddUniqueDynamic(this, &ACleanCutActionPlayerController::HandleInputDeviceChanged);
		}
		// Also on a timer: the interact prompt and touch overlay are made by Blueprint after this runs.
		GetWorldTimerManager().SetTimer(InputDeviceTimer, this, &ACleanCutActionPlayerController::ApplyInputDevice, 0.25f, true, 0.f);
	}
}

bool ACleanCutActionPlayerController::IsShowingTouchControls() const
{
	return ShouldUseTouchControls() && !ULTTInputDeviceSubsystem::IsGamepad();
}

void ACleanCutActionPlayerController::HandleInputDeviceChanged(ELTTInputDevice NewDevice)
{
	ApplyInputDevice();
}

namespace
{
	void LTTSetVisibilityIfChanged(UWidget* Widget, ESlateVisibility Visibility)
	{
		if (Widget && Widget->GetVisibility() != Visibility)
		{
			Widget->SetVisibility(Visibility);
		}
	}

	void LTTSetTextIfChanged(UTextBlock* Text, const FText& Value)
	{
		if (Text && !Text->GetText().EqualTo(Value))
		{
			Text->SetText(Value);
		}
	}
}

void ACleanCutActionPlayerController::ApplyInputDevice()
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	const bool bPad = ULTTInputDeviceSubsystem::IsGamepad();
	const bool bTouchUI = IsShowingTouchControls();

	// Touch overlay: hidden while a gamepad drives the game on a touch device (Steam Deck browser, tablet + controller).
	if (ShouldUseTouchControls())
	{
		const ESlateVisibility TouchVis = bTouchUI ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
		const bool bChanged = MobileControlsWidget && MobileControlsWidget->GetVisibility() != TouchVis;
		LTTSetVisibilityIfChanged(MobileControlsWidget, TouchVis);
		LTTSetVisibilityIfChanged(MusicToggleWidget, TouchVis);
		if (bChanged && !IsPaused() && !PauseMenuWidget)
		{
			SetShowMouseCursor(bTouchUI);
		}
	}

	// WBP_InteractPrompt (made by BP_FirstPersonPlayerController): "E / mouse" or "X", and "RT" for throw.
	static const FSoftClassPath PromptClassPath(TEXT("/Game/_CleanCutAction/Blueprints/Widgets/WBP_InteractPrompt.WBP_InteractPrompt_C"));
	UClass* PromptClass = PromptClassPath.ResolveClass();
	if (!PromptClass)
	{
		return;
	}
	TArray<UUserWidget*> Prompts;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Prompts, PromptClass, false);
	for (UUserWidget* Prompt : Prompts)
	{
		if (!Prompt || Prompt->GetOwningPlayer() != this)
		{
			continue;
		}
		// The prompt hides itself in touch mode (UI_TouchSimple sets the flag); a gamepad on a touch device needs it back.
		if (FBoolProperty* TouchFlag = FindFProperty<FBoolProperty>(Prompt->GetClass(), TEXT("bTouchMode")))
		{
			TouchFlag->SetPropertyValue_InContainer(Prompt, bTouchUI);
		}
		// VR: Touch controller labels (IMC_VR: trigger picks up / places, B throws).
		const bool bVR = ULTTVRComponent::IsHeadsetActive();
		const bool bHands = ULTTVRComponent::IsUsingHands(); // hand tracking: pinch picks up, a flick of the hand throws
		const bool bPadCaps = bPad || bVR;
		const ESlateVisibility MouseVis = bPadCaps ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
		LTTSetTextIfChanged(Cast<UTextBlock>(Prompt->GetWidgetFromName(TEXT("KeyText"))),
			bHands ? FText::FromString(TEXT("Pinch")) : bVR ? FText::FromString(TEXT("Trigger"))
				: bPad ? ULTTInputDeviceSubsystem::GamepadKeyLabel(EKeys::Gamepad_FaceButton_Left) : FText::FromString(TEXT("E")));
		LTTSetVisibilityIfChanged(Prompt->GetWidgetFromName(TEXT("OrText")), MouseVis);
		LTTSetVisibilityIfChanged(Prompt->GetWidgetFromName(TEXT("MouseLeftIcon")), MouseVis);
		LTTSetVisibilityIfChanged(Prompt->GetWidgetFromName(TEXT("MouseRightIcon")), MouseVis);

		// Throw row: an "RT" key cap in the same style as the E cap, made here if the WBP doesn't have one.
		UWidget* ThrowCap = Prompt->GetWidgetFromName(TEXT("ThrowPadCap"));
		UHorizontalBox* RowThrow = Cast<UHorizontalBox>(Prompt->GetWidgetFromName(TEXT("RowThrow")));
		if (!ThrowCap && bPadCaps && RowThrow && Prompt->WidgetTree)
		{
			UBorder* NewCap = Prompt->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ThrowPadCap"));
			UTextBlock* CapText = Prompt->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ThrowPadText"));
			if (const UBorder* ECap = Cast<UBorder>(Prompt->GetWidgetFromName(TEXT("KeyCapE"))))
			{
				NewCap->SetBrush(ECap->Background);
				NewCap->SetPadding(ECap->GetPadding());
				NewCap->SetHorizontalAlignment(ECap->GetHorizontalAlignment());
				NewCap->SetVerticalAlignment(ECap->GetVerticalAlignment());
			}
			if (const UTextBlock* EText = Cast<UTextBlock>(Prompt->GetWidgetFromName(TEXT("KeyText"))))
			{
				CapText->SetFont(EText->GetFont());
				CapText->SetColorAndOpacity(EText->GetColorAndOpacity());
			}
			CapText->SetText(ULTTInputDeviceSubsystem::GamepadKeyLabel(EKeys::Gamepad_RightTrigger));
			NewCap->SetContent(CapText);
			UHorizontalBoxSlot* NewSlot = Cast<UHorizontalBoxSlot>(RowThrow->InsertChildAt(0, NewCap));
			const UWidget* MouseRight = Prompt->GetWidgetFromName(TEXT("MouseRightIcon"));
			if (const UHorizontalBoxSlot* MouseSlot = MouseRight ? Cast<UHorizontalBoxSlot>(MouseRight->Slot) : nullptr)
			{
				if (NewSlot)
				{
					NewSlot->SetPadding(MouseSlot->GetPadding());
					NewSlot->SetVerticalAlignment(MouseSlot->GetVerticalAlignment());
				}
			}
			ThrowCap = NewCap;
		}
		LTTSetVisibilityIfChanged(ThrowCap, bPadCaps ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		LTTSetTextIfChanged(Cast<UTextBlock>(Prompt->GetWidgetFromName(TEXT("ThrowPadText"))),
			bHands ? FText::FromString(TEXT("Flick")) : bVR ? FText::FromString(TEXT("B")) : ULTTInputDeviceSubsystem::GamepadKeyLabel(EKeys::Gamepad_RightTrigger));
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

				// VR: the left controller's menu button. IMC_VR (an Enhanced Input default context) maps it; loaded by path (in MapsToCook).
				if (const UInputAction* VRPause = LoadObject<UInputAction>(nullptr, TEXT("/Game/_CleanCutAction/Input/VR/IA_VR_Pause.IA_VR_Pause")))
				{
					EIC->BindAction(VRPause, ETriggerEvent::Started, this, &ACleanCutActionPlayerController::HandlePauseInput);
				}
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
	if (ULTTVRComponent::IsHeadsetActive())
	{
		// VR: the menu is on a world-space panel clicked by the controller laser, which needs game input to keep flowing.
		FInputModeGameAndUI VRMode;
		VRMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(VRMode);
		return;
	}
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
	RestoreGameInput();
}

void ACleanCutActionPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (InPawn && !bHasSpawnTransform)
	{
		SpawnTransform = InPawn->GetActorTransform();
		SpawnControlRotation = FRotator(0.f, InPawn->GetActorRotation().Yaw, 0.f);
		bHasSpawnTransform = true;
	}
}

void ACleanCutActionPlayerController::TeleportToSpawn()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	FVector Location;
	FRotator Rotation;
	if (bHasSpawnTransform)
	{
		Location = SpawnTransform.GetLocation();
		Rotation = SpawnControlRotation;
	}
	else
	{
		// Possessed before we could record it: use the level's player start.
		AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
		AActor* Start = GameMode ? GameMode->FindPlayerStart(this) : nullptr;
		if (!Start)
		{
			return;
		}
		Location = Start->GetActorLocation();
		Rotation = FRotator(0.f, Start->GetActorRotation().Yaw, 0.f);
	}

	if (ACharacter* ActionCharacter = Cast<ACharacter>(MyPawn))
	{
		ActionCharacter->UnCrouch();
		if (UCharacterMovementComponent* Move = ActionCharacter->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->bWantsToCrouch = false;
		}
	}
	// TeleportTo nudges the pawn to a free spot if a prop now sits on the spawn point; force it if there's none.
	if (!MyPawn->TeleportTo(Location, FRotator(0.f, Rotation.Yaw, 0.f)))
	{
		MyPawn->SetActorLocationAndRotation(Location, FRotator(0.f, Rotation.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetControlRotation(Rotation);
}

void ACleanCutActionPlayerController::RestoreGameInput()
{
	if (IsShowingTouchControls())
	{
		// Same mode UI_TouchSimple sets on Construct: GameOnly would stop the touch buttons taking taps.
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	else
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
	}
}

void ACleanCutActionPlayerController::KeepGameInputWithCursor(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	// No widget focus: keyboard and gamepad keep driving the pawn; the mouse clicks the end screen's buttons.
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
}

void ACleanCutActionPlayerController::ResetGameInput(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	UGameplayStatics::SetGamePaused(PC, false);
	PC->ResetIgnoreMoveInput();
	PC->ResetIgnoreLookInput();
	if (ACleanCutActionPlayerController* CCPC = Cast<ACleanCutActionPlayerController>(PC))
	{
		CCPC->RestoreGameInput();
	}
	else
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

bool ACleanCutActionPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	// A Quest is an Android device, but the headset has no touch screen.
	if (LTTVR::IsActive())
	{
		return false;
	}
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
