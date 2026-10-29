#pragma once
#include "CoreMinimal.h"
#include "Grid/FoundrylineGrid.h"

// Local draft assistance only. The resulting rectangle still goes through the
// normal authoritative occupancy, ownership and setback validation.
struct FFoundrylineBuildingConnections
{
 static int32 SharedLength(const FFoundrylineGridBounds& A,const FFoundrylineGridBounds& B) {
  if(A.Min.X==B.Max.X || A.Max.X==B.Min.X) return FMath::Max(0,FMath::Min(A.Max.Y,B.Max.Y)-FMath::Max(A.Min.Y,B.Min.Y));
  if(A.Min.Y==B.Max.Y || A.Max.Y==B.Min.Y) return FMath::Max(0,FMath::Min(A.Max.X,B.Max.X)-FMath::Max(A.Min.X,B.Min.X));
  return 0;
 }
 static bool Snap(FIntVector& Min,FIntVector& Max,const TArray<FFoundrylineGridBounds>& Neighbors,int32 MinimumCm=400) {
  FFoundrylineGridBounds Best;int32 BestDistance=MAX_int32,BestJoin=-1;
  const FIntVector RawMin=Min,RawMax=Max;
  for(const auto& N:Neighbors) for(int32 Side=0;Side<4;++Side) {
   FFoundrylineGridBounds C;C.Min=RawMin;C.Max=RawMax;
   const int32 Distance=FMath::Abs(Side==0?RawMin.X-N.Max.X:Side==1?RawMax.X-N.Min.X:Side==2?RawMin.Y-N.Max.Y:RawMax.Y-N.Min.Y);
   if(Distance>150) continue;
   if(Side==0) C.Min.X=N.Max.X;if(Side==1) C.Max.X=N.Min.X;
   if(Side==2) C.Min.Y=N.Max.Y;if(Side==3) C.Max.Y=N.Min.Y;
   // Flush exterior corners are easier to draw than one-metre slivers. Only
   // align the along-wall axis, and never collapse the selected section.
   const int32 Axis=Side<2?1:0;
   auto Aligned=C;
   if(FMath::Abs(C.Min[Axis]-N.Min[Axis])<=100) Aligned.Min[Axis]=N.Min[Axis];
   if(FMath::Abs(C.Max[Axis]-N.Max[Axis])<=100) Aligned.Max[Axis]=N.Max[Axis];
   auto Valid=[&](const auto& B) {
    if(B.Max.X-B.Min.X<MinimumCm || B.Max.Y-B.Min.Y<MinimumCm || B.Max.X-B.Min.X>6400 || B.Max.Y-B.Min.Y>6400) return false;
    for(const auto& Other:Neighbors) if(B.Min.X<Other.Max.X && B.Max.X>Other.Min.X && B.Min.Y<Other.Max.Y && B.Max.Y>Other.Min.Y) return false;
    return SharedLength(B,N)>=400;
   };
   if(Valid(Aligned)) C=Aligned;
   if(!Valid(C)) continue;
   int32 Join=0;for(const auto& Other:Neighbors) Join+=SharedLength(C,Other);
   const int32 Movement=Distance+FMath::Abs(C.Min[Axis]-RawMin[Axis])+FMath::Abs(C.Max[Axis]-RawMax[Axis]);
   const bool Earlier=C.Min.X<Best.Min.X || (C.Min.X==Best.Min.X && (C.Min.Y<Best.Min.Y || (C.Min.Y==Best.Min.Y && (C.Max.X<Best.Max.X || (C.Max.X==Best.Max.X && C.Max.Y<Best.Max.Y)))));
   if(Movement<BestDistance || (Movement==BestDistance && (Join>BestJoin || (Join==BestJoin && Earlier)))) {Best=C;BestDistance=Movement;BestJoin=Join;}
  }
  if(BestDistance==MAX_int32) return false;
  Min=Best.Min;Max=Best.Max;return true;
 }
};
