#include "Production/FoundrylineFactorySimulation.h"

bool FFoundrylineFactorySimulation::CanEmployeeAdvance(FVector From,FVector To,int32 EmployeeId)const{
 const auto XY=[](FVector P){return FVector(P.X,P.Y,0);};From=XY(From);To=XY(To);
 // A person already inside a marked crossing clears it rather than stopping in traffic.
 for(const auto& C:FloorCrossings())if(!C.Bounds.IsInside(FVector2D(From))&&C.Bounds.ExpandBy(60).IsInside(FVector2D(To))){
  auto Distance=[&](FVector P){const double X=FMath::Max3(C.Bounds.Min.X-P.X,0.,P.X-C.Bounds.Max.X),Y=FMath::Max3(C.Bounds.Min.Y-P.Y,0.,P.Y-C.Bounds.Max.Y);return X*X+Y*Y;};
  // A person/cart clearing the approach ring must be allowed to retreat; otherwise both vehicles wait forever.
  if(Distance(To)>=Distance(From)-.01)continue;
  for(const auto& D:State.Drivers){if(C.Bounds.ExpandBy(200).IsInside(FVector2D(D.Position)))return false;
   if(D.Path.Num()>1&&!D.Waiting){const float End=FMath::Min(Length(D.Path),D.Distance+900);for(float At=D.Distance;At<=End;At+=50)if(C.Bounds.ExpandBy(120).IsInside(FVector2D(Along(D.Path,At))))return false;}
  }
 }
 // Unmarked machine spurs still need real clearance. Escaping an existing overlap is legal.
 for(const auto& D:State.Drivers){const FVector Offset=FRotator(0,D.Yaw,0).RotateVector({-95,0,0});const FVector P=XY(D.Position+Offset);const float Before=FVector::Dist(From,P),After=FVector::Dist(To,P);if(Before<185&&After>Before+.01)continue;
  float AheadYaw=D.Yaw;const FVector Ahead=D.Path.Num()>1&&!D.Waiting?XY(Along(D.Path,FMath::Min(Length(D.Path),D.Distance+300),&AheadYaw)+FRotator(0,AheadYaw,0).RotateVector({-95,0,0})):P;
  if(FMath::PointDistToSegment(To,P,Ahead)<185&&FMath::PointDistToSegment(To,P,Ahead)<=FMath::PointDistToSegment(From,P,Ahead)+.01)return false;
 }
 const auto Clear=[&](int32 Id,FVector Other){if(Id==EmployeeId)return true;Other=XY(Other);const float Before=FVector::Dist(From,Other),After=FVector::Dist(To,Other);return After>=52||After>Before+.01;};
 for(const auto& C:State.Collectors)if(!Clear(C.Id,C.Position)||!Clear(C.Id,C.Position-FRotator(0,C.Yaw,0).RotateVector(FVector(90,0,0))))return false;
 for(const auto& W:State.Walkers)if(!Clear(W.Id,W.Position))return false;
 for(const auto& O:State.Operators)if(!Clear(O.Id,O.Position))return false;
 for(const auto& D:State.UnassignedEmployees)if(!Clear(D.Id,D.Position))return false;
 return true;
}
