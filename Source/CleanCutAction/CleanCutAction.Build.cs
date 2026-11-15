// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CleanCutAction : ModuleRules
{
	public CleanCutAction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"AssetRegistry",
			"GeometryCollectionEngine",
			"Chaos",
			"LevelSequence",
			"MovieScene",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP", "Json" });

		// Editor-only: ULTTWidgetBaker copies the C++ widget layouts into WBP_ children.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor", "Kismet" });
		}

		// NVIDIA DLSS / Frame Generation / Reflex (Plugins/DLSS, Plugins/Streamline*): Windows only.
		// UI/LTTGraphicsSettings.cpp wraps every call in WITH_LTT_DLSS.
		bool bHasNgxImportLibs = Directory.Exists(Path.Combine(ModuleDirectory, "../../Plugins/DLSS/Source/ThirdParty/NGX/Lib"));

		if (Target.Platform == UnrealTargetPlatform.Win64 && bHasNgxImportLibs)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"DLSSBlueprint",
				"StreamlineBlueprint",
				"StreamlineDLSSGBlueprint",
				"StreamlineReflexBlueprint"
			});
			PrivateDefinitions.Add("WITH_LTT_DLSS=1");
		}
		else
		{
			PrivateDefinitions.Add("WITH_LTT_DLSS=0");
		}

		PublicIncludePaths.AddRange(new string[] {
			"CleanCutAction"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
