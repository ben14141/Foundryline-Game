#include "Construction/FoundrylineBuildSite.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UI/FoundrylinePlayerController.h"

FString AFoundrylineBuildSite::PlacementErrorText(EFoundrylinePlacementError E)
{
 switch(E) {
 case EFoundrylinePlacementError::None:return TEXT("Release to place");
 case EFoundrylinePlacementError::Pending:return TEXT("Checking placement...");
 case EFoundrylinePlacementError::Occupied:return TEXT("Overlaps construction or another opening");
 case EFoundrylinePlacementError::AccessDenied:return TEXT("Outside your build area, setback, or permission");
 case EFoundrylinePlacementError::StaleRevision:return TEXT("The property changed. Try the placement again");
 case EFoundrylinePlacementError::NotAuthority:return TEXT("Send construction changes through the server");
 case EFoundrylinePlacementError::UnknownObject:return TEXT("That object no longer exists. Select it again");
 case EFoundrylinePlacementError::InvalidTransform:return TEXT("Check size, snapping, wall attachment and clearance");
 case EFoundrylinePlacementError::Busy:return TEXT("Property is busy. Try again shortly");
 case EFoundrylinePlacementError::InvalidRegion:return TEXT("No valid building plot at this location");
 case EFoundrylinePlacementError::CostLimitExceeded:return TEXT("This exceeds the construction budget");
 case EFoundrylinePlacementError::TooManyActions:return TEXT("Too many changes in one action. Use smaller sections");
 case EFoundrylinePlacementError::InvalidIdentity:return TEXT("Construction ownership is unavailable");
 case EFoundrylinePlacementError::UnknownDefinition:
 case EFoundrylinePlacementError::InvalidDefinition:return TEXT("This construction item is unavailable");
 default:return TEXT("This change is unavailable. Check the selected item");
 }
}
FFoundrylinePlacementResult AFoundrylineBuildSite::ValidateConstruction(const FFoundrylinePlacementRequest& R)
{
 const auto Batch=Placement->PreviewBatch({R},LocalContext(),true);
 FFoundrylinePlacementResult Result;Result.Error=Batch.Error;Result.Revision=Batch.Revision;RecordConstructionResult(Result.Error,false);return Result;
}
FFoundrylinePlanResult AFoundrylineBuildSite::ExecuteConstructionBatch(const TArray<FFoundrylinePlacementRequest>& Requests)
{
 if(GetNetMode()==NM_Client) {
  FFoundrylinePlanResult Result;Result.Error=EFoundrylinePlacementError::NotAuthority;
  for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It) if(It->IsLocalController()) {Result.Error=It->SubmitConstruction(Requests)?EFoundrylinePlacementError::Pending:EFoundrylinePlacementError::Busy;break;}
  if(Result.Error!=EFoundrylinePlacementError::Pending) RecordConstructionResult(Result.Error,true);
  return Result;
 }
 FFoundrylinePlacementContext Trusted=Context;if(auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());Registry&&Registry->IsWorldBound()){Trusted={};for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It)if(It->IsLocalController()){Registry->ConstructionContext(*It,Trusted);break;}}
 FString StoreyError;if(!OfficeStoreyRequestsClear(Requests,StoreyError)){Status=StoreyError;FFoundrylinePlanResult Blocked;Blocked.Error=EFoundrylinePlacementError::Occupied;return Blocked;}
 auto Result=Placement->ExecuteBatch(Requests,Trusted,MAX_int64);RecordConstructionResult(Result.Error,true);return Result;
}
FFoundrylinePlacementResult AFoundrylineBuildSite::ExecuteConstruction(const FFoundrylinePlacementRequest& R)
{
 const auto Batch=ExecuteConstructionBatch({R});FFoundrylinePlacementResult Result;Result.Error=Batch.Error;Result.Revision=Batch.Revision;
 if(Batch.IsSuccess() && !Batch.Actions.IsEmpty()) Result=Batch.Actions[0];return Result;
}
bool AFoundrylineBuildSite::SentToHost(EFoundrylinePlacementError Error)
{
 // Clients submit construction to the host. The reply arrives as feedback, so waiting is not a failure.
 if(Error!=EFoundrylinePlacementError::Pending) return false;
 HidePreview();Status=TEXT("Sent to the host. Waiting for confirmation...");return true;
}
void AFoundrylineBuildSite::ReceiveConstructionResult(EFoundrylinePlacementError Error) {RecordConstructionResult(Error,true);if(Error==EFoundrylinePlacementError::None) Status=TEXT("Continue building or choose a tool");}

void AFoundrylineBuildSite::RecordConstructionResult(EFoundrylinePlacementError Error,bool Commit)
{
 if(Error==EFoundrylinePlacementError::Pending) return;
 if(Commit) {ActionError=Error;ActionReason.Empty();ActionFeedbackUntil=Error==EFoundrylinePlacementError::None?-1:GetWorld()->GetRealTimeSeconds()+3.5;}
 else {bHasPreviewFeedback=true;PreviewError=Error;DraftReason.Empty();}
}
void AFoundrylineBuildSite::DraftBlocked(const FString& Reason,bool Commit)
{
 bHasPreviewFeedback=true;PreviewError=EFoundrylinePlacementError::InvalidTransform;DraftReason=Reason;
 if(Commit) {ActionError=PreviewError;ActionReason=Reason;ActionFeedbackUntil=GetWorld()->GetRealTimeSeconds()+3.5;}
}
bool AFoundrylineBuildSite::HasActionFeedback() const {return GetWorld() && GetWorld()->GetRealTimeSeconds()<ActionFeedbackUntil;}
void AFoundrylineBuildSite::ClearConstructionFeedback() {bHasPreviewFeedback=false;DraftReason.Empty();ActionReason.Empty();ActionFeedbackUntil=-1;}
bool AFoundrylineBuildSite::IsConstructionFeedbackError() const {return HasActionFeedback()?ActionError!=EFoundrylinePlacementError::None:bHasPreviewFeedback && PreviewError!=EFoundrylinePlacementError::None;}
FString AFoundrylineBuildSite::GetConstructionFeedbackTitle() const
{
 if(HasActionFeedback()) return TEXT("NOT PLACED");
 if(bHasPreviewFeedback) return PreviewError==EFoundrylinePlacementError::None?TEXT("READY TO PLACE"):TEXT("PLACEMENT BLOCKED");
 return TEXT("CONSTRUCTION");
}
FString AFoundrylineBuildSite::GetConstructionFeedbackText() const
{
 for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It) if(It->IsLocalController() && It->IsConstructionPending()) return TEXT("Checking placement...");
 if(HasActionFeedback()) return ActionReason.IsEmpty()?PlacementErrorText(ActionError):ActionReason;
 if(bHasPreviewFeedback && PreviewError!=EFoundrylinePlacementError::None) return DraftReason.IsEmpty()?PlacementErrorText(PreviewError):DraftReason;
 return Status;
}
