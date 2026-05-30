// Copyright Lina Hal Hasnawi. Licensed under MIT.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAIK_LandscapeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FAIK_LandscapeModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FAIK_LandscapeModule>("AIK_Landscape");
    }

    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("AIK_Landscape");
    }

private:
    // Registers this module's tool surface with the host kit's AgentService.
    // Invoked once at StartupModule; tools become visible to any connected agent.
    void RegisterAgentTools();
    void UnregisterAgentTools();

    // Stable handle returned by AgentService::RegisterTool — used to unregister
    // on shutdown.
    TArray<int32> RegisteredToolHandles;
};
