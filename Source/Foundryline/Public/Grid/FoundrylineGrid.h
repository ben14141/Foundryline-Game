#pragma once

#include "CoreMinimal.h"
#include "FoundrylineGrid.generated.h"

UENUM(BlueprintType)
enum class EFoundrylineSnap : uint8
{
	Structural100cm,
	Fine25cm,
	Precision10cm,
	Precision5cm
};

// Integer coordinates use real Unreal centimetres. Max is exclusive; touching faces do not overlap.
USTRUCT(BlueprintType)
struct FOUNDRYLINE_API FFoundrylineGridBounds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FIntVector Min = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FIntVector Max = FIntVector::ZeroValue;

	bool IsValid() const;
	bool Overlaps(const FFoundrylineGridBounds& Other) const;
};

struct FOUNDRYLINE_API FFoundrylineGrid
{
	static constexpr int32 UnitCm = 1;
	static constexpr int32 MaxCoordinate = 10000000; // Safety bound, not map size.
	static int32 SnapStep(EFoundrylineSnap Snap);
	static bool IsSafeCoordinate(const FIntVector& Value);
	static bool IsOnSnap(const FIntVector& Anchor, EFoundrylineSnap Snap);
	// Grid origin is world (0,0,0). Returns false for NaN, infinity or out-of-range input.
	static bool SnapWorld(const FVector& WorldCm, EFoundrylineSnap Snap, FIntVector& OutAnchor);
	static FVector ToWorld(const FIntVector& Anchor);
 static FIntVector RotateOffset(const FIntVector& Offset,uint8 QuarterTurns);
	// Anchor is the unrotated minimum corner / rotation pivot, not the mesh pivot.
	static FFoundrylineGridBounds RotateBounds(const FFoundrylineGridBounds& Local,
		const FIntVector& Anchor, uint8 QuarterTurns);
};
