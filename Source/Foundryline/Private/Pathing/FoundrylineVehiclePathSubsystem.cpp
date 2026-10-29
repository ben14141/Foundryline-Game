#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
namespace {
 FFoundrylineDriveRect RelativeBounds(const FFoundrylinePlacedObject& A,const UFoundrylinePlaceableDefinition& AD,const FFoundrylinePlacedObject& B,const UFoundrylinePlaceableDefinition& BD) {
  const FTransform TA(FRotator(0,A.QuarterTurns*90,0),FVector(A.Anchor)),TB(FRotator(0,B.QuarterTurns*90,0),FVector(B.Anchor));
  FFoundrylineDriveRect R{{MAX_flt,MAX_flt},{-MAX_flt,-MAX_flt}};
  for(float X:{0.f,float(BD.Footprint.Max.X)}) for(float Y:{0.f,float(BD.Footprint.Max.Y)}) {const auto P=TA.InverseTransformPosition(TB.TransformPosition({X,Y,0}));R.Min.X=FMath::Min(R.Min.X,P.X);R.Min.Y=FMath::Min(R.Min.Y,P.Y);R.Max.X=FMath::Max(R.Max.X,P.X);R.Max.Y=FMath::Max(R.Max.Y,P.Y);}return R;
 }
 FGuid TargetId(FGuid Object,int32 X,int32 Y,uint32 Type) {return FGuid(Object.A^uint32(X),Object.B^uint32(Y),Object.C^Type,Object.D^0xD21A0001);}
}
TArray<FFoundrylineDriveRect> UFoundrylineVehiclePathSubsystem::Entrances(const UFoundrylinePlacementSubsystem& P,const FFoundrylinePlacedObject& O)
{
 TArray<FFoundrylineDriveRect> Out;const auto* D=P.FindDefinition(O.DefinitionId);if(!D) return Out;const float W=D->Footprint.Max.X,H=D->Footprint.Max.Y;
 for(const auto& Other:P.GetObjects()) {
  const auto* OD=P.FindDefinition(Other.DefinitionId);if(Other.ObjectId==O.ObjectId || !OD || !FFoundrylineDrivingLayout::IsPaved(OD->FunctionId)) continue;
  const auto B=RelativeBounds(O,*D,Other,*OD);
  const float X0=FMath::Max(40.f,B.Min.X),X1=FMath::Min(W-40,B.Max.X),Y0=FMath::Max(40.f,B.Min.Y),Y1=FMath::Min(H-40,B.Max.Y);
  if(X1>X0 && FMath::Abs(B.Max.Y)<1) Out.Add({{X0,0},{X1,550}});
  if(X1>X0 && FMath::Abs(B.Min.Y-H)<1) Out.Add({{X0,H-550},{X1,H}});
  if(Y1>Y0 && FMath::Abs(B.Max.X)<1) Out.Add({{0,Y0},{550,Y1}});
  if(Y1>Y0 && FMath::Abs(B.Min.X-W)<1) Out.Add({{W-550,Y0},{W,Y1}});
 }
 return Out;
}
TSharedPtr<const FFoundrylineDrivingMap,ESPMode::ThreadSafe> UFoundrylineVehiclePathSubsystem::GetMap()
{
 auto* P=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 if(Cached && Cached->Revision==P->GetRevision()) return Cached;
 auto M=MakeShared<FFoundrylineDrivingMap,ESPMode::ThreadSafe>();M->Revision=P->GetRevision();
 for(const auto& O:P->GetObjects()) {
  const auto* D=P->FindDefinition(O.DefinitionId);if(!D || !FFoundrylineDrivingLayout::IsPaved(D->FunctionId)) continue;
  FFoundrylineDriveSurface S;S.Id=O.ObjectId;S.Function=D->FunctionId;S.Size={double(D->Footprint.Max.X),double(D->Footprint.Max.Y)};S.Transform=FTransform(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));
  S.Public=O.CompanyId==FGuid(0xF001,99,1,1);
  for(const auto& Z:O.TruckZones) S.TruckZones.Add({FVector2D(Z.Min),FVector2D(Z.Max)});
  if(S.Function==TEXT("Parking")) {
   S.Entrances=Entrances(*P,O);S.Aisles=FFoundrylineDrivingLayout::ParkingAisles(S.Size.X,S.Size.Y,S.Entrances);S.Islands=FFoundrylineDrivingLayout::Islands(S.Size.X,S.Size.Y,O.bLandscaping);
   for(const auto& B:FFoundrylineDrivingLayout::Parking(S.Size.X,S.Size.Y,S.Entrances)) {
    FFoundrylineVehicleTarget T;T.Id=TargetId(O.ObjectId,FMath::RoundToInt(B.RearAxle.X),FMath::RoundToInt(B.RearAxle.Y),1);T.SurfaceId=S.Id;T.Bay=B.Bounds;T.Pose.Position=S.World(B.RearAxle);T.Pose.Yaw=FMath::DegreesToRadians(B.Heading+O.QuarterTurns*90);T.Pose.TrailerYaw=T.Pose.Yaw;M->Targets.Add(T);
   }
  }
  if(S.Function==TEXT("FreightYard") && S.Size.X>=5200) for(float Y=400;Y+400<=S.Size.Y-200;Y+=500) {
   FFoundrylineVehicleTarget T;T.Id=TargetId(O.ObjectId,1570,Y+200,2);T.SurfaceId=S.Id;T.Semi=true;T.Bay={{0,Y},{FFoundrylineDrivingLayout::TruckBayDepth,Y+400}};
   T.Pose.Position=S.World({1570,Y+200});T.Pose.Yaw=FMath::DegreesToRadians(O.QuarterTurns*90);T.Pose.TrailerYaw=T.Pose.Yaw;M->Targets.Add(T);
  }
  M->Surfaces.Add(S);
 }
 for(const auto& O:P->GetObjects()) {
  const auto* D=P->FindDefinition(O.DefinitionId);if(!D || D->FunctionId!=TEXT("BuildingShell")) continue;
  for(const auto& Door:O.Openings) if(Door.Kind==EFoundrylineOpeningKind::LoadingDock || Door.Kind==EFoundrylineOpeningKind::TruckGarage) {
   const float Along=Door.OffsetCm+Door.WidthCm()/2;
   const FVector2D Point=Door.Edge==0?FVector2D(Along,0):Door.Edge==1?FVector2D(D->Footprint.Max.X,Along):Door.Edge==2?FVector2D(Along,D->Footprint.Max.Y):FVector2D(0,Along);
   const float Yaw=Door.Edge==0?-90:Door.Edge==1?0:Door.Edge==2?90:180;
   const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));const auto W=T.TransformPosition(FVector(Point,0));
   FFoundrylineVehicleTarget Goal;Goal.Id=Door.OpeningId;Goal.SurfaceId=O.ObjectId;Goal.Semi=true;Goal.Loading=true;Goal.Bay={{-Door.WidthCm()/2.,-50},{Door.WidthCm()/2.,50}};Goal.Pose.Yaw=FMath::DegreesToRadians(Yaw+O.QuarterTurns*90);Goal.Pose.TrailerYaw=Goal.Pose.Yaw;
   Goal.Pose.Position=FVector2D(W.X,W.Y)+FVector2D(FMath::Cos(Goal.Pose.Yaw),FMath::Sin(Goal.Pose.Yaw))*1570;M->Targets.Add(Goal);
  }
 }
 for(auto& Target:M->Targets) {
  Target.LayoutClear=FFoundrylineVehiclePath::PoseClear(*M,Target.Semi?FFoundrylineVehicleProfile::Truck():FFoundrylineVehicleProfile::Car(),Target.Pose,false,Target.Id);
  if(Target.Semi) {const auto V=FFoundrylineVehicleProfile::BoxTruck();Target.SmallTruckClear=FFoundrylineVehiclePath::PoseClear(*M,V,FFoundrylineVehiclePath::TargetPose(Target,V),false,Target.Id);}
 }
 Cached=M;
 for(auto It=OccupiedTargets.CreateIterator();It;++It) if(!M->Targets.ContainsByPredicate([&](const auto& T){return T.Id==It.Key();})) It.RemoveCurrent();
 return Cached;
}
int32 UFoundrylineVehiclePathSubsystem::GetParkingTargetCount() {const auto M=GetMap();return M->Targets.FilterByPredicate([](const auto& T){return !T.Semi;}).Num();}
int32 UFoundrylineVehiclePathSubsystem::GetTruckTargetCount() {const auto M=GetMap();return M->Targets.FilterByPredicate([](const auto& T){return T.Semi;}).Num();}
FString UFoundrylineVehiclePathSubsystem::GetDrivingDigest() {const auto M=GetMap();int32 Zones=0;for(const auto& S:M->Surfaces) Zones+=S.TruckZones.Num();return FString::Printf(TEXT("%lld/%d/%d/%d"),M->Revision,M->Surfaces.Num(),M->Targets.Num(),Zones);}
bool UFoundrylineVehiclePathSubsystem::ReserveTarget(FGuid Target,FGuid Vehicle) {if(GetWorld()->GetNetMode()==NM_Client || !Vehicle.IsValid() || !GetMap()->Targets.ContainsByPredicate([&](const auto& T){return T.Id==Target;})) return false;const auto* Owner=OccupiedTargets.Find(Target);if(Owner && *Owner!=Vehicle) return false;OccupiedTargets.Add(Target,Vehicle);return true;}
void UFoundrylineVehiclePathSubsystem::ReleaseTarget(FGuid Target,FGuid Vehicle) {if(GetWorld()->GetNetMode()==NM_Client) return;if(const auto* Owner=OccupiedTargets.Find(Target);Owner && *Owner==Vehicle) OccupiedTargets.Remove(Target);}
bool UFoundrylineVehiclePathSubsystem::IsTargetAvailable(FGuid Target) const {return !OccupiedTargets.Contains(Target);}
