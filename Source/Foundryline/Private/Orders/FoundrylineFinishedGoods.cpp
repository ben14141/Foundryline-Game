#include "Production/FoundrylineFactorySimulation.h"
#include "Logistics/FoundrylineRackAccess.h"
#include "Production/FoundrylineWeldingStationLayout.h"
namespace {using Place=EFoundrylinePalletPlace;}
int32 FFoundrylineFactorySimulation::FinishedGoodsSlotForOrder(EFoundrylinePart Part,int32 Count,int32 Order,int32 Ignore)const{
 for(const auto& S:State.Slots){if(S.Id==Ignore||!FFoundrylineRackAccess::FinishedSlot(State,S.Id))continue;
  if(State.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Rack&&P.Owner==S.Id;})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.DestinationPlace==Place::Rack&&J.Destination==S.Id;}))continue;
  // Never mix order assignments or stack a second physical pallet in one slot.
  if(Count>FFoundrylineFactoryCatalog::Part(Part).Capacity)continue;
  if(Part==EFoundrylinePart::HeadacheRack){double Headroom=MAX_dbl;for(const auto& Other:State.Slots)if(Other.Id!=S.Id&&FVector::Dist2D(Other.Position,S.Position)<60&&Other.Position.Z>S.Position.Z+1)Headroom=FMath::Min(Headroom,Other.Position.Z-S.Position.Z-10);if(FFoundrylineWeldingStationLayout::PalletTopCm+Count*FFoundrylineWeldingStationLayout::RackSizeCm().Z>Headroom)continue;}
  return S.Id;
 }return 0;
}
bool FFoundrylineFactorySimulation::PrepareOrderStock(int32 Pallet,int32 Order,FString& Error){
 auto* O=State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Id==Order&&V.Status==EFoundrylineOrderState::Accepted;});
 auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Pallet;});
 if(!O||!P||uint8(P->Part)!=O->Part||P->BookingId||P->Reserved||P->Quantity<1||P->OrderId&&P->OrderId!=Order||(P->Place!=Place::Rack&&P->Place!=Place::MachineOutput&&P->Place!=Place::StationOut&&P->Place!=Place::Pickup)){Error=TEXT("Choose matching, unreserved stock for this accepted order");return false;}
 int Need=O->Quantity-O->Delivered;for(const auto& B:State.Commerce.Bookings)if(!B.Inbound&&B.OrderId==Order&&B.Status!=EFoundrylineBookingState::Delivered)for(const auto& I:B.Manifest)Need-=I.Quantity;
 for(const auto& V:State.Pallets)if(FFoundrylineRackAccess::Ready(State,V)&&V.OrderId==Order&&!V.BookingId)Need-=V.Quantity;
 for(const auto& J:State.Jobs)if(J.DestinationPlace==Place::Rack&&J.OrderId==Order)Need-=J.Quantity;
 if(Need<=0){Error=TEXT("The required stock is already in finished goods or booked");return false;}
 const int Count=FMath::Min(Need,P->Quantity);
 if(FFoundrylineRackAccess::Ready(State,*P)&&Count==P->Quantity){P->OrderId=Order;State.Commerce.PickupOrder=Order;return true;}
 const int Slot=FinishedGoodsSlotForOrder(P->Part,Count,Order,P->Place==Place::Rack?P->Owner:0);
 auto* Target=State.Slots.FindByPredicate([&](const auto& S){return S.Id==Slot;});if(!Target){Error=TEXT("Place a finished-goods rack with a free pallet position first");return false;}
 if(!State.Drivers.ContainsByPredicate([](const auto& D){return D.EmployeeId&&D.Priorities.IsValidIndex(2)&&D.Priorities[2]>=0;})){Error=TEXT("Assign a forklift driver and enable Shipping in their priorities");return false;}
 if(P->Place==Place::StationOut&&Count!=P->Quantity){Error=TEXT("Store the assembly pallet first before assigning a smaller quantity");return false;}
 const auto Was=Target->Part;Target->Part=P->Part;if(!MakeJob(Pallet,Slot,Place::Rack,Count,Target->Dock,Target->Position,90)){Target->Part=Was;Error=TEXT("Finished-goods transfer is blocked or already reserved");return false;}
 State.Jobs.Last().OrderId=Order;State.Commerce.PickupOrder=Order;State.Running=true;return true;
}
void FFoundrylineFactorySimulation::PlanFinishedGoods(){
 // Redirect legacy queued floor deliveries before a driver can unload them.
 for(auto& J:State.Jobs)if(J.DestinationPlace==Place::Pickup){const int Slot=FinishedGoodsSlotForOrder(J.Part,J.Quantity,J.OrderId);auto* S=State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});if(!S)continue;S->Part=J.Part;J.Destination=Slot;J.DestinationPlace=Place::Rack;J.Drop=S->Dock;J.StoredAt=S->Position;
  if(auto* D=State.Drivers.FindByPredicate([&](const auto& V){return V.Id==J.Driver;})){if(J.Phase>=3){J.Phase=3;D->Path=Path(D->Position,J.Drop);D->Distance=0;}D->Waiting=false;}}
 // Rehome legacy floor pallets through real forklift jobs, preserving shipments.
 for(const auto& P:State.Pallets)if(P.Place==Place::Pickup&&!P.Reserved){const int Slot=FinishedGoodsSlotForOrder(P.Part,P.Quantity,P.OrderId);auto* S=State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});if(!S)continue;const auto Was=S->Part;S->Part=P.Part;if(MakeJob(P.Id,Slot,Place::Rack,P.Quantity,S->Dock,S->Position,100)){auto& J=State.Jobs.Last();J.OrderId=P.OrderId;J.BookingId=P.BookingId;}else S->Part=Was;}
}
