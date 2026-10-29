#include "Production/FoundrylineFactorySimulation.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Production/FoundrylineWeldingStationLayout.h"
namespace {using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using BS=EFoundrylineBookingState;}
void FFoundrylineFactorySimulation::TickCommerce(float Dt){
 auto& C=State.Commerce;if(!C.Enabled)return;C.Clock+=Dt;
 int Working=0;for(const auto& M:State.Machines)if(M.Enabled&&M.Remaining>0&&MachineStaffReady(M.Id))++Working;for(const auto& D:State.Drivers)if(D.Job)Working+=3;
 C.ExpenseFraction+=Dt*Working;const int64 Cost=int64(C.ExpenseFraction);if(Cost){C.ExpenseFraction-=Cost;CommerceExpense(Cost,TEXT("Equipment operating costs"));}
 for(auto& O:C.Orders)if(O.Status==EFoundrylineOrderState::Accepted&&O.Deadline>0&&C.Clock>O.Deadline)O.Late=true;
 for(auto& B:C.Bookings){
  if(B.Status==BS::Booked&&C.Clock>=B.DispatchAt)B.Status=BS::Approaching;
  if(B.Status==BS::Approaching&&C.Clock>=B.ArrivalAt)B.Status=BS::Waiting;
  if(B.Status==BS::Departing&&C.Clock>=B.DepartedAt+20){if(C.ActiveBooking==B.Id)C.ActiveBooking=0;if(B.Inbound){B.Status=BS::Delivered;B.DeliveredAt=C.Clock;}else{B.Status=BS::InTransit;B.DeliveredAt=C.Clock+FFoundrylineCommerceCatalog::Carrier(B.Carrier).TransitMinutes;for(auto& P:State.Pallets)if(P.Place==Place::Outbound&&P.Owner==B.Id)P.Place=Place::InTransit;}}
  if(B.Status==BS::InTransit&&C.Clock>=B.DeliveredAt){auto* O=C.Orders.FindByPredicate([&](const auto& V){return V.Id==B.OrderId;});if(!O)continue;int Quantity=0;for(const auto& I:B.Manifest)Quantity+=I.Quantity;O->Delivered+=Quantity;C.EarnedRevenueCents+=Quantity*O->UnitPriceCents;
   for(const auto& P:State.Pallets)if(P.Place==Place::InTransit&&P.Owner==B.Id)State.FinishedSteelLb+=P.UnitLb*P.Quantity;State.Pallets.RemoveAll([&](const auto& P){return P.Place==Place::InTransit&&P.Owner==B.Id;});B.Status=BS::Delivered;
   const bool Complete=O->Delivered>=O->Quantity;const int64 Payment=O->Payment==EFoundrylinePayment::PerDelivery?Quantity*O->UnitPriceCents:O->Payment==EFoundrylinePayment::OnCompletion&&Complete?O->Quantity*O->UnitPriceCents:0;if(Payment){O->PaidCents+=Payment;CommerceIncome(Payment,TEXT("Customer payment / ")+O->Reference);}if(Complete){O->Status=EFoundrylineOrderState::Completed;O->Late=O->Deadline>0&&C.Clock>O->Deadline;}
  }
 }
 if(!C.ActiveBooking){FFoundrylineFreightBooking* First=nullptr;for(auto& B:C.Bookings)if(B.Status==BS::Waiting&&(!First||B.ArrivalAt<First->ArrivalAt||B.ArrivalAt==First->ArrivalAt&&B.Id<First->Id))First=&B;if(First){C.ActiveBooking=First->Id;First->Status=BS::Loading;First->BerthAt=C.Clock;First->DockedAt=C.Clock+12;}}
 if(auto* B=C.Bookings.FindByPredicate([&](const auto& V){return V.Id==C.ActiveBooking;})){if(B->Status==BS::Loading&&C.Clock>=B->DockedAt){
  if(B->Inbound&&!B->SpawnedCargo&&State.Pallets.Num()<63){FFoundrylineBoughtPart Bought;const bool Parts=B->Part&&FFoundrylineFactoryCatalog::Bought(Part(B->Part),&Bought);const int Id=AddPallet(Parts?Part(B->Part):Part::Sheet,B->Sheets,Place::Inbound,10,CarrierPalletPosition(0),Parts?Bought.UnitLb:FFoundrylineSteelSheet::PoundsEach());auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;});P->BookingId=B->Id;B->Manifest.Add({Id,uint8(P->Part),B->Sheets,P->UnitLb});State.ReceivedSteelLb+=B->Sheets*P->UnitLb;B->SpawnedCargo=true;}
  const bool InboundDone=B->Inbound&&B->SpawnedCargo&&!State.Pallets.ContainsByPredicate([&](const auto& P){return P.BookingId==B->Id&&P.Place==Place::Inbound;});
  int Loaded=0;for(const auto& P:State.Pallets)if(P.Place==Place::Outbound&&P.Owner==B->Id)++Loaded;
  if(InboundDone||!B->Inbound&&Loaded==B->Manifest.Num()&&Loaded>0){B->Status=BS::Departing;B->DepartedAt=C.Clock;}
 }}
 // Keep the live simulation bounded; aggregate earned revenue/costs survive history trimming.
 while(C.Bookings.Num()>64){const int I=C.Bookings.IndexOfByPredicate([](const auto& B){return B.Status==BS::Delivered;});if(I==INDEX_NONE)break;C.Bookings.RemoveAt(I);}
 while(C.Orders.Num()>32){const int I=C.Orders.IndexOfByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Completed;});if(I==INDEX_NONE)break;if(C.PickupOrder==C.Orders[I].Id)C.PickupOrder=0;C.Orders.RemoveAt(I);}
 RefreshCommerceOffers();
}
void FFoundrylineFactorySimulation::PlanCommerce(){
 auto& C=State.Commerce;if(!C.Enabled)return;
 auto* B=C.Bookings.FindByPredicate([&](const auto& V){return V.Id==C.ActiveBooking&&V.Status==BS::Loading&&C.Clock>=V.DockedAt;});
 // Bought assembly parts go from the truck straight to the ordering station's IN port, once that port is free.
 if(B&&B->Inbound&&B->StationId){const auto* Station=State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==B->StationId;});
  for(const auto& P:State.Pallets)if(Station&&!StationPortBusy(*Station)&&P.Place==Place::Inbound&&P.BookingId==B->Id&&P.Quantity>P.Reserved&&MakeJob(P.Id,Station->Id,Place::StationIn,P.Quantity-P.Reserved,FFoundrylineWeldingStationLayout::InPalletDock(*Station,FFoundrylineWeldingStationLayout::InSlot(P.Part)),StationInDrop(*Station,P.Part),100)){State.Jobs.Last().Task=EFoundrylineForkTask::Truck;State.Jobs.Last().BookingId=B->Id;break;}}
 else if(B&&B->Inbound){for(const auto& P:State.Pallets)if(P.Place==Place::Inbound&&P.BookingId==B->Id&&P.Quantity>P.Reserved){for(const auto& S:State.Slots)if(S.Part==Part::Sheet){const int Room=10-Available(Part::Sheet,Place::Rack,S.Id)-Incoming(Part::Sheet,S.Id);if(Room>0&&MakeJob(P.Id,S.Id,Place::Rack,FMath::Min(Room,P.Quantity-P.Reserved),S.Dock,S.Position,100)){State.Jobs.Last().Task=EFoundrylineForkTask::Truck;State.Jobs.Last().BookingId=B->Id;break;}}}}
 if(B&&!B->Inbound){for(int I=0;I<B->Manifest.Num();++I){const auto& Item=B->Manifest[I];const auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Item.PalletId&&V.Place==Place::Pickup&&V.BookingId==B->Id;});if(P&&MakeJob(P->Id,B->Id,Place::Outbound,P->Quantity,CarrierDock(I),CarrierPalletPosition(I),100)){auto& J=State.Jobs.Last();J.Task=EFoundrylineForkTask::Truck;J.OrderId=B->OrderId;J.BookingId=B->Id;}}}
 auto* O=C.Orders.FindByPredicate([&](const auto& V){return V.Id==C.PickupOrder&&V.Status==EFoundrylineOrderState::Accepted;});if(!O)return;
 int Need=O->Quantity-O->Delivered;for(const auto& Book:C.Bookings)if(!Book.Inbound&&Book.OrderId==O->Id&&Book.Status!=BS::Delivered)for(const auto& I:Book.Manifest)Need-=I.Quantity;for(const auto& P:State.Pallets)if(P.Place==Place::Pickup&&P.OrderId==O->Id&&!P.BookingId)Need-=P.Quantity;for(const auto& J:State.Jobs)if(J.DestinationPlace==Place::Pickup&&J.OrderId==O->Id)Need-=J.Quantity;
 for(int Slot=0;Slot<2&&Need>0;++Slot){const int Destination=1000+Slot;if(State.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Pickup&&P.Owner==Destination;})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.DestinationPlace==Place::Pickup&&J.Destination==Destination;}))continue;
  for(const auto& P:State.Pallets)if(P.Part==Part(O->Part)&&(P.Place==Place::Rack||P.Place==Place::StationOut&&!P.Reserved&&P.Quantity<=Need&&State.Routes.ContainsByPredicate([&](const auto& R){return R.Sources.Contains(P.Owner);}))&&!P.BookingId&&P.Quantity>P.Reserved){
   // A pallet on a welding station's OUT goes whole, and only when the order still needs all of it; otherwise it goes to storage first.
   const int Q=P.Place==Place::StationOut?P.Quantity:FMath::Min(Need,P.Quantity-P.Reserved);const auto Position=PickupPosition(Slot);if(MakeJob(P.Id,Destination,Place::Pickup,Q,Position-FVector(136,0,0),Position,50)){State.Jobs.Last().OrderId=O->Id;Need-=Q;break;}}
 }
}
bool FFoundrylineFactorySimulation::CommerceValid(FString& E)const{
 const auto& C=State.Commerce;if(!C.Enabled)return true;
 auto Fail=[&](){E=TEXT("Invalid order, booking or money snapshot");return false;};
 if(!State.bCutPartsStarter||!FMath::IsFinite(C.Clock)||C.Clock<0||C.Clock>1.e12||C.CashCents<0||C.CashCents>1000000000000LL||C.UnpaidCents<0||C.CostCents<0||C.EarnedRevenueCents<0||C.OfferSerial<0||!FMath::IsFinite(C.ExpenseFraction)||C.ExpenseFraction<0||C.ExpenseFraction>=1.001||C.Orders.Num()>48||C.Bookings.Num()>96||C.Money.Num()>32)return Fail();
 TSet<int> Ids;for(const auto& P:State.Pallets)Ids.Add(P.Id);for(const auto& Z:State.Zones)Ids.Add(Z.Id);for(const auto& D:State.Drivers)Ids.Add(D.Id);for(const auto& J:State.Jobs)Ids.Add(J.Id);for(const auto& L:State.Lanes)Ids.Add(L.Id);for(const auto& W:State.Walkers)Ids.Add(W.Id);for(const auto& O:State.Operators)Ids.Add(O.Id);for(const auto& D:State.UnassignedEmployees)Ids.Add(D.Id);for(const auto& D:State.Drivers)if(D.EmployeeId)Ids.Add(D.EmployeeId);
 for(const auto& O:C.Orders){
  if(O.Id<1||O.Id>=State.NextId||Ids.Contains(O.Id)||!FFoundrylineCommerceCatalog::Saleable(O.Part)||O.Quantity<1||O.Quantity>1000||O.Delivered<0||O.Delivered>O.Quantity||O.UnitPriceCents<1||O.UnitPriceCents>100000||O.PaidCents<0||O.PaidCents>O.Quantity*O.UnitPriceCents||int(O.Status)>2||int(O.Payment)>2||O.Customer.Len()>80||O.Reference.Len()>40||!FMath::IsFinite(O.Deadline)||!FMath::IsFinite(O.AllowedMinutes)||O.AllowedMinutes<0||O.AllowedMinutes>10080||!FMath::IsFinite(O.AcceptedAt)||O.AcceptedAt<0||O.AcceptedAt>C.Clock)return Fail();
  Ids.Add(O.Id);
  if(O.Status==EFoundrylineOrderState::Offered){if(O.Delivered||O.PaidCents||O.AcceptedAt||O.Deadline)return Fail();}
  else {const double Deadline=O.AllowedMinutes?O.AcceptedAt+O.AllowedMinutes:0;if(!FMath::IsNearlyEqual(O.Deadline,Deadline,.001))return Fail();const int64 Paid=O.Payment==EFoundrylinePayment::Upfront?O.Quantity*O.UnitPriceCents:O.Payment==EFoundrylinePayment::PerDelivery?O.Delivered*O.UnitPriceCents:O.Status==EFoundrylineOrderState::Completed?O.Quantity*O.UnitPriceCents:0;if(O.PaidCents!=Paid)return Fail();}
  if((O.Status==EFoundrylineOrderState::Completed)!=(O.Delivered==O.Quantity))return Fail();
 }
 int Occupying=0;
 for(const auto& B:C.Bookings){
  if(B.Id<1||B.Id>=State.NextId||Ids.Contains(B.Id)||B.Carrier<0||B.Carrier>3||int(B.Status)>6||B.OriginalFeeCents<0||B.OriginalFeeCents>100000||B.Manifest.Num()>20||B.Customer.Len()>80||!FMath::IsFinite(B.ArrivalAt)||!FMath::IsFinite(B.DispatchAt)||!FMath::IsFinite(B.BerthAt)||!FMath::IsFinite(B.DockedAt)||B.BerthAt<0||B.DockedAt<B.BerthAt||!FMath::IsFinite(B.DepartedAt)||!FMath::IsFinite(B.DeliveredAt)||B.ArrivalAt<0||B.DispatchAt<0||B.DepartedAt<0||B.DeliveredAt<0||B.DispatchAt>B.ArrivalAt)return Fail();
  Ids.Add(B.Id);if(B.Status==BS::Loading||B.Status==BS::Departing){++Occupying;if(C.ActiveBooking!=B.Id)return Fail();}
  const auto* O=C.Orders.FindByPredicate([&](const auto& V){return V.Id==B.OrderId;});
  if(B.Inbound){const bool Steel=!B.Part;if(Steel?B.Sheets>10||B.StationId:int(B.Part)>=FFoundrylineFactoryCatalog::PartCount||!FFoundrylineFactoryCatalog::Bought(Part(B.Part))||B.Sheets>FFoundrylineFactoryCatalog::Part(Part(B.Part)).Capacity||B.Status!=BS::Delivered&&!State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==B.StationId;}))return Fail();
   if(B.Sheets<1||B.OrderId||B.Status==BS::InTransit||B.Manifest.Num()>1||B.SpawnedCargo!=!B.Manifest.IsEmpty())return Fail();}
  else if(B.Manifest.IsEmpty()||!O&&B.Status!=BS::Delivered||O&&O->Status==EFoundrylineOrderState::Offered)return Fail();
  int Spaces=0;double Weight=0;TSet<int> Pallets;
  for(const auto& I:B.Manifest){
   if(I.PalletId<1||I.PalletId>=State.NextId||int(I.Part)>=FFoundrylineFactoryCatalog::PartCount||I.Quantity<1||I.Quantity>FFoundrylineFactoryCatalog::Part(Part(I.Part)).Capacity||!FMath::IsFinite(I.UnitLb)||I.UnitLb<0||Pallets.Contains(I.PalletId)||B.Inbound&&(I.Part!=B.Part||I.Quantity!=B.Sheets)||!B.Inbound&&O&&I.Part!=O->Part)return Fail();
   if(!B.Inbound&&!FFoundrylineCommerceCatalog::CarrierFits(B.Carrier,I.Part))return Fail();
   Pallets.Add(I.PalletId);Spaces+=FFoundrylineCommerceCatalog::PalletSpaces(I.Part);Weight+=FFoundrylineCommerceCatalog::PalletWeight(I.Part,I.Quantity,I.UnitLb);
   if(!B.Inbound&&B.Status!=BS::Delivered&&!State.Pallets.ContainsByPredicate([&](const auto& P){return P.Id==I.PalletId&&P.BookingId==B.Id&&P.OrderId==B.OrderId&&P.Quantity==I.Quantity&&uint8(P.Part)==I.Part&&FMath::IsNearlyEqual(P.UnitLb,I.UnitLb,.001f);}))return Fail();
  }
  const auto Spec=FFoundrylineCommerceCatalog::Carrier(B.Carrier);if(Spaces>Spec.Spaces||Weight>Spec.PayloadLb+.01)return Fail();
 }
 if(Occupying>1||bool(C.ActiveBooking)!=(Occupying==1))return Fail();
 if(C.PickupOrder&&!C.Orders.ContainsByPredicate([&](const auto& O){return O.Id==C.PickupOrder&&O.Status!=EFoundrylineOrderState::Offered;}))return Fail();
 for(const auto& O:C.Orders){int Committed=O.Delivered;for(const auto& B:C.Bookings)if(!B.Inbound&&B.OrderId==O.Id&&B.Status!=BS::Delivered)for(const auto& I:B.Manifest)Committed+=I.Quantity;if(Committed>O.Quantity)return Fail();}
 for(const auto& D:State.Drivers){if(D.Priorities.Num()!=3||D.Priorities[0]<0||D.Priorities[0]>2||D.Priorities[1]<0||D.Priorities[1]>2||D.Priorities[2]<0||D.Priorities[2]>2||D.Priorities[0]==D.Priorities[1]||D.Priorities[0]==D.Priorities[2]||D.Priorities[1]==D.Priorities[2])return Fail();}
 for(const auto& P:State.Pallets){if(int(P.Place)>int(Place::StationOut)||
P.BookingId&&!C.Bookings.ContainsByPredicate([&](const auto& B){return B.Id==P.BookingId&&B.Status!=BS::Delivered;}))return Fail();if(P.Place==Place::Pickup&&(!P.OrderId||!C.Orders.ContainsByPredicate([&](const auto& O){return O.Id==P.OrderId&&O.Part==uint8(P.Part)&&O.Status==EFoundrylineOrderState::Accepted;})))return Fail();}
 for(const auto& J:State.Jobs)if(int(J.Task)>2)return Fail();
 for(const auto& M:C.Money)if(!FMath::IsFinite(M.At)||M.At<0||M.At>C.Clock||M.Label.Len()>160)return Fail();
 if(!State.Machines.ContainsByPredicate([](const auto& M){return M.Id==1;})||C.BrakeOwned!=State.Machines.ContainsByPredicate([](const auto& M){return M.Id==2;}))return Fail();
 return true;
}
