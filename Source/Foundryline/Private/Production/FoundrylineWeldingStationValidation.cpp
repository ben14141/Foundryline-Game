// Load-time validation of every welding station record and its stock.
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "FoundrylineWeldingStationInternal.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;using Catalog=FFoundrylineFactoryCatalog;
using FoundrylineWeldingStationInternal::Overlap;using FoundrylineWeldingStationInternal::Access;}
bool FFoundrylineFactorySimulation::WeldingStationsValid(FString& E)const{
 if(State.WeldingStations.Num()>Layout::MaxStations){E=TEXT("Too many welding stations in this plant");return false;}
 const FBox2D Floor(State.FloorMin,State.FloorMax);TSet<int> Numbers;
 for(int I=0;I<State.WeldingStations.Num();++I){const auto& S=State.WeldingStations[I];
  if(S.Number<1||S.Number>99||Numbers.Contains(S.Number)||S.Center.ContainsNaN()||S.QuarterTurns<0||S.QuarterTurns>3||FMath::Abs(S.InDockOffsetFt)>Layout::MaxDockOffsetFt||FMath::Abs(S.OutDockOffsetFt)>Layout::MaxDockOffsetFt){E=TEXT("Invalid welding station record");return false;}Numbers.Add(S.Number);
  const auto B=Layout::Bounds(S);if(!Floor.IsInsideOrOn(B.Min)||!Floor.IsInsideOrOn(B.Max)){E=TEXT("Saved welding station is outside the shop floor");return false;}
  for(int J=I+1;J<State.WeldingStations.Num();++J)if(Overlap(B,Layout::Bounds(State.WeldingStations[J]))){E=TEXT("Saved welding stations overlap");return false;}
  for(const auto& L:State.Lanes)if(Overlap(B,FBox2D(L.Min,L.Max))){E=TEXT("Saved welding station overlaps a lane; layout was not loaded");return false;}
  for(const auto& Z:State.Zones)if(Overlap(B,FFoundrylineWorkZoneLayout::Bounds(Z))){E=TEXT("Saved welding station overlaps a work zone");return false;}
  FFoundrylineAssemblyRecipe R;
  if(S.Recipe<0||S.Recipe>Catalog::RecipeCount||S.Recipe&&!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<0||S.BatchRemaining>100||!S.Recipe&&S.BatchRemaining||!FMath::IsFinite(S.UnloadSeconds)||S.UnloadSeconds<0||S.UnloadSeconds>Layout::UnloadSeconds()){E=TEXT("Invalid welding station recipe or unload state");return false;}
  // Station stock: at most one pallet on IN, bins within their physical capacity, one stack per OUT pallet within its height.
  int OnIn=0,OnOut=0,OutSlots=0;TArray<int> Bins;Bins.Init(0,Catalog::PartCount);
  for(const auto& P:State.Pallets){if(P.Owner!=S.Id)continue;
   if(P.Place==Place::StationIn){++OnIn;if(!Layout::BinCapacity(P.Part)||P.Quantity>Layout::BinCapacity(P.Part)){E=TEXT("Invalid pallet on a welding station IN port");return false;}}
   else if(P.Place==Place::StationOut){OnOut+=P.Quantity;const int Slot=Layout::OutSlot(S,P.Position);
    if(P.Part!=Part::HeadacheRack||P.Quantity<1||(P.Quantity>Layout::RacksPerOutPallet&&!(P.bLegacyFiveStack&&P.Quantity==5))||P.Reserved<0||P.Reserved>P.Quantity||Slot<0||OutSlots&(1<<Slot)){E=TEXT("Invalid product stack on a welding station OUT pallet");return false;}OutSlots|=1<<Slot;}
   else if(P.Place==Place::Buffer){if(!Layout::BinCapacity(P.Part)||P.BookingId||P.OrderId){E=TEXT("Invalid welding station bin stock");return false;}Bins[int(P.Part)]+=P.Quantity;}}
  if(OnIn>1){E=TEXT("A welding station port holds more than one pallet");return false;}
  // The unit on the bench holds exactly its recipe's kit; an empty bench holds nothing.
  int32 FreeSlot=0,FreeLevel=0;const bool HasOutRoom=Layout::NextOutSpot(S,State.Pallets,FreeSlot,FreeLevel);
  FFoundrylineAssemblyRecipe Unit;const bool HasUnit=Catalog::Recipe(S.UnitRecipe,Unit);
  if(S.Stage<0||S.Stage>4||S.Reserved.Num()!=Catalog::PartCount||!FMath::IsFinite(S.Work)||S.Work<0||S.Work>Layout::StageSeconds(FMath::Clamp(S.Stage,0,4))||!FMath::IsFinite(S.ReservedLb)||S.ReservedLb<0||S.UnitsCompleted<0
   ||(S.Stage?!HasUnit||S.Reserved!=Unit.Bill||S.Stage==4&&!HasOutRoom:S.UnitRecipe||S.ReservedLb>0||S.Reserved.ContainsByPredicate([](int Q){return Q!=0;}))){E=TEXT("Invalid welding station assembly progress");return false;}
  for(int P=0;P<Catalog::PartCount;++P)if(Bins[P]>(CollectorSupplies(S.Id,P)?FFoundrylineFactoryCatalog::Part(Part(P)).Capacity:Layout::BinCapacity(Part(P)))){E=TEXT("Welding station bins hold more than they can fit");return false;}
 }
 for(const auto& P:State.Pallets)if((P.Place==Place::StationIn||P.Place==Place::StationOut)&&!State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==P.Owner;})){E=TEXT("Station stock has no welding station");return false;}
 return true;

}
