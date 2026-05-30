// Copyright Lina Hal Hasnawi. Licensed under MIT.
// Custom module for the Unreal Claude Agent Kit.
// Exposes landscape sculpt + query operations to AI agents via the kit's
// AgentService tool boundary.

using UnrealBuildTool;

public class AIK_Landscape : ModuleRules
{
    public AIK_Landscape(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Landscape",
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate",
            "SlateCore",
            "EditorSubsystem",
            "UnrealEd",
            "LandscapeEditor",
            "EditorScriptingUtilities",
            "Json",
            "JsonUtilities",
        });

        // The host kit's bridge module — exposes the tool registration API.
        // Falls back gracefully when building standalone (e.g., in CI without
        // the closed-source host).
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("AgentIntegrationKit");
        }
    }
}
