#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylineLoadingBayLayout.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Grid/FoundrylineFloorPlacement.h"

// A loading-door edit cannot move a reserved pallet or a carrier's committed work.
// Re-evaluate only when construction changes, after all freight using the old bay clears.
void AFoundrylineFactoryPlant::RefreshShippingPickup(){
 if(!HasAuthority()||!Model.State.Commerce.Enabled||!Model.CanRelocateShippingPickup())return;
 const auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();if(!Placement||Placement->GetRevision()==ShippingPickupPlacementRevision)return;
 const auto Revision=Placement->GetRevision();
 bool FoundPreset=false;
 for(int Pass=0;Pass<2;++Pass)for(const auto& Object:Placement->GetObjects()){
  if(Object.CompanyId!=Model.State.CompanyId||Object.PlantId!=Model.State.PlantId||Object.bOfficeAnnex)continue;
  const auto* Definition=Placement->FindDefinition(Object.DefinitionId);if(!Definition||Definition->FunctionId!=TEXT("BuildingShell"))continue;
  for(const auto& Opening:Object.Openings){
   if(Opening.Kind!=EFoundrylineOpeningKind::LoadingDock||!Opening.LoadingTier)continue;
   if((Pass==0)!=(Opening.OpeningId==Model.State.PickupOpeningId))continue;FoundPreset=true;
   const FTransform Building(FRotator(0,Object.QuarterTurns*90,0),FVector(Object.Anchor));
   const FVector Wall=FFoundrylineLoadingBayLayout::Point(*Definition,Opening,0,0,20);
   const FVector Out=Building.TransformVectorNoScale(FFoundrylineLoadingBayLayout::Point(*Definition,Opening,0,1,20)-Wall).GetSafeNormal();
   auto Candidate=Model;Candidate.State.bPresetPickup=true;Candidate.State.PickupOpeningId=Opening.OpeningId;
   Candidate.State.PickupOrigin=GetActorTransform().InverseTransformPosition(Building.TransformPosition(Wall)-Out*243.84);Candidate.State.PickupOrigin.Z=20;
   Candidate.State.PickupYaw=GetActorTransform().InverseTransformVectorNoScale(Out).Rotation().Yaw;
   const auto Bounds=Candidate.ShippingPickupBounds();FString Error;
   if(!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),Bounds,Error))continue;
   if(Candidate.Solids.ContainsByPredicate([&](const auto& Solid){return FFoundrylineFloorPlacement::Overlap(Solid,Bounds);}))continue;
   if(Candidate.OverlapsWeldingStation(Bounds,true))continue;
   if(Candidate.State.Zones.ContainsByPredicate([&](const auto& Z){return FFoundrylineFloorPlacement::Overlap(FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)),Bounds);}))continue;
   Model.State.FloorMin=Candidate.State.FloorMin;Model.State.FloorMax=Candidate.State.FloorMax;
   Model.State.bPresetPickup=true;Model.State.bPickupLayoutPending=false;Model.State.PickupOpeningId=Candidate.State.PickupOpeningId;Model.State.PickupOrigin=Candidate.State.PickupOrigin;Model.State.PickupYaw=Candidate.State.PickupYaw;
   ++Model.State.Revision;ShippingPickupPlacementRevision=Revision;return;
  }
 }
 if(!FoundPreset){const bool WasPreset=Model.State.bPresetPickup||Model.State.bPickupLayoutPending;Model.State.bPresetPickup=false;Model.State.bPickupLayoutPending=false;Model.State.PickupOpeningId.Invalidate();if(WasPreset)++Model.State.Revision;ShippingPickupPlacementRevision=Revision;}
 else if(!Model.State.bPickupLayoutPending){Model.State.bPickupLayoutPending=true;++Model.State.Revision;}
 // A blocked preset remains pending. Moving its obstructing equipment can make it usable
 // without another door edit; do not permanently cache the failed geometry check.
}
