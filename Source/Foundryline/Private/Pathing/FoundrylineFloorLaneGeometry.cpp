#include "Pathing/FoundrylineFloorLaneGeometry.h"

TArray<TPair<FVector2D,FVector2D>> FFoundrylineFloorLaneGeometry::VisibleLine(FVector2D A,FVector2D B,const TArray<FFoundrylineFactoryLane>& Lanes,int32 IgnoreId,FVector2D Probe,EFoundrylineFloorLane Kind){
 const bool X=FMath::Abs(B.X-A.X)>=FMath::Abs(B.Y-A.Y);const double Start=X?A.X:A.Y,End=X?B.X:B.Y;
 TArray<TPair<double,double>> Ranges;Ranges.Emplace(Start,End);
 for(const auto& Other:Lanes){if(Other.Id==IgnoreId)continue;
  const bool OtherX=Other.Max.X-Other.Min.X>Other.Max.Y-Other.Min.Y;
  const bool Joins=Other.Kind==Kind||Other.Kind==EFoundrylineFloorLane::Crossing||(Kind==EFoundrylineFloorLane::Pedestrian&&Other.Kind==EFoundrylineFloorLane::Forklift)||(Kind==EFoundrylineFloorLane::Forklift&&Other.Kind==EFoundrylineFloorLane::Pedestrian&&OtherX!=X);
  if(!Joins)continue;
  const double Across=X?A.Y+Probe.Y:A.X+Probe.X;
  if(Across<(X?Other.Min.Y:Other.Min.X)||Across>(X?Other.Max.Y:Other.Max.X))continue;
  const double Low=X?Other.Min.X:Other.Min.Y,High=X?Other.Max.X:Other.Max.Y;TArray<TPair<double,double>> Next;
  for(auto R:Ranges){if(High<=R.Key||Low>=R.Value){Next.Add(R);continue;}if(Low>R.Key)Next.Emplace(R.Key,Low);if(High<R.Value)Next.Emplace(High,R.Value);}Ranges=MoveTemp(Next);
 }
 TArray<TPair<FVector2D,FVector2D>> Out;for(auto R:Ranges)if(R.Value-R.Key>.1)Out.Emplace(X?FVector2D(R.Key,A.Y):FVector2D(A.X,R.Key),X?FVector2D(R.Value,A.Y):FVector2D(A.X,R.Value));return Out;
}
TArray<FFoundrylineFloorCrossing> FFoundrylineFloorLaneGeometry::Crossings(const TArray<FFoundrylineFactoryLane>& Lanes){
 TArray<FFoundrylineFloorCrossing> Out;
 for(const auto& L:Lanes)if(L.Kind==EFoundrylineFloorLane::Crossing)Out.Add({FBox2D(L.Min,L.Max),L.Max.X-L.Min.X>L.Max.Y-L.Min.Y});
 for(const auto& P:Lanes)if(P.Kind==EFoundrylineFloorLane::Pedestrian)for(const auto& F:Lanes)if(F.Kind==EFoundrylineFloorLane::Forklift){
  const bool PX=P.Max.X-P.Min.X>P.Max.Y-P.Min.Y,FX=F.Max.X-F.Min.X>F.Max.Y-F.Min.Y;if(PX==FX)continue;
  const FVector2D A(FMath::Max(P.Min.X,F.Min.X),FMath::Max(P.Min.Y,F.Min.Y)),B(FMath::Min(P.Max.X,F.Max.X),FMath::Min(P.Max.Y,F.Max.Y));if(B.X-A.X<5||B.Y-A.Y<5)continue;
  Out.Add({FBox2D(A,B),PX});
 }
 // Merge adjacent pieces of the same crossing without painting over an L-shaped void.
 bool Changed=true;while(Changed){Changed=false;for(int I=0;I<Out.Num()&&!Changed;++I)for(int J=I+1;J<Out.Num();++J){const auto A=Out[I].Bounds,B=Out[J].Bounds;
  if(Out[I].WalkingX!=Out[J].WalkingX)continue;
  const bool SameX=FMath::IsNearlyEqual(A.Min.X,B.Min.X,.01)&&FMath::IsNearlyEqual(A.Max.X,B.Max.X,.01),SameY=FMath::IsNearlyEqual(A.Min.Y,B.Min.Y,.01)&&FMath::IsNearlyEqual(A.Max.Y,B.Max.Y,.01);
  const bool Contained=(A.IsInsideOrOn(B.Min)&&A.IsInsideOrOn(B.Max))||(B.IsInsideOrOn(A.Min)&&B.IsInsideOrOn(A.Max));
  if(Contained||(SameX&&A.Min.Y<=B.Max.Y+.01&&B.Min.Y<=A.Max.Y+.01)||(SameY&&A.Min.X<=B.Max.X+.01&&B.Min.X<=A.Max.X+.01)){
   Out[I].Bounds=FBox2D(FVector2D(FMath::Min(A.Min.X,B.Min.X),FMath::Min(A.Min.Y,B.Min.Y)),FVector2D(FMath::Max(A.Max.X,B.Max.X),FMath::Max(A.Max.Y,B.Max.Y)));Out.RemoveAt(J);Changed=true;break;
  }
 }}return Out;
}
