// Copyright Lina Hal Hasnawi. Licensed under MIT.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AIK_LandscapeTypes.h"
#include "AIK_LandscapeSubsystem.generated.h"

class ALandscape;
class UTexture2D;

/**
 * Editor-side subsystem exposing landscape operations to AI agents.
 *
 * Architectural rules this subsystem enforces at the boundary:
 *
 *   1. All region inputs are landscape grid coords {x1, y1, x2, y2}, never
 *      world cm. Positional-array region inputs from the host kit are
 *      explicitly rejected (see the descriptor in AgentTools.json).
 *
 *   2. Every public method returns FAIK_LandscapeResult — never throws,
 *      never silently no-ops. Agents branch on bSuccess, not on text parsing.
 *
 *   3. Mutating ops (Sculpt, Paint) wrap a single transaction so undo works
 *      from the editor as if the operation came from a human.
 *
 *   4. No method here knows about ACP, MCP, or any specific agent protocol.
 *      The host kit's AgentService is the only translation layer.
 */
UCLASS()
class AIK_LANDSCAPE_API UAIK_LandscapeSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Set the height of every grid sample in Region to AbsoluteHeight (in cm).
     * Replaces existing data — does not add. Wrapped in a single transaction.
     */
    UFUNCTION(BlueprintCallable, Category = "AIK|Landscape",
              meta = (DisplayName = "Sculpt Region To Height"))
    FAIK_LandscapeResult SculptRegionToHeight(
        ALandscape* Landscape,
        const FAIK_LandscapeRegion& Region,
        float AbsoluteHeightCm);

    /**
     * Add DeltaHeight (in cm) to every grid sample in Region. Saturating —
     * clamps to landscape's signed 16-bit height range.
     */
    UFUNCTION(BlueprintCallable, Category = "AIK|Landscape",
              meta = (DisplayName = "Sculpt Region By Delta"))
    FAIK_LandscapeResult SculptRegionByDelta(
        ALandscape* Landscape,
        const FAIK_LandscapeRegion& Region,
        float DeltaHeightCm);

    /**
     * Apply a grayscale texture as heightmap data over Region. Texture is
     * sampled at the region's resolution; values are mapped from [0,1] to
     * [MinHeight, MaxHeight] in cm.
     */
    UFUNCTION(BlueprintCallable, Category = "AIK|Landscape",
              meta = (DisplayName = "Paint Heightmap From Texture"))
    FAIK_LandscapeResult PaintHeightmapFromTexture(
        ALandscape* Landscape,
        const FAIK_LandscapeRegion& Region,
        UTexture2D* HeightmapTexture,
        float MinHeightCm,
        float MaxHeightCm);

    /**
     * Find the highest Z point in Region. Read-only — always safe to invoke.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AIK|Landscape",
              meta = (DisplayName = "Query Peak In Region"))
    FAIK_LandscapeResult QueryPeakInRegion(
        ALandscape* Landscape,
        const FAIK_LandscapeRegion& Region,
        FAIK_LandscapePeak& OutPeak);

    /**
     * Return the surface normal at a world-space point. Convenience wrapper
     * over a downward line trace. Read-only.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AIK|Landscape",
              meta = (DisplayName = "Query Normal At Point"))
    FAIK_LandscapeResult QueryNormalAtPoint(
        ALandscape* Landscape,
        const FVector& WorldLocation,
        FVector& OutNormal);

private:
    // Resolve the editing interface for a landscape; returns nullptr if
    // the landscape isn't initialized or isn't editable in the current PIE state.
    class FLandscapeEditDataInterface* AcquireEditInterface(ALandscape* Landscape) const;

    // Validate region against the landscape's component grid resolution.
    bool RegionFitsLandscape(ALandscape* Landscape, const FAIK_LandscapeRegion& Region) const;
};
