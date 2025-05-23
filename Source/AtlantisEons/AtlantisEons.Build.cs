// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class AtlantisEons : ModuleRules
{
	public AtlantisEons(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "AtlantisEons.h";
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "EnhancedInput", "UMG", "NavigationSystem", "Slate", "SlateCore", "Niagara" });

		// Ensure UI modules are included
		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
		PublicIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Engine/Public"));  
	}
}
