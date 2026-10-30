#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkZoneLayout.h"

namespace {
bool PassingIntersects(FVector2D A,FVector2D B,const FBox2D& Box){
 double Lo=0,Hi=1;const auto D=B-A;
 for(int Axis=0;Axis<2;++Axis){const double P=Axis?A.Y:A.X,V=Axis?D.Y:D.X,Min=Axis?Box.Min.Y:Box.Min.X,Max=Axis?Box.Max.Y:Box.Max.X;if(FMath::Abs(V)<.000001){if(P<Min||P>Max)return false;continue;}double T0=(Min-P)/V,T1=(Max-P)/V;if(T0>T1)Swap(T0,T1);Lo=FMath::Max(Lo,T0);Hi=FMath::Min(Hi,T1);if(Lo>Hi)return false;}
 return true;
}
}
bool FFoundrylineFactorySimulation::TryEmployeePassing(FVector From,FVector To,int32 EmployeeId,TArray<FVector>& Route,float& Travelled)const{
 if(Route.Num()<2||From.ContainsNaN()||To.ContainsNaN()||!FMath::IsFinite(Travelled)||Travelled<0||CanEmployeeAdvance(From,To,EmployeeId))return false;
 const FVector Forward=(To-From).GetSafeNormal2D();if(Forward.IsNearlyZero())return false;
 bool HumanBlocked=false;
 auto Human=[&](int32 Id,FVector P,bool Moving){if(Id==EmployeeId)return;const float Before=FVector::Dist2D(From,P),After=FVector::Dist2D(To,P);if(Moving&&After<52&&After<=Before+.01)HumanBlocked=true;};
 for(const auto& O:State.Operators)Human(O.Id,O.Position,O.Path.Num()>1&&O.Distance<Length(O.Path)-1);
 for(const auto& C:State.Collectors)Human(C.Id,C.Position,C.Speed>.1);
 for(const auto& W:State.Walkers)Human(W.Id,W.Position,W.Path.Num()>1&&W.Distance<Length(W.Path)-1);
 if(!HumanBlocked)return false;
 // Passing is never a way around forklift priority. Be deliberately conservative
 // near a vehicle or its approaching route; normal crossing/traffic checks still apply.
 for(const auto& D:State.Drivers){
  if(FVector::Dist2D(From,D.Position)<500)return false;
  if(D.Path.Num()>1&&!D.Waiting){const float End=FMath::Min(Length(D.Path),D.Distance+900);for(float At=D.Distance;At<=End;At+=50)if(FVector::Dist2D(From,Along(D.Path,At))<500)return false;}
 }
 const float RejoinAt=FMath::Min(Length(Route),Travelled+200);const FVector Rejoin=Along(Route,RejoinAt);const FVector Delta=Rejoin-From;
 const FVector Sideways(-Forward.Y,Forward.X,0);
 if(FVector::DotProduct(Delta,Forward)<100||FMath::Abs(FVector::DotProduct(Delta,Sideways))>2)return false;
 const FVector Side=From+Sideways*35,Parallel=Rejoin+Sideways*35;
 bool WidePaint=false;
 for(const auto& L:State.Lanes)if(L.Kind==EFoundrylineFloorLane::Pedestrian&&FMath::Min(L.Max.X-L.Min.X,L.Max.Y-L.Min.Y)>=140){const FBox2D Inner(L.Min+FVector2D(35,35),L.Max-FVector2D(35,35));if(Inner.IsInsideOrOn(FVector2D(From))&&Inner.IsInsideOrOn(FVector2D(Side))&&Inner.IsInsideOrOn(FVector2D(Parallel))&&Inner.IsInsideOrOn(FVector2D(Rejoin))){WidePaint=true;break;}}
 if(!WidePaint||!CanEmployeeAdvance(From,Side,EmployeeId))return false;
 auto PersonClear=[&](int32 Id,FVector P){return Id==EmployeeId||FMath::PointDistToSegment(FVector(P.X,P.Y,0),FVector(From.X,From.Y,0),FVector(Side.X,Side.Y,0))>=52-.01;};
 for(const auto& O:State.Operators)if(!PersonClear(O.Id,O.Position))return false;
 for(const auto& C:State.Collectors)if(!PersonClear(C.Id,C.Position))return false;
 for(const auto& W:State.Walkers)if(!PersonClear(W.Id,W.Position))return false;
 for(const auto& E:State.UnassignedEmployees)if(!PersonClear(E.Id,E.Position))return false;
 TArray<FVector> Candidate{From,Side,Parallel,Rejoin};float Accumulated=0;
 for(int I=1;I<Route.Num();++I){Accumulated+=FVector::Distance(Route[I-1],Route[I]);if(Accumulated+1<RejoinAt)continue;if(!Candidate.Last().Equals(Route[I],.1))Candidate.Add(Route[I]);}
 auto Physical=[&](FVector A,FVector B){
  for(const FVector P:{A,B})if(P.X<State.FloorMin.X+35||P.Y<State.FloorMin.Y+35||P.X>State.FloorMax.X-35||P.Y>State.FloorMax.Y-35)return false;
  for(const auto& S:Solids)if(PassingIntersects(FVector2D(A),FVector2D(B),S.ExpandBy(35)))return false;
  for(const auto& Z:State.Zones){if(PassingIntersects(FVector2D(A),FVector2D(B),FFoundrylineWorkZoneLayout::WelderBounds(Z).ExpandBy(35)))return false;const auto R=FFoundrylineWorkZoneLayout::Rotation(Z);const auto C=FFoundrylineWorkZoneLayout::ProductCenter(Z);if(PassingIntersects(FVector2D(R.UnrotateVector(A-C)),FVector2D(R.UnrotateVector(B-C)),FBox2D({-700,-165},{700,165})))return false;}
  return true;
 };
 for(int I=1;I<Candidate.Num();++I)if(!Physical(Candidate[I-1],Candidate[I]))return false;
 // Only the sidestep is reserved against CURRENT people. The forward track is
 // checked on every real movement step, letting the other person step aside too.
 Route=MoveTemp(Candidate);Travelled=0;return true;
}
