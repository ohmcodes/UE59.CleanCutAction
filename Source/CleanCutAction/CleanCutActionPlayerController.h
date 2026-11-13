// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "CleanCutActionPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class CLEANCUTACTION_API ACleanCutActionPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ACleanCutActionPlayerController();

	/** Opens the pause menu if it's closed, closes it if it's open. */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void TogglePauseMenu();

	/** Pauses the game and shows the pause menu. Does nothing while something else (the end screen) has the game paused. */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void OpenPauseMenu();

	/** Hides the pause menu, unpauses and gives input back to the game. */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void ClosePauseMenu();

	UFUNCTION(BlueprintPure, Category = "Pause")
	bool IsPauseMenuOpen() const { return PauseMenuWidget != nullptr; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Widget shown on pause (defaults to the C++ ULTTPauseMenu layout). */
	UPROPERTY(EditAnywhere, Category = "Pause")
	TSubclassOf<UUserWidget> PauseMenuClass;

	/** Optional Input Action for pause. Leave empty to use the keys below (a runtime action is made for them). */
	UPROPERTY(EditAnywhere, Category = "Pause")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, Category = "Pause")
	TArray<FKey> PauseKeys = { EKeys::P, EKeys::Escape, EKeys::Gamepad_Special_Right };

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimePauseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimePauseContext;

	void HandlePauseInput();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
