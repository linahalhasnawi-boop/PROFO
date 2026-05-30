// Copyright Lina Hal Hasnawi. Licensed under MIT.

#include "AIK_LandscapeModule.h"
#include "AIK_LandscapeSubsystem.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"

// The host kit's AgentService tool-registration API. Resolved at runtime to
// keep this module compilable when the closed-source host isn't present.
#if WITH_EDITOR && __has_include("AgentService.h")
    #include "AgentService.h"
    #define AIK_HAS_HOST_KIT 1
#else
    #define AIK_HAS_HOST_KIT 0
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAIKLandscape, Log, All);

void FAIK_LandscapeModule::StartupModule()
{
    UE_LOG(LogAIKLandscape, Log, TEXT("AIK_Landscape: StartupModule"));
    RegisterAgentTools();
}

void FAIK_LandscapeModule::ShutdownModule()
{
    UnregisterAgentTools();
    UE_LOG(LogAIKLandscape, Log, TEXT("AIK_Landscape: ShutdownModule"));
}

void FAIK_LandscapeModule::RegisterAgentTools()
{
#if AIK_HAS_HOST_KIT
    FAgentService* Service = FAgentService::Get();
    if (!Service)
    {
        UE_LOG(LogAIKLandscape, Warning,
            TEXT("AIK_Landscape: AgentService not available; landscape tools "
                 "will not be visible to agents this session."));
        return;
    }

    // Tool surface declared here is the agent-facing contract. The strings
    // must match docs/tool-surface.md. Changes here are an API break.
    RegisteredToolHandles.Add(Service->RegisterTool(
        TEXT("landscape.sculpt_region_to_height"),
        TEXT("Set every grid sample inside a region to an absolute height in cm. "
             "Region uses landscape grid coords (0..resolution), NOT world cm."),
        UAIK_LandscapeSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UAIK_LandscapeSubsystem, SculptRegionToHeight)));

    RegisteredToolHandles.Add(Service->RegisterTool(
        TEXT("landscape.sculpt_region_by_delta"),
        TEXT("Add a delta height in cm to every grid sample inside a region. "
             "Saturating at the landscape's signed 16-bit height range."),
        UAIK_LandscapeSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UAIK_LandscapeSubsystem, SculptRegionByDelta)));

    RegisteredToolHandles.Add(Service->RegisterTool(
        TEXT("landscape.paint_heightmap_from_texture"),
        TEXT("Apply a grayscale texture as heightmap data over a region. "
             "Sample range [0,1] mapped to [MinHeightCm, MaxHeightCm]."),
        UAIK_LandscapeSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UAIK_LandscapeSubsystem, PaintHeightmapFromTexture)));

    RegisteredToolHandles.Add(Service->RegisterTool(
        TEXT("landscape.query_peak_in_region"),
        TEXT("Find the highest Z point inside a region. Read-only."),
        UAIK_LandscapeSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UAIK_LandscapeSubsystem, QueryPeakInRegion)));

    RegisteredToolHandles.Add(Service->RegisterTool(
        TEXT("landscape.query_normal_at_point"),
        TEXT("Return the surface normal at a world-space point on the landscape. "
             "Read-only."),
        UAIK_LandscapeSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UAIK_LandscapeSubsystem, QueryNormalAtPoint)));

    UE_LOG(LogAIKLandscape, Log,
        TEXT("AIK_Landscape: registered %d agent tools."),
        RegisteredToolHandles.Num());
#else
    UE_LOG(LogAIKLandscape, Log,
        TEXT("AIK_Landscape: host kit (AgentIntegrationKit) not present at "
             "compile time; tool registration skipped. Subsystem still usable "
             "from Blueprint and direct C++ callers."));
#endif
}

void FAIK_LandscapeModule::UnregisterAgentTools()
{
#if AIK_HAS_HOST_KIT
    if (FAgentService* Service = FAgentService::Get())
    {
        for (int32 Handle : RegisteredToolHandles)
        {
            Service->UnregisterTool(Handle);
        }
    }
#endif
    RegisteredToolHandles.Reset();
}

IMPLEMENT_MODULE(FAIK_LandscapeModule, AIK_Landscape)
