#pragma once
#include "CoreMinimal.h"
#include "Grid/FoundrylineGrid.h"

// Local draft assistance only. The resulting rectangle still goes through the
// normal authoritative occupancy, ownership and setback validation.
// One magnet rule for every drawn rectangle: an edge within MagnetCm of a
// neighbour's edge snaps to it, either flush against it or aligned with its corner.
struct FFoundrylineBuildingConnections
{
 static constexpr int32 MagnetCm=200;
 static constexpr int32 JoinCm=400;
 enum : uint8 {MinX=1,MaxX=2,MinY=4,MaxY=8,AllEdges=15};
 static int32 SharedLength(const FFoundrylineGridBounds& A,const FFoundrylineGridBounds& B) {
  if(A.Min.X==B.Max.X || A.Max.X==B.Min.X) return FMath::Max(0,FMath::Min(A.Max.Y,B.Max.Y)-FMath::Max(A.Min.Y,B.Min.Y));
  if(A.Min.Y==B.Max.Y || A.Max.Y==B.Min.Y) return FMath::Max(0,FMath::Min(A.Max.X,B.Max.X)-FMath::Max(A.Min.X,B.Min.X));
  return 0;
 }
 static bool OverlapsXY(const FFoundrylineGridBounds& A,const FFoundrylineGridBounds& B) {return A.Min.X<B.Max.X && A.Max.X>B.Min.X && A.Min.Y<B.Max.Y && A.Max.Y>B.Min.Y;}
 static int32 LongestJoin(const FFoundrylineGridBounds& A,const TArray<FFoundrylineGridBounds>& Neighbors) {int32 Best=0;for(const auto& N:Neighbors) Best=FMath::Max(Best,SharedLength(A,N));return Best;}
 static bool Clear(const FFoundrylineGridBounds& A,const TArray<FFoundrylineGridBounds>& Neighbors) {for(const auto& N:Neighbors) if(OverlapsXY(A,N)) return false;return true;}

 // Snaps the free edges independently. The nearest target wins; ties prefer the
 // lower coordinate, so the result never depends on record order. Returns true
 // when the result is clear and shares at least JoinCm of wall with a neighbour.
 static bool Snap(FIntVector& Min,FIntVector& Max,const TArray<FFoundrylineGridBounds>& Neighbors,int32 MinimumCm=400,uint8 FreeEdges=AllEdges,int32 RangeCm=MagnetCm,int32 GridCm=100,int32 MaximumCm=6400) {
  FFoundrylineGridBounds C;C.Min=Min;C.Max=Max;
  const auto Raw=C;
  for(int32 Axis=0;Axis<2;++Axis) {
   const int32 Cross=1-Axis;
   int32 Best[2]={C.Min[Axis],C.Max[Axis]},Distance[2]={RangeCm+1,RangeCm+1};
   for(const auto& N:Neighbors) {
    // Only neighbours beside this rectangle guide it; distant buildings in line do not.
    if(N.Max[Cross]<Raw.Min[Cross]-RangeCm || N.Min[Cross]>Raw.Max[Cross]+RangeCm) continue;
    for(int32 Side=0;Side<2;++Side) {
     if(!(FreeEdges&(1<<(Axis*2+Side)))) continue;
     const int32 Edge=Side?Raw.Max[Axis]:Raw.Min[Axis];
     for(const int32 Value:{N.Min[Axis],N.Max[Axis]}) {
      const int32 D=FMath::Abs(Value-Edge);
      if(GridCm>1 && Value%GridCm) continue;
      if(D<Distance[Side] || (D==Distance[Side] && Value<Best[Side])) {Distance[Side]=D;Best[Side]=Value;}
     }
    }
   }
   auto Fits=[&](int32 Low,int32 High){return High-Low>=MinimumCm && High-Low<=MaximumCm;};
   if(Fits(Best[0],Best[1])) {C.Min[Axis]=Best[0];C.Max[Axis]=Best[1];}
   else if(Distance[0]<=Distance[1] && Fits(Best[0],C.Max[Axis])) C.Min[Axis]=Best[0];
   else if(Fits(C.Min[Axis],Best[1])) C.Max[Axis]=Best[1];
  }
  // A magnet never pulls a draft into something else. Keep what the player drew instead.
  if(!Clear(C,Neighbors)) return false;
  Min=C.Min;Max=C.Max;
  return LongestJoin(C,Neighbors)>=JoinCm;
 }

 // Moves keep their size: the closest edge-to-edge offset on each axis wins.
 static FIntVector SnapOffset(const FFoundrylineGridBounds& B,const TArray<FFoundrylineGridBounds>& Neighbors,int32 RangeCm=MagnetCm,int32 GridCm=100) {
  int32 Best[2]={0,0};bool Found[2]={false,false};
  for(int32 Axis=0;Axis<2;++Axis) {
   const int32 Cross=1-Axis;int32 Distance=RangeCm+1;
   for(const auto& N:Neighbors) {
    if(N.Max[Cross]<B.Min[Cross]-RangeCm || N.Min[Cross]>B.Max[Cross]+RangeCm) continue;
    for(const int32 Edge:{B.Min[Axis],B.Max[Axis]}) for(const int32 Value:{N.Min[Axis],N.Max[Axis]}) {
     const int32 Delta=Value-Edge;
     if(GridCm>1 && Delta%GridCm) continue;
     if(FMath::Abs(Delta)<Distance || (FMath::Abs(Delta)==Distance && Delta<Best[Axis])) {Distance=FMath::Abs(Delta);Best[Axis]=Delta;Found[Axis]=true;}
    }
   }
  }
  // Try both axes, then one axis, so a clash on one side does not lose the other snap.
  for(const FIntVector Offset:{FIntVector(Best[0],Best[1],0),FIntVector(Best[0],0,0),FIntVector(0,Best[1],0)}) {
   if((Offset.X && !Found[0]) || (Offset.Y && !Found[1])) continue;
   FFoundrylineGridBounds C=B;C.Min+=Offset;C.Max+=Offset;
   if(Clear(C,Neighbors)) return Offset;
  }
  return FIntVector::ZeroValue;
 }

 // Dragging from inside a building past one of its walls starts the new
 // section on that wall. A mostly perpendicular pull spans the whole wall.
 static bool ExtendFrom(const FFoundrylineGridBounds& Host,FIntVector Start,FIntVector End,FIntVector& Min,FIntVector& Max,int32 FullWallBelowCm=JoinCm) {
  // South, east, north, west: the same edge order as openings.
  const int32 Out[4]={Host.Min.Y-End.Y,End.X-Host.Max.X,End.Y-Host.Max.Y,Host.Min.X-End.X};
  int32 Edge=INDEX_NONE;
  for(int32 E=0;E<4;++E) if(Out[E]>0 && (Edge==INDEX_NONE || Out[E]>Out[Edge])) Edge=E;
  if(Edge==INDEX_NONE) return false;
  const int32 Axis=Edge%2?0:1,Along=1-Axis;
  Min[Axis]=Edge==1?Host.Max.X:Edge==2?Host.Max.Y:End[Axis];
  Max[Axis]=Edge==0?Host.Min.Y:Edge==3?Host.Min.X:End[Axis];
  if(FMath::Abs(End[Along]-Start[Along])<FullWallBelowCm) {Min[Along]=Host.Min[Along];Max[Along]=Host.Max[Along];}
  else {Min[Along]=FMath::Min(Start[Along],End[Along]);Max[Along]=FMath::Max(Start[Along],End[Along]);}
  Min.Z=Max.Z=0;
  return true;
 }
};
