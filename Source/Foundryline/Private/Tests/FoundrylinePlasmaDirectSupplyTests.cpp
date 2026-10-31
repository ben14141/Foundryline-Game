#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Economy/FoundrylineEconomyTuning.h"

namespace PlasmaDirectSupplyTest {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;
FFoundrylineFactorySimulation Ready(){FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Commerce.CashCents=2000000;M.State.FloorMin={-12000,-12000};M.State.FloorMax={12000,12000};for(auto& D:M.State.Drivers)D.Priorities={-1,-1,-1};for(auto& O:M.State.Operators){O.Position=O.Target=O.WorkPoint;O.Path.Reset();}M.Solids=FFoundrylineWorkshopLayout::PlantSolids(M.State);return M;}
TArray<int> Inputs(const FFoundrylineFactorySimulation& M,int Machine=1){TArray<int> Out;for(const auto& S:M.State.Slots)if(S.MachineId==Machine&&S.bInputSupply&&S.Part==Part::Sheet)Out.Add(S.Id);return Out;}
int Sheets(FFoundrylineFactorySimulation& M,int Owner,int Quantity,Place Where=Place::Rack){
 const auto* Slot=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==Owner;});const auto* Machine=M.State.Machines.FindByPredicate([&](const auto& A){return A.Id==Owner;});if(Where==Place::Rack&&!Slot||Where==Place::MachineInput&&!Machine)return 0;
 FFoundrylineFactoryPallet P;P.Id=M.State.NextId++;P.Part=Part::Sheet;P.Quantity=Quantity;P.UnitLb=FFoundrylineSteelSheet::PoundsEach();P.Owner=Owner;P.Place=Where;P.Position=Slot&&Where==Place::Rack?Slot->Position:Machine->InputStock;M.State.Pallets.Add(P);M.State.ReceivedSteelLb+=P.Quantity*P.UnitLb;return P.Id;
}
void Run(FFoundrylineFactorySimulation& M,int Ticks){M.State.Running=true;M.State.Speed=1;for(int I=0;I<Ticks;++I)M.Step(.1f);}
bool FinishBracket(FFoundrylineFactorySimulation& M){for(int I=0;I<10000&&M.Total(Part::Bracket)<1;++I)Run(M,1);return M.Total(Part::Bracket)==1;}
bool HasInternalTrip(const FFoundrylineFactorySimulation& M){return M.State.Jobs.ContainsByPredicate([](const auto& J){return J.DestinationPlace==Place::MachineInput&&J.Destination==1;});}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlasmaAttachedRackOnly,"Foundryline.Commerce.PlasmaSupply.OnlyOwnRackFeedsTable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlasmaAttachedRackOnly::RunTest(const FString&){
 using namespace PlasmaDirectSupplyTest;auto M=Ready();FString Error;const auto Own=Inputs(M);
 if(!TestEqual(TEXT("Modern plasma has two attached sheet positions"),Own.Num(),2))return false;
 if(!TestTrue(TEXT("Place a separate plasma workspace"),M.BuyPlanningEquipment(4,{6000,6000,20},0,1,Error))){AddError(Error);return false;}
 const int OtherMachine=M.State.Machines.Last().Id;const auto OtherSlots=Inputs(M,OtherMachine);
 if(!TestTrue(TEXT("Other machine has its own separate sheet rack"),OtherSlots.Num()>0))return false;
 const int OtherPallet=Sheets(M,OtherSlots[0],1);M.State.Machines.Last().Enabled=false;
 if(!TestTrue(TEXT("Program a bracket at the original table"),M.Program(1,Part::Bracket,1,false,Error))){AddError(Error);return false;}
 Run(M,100);TestEqual(TEXT("Empty attached rack keeps cutting stopped"),M.State.Machines[0].Remaining,0.f);TestEqual(TEXT("No sheet can be pulled remotely from another machine"),M.State.RawConsumed,0);TestEqual(TEXT("Other machine keeps its sheet"),M.Available(Part::Sheet,Place::Rack,OtherSlots[0]),1);TestTrue(TEXT("Empty rack gives restocking guidance"),M.State.Status.Contains(TEXT("input rack")));TestTrue(TEXT("Other input buffers are not forklift refill sources"),M.State.Jobs.IsEmpty());
 TestTrue(TEXT("Add sheets to the second own input position"),Sheets(M,Own[1],2)>0);Run(M,1);
 TestEqual(TEXT("Sheet loading still takes time"),M.State.RawConsumed,0);TestTrue(TEXT("Existing five-minute sheet-load progress is retained"),M.State.Machines[0].SheetLoad>0&&M.State.Machines[0].SheetLoad<FFoundrylineEconomyTuning::SheetLoadMinutes);
 TestTrue(TEXT("The table directly pulls a sheet and cuts the bracket"),FinishBracket(M));TestEqual(TEXT("Exactly one sheet was loaded"),M.State.RawConsumed,1);TestEqual(TEXT("One sheet remains in its input pallet"),M.Available(Part::Sheet,Place::Rack,Own[1]),1);TestTrue(TEXT("Unrelated pallet identity remains present"),M.State.Pallets.ContainsByPredicate([&](const auto& P){return P.Id==OtherPallet&&P.Quantity==1;}));TestFalse(TEXT("No forklift rack-to-table job is invented"),HasInternalTrip(M));TestTrue(TEXT("Direct draw conserves all material"),FMath::IsNearlyEqual(M.SteelMass(),M.State.ReceivedSteelLb,.2f));if(!TestTrue(TEXT("Direct-supply ledger validates"),M.Valid(Error)))AddError(Error);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlasmaReservedInput,"Foundryline.Commerce.PlasmaSupply.ReservedSheetIsNeverConsumed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlasmaReservedInput::RunTest(const FString&){
 using namespace PlasmaDirectSupplyTest;auto M=Ready();FString Error;const auto Own=Inputs(M);if(Own.IsEmpty()){AddError(TEXT("Modern plasma input slots are missing"));return false;}
 const int Pallet=Sheets(M,Own[0],1);const auto* Slot=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==Own[0];});const auto* Bulk=M.State.Slots.FindByPredicate([](const auto& S){return S.Id==100;});if(!Slot||!Bulk)return false;
 FFoundrylineFactoryJob J;J.Id=M.State.NextId++;J.SourcePallet=Pallet;J.SourcePlace=Place::Rack;J.Part=Part::Sheet;J.Quantity=1;J.Destination=100;J.DestinationPlace=Place::Rack;J.Pickup=Slot->Dock;J.Drop=Bulk->Dock;J.StoredAt=Bulk->Position;J.Task=EFoundrylineForkTask::Storage;M.State.Jobs.Add(J);M.State.Pallets.Last().Reserved=1;
 if(!TestTrue(TEXT("Reserved-input fixture validates"),M.Valid(Error))){AddError(Error);return false;}
 TestTrue(TEXT("Program a bracket"),M.Program(1,Part::Bracket,1,false,Error));const float Mass=M.SteelMass();Run(M,100);
 TestEqual(TEXT("Reserved sheet is not loaded"),M.State.RawConsumed,0);const auto* Retained=M.State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Pallet;});TestTrue(TEXT("Reserved sheet never disappears"),Retained&&Retained->Quantity==1);TestEqual(TEXT("No loading progress without available stock"),M.State.Machines[0].SheetLoad,0.f);TestTrue(TEXT("Reservation preserves material"),FMath::IsNearlyEqual(M.SteelMass(),Mass,.001f));
 M.State.Jobs.Reset();for(auto& P:M.State.Pallets)if(P.Id==Pallet)P.Reserved=0;
 TestTrue(TEXT("Releasing the reservation allows direct supply"),FinishBracket(M));TestEqual(TEXT("Released sheet is loaded exactly once"),M.State.RawConsumed,1);TestFalse(TEXT("Release does not schedule an internal forklift leg"),HasInternalTrip(M));if(!TestTrue(TEXT("Released-stock ledger validates"),M.Valid(Error)))AddError(Error);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlasmaRackReplenishment,"Foundryline.Commerce.PlasmaSupply.ForkliftsReplenishRackNotTable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlasmaRackReplenishment::RunTest(const FString&){
 using namespace PlasmaDirectSupplyTest;auto M=Ready();FString Error;const auto Own=Inputs(M);if(!TestEqual(TEXT("Two direct-input destinations"),Own.Num(),2))return false;
 const auto* Bulk=M.State.Slots.FindByPredicate([](const auto& S){return S.Id==100;});if(!TestTrue(TEXT("Bulk raw storage is independent from the machine"),Bulk&&Bulk->MachineId==0))return false;
 const int Source=Sheets(M,100,4);TestTrue(TEXT("Program a bracket batch"),M.Program(1,Part::Bracket,1,false,Error));for(auto& R:M.State.Routes)R.Enabled=false;Run(M,2);TestTrue(TEXT("Disabled routes cannot reserve refill loads"),M.State.Jobs.IsEmpty());TestEqual(TEXT("External rack cannot directly feed the table"),M.State.RawConsumed,0);
 for(auto& R:M.State.Routes)R.Enabled=true;Run(M,1);
 const auto* Refill=M.State.Jobs.FindByPredicate([&](const auto& J){return J.SourcePallet==Source&&J.DestinationPlace==Place::Rack&&Own.Contains(J.Destination);});TestTrue(TEXT("Real forklift request moves external sheets to attached rack"),Refill&&Refill->Quantity==4&&Refill->SourcePlace==Place::Rack);const int Jobs=M.State.Jobs.Num();
 Run(M,30);TestEqual(TEXT("Reserved source and incoming counts prevent duplicate refill requests"),M.State.Jobs.Num(),Jobs);TestFalse(TEXT("No fictitious forklift delivery from rack to cutting bed"),HasInternalTrip(M));TestEqual(TEXT("Table waits until the refill actually arrives"),M.State.RawConsumed,0);TestEqual(TEXT("Undelivered stock remains physically at bulk storage"),M.Total(Part::Sheet),4);TestTrue(TEXT("Waiting refill preserves material"),FMath::IsNearlyEqual(M.SteelMass(),M.State.ReceivedSteelLb,.001f));if(!TestTrue(TEXT("Rack refill request validates"),M.Valid(Error)))AddError(Error);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlasmaLegacySheetCompatibility,"Foundryline.Commerce.PlasmaSupply.LegacyInputStockAndTransportSurvive",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlasmaLegacySheetCompatibility::RunTest(const FString&){
 using namespace PlasmaDirectSupplyTest;auto M=Ready();FString Error;TestTrue(TEXT("Legacy table-side stock can be restored in a modern save"),Sheets(M,1,1,Place::MachineInput)>0);TestTrue(TEXT("Program legacy-stock bracket"),M.Program(1,Part::Bracket,1,false,Error));TestTrue(TEXT("Existing table-side stock is consumed rather than orphaned"),FinishBracket(M));TestEqual(TEXT("Legacy sheet is loaded once"),M.State.RawConsumed,1);TestEqual(TEXT("Old machine input is drained"),M.Available(Part::Sheet,Place::MachineInput,1),0);TestTrue(TEXT("Legacy stock mass preserved"),FMath::IsNearlyEqual(M.SteelMass(),M.State.ReceivedSteelLb,.2f));
 FFoundrylineFactorySimulation Legacy;Legacy.SeedCutPartsShop(false);TestFalse(TEXT("Practice workshop retains legacy transport model"),Legacy.State.Machines[0].bRackOutput);TestTrue(TEXT("Program legacy transport fixture"),Legacy.Program(1,Part::Bracket,1,false,Error));Run(Legacy,1);TestTrue(TEXT("Legacy workshop still uses its rack-to-table forklift job"),HasInternalTrip(Legacy));if(!TestTrue(TEXT("Legacy simulation remains valid"),Legacy.Valid(Error)))AddError(Error);return true;
}
#endif
