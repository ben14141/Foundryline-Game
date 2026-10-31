#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"

namespace {
using Kind=EFoundrylinePlannerItem;using Place=EFoundrylinePalletPlace;
bool Overlap(const FBox& A,const FBox& B){return A.Min.X<B.Max.X-.1&&A.Max.X>B.Min.X+.1&&A.Min.Y<B.Max.Y-.1&&A.Max.Y>B.Min.Y+.1;}
FBox RackAccess(const FFoundrylinePlannerItem& I){
 if(I.Kind==Kind::StorageRack){const auto B=FFoundrylinePlanningEquipment::Service(I.Center,I.Quarter,FMath::Max(I.Bounds.GetSize().X,I.Bounds.GetSize().Y));return FBox(FVector(B.Min,20),FVector(B.Max,300));}
 if(I.Kind==Kind::FixedRack){const auto B=FFoundrylinePlanningEquipment::Bounds(I.Center+FRotator(0,I.Quarter*90,0).RotateVector({242.88,0,0}),I.Quarter,{365.76,416,0});return FBox(FVector(B.Min,20),FVector(B.Max,300));}
 return FBox(ForceInit);
}
bool Apply(FFoundrylineFactorySimulation& M,Kind K,int Id,FVector At,bool Remove,FString& E){
 auto& S=M.State;const auto Items=FFoundrylinePlannerItems::List(S);const auto* Item=Items.FindByPredicate([&](const auto& V){return V.Kind==K&&V.Id==Id;});
 if(!Item){E=TEXT("That item no longer exists in this business");return false;}
 if(S.Running){E=TEXT("Pause production before changing the floor layout");return false;}
 if(!S.Jobs.IsEmpty()){E=TEXT("Let the current forklift deliveries finish before moving or removing equipment");return false;}
 if(At.ContainsNaN()||At.GetAbsMax()>1000000||FMath::Abs(At.Z-20)>.1){E=TEXT("Choose a valid floor position");return false;}
 auto BusySlots=[&](const TArray<int32>& Slots){return S.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Rack&&Slots.Contains(P.Owner);});};
 auto ClearRoutes=[&](const TArray<int32>& Endpoints){for(auto& R:S.Routes){R.Sources.RemoveAll([&](int N){return Endpoints.Contains(N);});R.Destinations.RemoveAll([&](int N){return Endpoints.Contains(N);});}};
 auto ClearPeople=[&](int Target){for(auto& O:S.Operators)if(O.Machine==Target){O.Machine=0;O.WorkPoint=O.Target=O.Home;O.Path.Reset();O.Distance=O.Retry=O.Speed=0;}for(auto& W:S.Walkers)if(W.Zone==Target||W.Station==Target){W.Zone=W.Station=W.StationSide=0;W.Path.Reset();W.Distance=W.Retry=W.Speed=0;}};
 const FVector Delta=At-Item->Center;TSet<int32> MovedSlots;
 if(K==Kind::Pallet){
  auto* P=S.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!P||P->Reserved||P->BookingId||P->Place==Place::Forks){E=TEXT("This pallet is reserved or already booked for shipping");return false;}
  if(Remove){S.ScrapLb+=P->Quantity*P->UnitLb;S.Pallets.RemoveAll([&](const auto& V){return V.Id==Id;});return true;}
  const FFoundrylineFactorySlot* Slot=nullptr;double Best=250;
  for(const auto& V:S.Slots){const double D=FVector::Dist2D(V.Position,At);if(D<Best&&V.Part==P->Part&&!S.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::Rack&&A.Owner==V.Id;})){Best=D;Slot=&V;}}
  if(!Slot){E=TEXT("Choose an empty rack slot assigned to this part; a forklift will move the pallet");return false;}
  if(S.Drivers.IsEmpty()){E=TEXT("Assign a forklift driver before requesting a pallet move");return false;}
  if(!M.RequestPlanningPalletMove(P->Id,Slot->Id,E)){E=TEXT("No valid forklift move could be scheduled");return false;}
  return true;
 }
 if(K==Kind::StorageRack||K==Kind::FixedRack){
  if(BusySlots(Item->Slots)){E=TEXT("Empty this rack before moving or removing it");return false;}
  if(Remove){ClearRoutes(Item->Slots);S.Slots.RemoveAll([&](const auto& V){return Item->Slots.Contains(V.Id);});S.StorageRacks.RemoveAll([&](const auto& V){return K==Kind::StorageRack&&V.Id==Id;});}
  else {for(auto& Slot:S.Slots)if(Item->Slots.Contains(Slot.Id)){Slot.Position+=Delta;Slot.Dock+=Delta;MovedSlots.Add(Slot.Id);}if(auto* R=S.StorageRacks.FindByPredicate([&](const auto& V){return K==Kind::StorageRack&&V.Id==Id;}))R->Center=At;}
 }
 else if(K==Kind::Machine){
  auto* Machine=S.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!Machine)return false;
  if(Machine->Remaining>0||Machine->ReservedLb>0||Machine->SheetLoad>0){E=TEXT("Finish the current machine cycle before relocating or storing it");return false;}
  TArray<int32> Slots;for(const auto& Slot:S.Slots)if(Slot.MachineId==Id)Slots.Add(Slot.Id);
  if(BusySlots(Slots)||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::MachineInput||P.Place==Place::MachineOutput);})) {E=TEXT("Empty the machine input, output and attached racks first");return false;}
  if(Remove){if(Machine->bStored){E=TEXT("This machine is already in equipment storage");return false;}Machine->StoredSlots.Reset();for(const auto& Slot:S.Slots)if(Slots.Contains(Slot.Id))Machine->StoredSlots.Add(Slot);S.Slots.RemoveAll([&](const auto& V){return Slots.Contains(V.Id);});Machine->bStored=true;Machine->Enabled=false;ClearPeople(Id);}
  else {
   if(Machine->bStored){for(auto Slot:Machine->StoredSlots){Slot.Position+=Delta;Slot.Dock+=Delta;S.Slots.Add(Slot);MovedSlots.Add(Slot.Id);}Machine->StoredSlots.Reset();Machine->bStored=false;}
   else for(auto& Slot:S.Slots)if(Slots.Contains(Slot.Id)){Slot.Position+=Delta;Slot.Dock+=Delta;MovedSlots.Add(Slot.Id);}
   Machine->Origin+=Delta;Machine->InputDock+=Delta;Machine->InputStock+=Delta;Machine->OutputDock+=Delta;Machine->OutputStock+=Delta;
  }
 }
 else if(K==Kind::Station){
  const int Index=S.WeldingStations.IndexOfByPredicate([&](const auto& V){return V.Id==Id;});if(Index<0)return false;
  if(Remove)return M.RemoveWeldingStation(Id,E);
  if(S.WeldingStations[Index].Stage||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::Buffer||P.Place==Place::StationIn||P.Place==Place::StationOut);})||S.Collectors.ContainsByPredicate([&](const auto& C){return C.Area==Id;})){E=TEXT("Empty the station and release its collector before relocating it");return false;}
  auto Station=S.WeldingStations[Index];S.WeldingStations.RemoveAt(Index);Station.Center=At;
  if(!M.CanPlaceWeldingStation(Station,E))return false;S.WeldingStations.Insert(Station,Index);
  for(auto& Bin:S.SupplyBins)if(Bin.Area==Id){Bin.Position+=Delta;Bin.Access+=Delta;}
 }
 else if(K==Kind::WorkZone){
  const int Index=S.Zones.IndexOfByPredicate([&](const auto& V){return V.Id==Id;});if(Index<0)return false;auto Z=S.Zones[Index];
  if(Z.Ordered||Z.ActiveStep>=0||Z.InstalledLb>0||Z.ReservedLb>0||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&P.Place==Place::Buffer;})||S.Collectors.ContainsByPredicate([&](const auto& C){return C.Area==Id;})){E=TEXT("Complete the product and empty/release the work zone before editing it");return false;}
  S.Zones.RemoveAt(Index);
  if(Remove){ClearPeople(Id);ClearRoutes({Id});S.SupplyBins.RemoveAll([&](const auto& B){return B.Area==Id;});}
  else {Z.Min+=Delta;Z.Max+=Delta;Z.DropMin+=Delta;Z.DropMax+=Delta;if(!M.CanPlaceZone(Z,E))return false;S.Zones.Insert(Z,Index);for(auto& B:S.SupplyBins)if(B.Area==Id){B.Position+=Delta;B.Access+=Delta;}}
 }
 else if(K==Kind::Forklift){
  auto* D=S.Drivers.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!D)return false;
  if(D->Job||FVector::Dist2D(D->Position,D->Parking)>25){E=TEXT("Park this forklift and finish its load before editing it");return false;}
  if(Remove){if(D->EmployeeId){E=TEXT("Release the forklift driver's assignment before removing the vehicle");return false;}S.Drivers.RemoveAll([&](const auto& V){return V.Id==Id;});}
  else {D->Position=D->Parking=At;D->Path.Reset();D->Distance=0;}
 }
 else if(K==Kind::Lane){auto* L=S.Lanes.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!L)return false;if(Remove)S.Lanes.RemoveAll([&](const auto& V){return V.Id==Id;});else {L->Min+=FVector2D(Delta);L->Max+=FVector2D(Delta);}}
 else {E=TEXT("This item is not editable");return false;}
 if(!Remove){
  const auto After=FFoundrylinePlannerItems::List(S);
  for(const auto& A:After){const bool IsSelf=A.Kind==K&&A.Id==Id;const bool Attached=A.Kind==Kind::FixedRack&&A.Slots.ContainsByPredicate([&](int Slot){return MovedSlots.Contains(Slot);});if((!IsSelf&&!Attached)||A.Stored)continue;
   const FBox2D Floor(S.FloorMin,S.FloorMax);const FBox Access=RackAccess(A);if(!Floor.IsInsideOrOn(FVector2D(A.Bounds.Min))||!Floor.IsInsideOrOn(FVector2D(A.Bounds.Max))||(Access.IsValid&&(!Floor.IsInsideOrOn(FVector2D(Access.Min))||!Floor.IsInsideOrOn(FVector2D(Access.Max))))){E=TEXT("Keep the complete item and its loading aisle inside the usable shop floor");return false;}
   for(const auto& B:After){if(B.Stored||(B.Kind==K&&B.Id==Id)||(B.Kind==Kind::FixedRack&&B.Slots.ContainsByPredicate([&](int Slot){return MovedSlots.Contains(Slot);})))continue;
    if(A.Kind==Kind::Lane&&B.Kind==Kind::Lane)continue;
    if(B.Kind!=Kind::Lane&&B.Kind!=Kind::Pallet&&((Access.IsValid&&Overlap(Access,B.Bounds))||(RackAccess(B).IsValid&&Overlap(A.Bounds,RackAccess(B))))){E=TEXT("Keep the rack loading aisle clear; shared aisles may overlap");return false;}
    if((A.Kind==Kind::Machine&&B.Kind==Kind::Lane)||(A.Kind==Kind::FixedRack&&B.Kind==Kind::Lane))continue;
    if(Overlap(A.Bounds,B.Bounds)){E=TEXT("That position overlaps another item or a reserved work area");return false;}
   }
  }
 }
 for(auto& D:S.Drivers){D.Path.Reset();D.Distance=0;}for(auto& O:S.Operators){O.Path.Reset();O.Distance=O.Retry=0;}
 for(auto& W:S.Walkers){W.Path.Reset();W.Distance=W.Retry=0;}
 M.Solids=FFoundrylineWorkshopLayout::PlantSolids(S);return true;
}
}
bool FFoundrylineFactorySimulation::EditPlanningItem(int32 KindId,int32 Id,FVector At,bool Remove,FString& E){
 if(KindId<1||KindId>8){E=TEXT("Choose an existing planner item");return false;}
 auto Candidate=*this;if(!Apply(Candidate,EFoundrylinePlannerItem(KindId),Id,At,Remove,E)||!Candidate.Valid(E))return false;
 ++Candidate.State.Revision;*this=MoveTemp(Candidate);return true;
}

bool FFoundrylineFactorySimulation::RequestPlanningPalletMove(int32 Pallet,int32 Slot,FString& E){
 const auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Pallet;});const auto* S=State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});
 if(!P||!S||P->Reserved||P->BookingId||P->Part!=S->Part||Available(P->Part,EFoundrylinePalletPlace::Rack,Slot)>0){E=TEXT("Choose unreserved stock and an empty matching rack slot");return false;}
 return MakeJob(Pallet,Slot,EFoundrylinePalletPlace::Rack,P->Quantity,S->Dock,S->Position,100);
}
