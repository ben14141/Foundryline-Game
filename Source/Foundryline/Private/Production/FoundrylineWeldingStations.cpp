#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;
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
 S.Id=State.NextId++;for(int N=1;N<=99;++N)if(!State.WeldingStations.ContainsByPredicate([&](const auto& O){return O.Number==N;})){S.Number=N;break;}
 State.WeldingStations.Add(S);
 for(auto& O:State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}for(auto& W:State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}
 ++State.Revision;return true;
}
bool FFoundrylineFactorySimulation::RemoveWeldingStation(int32 Id,FString& E){
 const int I=State.WeldingStations.IndexOfByPredicate([&](const auto& S){return S.Id==Id;});if(I==INDEX_NONE){E=TEXT("Choose a welding station in this plant");return false;}
 if(State.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&P.Place==Place::Buffer;})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==Id;})){E=TEXT("Return the station's supplies and finish its transport before removing it");return false;}
 State.WeldingStations.RemoveAt(I);++State.Revision;return true;
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
 }
 return true;
}
