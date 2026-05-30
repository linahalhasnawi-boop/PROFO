// Copyright Lina Hal Hasnawi. Licensed under MIT.

#pragma once

#include "CoreMinimal.h"
#include "AIK_LandscapeTypes.generated.h"

/**
 * Axis-aligned region in landscape grid coordinates (0..resolution),
 * NOT in world centimeters. Using grid coords is a deliberate guard
 * against the positional-array footgun in the host kit's stock
 * landscape ops — see docs/tool-surface.md for the full rationale.
 *
 * x1, y1 = inclusive top-left
 * x2, y2 = exclusive bottom-right
 * x2 > x1, y2 > y1 always; degenerate regions are rejected at the
 * tool boundary, not silently no-op'd.
 */
USTRUCT(BlueprintType)
struct AIK_LANDSCAPE_API FAIK_LandscapeRegion
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "AIK|Landscape")
    int32 X1 = 0;

    UPROPERTY(BlueprintReadWrite, Category = "AIK|Landscape")
    int32 Y1 = 0;

    UPROPERTY(BlueprintReadWrite, Category = "AIK|Landscape")
    int32 X2 = 0;

    UPROPERTY(BlueprintReadWrite, Category = "AIK|Landscape")
    int32 Y2 = 0;

    bool IsValid() const
    {
        return X2 > X1 && Y2 > Y1 && X1 >= 0 && Y1 >= 0;
    }

    int32 Width() const  { return FMath::Max(0, X2 - X1); }
    int32 Height() const { return FMath::Max(0, Y2 - Y1); }
    int32 Area() const   { return Width() * Height(); }
};

/**
 * Result of a peak query — the highest Z point inside a region.
 * GridX, GridY are returned in landscape grid coords for symmetry with input;
 * WorldZ is in centimeters (UE world units).
 */
USTRUCT(BlueprintType)
struct AIK_LANDSCAPE_API FAIK_LandscapePeak
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    int32 GridX = 0;

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    int32 GridY = 0;

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    float WorldZ = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    FVector WorldLocation = FVector::ZeroVector;
};

/**
 * Tool result envelope. Every operation returns one of these so the agent
 * can branch on Success without parsing free-text error messages.
 * ErrorCode is one of the EAIK_LandscapeError enum values; ErrorMessage
 * is a developer-facing string for logs (never surfaced in agent output).
 */
UENUM(BlueprintType)
enum class EAIK_LandscapeError : uint8
{
    None,
    LandscapeNotFound,
    RegionInvalid,
    RegionOutOfBounds,
    EditFailed,
    EditorOnlyOperation,
};

USTRUCT(BlueprintType)
struct AIK_LANDSCAPE_API FAIK_LandscapeResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    EAIK_LandscapeError ErrorCode = EAIK_LandscapeError::None;

    UPROPERTY(BlueprintReadOnly, Category = "AIK|Landscape")
    FString ErrorMessage;

    static FAIK_LandscapeResult Ok()
    {
        return FAIK_LandscapeResult{ true, EAIK_LandscapeError::None, TEXT("") };
    }

    static FAIK_LandscapeResult Fail(EAIK_LandscapeError Code, const FString& Msg)
    {
        return FAIK_LandscapeResult{ false, Code, Msg };
    }
};
