// Placing, removing and configuring welding stations: footprint, access ring, forklift and walking access, and save migration.
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "FoundrylineWeldingStationInternal.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;using Catalog=FFoundrylineFactoryCatalog;
using FoundrylineWeldingStationInternal::Overlap;using FoundrylineWeldingStationInternal::Access;}
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
 // Forklifts may bring supplies to the new IN port and collect from its OUT pallets on every existing route.
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
 // 1 ft 3 in toward OUT), so its bench, machines, welders and IN stay where they were; a rack on the old OUT cradle
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
