#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Inventory/FoundrylineSheetStock.h"

void FFoundrylineFactorySimulation::SeedCutPartsShop(bool Commerce){
 State=FFoundrylineFactoryState();State.bCutPartsStarter=true;
 State.FloorMin={-2370,30};State.FloorMax={370,2170};Solids=FFoundrylineWorkshopLayout::CutShopSolids();
 FFoundrylineFactoryMachine M;M.Id=1;M.Origin=FFoundrylineWorkshopLayout::Machine(1);M.InputDock=FFoundrylineWorkshopLayout::InputDock(1);M.InputStock=FFoundrylineWorkshopLayout::InputStock(1);M.OutputDock={-1300,1300,20};M.OutputStock={-1200,1300,20};M.Auto=false;M.BatchRemaining=10;State.Machines.Add(M);
 for(const auto& P:FFoundrylineFactoryCatalog::DefaultPrograms())if(P.Machine==1&&(!Commerce||P.Part==EFoundrylinePart::Bracket||P.Part==EFoundrylinePart::MountingPlate||P.Part==EFoundrylinePart::GussetPlate||P.Part==EFoundrylinePart::ReinforcementPlate))State.Programs.Add(P);
 for(const auto& S:FFoundrylineWorkshopLayout::Slots())if(S.Id==100||S.Id>=107&&S.Id<=109)State.Slots.Add(S);
 if(!Commerce)AddPallet(EFoundrylinePart::Sheet,10,EFoundrylinePalletPlace::Rack,100,State.Slots[0].Position,FFoundrylineSteelSheet::PoundsEach());
 FFoundrylineFactoryRoute R;R.Id=1;R.Name=TEXT("Steel > cutting > finished racks");R.Sources={10,1,100,107,108,109};R.Destinations={1,100,107,108,109};State.Routes.Add(R);
 auto Lane=[&](EFoundrylineFloorLane K,FVector2D A,FVector2D B){FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=K;L.Min=A;L.Max=B;State.Lanes.Add(L);};
 Lane(EFoundrylineFloorLane::Forklift,{-500,200},{-100,2000});Lane(EFoundrylineFloorLane::Forklift,{-2100,1275},{-300,1625});
 Lane(EFoundrylineFloorLane::Pedestrian,{-2350,80},{200,200});
 FString E;AddDriver(1,E);if(Commerce){State.Machines[0].BatchRemaining=0;SeedCommerce();}AddMachineOperator(1,!Commerce);State.Status=TEXT("Cut-parts starter: run the plant, choose a plasma batch, store finished pallets for box-trailer shipping.");
}
