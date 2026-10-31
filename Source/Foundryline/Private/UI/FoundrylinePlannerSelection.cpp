#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"

namespace {using Kind=EFoundrylinePlannerItem;
const FFoundrylinePlannerItem* Find(const TArray<FFoundrylinePlannerItem>& Items,int32 K,int32 Id){return Items.FindByPredicate([&](const auto& V){return int32(V.Kind)==K&&V.Id==Id;});}
}
bool AFoundrylinePlayerController::SelectPlannerItem(FIntVector Cursor){
 auto* Plant=FactoryPlant();if(!Plant||!bPropertyViewAccess||IsWalking())return false;
 const FVector P=Plant->GetActorTransform().InverseTransformPosition(FVector(Cursor));const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const FFoundrylinePlannerItem* Best=nullptr;double Score=MAX_dbl;
 for(const auto& I:Items){if(I.Stored||!FBox2D(FVector2D(I.Bounds.Min),FVector2D(I.Bounds.Max)).ExpandBy(12).IsInsideOrOn(FVector2D(P)))continue;
  const double Area=(I.Bounds.Max.X-I.Bounds.Min.X)*(I.Bounds.Max.Y-I.Bounds.Min.Y)+(I.Kind==Kind::Lane?1.e12:I.Kind==Kind::WorkZone?1.e9:0);
  if(Area<Score){Best=&I;Score=Area;}}
 if(!Best)return false;PlannerKind=int32(Best->Kind);PlannerId=Best->Id;PlannerItemName=Best->Name;PlannerStatus.Reset();return true;
}
void AFoundrylinePlayerController::StartPlannerMove(FIntVector Cursor,bool Drag){
 auto* Plant=FactoryPlant();if(!Plant||bPlannerPending)return;const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Item=Find(Items,PlannerKind,PlannerId);if(!Item)return;
 bPlannerMoving=true;bPlannerDrag=Drag;PlannerGrab=Cursor;PlannerAt=PlannerOrigin=Item->Center;BuildTool=NAME_None;bDraggingBuild=false;PlannerStatus=TEXT("Choose a new position / Right-click cancels");
}
void AFoundrylinePlayerController::PlannerAction(FName Action){
 auto* Plant=FactoryPlant();if(!Plant||!bPropertyViewAccess||IsWalking()||bPlannerPending)return;
 const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Item=Find(Items,PlannerKind,PlannerId);
 if(Action==TEXT("Cancel")){PlannerKind=PlannerId=0;bPlannerMoving=false;PlannerStatus.Reset();return;}
 if(Action.ToString().StartsWith(TEXT("Stored"))){const int Id=FCString::Atoi(*Action.ToString().RightChop(6));const auto* V=Find(Items,1,Id);if(V&&V->Stored){PlannerKind=1;PlannerId=Id;PlannerItemName=V->Name;StartPlannerMove(FIntVector::ZeroValue,false);}return;}
 if(!Item){PlannerKind=PlannerId=0;return;}
 if(Action==TEXT("Move")){StartPlannerMove(FIntVector::ZeroValue,false);return;}
 if(Action==TEXT("Inspect")){if(Item->Kind==Kind::Machine){OpenFactoryComputer(Item->Id);return;}if(Item->Kind==Kind::Pallet){OperationsInspect=Item->Id;OpenOperations(3);return;}if(Item->Kind==Kind::Station){FactoryStation=Item->Id;FactoryPage=0;bFactoryMenu=true;return;}if(Item->Kind==Kind::WorkZone){FactoryZone=Item->Id;FactoryPage=0;bFactoryMenu=true;return;}OpenOperations(1);OperationsPage=Item->Kind==Kind::Forklift?7:11;return;}
 const bool Remove=Action==TEXT("Delete");if(!Remove&&Action!=TEXT("Place"))return;
 const FVector At=Remove?FVector(Item->Center.X,Item->Center.Y,20):PlannerAt;
 FFoundrylineFactorySimulation Preview;Preview.State=Plant->Snapshot;Preview.Solids=Plant->Model.Solids;FString Error;
 if(!Preview.EditPlanningItem(PlannerKind,PlannerId,At,Remove,Error)){PlannerStatus=Error;return;}
 FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::EditPlanningItem;C.TargetPlantId=Plant->BusinessPlantId;C.Count=PlannerKind;C.Id=PlannerId;C.A=At;C.Flag=Remove;
 bPlannerPending=true;SendFactory(C);bPlannerMoving=false;PlannerStatus.Reset();
 if(Remove&&Item->Kind!=Kind::Machine){PlannerKind=PlannerId=0;PlannerItemName.Reset();}
}
bool AFoundrylinePlayerController::HandlePlannerInput(const FInputKeyEventArgs& P){
 if(!bPlannerMoving||IsWalking()||OperationsScreen||bFactoryMenu)return false;
 if(P.Event==IE_Pressed&&(P.Key==EKeys::Escape||P.Key==EKeys::RightMouseButton)){bPlannerMoving=false;PlannerStatus.Reset();return true;}
 if(P.Key==EKeys::LeftMouseButton){if((!bPlannerDrag&&P.Event==IE_Pressed)||(bPlannerDrag&&P.Event==IE_Released)){if(!IsOverBuildHUD())PlannerAction(TEXT("Place"));}return true;}
 return P.Key==EKeys::Delete;
}
void AFoundrylinePlayerController::TickPlannerSelection(bool HasCursor,FIntVector Cursor){
 if(IsWalking()||!bPropertyViewAccess){PlannerKind=PlannerId=0;bPlannerMoving=false;return;}if(OperationsScreen||bFactoryMenu)return;auto* Plant=FactoryPlant();if(!Plant)return;
 const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Selected=Find(Items,PlannerKind,PlannerId);if(!Selected&&PlannerKind){PlannerKind=PlannerId=0;bPlannerMoving=false;}
 auto Outline=[&](const FFoundrylinePlannerItem& I,bool Hover,FVector Offset,bool Valid){
  const FBox B=I.Bounds.ShiftBy(Offset);FBox World(ForceInit);for(int X:{0,1})for(int Y:{0,1})for(int Z:{0,1})World+=Plant->GetActorTransform().TransformPosition(FVector(X?B.Max.X:B.Min.X,Y?B.Max.Y:B.Min.Y,Z?B.Max.Z:B.Min.Z));
  for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->SelectionOutlines.Add({FVector2D(World.Min),FVector2D(World.Max),float(World.Min.Z),Hover,float(World.Max.Z-World.Min.Z),!Valid});break;}
 };
 if(Selected){PlannerItemName=Selected->Name;if(!Selected->Stored)Outline(*Selected,false,FVector::ZeroVector,true);}
 if(bPlannerMoving&&Selected&&HasCursor){const FVector Local=Plant->GetActorTransform().InverseTransformPosition(FVector(Cursor));const FVector Grab=Plant->GetActorTransform().InverseTransformPosition(FVector(PlannerGrab));PlannerAt=bPlannerDrag?PlannerOrigin+Local-Grab:Local;PlannerAt.X=FMath::RoundToDouble(PlannerAt.X/10)*10;PlannerAt.Y=FMath::RoundToDouble(PlannerAt.Y/10)*10;PlannerAt.Z=20;
  FFoundrylineFactorySimulation Preview;Preview.State=Plant->Snapshot;Preview.Solids=Plant->Model.Solids;FString Error;bPlannerValid=Preview.EditPlanningItem(PlannerKind,PlannerId,PlannerAt,false,Error);PlannerStatus=bPlannerValid?TEXT("Click to place / Right-click cancels"):Error;Outline(*Selected,true,PlannerAt-Selected->Center,bPlannerValid);return;}
 if(!HasCursor||bPlannerMoving)return;
 const int OldKind=PlannerKind,OldId=PlannerId;const FString OldName=PlannerItemName,OldStatus=PlannerStatus;
 if(SelectPlannerItem(Cursor)){if(PlannerKind!=OldKind||PlannerId!=OldId)if(const auto* Hover=Find(Items,PlannerKind,PlannerId))Outline(*Hover,true,FVector::ZeroVector,true);}
 PlannerKind=OldKind;PlannerId=OldId;PlannerItemName=OldName;PlannerStatus=OldStatus;
}
