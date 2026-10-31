#pragma once
#include "Construction/FoundrylinePlacementTypes.h"
#include "Construction/FoundrylinePlaceableDefinition.h"

// A loading bay is owned by its opening: no independently editable paint or orphan props.
struct FOUNDRYLINE_API FFoundrylineLoadingBayLayout {
 struct FSpec {const TCHAR* Name;float Width,Depth,VehicleWidth,VehicleLength,AxleOffset;};
 static FSpec Spec(uint8 Tier);
 static FVector Point(const UFoundrylinePlaceableDefinition& Building,const FFoundrylineOpening& Opening,float Across,float Outward,float Z=0);
 static FFoundrylineGridBounds Bounds(const UFoundrylinePlaceableDefinition& Building,const FFoundrylinePlacedObject& Object,const FFoundrylineOpening& Opening);
 static FGuid SurfaceId(FGuid Opening){return FGuid(Opening.A^0xBA700001,Opening.B,Opening.C,Opening.D);}
};
