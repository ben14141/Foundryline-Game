#pragma once
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"

// Shared setup for the welding station automation tests.
namespace FoundrylineWeldingTestHelpers {
// The reorganised cut-parts starter: three paint strokes (west fork lane, walkway, aisle crossing), then the station north of
// the burn table. The same plan as Foundryline.Factory.WeldingStation.StarterShop.
inline bool PaintStarterAndPlaceStation(FFoundrylineFactorySimulation& M,FString& E){
 return M.PaintLane(EFoundrylineFloorLane::Forklift,{-2217.6,1275,20},{-2217.6,2170,20},304.8f,0,E)&&M.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1455,1214.04,20},{-810,1214.04,20},121.92f,0,E)
  &&M.PaintLane(EFoundrylineFloorLane::Crossing,{-1370,1450,20},{-1370,1450,20},200,0,E)&&M.AddWeldingStation({-1768.02,1823.12,20},0,false,E);
}
// Seeded starter with $10,000 and the station placed.
inline bool StarterStation(FFoundrylineFactorySimulation& M,FString& E){M.SeedCutPartsShop(true);M.State.Commerce.CashCents=1000000;return PaintStarterAndPlaceStation(M,E);}
// Mass of one headache-rack kit: 2 plates, 4 brackets, 6 frame sections, 2 light kits.
constexpr float KitLb=2*2+4*5+6*16+2*4.5f;
// Two whole kits in the station's IN stock (as Phase 2 delivers them): added on top of what is there, or topped up to exactly two.
inline void KitStock(FFoundrylineFactorySimulation& M,const FFoundrylineWeldingStationRecord& S,bool TopUp){
 for(auto [P,Q,Lb]:{TTuple<EFoundrylinePart,int,float>{EFoundrylinePart::MountingPlate,4,2},{EFoundrylinePart::Bracket,8,5},{EFoundrylinePart::FrameSection,12,16},{EFoundrylinePart::LightKit,4,4.5f}}){
  auto* Stock=TopUp?M.State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==EFoundrylinePalletPlace::Buffer&&A.Part==P;}):nullptr;
  if(Stock){Stock->Quantity=Q;continue;}
  FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=Q;A.Place=EFoundrylinePalletPlace::Buffer;A.Owner=S.Id;A.Position=FFoundrylineWeldingStationLayout::BinPoint(S,P);A.UnitLb=Lb;M.State.Pallets.Add(A);}
}
inline int KitStockOf(const FFoundrylineFactorySimulation& M,int Station,EFoundrylinePart P){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==EFoundrylinePalletPlace::Buffer&&A.Part==P)V+=A.Quantity;return V;}
// Assembly-only checks: no forklift route collects from OUT (Phase 5 covers collection), so finished racks stay there.
inline void LeaveOut(FFoundrylineFactorySimulation& M){for(const auto& S:M.State.WeldingStations)for(auto& R:M.State.Routes)R.Sources.Remove(S.Id);}
// Steps until the condition holds; false if it never does.
template<class F> bool RunUntil(FFoundrylineFactorySimulation& M,F Done,int Steps=20000){for(int I=0;I<Steps;++I){if(Done())return true;M.Step(.1f);}return Done();}
}
