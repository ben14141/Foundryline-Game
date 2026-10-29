#pragma once
#include "CoreMinimal.h"
#include "Grid/FoundrylineGrid.h"

struct FFoundrylineDriveRect {
 FVector2D Min,Max;
 bool Contains(FVector2D P,float Margin=0) const {return P.X>=Min.X+Margin && P.Y>=Min.Y+Margin && P.X<=Max.X-Margin && P.Y<=Max.Y-Margin;}
 bool Overlaps(const FFoundrylineDriveRect& B) const {return Min.X<B.Max.X && Max.X>B.Min.X && Min.Y<B.Max.Y && Max.Y>B.Min.Y;}
};
struct FFoundrylineParkingBay {FFoundrylineDriveRect Bounds;FVector2D RearAxle;float Heading=0;};
// Shared by paint, curbs, targets and pathfinding. All values are authored centimetres.
struct FOUNDRYLINE_API FFoundrylineDrivingLayout {
 static constexpr float CarBayWidth=250;
 static constexpr float CarBayDepth=550;
 static constexpr float TruckBayDepth=2200;
 static constexpr float TruckBayWidth=400;
 static constexpr float TruckApron=3000;
 static bool IsPaved(FName Function);
 static TArray<FFoundrylineParkingBay> Parking(float Width,float Depth,const TArray<FFoundrylineDriveRect>& Entrances);
 static TArray<FFoundrylineDriveRect> ParkingAisles(float Width,float Depth,const TArray<FFoundrylineDriveRect>& Entrances);
 static TArray<FFoundrylineDriveRect> Islands(float Width,float Depth,bool Landscaped);
};
