// Clean! Cut! Action!

#include "UI/LTTWidgetBaker.h"
#include "UI/LTTBriefingScreen.h"
#include "UI/LTTPauseMenu.h"
#include "Interaction/LTTEndScreen.h"
#include "Interaction/LTTCleanupHUD.h"
#include "UI/LTTLeaderboardPanel.h"
#include "UI/LTTVirtualKeyboard.h"
#include "UI/LTTMusicToggle.h"
#include "UI/LTTEndingOverlay.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Editor.h"
#endif

UUserWidget* ULTTWidgetBaker::BuildTempLayout(UObject* WidgetBlueprint, UClass*& OutNative, FString& Error)
{
#if WITH_EDITOR
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP)
	{
		Error = TEXT("ERROR: not a Widget Blueprint");
		return nullptr;
	}

	// The first native class up the chain decides which C++ layout to build.
	UClass* Native = WBP->ParentClass;
	while (Native && !Native->HasAnyClassFlags(CLASS_Native))
	{
		Native = Native->GetSuperClass();
	}
	if (!Native)
	{
		Error = TEXT("ERROR: no native parent class");
		return nullptr;
	}
	OutNative = Native;

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
	else if (ULTTLeaderboardPanel* Board = Cast<ULTTLeaderboardPanel>(Temp))
	{
		Board->BuildDefaultLayout();
	}
	else if (ULTTVirtualKeyboard* Keyboard = Cast<ULTTVirtualKeyboard>(Temp))
	{
		Keyboard->BuildDefaultLayout();
	}
	else if (ULTTMusicToggle* Music = Cast<ULTTMusicToggle>(Temp))
	{
		Music->BuildDefaultLayout();
	}
	else if (ULTTEndingOverlay* Ending = Cast<ULTTEndingOverlay>(Temp))
	{
		Ending->BuildDefaultLayout();
	}
	else
	{
		Error = FString::Printf(TEXT("ERROR: %s has no code layout to bake"), *Native->GetName());
		return nullptr;
	}
	if (!Temp->WidgetTree->RootWidget)
	{
		Error = TEXT("ERROR: the code layout produced no root widget");
		return nullptr;
	}
	return Temp;
#else
	Error = TEXT("ERROR: editor only");
	return nullptr;
#endif
}

FString ULTTWidgetBaker::BakeWidgetIntoWidgetBlueprint(UObject* WidgetBlueprint, FName WidgetName, FName InsertBefore, FName ParentPanel)
{
#if WITH_EDITOR
	if (GEditor && GEditor->PlayWorld)
	{
		return TEXT("ERROR: stop Play-In-Editor first (recompiling widgets while PIE runs crashes the editor when PIE ends)");
	}
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP || !WBP->WidgetTree)
	{
		return TEXT("ERROR: not a Widget Blueprint");
	}
	if (WBP->WidgetTree->FindWidget(WidgetName))
	{
		return FString::Printf(TEXT("%s: already has %s, left as is"), *WBP->GetName(), *WidgetName.ToString());
	}

	// Where it goes in the Blueprint.
	UPanelWidget* Target = nullptr;
	int32 Index = INDEX_NONE;
	if (!InsertBefore.IsNone())
	{
		UWidget* Before = WBP->WidgetTree->FindWidget(InsertBefore);
		Target = Before ? Before->GetParent() : nullptr;
		Index = Target ? Target->GetChildIndex(Before) : INDEX_NONE;
	}
	else
	{
		Target = Cast<UPanelWidget>(WBP->WidgetTree->FindWidget(ParentPanel));
	}
	if (!Target)
	{
		return FString::Printf(TEXT("ERROR: %s has no %s to insert next to"), *WBP->GetName(), *(InsertBefore.IsNone() ? ParentPanel : InsertBefore).ToString());
	}

	UClass* Native = nullptr;
	FString Error;
	UUserWidget* Temp = BuildTempLayout(WBP, Native, Error);
	if (!Temp)
	{
		return Error;
	}
	UWidget* Source = Temp->WidgetTree->FindWidget(WidgetName);
	if (!Source)
	{
		Temp->MarkAsGarbage();
		return FString::Printf(TEXT("ERROR: the code layout has no %s"), *WidgetName.ToString());
	}

	// The subtree: the widget and everything under it.
	TArray<UWidget*> Subtree;
	Subtree.Add(Source);
	UWidgetTree::GetChildWidgets(Source, Subtree);

	WBP->Modify();
	WBP->WidgetTree->Modify();
	Target->Modify();
	Source->RemoveFromParent();

	// Move each widget into the Blueprint's tree (slots are outered to their panel and travel with it).
	// Names that clash with an existing widget get a fresh one; bound names are unique to the new subtree.
	TArray<FString> Bound;
	for (UWidget* Widget : Subtree)
	{
		FName NewName = Widget->GetFName();
		if (StaticFindObjectFast(nullptr, WBP->WidgetTree, NewName) || WBP->WidgetTree->FindWidget(NewName))
		{
			NewName = MakeUniqueObjectName(WBP->WidgetTree, Widget->GetClass(), Widget->GetClass()->GetFName());
		}
		Widget->Rename(*NewName.ToString(), WBP->WidgetTree, REN_DontCreateRedirectors | REN_NonTransactional);
		TArray<UObject*> Inner;
		GetObjectsWithOuter(Widget, Inner, EGetObjectsFlags::IncludeNestedObjects);
		Inner.Add(Widget);
		for (UObject* Obj : Inner)
		{
			Obj->ClearFlags(RF_Transient);
			Obj->SetFlags(RF_Transactional);
		}
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			if (Text->GetText().IsEmpty())
			{
				Text->SetText(FText::FromString(Text->GetName()));
			}
		}
		const bool bBound = FindFProperty<FObjectPropertyBase>(Native, Widget->GetFName()) != nullptr;
		Widget->bIsVariable = bBound;
		if (bBound)
		{
			Bound.Add(Widget->GetName());
		}
		WBP->OnVariableAdded(Widget->GetFName());
	}

	if (Index != INDEX_NONE)
	{
		Target->InsertChildAt(Index, Source);
	}
	else
	{
		Target->AddChild(Source);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);
	WBP->MarkPackageDirty();
	Temp->MarkAsGarbage();

	return FString::Printf(TEXT("%s: added %s (%d widgets) to %s, bound: %s"), *WBP->GetName(), *WidgetName.ToString(), Subtree.Num(),
		*Target->GetName(), *FString::Join(Bound, TEXT(", ")));
#else
	return TEXT("ERROR: editor only");
#endif
}

FString ULTTWidgetBaker::RemoveWidgetFromWidgetBlueprint(UObject* WidgetBlueprint, FName WidgetName)
{
#if WITH_EDITOR
	if (GEditor && GEditor->PlayWorld)
	{
		return TEXT("ERROR: stop Play-In-Editor first (recompiling widgets while PIE runs crashes the editor when PIE ends)");
	}
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP || !WBP->WidgetTree)
	{
		return TEXT("ERROR: not a Widget Blueprint");
	}
	UWidget* Widget = WBP->WidgetTree->FindWidget(WidgetName);
	if (!Widget)
	{
		return FString::Printf(TEXT("%s: has no %s, nothing removed"), *WBP->GetName(), *WidgetName.ToString());
	}
	TArray<UWidget*> Subtree;
	Subtree.Add(Widget);
	UWidgetTree::GetChildWidgets(Widget, Subtree);

	WBP->Modify();
	WBP->WidgetTree->Modify();
	if (!WBP->WidgetTree->RemoveWidget(Widget))
	{
		return FString::Printf(TEXT("ERROR: %s couldn't remove %s"), *WBP->GetName(), *WidgetName.ToString());
	}
	for (UWidget* W : Subtree)
	{
		const FName OldName = W->GetFName();
		WBP->OnVariableRemoved(OldName); // drop its variable GUID, or the compiler ensures on the stale entry
		W->Rename(*MakeUniqueObjectName(GetTransientPackage(), W->GetClass(), *FString::Printf(TEXT("Removed_%s"), *OldName.ToString())).ToString(),
			GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		W->ClearFlags(RF_Transactional | RF_Public | RF_Standalone);
		W->SetFlags(RF_Transient);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);
	WBP->MarkPackageDirty();
	return FString::Printf(TEXT("%s: removed %s (%d widgets)"), *WBP->GetName(), *WidgetName.ToString(), Subtree.Num());
#else
	return TEXT("ERROR: editor only");
#endif
}

FString ULTTWidgetBaker::BakeLayoutIntoWidgetBlueprint(UObject* WidgetBlueprint)
{
#if WITH_EDITOR
	if (GEditor && GEditor->PlayWorld)
	{
		return TEXT("ERROR: stop Play-In-Editor first (recompiling widgets while PIE runs crashes the editor when PIE ends)");
	}
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP)
	{
		return TEXT("ERROR: not a Widget Blueprint");
	}

	UClass* Native = nullptr;
	FString Error;
	UUserWidget* Temp = BuildTempLayout(WBP, Native, Error);
	if (!Temp)
	{
		return Error;
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
