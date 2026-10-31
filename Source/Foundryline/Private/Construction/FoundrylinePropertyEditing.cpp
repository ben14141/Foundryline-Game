#include "Construction/FoundrylineBuildSite.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "SaveSystem/FoundrylineSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void AFoundrylineBuildSite::RecordUndo(const FFoundrylinePlacementRequest& R)
{
 UndoStack.Add(R);RedoStack.Reset();
 if(UndoStack.Num()>100) UndoStack.RemoveAt(0);
 for(auto It=CompoundHistory.CreateIterator();It;++It)if(!UndoStack.ContainsByPredicate([&](const auto& R){return R.ObjectId==It.Key();})&&!RedoStack.ContainsByPredicate([&](const auto& R){return R.ObjectId==It.Key();}))It.RemoveCurrent();
}
void AFoundrylineBuildSite::Undo() {ReplayHistory(false);}
void AFoundrylineBuildSite::Redo() {ReplayHistory(true);}
void AFoundrylineBuildSite::ReplayHistory(bool Redo)
{
 auto& From=Redo?RedoStack:UndoStack;auto& To=Redo?UndoStack:RedoStack;
 if(!bReady || IsPropertyBusy() || From.IsEmpty()) {Status=TEXT("No available history action");return;}
 auto R=From.Last();R.ExpectedRevision=Placement->GetRevision();
 if(R.DefinitionId==TEXT("__CompoundHistory")){
  const auto* Stored=CompoundHistory.Find(R.ObjectId);if(!Stored)return;auto Requests=*Stored;TArray<FFoundrylinePlacementRequest> Inverse;
  for(int I=Requests.Num()-1;I>=0;--I){auto& Q=Requests[I];Q.ExpectedRevision=Placement->GetRevision();FFoundrylinePlacementRequest Back;Back.ObjectId=Q.ObjectId;
   if(const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Q.ObjectId;})){Back.Replacement=*O;Back.Action=Q.Action==EFoundrylinePlacementAction::Remove?EFoundrylinePlacementAction::RestoreRemoved:EFoundrylinePlacementAction::ReplaceObject;}else Back.Action=EFoundrylinePlacementAction::Remove;Inverse.Add(Back);}
  const auto Result=ExecuteConstructionBatch(Requests);if(!Result.IsSuccess()){Status=TEXT("Undo blocked by current property state");return;}From.Pop();CompoundHistory.Remove(R.ObjectId);R.ObjectId=FGuid::NewGuid();CompoundHistory.Add(R.ObjectId,MoveTemp(Inverse));To.Add(R);SelectedPavementMark.Invalidate();HidePreview();return;
 }

 FFoundrylinePlacementRequest Inverse;Inverse.ObjectId=R.ObjectId;
 const auto* Current=Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==R.ObjectId;});
 if(Current) {Inverse.Action=R.Action==EFoundrylinePlacementAction::Remove?EFoundrylinePlacementAction::RestoreRemoved:EFoundrylinePlacementAction::ReplaceObject;Inverse.Replacement=*Current;}
 else Inverse.Action=EFoundrylinePlacementAction::Remove;
 const auto Result=ExecuteConstruction(R);
 if(!Result.IsSuccess()) {Status=TEXT("History blocked by current connections or clearance");return;}
 From.Pop();To.Add(Inverse);HidePreview();
 if(!GetSelectedObject()) SelectedObject.Invalidate();
 Status=Redo?TEXT("Construction redone"):TEXT("Construction undone");
}
void AFoundrylineBuildSite::RecordSelection(FIntVector Cursor,bool Hover)
{
 const auto* Selected=GetSelectedObject();
 FFoundrylineOpening Opening;FGuid Room;
 if(Hover){if(const auto* Target=PickOpening(Cursor,Opening,Room)){if(!Selected||Selected->ObjectId!=Target->ObjectId||!GetSelectedOpening()||GetSelectedOpening()->OpeningId!=Opening.OpeningId)RecordOpeningOutline(*Target,Opening,Room,true);return;}}
 else if(Selected&&GetSelectedOpening()){RecordOpeningOutline(*Selected,*GetSelectedOpening(),SelectedRoom,false);return;}
 const auto* O=Hover?Pick(Cursor):Selected;
 if(!O || (Hover && Selected && O->ObjectId==Selected->ObjectId)) return;
 const auto* D=Placement->FindDefinition(O->DefinitionId);if(!D) return;
 if(!O->Pavement.Polygons.IsEmpty())return;
 const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,O->Anchor,O->QuarterTurns);
 SelectionOutlines.Add({FVector2D(B.Min.X-35,B.Min.Y-35),FVector2D(B.Max.X+35,B.Max.Y+35),SlabHeightCm+8.f,Hover,D->FunctionId==TEXT("ManufacturingSign")?300.f:0.f});
}
bool AFoundrylineBuildSite::IsPropertyBusy() const
{
 const auto* GI=GetGameInstance();const auto* S=GI?GI->GetSubsystem<UFoundrylineSaveSubsystem>():nullptr;
 return S && S->IsBusy();
}
bool AFoundrylineBuildSite::HasUnsavedChanges() const {return Placement && SavedRevision!=Placement->GetRevision();}
void AFoundrylineBuildSite::SaveProperty(const FString& Slot)
{
 auto* S=GetGameInstance()?GetGameInstance()->GetSubsystem<UFoundrylineSaveSubsystem>():nullptr;
 if(!S || !bReady) return;
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&R->HasMultipleBusinesses()){Status=TEXT("Multiple businesses require a combined world save");return;}
 SavingRevision=Placement->GetRevision();
 const auto E=S->SavePlacement(Placement,bCutPartsStarter&&Slot==TEXT("Property01")?TEXT("CutPartsProperty01"):Slot,true);
 if(E==EFoundrylineSaveError::None) {PropertyStatus=TEXT("SAVING...");Status=TEXT("Saving property 01");}
 else {SavingRevision=-1;Status=TEXT("Save unavailable. Wait for the current operation.");}
}
void AFoundrylineBuildSite::LoadProperty(const FString& Slot)
{
 auto* S=GetGameInstance()?GetGameInstance()->GetSubsystem<UFoundrylineSaveSubsystem>():nullptr;
 if(!S || !bReady) return;
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&R->HasMultipleBusinesses()){Status=TEXT("Multiple businesses require a combined world save");return;}
 SavingRevision=-1;
 const auto E=S->LoadPlacement(Placement,bCutPartsStarter&&Slot==TEXT("Property01")?TEXT("CutPartsProperty01"):Slot);
 if(E==EFoundrylineSaveError::None) {HidePreview();PropertyStatus=TEXT("LOADING...");Status=TEXT("Loading property 01");}
 else Status=TEXT("Load unavailable. Wait for the current operation.");
}
void AFoundrylineBuildSite::SaveCompleted(const FFoundrylineSaveResult& R)
{
 if(!R.IsSuccess()) {PropertyStatus=TEXT("OPERATION FAILED");Status=R.Error==EFoundrylineSaveError::ReadFailed?TEXT("No readable property save. Current construction kept."):TEXT("Save/load failed validation. Current property kept.");SavingRevision=-1;return;}
 if(SavingRevision>=0) {SavedRevision=SavingRevision;Status=TEXT("Property 01 saved to disk");}
 else {SavedRevision=Placement->GetRevision();UndoStack.Reset();RedoStack.Reset();SelectedObject.Invalidate();HidePreview();DoorStates.Reset();Status=TEXT("Property 01 loaded. Construction history reset.");}
 SavingRevision=-1;PropertyStatus=TEXT("SAVED / PROPERTY 01");
}
