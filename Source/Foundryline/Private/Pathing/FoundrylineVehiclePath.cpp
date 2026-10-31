#include "Construction/FoundrylinePavementGeometry.h"
#include "Pathing/FoundrylineVehiclePath.h"
#include "HAL/PlatformTime.h"
#include <queue>
#include "Algo/Reverse.h"

bool FFoundrylineDrivingMap::PointAllowed(FVector2D P,bool Semi,bool Zones,const FGuid& Target) const
{
 for(const auto& S:Surfaces) {
  const auto L=S.Local(P);if(!S.Pavement.Polygons.IsEmpty()&&!FFoundrylinePavementGeometry::Contains(S.Pavement.Polygons,L))continue;
  if(L.X<-.001 || L.Y<-.001 || L.X>S.Size.X+.001 || L.Y>S.Size.Y+.001) continue;
  if((S.Function==TEXT("Parking") || S.Function==TEXT("Road2") || S.Function==TEXT("Road4")) && (L.X<40 || L.X>S.Size.X-40 || (S.Function==TEXT("Parking") && (L.Y<40 || L.Y>S.Size.Y-40)))) {
   bool Portal=false;
   for(const auto& Other:Surfaces) if(Other.Id!=S.Id) {const auto Q=Other.Local(P);if(Other.Pavement.Polygons.IsEmpty()?FFoundrylineDriveRect{{-50,-50},Other.Size+FVector2D(50,50)}.Contains(Q):FFoundrylinePavementGeometry::Contains(Other.Pavement.Polygons,Q,50)) {Portal=true;break;}}
   if(!Portal) continue;
  }
  if(S.Islands.ContainsByPredicate([&](const auto& R){return R.Contains(L);})) continue;
  if(Semi && Zones && !S.Public && !S.Pavement.Lanes && !S.TruckZones.ContainsByPredicate([&](const auto& R){return R.Contains(L);})) continue;
  if(!S.Pavement.Polygons.IsEmpty()&&S.Pavement.Curbs){bool Border=false;for(const auto& Poly:S.Pavement.Polygons){auto Edge=[&](const auto& Loop){for(int I=0;I<Loop.Num();++I)if(FMath::PointDistToSegment(FVector(L,0),FVector(Loop[I],0),FVector(Loop[(I+1)%Loop.Num()],0))<18)Border=true;};Edge(Poly.Outer);for(const auto& Hole:Poly.Holes)Edge(Hole.Points);}if(Border){bool Portal=false;for(const auto& Other:Surfaces)if(Other.Id!=S.Id){const auto Q=Other.Local(P);if(Other.Pavement.Polygons.IsEmpty()?FFoundrylineDriveRect{{-20,-20},Other.Size+FVector2D(20,20)}.Contains(Q):FFoundrylinePavementGeometry::Contains(Other.Pavement.Polygons,Q,20)){Portal=true;break;}}if(!Portal)continue;}}
  if(S.Function==TEXT("Parking")) {
   if(S.Aisles.ContainsByPredicate([&](const auto& R){return R.Contains(L);})) return true;
   if(!Semi) if(const auto* T=Targets.FindByPredicate([&](const auto& T){return T.Id==Target && T.SurfaceId==S.Id;})) if(T->Bay.Contains(L)) return true;
   if(!Semi) return true; // Empty parking pavement remains usable for manoeuvring.
   continue;
  }
  return true;
 }
 return false;
}
bool FFoundrylineDrivingMap::ReverseAllowed(FVector2D P) const
{
 for(const auto& S:Surfaces) if((S.Function==TEXT("Parking") || S.Function==TEXT("FreightYard") || S.Function==TEXT("Pavement")&&!S.Pavement.Lanes) && FFoundrylineDriveRect{{0,0},S.Size}.Contains(S.Local(P)) && (S.Pavement.Polygons.IsEmpty()||FFoundrylinePavementGeometry::Contains(S.Pavement.Polygons,S.Local(P)))) return true;return false;
}
bool FFoundrylineDrivingMap::LaneAllowed(const FFoundrylineVehiclePose& P,bool Semi) const
{
 const auto Dir=FVector2D(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw));
 for(const auto& S:Surfaces) {
  if(S.Pavement.Lanes){const auto L=S.Local(P.Position);if(!FFoundrylinePavementGeometry::Contains(S.Pavement.Polygons,L))continue;
   if(FFoundrylinePavementGeometry::InEntrance(S.Pavement,L))return true;
   bool Junction=false;for(const auto& Other:Surfaces)if(Other.Id!=S.Id){const auto Q=Other.Local(P.Position);if(Other.Pavement.Polygons.IsEmpty()?FFoundrylineDriveRect{{0,0},Other.Size}.Contains(Q):FFoundrylinePavementGeometry::Contains(Other.Pavement.Polygons,Q)){Junction=true;break;}}if(Junction)return true;
   TArray<FVector2D> Fallback;if(S.Centerline.IsEmpty())Fallback=FFoundrylinePavementGeometry::RoadPath(S.Pavement.RoadControls,S.Pavement.Spline);const auto& Path=S.Centerline.IsEmpty()?Fallback:S.Centerline;double Best=MAX_dbl;FVector2D Tangent,Nearest;
   for(int I=1;I<Path.Num();++I){const auto A=Path[I-1],B=Path[I];const auto Q=FMath::ClosestPointOnSegment(FVector(L,0),FVector(A,0),FVector(B,0));const double Distance=FVector2D::Distance(L,FVector2D(Q));if(Distance<Best){Best=Distance;Tangent=(B-A).GetSafeNormal();Nearest=FVector2D(Q);}}
   const auto LocalDir=FVector2D(S.Transform.InverseTransformVector(FVector(Dir,0)));const double Dot=FVector2D::DotProduct(Tangent,LocalDir),Side=Tangent.X*(L.Y-Nearest.Y)-Tangent.Y*(L.X-Nearest.X);return S.Pavement.Lanes==1?Dot>.2:Side<0?Dot>.2:Dot<-.2;
  }
  if(S.Function!=TEXT("Road2") && S.Function!=TEXT("Road4")) continue;
  const auto L=S.Local(P.Position);if(!FFoundrylineDriveRect{{0,0},S.Size}.Contains(L)) continue;
  // Junction/driveway crossing envelope: local lane restrictions yield to turn geometry.
  for(const auto& Other:Surfaces) if(Other.Id!=S.Id) {
   const auto Q=Other.Local(P.Position);const float Margin=Semi?1500:600;if(Other.Pavement.Polygons.IsEmpty()?(Q.X>-Margin && Q.Y>-Margin && Q.X<Other.Size.X+Margin && Q.Y<Other.Size.Y+Margin):FFoundrylinePavementGeometry::Contains(Other.Pavement.Polygons,Q,Margin)) return true;
  }
  const auto D=S.Transform.InverseTransformVector(FVector(Dir,0));
  return (L.X>=S.Size.X/2?D.Y>.55:D.Y<-.55);
 }
 return true;
}
FFoundrylineVehiclePose FFoundrylineVehiclePath::Advance(FFoundrylineVehiclePose P,const FFoundrylineVehicleProfile& V,float Distance,float Curvature)
{
 const int32 N=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Distance)/25));const float D=Distance/N;
 for(int32 I=0;I<N;++I) {
  const float Mid=P.Yaw+D*Curvature/2;
  P.Position+=FVector2D(FMath::Cos(Mid),FMath::Sin(Mid))*D;
  if(V.Semi) P.TrailerYaw=FMath::UnwindRadians(P.TrailerYaw+D*(FMath::Sin(P.Yaw-P.TrailerYaw)-V.Hitch*Curvature*FMath::Cos(P.Yaw-P.TrailerYaw))/V.TrailerAxle);
  P.Yaw=FMath::UnwindRadians(P.Yaw+D*Curvature);
 }
 P.Reverse=Distance<0;return P;
}
FFoundrylineVehiclePose FFoundrylineVehiclePath::TargetPose(const FFoundrylineVehicleTarget& Target,const FFoundrylineVehicleProfile& V) {
 auto Pose=Target.Pose;
 if(Target.Loading && !V.Semi) {const FVector2D F(FMath::Cos(Pose.Yaw),FMath::Sin(Pose.Yaw));Pose.Position+=F*(V.Rear+50-Target.LoadingOffset);}
 return Pose;
}
namespace {
 bool FitsTarget(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,const FFoundrylineVehiclePose& P,FGuid Id) {
  if(!Id.IsValid()) return true;const auto* T=M.Targets.FindByPredicate([&](const auto& T){return T.Id==Id;});if(!T) return false;
  if(T->LoadingTier&&T->LoadingTier<3&&V.Semi)return false;
  if(T->Loading) {const FVector2D F(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw)),Trailer(FMath::Cos(P.TrailerYaw),FMath::Sin(P.TrailerYaw)),Goal(FMath::Cos(T->Pose.Yaw),FMath::Sin(T->Pose.Yaw)),Right(-Goal.Y,Goal.X);const auto Tail=V.Semi?P.Position-F*V.Hitch-Trailer*V.TrailerRear:P.Position-F*V.Rear,Expected=FFoundrylineVehiclePath::TargetPose(*T,V).Position-Goal*(V.Semi?V.Hitch+V.TrailerRear:V.Rear),D=Tail-Expected;return FMath::Abs(FVector2D::DotProduct(D,Right))<=T->Bay.Max.X-(V.Semi?V.TrailerWidth:V.Width)/2-V.Clearance && FMath::Abs(FVector2D::DotProduct(D,Goal))<=30 && FMath::Abs(FMath::FindDeltaAngleRadians(V.Semi?P.TrailerYaw:P.Yaw,T->Pose.Yaw))<.035f;}
  const auto* S=M.Surfaces.FindByPredicate([&](const auto& S){return S.Id==T->SurfaceId;});if(!S) return false;
  auto Box=[&](FVector2D Origin,float Yaw,float Front,float Rear,float Width){const FVector2D F(FMath::Cos(Yaw),FMath::Sin(Yaw)),R(-F.Y,F.X);for(float X:{Front+V.Clearance,-Rear-V.Clearance}) for(float Y:{Width/2+V.Clearance,-Width/2-V.Clearance}) {const auto L=S->Local(Origin+F*X+R*Y);if(T->Footprint.IsEmpty()?!T->Bay.Contains(L):!FFoundrylinePavementGeometry::Contains({FFoundrylinePavementPolygon{T->Footprint,{}}},L))return false;}return true;};
  if(!Box(P.Position,P.Yaw,V.Front,V.Rear,V.Width)) return false;
  const FVector2D F(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw));return !V.Semi || Box(P.Position-F*V.Hitch,P.TrailerYaw,V.TrailerFront,V.TrailerRear,V.TrailerWidth);
 }
 struct FCollision {
  const FFoundrylineDrivingMap& Map;const FFoundrylineVehicleProfile& Vehicle;bool Zones;FGuid Target;
  TMap<FIntPoint,uint8> Cells;
  uint8 Cell(FIntPoint K) {
   if(const auto* B=Cells.Find(K)) return *B;
   int32 Count=0;for(const auto Offset:{FVector2D(1,1),FVector2D(49,1),FVector2D(1,49),FVector2D(49,49),FVector2D(25,25)}) Count+=Map.PointAllowed(FVector2D(K.X*50,K.Y*50)+Offset,Vehicle.IsFreight(),Zones,Target)?1:0;
   const uint8 State=Count==5?1:Count==0?0:2;Cells.Add(K,State);return State;
  }
  bool BoundaryCell(const TArray<FVector2D>& Body,int32 X,int32 Y) {
   TArray<FVector2D> Polygon=Body;
   for(int32 Edge=0;Edge<4 && !Polygon.IsEmpty();++Edge) {
    const bool Horizontal=Edge<2;const double Boundary=(Horizontal?X:Y)*50+((Edge%2)?50:0);
    auto Value=[&](FVector2D P){return Horizontal?P.X:P.Y;};
    auto Inside=[&](FVector2D P){return Edge%2?Value(P)<=Boundary:Value(P)>=Boundary;};
    TArray<FVector2D> Next;
    auto Previous=Polygon.Last();bool WasInside=Inside(Previous);
    for(const auto& P:Polygon) {const bool In=Inside(P);if(In!=WasInside) {const double T=(Boundary-Value(Previous))/(Value(P)-Value(Previous));Next.Add(Previous+(P-Previous)*T);}if(In) Next.Add(P);Previous=P;WasInside=In;}
    Polygon=MoveTemp(Next);
   }
   if(Polygon.IsEmpty()) return true;
   FVector2D Center(0,0);
   for(int32 I=0;I<Polygon.Num();++I) {Center+=Polygon[I];const auto A=Polygon[I],B=Polygon[(I+1)%Polygon.Num()];const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,B)/12.5));for(int32 J=0;J<=N;++J) if(!Map.PointAllowed(FMath::Lerp(A,B,double(J)/N),Vehicle.IsFreight(),Zones,Target)) return false;}
   return Map.PointAllowed(Center/Polygon.Num(),Vehicle.IsFreight(),Zones,Target);
  }

  bool Box(FVector2D Origin,float Yaw,float Front,float Rear,float Width) {
   const FVector2D F(FMath::Cos(Yaw),FMath::Sin(Yaw)),R(-F.Y,F.X);const float Half=Width/2+Vehicle.Clearance;
   const auto Center=Origin+F*((Front-Rear)/2);const float Length=(Front+Rear)/2+Vehicle.Clearance;
   const FVector2D Ext(FMath::Abs(F.X)*Length+FMath::Abs(R.X)*Half,FMath::Abs(F.Y)*Length+FMath::Abs(R.Y)*Half);
   const TArray<FVector2D> Body={Center+F*Length+R*Half,Center-F*Length+R*Half,Center-F*Length-R*Half,Center+F*Length-R*Half};
   for(int32 X=FMath::FloorToInt((Center.X-Ext.X)/50);X<=FMath::FloorToInt((Center.X+Ext.X)/50);++X)
   for(int32 Y=FMath::FloorToInt((Center.Y-Ext.Y)/50);Y<=FMath::FloorToInt((Center.Y+Ext.Y)/50);++Y) {
    const auto Delta=FVector2D(X*50+25,Y*50+25)-Center;
    if(FMath::Abs(FVector2D::DotProduct(Delta,F))<=Length+25*(FMath::Abs(F.X)+FMath::Abs(F.Y)) && FMath::Abs(FVector2D::DotProduct(Delta,R))<=Half+25*(FMath::Abs(R.X)+FMath::Abs(R.Y)) && Cell({X,Y})!=1 && !BoundaryCell(Body,X,Y)) return false;
   }
   return true;
  }
  bool Pose(const FFoundrylineVehiclePose& P) {
   if(Vehicle.Semi && FMath::Abs(FMath::FindDeltaAngleRadians(P.Yaw,P.TrailerYaw))>FMath::DegreesToRadians(Vehicle.MaxArticulationDegrees)) return false;
   if(!Map.LaneAllowed(P,Vehicle.IsFreight()) || !Box(P.Position,P.Yaw,Vehicle.Front,Vehicle.Rear,Vehicle.Width)) return false;
   const auto Hitch=P.Position-FVector2D(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw))*Vehicle.Hitch;
   return !Vehicle.Semi || Box(Hitch,P.TrailerYaw,Vehicle.TrailerFront,Vehicle.TrailerRear,Vehicle.TrailerWidth);
  }
 };
 struct FNode {FFoundrylineVehiclePose Pose;float Cost=0;int32 Parent=-1;float Curvature=0,Distance=0;};
 struct FOpen {float Score;int32 Index;bool operator<(const FOpen& B) const {return Score>B.Score;}};
 FString Key(const FFoundrylineVehiclePose& P,bool Semi) {return FString::Printf(TEXT("%d,%d,%d,%d,%d"),FMath::RoundToInt(P.Position.X/50),FMath::RoundToInt(P.Position.Y/50),FMath::RoundToInt(FMath::UnwindDegrees(FMath::RadiansToDegrees(P.Yaw))/5.625f),Semi?FMath::RoundToInt(FMath::RadiansToDegrees(FMath::UnwindRadians(P.TrailerYaw))/10):0,P.Reverse);}
}
bool FFoundrylineVehiclePath::PoseClear(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,const FFoundrylineVehiclePose& P,bool Zones,FGuid Target) {FCollision C{M,V,Zones,Target,{}};return C.Pose(P);}
static FFoundrylineVehicleRoute SearchRoute(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target,int32 MaxExpanded,double Deadline=DBL_MAX)
{
 FFoundrylineVehicleRoute Out;Out.Revision=M.Revision;const double Began=FPlatformTime::Seconds();
 FCollision Collision{M,V,Zones,Target,{}};
 if(!Collision.Pose(Start) || !Collision.Pose(Goal)) {Out.Reason=TEXT("Entry or destination lacks vehicle/trailer clearance, or a truck zone");return Out;}
 // Exact straight connector, including backing into aligned parking/loading targets.
 const FVector2D Forward(FMath::Cos(Start.Yaw),FMath::Sin(Start.Yaw)),Right(-Forward.Y,Forward.X),Delta=Goal.Position-Start.Position;
 const float Along=FVector2D::DotProduct(Delta,Forward);
 if(FMath::Abs(FVector2D::DotProduct(Delta,Right))<.1f && FMath::Abs(FMath::FindDeltaAngleRadians(Start.Yaw,Goal.Yaw))<.0001f && (!V.Semi || FMath::Abs(FMath::FindDeltaAngleRadians(Start.TrailerYaw,Goal.TrailerYaw))<.0001f)) {
  auto P=Start;TArray<FFoundrylineVehiclePose> Poses{P};bool Clear=true;const int32 Count=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Along)/25));
  for(int32 I=0;I<Count;++I) {P=FFoundrylineVehiclePath::Advance(P,V,Along/Count,0);if(!Collision.Pose(P) || (Along<0 && !M.ReverseAllowed(P.Position))) {Clear=false;break;}Poses.Add(P);}
  if(Clear && FitsTarget(M,V,P,Target)) {Out.Success=true;Out.Poses=MoveTemp(Poses);Out.Reason=TEXT("Route clear / swept body and trailer checked");Out.Milliseconds=(FPlatformTime::Seconds()-Began)*1000;return Out;}
 }
 // Holonomic cost-to-go respects disconnected paving and parking entrances.
 // It guides the kinematic search; every accepted motion still checks full bodies.
 struct FGridOpen {float Cost;FIntPoint Point;bool operator<(const FGridOpen& B) const{return Cost>B.Cost;}};
 TMap<FIntPoint,float> GridDistance;TMap<FIntPoint,bool> GridValid;std::priority_queue<FGridOpen> GridOpen;
 FIntPoint MinCell(MAX_int32,MAX_int32),MaxCell(MIN_int32,MIN_int32);
 for(const auto& S:M.Surfaces) for(float X:{0.f,float(S.Size.X)}) for(float Y:{0.f,float(S.Size.Y)}) {const auto P=S.World({X,Y});MinCell.X=FMath::Min(MinCell.X,FMath::FloorToInt32(P.X/100)-1);MinCell.Y=FMath::Min(MinCell.Y,FMath::FloorToInt32(P.Y/100)-1);MaxCell.X=FMath::Max(MaxCell.X,FMath::CeilToInt32(P.X/100)+1);MaxCell.Y=FMath::Max(MaxCell.Y,FMath::CeilToInt32(P.Y/100)+1);}
 auto CanVisit=[&](FIntPoint P){if(P.X<MinCell.X || P.Y<MinCell.Y || P.X>MaxCell.X || P.Y>MaxCell.Y) return false;if(const auto* B=GridValid.Find(P)) return *B;const bool B=M.PointAllowed(FVector2D(P.X*100,P.Y*100),V.IsFreight(),Zones,Target);GridValid.Add(P,B);return B;};
 const FIntPoint GoalCell(FMath::RoundToInt32(Goal.Position.X/100),FMath::RoundToInt32(Goal.Position.Y/100));
 GridDistance.Add(GoalCell,0);GridOpen.push({0,GoalCell});int32 GridExpanded=0;
 while(!GridOpen.empty() && GridExpanded++<40000 && FPlatformTime::Seconds()<Deadline) {const auto P=GridOpen.top();GridOpen.pop();if(P.Cost>GridDistance.FindChecked(P.Point)) continue;
  for(int32 X=-1;X<=1;++X) for(int32 Y=-1;Y<=1;++Y) {if(X==0 && Y==0) continue;const FIntPoint N=P.Point+FIntPoint(X,Y);if(!CanVisit(N) || (X && Y && (!CanVisit(P.Point+FIntPoint(X,0)) || !CanVisit(P.Point+FIntPoint(0,Y))))) continue;const float Cost=P.Cost+(X && Y?141.421f:100.f);if(const auto* Previous=GridDistance.Find(N);Previous && *Previous<=Cost) continue;GridDistance.Add(N,Cost);GridOpen.push({Cost,N});}
 }
 auto Heuristic=[&](const FFoundrylineVehiclePose& P){const float Straight=FVector2D::Distance(P.Position,Goal.Position);const auto* Distance=GridDistance.Find({FMath::RoundToInt32(P.Position.X/100),FMath::RoundToInt32(P.Position.Y/100)});return FMath::Max(Straight,Distance?*Distance:Straight+10000)+FMath::Abs(FMath::FindDeltaAngleRadians(P.Yaw,Goal.Yaw))*V.TurnRadius*.25f;};
 TArray<FNode> Nodes;Nodes.Add({Start,0,-1});TMap<FString,float> Best;Best.Add(Key(Start,V.Semi),0);std::priority_queue<FOpen> Open;Open.push({float(FVector2D::Distance(Start.Position,Goal.Position)),0});
 const float Step=V.Semi?150:75;int32 Found=-1;
 while(!Open.empty() && Out.Expanded<MaxExpanded && FPlatformTime::Seconds()-Began<3.0 && FPlatformTime::Seconds()<Deadline) {
  const auto Item=Open.top();Open.pop();const auto Current=Nodes[Item.Index];if(const auto* Known=Best.Find(Key(Current.Pose,V.Semi));Known && Current.Cost>*Known) continue;++Out.Expanded;
  const float Distance=FVector2D::Distance(Current.Pose.Position,Goal.Position);
  if(Distance<=55 && FMath::Abs(FMath::FindDeltaAngleRadians(Current.Pose.Yaw,Goal.Yaw))<FMath::DegreesToRadians(6.f) && (!V.Semi || FMath::Abs(FMath::FindDeltaAngleRadians(Current.Pose.TrailerYaw,Goal.TrailerYaw))<FMath::DegreesToRadians(Target.IsValid()?6.f:15.f)) && FitsTarget(M,V,Current.Pose,Target)) {Found=Item.Index;break;}
  for(int32 Direction:{1,-1}) {
   if(Direction<0 && !M.ReverseAllowed(Current.Pose.Position)) continue;
   for(float Steering:{-1.f,-.5f,0.f,.5f,1.f}) {
    auto Next=Current.Pose;bool Clear=true;
    for(int32 I=0;I<3;++I) {Next=FFoundrylineVehiclePath::Advance(Next,V,Direction*Step/3,Steering/V.TurnRadius);if(!Collision.Pose(Next) || (Direction<0 && !M.ReverseAllowed(Next.Position))) {Clear=false;break;}}
    if(!Clear) continue;
    const float Cost=Current.Cost+Step*(Direction<0?1.15f:1.f)+FMath::Abs(Steering)*10+(Next.Reverse!=Current.Pose.Reverse?100:0);
    const auto K=Key(Next,V.Semi);if(const auto* Previous=Best.Find(K);Previous && *Previous<=Cost) continue;
    Best.Add(K,Cost);const int32 Index=Nodes.Add({Next,Cost,Item.Index,Steering/V.TurnRadius,Direction*Step});
    Open.push({Cost+Heuristic(Next)*1.5f,Index});
   }
  }
 }
 if(Found>=0) {
  TArray<int32> Chain;for(int32 I=Found;I>=0;I=Nodes[I].Parent) Chain.Add(I);Algo::Reverse(Chain);Out.Poses.Add(Start);bool Clear=true;
  for(int32 I=1;I<Chain.Num();++I) {const auto& Edge=Nodes[Chain[I]];auto P=Nodes[Chain[I-1]].Pose;const int32 Steps=FMath::CeilToInt(FMath::Abs(Edge.Distance)/25);for(int32 J=0;J<Steps;++J) {P=FFoundrylineVehiclePath::Advance(P,V,Edge.Distance/Steps,Edge.Curvature);if(!Collision.Pose(P)) Clear=false;Out.Poses.Add(P);}}
  Out.Success=Clear;Out.Reason=Clear?TEXT("Route clear / swept body and trailer checked"):TEXT("The turn sweep is too tight. Widen this connection");
 }
 else Out.Reason=TEXT("No route within search budget. Check connections, zones, aisle width and turning space");
 Out.Milliseconds=(FPlatformTime::Seconds()-Began)*1000;return Out;
}

namespace {
 FFoundrylineVehicleRoute ConnectLines(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target) {
  FFoundrylineVehicleRoute Out;Out.Revision=M.Revision;Out.Reason=TEXT("No clear tangent connection");const double Began=FPlatformTime::Seconds();
  const FVector2D A(FMath::Cos(Start.Yaw),FMath::Sin(Start.Yaw)),B(FMath::Cos(Goal.Yaw),FMath::Sin(Goal.Yaw));const double Cross=A.X*B.Y-A.Y*B.X;
  const float Angle=FMath::FindDeltaAngleRadians(Start.Yaw,Goal.Yaw);if(FMath::Abs(Cross)<.2 || FMath::Abs(Angle)>2.6) return Out;
  const auto Delta=Goal.Position-Start.Position;const double Distance=(Delta.X*B.Y-Delta.Y*B.X)/Cross;const auto Intersection=Start.Position+A*Distance;
  const double Remaining=FVector2D::DotProduct(Goal.Position-Intersection,B);
  for(float Scale:{1.f,1.2f,1.4f,1.6f}) {
   const float Radius=V.TurnRadius*Scale,Tangent=Radius*FMath::Tan(FMath::Abs(Angle)/2);
   if(Distance<Tangent || Remaining<Tangent) continue;
   FCollision Collision{M,V,Zones,Target,{}};auto P=Start;TArray<FFoundrylineVehiclePose> Poses{P};bool Clear=true;
   auto Leg=[&](float Length,float Curvature){const int32 Count=FMath::Max(1,FMath::CeilToInt(Length/25));for(int32 I=0;I<Count && Clear;++I) {P=FFoundrylineVehiclePath::Advance(P,V,Length/Count,Curvature);if(!Collision.Pose(P)) Clear=false;else Poses.Add(P);}};
   Leg(Distance-Tangent,0);Leg(Radius*FMath::Abs(Angle),FMath::Sign(Angle)/Radius);Leg(Remaining-Tangent,0);
   if(Clear && FVector2D::Distance(P.Position,Goal.Position)<10 && FitsTarget(M,V,P,Target)) {Out.Success=true;Out.Poses=MoveTemp(Poses);Out.Reason=TEXT("Route clear / continuous steering and trailer sweep");Out.Milliseconds=(FPlatformTime::Seconds()-Began)*1000;return Out;}
  }
  return Out;
 }
 FFoundrylineVehicleRoute ShiftLine(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target) {
  FFoundrylineVehicleRoute Out;Out.Revision=M.Revision;const FVector2D F(FMath::Cos(Start.Yaw),FMath::Sin(Start.Yaw)),L(-F.Y,F.X),Delta=Goal.Position-Start.Position;
  const float Forward=FVector2D::DotProduct(Delta,F),Side=FVector2D::DotProduct(Delta,L);if(Forward<=0 || FMath::Abs(Side)<1 || FMath::Abs(FMath::FindDeltaAngleRadians(Start.Yaw,Goal.Yaw))>.01) return Out;
  const float Radius=(Forward*Forward+Side*Side)/(4*FMath::Abs(Side)),Angle=2*FMath::Atan(FMath::Abs(Side)/Forward);if(Radius<V.TurnRadius) return Out;
  auto P=Start;TArray<FFoundrylineVehiclePose> Poses{P};FCollision Collision{M,V,Zones,Target,{}};
  for(float Sign:{FMath::Sign(Side),-FMath::Sign(Side)}) {const int32 Count=FMath::CeilToInt(Radius*Angle/25);for(int32 I=0;I<Count;++I) {P=FFoundrylineVehiclePath::Advance(P,V,Radius*Angle/Count,Sign/Radius);if(!Collision.Pose(P)) return Out;Poses.Add(P);}}
  if(FVector2D::Distance(P.Position,Goal.Position)<10 && FitsTarget(M,V,P,Target)) {Out.Success=true;Out.Poses=MoveTemp(Poses);Out.Reason=TEXT("Route clear / wide turn and lane merge checked");}return Out;
 }
 FFoundrylineVehicleRoute ReverseDock(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target) {
  FFoundrylineVehicleRoute Out;Out.Revision=M.Revision;Out.Reason=TEXT("Backing manoeuvre needs more clear space");
  const FVector2D Forward(FMath::Cos(Goal.Yaw),FMath::Sin(Goal.Yaw)),Right(-Forward.Y,Forward.X);
  const auto GoalAxle=Goal.Position-Forward*(V.Hitch+V.TrailerAxle);
  for(float LateralScale:{1200.f,1800.f,2600.f,3600.f}) for(float HeadingGain:{.75f,1.f,1.6f,2.2f}) {
   FCollision Collision{M,V,Zones,Target,{}};auto P=Start;TArray<FFoundrylineVehiclePose> Poses{P};bool Clear=true;
   for(int32 I=0;I<260;++I) {
    const float Remaining=FVector2D::DotProduct(P.Position-Goal.Position,Forward);if(Remaining<12.5f) break;
    const FVector2D Tractor(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw)),Trailer(FMath::Cos(P.TrailerYaw),FMath::Sin(P.TrailerYaw));const auto Axle=P.Position-Tractor*V.Hitch-Trailer*V.TrailerAxle;
    const float Lateral=FVector2D::DotProduct(Axle-GoalAxle,Right);
    const float DesiredTrailer=Goal.Yaw+FMath::Clamp(FMath::Atan(Lateral/FMath::Max(400.f,FMath::Min(LateralScale,Remaining))),-.4f,.4f);
    const float DesiredTractor=P.TrailerYaw+FMath::Clamp(HeadingGain*FMath::FindDeltaAngleRadians(DesiredTrailer,P.TrailerYaw),-.55f,.55f);
    const float Curvature=FMath::Clamp(-FMath::FindDeltaAngleRadians(P.Yaw,DesiredTractor)/350,-1/V.TurnRadius,1/V.TurnRadius);
    P=FFoundrylineVehiclePath::Advance(P,V,-FMath::Min(25.f,Remaining),Curvature);
    if(!Collision.Pose(P) || !M.ReverseAllowed(P.Position)) {Clear=false;break;}Poses.Add(P);
   }
   const FVector2D F(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw)),T(FMath::Cos(P.TrailerYaw),FMath::Sin(P.TrailerYaw));const auto Tail=P.Position-F*V.Hitch-T*V.TrailerRear;const auto GoalTail=Goal.Position-Forward*(V.Hitch+V.TrailerRear);
   if(Clear && FitsTarget(M,V,P,Target) && FVector2D::Distance(P.Position,Goal.Position)<55 && FMath::Abs(FMath::FindDeltaAngleRadians(P.Yaw,Goal.Yaw))<.08 && FMath::Abs(FMath::FindDeltaAngleRadians(P.TrailerYaw,Goal.TrailerYaw))<.035 && FMath::Abs(FVector2D::DotProduct(Tail-GoalTail,Right))<25) {Out.Success=true;Out.Poses=MoveTemp(Poses);Out.Reason=TEXT("Route clear / articulated backing checked");return Out;}
  }
  return Out;
 }
 void Append(FFoundrylineVehicleRoute& A,const FFoundrylineVehicleRoute& B) {for(int32 I=1;I<B.Poses.Num();++I) A.Poses.Add(B.Poses[I]);A.Expanded+=B.Expanded;A.Milliseconds+=B.Milliseconds;A.Success=B.Success;A.Reason=B.Reason;}
}
static FFoundrylineVehicleRoute PlanRoute(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target,int32 MaxExpanded,double Deadline)
{
 auto Search=[&](const FFoundrylineDrivingMap& Map,const FFoundrylineVehicleProfile& Profile,FFoundrylineVehiclePose A,FFoundrylineVehiclePose B,bool ZonePolicy,FGuid Id,int32 Budget){return SearchRoute(Map,Profile,A,B,ZonePolicy,Id,Budget,Deadline);};
 auto Result=Search(M,V,Start,Goal,Zones,Target,MaxExpanded);if(Result.Success || !V.IsFreight() || Result.Expanded==0) return Result;
 const auto* Destination=M.Targets.FindByPredicate([&](const auto& T){return T.Id==Target;});
 // Incoming and departing articulated routes split at a full-body-clear access pose.
 // Each segment starts at the exact final pose of its predecessor; nothing teleports.
 for(const auto& Access:M.Surfaces) if(FPlatformTime::Seconds()<Deadline && Access.Function==TEXT("TruckAccess") && Access.Size.Y>=1600) {
  if(Destination && Destination->Semi) {
   FFoundrylineVehiclePose Gate;Gate.Position=Access.World({Access.Size.X/2,Access.Size.Y+1000});Gate.Yaw=FMath::DegreesToRadians(Access.Transform.Rotator().Yaw+90);Gate.TrailerYaw=Gate.Yaw;
   if(!FFoundrylineVehiclePath::PoseClear(M,V,Gate,Zones,Target)) continue;
   auto Approach=ConnectLines(M,V,Start,Gate,Zones,{});if(!Approach.Success) Approach=Search(M,V,Start,Gate,Zones,{},MaxExpanded);if(!Approach.Success) continue;
   for(float Distance:{3900.f,3700.f,3500.f,3000.f,2500.f,2000.f}) {
    if(FPlatformTime::Seconds()>=Deadline) break;
    auto Stage=Goal;Stage.Position+=FVector2D(FMath::Cos(Goal.Yaw),FMath::Sin(Goal.Yaw))*Distance;
    if(!FFoundrylineVehiclePath::PoseClear(M,V,Stage,Zones,Target)) continue;
    auto Manoeuvre=ConnectLines(M,V,Approach.Poses.Last(),Stage,Zones,{});if(!Manoeuvre.Success) Manoeuvre=Search(M,V,Approach.Poses.Last(),Stage,Zones,{},MaxExpanded);if(!Manoeuvre.Success) continue;
    auto Back=V.Semi?ReverseDock(M,V,Manoeuvre.Poses.Last(),Goal,Zones,Target):Search(M,V,Manoeuvre.Poses.Last(),Goal,Zones,Target,MaxExpanded);
    if(!Back.Success) Back=Search(M,V,Manoeuvre.Poses.Last(),Goal,Zones,Target,MaxExpanded);
    if(Back.Success) {Append(Approach,Manoeuvre);Append(Approach,Back);return Approach;}
   }
  } else if(M.ReverseAllowed(Start.Position)) {
   const FVector2D StartForward(FMath::Cos(Start.Yaw),FMath::Sin(Start.Yaw));const float ExitYaw=FMath::DegreesToRadians(Access.Transform.Rotator().Yaw-90);const float Angle=FMath::FindDeltaAngleRadians(Start.Yaw,ExitYaw);
   if(FMath::Abs(FMath::Abs(Angle)-PI/2)<.1) for(float Radius:{V.TurnRadius,V.TurnRadius*1.1f,V.TurnRadius*1.2f}) {
    const auto AccessPoint=Access.World({Access.Size.X/2,Access.Size.Y});const float Distance=FVector2D::DotProduct(AccessPoint-Start.Position,StartForward)+Radius;if(Distance<0) continue;
    if(FPlatformTime::Seconds()>=Deadline) break;
    auto Stage=Start;Stage.Position+=StartForward*Distance;
    auto Compound=Search(M,V,Start,Stage,Zones,{},MaxExpanded);if(!Compound.Success) continue;
    FCollision Collision{M,V,Zones,Target,{}};auto P=Compound.Poses.Last();bool Clear=true;
    for(float Sign:{FMath::Sign(Angle),FMath::Sign(Angle),-FMath::Sign(Angle)}) {const int32 Count=FMath::CeilToInt(Radius*PI/2/25);for(int32 I=0;I<Count && Clear;++I) {P=FFoundrylineVehiclePath::Advance(P,V,Radius*PI/2/Count,Sign/Radius);if(!Collision.Pose(P)) Clear=false;else Compound.Poses.Add(P);}}
    if(Clear) {
     const FVector2D RoadForward(FMath::Cos(Goal.Yaw),FMath::Sin(Goal.Yaw)),RoadLeft(-RoadForward.Y,RoadForward.X);
     auto Wide=Goal;Wide.Position-=RoadForward*2800;Wide.Position+=RoadLeft*400;
     auto TurnWide=ConnectLines(M,V,P,Wide,Zones,{});
     if(TurnWide.Success) {auto Merge=ShiftLine(M,V,TurnWide.Poses.Last(),Goal,Zones,Target);if(Merge.Success) {Append(Compound,TurnWide);Append(Compound,Merge);return Compound;}}
     auto Exit=ConnectLines(M,V,P,Goal,Zones,Target);if(!Exit.Success) Exit=Search(M,V,P,Goal,Zones,Target,MaxExpanded);if(Exit.Success) {Append(Compound,Exit);return Compound;}}
   }

   FFoundrylineVehiclePose Gate;Gate.Position=Access.World({Access.Size.X/2,Access.Size.Y/2});Gate.Yaw=FMath::DegreesToRadians(Access.Transform.Rotator().Yaw-90);Gate.TrailerYaw=Gate.Yaw;
   if(!FFoundrylineVehiclePath::PoseClear(M,V,Gate,Zones,Target)) continue;
   for(float Distance:{1500.f,2000.f,2500.f}) {
    if(FPlatformTime::Seconds()>=Deadline) break;
    auto Stage=Start;Stage.Position+=FVector2D(FMath::Cos(Start.Yaw),FMath::Sin(Start.Yaw))*Distance;
    if(!FFoundrylineVehiclePath::PoseClear(M,V,Stage,Zones,Target)) continue;
    auto Forward=Search(M,V,Start,Stage,Zones,{},MaxExpanded);if(!Forward.Success) continue;
    auto Turn=Search(M,V,Forward.Poses.Last(),Gate,Zones,{},MaxExpanded);if(!Turn.Success) continue;
    auto Exit=Search(M,V,Turn.Poses.Last(),Goal,Zones,Target,MaxExpanded);if(Exit.Success) {Append(Forward,Turn);Append(Forward,Exit);return Forward;}
   }
  }
 }
 return Result;
}

FFoundrylineVehicleRoute FFoundrylineVehiclePath::Find(const FFoundrylineDrivingMap& M,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones,FGuid Target,int32 MaxExpanded)
{
 const double Began=FPlatformTime::Seconds();auto Result=PlanRoute(M,V,Start,Goal,Zones,Target,MaxExpanded,Began+12.0);
 Result.Milliseconds=(FPlatformTime::Seconds()-Began)*1000;return Result;
}
