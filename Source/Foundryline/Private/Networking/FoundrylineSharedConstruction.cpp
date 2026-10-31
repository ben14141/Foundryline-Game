#include "UI/FoundrylinePlayerController.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Construction/FoundrylineBuildSite.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool AFoundrylinePlayerController::SubmitConstruction(const TArray<FFoundrylinePlacementRequest>& Requests)
{
 if(!IsLocalController() || bConstructionPending || IsWalking() || Requests.IsEmpty() || Requests.Num()>128) return false;
 bConstructionPending=true;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) It->ClearConstructionFeedback();
 ServerConstruct(Requests,++ConstructionSequence);return true;
}
void AFoundrylinePlayerController::ServerConstruct_Implementation(const TArray<FFoundrylinePlacementRequest>& Requests,uint32 Sequence)
{
 auto Error=EFoundrylinePlacementError::InvalidAction;
 if(Sequence>LastServerConstructionSequence) {
  LastServerConstructionSequence=Sequence;
  const double Now=GetWorld()->GetTimeSeconds();
  if(Now-ConstructionRateWindow>=1) {ConstructionRateWindow=Now;ConstructionRateCount=0;}
  if(++ConstructionRateCount>16) Error=EFoundrylinePlacementError::Busy;
  else if(!IsWalking()) for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {Error=It->ApplyRemoteConstruction(this,Requests).Error;break;}
 }
 ClientConstructionResult(Sequence,uint8(Error));
}
void AFoundrylinePlayerController::ClientConstructionResult_Implementation(uint32 Sequence,uint8 Error)
{
 if(Sequence!=ConstructionSequence) return;
 bConstructionPending=false;LastConstructionError=Error;++ConstructionReplyCount;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {It->ReceiveConstructionResult(EFoundrylinePlacementError(Error));break;}
}
FFoundrylinePlanResult AFoundrylineBuildSite::ApplyRemoteConstruction(APlayerController* Player,const TArray<FFoundrylinePlacementRequest>& Requests)
{
 FFoundrylinePlanResult Result;Result.Error=EFoundrylinePlacementError::AccessDenied;
 if(!HasAuthority()) {Result.Error=EFoundrylinePlacementError::NotAuthority;return Result;}
 const auto* State=GetWorld()->GetGameState();
 if(!bReady || !Player || Player->GetWorld()!=GetWorld() || !Player->PlayerState || !State || !State->PlayerArray.Contains(Player->PlayerState) || Player->PlayerState->IsOnlyASpectator()) return Result;
 if(IsPropertyBusy()) {Result.Error=EFoundrylinePlacementError::Busy;return Result;}
 if(Requests.IsEmpty() || Requests.Num()>128) {Result.Error=EFoundrylinePlacementError::TooManyActions;return Result;}
 // Only door relocation uses a two-operation atomic request. No client context,
 // price, mesh path, permission, history restoration or property load is accepted.
 const bool RoutingBatch=Requests.ContainsByPredicate([](const auto& R){return R.Action!=EFoundrylinePlacementAction::SetTruckZones && R.Action!=EFoundrylinePlacementAction::SetRouteNodes;})==false;
 const bool PavementBatch=Requests.Num()<=64&&Requests.ContainsByPredicate([&](const auto& R){
  if(R.Action==EFoundrylinePlacementAction::Build)return !R.DefinitionId.ToString().StartsWith(TEXT("Pavement_"));
  if(R.Action!=EFoundrylinePlacementAction::Remove&&R.Action!=EFoundrylinePlacementAction::ReplaceObject)return true;
  const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==R.ObjectId;});if(!O||O->Pavement.Polygons.IsEmpty())return true;
  return R.Action==EFoundrylinePlacementAction::ReplaceObject&&!R.Replacement.DefinitionId.ToString().StartsWith(TEXT("Pavement_"));
 })==false;
 if(Requests.Num()>1 && !RoutingBatch && !PavementBatch && (Requests.Num()!=2 || !(Requests[0].Action==EFoundrylinePlacementAction::RemoveOpening && Requests[1].Action==EFoundrylinePlacementAction::AddOpening && Requests[0].ObjectId==Requests[1].ObjectId))) {Result.Error=EFoundrylinePlacementError::InvalidAction;return Result;}
 for(const auto& R:Requests) {
  if(uint8(R.Action)>uint8(EFoundrylinePlacementAction::SetRouteNodes) || R.Action==EFoundrylinePlacementAction::RestoreRemoved) {Result.Error=EFoundrylinePlacementError::InvalidAction;return Result;}
  auto OversizedPave=[](const auto& D){if(D.Polygons.Num()>32||D.Markings.Num()>128||D.RoadControls.Num()>(D.Spline?32:3))return true;int Count=0;for(const auto& P:D.Polygons){if(P.Outer.Num()>(D.Spline?1536:512)||P.Holes.Num()>16)return true;Count+=P.Outer.Num();for(const auto& H:P.Holes){if(H.Points.Num()>512)return true;Count+=H.Points.Num();}}for(const auto& M:D.Markings)if(M.Points.Num()>(D.Spline&&M.RoadGenerated?513:129))return true;return Count>4096;};
  if(OversizedPave(R.Pavement)||OversizedPave(R.Replacement.Pavement)){Result.Error=EFoundrylinePlacementError::TooManyActions;return Result;}
  auto Oversized=[](const auto& Spaces){if(Spaces.Num()>256) return true;for(const auto& Space:Spaces) if(Space.Openings.Num()>256) return true;return false;};
  auto OversizedNodes=[](const auto& Nodes){if(Nodes.Num()>64) return true;for(const auto& N:Nodes) if(N.Links.Num()>8) return true;return false;};
  if(OversizedNodes(R.RouteNodes) || OversizedNodes(R.Replacement.RouteNodes) || R.TruckZones.Num()>64 || R.Replacement.TruckZones.Num()>64 || R.Openings.Num()>256 || R.Crosswalks.Num()>256 || Oversized(R.OfficeAreas) || R.Replacement.Openings.Num()>256 || R.Replacement.Crosswalks.Num()>256 || Oversized(R.Replacement.OfficeAreas) || Oversized(R.Replacement.OfficeRooms)) {Result.Error=EFoundrylinePlacementError::TooManyActions;return Result;}
  const auto Definition=R.Action==EFoundrylinePlacementAction::Build?R.DefinitionId:R.Action==EFoundrylinePlacementAction::ReplaceObject?R.Replacement.DefinitionId:NAME_None;
  if(!Definition.IsNone() && !Placement->FindDefinition(Definition) && !Placement->EnsureGeneratedDefinition(Definition)) {Result.Error=EFoundrylinePlacementError::UnknownDefinition;return Result;}
 }
 // This cooperative starter property has one server-assigned company/plant.
 // ValidateAgainst checks every target against that trusted Context and revision.
 // Execution stages the whole action before broadcasting any visual changes.
 FFoundrylinePlacementContext Trusted=Context;if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld()))if(!R->ConstructionContext(Player,Trusted))return Result;
 FString StoreyError;if(!OfficeStoreyRequestsClear(Requests,StoreyError)){Status=StoreyError;Result.Error=EFoundrylinePlacementError::Occupied;return Result;}
 return Placement->ExecuteBatch(Requests,Trusted,MAX_int64);
}
