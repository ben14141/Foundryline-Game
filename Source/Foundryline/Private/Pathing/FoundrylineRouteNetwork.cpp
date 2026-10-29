#include "Pathing/FoundrylineRouteNetwork.h"
#include "Construction/FoundrylinePlacementSubsystem.h"

FFoundrylineRouteNetwork FFoundrylineRouteNetwork::Read(const UFoundrylinePlacementSubsystem& P) {
 FFoundrylineRouteNetwork G;G.Revision=P.GetRevision();
 for(const auto& O:P.GetObjects()) {
  const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));
  for(const auto& N:O.RouteNodes) {FFoundrylineWorldRouteNode W;W.Data=N;W.Surface=O.ObjectId;W.Company=O.CompanyId;W.Plant=O.PlantId;const auto V=T.TransformPosition(FVector(N.Position.X,N.Position.Y,0));W.Position={V.X,V.Y};G.Nodes.Add(W);}
 }
 for(const auto& N:G.Nodes) for(const auto& E:N.Data.Links) {const auto* B=G.Find(E.To);if(!B || B->Company!=N.Company || B->Plant!=N.Plant) ++G.BrokenLinks;}
 return G;
}
const FFoundrylineWorldRouteNode* FFoundrylineRouteNetwork::Find(FGuid Id) const {return Nodes.FindByPredicate([&](const auto& N){return N.Data.Id==Id;});}
TArray<FGuid> FFoundrylineRouteNetwork::Path(FGuid From,FGuid To,const TMap<FGuid,float>& Wait) const {
 TArray<FGuid> Out;if(!Find(From) || !Find(To) || From==To || Nodes.Num()>4096) return Out;
 TMap<FGuid,double> Cost;TMap<FGuid,FGuid> Parent;TSet<FGuid> Closed;Cost.Add(From,0);
 for(int32 I=0;I<Nodes.Num();++I) {
  FGuid Current;double Best=MAX_dbl;for(const auto& C:Cost) if(!Closed.Contains(C.Key) && C.Value<Best){Best=C.Value;Current=C.Key;}
  if(!Current.IsValid()) break;if(Current==To) {for(FGuid N=To;N!=From;N=Parent.FindChecked(N)) Out.Insert(N,0);Out.Insert(From,0);return Out;}
  Closed.Add(Current);const auto& A=*Find(Current);
  for(const auto& E:A.Data.Links) {const auto* B=Find(E.To);if(!B || B->Company!=A.Company || B->Plant!=A.Plant || Closed.Contains(E.To)) continue;
   const float Delay=Wait.FindRef(E.To);if(!FMath::IsFinite(Delay) || Delay<0) continue;
   const double Next=Best+FVector2D::Distance(A.Position,B->Position)/(E.Reverse?100.:400.)+Delay;
   if(!Cost.Contains(E.To) || Next<Cost[E.To]) {Cost.Add(E.To,Next);Parent.Add(E.To,Current);}
  }
 }return Out;
}
FFoundrylineNetworkCheck FFoundrylineRouteNetwork::Check(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FGuid From,FGuid To,const TMap<FGuid,float>& Wait) const {
 FFoundrylineNetworkCheck R;R.Revision=Revision;R.Route.Revision=Revision;R.Nodes=Path(From,To,Wait);
 auto Fail=[&](FString Reason){R.Reason=MoveTemp(Reason);R.Route.Reason=R.Reason;return R;};
 if(R.Nodes.Num()<2) return Fail(TEXT("No directed connection. Draw links from START to FINISH"));
 struct FCorner {FVector2D Enter,Exit;double Trim=0,Angle=0,Radius=0;};
 const int32 Count=R.Nodes.Num();TArray<FCorner> Corners;Corners.SetNum(Count);TArray<bool> Reverse;
 for(int32 I=0;I<Count;++I) Corners[I].Enter=Corners[I].Exit=Find(R.Nodes[I])->Position;
 for(int32 I=0;I<Count-1;++I) {const auto* A=Find(R.Nodes[I]);const auto* E=A->Data.Links.FindByPredicate([&](const auto& L){return L.To==R.Nodes[I+1];});Reverse.Add(E->Reverse);if(FVector2D::Distance(Corners[I].Exit,Corners[I+1].Enter)<100) return Fail(TEXT("Nodes must be at least 1 m apart"));}
 for(int32 I=1;I<Count-1;++I) {
  const auto A=Find(R.Nodes[I-1])->Position,B=Find(R.Nodes[I])->Position,C=Find(R.Nodes[I+1])->Position;
  const auto In=(B-A).GetSafeNormal(),Out=(C-B).GetSafeNormal();auto& F=Corners[I];
  F.Angle=FMath::Atan2(In.X*Out.Y-In.Y*Out.X,FVector2D::DotProduct(In,Out));
  if(Reverse[I]!=Reverse[I-1]) {if(FMath::Abs(FMath::Abs(F.Angle)-PI)>.01) return Fail(TEXT("Changing gear requires a straight backing approach in the opposite direction"));F.Angle=0;continue;}
  if(FMath::Abs(F.Angle)>PI-.05) return Fail(FString::Printf(TEXT("NODE %d: U-turn needs a loop, not a sharp reversal"),I+1));
  if(Reverse[I]!=Reverse[I-1] && FMath::Abs(F.Angle)>.01) return Fail(TEXT("Gear changes need a straight, aligned approach. Move the backing node"));
  F.Radius=FMath::Max(float(Find(R.Nodes[I])->Data.RadiusCm),V.TurnRadius);
  F.Trim=F.Radius*FMath::Tan(FMath::Abs(F.Angle)/2);F.Enter=B-In*F.Trim;F.Exit=B+Out*F.Trim;
 }
 for(int32 I=0;I<Count-1;++I) if(Corners[I].Trim+Corners[I+1].Trim>FVector2D::Distance(Find(R.Nodes[I])->Position,Find(R.Nodes[I+1])->Position)-1)
  return Fail(FString::Printf(TEXT("TURN %d-%d needs more space. Move nodes apart or use Fit Turn"),I+1,I+2));
 FFoundrylineVehiclePose P;const auto D=(Corners[1].Enter-Corners[0].Exit).GetSafeNormal();
 P.Position=Corners[0].Exit;P.Yaw=FMath::Atan2(D.Y,D.X)+(Reverse[0]?PI:0);P.TrailerYaw=P.Yaw;P.Reverse=Reverse[0];
 auto Wide=V;Wide.Clearance+=50;R.Comfortable=true;
 auto Sample=[&](){
  if((P.Reverse && !M.ReverseAllowed(P.Position)) || R.Route.Poses.Num()>20000 || !FFoundrylineVehiclePath::PoseClear(M,V,P,true)) return false;
  if(!FFoundrylineVehiclePath::PoseClear(M,Wide,P,true)) R.Comfortable=false;const float Time=R.Route.Poses.IsEmpty()?0:R.PoseTimes.Last()+FVector2D::Distance(R.Route.Poses.Last().Position,P.Position)/(P.Reverse?100.f:400.f);R.PoseTimes.Add(Time);R.Route.Poses.Add(P);return true;
 };
 if(!Sample()) return Fail(TEXT("START needs room behind it for the whole vehicle and trailer"));
 for(int32 I=0;I<Count-1;++I) {
  P.Reverse=Reverse[I];const float Sign=P.Reverse?-1.f:1.f;
  const double Distance=FVector2D::Distance(Corners[I].Exit,Corners[I+1].Enter);
  for(double Left=Distance;Left>.01;) {const float Step=FMath::Min(25.,Left);P=FFoundrylineVehiclePath::Advance(P,V,Sign*Step,0);Left-=Step;if(!Sample()) return Fail(FString::Printf(TEXT("LINK %d: cab or trailer leaves permitted paving, enters an island, or jackknifes"),I+1));}
  if(I+1<Count-1 && FMath::Abs(Corners[I+1].Angle)>.0001) {
   const auto& C=Corners[I+1];const float Curvature=C.Angle>=0?1.f/C.Radius:-1.f/C.Radius;
   for(double Left=FMath::Abs(C.Angle)*C.Radius;Left>.01;) {const float Step=FMath::Min(25.,Left);P=FFoundrylineVehiclePath::Advance(P,V,Sign*Step,Curvature*Sign);Left-=Step;if(!Sample()) return Fail(FString::Printf(TEXT("NODE %d: trailer sweep needs wider paving or a wider turn"),I+2));}
  }
 }
 R.Success=R.Route.Success=true;R.Reason=R.Comfortable?TEXT("CLEAR / turning radius and full trailer sweep pass, with extra margin"):TEXT("TIGHT / physical clearance passes; allow more space for comfortable traffic");return R;
}
