#include "Logistics/FoundrylineLogisticsTrip.h"
#include "Construction/FoundrylinePavementGeometry.h"
TArray<FFoundrylineLogisticsGate> FFoundrylineLogisticsTrip::Gates(const FFoundrylineDrivingMap& M,bool Semi,bool SmallTruck)
{
 TArray<FFoundrylineLogisticsGate> Out;const auto V=Semi?(SmallTruck?FFoundrylineVehicleProfile::BoxTruck():FFoundrylineVehicleProfile::Truck()):FFoundrylineVehicleProfile::Car();
 for(const auto& S:M.Surfaces) {
  if(S.Public || (S.Function!=TEXT("Pavement")&&(Semi?S.Function!=TEXT("TruckAccess"):S.Function!=TEXT("Driveway")))) continue;
  for(const auto& Road:M.Surfaces) if(Road.Public) {
   FVector2D Min(DBL_MAX,DBL_MAX),Max(-DBL_MAX,-DBL_MAX);
   for(float X:{0.f,float(Road.Size.X)}) for(float Y:{0.f,float(Road.Size.Y)}) {auto P=S.Local(Road.World({X,Y}));Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);}
   FVector2D Center(0,0);float Angle=0;bool Touch=false;
   const double X0=FMath::Max(0.,Min.X),X1=FMath::Min(S.Size.X,Max.X),Y0=FMath::Max(0.,Min.Y),Y1=FMath::Min(S.Size.Y,Max.Y),Width=V.Width+2*V.Clearance;
   if(X1-X0>=Width && FMath::Abs(Max.Y)<1) {Center={(X0+X1)/2,0};Angle=90;Touch=true;}
   else if(X1-X0>=Width && FMath::Abs(Min.Y-S.Size.Y)<1) {Center={(X0+X1)/2,S.Size.Y};Angle=-90;Touch=true;}
   else if(Y1-Y0>=Width && FMath::Abs(Max.X)<1) {Center={0,(Y0+Y1)/2};Angle=0;Touch=true;}
   else if(Y1-Y0>=Width && FMath::Abs(Min.X-S.Size.X)<1) {Center={S.Size.X,(Y0+Y1)/2};Angle=180;Touch=true;}
   if(S.Function==TEXT("Pavement")){
    Touch=false;double BestSpan=0;
    for(const auto& Poly:S.Pavement.Polygons)for(int I=0;I<Poly.Outer.Num();++I){const auto A=Poly.Outer[I],B=Poly.Outer[(I+1)%Poly.Outer.Num()],RA=Road.Local(S.World(A)),RB=Road.Local(S.World(B));
     const bool AlongY=FMath::Abs(RA.X-RB.X)<1;const double Cross=AlongY?(RA.X+RB.X)*.5:(RA.Y+RB.Y)*.5,Size=AlongY?Road.Size.X:Road.Size.Y;
     if(FMath::Abs(AlongY?RA.Y-RB.Y:RA.X-RB.X)<1||FMath::Min(FMath::Abs(Cross),FMath::Abs(Cross-Size))>1)continue;
     const double Low=FMath::Max(0.,FMath::Min(AlongY?RA.Y:RA.X,AlongY?RB.Y:RB.X)),High=FMath::Min(AlongY?Road.Size.Y:Road.Size.X,FMath::Max(AlongY?RA.Y:RA.X,AlongY?RB.Y:RB.X));if(High-Low<V.Width+2*V.Clearance||High-Low<=BestSpan)continue;
     const auto Middle=AlongY?FVector2D(Cross,(Low+High)/2):FVector2D((Low+High)/2,Cross);Center=S.Local(Road.World(Middle));const auto Edge=(B-A).GetSafeNormal();const auto Inside=FVector2D(-Edge.Y,Edge.X);Angle=FMath::RadiansToDegrees(FMath::Atan2(Inside.Y,Inside.X));BestSpan=High-Low;Touch=true;
    }
   }
   if(!Touch) continue;FFoundrylineLogisticsGate G;G.SurfaceId=S.Id;G.Portal=S.World(Center);G.InwardYaw=FMath::DegreesToRadians(S.Transform.Rotator().Yaw+Angle);
   if(S.Pavement.Lanes==1&&!S.Pavement.RoadControls.IsEmpty()){const bool Start=FVector2D::Distance(Center,S.Pavement.RoadControls[0])<FVector2D::Distance(Center,S.Pavement.RoadControls.Last());G.CanEnter=Start;G.CanExit=!Start;}
   auto StreetPose=[&](bool Incoming,FFoundrylineVehiclePose& Pose) {
    Pose.Yaw=FMath::UnwindRadians(G.InwardYaw+(Incoming?-PI/2:PI/2));Pose.TrailerYaw=Pose.Yaw;
    const FVector2D F(FMath::Cos(Pose.Yaw),FMath::Sin(Pose.Yaw));const auto LocalDirection=Road.Transform.InverseTransformVector(FVector(F,0));auto Local=Road.Local(G.Portal);
    const bool Positive=LocalDirection.Y>0;Local.X=Road.Function==TEXT("Road4")?(Positive?Road.Size.X-175:175):Road.Size.X*(Positive?.75:.25);
    const auto Base=Road.World(Local);
    for(float Distance:{7000.f,6000.f,5000.f,4000.f,3000.f,2600.f,2000.f,1000.f}) {
     if((!Incoming && Distance>4000) || (Incoming && !Semi && Distance>2600)) continue;Pose.Position=Base+F*(Incoming?-Distance:Distance);
     const bool Street=M.Surfaces.ContainsByPredicate([&](const auto& R){return R.Public && FFoundrylineDriveRect{{0,0},R.Size}.Contains(R.Local(Pose.Position));});
     if(Street && FFoundrylineVehiclePath::PoseClear(M,V,Pose,false)) return true;
    }return false;
   };
   if((!G.CanEnter||StreetPose(true,G.Entry)) && (!G.CanExit||StreetPose(false,G.Exit)) && !Out.ContainsByPredicate([&](const auto& E){return FVector2D::Distance(E.Portal,G.Portal)<100;})) Out.Add(G);
  }
 }
 Out.Sort([](const auto& A,const auto& B){return A.Portal.X==B.Portal.X?A.Portal.Y<B.Portal.Y:A.Portal.X<B.Portal.X;});
 for(int32 I=0;I<Out.Num();++I) Out[I].Label=FString::Printf(TEXT("%s %02d"),Semi?(Out[I].CanEnter&&Out[I].CanExit?TEXT("TRUCK GATE"):Out[I].CanEnter?TEXT("TRUCK ENTRY"):TEXT("TRUCK EXIT")):(Out[I].CanEnter&&Out[I].CanExit?TEXT("CAR ENTRANCE"):Out[I].CanEnter?TEXT("CAR ENTRY"):TEXT("CAR EXIT")),I+1);return Out;
}
FVector2D FFoundrylineLogisticsTrip::TargetMarker(const FFoundrylineVehicleTarget& T) {return T.Loading?T.Pose.Position-FVector2D(FMath::Cos(T.Pose.Yaw),FMath::Sin(T.Pose.Yaw))*T.LoadingOffset:T.Pose.Position;}
FString FFoundrylineLogisticsTrip::TargetLabel(const FFoundrylineDrivingMap& M,FGuid Id) {
 const auto* T=M.Targets.FindByPredicate([&](const auto& X){return X.Id==Id;});if(!T) return TEXT("Choose a destination");
 TArray<const FFoundrylineVehicleTarget*> Same;for(const auto& X:M.Targets) if(X.Semi==T->Semi && X.Loading==T->Loading && (X.Footprint.IsEmpty()==T->Footprint.IsEmpty()||!T->Semi)) Same.Add(&X);
 Same.Sort([](const auto& A,const auto& B){const auto P=TargetMarker(A),Q=TargetMarker(B);return P.Y==Q.Y?P.X<Q.X:P.Y<Q.Y;});
 return FString::Printf(TEXT("%s %02d"),T->Loading?TEXT("LOADING DOOR"):T->Semi?(!T->Footprint.IsEmpty()?TEXT("LOADING BAY"):TEXT("STAGING BAY")):TEXT("PARKING BAY"),Same.IndexOfByKey(T)+1);
}
bool FFoundrylineLogisticsTrip::Select(const FFoundrylineDrivingMap& M,FName Step,FVector2D Point,FString& Message) {
 if(Step==TEXT("TripEntrance") || Step==TEXT("TripExit")) {
  auto Options=Gates(M,Semi,SmallTruck);const FFoundrylineLogisticsGate* Best=nullptr;double Distance=FMath::Square(1400.);
  for(const auto& G:Options) if(Step==TEXT("TripEntrance")?G.CanEnter:G.CanExit) if(const double D=FVector2D::DistSquared(Point,G.Portal);D<Distance) {Distance=D;Best=&G;}
  if(!Best) {Message=Semi?TEXT("Click an allowed truck gate where pavement meets the street"):TEXT("Click a blue car entrance where a driveway meets the street");return false;}
  if(Step==TEXT("TripEntrance")) {Entrance=*Best;HasEntry=true;Message=TEXT("Entrance selected. Paint where the truck may drive");}
  else {Departure=*Best;HasExit=true;Message=TEXT("Exit selected. Test the complete trip to check both directions");}return true;
 }
 if(Step==TEXT("TripDestination")) {
  const FFoundrylineVehicleTarget* Best=nullptr;double Distance=FMath::Square(900.);
  for(const auto& T:M.Targets) if(T.Semi==Semi) {const double D=FMath::Min(FVector2D::DistSquared(Point,TargetMarker(T)),FVector2D::DistSquared(Point,T.Pose.Position));if(D<Distance) {Distance=D;Best=&T;}}
  if(!Best) {Message=Semi?TEXT("Click an amber loading door or staging-bay marker"):TEXT("Click a blue parking bay");return false;}
  if(Best->LoadingTier&&Best->LoadingTier<3&&Semi&&!SmallTruck){Message=TEXT("Upgrade this bay before scheduling a semi");return false;}
  if(!(Semi && SmallTruck?Best->SmallTruckClear:Best->LayoutClear)) {Message=TEXT("This destination is too tight for the vehicle. Widen its paving or bay");return false;}
  TargetId=Best->Id;Message=TargetLabel(M,TargetId)+TEXT(" selected. Now choose where the vehicle leaves");return true;
 }
 return false;
}
bool FFoundrylineLogisticsTrip::Ready(const FFoundrylineDrivingMap& M) const {
 return HasEntry && HasExit && Entrance.CanEnter && Departure.CanExit && M.Targets.ContainsByPredicate([&](const auto& T){return T.Id==TargetId && T.Semi==Semi && (!T.LoadingTier||T.LoadingTier==3||!Semi||SmallTruck);});
}
FFoundrylineLogisticsTripResult FFoundrylineLogisticsTrip::Test(const FFoundrylineDrivingMap& M,const FFoundrylineLogisticsTrip& Trip) {
 FFoundrylineLogisticsTripResult R;R.Revision=M.Revision;
 if(!Trip.Ready(M)) {R.Reason=TEXT("Choose an entrance, destination and exit before testing");return R;}
 const auto* T=M.Targets.FindByPredicate([&](const auto& X){return X.Id==Trip.TargetId;});const auto V=T->LoadingTier==1&&Trip.SmallTruck?FFoundrylineVehicleProfile::SmallFlatbed():Trip.Vehicle();
 R.Inbound=FFoundrylineVehiclePath::Find(M,V,Trip.Entrance.Entry,FFoundrylineVehiclePath::TargetPose(*T,V),true,T->Id);
 if(!R.Inbound.Success) {R.Reason=TEXT("ARRIVAL BLOCKED. Check painted access and turning space near the destination");return R;}
 R.Outbound=FFoundrylineVehiclePath::Find(M,V,R.Inbound.Poses.Last(),Trip.Departure.Exit,true,{});
 if(!R.Outbound.Success) {R.Reason=TEXT("DEPARTURE BLOCKED. Widen the exit turn or extend the painted access");return R;}
 R.Success=true;R.Reason=Trip.Semi?TEXT("Trip ready. Watch the truck arrive, stop at the destination and leave"):TEXT("Trip ready. Watch the car enter, park and leave");return R;
}
