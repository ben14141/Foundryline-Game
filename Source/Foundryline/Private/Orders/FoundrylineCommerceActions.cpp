#include "Production/FoundrylineFactorySimulation.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Production/FoundrylineWorkshopLayout.h"
namespace {using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;using BS=EFoundrylineBookingState;
template<typename T> bool CanOfferPart(const T& S,uint8 P){
 if(!FFoundrylineFactoryCatalog::AvailableNow(Part(P)))return false;
 // Headache racks are only offered to a business that has a welding station to build them.
 if(P==uint8(Part::HeadacheRack))return !S.WeldingStations.IsEmpty();
 if(P!=uint8(Part::BentBracket)&&P!=uint8(Part::SupportChannel))return true;
 return S.Commerce.BrakeOwned&&S.Machines.ContainsByPredicate([](const auto& M){return M.Kind()==2;})&&S.Operators.ContainsByPredicate([&](const auto& O){return S.Machines.ContainsByPredicate([&](const auto& M){return M.Kind()==2&&M.Id==O.Machine&&!M.bStored;});});
}
}
FVector FFoundrylineFactorySimulation::PickupPosition(int32 S){return {S%2?-725.:-975.,2000.,20};}
FVector FFoundrylineFactorySimulation::ShippingPickupPosition(int32 Slot)const{return State.bPresetPickup?State.PickupOrigin+FRotator(0,State.PickupYaw,0).RotateVector(FVector(0,Slot%2?125.:-125.,0)):PickupPosition(Slot);}
FVector FFoundrylineFactorySimulation::ShippingPickupDock(int32 Slot)const{return ShippingPickupPosition(Slot)-FRotator(0,State.bPresetPickup?State.PickupYaw:0,0).RotateVector(FVector(136,0,0));}
FBox2D FFoundrylineFactorySimulation::ShippingPickupBounds()const{FBox2D Bounds(ForceInit);for(int Slot=0;Slot<2;++Slot){Bounds+=FVector2D(ShippingPickupPosition(Slot));Bounds+=FVector2D(ShippingPickupDock(Slot));}return Bounds.ExpandBy(80);}
bool FFoundrylineFactorySimulation::CanRelocateShippingPickup()const{return !State.Pallets.ContainsByPredicate([](const auto& P){return P.Place==Place::Pickup||P.Place==Place::Outbound||P.Place==Place::InTransit||P.BookingId;})&&!State.Jobs.ContainsByPredicate([](const auto& J){return J.DestinationPlace==Place::Pickup||J.DestinationPlace==Place::Outbound;})&&!State.Commerce.Bookings.ContainsByPredicate([](const auto& B){return B.Status!=BS::Delivered;});}
FVector FFoundrylineFactorySimulation::CarrierPalletPosition(int32 S,int32 Carrier,bool Wide){if(Carrier==4)return {1260+(Wide?0:S%2?-64.:64.),1700.+1400-1460.4+(S/2)*122,167};return {1200,S%2?1765.:1635.,133};}
FVector FFoundrylineFactorySimulation::CarrierDock(int32 S,int32 Carrier,bool Wide){return CarrierPalletPosition(S,Carrier,Wide)-FVector(136,0,Carrier==4?147:113);}
void FFoundrylineFactorySimulation::CommerceExpense(int64 C,const FString& Label){if(C<=0)return;auto& S=State.Commerce;S.CostCents+=C;const int64 Paid=FMath::Min(S.CashCents,C);S.CashCents-=Paid;S.UnpaidCents+=C-Paid;// Running costs (operating and wages) post every game minute: each merges into its own line among the trailing running-cost
 // lines, so the ledger keeps one line of each between real transactions.
 auto Running=[](const FString& L){return L==TEXT("Equipment operating costs")||L==TEXT("Wages");};int32 Into=INDEX_NONE;
 if(Running(Label))for(int32 I=S.Money.Num()-1;I>=0&&I>=S.Money.Num()-2&&Running(S.Money[I].Label);--I)if(S.Money[I].Label==Label){Into=I;break;}
 if(Into!=INDEX_NONE){S.Money[Into].Cents-=C;S.Money[Into].At=S.Clock;}else S.Money.Add({S.Clock,-C,Label});if(S.Money.Num()>32)S.Money.RemoveAt(0);}
void FFoundrylineFactorySimulation::CommerceIncome(int64 C,const FString& Label){if(C<=0)return;auto& S=State.Commerce;const int64 Debt=FMath::Min(C,S.UnpaidCents);S.UnpaidCents-=Debt;S.CashCents+=C-Debt;S.Money.Add({S.Clock,C,Label});if(S.Money.Num()>32)S.Money.RemoveAt(0);}
void FFoundrylineFactorySimulation::SeedCommerce(){
 auto& C=State.Commerce;C={};C.Enabled=true;State.FloorMax.X=1600;
 FFoundrylineFactoryLane Lane;Lane.Id=State.NextId++;Lane.Kind=EFoundrylineFloorLane::Forklift;Lane.Min={-300,1450};Lane.Max={1250,1950};State.Lanes.Add(Lane);
 FFoundrylineFactoryLane Access;Access.Id=State.NextId++;Access.Kind=EFoundrylineFloorLane::Forklift;Access.Min={-1300,1450};Access.Max={-300,1810};State.Lanes.Add(Access);
 FFoundrylineFactoryLane People;People.Id=State.NextId++;People.Kind=EFoundrylineFloorLane::Pedestrian;People.Min={270,200};People.Max={370,1350};State.Lanes.Add(People);
 if(State.Routes.Num()){auto& R=State.Routes[0];R.Destinations.AddUnique(12);for(int I=0;I<4;++I){R.Sources.AddUnique(1000+I);R.Destinations.AddUnique(1000+I);}}
 for(int I=0;I<State.Drivers.Num();++I){State.Drivers[I].Parking={100,I?1100.:500.,20};State.Drivers[I].Position=State.Drivers[I].Parking;State.Drivers[I].Path.Reset();State.Drivers[I].Distance=0;}
 RefreshCommerceOffers();C.Money.Add({C.Clock,FFoundrylineEconomyTuning::StartingCashCents,TEXT("Starting working capital")});
}
void FFoundrylineFactorySimulation::RefreshCommerceOffers(){
 auto& C=State.Commerce;if(!C.Enabled)return;
 C.Orders.RemoveAll([&](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&!CanOfferPart(State,O.Part);});
 // Unaccepted listings follow current balance data; accepted agreements retain their signed price.
 for(auto& O:C.Orders)if(O.Status==EFoundrylineOrderState::Offered){const int Modifier=O.Part==uint8(Part::HeadacheRack)?100:O.Rush?140:O.Payment==EFoundrylinePayment::Upfront?90:100;O.UnitPriceCents=FFoundrylineCommerceCatalog::SalePrice(O.Part)*Modifier/100;}

 // Headache-rack offers first, so part offers never crowd them out: RackOffersOnSale stay open while the business has a welding
 // station, each replaced as soon as it is accepted.
 for(int Racks=C.Orders.FilterByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::HeadacheRack);}).Num();CanOfferPart(State,uint8(Part::HeadacheRack))&&Racks<FFoundrylineEconomyTuning::RackOffersOnSale&&C.Orders.Num()<48;++Racks){
  const int I=C.RackOfferSerial++;FFoundrylineCustomerOrder O;O.Id=State.NextId++;O.Part=uint8(Part::HeadacheRack);O.Quantity=FFoundrylineEconomyTuning::RackOrderMin+I*3%(FFoundrylineEconomyTuning::RackOrderMax-FFoundrylineEconomyTuning::RackOrderMin+1);
  const TCHAR* Customers[]={TEXT("Ridgeline Truck Outfitters"),TEXT("Crossroads Fleet Service"),TEXT("Prairie Pickup Accessories")};O.Customer=Customers[I%3];
  O.AllowedMinutes=4320;O.Payment=I%3==2?EFoundrylinePayment::PerDelivery:EFoundrylinePayment::OnCompletion;O.UnitPriceCents=FFoundrylineCommerceCatalog::SalePrice(O.Part);O.Reference=FString::Printf(TEXT("FL-R%03d"),I+1);C.Orders.Add(O);}
 int Offered=0;for(const auto& O:C.Orders)if(O.Status==EFoundrylineOrderState::Offered)++Offered;
 while(Offered<8&&C.Orders.Num()<48){const int I=C.OfferSerial++;FFoundrylineCustomerOrder O;O.Id=State.NextId++;const Part Choices[]={Part::Bracket,Part::MountingPlate,Part::BentBracket};O.Part=uint8(Choices[I%UE_ARRAY_COUNT(Choices)]);if(!CanOfferPart(State,O.Part))continue;O.Quantity=I==0?40:I%6==0?50:I%6==1?60:I%6==2?75:30;const TCHAR* Customers[]={TEXT("Hartwell Fabrication"),TEXT("Northfield Repairs"),TEXT("Red Cedar Equipment"),TEXT("Mason Agricultural"),TEXT("Alder Workshop"),TEXT("Union Conveyor Co.")};O.Customer=Customers[I%6];O.Email=I%4==3;O.Rush=I%5==4;O.AllowedMinutes=I%5==2?0:O.Rush?360:2880;O.Payment=I%4==3?EFoundrylinePayment::Upfront:I%3==1?EFoundrylinePayment::PerDelivery:EFoundrylinePayment::OnCompletion;O.UnitPriceCents=FFoundrylineCommerceCatalog::SalePrice(O.Part)*(O.Rush?140:O.Payment==EFoundrylinePayment::Upfront?90:100)/100;O.Reference=FString::Printf(TEXT("FL-%04d"),I+1);C.Orders.Add(O);++Offered;}

}
bool FFoundrylineFactorySimulation::InstallBrake(FString& E){
 auto& C=State.Commerce;if(C.BrakeOwned||C.CashCents<FFoundrylineEconomyTuning::PressBrakeCents||C.UnpaidCents){E=TEXT("Press brake costs $4500; clear outstanding costs first");return false;}
 // The brake bay, its feed lane and operator walkways must be clear of welding stations.
 for(const FBox2D& Bay:{FBox2D({-2310,1336},{-1364,2170}),FBox2D({-1450,1450},{-300,1750}),FBox2D({-2310,140},{-2190,2170})})if(OverlapsWeldingStation(Bay,false)){E=TEXT("A welding station occupies the press-brake bay; move it before installing the brake");return false;}
 FFoundrylineFactoryMachine M;M.Id=2;M.Origin={-1900,1850,20};M.InputDock={-1364,1850,20};M.InputStock={-1500,1850,60};M.OutputDock={-1900,1336,20};M.OutputStock={-1900,1200,20};M.Auto=false;M.Making=Part::BentBracket;State.Machines.Add(M);
 for(const auto& P:FFoundrylineFactoryCatalog::DefaultPrograms())if(P.Machine==2&&P.Part==Part::BentBracket)State.Programs.Add(P);
 for(int I=0;I<1;++I){FFoundrylineFactorySlot S;S.Id=110+I;S.Part=I?Part::ReinforcementPlate:Part::Bracket;S.Position={-1500,1850,190.+I*130};S.Dock=M.InputDock;S.MachineId=2;S.bInputSupply=true;State.Slots.Add(S);for(auto& R:State.Routes){R.Sources.AddUnique(S.Id);R.Destinations.AddUnique(S.Id);R.Sources.AddUnique(2);R.Destinations.AddUnique(2);}}
 FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=EFoundrylineFloorLane::Forklift;L.Min={-1450,1450};L.Max={-300,1750};State.Lanes.Add(L);
 Solids=FFoundrylineWorkshopLayout::PlantSolids(State);CommerceExpense(FFoundrylineEconomyTuning::PressBrakeCents,TEXT("Press brake installation"));C.BrakeOwned=true;C.BrakeInstallMinutes=FFoundrylineEconomyTuning::BrakeInstallMinutes;AddOperatorAccessLanes(2);return true;
}
bool FFoundrylineFactorySimulation::CommerceCommand(const FFoundrylineCommerceCommand& A,FString& E){
 if(!State.Commerce.Enabled||A.PlantId.IsValid()&&A.PlantId!=State.PlantId){E=TEXT("Operations unavailable for this plant");return false;}
 auto Backup=State;const auto OldSolids=Solids;auto Apply=[&]()->bool{
 auto& C=State.Commerce;auto* O=C.Orders.FindByPredicate([&](const auto& V){return V.Id==A.Id;});
 if(A.Action==Act::AcceptOrder){if(!O||O->Status!=EFoundrylineOrderState::Offered){E=TEXT("This offer is no longer available");return false;}if(!CanOfferPart(State,O->Part)){E=O->Part==uint8(Part::HeadacheRack)?TEXT("This order needs a welding station to build headache racks"):TEXT("This order requires an installed press brake with an assigned operator");return false;}int Active=0;for(const auto& V:C.Orders)Active+=V.Status==EFoundrylineOrderState::Accepted;if(Active>=6){E=TEXT("Finish an active order before accepting another");return false;}O->Status=EFoundrylineOrderState::Accepted;O->AcceptedAt=C.Clock;O->Deadline=O->AllowedMinutes>0?C.Clock+O->AllowedMinutes:0;if(O->Payment==EFoundrylinePayment::Upfront){O->PaidCents=O->Quantity*O->UnitPriceCents;CommerceIncome(O->PaidCents,TEXT("Customer advance / ")+O->Reference);}RefreshCommerceOffers();return true;}
 if(A.Action==Act::BuySteel){if(A.Quantity<1||A.Quantity>10||C.Bookings.Num()>=96){E=TEXT("Choose one to ten sheets");return false;}const int64 Steel=FMath::RoundToInt64(FFoundrylineSteelSheet::PoundsEach()*A.Quantity*FFoundrylineEconomyTuning::SteelCentsPerLb);if(C.UnpaidCents||C.CashCents<Steel+2500){E=TEXT("Insufficient available cash for steel and delivery");return false;}const double ETA=A.Arrival==0?C.Clock+60:A.Arrival;if(!FMath::IsFinite(ETA)||ETA<C.Clock+30||ETA>C.Clock+4320){E=TEXT("Choose a delivery at least 30 minutes from now");return false;}FFoundrylineFreightBooking B;B.Id=State.NextId++;B.Inbound=true;B.Sheets=A.Quantity;B.Carrier=FFoundrylineCommerceCatalog::PalletWeight(0,A.Quantity,FFoundrylineSteelSheet::PoundsEach())<=2000?1:2;B.Customer=TEXT("Foundry Steel Supply");B.OriginalFeeCents=FFoundrylineEconomyTuning::SupplierFreightCents;B.ArrivalAt=ETA;B.DispatchAt=ETA-20;C.Bookings.Add(B);CommerceExpense(Steel,TEXT("Steel sheets / ")+FString::FromInt(A.Quantity));CommerceExpense(FFoundrylineEconomyTuning::SupplierFreightCents,TEXT("Supplier freight booking"));return true;}
 // Bought frame sections or light kits for one welding station: at most what its bins still lack and one bundle or crate.
 if(A.Action==Act::BuyParts){const auto* S=State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==A.Id;});FFoundrylineBoughtPart Bought;
  if(!S||A.Choice<0||A.Choice>=FFoundrylineFactoryCatalog::PartCount||!FFoundrylineFactoryCatalog::Bought(Part(A.Choice),&Bought)||C.Bookings.Num()>=96){E=TEXT("Choose a welding station and a part to buy");return false;}
  const Part P=Part(A.Choice);const int Room=FMath::Min(FMath::Max(0,StationShortfall(*S,P)-StationRackSupply(P)),FFoundrylineFactoryCatalog::Part(P).Capacity);
  if(Room<1){E=TEXT("This station already has or is expecting enough ")+FString(FFoundrylineFactoryCatalog::AssemblyName(P)).ToLower();return false;}
  if(A.Quantity<1||A.Quantity>Room){E=FString::Printf(TEXT("Buy 1 to %d for this station"),Room);return false;}
  const int64 Cost=A.Quantity*Bought.PriceCents;if(C.UnpaidCents||C.CashCents<Cost+FFoundrylineEconomyTuning::SupplierFreightCents){E=TEXT("Insufficient available cash for parts and delivery");return false;}
  const double ETA=A.Arrival==0?C.Clock+60:A.Arrival;if(!FMath::IsFinite(ETA)||ETA<C.Clock+30||ETA>C.Clock+4320){E=TEXT("Choose a delivery at least 30 minutes from now");return false;}
  FFoundrylineFreightBooking B;B.Id=State.NextId++;B.Inbound=true;B.Sheets=A.Quantity;B.Part=uint8(P);B.StationId=S->Id;B.Carrier=FFoundrylineCommerceCatalog::PalletWeight(B.Part,A.Quantity,Bought.UnitLb)<=2000?1:2;B.Customer=TEXT("Foundry Steel Supply");B.OriginalFeeCents=FFoundrylineEconomyTuning::SupplierFreightCents;B.ArrivalAt=ETA;B.DispatchAt=ETA-20;C.Bookings.Add(B);
  CommerceExpense(Cost,FString::Printf(TEXT("%s / %d"),FFoundrylineFactoryCatalog::AssemblyName(P),A.Quantity));CommerceExpense(FFoundrylineEconomyTuning::SupplierFreightCents,TEXT("Supplier freight booking"));return true;}
 if(A.Action==Act::AssignPickup){
if(!O||O->Status!=EFoundrylineOrderState::Accepted||O->Delivered>=O->Quantity){E=TEXT("Choose an accepted, unfinished order");return false;}if(State.Pallets.ContainsByPredicate([](const auto& P){return P.Place==Place::Pickup;})||State.Jobs.ContainsByPredicate([](const auto& J){return J.DestinationPlace==Place::Pickup;})){E=TEXT("Ship the pallets already assigned to this pickup zone first");return false;}C.PickupOrder=O->Id;return true;}
 if(A.Action==Act::StagePallet){
  if(State.bPickupLayoutPending){E=TEXT("Clear the floor and access just inside your loading door before staging a shipment");return false;}
  if(!O||O->Status!=EFoundrylineOrderState::Accepted||A.Pallets.Num()!=1){E=TEXT("Choose an accepted contract and one pallet to stage");return false;}
  const auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==A.Pallets[0];});
  if(!P||uint8(P->Part)!=O->Part||P->BookingId||P->Reserved||P->Quantity<=0||(P->Place!=Place::Rack&&P->Place!=Place::MachineOutput&&P->Place!=Place::StationOut)){E=TEXT("Choose matching, unreserved stock on a rack or machine output");return false;}
  if(C.PickupOrder!=O->Id&&(State.Pallets.ContainsByPredicate([](const auto& V){return V.Place==Place::Pickup;})||State.Jobs.ContainsByPredicate([](const auto& V){return V.DestinationPlace==Place::Pickup;}))){E=TEXT("The loading bay is staging another contract; finish that shipment first");return false;}
  int Need=O->Quantity-O->Delivered;for(const auto& B:C.Bookings)if(!B.Inbound&&B.OrderId==O->Id&&B.Status!=BS::Delivered)for(const auto& I:B.Manifest)Need-=I.Quantity;for(const auto& V:State.Pallets)if(V.Place==Place::Pickup&&V.OrderId==O->Id&&!V.BookingId)Need-=V.Quantity;for(const auto& J:State.Jobs)if(J.DestinationPlace==Place::Pickup&&J.OrderId==O->Id)Need-=J.Quantity;
  if(Need<=0){E=TEXT("Enough parts are already staged or booked for this contract");return false;}
  if(!State.Drivers.ContainsByPredicate([](const auto& D){return D.EmployeeId&&D.Priorities.IsValidIndex(2)&&D.Priorities[2]>=0;})){E=TEXT("Assign a forklift driver and enable Shipping in their task priorities");return false;}
  const int Quantity=FMath::Min(Need,P->Quantity);if(P->Place==Place::StationOut&&Quantity!=P->Quantity){E=TEXT("Move this assembly pallet to storage first so only the required quantity can be picked");return false;}
  for(int Slot=0;Slot<2;++Slot){const int Destination=1000+Slot;if(State.Pallets.ContainsByPredicate([&](const auto& V){return V.Place==Place::Pickup&&V.Owner==Destination;})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.DestinationPlace==Place::Pickup&&J.Destination==Destination;}))continue;
   const auto At=ShippingPickupPosition(Slot);if(!MakeJob(P->Id,Destination,Place::Pickup,Quantity,ShippingPickupDock(Slot),At,90))continue;
   State.Jobs.Last().OrderId=O->Id;C.PickupOrder=O->Id;State.Running=true;return true;
  }
  E=TEXT("No free connected loading position; clear the bay and check the forklift route");return false;
 }
 if(A.Action==Act::BookPickup){if(A.Choice<0||A.Choice>4||A.Pallets.IsEmpty()||A.Pallets.Num()>26||A.Pallets.Num()!=A.Quantities.Num()||C.Bookings.Num()>=96){E=TEXT("Select a vehicle and at least one staged pallet");return false;}auto* Order=C.Orders.FindByPredicate([&](const auto& V){return V.Id==C.PickupOrder;});if(!Order||Order->Status!=EFoundrylineOrderState::Accepted){E=TEXT("Assign an active customer order to this zone");return false;}const auto Carrier=FFoundrylineCommerceCatalog::Carrier(A.Choice);if(!FMath::IsFinite(A.Arrival)||A.Arrival<C.Clock+Carrier.LeadMinutes+1||A.Arrival>C.Clock+4320){E=TEXT("Choose an arrival after the carrier's lead time");return false;}if(C.UnpaidCents||C.CashCents<Carrier.FeeCents){E=TEXT("Insufficient available cash for this pickup fee");return false;}FFoundrylineFreightBooking B;B.Id=State.NextId++;B.OrderId=Order->Id;B.Customer=Order->Customer;B.Carrier=A.Choice;B.ArrivalAt=A.Arrival;B.DispatchAt=B.ArrivalAt-Carrier.LeadMinutes;B.OriginalFeeCents=Carrier.FeeCents;TSet<int> Seen;int Spaces=0,Quantity=0;double Weight=0;
 for(int Id:A.Pallets)if(const auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;}))if(!FFoundrylineCommerceCatalog::CarrierFits(A.Choice,uint8(P->Part))){E=FString::Printf(TEXT("A %s won't fit a %s; choose a flatbed or larger"),*FString(FFoundrylineFactoryCatalog::Part(P->Part).Name).ToLower().LeftChop(1),*FString(Carrier.Vehicle).ToLower());return false;}
 for(int I=0;I<A.Pallets.Num();++I){auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==A.Pallets[I];});if(!P||P->Place!=Place::Pickup||P->OrderId!=Order->Id||P->BookingId||P->Reserved||P->Quantity!=A.Quantities[I]||!FFoundrylineFactoryCatalog::FitsPalletWeight(P->Part,P->Quantity,P->UnitLb)||Seen.Contains(P->Id)){E=TEXT("A selected pallet changed or is already booked; refresh the list");return false;}Seen.Add(P->Id);Spaces+=FFoundrylineCommerceCatalog::PalletSpaces(uint8(P->Part));Weight+=FFoundrylineCommerceCatalog::PalletWeight(uint8(P->Part),P->Quantity,P->UnitLb);Quantity+=P->Quantity;B.Manifest.Add({P->Id,uint8(P->Part),P->Quantity,P->UnitLb});}
 int Committed=Order->Delivered;for(const auto& Existing:C.Bookings)if(!Existing.Inbound&&Existing.OrderId==Order->Id&&Existing.Status!=BS::Delivered)for(const auto& Item:Existing.Manifest)Committed+=Item.Quantity;
 if(Spaces>Carrier.Spaces||Weight>Carrier.PayloadLb+.01||Committed+Quantity>Order->Quantity){E=TEXT("Selected pallets exceed vehicle capacity or the order's remaining quantity");return false;}
 for(int Id:A.Pallets)State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Id;})->BookingId=B.Id;C.Bookings.Add(B);CommerceExpense(Carrier.FeeCents,FString(Carrier.Name)+TEXT(" / pickup booked"));return true;}
 if(A.Action==Act::Reschedule){auto* B=C.Bookings.FindByPredicate([&](const auto& V){return V.Id==A.Id;});if(!B||B->Status!=BS::Booked||C.Clock>=B->DispatchAt){E=TEXT("The carrier is already travelling; this booking is locked");return false;}const auto Spec=FFoundrylineCommerceCatalog::Carrier(B->Carrier);const double Lead=B->Inbound?20:Spec.LeadMinutes;const int64 Fee=B->OriginalFeeCents/2;if(!FMath::IsFinite(A.Arrival)||A.Arrival<C.Clock+Lead+1||A.Arrival>C.Clock+4320||C.UnpaidCents||C.CashCents<Fee){E=TEXT("Choose a valid arrival and allow for the rebooking fee");return false;}B->ArrivalAt=A.Arrival;B->DispatchAt=A.Arrival-Lead;CommerceExpense(Fee,TEXT("Carrier rebooking / 50% original fee"));return true;}
 if(A.Action==Act::DriverPriority){auto* D=State.Drivers.FindByPredicate([&](const auto& V){return V.Id==A.Id;});if(!D||A.Choice<0||A.Choice>2||A.Quantity< -1||A.Quantity>2||D->Priorities.Num()!=3){E=TEXT("Choose a driver task priority");return false;}D->Priorities[A.Choice]=A.Quantity;return true;}
 if(A.Action==Act::BuyBrake)return InstallBrake(E);
 if(A.Action==Act::BuyForklift){if(C.UnpaidCents||C.CashCents<FFoundrylineEconomyTuning::ForkliftCents||State.Drivers.Num()>=2){E=TEXT("A second forklift costs $1200; this starter has two parking stalls");return false;}if(!AddDriver(1,E,false))return false;auto& D=State.Drivers.Last();D.Parking={100,1100,20};D.Position=D.Parking;CommerceExpense(FFoundrylineEconomyTuning::ForkliftCents,TEXT("Forklift purchase"));C.ForkliftInstallMinutes=FFoundrylineEconomyTuning::ForkliftInstallMinutes;C.ForkliftInstallDriver=D.Id;return true;}
 E=TEXT("Unknown operations request");return false;};
 if(!Apply()){State=MoveTemp(Backup);Solids=OldSolids;return false;}++State.Revision;return true;
}
