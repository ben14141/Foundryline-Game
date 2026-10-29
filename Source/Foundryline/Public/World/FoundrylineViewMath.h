#pragma once

#include "CoreMinimal.h"
#include "Grid/FoundrylineGrid.h"

// Camera-independent math, shared by mouse picking and future touch/controller adapters.
struct FOUNDRYLINE_API FFoundrylineViewMath
{
	static bool GroundAnchor(const FVector& Origin, const FVector& Direction,
		EFoundrylineSnap Snap, FIntVector& OutAnchorCm)
	{
		if (Origin.ContainsNaN() || Direction.ContainsNaN() || FMath::Abs(Direction.Z) < 0.000001)
			return false;
		const double Distance = -Origin.Z / Direction.Z;
		if (Distance < 0.0 || !FMath::IsFinite(Distance)) return false;
		return FFoundrylineGrid::SnapWorld(Origin + Direction * Distance, Snap, OutAnchorCm);
	}
};
