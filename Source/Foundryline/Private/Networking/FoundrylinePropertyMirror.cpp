#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::ApplyNetworkSnapshot(
 const TArray<FFoundrylinePlacedObject>& InObjects,const TArray<FFoundrylineBuildRegion>& InRegions,int64 InRevision)
{
 if(!GetWorld() || GetWorld()->GetNetMode()!=NM_Client) return EFoundrylinePlacementError::NotAuthority;
 if(bApplyingChanges) return EFoundrylinePlacementError::Busy;
 if(InRevision<Revision || InRevision<=0) return EFoundrylinePlacementError::StaleRevision;
 if(InRevision==Revision) return EFoundrylinePlacementError::None;
 auto Error=ValidateRegions(InRegions);if(Error!=EFoundrylinePlacementError::None) return Error;
 TSet<FGuid> Seen;
 for(const auto& O:InObjects)
 {
  if(!O.ObjectId.IsValid() || Seen.Contains(O.ObjectId)) return EFoundrylinePlacementError::DuplicateId;
  Seen.Add(O.ObjectId);
  if(!EnsureGeneratedDefinition(O.DefinitionId)) return EFoundrylinePlacementError::UnknownDefinition;
 }
 for(const auto& O:InObjects) {Error=ValidateObject(O,InObjects,InRegions);if(Error!=EFoundrylinePlacementError::None) return Error;}
 const auto Previous=Objects;
 Objects=InObjects;Regions=InRegions;Revision=InRevision;
 TGuardValue<bool> Guard(bApplyingChanges,true);
 for(const auto& Old:Previous) if(!Objects.ContainsByPredicate([&](const auto& O){return O.ObjectId==Old.ObjectId;}))
 {
  FFoundrylinePlacementChange C;C.ObjectId=Old.ObjectId;C.bHadPrevious=true;C.Previous=Old;C.bAffectsNavigation=true;
  C.PreviousBounds=FFoundrylineGrid::RotateBounds(FindDefinition(Old.DefinitionId)->Footprint,Old.Anchor,Old.QuarterTurns);OnPlacementChanged.Broadcast(C);
 }
 for(const auto& O:Objects)
 {
  const auto* Old=Previous.FindByPredicate([&](const auto& V){return V.ObjectId==O.ObjectId;});
  if(Old && FFoundrylinePlacedObject::StaticStruct()->CompareScriptStruct(Old,&O,0)) continue;
  FFoundrylinePlacementChange C;C.ObjectId=O.ObjectId;C.bHasCurrent=true;C.Current=O;C.bAffectsNavigation=true;
  C.CurrentBounds=FFoundrylineGrid::RotateBounds(FindDefinition(O.DefinitionId)->Footprint,O.Anchor,O.QuarterTurns);
  if(Old) {C.bHadPrevious=true;C.Previous=*Old;C.PreviousBounds=FFoundrylineGrid::RotateBounds(FindDefinition(Old->DefinitionId)->Footprint,Old->Anchor,Old->QuarterTurns);}
  OnPlacementChanged.Broadcast(C);
 }
 return EFoundrylinePlacementError::None;
}
