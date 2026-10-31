#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Production/FoundrylineWorkZoneLayout.h"

namespace {using Kind=EFoundrylinePlannerItem;
const FFoundrylinePlannerItem* Find(const TArray<FFoundrylinePlannerItem>& Items,int32 K,int32 Id){return Items.FindByPredicate([&](const auto& V){return int32(V.Kind)==K&&V.Id==Id;});}
// Relocation must admit the same owned expansions as a new machine purchase.
// Prepare only the temporary preview; rejected floor coverage never grows live bounds.
bool PrepareMoveFloor(FFoundrylineFactorySimulation& Candidate,AFoundrylineFactoryPlant* Plant,int32 K,int32 Id,FVector At,bool Remove,int32 Quarter,FString& Error){
 if(Remove)return true;
 if(K==int32(Kind::OfficeDesk))return FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,Plant->GetWorld(),Plant->GetActorTransform(),FFoundrylinePlanningEquipment::Bounds(At+FRotator(0,(Quarter<0?0:Quarter)*90,0).RotateVector(FVector(0,40,0)),Quarter<0?0:Quarter,FFoundrylinePlanningEquipment::Size(7)),Error,Id,At.Z);
 if(K==int32(Kind::Station)){const auto* Station=Candidate.State.WeldingStations.FindByPredicate([&](const auto& S){return S.Id==Id;});return !Station||FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,Plant->GetWorld(),Plant->GetActorTransform(),FFoundrylinePlannerItems::StationWorkspaceBounds(At,Quarter<0?Station->QuarterTurns:Quarter,Station->bSwapSides),Error);}
 if(K==int32(Kind::WorkZone)){const auto* Zone=Candidate.State.Zones.FindByPredicate([&](const auto& Z){return Z.Id==Id;});if(!Zone)return true;FVector2D Half=FFoundrylineWorkZoneLayout::Bounds(*Zone).GetSize()*.5;if(Quarter>=0&&(Quarter-Zone->QuarterTurns+4)%2)Swap(Half.X,Half.Y);return FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,Plant->GetWorld(),Plant->GetActorTransform(),FBox2D(FVector2D(At)-Half,FVector2D(At)+Half),Error);}
 if(K!=int32(Kind::Machine))return true;
 const auto* Machine=Candidate.State.Machines.FindByPredicate([&](const auto& M){return M.Id==Id;});if(!Machine)return true;
 return FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,Plant->GetWorld(),Plant->GetActorTransform(),FFoundrylineFactorySimulation::MachineWorkspaceBounds(Machine->Kind(),At,Quarter<0?Machine->QuarterTurns:Quarter),Error);
}
}
bool AFoundrylinePlayerController::SelectPlannerItem(FIntVector Cursor){
 auto* Plant=FactoryPlant();if(!Plant||!bPropertyViewAccess||IsWalking())return false;
 const FVector P=Plant->GetActorTransform().InverseTransformPosition(FVector(Cursor));const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const FFoundrylinePlannerItem* Best=nullptr;double Score=MAX_dbl;
 for(const auto& I:Items){if(I.Stored||FMath::Abs(I.Center.Z-(20+OfficePlanningLevel*320))>100||!FBox2D(FVector2D(I.Bounds.Min),FVector2D(I.Bounds.Max)).ExpandBy(12).IsInsideOrOn(FVector2D(P)))continue;
  const double Area=(I.Bounds.Max.X-I.Bounds.Min.X)*(I.Bounds.Max.Y-I.Bounds.Min.Y)+(I.Kind==Kind::Lane?1.e12:I.Kind==Kind::WorkZone?1.e9:0);
  if(Area<Score){Best=&I;Score=Area;}}
 if(!Best)return false;PlannerKind=int32(Best->Kind);PlannerId=Best->Id;PlannerItemName=Best->Name;PlannerStatus.Reset();return true;
}
void AFoundrylinePlayerController::StartPlannerMove(FIntVector Cursor,bool Drag){
 auto* Plant=FactoryPlant();if(!Plant||bPlannerPending)return;const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Item=Find(Items,PlannerKind,PlannerId);if(!Item)return;
 bPlannerMoving=true;bPlannerResizing=false;PlannerSize=FVector::ZeroVector;PlannerQuarter=Item->Quarter;bPlannerDrag=Drag;PlannerGrab=Cursor;PlannerAt=PlannerOrigin=Item->Center;BuildTool=NAME_None;bDraggingBuild=false;PlannerStatus=TEXT("Choose a new position / R rotates / Right-click cancels");
}
void AFoundrylinePlayerController::PlannerAction(FName Action){
 auto* Plant=FactoryPlant();if(!Plant||!bPropertyViewAccess||IsWalking()||bPlannerPending)return;
 const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Item=Find(Items,PlannerKind,PlannerId);
 if(Action==TEXT("Cancel")){PlannerKind=PlannerId=0;bPlannerMoving=false;PlannerStatus.Reset();return;}
 if(Action.ToString().StartsWith(TEXT("Stored"))){const int Id=FCString::Atoi(*Action.ToString().RightChop(6));const auto* V=Find(Items,1,Id);if(V&&V->Stored){PlannerKind=1;PlannerId=Id;PlannerItemName=V->Name;StartPlannerMove(FIntVector::ZeroValue,false);}return;}
 if(!Item){PlannerKind=PlannerId=0;return;}
 if(Action==TEXT("Resize")){if(Item->Kind!=Kind::Lane){PlannerStatus=TEXT("Resize is available for lanes; use building corner handles for buildings");return;}StartPlannerMove(FIntVector::ZeroValue,false);bPlannerResizing=true;PlannerResizeAnchor=Item->Bounds.Min;PlannerSize=Item->Bounds.GetSize();PlannerSize.Z=0;return;}
 if(Action==TEXT("Move")){if(Item->Kind==Kind::FixedRack&&Item->MachineId){PlannerKind=int(Kind::Machine);PlannerId=Item->MachineId;}StartPlannerMove(FIntVector::ZeroValue,false);return;}
 if(Action==TEXT("Inspect")){if(Item->Kind==Kind::Machine){PlannerStatus=TEXT("Walk to this machine terminal and press E to program production");return;}if(Item->Kind==Kind::Pallet){OperationsInspect=Item->Id;OpenOperations(3);return;}if(Item->Kind==Kind::Station){FactoryStation=Item->Id;FactoryPage=0;bFactoryMenu=true;return;}if(Item->Kind==Kind::WorkZone){FactoryZone=Item->Id;FactoryPage=0;bFactoryMenu=true;return;}OpenOperations(1);OperationsPage=Item->Kind==Kind::Forklift?7:11;return;}
 const bool Remove=Action==TEXT("Delete");if(!Remove&&Action!=TEXT("Place"))return;
 const FVector At=Remove?FVector(Item->Center.X,Item->Center.Y,Item->Center.Z):PlannerAt;
 FFoundrylineFactorySimulation Preview;Preview.State=Plant->Snapshot;Preview.Solids=Plant->Model.Solids;FString Error;
 if(!PrepareMoveFloor(Preview,Plant,PlannerKind,PlannerId,At,Remove,PlannerQuarter,Error)||!Preview.EditPlanningItem(PlannerKind,PlannerId,At,Remove,Error,PlannerQuarter,PlannerSize)){PlannerStatus=Error;return;}
 FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::EditPlanningItem;C.TargetPlantId=Plant->BusinessPlantId;C.Count=PlannerKind;C.Id=PlannerId;C.A=At;C.Flag=Remove;C.QuarterTurns=PlannerQuarter;C.B=PlannerSize;
 bPlannerPending=true;SendFactory(C);bPlannerMoving=false;PlannerStatus.Reset();
 if(Remove&&Item->Kind!=Kind::Machine){PlannerKind=PlannerId=0;PlannerItemName.Reset();}
}
bool AFoundrylinePlayerController::HandlePlannerInput(const FInputKeyEventArgs& P){
 if(!bPlannerMoving||IsWalking()||OperationsScreen||bFactoryMenu)return false;
 if(P.Event==IE_Pressed&&(P.Key==EKeys::Escape||P.Key==EKeys::RightMouseButton)){bPlannerMoving=false;PlannerStatus.Reset();return true;}
 if(P.Event==IE_Pressed&&P.Key==EKeys::R){if(!bPlannerResizing)PlannerQuarter=(PlannerQuarter+1)%4;return true;}
 if(P.Key==EKeys::LeftMouseButton){if((!bPlannerDrag&&P.Event==IE_Pressed)||(bPlannerDrag&&P.Event==IE_Released)){if(!IsOverBuildHUD())PlannerAction(TEXT("Place"));}return true;}
 return P.Key==EKeys::Delete;
}
void AFoundrylinePlayerController::TickPlannerSelection(bool HasCursor,FIntVector Cursor){
 if(IsWalking()||!bPropertyViewAccess){PlannerKind=PlannerId=0;bPlannerMoving=false;return;}if(OperationsScreen||bFactoryMenu)return;auto* Plant=FactoryPlant();if(!Plant)return;
 auto WorkspaceItems=[](const FFoundrylineFactoryState& State){auto Items=FFoundrylinePlannerItems::List(State);
  for(auto& Item:Items)if(Item.Kind==Kind::Machine){FBox Area=Item.Bounds;for(const auto& Rack:Items)if(Rack.Kind==Kind::FixedRack&&Rack.MachineId==Item.Id)Area+=Rack.Bounds;
   if(Item.Stored)if(const auto* Machine=State.Machines.FindByPredicate([&](const auto& M){return M.Id==Item.Id;})){const auto B=FFoundrylineFactorySimulation::MachineWorkspaceBounds(Machine->Kind(),Machine->Origin,Machine->QuarterTurns);Area=FBox(FVector(B.Min,20),FVector(B.Max,325));}
   Item.Bounds=Area.ExpandBy(FVector(25,25,0));}return Items;};
 auto Items=WorkspaceItems(Plant->Snapshot);
 const auto* Selected=Find(Items,PlannerKind,PlannerId);if(!Selected&&PlannerKind){PlannerKind=PlannerId=0;bPlannerMoving=false;}
 auto Outline=[&](const FFoundrylinePlannerItem& I,bool Hover,FVector Offset,bool Valid){
  const FBox B=I.Bounds.ShiftBy(Offset);FBox World(ForceInit);for(int X:{0,1})for(int Y:{0,1})for(int Z:{0,1})World+=Plant->GetActorTransform().TransformPosition(FVector(X?B.Max.X:B.Min.X,Y?B.Max.Y:B.Min.Y,Z?B.Max.Z:B.Min.Z));
  for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->SelectionOutlines.Add({FVector2D(World.Min),FVector2D(World.Max),float(World.Min.Z),Hover,float(World.Max.Z-World.Min.Z),!Valid});break;}
 };
 if(Selected){PlannerItemName=Selected->Name;if(!Selected->Stored)Outline(*Selected,false,FVector::ZeroVector,true);}
 if(bPlannerMoving&&Selected&&HasCursor){const FVector Local=Plant->GetActorTransform().InverseTransformPosition(FVector(Cursor));const FVector Grab=Plant->GetActorTransform().InverseTransformPosition(FVector(PlannerGrab));PlannerAt=bPlannerDrag?PlannerOrigin+Local-Grab:Local;const double Step=FactoryGridStep();PlannerAt.X=FMath::GridSnap(PlannerAt.X,Step);PlannerAt.Y=FMath::GridSnap(PlannerAt.Y,Step);PlannerAt.Z=PlannerKind==int32(Kind::OfficeDesk)?20+OfficePlanningLevel*320:20;if(bPlannerResizing){const FVector Low=PlannerResizeAnchor.ComponentMin(PlannerAt),High=PlannerResizeAnchor.ComponentMax(PlannerAt);PlannerSize=High-Low;PlannerSize.Z=0;PlannerAt=(Low+High)/2;PlannerAt.Z=20;}
  FFoundrylineFactorySimulation Preview;Preview.State=Plant->Snapshot;Preview.Solids=Plant->Model.Solids;FString Error;bPlannerValid=PrepareMoveFloor(Preview,Plant,PlannerKind,PlannerId,PlannerAt,false,PlannerQuarter,Error)&&Preview.EditPlanningItem(PlannerKind,PlannerId,PlannerAt,false,Error,PlannerQuarter,PlannerSize);PlannerStatus=bPlannerValid?TEXT("Click to place / R rotates / Right-click cancels"):Error;const auto Next=WorkspaceItems(Preview.State);const auto* Updated=Find(Next,PlannerKind,PlannerId);if(bPlannerValid&&Updated)Outline(*Updated,true,FVector::ZeroVector,true);else Outline(*Selected,true,PlannerAt-Selected->Center,false);return;}
 if(!HasCursor||bPlannerMoving)return;
 const int OldKind=PlannerKind,OldId=PlannerId;const FString OldName=PlannerItemName,OldStatus=PlannerStatus;
 if(SelectPlannerItem(Cursor)){if(PlannerKind!=OldKind||PlannerId!=OldId)if(const auto* Hover=Find(Items,PlannerKind,PlannerId))Outline(*Hover,true,FVector::ZeroVector,true);}
 PlannerKind=OldKind;PlannerId=OldId;PlannerItemName=OldName;PlannerStatus=OldStatus;
}

void AFoundrylinePlayerController::RedoPlanner(){if(!bLastPlannerEdit||!bPropertyViewAccess||IsWalking()||bPlannerPending)return;auto* Plant=FactoryPlant();if(!Plant)return;FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::RedoPlanner;C.TargetPlantId=Plant->BusinessPlantId;bPlannerPending=true;SendFactory(C);}
