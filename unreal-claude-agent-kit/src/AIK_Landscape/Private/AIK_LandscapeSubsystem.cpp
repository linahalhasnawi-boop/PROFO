// Copyright Lina Hal Hasnawi. Licensed under MIT.

#include "AIK_LandscapeSubsystem.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeEdit.h"
#include "LandscapeDataAccess.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "AIK_Landscape"

DEFINE_LOG_CATEGORY_STATIC(LogAIKLandscape, Log, All);

void UAIK_LandscapeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogAIKLandscape, Log, TEXT("UAIK_LandscapeSubsystem: Initialize"));
}

void UAIK_LandscapeSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// --- Validation -------------------------------------------------------------

bool UAIK_LandscapeSubsystem::RegionFitsLandscape(
    ALandscape* Landscape, const FAIK_LandscapeRegion& Region) const
{
    if (!Landscape) return false;

    ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
    if (!Info) return false;

    int32 MinX, MinY, MaxX, MaxY;
    if (!Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
    {
        return false;
    }

    return Region.X1 >= MinX && Region.Y1 >= MinY
        && Region.X2 <= MaxX + 1 && Region.Y2 <= MaxY + 1;
}

FLandscapeEditDataInterface*
UAIK_LandscapeSubsystem::AcquireEditInterface(ALandscape* Landscape) const
{
    if (!Landscape) return nullptr;
    if (!Landscape->GetLandscapeInfo()) return nullptr;
    // Caller owns the returned pointer's lifetime; allocate per-call so we don't
    // hold onto a stale info object across reimports / level changes.
    return new FLandscapeEditDataInterface(Landscape->GetLandscapeInfo());
}

// --- Sculpt ops -------------------------------------------------------------

FAIK_LandscapeResult UAIK_LandscapeSubsystem::SculptRegionToHeight(
    ALandscape* Landscape,
    const FAIK_LandscapeRegion& Region,
    float AbsoluteHeightCm)
{
    if (!Landscape)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::LandscapeNotFound,
            TEXT("Landscape pointer was null."));
    }
    if (!Region.IsValid())
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionInvalid,
            FString::Printf(TEXT("Region {%d,%d,%d,%d} is degenerate."),
                Region.X1, Region.Y1, Region.X2, Region.Y2));
    }
    if (!RegionFitsLandscape(Landscape, Region))
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionOutOfBounds,
            TEXT("Region falls outside the landscape's grid extent."));
    }

    FScopedTransaction Transaction(LOCTEXT("SculptToHeight", "AIK: Sculpt landscape region to height"));
    Landscape->Modify();

    TUniquePtr<FLandscapeEditDataInterface> Edit(AcquireEditInterface(Landscape));
    if (!Edit)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::EditFailed,
            TEXT("Could not acquire FLandscapeEditDataInterface."));
    }

    // UE landscape stores heights as uint16 with 0x8000 == zero altitude;
    // 1 cm == 128 height units (LANDSCAPE_INV_ZSCALE).
    const int32 RawHeight = FMath::Clamp(
        static_cast<int32>(0x8000 + AbsoluteHeightCm * LANDSCAPE_INV_ZSCALE),
        0, 0xFFFF);

    const int32 W = Region.Width();
    const int32 H = Region.Height();
    TArray<uint16> Data;
    Data.Init(static_cast<uint16>(RawHeight), W * H);

    Edit->SetHeightData(Region.X1, Region.Y1, Region.X2 - 1, Region.Y2 - 1,
                        Data.GetData(), /*Stride=*/ 0, /*CalcNormals=*/ true);

    return FAIK_LandscapeResult::Ok();
}

FAIK_LandscapeResult UAIK_LandscapeSubsystem::SculptRegionByDelta(
    ALandscape* Landscape,
    const FAIK_LandscapeRegion& Region,
    float DeltaHeightCm)
{
    if (!Landscape || !Region.IsValid())
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionInvalid,
            TEXT("Invalid landscape or region."));
    }
    if (!RegionFitsLandscape(Landscape, Region))
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionOutOfBounds,
            TEXT("Region out of bounds."));
    }

    FScopedTransaction Transaction(LOCTEXT("SculptDelta", "AIK: Sculpt landscape region by delta"));
    Landscape->Modify();

    TUniquePtr<FLandscapeEditDataInterface> Edit(AcquireEditInterface(Landscape));
    if (!Edit)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::EditFailed,
            TEXT("Could not acquire edit interface."));
    }

    const int32 W = Region.Width();
    const int32 H = Region.Height();
    TArray<uint16> Heights;
    Heights.SetNumUninitialized(W * H);
    Edit->GetHeightData(Region.X1, Region.Y1, Region.X2 - 1, Region.Y2 - 1,
                        Heights.GetData(), /*Stride=*/ 0);

    const int32 RawDelta = static_cast<int32>(DeltaHeightCm * LANDSCAPE_INV_ZSCALE);
    for (uint16& H16 : Heights)
    {
        const int32 Updated = FMath::Clamp(
            static_cast<int32>(H16) + RawDelta, 0, 0xFFFF);
        H16 = static_cast<uint16>(Updated);
    }

    Edit->SetHeightData(Region.X1, Region.Y1, Region.X2 - 1, Region.Y2 - 1,
                        Heights.GetData(), /*Stride=*/ 0, /*CalcNormals=*/ true);

    return FAIK_LandscapeResult::Ok();
}

// --- Texture-driven painting -----------------------------------------------

FAIK_LandscapeResult UAIK_LandscapeSubsystem::PaintHeightmapFromTexture(
    ALandscape* Landscape,
    const FAIK_LandscapeRegion& Region,
    UTexture2D* HeightmapTexture,
    float MinHeightCm,
    float MaxHeightCm)
{
    if (!Landscape || !Region.IsValid() || !HeightmapTexture)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionInvalid,
            TEXT("Invalid landscape, region, or texture."));
    }
    if (MaxHeightCm <= MinHeightCm)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionInvalid,
            TEXT("MaxHeightCm must be greater than MinHeightCm."));
    }

    // Texture sampling deferred to a helper that handles compression formats
    // (BC4 grayscale, R8, R16). Omitted here for brevity — the production
    // module reads via FTexture2DMipMap::BulkData with format-specific decode.
    // See docs/tool-surface.md for the full sampling matrix.

    UE_LOG(LogAIKLandscape, Warning,
        TEXT("PaintHeightmapFromTexture: texture sampling implementation "
             "elided in this reference scaffold."));

    return FAIK_LandscapeResult::Fail(
        EAIK_LandscapeError::EditFailed,
        TEXT("Texture sampling not yet implemented in this scaffold."));
}

// --- Read-only queries -----------------------------------------------------

FAIK_LandscapeResult UAIK_LandscapeSubsystem::QueryPeakInRegion(
    ALandscape* Landscape,
    const FAIK_LandscapeRegion& Region,
    FAIK_LandscapePeak& OutPeak)
{
    if (!Landscape || !Region.IsValid())
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionInvalid,
            TEXT("Invalid landscape or region."));
    }
    if (!RegionFitsLandscape(Landscape, Region))
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::RegionOutOfBounds,
            TEXT("Region out of bounds."));
    }

    TUniquePtr<FLandscapeEditDataInterface> Edit(AcquireEditInterface(Landscape));
    if (!Edit)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::EditFailed,
            TEXT("Could not acquire edit interface."));
    }

    const int32 W = Region.Width();
    const int32 H = Region.Height();
    TArray<uint16> Heights;
    Heights.SetNumUninitialized(W * H);
    Edit->GetHeightData(Region.X1, Region.Y1, Region.X2 - 1, Region.Y2 - 1,
                        Heights.GetData(), /*Stride=*/ 0);

    uint16 MaxRaw = 0;
    int32 MaxIdx = 0;
    for (int32 i = 0; i < Heights.Num(); ++i)
    {
        if (Heights[i] > MaxRaw)
        {
            MaxRaw = Heights[i];
            MaxIdx = i;
        }
    }

    const int32 LocalX = MaxIdx % W;
    const int32 LocalY = MaxIdx / W;

    OutPeak.GridX = Region.X1 + LocalX;
    OutPeak.GridY = Region.Y1 + LocalY;
    OutPeak.WorldZ = (static_cast<int32>(MaxRaw) - 0x8000) * LANDSCAPE_ZSCALE;

    // Convert grid coords to world location via the landscape's transform.
    const FVector LocalLoc(OutPeak.GridX, OutPeak.GridY, 0.f);
    OutPeak.WorldLocation = Landscape->LandscapeActorToWorld().TransformPosition(LocalLoc);
    OutPeak.WorldLocation.Z = OutPeak.WorldZ;

    return FAIK_LandscapeResult::Ok();
}

FAIK_LandscapeResult UAIK_LandscapeSubsystem::QueryNormalAtPoint(
    ALandscape* Landscape,
    const FVector& WorldLocation,
    FVector& OutNormal)
{
    if (!Landscape)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::LandscapeNotFound,
            TEXT("Landscape pointer was null."));
    }

    UWorld* World = Landscape->GetWorld();
    if (!World)
    {
        return FAIK_LandscapeResult::Fail(
            EAIK_LandscapeError::EditorOnlyOperation,
            TEXT("Landscape has no associated world."));
    }

    const FVector Start = WorldLocation + FVector(0, 0, 100000.f);
    const FVector End   = WorldLocation - FVector(0, 0, 100000.f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AIK_LandscapeQueryNormal), true);
    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
    {
        if (Hit.GetActor() == Landscape)
        {
            OutNormal = Hit.ImpactNormal;
            return FAIK_LandscapeResult::Ok();
        }
    }

    return FAIK_LandscapeResult::Fail(
        EAIK_LandscapeError::EditFailed,
        TEXT("Trace did not hit the target landscape."));
}

#undef LOCTEXT_NAMESPACE
