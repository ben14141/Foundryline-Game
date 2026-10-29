// Station logistics: kit stock on the IN supply pallets, supply deliveries and unloading, and forklift collection of OUT pallets to storage or an order.
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "FoundrylineWeldingStationInternal.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;using Catalog=FFoundrylineFactoryCatalog;
using FoundrylineWeldingStationInternal::Overlap;using FoundrylineWeldingStationInternal::Access;}
bool FFoundrylineFactorySimulation::IsStationStock(const FFoundrylineFactoryPallet& P)const{
 return (P.Place==Place::Buffer||P.Place==Place::StationIn||P.Place==Place::StationOut)&&State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==P.Owner;});
}
int32 FFoundrylineFactorySimulation::StationStock(const FFoundrylineWeldingStationRecord& S,Part P,bool IncludeIncoming)const{
 int V=0;for(const auto& A:State.Pallets)if(A.Part==P&&A.Owner==S.Id&&(A.Place==Place::Buffer||A.Place==Place::StationIn))V+=A.Quantity;
 if(!IncludeIncoming)return V;
 V+=Incoming(P,S.Id);
 // A booked truck counts until its pallet is unloaded; then the pallet's unclaimed quantity does, then the forklift job.
 for(const auto& B:State.Commerce.Bookings)if(B.Inbound&&B.StationId==S.Id&&B.Part==uint8(P)&&B.Status!=EFoundrylineBookingState::Delivered){
  if(!B.SpawnedCargo)V+=B.Sheets;else for(const auto& A:State.Pallets)if(A.BookingId==B.Id&&A.Place==Place::Inbound)V+=A.Quantity-A.Reserved;}
 return V;
}
int32 FFoundrylineFactorySimulation::StationTarget(const FFoundrylineWeldingStationRecord& S,Part P)const{
 FFoundrylineAssemblyRecipe R;if(S.BatchRemaining<=0||!Catalog::Recipe(S.Recipe,R)||!R.Bill.IsValidIndex(int(P)))return 0;
 return FMath::Min(R.Bill[int(P)]*FMath::Min(S.BatchRemaining,2),Layout::BinCapacity(P));
}
int32 FFoundrylineFactorySimulation::StationRackSupply(Part P)const{
 int Free=0;for(const auto& A:State.Pallets)if(A.Part==P&&A.Place==Place::Rack&&!A.BookingId)Free+=A.Quantity-A.Reserved;
 // Accepted customer orders keep first claim on cut plates and brackets still in the racks.
 for(const auto& O:State.Commerce.Orders)if(O.Status==EFoundrylineOrderState::Accepted&&O.Part==uint8(P)){int Open=O.Quantity-O.Delivered;
  for(const auto& A:State.Pallets)if(A.OrderId==O.Id&&A.Place!=Place::Rack)Open-=A.Quantity;for(const auto& J:State.Jobs)if(J.OrderId==O.Id&&J.Phase<3)Open-=J.Quantity;Free-=FMath::Max(0,Open);}
 return FMath::Max(0,Free);
}
bool FFoundrylineFactorySimulation::StationPortBusy(const FFoundrylineWeldingStationRecord& S)const{
 return State.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::StationIn&&A.Owner==S.Id;})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==S.Id&&J.DestinationPlace==Place::StationIn;});
}
// An OUT pallet is collected whole when it is full, when its station will add nothing more (no batch and no unit on the
// bench), or when it completes what an accepted order for its product still needs beyond finished-goods storage.
bool FFoundrylineFactorySimulation::OutPalletReady(const FFoundrylineWeldingStationRecord& S,const FFoundrylineFactoryPallet& A)const{
 if(A.Place!=Place::StationOut||A.Owner!=S.Id||A.Reserved||A.BookingId||A.Quantity<1||!OutCollected(S))return false;
 if(A.Quantity>=Layout::RacksPerOutPallet||!S.Stage&&(!S.Recipe||S.BatchRemaining<=0))return true;
 int Stored=0;for(const auto& P:State.Pallets)if(P.Part==A.Part&&P.Place==Place::Rack&&!P.BookingId)Stored+=P.Quantity-P.Reserved;
 // For an order, wait until this pallet (with what is stored) covers everything it still needs, so one trip carries it.
 for(const auto& O:State.Commerce.Orders)if(O.Status==EFoundrylineOrderState::Accepted&&O.Part==uint8(A.Part)){
  int Need=O.Quantity-O.Delivered;for(const auto& B:State.Commerce.Bookings)if(!B.Inbound&&B.OrderId==O.Id&&B.Status!=EFoundrylineBookingState::Delivered)for(const auto& I:B.Manifest)Need-=I.Quantity;
  for(const auto& P:State.Pallets)if(P.Place==Place::Pickup&&P.OrderId==O.Id&&!P.BookingId)Need-=P.Quantity;for(const auto& J:State.Jobs)if(J.DestinationPlace==Place::Pickup&&J.OrderId==O.Id)Need-=J.Quantity;
  if(Need>Stored&&A.Quantity+Stored>=Need)return true;}
 return false;
}
// A forklift route that picks up from this station's OUT (new stations join every route; a player can take them off).
bool FFoundrylineFactorySimulation::OutCollected(const FFoundrylineWeldingStationRecord& S)const{return State.Routes.ContainsByPredicate([&](const auto& R){return R.Sources.Contains(S.Id);});}
// Where a finished product on OUT goes when no accepted order is collecting it: a finished-goods rack slot already holding
// that product with room, else an empty slot that is not a machine's supply rack.
int32 FFoundrylineFactorySimulation::FinishedGoodsSlot(Part P,int32 Count)const{
 // A stack of finished racks needs the headroom up to the next shelf in the same rack bay (a 130 cm shelf pitch takes 4 racks,
 // the top level any number); racks only, other products have no stack height here.
 auto Headroom=[&](const FFoundrylineFactorySlot& S){double Room=TNumericLimits<double>::Max();for(const auto& O:State.Slots)if(O.Id!=S.Id&&FVector::Dist2D(O.Position,S.Position)<60&&O.Position.Z>S.Position.Z+1)Room=FMath::Min(Room,O.Position.Z-S.Position.Z-10);return Room;};
 auto Fits=[&](const FFoundrylineFactorySlot& S,int32 Total){return P!=Part::HeadacheRack||Layout::PalletTopCm+Total*Layout::RackSizeCm().Z<=Headroom(S);};
 for(const auto& S:State.Slots){const int Have=Available(P,Place::Rack,S.Id)+Incoming(P,S.Id);if(S.Part==P&&!S.bInputSupply&&Have+Count<=Catalog::Part(P).Capacity&&Fits(S,Have+Count)&&!State.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::Rack&&A.Owner==S.Id&&A.Part!=P;}))return S.Id;}
 for(const auto& S:State.Slots)if(!S.bInputSupply&&Fits(S,Count)&&!State.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::Rack&&A.Owner==S.Id;})&&!State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==S.Id;}))return S.Id;
 return 0;
}
// IN takes one delivery at a time: a single-type pallet set down on its supply pallet and unloaded onto it before the next.
// OUT is collected automatically, so a batch does not stop after its first product.
void FFoundrylineFactorySimulation::PlanWeldingStations(){
 // A forklift takes a whole OUT pallet to finished-goods storage once it is ready (OutPalletReady); an empty pallet takes its
 // place. Accepted orders take whole OUT pallets straight to the pickup zone (PlanCommerce) when that fits what they still need.
 for(const auto& S:State.WeldingStations)for(const auto& A:State.Pallets)if(A.Place==Place::StationOut&&A.Owner==S.Id&&OutPalletReady(S,A)){
  auto* Slot=State.Slots.FindByPredicate([&,Id=FinishedGoodsSlot(A.Part,A.Quantity)](const auto& V){return V.Id==Id;});if(!Slot)continue;
  // An empty slot is only retagged for the product once the forklift job is actually booked.
  const Part Was=Slot->Part;Slot->Part=A.Part;if(!MakeJob(A.Id,Slot->Id,Place::Rack,A.Quantity,Slot->Dock,Slot->Position,85))Slot->Part=Was;break;}
 for(const auto& S:State.WeldingStations){if(StationPortBusy(S))continue;
  TArray<TPair<float,Part>> Wanted;for(Part P:Catalog::AssemblyParts()){const int Target=StationTarget(S,P),Short=StationShortfall(S,P);if(Short>0&&StationRackSupply(P)>0)Wanted.Add({float(Short)/Target,P});}
  Wanted.Sort([](const auto& A,const auto& B){return A.Key>B.Key;});bool Booked=false;
  for(const auto& W:Wanted){const int Short=FMath::Min(StationShortfall(S,W.Value),StationRackSupply(W.Value));
   for(const auto& A:State.Pallets)if(A.Part==W.Value&&A.Place==Place::Rack&&!A.BookingId&&A.Quantity>A.Reserved&&MakeJob(A.Id,S.Id,Place::StationIn,FMath::Min(Short,A.Quantity-A.Reserved),Layout::InPalletDock(S,Layout::InSlot(A.Part)),StationInDrop(S,A.Part),90)){Booked=true;break;}
   if(Booked)break;}
 }
}
void FFoundrylineFactorySimulation::UnloadWeldingStations(float Dt){
 for(auto& S:State.WeldingStations){auto* In=State.Pallets.FindByPredicate([&](const auto& A){return A.Place==Place::StationIn&&A.Owner==S.Id;});
  if(!In||In->Reserved){S.UnloadSeconds=0;continue;}
  S.UnloadSeconds+=Dt;if(S.UnloadSeconds<Layout::UnloadSeconds())continue;S.UnloadSeconds=0;
  // The empty pallet's return is abstracted into the unload; only the parts remain, in their bin.
  const int InId=In->Id;const Part P=In->Part;const float Lb=In->UnitLb;
  if(auto* Bin=State.Pallets.FindByPredicate([&](const auto& A){return A.Place==Place::Buffer&&A.Owner==S.Id&&A.Part==P&&FMath::IsNearlyEqual(A.UnitLb,Lb,.01f);})){Bin->Quantity+=In->Quantity;State.Pallets.RemoveAll([&](const auto& A){return A.Id==InId;});}
  else {In->Place=Place::Buffer;In->Position=Layout::BinPoint(S,P);In->BookingId=0;In->OrderId=0;}
 }
}
FVector FFoundrylineFactorySimulation::StationInDrop(const FFoundrylineWeldingStationRecord& S,Part P)const{
 const int32 Slot=Layout::InSlot(P);return Layout::InDrop(S,P,State.Pallets.ContainsByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::Buffer&&A.Quantity>0&&Layout::InSlot(A.Part)==Slot;}));
}
int32 FFoundrylineFactorySimulation::OutTotal(const FFoundrylineWeldingStationRecord& S)const{int32 N=0;for(int32 I=0;I<Layout::OutPallets;++I)N+=Layout::OutCount(S,State.Pallets,I);return N;}
// What is happening to the finished products on OUT, after Lead ("3 finished on OUT"); empty when OUT is clear.
FString FFoundrylineFactorySimulation::StationOutStatus(const FFoundrylineWeldingStationRecord& S,const FString& Lead)const{
 if(!OutTotal(S))return FString();
 if(State.Pallets.ContainsByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut&&A.Reserved;}))return Lead+TEXT(" / forklift collecting a pallet");
 if(!OutCollected(S))return Lead+TEXT(" / add this station to a forklift route");
 if(!State.Drivers.ContainsByPredicate([](const auto& D){return D.EmployeeId!=0;}))return Lead+TEXT(" / assign a forklift driver");
 const auto* Ready=State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut&&OutPalletReady(S,A);});
 if(!Ready)return Lead+FString::Printf(TEXT(" / collected at %d per pallet"),Layout::RacksPerOutPallet);
 if(!FinishedGoodsSlot(Ready->Part,Ready->Quantity))return Lead+TEXT(" / no free rack slot for a pallet");
 return Lead+TEXT(" / waiting for a forklift");
}
