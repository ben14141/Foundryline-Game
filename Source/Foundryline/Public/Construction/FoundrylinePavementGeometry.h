#pragma once
#include "Construction/FoundrylinePavementTypes.h"

// One bounded geometry contract for pavement rendering, validation and driving.
struct FOUNDRYLINE_API FFoundrylinePavementGeometry {
 struct FEditHandle {FVector2D Position;FName Tool;FString Hint;bool Corner=false;};
 static TArray<FEditHandle> EditHandles(const FFoundrylinePavementData& Data);
 using FPolygon=FFoundrylinePavementPolygon;
 static TArray<FPolygon> Rectangle(FVector2D A,FVector2D B);
 static TArray<FVector2D> RoadPath(const TArray<FVector2D>& Controls,bool Spline=false);
 static TArray<FPolygon> Road(const TArray<FVector2D>& Controls,double Width,bool Spline=false);
 static TArray<FPolygon> RoadSurface(const FFoundrylinePavementData& Data);
 static bool InEntrance(const FFoundrylinePavementData& Data,FVector2D Point);
 static bool Boolean(const TArray<FPolygon>& A,const TArray<FPolygon>& B,bool Subtract,TArray<FPolygon>& Out);
 static bool RoundCorner(TArray<FPolygon>& Polygons,FVector2D Near,double Radius);
 static bool Contains(const TArray<FPolygon>& Polygons,FVector2D P,double Tolerance=.1);
 static bool Intersects(const TArray<FPolygon>& A,const TArray<FPolygon>& B);
 static FBox2D Bounds(const TArray<FPolygon>& Polygons);
 static double Area(const TArray<FPolygon>& Polygons);
 static bool Valid(const FFoundrylinePavementData& Data,FVector2D Size,FString& Error);
 static bool Triangulate(const FPolygon& Polygon,TArray<FVector2D>& Vertices,TArray<int32>& Triangles);
 static void Translate(FFoundrylinePavementData& Data,FVector2D Delta);
 static TArray<FPolygon> Transform(const TArray<FPolygon>& Polygons,const FTransform& Transform);
 static double MinimumWidth(int Lanes,bool Truck);
 static TArray<TPair<FVector2D,FVector2D>> PaintLines(const FFoundrylinePavementMarking& Mark);
 static bool MarkingFits(const FFoundrylinePavementData& Data,const FFoundrylinePavementMarking& Mark);
 // Shared preview/commit gesture: row length, rectangular areas, optional 45-degree line snap.
 static FFoundrylinePavementMarking PaintDraft(EFoundrylinePavementMark Kind,FVector2D Start,FVector2D End,float ParkingDepth,bool Flip,bool AngleSnap);
 static FString PaintName(EFoundrylinePavementMark Kind);
 static void RoadPaint(FFoundrylinePavementData& Data,bool Force=false);
};
