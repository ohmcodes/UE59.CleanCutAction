// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

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
			"Niagara",
			"HeadMountedDisplay"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP", "Json" });

		// VR (OpenXR): the controller models come from XRBase, which is only enabled where OpenXR is (not Wasm).
		bool bWithXRBase = Target.Platform == UnrealTargetPlatform.Win64
			|| Target.Platform == UnrealTargetPlatform.Android
			|| Target.Platform == UnrealTargetPlatform.Linux;
		if (bWithXRBase)
		{
			PrivateDependencyModuleNames.Add("XRBase");
		}
		PrivateDefinitions.Add("LTT_WITH_XRBASE=" + (bWithXRBase ? "1" : "0"));

		// Editor-only: ULTTWidgetBaker copies the C++ widget layouts into WBP_ children.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor", "Kismet" });
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
