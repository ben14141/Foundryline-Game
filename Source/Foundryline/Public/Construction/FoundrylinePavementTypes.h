#pragma once
#include "CoreMinimal.h"
#include "FoundrylinePavementTypes.generated.h"

USTRUCT()
struct FOUNDRYLINE_API FFoundrylinePavementLoop {
 GENERATED_BODY()
 UPROPERTY(SaveGame) TArray<FVector2D> Points;
};
USTRUCT()
struct FOUNDRYLINE_API FFoundrylinePavementPolygon {
 GENERATED_BODY()
 UPROPERTY(SaveGame) TArray<FVector2D> Outer;
 UPROPERTY(SaveGame) TArray<FFoundrylinePavementLoop> Holes;
};
UENUM()
enum class EFoundrylinePavementMark:uint8 {Parking,WhiteLine,YellowLine,DashedLine,Arrow,Loading,KeepClear,Stop};
USTRUCT()
struct FOUNDRYLINE_API FFoundrylinePavementMarking {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid Id;
 UPROPERTY(SaveGame) EFoundrylinePavementMark Kind=EFoundrylinePavementMark::Parking;
 UPROPERTY(SaveGame) TArray<FVector2D> Points;
 // Parking bay depth, loading depth or stripe envelope width. Centimetres.
 UPROPERTY(SaveGame) float WidthCm=550;
 UPROPERTY(SaveGame) bool RoadGenerated=false;
};
USTRUCT()
struct FOUNDRYLINE_API FFoundrylinePavementData {
 GENERATED_BODY()
 UPROPERTY(SaveGame) TArray<FFoundrylinePavementPolygon> Polygons;
 UPROPERTY(SaveGame) TArray<FVector2D> RoadControls;
 UPROPERTY(SaveGame) bool Spline=false;
 UPROPERTY(SaveGame) float RoadWidthCm=0;
 // Circular curb returns at connected road ends. Old saves keep square ends.
 UPROPERTY(SaveGame) float EntranceRadiusCm=0;
 UPROPERTY(SaveGame) uint8 EntranceEnds=0; // bit 0 = start, bit 1 = end
 UPROPERTY(SaveGame) uint8 Lanes=0;
 UPROPERTY(SaveGame) bool TruckAccess=false;
 UPROPERTY(SaveGame) bool Curbs=true;
 UPROPERTY(SaveGame) TArray<FFoundrylinePavementMarking> Markings;
};
