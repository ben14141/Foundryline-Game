#include "UI/FoundrylinePlayerController.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Core/FoundrylineUnits.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
double AFoundrylinePlayerController::FactoryGridStep()const{if(IsInputKeyDown(EKeys::LeftShift)||IsInputKeyDown(EKeys::RightShift))return 2.54;return FactorySnapIndex==0?15.24:FactorySnapIndex==1?30.48:60.96;}
bool AFoundrylinePlayerController::FactoryFloorCursor(FVector& Out)const{FVector O,D;return IsLocalController()&&DeprojectMousePositionToWorld(O,D)&&FFoundrylineFloorPlacement::FloorPoint(O,D,Out);}
bool AFoundrylinePlayerController::PreviewFactoryFloor(FVector Cursor){
 auto* Plant=FactoryPlant();if(!Plant||Cursor.ContainsNaN())return false;
 FFoundrylineFactorySimulation Check;Check.State=Plant->Snapshot;Check.Solids=Plant->Model.Solids;
 // Clients never seed the plant model; rebuild the authored equipment footprint from replicated state.
 if(!Plant->HasAuthority())Check.Solids=FFoundrylineWorkshopLayout::PlantSolids(Check.State);
 TArray<FBox2D> Targets=Check.Solids;for(const auto& Z:Check.State.Zones)Targets.Add(FFoundrylineWorkZoneLayout::Bounds(Z));for(const auto& S:Check.State.WeldingStations)Targets.Add(FFoundrylineWeldingStationLayout::Bounds(S));if(FactoryTool!=TEXT("Zone"))for(const auto& L:Check.State.Lanes)Targets.Emplace(L.Min,L.Max);
 const FBox2D Floor(Check.State.FloorMin,Check.State.FloorMax);const double Step=FactoryGridStep(),Gap=FactoryGapIndex==0?0:FactoryGapIndex==1?15.24:30.48;FString Error;bFactorySnapped=false;
 if(FactoryTool==TEXT("Equipment")){FactoryPreviewA=FactoryPreviewB=FVector(FMath::GridSnap(Cursor.X,Step),FMath::GridSnap(Cursor.Y,Step),20);FactoryPreviewBounds=FFoundrylinePlanningEquipment::Bounds(FactoryPreviewA,FactoryStationQuarter,FFoundrylinePlanningEquipment::Size(FactoryEquipmentChoice,FactoryRackLevels));bFactoryPresetValid=Check.CanBuyPlanningEquipment(FactoryEquipmentChoice,FactoryPreviewA,FactoryStationQuarter,FactoryRackLevels,Error);}
 else if(FactoryTool==TEXT("Station")){
  const auto Size=FFoundrylineWeldingStationLayout::FootprintCm(FactoryStationQuarter);
  const auto Center=FFoundrylineFloorPlacement::Snap(FVector2D(Cursor),Size/2,Step,Floor,Targets,Gap,bFactoryEdgeSnap,bFactorySnapped);
  FactoryPreviewA=FactoryPreviewB=FVector(Center,20);FactoryPreviewBounds=FBox2D(Center-Size/2,Center+Size/2);
  FFoundrylineWeldingStationRecord S;S.Center=FactoryPreviewA;S.QuarterTurns=FactoryStationQuarter;S.bSwapSides=bFactoryStationSwap;
  uint32 Layout=GetTypeHash(Check.State.Lanes.Num());for(const auto& L:Check.State.Lanes)Layout=HashCombine(Layout,HashCombine(GetTypeHash(L.Min),GetTypeHash(L.Max)));for(const auto& O:Check.State.WeldingStations)Layout=HashCombine(Layout,GetTypeHash(O.Center));
  const FString Key=FString::Printf(TEXT("%s|%d|%d|%u|%d"),*Center.ToString(),FactoryStationQuarter,bFactoryStationSwap?1:0,Layout,Check.State.Zones.Num());const double Now=GetWorld()->GetRealTimeSeconds();
  if(Key!=FactoryStationCheckKey||Now-FactoryStationCheckAt>1.5){FactoryStationCheckKey=Key;FactoryStationCheckAt=Now;bFactoryStationCheckValid=Check.CanPlaceWeldingStation(S,FactoryStationCheckError);}
  bFactoryPresetValid=bFactoryStationCheckValid;Error=FactoryStationCheckError;
 }else if(FactoryTool==TEXT("Zone")){
  auto Size=FFoundrylineWorkZoneLayout::TotalSize(FactoryZonePreset);if(FactoryZoneQuarter%2)Swap(Size.X,Size.Y);
  const auto Center=FFoundrylineFloorPlacement::Snap(FVector2D(Cursor),Size/2,Step,Floor,Targets,Gap,bFactoryEdgeSnap,bFactorySnapped);
  FactoryPreviewA=FVector(Center,20);FactoryPreviewB=FactoryPreviewA;FactoryPreviewBounds=FBox2D(Center-Size/2,Center+Size/2);
  FFoundrylineFactoryZone Z;bFactoryPresetValid=FFoundrylineWorkZoneLayout::Create(FactoryPreviewA,FactoryZonePreset,FactoryZoneQuarter,FactoryZoneSide,Z)&&Check.CanPlaceZone(Z,Error);
  if(!Check.State.Routes.ContainsByPredicate([&](const auto& R){return R.Id==FactoryRoute;})){bFactoryPresetValid=false;Error=TEXT("Choose a supply route first");}
 }else{
  const bool Crossing=FactoryTool==TEXT("Crossing");FVector A=bFactoryDragging?FactoryDragStart:Cursor,B=bFactoryDragging?FactoryDragEnd:Cursor+FVector(FMath::Max(243.84,Step*4),0,0);
  const FVector RawA=A,RawB=B;
  A.X=FMath::GridSnap(A.X,Step);A.Y=FMath::GridSnap(A.Y,Step);B.X=FMath::GridSnap(B.X,Step);B.Y=FMath::GridSnap(B.Y,Step);A.Z=B.Z=20;
  const bool X=FMath::Abs(B.X-A.X)>=FMath::Abs(B.Y-A.Y);if(X)B.Y=A.Y;else B.X=A.X;
  bool EndpointSnapped=false;
  if(!Crossing&&bFactoryEdgeSnap){const FVector2D EndHalf=X?FVector2D(0,FactoryWidth/2):FVector2D(FactoryWidth/2,0);bool SA=false,SB=false;
   const auto PA=FFoundrylineFloorPlacement::Snap(FVector2D(RawA),EndHalf,Step,Floor,Targets,0,true,SA),PB=FFoundrylineFloorPlacement::Snap(FVector2D(RawB),EndHalf,Step,Floor,Targets,0,true,SB);
   if(X){A.X=PA.X;B.X=PB.X;}else{A.Y=PA.Y;B.Y=PB.Y;}EndpointSnapped=SA||SB;
  }
  const FVector2D Half=Crossing?FVector2D(100,250):X?FVector2D(FMath::Abs(B.X-A.X)/2,FactoryWidth/2):FVector2D(FactoryWidth/2,FMath::Abs(B.Y-A.Y)/2);
  const FVector2D Center=Crossing?FVector2D(Cursor):FVector2D((A+B)/2);
  // Drag endpoints define length. Magnet shifts the cross-axis only, keeping the start stable.
  FVector2D Snapped=FFoundrylineFloorPlacement::Snap(Center,Half,Step,Floor,Targets,0,bFactoryEdgeSnap,bFactorySnapped);
  if(!Crossing){if(X)Snapped.X=Center.X;else Snapped.Y=Center.Y;}
  bFactorySnapped|=EndpointSnapped;
  const FVector Delta(Snapped-Center,0);FactoryPreviewA=Crossing?FVector(Snapped,20):A+Delta;FactoryPreviewB=B+Delta;
  FactoryPreviewBounds=FBox2D(Snapped-Half,Snapped+Half);
  bFactoryPresetValid=Check.PaintLane(Crossing?EFoundrylineFloorLane::Crossing:FactoryTool==TEXT("Pedestrian")?EFoundrylineFloorLane::Pedestrian:EFoundrylineFloorLane::Forklift,FactoryPreviewA,FactoryPreviewB,FactoryWidth,FactoryDirection,Error);
  if(bFactoryPresetValid){const auto& L=Check.State.Lanes.Last();FactoryPreviewBounds=FBox2D(L.Min,L.Max);}
 }
 if(Plant->Snapshot.Running&&FactoryTool!=TEXT("Station")&&FactoryTool!=TEXT("Equipment")){bFactoryPresetValid=false;Error=TEXT("Pause the plant before painting");}
 const auto Size=FactoryPreviewBounds.GetSize();FactoryBrushDimensions=FFoundrylineUnits::Length(Size.X)+TEXT(" x ")+FFoundrylineUnits::Length(Size.Y);
 FactoryPlacementHint=bFactoryPresetValid?(bFactorySnapped?TEXT("EDGE ALIGNED / ready"):TEXT("GRID ALIGNED / ready")):Error;
 return bFactoryPresetValid;
}

bool AFoundrylinePlayerController::CommitFactoryFloorBrush(){
 if(!bFactoryPresetValid)return false;FFoundrylineFactoryCommand C;const bool Zone=FactoryTool==TEXT("Zone");
 if(FactoryTool==TEXT("Equipment")){auto* Plant=FactoryPlant();if(!Plant)return false;C.Action=EFoundrylineFactoryAction::BuyPlanningEquipment;C.TargetPlantId=Plant->BusinessPlantId;C.Id=FactoryEquipmentChoice;C.Count=FactoryRackLevels;C.QuarterTurns=FactoryStationQuarter;C.A=FactoryPreviewA;SendFactory(C);return true;}
 if(FactoryTool==TEXT("Station")){auto* Plant=FactoryPlant();if(!Plant)return false;C.Action=EFoundrylineFactoryAction::AddWeldingStation;C.A=FactoryPreviewA;C.QuarterTurns=FactoryStationQuarter;C.SupplySide=bFactoryStationSwap?-1:1;C.TargetPlantId=Plant->BusinessPlantId;FactoryStationCheckKey.Reset();SendFactory(C);return true;}
 C.Action=Zone?EFoundrylineFactoryAction::AddPresetZone:EFoundrylineFactoryAction::PaintLane;C.A=FactoryPreviewA;C.B=FactoryPreviewB;C.Id=Zone?FactoryZonePreset:FactoryTool==TEXT("Pedestrian")?1:FactoryTool==TEXT("Crossing")?2:0;C.WidthCm=FactoryWidth;C.QuarterTurns=FactoryZoneQuarter;C.SupplySide=FactoryZoneSide;C.Other=Zone?FactoryRoute:FactoryDirection;SendFactory(C);return true;
}
