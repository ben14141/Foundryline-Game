// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Foundryline : ModuleRules
{
	public Foundryline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Foliage" });
		if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[] { "MeshDescription", "StaticMeshDescription", "AssetRegistry", "UnrealEd" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" }); // Runtime ERP typography and measured table text.
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "GeometryAlgorithms", "ProceduralMeshComponent" });
		PrivateDependencyModuleNames.Add("RenderCore"); // Canvas brand geometry uses the engine's white texture.

		// Domain APIs live in Public/<System>; implementations in Private/<System>.
		// Add UI/online dependencies only when those integrations actually exist.
	}
}
