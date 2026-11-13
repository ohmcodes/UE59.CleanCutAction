// Clean! Cut! Action!

#include "UI/LTTWidgetBaker.h"
#include "UI/LTTBriefingScreen.h"
#include "UI/LTTPauseMenu.h"
#include "Interaction/LTTEndScreen.h"
#include "Interaction/LTTCleanupHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

FString ULTTWidgetBaker::BakeLayoutIntoWidgetBlueprint(UObject* WidgetBlueprint)
{
#if WITH_EDITOR
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP)
	{
		return TEXT("ERROR: not a Widget Blueprint");
	}

	// The first native class up the chain decides which C++ layout to build.
	UClass* Native = WBP->ParentClass;
	while (Native && !Native->HasAnyClassFlags(CLASS_Native))
	{
		Native = Native->GetSuperClass();
	}
	if (!Native)
	{
		return TEXT("ERROR: no native parent class");
	}

	// Temporary instance of the C++ class with its own tree; run the code layout on it.
	UUserWidget* Temp = NewObject<UUserWidget>(GetTransientPackage(), Native, NAME_None, RF_Transient);
	Temp->WidgetTree = NewObject<UWidgetTree>(Temp, TEXT("WidgetTree"), RF_Transient);
	if (ULTTBriefingScreen* Briefing = Cast<ULTTBriefingScreen>(Temp))
	{
		Briefing->BuildDefaultLayout();
		Briefing->ApplyTexts();
	}
	else if (ULTTPauseMenu* Pause = Cast<ULTTPauseMenu>(Temp))
	{
		Pause->BuildDefaultLayout();
		Pause->RefreshMusicLabel();
	}
	else if (ULTTEndScreen* End = Cast<ULTTEndScreen>(Temp))
	{
		End->BuildDefaultLayout();
		End->ApplyTexts();
	}
	else if (ULTTCleanupHUD* HUD = Cast<ULTTCleanupHUD>(Temp))
	{
		HUD->BuildDefaultLayout();
	}
	else
	{
		return FString::Printf(TEXT("ERROR: %s has no code layout to bake"), *Native->GetName());
	}
	if (!Temp->WidgetTree->RootWidget)
	{
		return TEXT("ERROR: the code layout produced no root widget");
	}

	// Placeholder text for anything the game fills in at runtime, so the designer isn't blank.
	Temp->WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			if (Text->GetText().IsEmpty())
			{
				Text->SetText(FText::FromString(Text->GetName()));
			}
		}
	});

	// Swap the Blueprint's tree for a duplicate of the code-built one.
	WBP->Modify();
	if (UWidgetTree* OldTree = WBP->WidgetTree)
	{
		OldTree->Rename(*MakeUniqueObjectName(GetTransientPackage(), UWidgetTree::StaticClass(), TEXT("ReplacedWidgetTree")).ToString(),
			GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}
	UWidgetTree* NewTree = DuplicateObject<UWidgetTree>(Temp->WidgetTree, WBP, TEXT("WidgetTree"));
	NewTree->ClearFlags(RF_Transient);
	NewTree->SetFlags(RF_Transactional);
	TArray<UObject*> Inner;
	GetObjectsWithOuter(NewTree, Inner, EGetObjectsFlags::IncludeNestedObjects);
	for (UObject* Obj : Inner)
	{
		Obj->ClearFlags(RF_Transient);
		Obj->SetFlags(RF_Transactional);
	}
	WBP->WidgetTree = NewTree;

	// Widgets a C++ property binds to become variables; the rest stay plain (keeps My Blueprint tidy).
	int32 Total = 0;
	TArray<FString> Bound;
	NewTree->ForEachWidget([&](UWidget* Widget)
	{
		++Total;
		const bool bBound = FindFProperty<FObjectPropertyBase>(Native, Widget->GetFName()) != nullptr;
		Widget->bIsVariable = bBound;
		if (bBound)
		{
			Bound.Add(Widget->GetName());
		}
		WBP->OnVariableAdded(Widget->GetFName());
	});

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);
	WBP->MarkPackageDirty();
	Temp->MarkAsGarbage();

	return FString::Printf(TEXT("%s: %d widgets, bound: %s"), *WBP->GetName(), Total, *FString::Join(Bound, TEXT(", ")));
#else
	return TEXT("ERROR: editor only");
#endif
}
