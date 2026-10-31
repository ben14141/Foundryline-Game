#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Inventory/FoundrylineSheetStock.h"

void FFoundrylineFactorySimulation::SeedCutPartsShop(bool Commerce){
 State=FFoundrylineFactoryState();State.bCutPartsStarter=true;
 State.FloorMin={-2370,30};State.FloorMax={370,2170};Solids=FFoundrylineWorkshopLayout::CutShopSolids();
 FFoundrylineFactoryMachine M;M.Id=1;M.Origin=FFoundrylineWorkshopLayout::Machine(1);M.InputDock=FFoundrylineWorkshopLayout::InputDock(1);M.InputStock=FFoundrylineWorkshopLayout::InputStock(1);M.OutputDock={-1300,1300,20};M.OutputStock={-1200,1300,20};M.Auto=false;M.BatchRemaining=10;State.Machines.Add(M);
 if(Commerce)State.Programs=FFoundrylineFactoryCatalog::StarterPrograms();
 else for(const auto& P:FFoundrylineFactoryCatalog::DefaultPrograms())if(P.Machine==1)State.Programs.Add(P);
 for(const auto& S:FFoundrylineWorkshopLayout::Slots())if(S.Id==100||S.Id>=107&&S.Id<=109)State.Slots.Add(S);
 if(!Commerce)AddPallet(EFoundrylinePart::Sheet,10,EFoundrylinePalletPlace::Rack,100,State.Slots[0].Position,FFoundrylineSteelSheet::PoundsEach());
 FFoundrylineFactoryRoute R;R.Id=1;R.Name=TEXT("Steel > cutting > finished racks");R.Sources={10,1,100,107,108,109};R.Destinations={1,100,107,108,109};State.Routes.Add(R);if(Commerce){InitialiseMachineRacks(1,true);Solids=FFoundrylineWorkshopLayout::PlantSolids(State);}
 auto Lane=[&](EFoundrylineFloorLane K,FVector2D A,FVector2D B){FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=K;L.Min=A;L.Max=B;State.Lanes.Add(L);};
 Lane(EFoundrylineFloorLane::Forklift,{-500,200},{-100,2000});Lane(EFoundrylineFloorLane::Forklift,{-2370,1275},{-300,1635});
 // The west-side input rack needs a legal connected area, not a long unreachable spur.
 if(Commerce)Lane(EFoundrylineFloorLane::Forklift,{-2370,400},{-2010,1450});
 Lane(EFoundrylineFloorLane::Pedestrian,{-2350,80},{200,200});
 FString E;AddDriver(1,E);if(Commerce){State.Machines[0].BatchRemaining=0;SeedCommerce();}AddMachineOperator(1,!Commerce);State.Status=TEXT("Cut-parts starter: run the plant, choose a plasma batch, store finished pallets for box-trailer shipping.");
}
void FFoundrylineFactorySimulation::MigrateCurrentParts(){
 if(!State.bCutPartsStarter||!State.Commerce.Enabled)return;
 // Upgrade the original plasma's saved floor output to its four-slot workspace rack.
 // Reserved historical pallets keep their active trip until it finishes.
 if(auto* Machine=State.Machines.FindByPredicate([](const auto& M){return M.Id==1&&!M.bRackOutput&&!M.bStored;})){
  InitialiseMachineRacks(1,true);
  for(auto& Pallet:State.Pallets)if(Pallet.Place==EFoundrylinePalletPlace::Rack){if(auto* Slot=State.Slots.FindByPredicate([&](const auto& V){return V.Id==Pallet.Owner&&V.bMachineOutput&&V.MachineId==1;})){Slot->Part=Pallet.Part;Pallet.Position=Slot->Position;}}
  for(auto& Pallet:State.Pallets)if(Pallet.Place==EFoundrylinePalletPlace::MachineOutput&&Pallet.Owner==1&&!Pallet.Reserved){for(auto& Slot:State.Slots)if(Slot.MachineId==1&&Slot.bMachineOutput&&!State.Pallets.ContainsByPredicate([&](const auto& Other){return Other.Place==EFoundrylinePalletPlace::Rack&&Other.Owner==Slot.Id;})){Slot.Part=Pallet.Part;Pallet.Place=EFoundrylinePalletPlace::Rack;Pallet.Owner=Slot.Id;Pallet.Position=Slot.Position;break;}}
  if(Machine->Remaining>0)Machine->OutputSlot=SelectMachineOutput(1,Machine->Making);
  Solids=FFoundrylineWorkshopLayout::PlantSolids(State);
 }
 // Upgrade the old integral raw-steel rack to external storage and add the
 // opposite-side machine input rack. Existing stock remains stock, with no lost steel.
 if(auto* Machine=State.Machines.FindByPredicate([](const auto& M){return M.Id==1&&M.bRackOutput&&!M.bStored;}))if(State.Slots.ContainsByPredicate([](const auto& S){return S.Id==100&&S.MachineId==1;})){
  const auto OldRaw=*State.Slots.FindByPredicate([](const auto& S){return S.Id==100;});
  InitialiseMachineRacks(1,true);
  auto* Raw=State.Slots.FindByPredicate([](const auto& S){return S.Id==100;});
  if(!State.Jobs.IsEmpty()){Raw->Position=OldRaw.Position;Raw->Dock=OldRaw.Dock;}
  for(auto& P:State.Pallets)if(P.Place==EFoundrylinePalletPlace::Rack&&P.Owner==100&&!P.Reserved)P.Position=Raw->Position;
  Solids=FFoundrylineWorkshopLayout::PlantSolids(State);
 }
 // Only repair the known starter aisle when upgrading its original intake layout.
 // Custom/moved cells retain the player's painted areas and require their own access.
 if(const auto* M=State.Machines.FindByPredicate([](const auto& V){return V.Id==1&&V.bRackOutput&&!V.bStored&&V.QuarterTurns==0&&V.Origin.Equals({-1300,800,20},.1);})){
  for(auto& L:State.Lanes)if(L.Kind==EFoundrylineFloorLane::Forklift&&L.Min.Equals({-2100,1275},.1)&&L.Max.Equals({-300,1625},.1)){L.Min.X=-2370;L.Max.Y=1635;}
  const bool HasWest=State.Lanes.ContainsByPredicate([](const auto& L){return L.Kind==EFoundrylineFloorLane::Forklift&&L.Min.X<=-2370&&L.Min.Y<=400&&L.Max.X>=-2010&&L.Max.Y>=1450;});
  if(!HasWest){FString AccessError;if(!PaintForkliftArea({-2370,400,20},{-2010,1450,20},AccessError))State.Status=TEXT("Connect a forklift area to the plasma input rack: ")+AccessError;}
 }
 State.Programs.RemoveAll([](const auto& P){return !FFoundrylineFactoryCatalog::AvailableNow(P.Part);});
 for(auto& M:State.Machines){if(M.HasQueuedProgram&&!FFoundrylineFactoryCatalog::AvailableNow(M.NextPart))M.HasQueuedProgram=false;if(!FFoundrylineFactoryCatalog::AvailableNow(M.Making)){M.BatchRemaining=0;M.Auto=false;}}
 // In-progress units and existing stock retain their IDs/weight; retired offers are withdrawn.
 RefreshCommerceOffers();
}
