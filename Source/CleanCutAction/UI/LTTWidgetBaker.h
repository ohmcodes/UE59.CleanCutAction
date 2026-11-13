// Clean! Cut! Action! - Editor tool: copies a C++ widget's code-built layout (BuildDefaultLayout) into a
// Widget Blueprint child, so the layout can be edited in the UMG designer. The widgets keep the names the
// C++ BindWidgetOptional properties expect, so the Blueprint binds to the same code.
// Does nothing in a packaged game.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LTTWidgetBaker.generated.h"

UCLASS()
class CLEANCUTACTION_API ULTTWidgetBaker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces WidgetBlueprint's widget tree with the layout its C++ parent class builds in code
	 * (ULTTBriefingScreen, ULTTPauseMenu, ULTTEndScreen or ULTTCleanupHUD), then compiles it.
	 * Widgets bound by a C++ property are marked as variables. Empty texts get a placeholder so the
	 * designer shows something. Returns a short report, or an error string starting with "ERROR".
	 */
	UFUNCTION(BlueprintCallable, Category = "LTT|Editor", meta = (DevelopmentOnly))
	static FString BakeLayoutIntoWidgetBlueprint(UObject* WidgetBlueprint);
};
