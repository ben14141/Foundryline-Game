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
 if(!OnFloor(B)){E=TEXT("The whole 17 ft x 13 ft station must fit on the shop floor");return false;}
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
 // Forklifts may bring supplies to the new IN port on every existing route.
 for(auto& R:State.Routes)if(R.Destinations.Num()<80)R.Destinations.AddUnique(S.Id);
 for(auto& O:State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}for(auto& W:State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}
 ++State.Revision;return true;
}
bool FFoundrylineFactorySimulation::RemoveWeldingStation(int32 Id,FString& E){
 const int I=State.WeldingStations.IndexOfByPredicate([&](const auto& S){return S.Id==Id;});if(I==INDEX_NONE){E=TEXT("Choose a welding station in this plant");return false;}
 if(State.WeldingStations[I].Stage){E=TEXT("Finish the headache rack on the bench before removing the station");return false;}
 if(State.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::Buffer||P.Place==Place::StationIn||P.Place==Place::StationOut);})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==Id;})||State.Commerce.Bookings.ContainsByPredicate([&](const auto& B){return B.StationId==Id&&B.Status!=EFoundrylineBookingState::Delivered;})){E=TEXT("Return the station's supplies and finish its transport before removing it");return false;}
 // Its welder goes back to the waiting area, unassigned.
 for(auto& W:State.Walkers)if(W.Station==Id){W.Station=W.StationSide=0;W.Path.Reset();W.Distance=0;W.Retry=0;}
 State.WeldingStations.RemoveAt(I);for(auto& R:State.Routes)R.Destinations.Remove(Id);++State.Revision;return true;
}
void FFoundrylineFactorySimulation::MigrateWeldingStations(){
 if(State.Version==9)State.Version=10;
 if(State.Version==10){
 // Zone part ledgers grow to cover frame sections, light kits and finished racks; saved stations join existing routes.
 State.Version=11;for(auto& Z:State.Zones){Z.ReservedParts.SetNumZeroed(Catalog::PartCount);Z.Installed.SetNumZeroed(Catalog::PartCount);}
 for(const auto& S:State.WeldingStations)for(auto& R:State.Routes)if(R.Destinations.Num()<80)R.Destinations.AddUnique(S.Id);}
 // Schema 12: stations start with an empty bench; welders keep their zone jobs.
 // Schema 13: station welders gain a slot; saved ones are Welder A.
 if(State.Version==12){State.Version=13;for(auto& V:State.Walkers)if(!V.Station)V.StationSide=0;return;}
 if(State.Version==11){State.Version=12;for(auto& S:State.WeldingStations){S.Reserved.SetNumZeroed(Catalog::PartCount);S.Stage=S.UnitRecipe=0;S.Work=S.ReservedLb=0;}State.Version=13;}
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
// IN is a transfer port: one forklift and one single-type pallet at a time, emptied into the bins before the next category.
void FFoundrylineFactorySimulation::PlanWeldingStations(){
 for(const auto& S:State.WeldingStations){if(StationPortBusy(S))continue;
  TArray<TPair<float,Part>> Wanted;for(Part P:Catalog::AssemblyParts()){const int Target=StationTarget(S,P),Short=StationShortfall(S,P);if(Short>0&&StationRackSupply(P)>0)Wanted.Add({float(Short)/Target,P});}
  Wanted.Sort([](const auto& A,const auto& B){return A.Key>B.Key;});bool Booked=false;
  for(const auto& W:Wanted){const int Short=FMath::Min(StationShortfall(S,W.Value),StationRackSupply(W.Value));
   for(const auto& A:State.Pallets)if(A.Part==W.Value&&A.Place==Place::Rack&&!A.BookingId&&A.Quantity>A.Reserved&&MakeJob(A.Id,S.Id,Place::StationIn,FMath::Min(Short,A.Quantity-A.Reserved),Layout::Dock(S,false),Layout::PortDrop(S,false),90)){Booked=true;break;}
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
// One unit at a time on the bench. The kit and the OUT cradle are claimed together when the unit starts; progress is shared
// by the welders at their work points (100% for one, the recipe's two-welder rate for both) and only advances while the
// plant runs. It is kept through pauses, staff changes and saves.
void FFoundrylineFactorySimulation::AssembleWeldingStations(float Dt){
 for(auto& S:State.WeldingStations){S.bAdvancing=false;FFoundrylineAssemblyRecipe R;
  const bool OutFree=!State.Pallets.ContainsByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut;});
  if(!S.Stage){
   if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0||!OutFree||!StationWelderReady(S))continue;
   bool Kit=true;for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]&&Available(Part(P),Place::Buffer,S.Id)<R.Bill[P])Kit=false;if(!Kit)continue;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;
   for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]){float Lb=0;Consume(Part(P),R.Bill[P],Place::Buffer,S.Id,&Lb);S.Reserved[P]=R.Bill[P];S.ReservedLb+=Lb;}
   S.UnitRecipe=S.Recipe;--S.BatchRemaining;if(!S.BatchRemaining)S.Recipe=0;S.Stage=1;S.Work=0;continue;
  }
  // The finished rack is carried from the fixture to the cradle; it becomes the OUT product exactly once, on arrival.
  if(S.Stage==4){S.Work=FMath::Min(S.Work+Dt,Layout::StageSeconds(4));if(S.Work<Layout::StageSeconds(4)||!OutFree||State.Pallets.Num()>=64||!Catalog::Recipe(S.UnitRecipe,R))continue;
   AddPallet(R.Output,1,Place::StationOut,S.Id,Layout::CradleTransform(S).GetLocation(),S.ReservedLb);++S.UnitsCompleted;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;S.UnitRecipe=0;S.Stage=0;S.Work=0;continue;}
  const float Rate=StationRate(S);if(Rate<=0)continue;
  for(auto& W:State.Walkers)if(W.Station==S.Id&&FVector::Dist2D(W.Position,Layout::WorkPoint(S,W.StationSide))<=1)W.Yaw=Layout::WorkYaw(S,W.StationSide);
  S.Work=FMath::Min(S.Work+Dt*Rate,Layout::StageSeconds(S.Stage));S.bAdvancing=true;if(S.Work<Layout::StageSeconds(S.Stage))continue;S.Work=0;++S.Stage;
 }
}
FString FFoundrylineFactorySimulation::WeldingStationStatus(const FFoundrylineWeldingStationRecord& S)const{
 FFoundrylineAssemblyRecipe R;const bool HasWelder=State.Walkers.ContainsByPredicate([&](const auto& W){return W.Station==S.Id;});
 if(S.Stage&&Catalog::Recipe(S.UnitRecipe,R)){
  static const TCHAR* Stages[]={TEXT(""),TEXT("Fit-up"),TEXT("Welding"),TEXT("Installing lights"),TEXT("Moving to OUT")};const int Percent=FMath::FloorToInt(100*S.Work/Layout::StageSeconds(S.Stage));
  if(S.Stage<4&&!HasWelder)return FString::Printf(TEXT("%s paused at %d%% / assign a welder"),Stages[S.Stage],Percent);
  if(S.Stage<4&&!StationWelderReady(S))return FString::Printf(TEXT("%s paused at %d%% / welder walking"),Stages[S.Stage],Percent);
  return FString::Printf(TEXT("%s %s %d%%%s"),Stages[S.Stage],*FString(R.Name).ToLower(),Percent,S.Stage<4&&StationWelders(S,true)>1?TEXT(" / 2 welders"):TEXT(""));}
 if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0)return State.Pallets.ContainsByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut;})?TEXT("Finished rack on OUT / awaiting collection"):TEXT("Choose a product and quantity");
 // A finished product waiting on OUT is what holds the next unit back, whatever the bins still need.
 if(State.Pallets.ContainsByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut;}))return TEXT("OUT cradle full / awaiting collection");
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
  // Station stock: at most one pallet on IN, bins within their physical capacity, at most one finished product on OUT.
  int OnIn=0,OnOut=0;TArray<int> Bins;Bins.Init(0,Catalog::PartCount);
  for(const auto& P:State.Pallets){if(P.Owner!=S.Id)continue;
   if(P.Place==Place::StationIn){++OnIn;if(!Layout::BinCapacity(P.Part)||P.Quantity>Layout::BinCapacity(P.Part)){E=TEXT("Invalid pallet on a welding station IN port");return false;}}
   else if(P.Place==Place::StationOut){++OnOut;if(P.Part!=Part::HeadacheRack||P.Quantity!=1||P.Reserved>1){E=TEXT("Invalid product on a welding station OUT cradle");return false;}}
   else if(P.Place==Place::Buffer){if(!Layout::BinCapacity(P.Part)||P.BookingId||P.OrderId){E=TEXT("Invalid welding station bin stock");return false;}Bins[int(P.Part)]+=P.Quantity;}}
  if(OnIn>1||OnOut>1){E=TEXT("A welding station port holds more than one pallet");return false;}
  // The unit on the bench holds exactly its recipe's kit; an empty bench holds nothing.
  FFoundrylineAssemblyRecipe Unit;const bool HasUnit=Catalog::Recipe(S.UnitRecipe,Unit);
  if(S.Stage<0||S.Stage>4||S.Reserved.Num()!=Catalog::PartCount||!FMath::IsFinite(S.Work)||S.Work<0||S.Work>Layout::StageSeconds(FMath::Clamp(S.Stage,0,4))||!FMath::IsFinite(S.ReservedLb)||S.ReservedLb<0||S.UnitsCompleted<0
   ||(S.Stage?!HasUnit||S.Reserved!=Unit.Bill||S.Stage==4&&OnOut:S.UnitRecipe||S.ReservedLb>0||S.Reserved.ContainsByPredicate([](int Q){return Q!=0;}))){E=TEXT("Invalid welding station assembly progress");return false;}
  for(int P=0;P<Catalog::PartCount;++P)if(Bins[P]>Layout::BinCapacity(Part(P))){E=TEXT("Welding station bins hold more than they can fit");return false;}
 }
 for(const auto& P:State.Pallets)if((P.Place==Place::StationIn||P.Place==Place::StationOut)&&!State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==P.Owner;})){E=TEXT("Station stock has no welding station");return false;}
 return true;

}
