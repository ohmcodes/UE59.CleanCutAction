// Clean! Cut! Action! - Interaction contract (see Docs/02_Architecture_and_Systems.md §5)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LTTInteractable.generated.h"

/** How the interaction component should treat an interactable. */
UENUM(BlueprintType)
enum class ELTTInteractKind : uint8
{
	/** Single press runs OnInteract. */
	Press,
	/** Hold the button for GetHoldDuration seconds. */
	Hold,
	/** Takes over input (Phase 2 effect stations). */
	Station,
	/** Picked up and carried by ULTTCarryComponent. */
	Pickup
};

UINTERFACE(MinimalAPI, BlueprintType)
class ULTTInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Anything the player can focus and use. */
class CLEANCUTACTION_API ILTTInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* InstigatorActor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetPromptText(AActor* InstigatorActor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	ELTTInteractKind GetInteractKind() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	float GetHoldDuration() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnFocusBegin(AActor* InstigatorActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnFocusEnd(AActor* InstigatorActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(AActor* InstigatorActor);
};
