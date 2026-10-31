#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Inventory/FoundrylineSheetStock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterSupplyAccessTest,"Foundryline.Commerce.StarterSupply.AllPhysicalRackDocksReachable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterSupplyAccessTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);FString Error;const auto& Machine=M.State.Machines[0];const auto* Raw=M.State.Slots.FindByPredicate([](const auto& S){return S.Id==100;});
 if(!TestNotNull(TEXT("Starter raw storage exists"),Raw)||!TestTrue(TEXT("Starter has a real forklift"),!M.State.Drivers.IsEmpty()))return false;
 const auto Parking=M.State.Drivers[0].Parking;TestTrue(TEXT("Actual seeded envelope stays bounded; no giant test floor"),M.State.FloorMin.Equals({-2370,30})&&M.State.FloorMax.Equals({1600,2170}));
 TestTrue(TEXT("Forklift reaches the raw storage dock with physical clearance"),!M.Path(Parking,Raw->Dock).IsEmpty());
 int Inputs=0,Outputs=0;for(const auto& Slot:M.State.Slots)if(Slot.MachineId==Machine.Id){
  TestTrue(FString::Printf(TEXT("Real rack slot %d is reachable from parking (%s)"),Slot.Id,*Slot.Dock.ToString()),!M.Path(Parking,Slot.Dock).IsEmpty());
  if(Slot.bInputSupply){++Inputs;TestTrue(TEXT("Stored sheets can travel from raw storage to each machine intake level"),!M.Path(Raw->Dock,Slot.Dock).IsEmpty());}
  if(Slot.bMachineOutput)++Outputs;
 }
 TestTrue(TEXT("Attached input storage has two sheet pallet positions"),Inputs==2);TestEqual(TEXT("Four physical output pallet positions"),Outputs,4);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterRealSheetToCutPartTest,"Foundryline.Commerce.StarterSupply.DeliveryRestocksIntakeThenCuts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterRealSheetToCutPartTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);FString Error;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;
 TestEqual(TEXT("No starter sheets are granted"),M.Total(Part::Sheet),0);TestEqual(TEXT("No starter bracket stock"),M.Total(Part::Bracket),0);
 FFoundrylineCommerceCommand Buy;Buy.Action=EFoundrylineCommerceAction::BuySteel;Buy.Quantity=1;if(!TestTrue(TEXT("Purchase one real carrier-delivered sheet"),M.CommerceCommand(Buy,Error)))return false;
 if(!TestTrue(TEXT("Program one cut part"),M.Program(1,Part::Bracket,1,false,Error)))return false;
 M.State.Running=true;M.State.Speed=8;bool SawTruck=false,SawRaw=false,SawInput=false,SawForks=false;
 for(int Tick=0;Tick<12000&&M.Total(Part::Bracket)<1;++Tick){M.Step(.1f);for(const auto& P:M.State.Pallets)if(P.Part==Part::Sheet){
   SawTruck|=P.Place==Place::Inbound;SawForks|=P.Place==Place::Forks;SawRaw|=P.Place==Place::Rack&&P.Owner==100;
   SawInput|=P.Place==Place::Rack&&M.State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==P.Owner&&S.MachineId==1&&S.bInputSupply;});
  }}
 TestTrue(TEXT("Sheet arrived on the carrier"),SawTruck);TestTrue(TEXT("A forklift physically carried material"),SawForks);TestTrue(TEXT("Delivery passed through designated raw storage"),SawRaw);TestTrue(TEXT("Forklift restocked the plasma's own input rack"),SawInput);
 TestEqual(TEXT("One programmed bracket completes without free cargo"),M.Total(Part::Bracket),1);TestEqual(TEXT("Exactly one sheet loaded into the plasma"),M.State.RawConsumed,1);
 TestTrue(TEXT("Purchased steel ledger equals one sheet"),FMath::IsNearlyEqual(M.State.ReceivedSteelLb,FFoundrylineSteelSheet::PoundsEach(),.1f));TestTrue(TEXT("All purchased pounds remain accounted for"),FMath::IsNearlyEqual(M.SteelMass(),M.State.ReceivedSteelLb,.2f));
 TestTrue(TEXT("Real delivery and production leave a valid ledger: ")+Error,M.Valid(Error));return true;
}
#endif
