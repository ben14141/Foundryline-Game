#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "EngineUtils.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"

FBox2D FFoundrylinePlannerItems::StationWorkspaceBounds(FVector Center,int32 Quarter,bool Swap){
 FFoundrylineWeldingStationRecord Station;Station.Center=Center;Station.QuarterTurns=Quarter;Station.bSwapSides=Swap;
 FBox2D Bounds=FFoundrylineWeldingStationLayout::Bounds(Station);
 for(const auto& Access:FFoundrylineWeldingStationLayout::AccessRing(Station)){Bounds+=Access.Min;Bounds+=Access.Max;}
 return Bounds;
}

namespace {
constexpr int32 MaxFloorSections=256,MaxPlacementObjects=16384,MaxFloorSamples=4096;
struct FPlannerFloorCache {
 TWeakObjectPtr<UWorld> World;FGuid Company,Plant;int64 Revision=-1;
 bool bAnyBuildings=false;FString Error;TArray<FBox2D> Floors,UpperFloors,Stairs;
};
// At most16 business/world snapshots. Placement revision invalidates expansions,
// moves, removals and replication; no scanning of the whole town on each preview tick.
TArray<FPlannerFloorCache> PlannerFloorCaches;
const FPlannerFloorCache& OwnedFloorSnapshot(UWorld* World,const UFoundrylinePlacementSubsystem* Placement,const FFoundrylineFactoryState& State){
 PlannerFloorCaches.RemoveAll([](const auto& C){return !C.World.IsValid();});
 const int64 Revision=Placement->GetRevision();
 if(const auto* Cached=PlannerFloorCaches.FindByPredicate([&](const auto& C){return C.World.Get()==World&&C.Company==State.CompanyId&&C.Plant==State.PlantId&&C.Revision==Revision;}))return *Cached;
 if(PlannerFloorCaches.Num()>=16)PlannerFloorCaches.RemoveAt(0);
 auto& Cache=PlannerFloorCaches.AddDefaulted_GetRef();Cache.World=World;Cache.Company=State.CompanyId;Cache.Plant=State.PlantId;Cache.Revision=Revision;
 const auto& Objects=Placement->GetObjects();
 if(Objects.Num()>MaxPlacementObjects){Cache.Error=TEXT("The site contains too many construction records to check this workspace safely");return Cache;}
 for(const auto& Object:Objects){
  const auto* Definition=Placement->FindDefinition(Object.DefinitionId);if(!Definition||Definition->FunctionId!=TEXT("BuildingShell"))continue;
  Cache.bAnyBuildings=true;if(Object.CompanyId!=State.CompanyId||Object.PlantId!=State.PlantId)continue;
  if(Cache.Floors.Num()>=MaxFloorSections){Cache.Error=TEXT("This business has too many floor sections for one planning workspace");return Cache;}
  if(Object.QuarterTurns>3||!Definition->Footprint.IsValid()){Cache.Error=TEXT("An owned building has invalid floor dimensions");return Cache;}
  // Building objects use integer world anchors and quarter turns, so their real
  // rotated floor rectangles remain axis-aligned in world XY.
  const auto Bounds=FFoundrylineGrid::RotateBounds(Definition->Footprint,Object.Anchor,Object.QuarterTurns);
  Cache.Floors.Add(FBox2D(FVector2D(Bounds.Min.X,Bounds.Min.Y),FVector2D(Bounds.Max.X,Bounds.Max.Y)));
  if(Object.bOfficeSecondFloor){const FTransform T(FRotator(0,Object.QuarterTurns*90,0),FVector(Object.Anchor));auto Convert=[&](const FBox2D& B){FBox2D Out(ForceInit);for(int X:{0,1})for(int Y:{0,1})Out+=FVector2D(T.TransformPosition({X?B.Max.X:B.Min.X,Y?B.Max.Y:B.Min.Y,0}));return Out;};for(const auto& Panel:FFoundrylineOfficeStairLayout::FloorPanels(Definition->Footprint.Max.X,Definition->Footprint.Max.Y,Object.OfficeStairOrigin,Object.OfficeStairQuarter,Object.OfficeStairStyle))Cache.UpperFloors.Add(Convert(Panel));Cache.Stairs.Add(Convert(FFoundrylineOfficeStairLayout::FullBounds(Object.OfficeStairOrigin,Object.OfficeStairQuarter,Object.OfficeStairStyle)));}

 }
 return Cache;
}
}

bool FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(FFoundrylineFactoryState& State,UWorld* World,const FTransform& PlantTransform,const FBox2D& RequiredLocal,FString& Error,int32 OfficeDeskId,double FloorZ){
 Error.Reset();if(!FMath::IsFinite(FloorZ)||(FMath::Abs(FloorZ-20)>.1&&FMath::Abs(FloorZ-340)>.1)){Error=TEXT("Choose a valid office floor");return false;}
 if(!RequiredLocal.bIsValid||RequiredLocal.Min.ContainsNaN()||RequiredLocal.Max.ContainsNaN()||PlantTransform.ContainsNaN()){
  Error=TEXT("Choose a valid workspace on the building floor");return false;
 }
 const FVector2D Size=RequiredLocal.GetSize();
 if(Size.X<=0||Size.Y<=0||Size.X>20000||Size.Y>20000){Error=TEXT("Choose a workspace smaller than 656 ft in each direction");return false;}
 const FVector Scale=PlantTransform.GetScale3D();
 if(FMath::Abs(Scale.X)<.001||FMath::Abs(Scale.Y)<.001||FMath::Abs(Scale.Z)<.001){Error=TEXT("The plant has an invalid placement transform");return false;}
 const auto* Placement=World?World->GetSubsystem<UFoundrylinePlacementSubsystem>():nullptr;
 // Plain deterministic simulation tests have no authored world geometry.
 if(!Placement)return true;
 const auto& Cache=OwnedFloorSnapshot(World,Placement,State);
 if(!Cache.Error.IsEmpty()){Error=Cache.Error;return false;}
 if(!Cache.bAnyBuildings)return true;
 const auto& Floors=FloorZ>300?Cache.UpperFloors:Cache.Floors;
 if(Floors.IsEmpty()){Error=TEXT("Place this business's shop before installing production equipment");return false;}
 FVector2D ExpandedMin=State.FloorMin,ExpandedMax=State.FloorMax;
 for(const auto& Floor:Cache.Floors)for(int X:{0,1})for(int Y:{0,1}){
  const FVector Local=PlantTransform.InverseTransformPosition(FVector(X?Floor.Max.X:Floor.Min.X,Y?Floor.Max.Y:Floor.Min.Y,PlantTransform.GetLocation().Z));
  if(Local.ContainsNaN()){Error=TEXT("A building floor could not be aligned with this plant");return false;}
  ExpandedMin.X=FMath::Min(ExpandedMin.X,Local.X);ExpandedMin.Y=FMath::Min(ExpandedMin.Y,Local.Y);ExpandedMax.X=FMath::Max(ExpandedMax.X,Local.X);ExpandedMax.Y=FMath::Max(ExpandedMax.Y,Local.Y);
 }
 int32 NX=FMath::Max(1,FMath::CeilToInt(Size.X/50)),NY=FMath::Max(1,FMath::CeilToInt(Size.Y/50));
 if(int64(NX+1)*(NY+1)>MaxFloorSamples){NX=FMath::Max(1,FMath::CeilToInt(Size.X/100));NY=FMath::Max(1,FMath::CeilToInt(Size.Y/100));}
 if(int64(NX+1)*(NY+1)>MaxFloorSamples){Error=TEXT("This workspace is too large to validate; split it into smaller sections");return false;}
 // Include every boundary and both ends. Adjacent shell rectangles form a union;
 // crossing a join is valid, crossing grass or another business's floor is not.
 for(int32 IX=0;IX<=NX;++IX)for(int32 IY=0;IY<=NY;++IY){
  const FVector Local(RequiredLocal.Min.X+Size.X*IX/NX,RequiredLocal.Min.Y+Size.Y*IY/NY,FloorZ);
  const FVector2D Point(PlantTransform.TransformPosition(Local));
  if(!Floors.ContainsByPredicate([&](const auto& Floor){return Floor.ExpandBy(.1).IsInsideOrOn(Point);})){
   Error=TEXT("Keep the complete workspace, storage and access on your building floors; expand the shop first");return false;
  }
 }
 {FBox2D WorldRequired(ForceInit);for(int X:{0,1})for(int Y:{0,1})WorldRequired+=FVector2D(PlantTransform.TransformPosition({X?RequiredLocal.Max.X:RequiredLocal.Min.X,Y?RequiredLocal.Max.Y:RequiredLocal.Min.Y,FloorZ}));if(Cache.Stairs.ContainsByPredicate([&](const auto& B){return B.Intersect(WorldRequired);})){Error=TEXT("Keep the staircase and its landings clear");return false;}}
 if(OfficeDeskId>=0)for(TActorIterator<AFoundrylineOperationsTerminal> It(World);It;++It)if(It->Terminal==1&&FMath::Abs(It->GetActorLocation().Z-PlantTransform.TransformPosition({0,0,FloorZ}).Z)<100&&It->PlantId==State.PlantId&&(OfficeDeskId==0||It->PlanningDeskId!=OfficeDeskId)){const FVector P=PlantTransform.InverseTransformPosition(It->GetActorLocation());const int Q=(FMath::RoundToInt((It->GetActorRotation().Yaw-PlantTransform.Rotator().Yaw)/90)+4)%4;if(RequiredLocal.Intersect(FFoundrylinePlanningEquipment::Bounds(P+FRotator(0,Q*90,0).RotateVector(FVector(0,40,0)),Q,FFoundrylinePlanningEquipment::Size(7)))){Error=TEXT("Leave room for the existing desk, chair and computer");return false;}}
 // Preserve seeded outdoor lanes/loading aprons, while admitting real additions.
 // Commit only after coverage succeeds; an invalid preview cannot mutate the model.
 State.FloorMin=ExpandedMin;State.FloorMax=ExpandedMax;return true;
}
