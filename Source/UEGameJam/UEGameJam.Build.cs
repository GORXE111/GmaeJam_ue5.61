// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UEGameJam : ModuleRules
{
	public UEGameJam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Niagara",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Editor-only deps（自定义 MaterialExpression 的 Compile 用）
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"MaterialEditor",
				"UnrealEd"
			});
		}

		PublicIncludePaths.AddRange(new string[] {
			"UEGameJam",
			"UEGameJam/Realm",
			"UEGameJam/Enemy",
			"UEGameJam/Enemy/StateTree",
			"UEGameJam/Enemy/Melee",
			"UEGameJam/Enemy/Pistol",
			"UEGameJam/Enemy/MachineGun",
			"UEGameJam/Enemy/Ghost",
			"UEGameJam/Variant_Horror",
			"UEGameJam/Variant_Horror/UI",
			"UEGameJam/Variant_Shooter",
			"UEGameJam/Variant_Shooter/AI",
			"UEGameJam/Variant_Shooter/UI",
			"UEGameJam/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
