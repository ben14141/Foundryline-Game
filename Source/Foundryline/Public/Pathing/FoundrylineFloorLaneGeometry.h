#pragma once
#include "Production/FoundrylineFactoryTypes.h"

struct FFoundrylineFloorCrossing {
 FBox2D Bounds;
 bool WalkingX=false;
};
struct FOUNDRYLINE_API FFoundrylineFloorLaneGeometry {
 // Removes segments covered by forklift pavement. Probe the outside of a
 // boundary to preserve shared straight edges but open connected junctions.
 static TArray<TPair<FVector2D,FVector2D>> VisibleLine(FVector2D A,FVector2D B,const TArray<FFoundrylineFactoryLane>& Lanes,int32 IgnoreId,FVector2D OutsideProbe=FVector2D::ZeroVector,EFoundrylineFloorLane Kind=EFoundrylineFloorLane::Forklift);
 static TArray<FFoundrylineFloorCrossing> Crossings(const TArray<FFoundrylineFactoryLane>& Lanes);
};
