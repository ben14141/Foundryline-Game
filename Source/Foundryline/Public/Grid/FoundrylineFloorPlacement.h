#pragma once
#include "CoreMinimal.h"
// Shared physical floor coordinates, independent of the construction grid and camera.
struct FOUNDRYLINE_API FFoundrylineFloorPlacement {
 static bool FloorPoint(FVector Origin,FVector Direction,FVector& Out);
 static bool Overlap(const FBox2D& A,const FBox2D& B);
 static FVector2D Snap(FVector2D Raw,FVector2D Half,double Step,const FBox2D& Floor,const TArray<FBox2D>& Targets,double Gap,bool Magnet,bool& Snapped);
};
