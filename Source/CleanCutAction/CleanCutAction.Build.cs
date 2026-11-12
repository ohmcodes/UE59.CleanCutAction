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
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CleanCutAction",
			"CleanCutAction/Variant_Horror",
			"CleanCutAction/Variant_Horror/UI",
			"CleanCutAction/Variant_Shooter",
			"CleanCutAction/Variant_Shooter/AI",
			"CleanCutAction/Variant_Shooter/UI",
			"CleanCutAction/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
