#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;using Catalog=FFoundrylineFactoryCatalog;
bool Overlap(const FBox2D& A,const FBox2D& B){return FFoundrylineFloorPlacement::Overlap(A,B);}
// Port-end bays first, then the Welder A and B sides of the access ring.
TArray<FBox2D> Access(const FFoundrylineWeldingStationRecord& S){return Layout::AccessRing(S);}
}
bool FFoundrylineFactorySimulation::OverlapsWeldingStation(const FBox2D& Box,bool Approaches)const{
 for(const auto& S:State.WeldingStations){if(Overlap(Box,Layout::Bounds(S)))return true;if(Approaches)for(const auto& A:Access(S))if(Overlap(Box,A))return true;}return false;
}
bool FFoundrylineFactorySimulation::CanPlaceWeldingStation(FFoundrylineWeldingStationRecord& S,FString& E)const{
 if(S.Center.ContainsNaN()||FMath::Abs(S.Center.X)>1e6||FMath::Abs(S.Center.Y)>1e6||S.QuarterTurns<0||S.QuarterTurns>3){E=TEXT("Choose a valid station position and rotation");return false;}
 if(State.WeldingStations.Num()>=Layout::MaxStations&&!State.WeldingStations.ContainsByPredicate([&](const auto& O){return O.Id==S.Id;})){E=TEXT("Station limit reached: four welding stations per plant");return false;}
 // The product is placed at 1:1 scale; a station that cannot hold it is never offered.
 if(!Layout::ProductFits(Layout::RackOnBenchCm(),FVector2D(Layout::RackSizeCm()),E))return false;
 const auto B=Layout::Bounds(S);const FBox2D Floor=State.bCutPartsStarter?FFoundrylineWorkshopLayout::CutShopInterior():FBox2D(State.FloorMin,State.FloorMax);const auto Approaches=Access(S);
 auto OnFloor=[&](const FBox2D& X){return Floor.IsInsideOrOn(X.Min)&&Floor.IsInsideOrOn(X.Max);};
 if(!OnFloor(B)){E=TEXT("The whole 19 ft 6 in x 13 ft station must fit on the shop floor");return false;}
 for(int I=0;I<Approaches.Num();++I)if(!OnFloor(Approaches[I])){E=I<2?TEXT("Leave 4 ft of forklift space inside the shop at both station ends"):TEXT("Leave 4 ft of walking space inside the shop along both welder sides");return false;}
 TArray<FBox2D> Fixed=Solids;if(State.bCutPartsStarter)Fixed.Append(FFoundrylineWorkshopLayout::CutShopReserved());
 for(const auto& X:Fixed){if(Overlap(X,B)){E=TEXT("Station overlaps equipment, racks or a marked work area");return false;}for(const auto& A:Approaches)if(Overlap(X,A)){E=TEXT("Equipment or racks block the station's forklift or welder access");return false;}}
 for(const auto& L:State.Lanes)if(Overlap(FBox2D(L.Min,L.Max),B)){E=TEXT("Station crosses a painted lane; place it beside the lane");return false;}
 for(const auto& Z:State.Zones){const auto ZB=FFoundrylineWorkZoneLayout::Bounds(Z);if(Overlap(ZB,B)){E=TEXT("Another work zone already reserves that space");return false;}for(const auto& A:Approaches)if(Overlap(ZB,A)){E=TEXT("A work zone blocks the station's forklift or welder access");return false;}}
 for(const auto& O:State.WeldingStations)if(O.Id!=S.Id){const auto OB=Layout::Bounds(O);
  if(Overlap(OB,B)){E=Layout::Name(O.Number)+TEXT(" already uses that space");return false;}
  for(const auto& A:Approaches)if(Overlap(OB,A)){E=Layout::Name(O.Number)+TEXT(" blocks this station's forklift or welder access");return false;}
  for(const auto& A:Access(O))if(Overlap(B,A)){E=Layout::Name(O.Number)+TEXT(" needs that space for its forklift or welder access");return false;}
 }
 for(const auto& P:State.Pallets)if(P.Place!=Place::Forks&&P.Place!=Place::Inbound&&P.Place!=Place::Outbound&&P.Place!=Place::InTransit&&B.IsInside(FVector2D(P.Position))){E=TEXT("Move stored pallets out of the station footprint first");return false;}
 for(const auto& Person:FFoundrylineEmployeeRoster::Build(State))if(!Person.bSeated&&(B.ExpandBy(35).IsInside(FVector2D(Person.Position))||B.ExpandBy(35).IsInside(FVector2D(Person.Home))||B.ExpandBy(35).IsInside(FVector2D(Person.WorkPoint)))){E=TEXT("An employee, their waiting spot or workstation is inside the station footprint");return false;}
 for(const auto& D:State.Drivers)if(B.ExpandBy(80).IsInside(FVector2D(D.Position))||B.ExpandBy(80).IsInside(FVector2D(D.Parking))){E=TEXT("A forklift or its parking bay is inside the station footprint");return false;}
 FFoundrylineFactorySimulation Candidate;Candidate.State=State;Candidate.Solids=Solids;Candidate.State.WeldingStations.RemoveAll([&](const auto& O){return O.Id==S.Id;});Candidate.State.WeldingStations.Add(S);
 if(!Candidate.WeldingStationAccess(S,E))return false;
 return true;
}
bool FFoundrylineFactorySimulation::WeldingStationAccess(FFoundrylineWeldingStationRecord& S,FString& E)const{
 // Forklifts must reach both port docks along real lanes from where they park; they never drive through the station.
 FVector Fork=FVector::ZeroVector;bool HasFork=false;
 if(State.Drivers.Num()){Fork=State.Drivers[0].Parking;HasFork=true;}
 else if(const auto* L=State.Lanes.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineFloorLane::Forklift;})){Fork=FVector((L->Min+L->Max)/2,20);HasFork=true;}
 if(!HasFork){E=TEXT("Paint a forklift lane that reaches the IN and OUT ends");return false;}
 // Prefer the centred stop; shift up to 2 ft along the port toward a reachable aisle.
 for(bool Out:{false,true}){int32& Offset=Out?S.OutDockOffsetFt:S.InDockOffsetFt;bool Found=false;
  for(int32 Try:{0,1,-1,2,-2}){Offset=Try;if(!Path(Fork,Layout::Dock(S,Out)).IsEmpty()){Found=true;break;}}
  if(!Found){Offset=0;E=FString::Printf(TEXT("Forklifts cannot reach the %s end; connect it to a forklift lane"),Out?TEXT("OUT"):TEXT("IN"));return false;}}
 // A forklift collects each OUT pallet, and delivers to each IN stock pallet, from straight in front of it.
 for(int32 Slot=0;Slot<Layout::OutPallets;++Slot)if(Path(Fork,Layout::OutPalletDock(S,Slot)).IsEmpty()){E=TEXT("Forklifts cannot reach both OUT pallets; widen the forklift space at the OUT end");return false;}
 for(int32 Slot=0;Slot<2;++Slot)if(Path(Fork,Layout::InPalletDock(S,Slot)).IsEmpty()){E=TEXT("Forklifts cannot reach both IN pallets; widen the forklift space at the IN end");return false;}
 // People walk only on walkways and inside the two worker clearances, so each side needs its own connection.
 FVector Home=FVector::ZeroVector;bool HasHome=false;
 for(const auto& P:FFoundrylineEmployeeRoster::Build(State))if(!P.bSeated){Home=P.Home;HasHome=true;break;}
 if(!HasHome)HasHome=FindEmployeeWaitingPosition(Home);
 if(!HasHome){E=TEXT("No employee waiting area can reach this station");return false;}
 for(int Side=0;Side<2;++Side)if(Path(Home,Layout::WorkPoint(S,Side),true).IsEmpty()){E=FString::Printf(TEXT("Welder %s side cannot be reached on foot; paint a walkway to the station's access ring"),Side?TEXT("B"):TEXT("A"));return false;}
 return true;
}
bool FFoundrylineFactorySimulation::AddWeldingStation(FVector Center,int32 Quarter,bool Swap,FString& E){
 FFoundrylineWeldingStationRecord S;S.Center=FVector(Center.X,Center.Y,20);S.QuarterTurns=Quarter;S.bSwapSides=Swap;
 if(!CanPlaceWeldingStation(S,E))return false;
 S.Id=State.NextId++;S.Reserved.Init(0,Catalog::PartCount);for(int N=1;N<=99;++N)if(!State.WeldingStations.ContainsByPredicate([&](const auto& O){return O.Number==N;})){S.Number=N;break;}
 State.WeldingStations.Add(S);
 // Forklifts may bring supplies to the new IN port and collect from its OUT cradle on every existing route.
 for(auto& R:State.Routes){if(R.Destinations.Num()<80)R.Destinations.AddUnique(S.Id);if(R.Sources.Num()<80)R.Sources.AddUnique(S.Id);}
 for(auto& O:State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}for(auto& W:State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}
 ++State.Revision;return true;
}
bool FFoundrylineFactorySimulation::RemoveWeldingStation(int32 Id,FString& E){
 const int I=State.WeldingStations.IndexOfByPredicate([&](const auto& S){return S.Id==Id;});if(I==INDEX_NONE){E=TEXT("Choose a welding station in this plant");return false;}
 if(State.WeldingStations[I].Stage){E=TEXT("Finish the headache rack on the bench before removing the station");return false;}
 if(State.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::Buffer||P.Place==Place::StationIn||P.Place==Place::StationOut);})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==Id;})||State.Commerce.Bookings.ContainsByPredicate([&](const auto& B){return B.StationId==Id&&B.Status!=EFoundrylineBookingState::Delivered;})){E=TEXT("Return the station's supplies and finish its transport before removing it");return false;}
 // Its welder goes back to the waiting area, unassigned.
 for(auto& W:State.Walkers)if(W.Station==Id){W.Station=W.StationSide=0;W.Path.Reset();W.Distance=0;W.Retry=0;}
 State.WeldingStations.RemoveAt(I);for(auto& R:State.Routes){R.Destinations.Remove(Id);R.Sources.Remove(Id);}++State.Revision;return true;
}
void FFoundrylineFactorySimulation::MigrateWeldingStations(){
 // Station stock positions are derived: it always sits on its IN supply pallet.
 for(const auto& S:State.WeldingStations)for(auto& P:State.Pallets)if(P.Owner==S.Id&&P.Place==Place::Buffer)P.Position=Layout::BinPoint(S,P.Part);
 if(State.Version==9)State.Version=10;
 if(State.Version==10){
 // Zone part ledgers grow to cover frame sections, light kits and finished racks; saved stations join existing routes.
 State.Version=11;for(auto& Z:State.Zones){Z.ReservedParts.SetNumZeroed(Catalog::PartCount);Z.Installed.SetNumZeroed(Catalog::PartCount);}
 for(const auto& S:State.WeldingStations)for(auto& R:State.Routes)if(R.Destinations.Num()<80)R.Destinations.AddUnique(S.Id);}
 // Schema 12: stations start with an empty bench; welders keep their zone jobs.
 // Schema 13: station welders gain a slot; saved ones are Welder A.
 if(State.Version==11){State.Version=12;for(auto& S:State.WeldingStations){S.Reserved.SetNumZeroed(Catalog::PartCount);S.Stage=S.UnitRecipe=0;S.Work=S.ReservedLb=0;}}
 if(State.Version==12){State.Version=13;for(auto& V:State.Walkers)if(!V.Station)V.StationSide=0;}
 // Schema 14: OUT widens from 4 ft to 6 ft 6 in with two pallets. A saved station grows at its OUT end only (its centre moves
 // 1 ft 3 in toward OUT), so its bench, bins, machines, welders and IN stay where they were; a rack on the old OUT cradle
 // moves onto the first OUT pallet. Validation still rejects the plant if the longer station no longer fits its spot.
 if(State.Version==13){State.Version=14;for(auto& S:State.WeldingStations){
  S.Center+=Layout::Schema14Shift(S);
  for(auto& P:State.Pallets)if(P.Owner==S.Id&&P.Place==Place::StationOut)P.Position=Layout::OutPalletPoint(S,0);
  // Forklifts now collect from OUT, so every saved station becomes a source on every route too.
  for(auto& R:State.Routes)if(R.Sources.Num()<80)R.Sources.AddUnique(S.Id);}}
}
bool FFoundrylineFactorySimulation::ConfigureWeldingStation(int32 Id,int32 Recipe,int32 Batch,FString& E){
 auto* S=State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!S){E=TEXT("Choose a welding station in this plant");return false;}
 FFoundrylineAssemblyRecipe R;if(Recipe&&!Catalog::Recipe(Recipe,R)||Batch<0||Batch>100||!Recipe&&Batch){E=TEXT("Choose a product and a quantity of 1 to 100");return false;}
 S->Recipe=Recipe;S->BatchRemaining=Batch;++State.Revision;return true;
}
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
// IN is a transfer port: one forklift and one single-type pallet at a time, emptied into the bins before the next category.
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
  S.UnloadSeconds+=Dt;if(S.UnloadSeconds<Layout::UnloadSeconds)continue;S.UnloadSeconds=0;
  // The empty pallet's return is abstracted into the unload; only the parts remain, in their bin.
  const int InId=In->Id;const Part P=In->Part;const float Lb=In->UnitLb;
  if(auto* Bin=State.Pallets.FindByPredicate([&](const auto& A){return A.Place==Place::Buffer&&A.Owner==S.Id&&A.Part==P&&FMath::IsNearlyEqual(A.UnitLb,Lb,.01f);})){Bin->Quantity+=In->Quantity;State.Pallets.RemoveAll([&](const auto& A){return A.Id==InId;});}
  else {In->Place=Place::Buffer;In->Position=Layout::BinPoint(S,P);In->BookingId=0;In->OrderId=0;}
 }
}
int32 FFoundrylineFactorySimulation::StationWelders(const FFoundrylineWeldingStationRecord& S,bool AtWork)const{
 int32 N=0;for(const auto& W:State.Walkers)if(W.Station==S.Id&&(!AtWork||FVector::Dist2D(W.Position,Layout::WorkPoint(S,W.StationSide))<=1))++N;return N;
}
float FFoundrylineFactorySimulation::StationRate(const FFoundrylineWeldingStationRecord& S,int32 AtWork)const{
 FFoundrylineAssemblyRecipe R;if(!Catalog::Recipe(S.Stage?S.UnitRecipe:S.Recipe,R))Catalog::Recipe(1,R);const int32 N=AtWork<0?StationWelders(S,true):AtWork;return N>=2?R.TwoWelderRate:N==1?1.f:0.f;
}
// One unit at a time on the bench. The kit is claimed when the unit starts, and only if an OUT pallet has room; progress is shared
// by the welders at their work points (100% for one, the recipe's two-welder rate for both) and only advances while the
// plant runs. It is kept through pauses, staff changes and saves.
void FFoundrylineFactorySimulation::AssembleWeldingStations(float Dt){
 for(auto& S:State.WeldingStations){S.bAdvancing=false;FFoundrylineAssemblyRecipe R;
  int32 Slot,Level;const bool OutRoom=Layout::NextOutSpot(S,State.Pallets,Slot,Level);
  if(!S.Stage){
   if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0||!OutRoom||!StationWelderReady(S))continue;
   bool Kit=true;for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]&&Available(Part(P),Place::Buffer,S.Id)<R.Bill[P])Kit=false;if(!Kit)continue;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;
   for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]){float Lb=0;Consume(Part(P),R.Bill[P],Place::Buffer,S.Id,&Lb);S.Reserved[P]=R.Bill[P];S.ReservedLb+=Lb;}
   S.UnitRecipe=S.Recipe;--S.BatchRemaining;if(!S.BatchRemaining)S.Recipe=0;S.Stage=1;S.Work=0;continue;
  }
  // The finished rack is carried from the fixture onto the first OUT pallet with room; it joins that stack exactly once, on
  // arrival. Racks on one pallet are one stock record (mass kept as the average per rack).
  if(S.Stage==4){S.Work=FMath::Min(S.Work+Dt,Layout::StageSeconds(4));if(S.Work<Layout::StageSeconds(4)||!OutRoom||!Catalog::Recipe(S.UnitRecipe,R))continue;
   auto* Stack=State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut&&Layout::OutSlot(S,A.Position)==Slot;});
   if(Stack){Stack->UnitLb=(Stack->UnitLb*Stack->Quantity+S.ReservedLb)/(Stack->Quantity+1);++Stack->Quantity;}
   else if(State.Pallets.Num()>=64)continue;else AddPallet(R.Output,1,Place::StationOut,S.Id,Layout::OutPalletPoint(S,Slot),S.ReservedLb);
   ++S.UnitsCompleted;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;S.UnitRecipe=0;S.Stage=0;S.Work=0;continue;}
  const float Rate=StationRate(S);if(Rate<=0)continue;
  for(auto& W:State.Walkers)if(W.Station==S.Id&&FVector::Dist2D(W.Position,Layout::WorkPoint(S,W.StationSide))<=1)W.Yaw=Layout::WorkYaw(S,W.StationSide);
  S.Work=FMath::Min(S.Work+Dt*Rate,Layout::StageSeconds(S.Stage));S.bAdvancing=true;if(S.Work<Layout::StageSeconds(S.Stage))continue;S.Work=0;++S.Stage;
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
FString FFoundrylineFactorySimulation::WeldingStationStatus(const FFoundrylineWeldingStationRecord& S)const{
 FFoundrylineAssemblyRecipe R;const bool HasWelder=State.Walkers.ContainsByPredicate([&](const auto& W){return W.Station==S.Id;});
 if(S.Stage&&Catalog::Recipe(S.UnitRecipe,R)){
  static const TCHAR* Stages[]={TEXT(""),TEXT("Fit-up"),TEXT("Welding"),TEXT("Installing lights"),TEXT("Moving to OUT")};const int Percent=FMath::FloorToInt(100*S.Work/Layout::StageSeconds(S.Stage));
  if(S.Stage<4&&!HasWelder)return FString::Printf(TEXT("%s paused at %d%% / assign a welder"),Stages[S.Stage],Percent);
  if(S.Stage<4&&!StationWelderReady(S))return FString::Printf(TEXT("%s paused at %d%% / welder walking"),Stages[S.Stage],Percent);
  return FString::Printf(TEXT("%s %s %d%%%s"),Stages[S.Stage],*FString(R.Name).ToLower(),Percent,S.Stage<4&&StationWelders(S,true)>1?TEXT(" / 2 welders"):TEXT(""));}
 if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0){const FString Out=StationOutStatus(S,FString::Printf(TEXT("%d finished on OUT"),OutTotal(S)));return Out.IsEmpty()?FString(TEXT("Choose a product and quantity")):Out;}
 // Both OUT pallets full is what holds the next unit back, whatever the bins still need.
 {int32 Slot,Level;if(!Layout::NextOutSpot(S,State.Pallets,Slot,Level))return StationOutStatus(S,FString::Printf(TEXT("OUT full (%d on %d pallets)"),OutTotal(S),Layout::OutPallets));}
 for(Part P:Catalog::AssemblyParts())if(StationShortfall(S,P)>0){const FString Name=FString(Catalog::AssemblyName(P)).ToLower();
  if(StationRackSupply(P)>0)return TEXT("Requesting ")+Name;
  return Catalog::Bought(P)?TEXT("Missing ")+Name+TEXT(" / buy parts"):TEXT("Missing ")+Name+TEXT(" / cut more on the plasma table");}
 for(Part P:Catalog::AssemblyParts())if(StationStock(S,P,false)<StationTarget(S,P))return TEXT("Receiving ")+FString(Catalog::AssemblyName(P)).ToLower();
 const int Kits=FMath::Min(S.BatchRemaining,2);const FString Ready=FString::Printf(TEXT("Supplies ready for %d %s%s"),Kits,*FString(R.Name).ToLower(),Kits==1?TEXT(""):TEXT("s"));
 return Ready+(!HasWelder?TEXT(" / assign a welder"):!StationWelderReady(S)?TEXT(" / welder walking"):TEXT(""));
}
bool FFoundrylineFactorySimulation::WeldingStationsValid(FString& E)const{
 if(State.WeldingStations.Num()>Layout::MaxStations){E=TEXT("Too many welding stations in this plant");return false;}
 const FBox2D Floor(State.FloorMin,State.FloorMax);TSet<int> Numbers;
 for(int I=0;I<State.WeldingStations.Num();++I){const auto& S=State.WeldingStations[I];
  if(S.Number<1||S.Number>99||Numbers.Contains(S.Number)||S.Center.ContainsNaN()||S.QuarterTurns<0||S.QuarterTurns>3||FMath::Abs(S.InDockOffsetFt)>Layout::MaxDockOffsetFt||FMath::Abs(S.OutDockOffsetFt)>Layout::MaxDockOffsetFt){E=TEXT("Invalid welding station record");return false;}Numbers.Add(S.Number);
  const auto B=Layout::Bounds(S);const FBox2D Shop=State.bCutPartsStarter?FFoundrylineWorkshopLayout::CutShopInterior():Floor;if(!Shop.IsInsideOrOn(B.Min)||!Shop.IsInsideOrOn(B.Max)){E=TEXT("Saved welding station is outside the shop floor");return false;}
  for(int J=I+1;J<State.WeldingStations.Num();++J)if(Overlap(B,Layout::Bounds(State.WeldingStations[J]))){E=TEXT("Saved welding stations overlap");return false;}
  for(const auto& L:State.Lanes)if(Overlap(B,FBox2D(L.Min,L.Max))){E=TEXT("Saved welding station overlaps a lane; layout was not loaded");return false;}
  for(const auto& Z:State.Zones)if(Overlap(B,FFoundrylineWorkZoneLayout::Bounds(Z))){E=TEXT("Saved welding station overlaps a work zone");return false;}
  FFoundrylineAssemblyRecipe R;
  if(S.Recipe<0||S.Recipe>Catalog::RecipeCount||S.Recipe&&!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<0||S.BatchRemaining>100||!S.Recipe&&S.BatchRemaining||!FMath::IsFinite(S.UnloadSeconds)||S.UnloadSeconds<0||S.UnloadSeconds>Layout::UnloadSeconds){E=TEXT("Invalid welding station recipe or unload state");return false;}
  // Station stock: at most one pallet on IN, bins within their physical capacity, one stack per OUT pallet within its height.
  int OnIn=0,OnOut=0,OutSlots=0;TArray<int> Bins;Bins.Init(0,Catalog::PartCount);
  for(const auto& P:State.Pallets){if(P.Owner!=S.Id)continue;
   if(P.Place==Place::StationIn){++OnIn;if(!Layout::BinCapacity(P.Part)||P.Quantity>Layout::BinCapacity(P.Part)){E=TEXT("Invalid pallet on a welding station IN port");return false;}}
   else if(P.Place==Place::StationOut){OnOut+=P.Quantity;const int Slot=Layout::OutSlot(S,P.Position);
    if(P.Part!=Part::HeadacheRack||P.Quantity<1||P.Quantity>Layout::RacksPerOutPallet||P.Reserved<0||P.Reserved>P.Quantity||Slot<0||OutSlots&(1<<Slot)){E=TEXT("Invalid product stack on a welding station OUT pallet");return false;}OutSlots|=1<<Slot;}
   else if(P.Place==Place::Buffer){if(!Layout::BinCapacity(P.Part)||P.BookingId||P.OrderId){E=TEXT("Invalid welding station bin stock");return false;}Bins[int(P.Part)]+=P.Quantity;}}
  if(OnIn>1){E=TEXT("A welding station port holds more than one pallet");return false;}
  // The unit on the bench holds exactly its recipe's kit; an empty bench holds nothing.
  FFoundrylineAssemblyRecipe Unit;const bool HasUnit=Catalog::Recipe(S.UnitRecipe,Unit);
  if(S.Stage<0||S.Stage>4||S.Reserved.Num()!=Catalog::PartCount||!FMath::IsFinite(S.Work)||S.Work<0||S.Work>Layout::StageSeconds(FMath::Clamp(S.Stage,0,4))||!FMath::IsFinite(S.ReservedLb)||S.ReservedLb<0||S.UnitsCompleted<0
   ||(S.Stage?!HasUnit||S.Reserved!=Unit.Bill||S.Stage==4&&OnOut>=Layout::OutPallets*Layout::RacksPerOutPallet:S.UnitRecipe||S.ReservedLb>0||S.Reserved.ContainsByPredicate([](int Q){return Q!=0;}))){E=TEXT("Invalid welding station assembly progress");return false;}
  for(int P=0;P<Catalog::PartCount;++P)if(Bins[P]>Layout::BinCapacity(Part(P))){E=TEXT("Welding station bins hold more than they can fit");return false;}
 }
 for(const auto& P:State.Pallets)if((P.Place==Place::StationIn||P.Place==Place::StationOut)&&!State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==P.Owner;})){E=TEXT("Station stock has no welding station");return false;}
 return true;

}
