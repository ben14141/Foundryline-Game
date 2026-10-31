#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicShopBackendTest,"Foundryline.Commerce.BasicShop.EmptyBusinessCanPurchase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBasicShopBackendTest::RunTest(const FString&){FFoundrylineFactorySimulation M;M.State.bCutPartsStarter=true;M.SeedCommerce();M.State.Lanes.Reset();M.State.FloorMin={-2370,30};M.State.FloorMax={-30,1770};FString E;
 const bool InitialValid=M.Valid(E);if(!InitialValid)AddError(E);TestTrue(TEXT("Empty owned shop backend is valid"),InitialValid);auto Assembly=M;TestTrue(TEXT("Empty shop allows a workstation before lanes or forklifts"),Assembly.BuyPlanningEquipment(0,{-1300,900,20},0,1,E));TestTrue(TEXT("Machine-less workstation state saves and validates"),Assembly.Valid(E));
 TestTrue(TEXT("Racks can be purchased without a preinstalled machine"),M.BuyPlanningEquipment(1,{-1700,700,20},0,1,E));TestTrue(TEXT("Purchase keeps owned shop state valid"),M.Valid(E));TestEqual(TEXT("No machines are silently added"),M.State.Machines.Num(),0);TestEqual(TEXT("One rack purchased"),M.State.StorageRacks.Num(),1);
 TestTrue(TEXT("First forklift creates its shop route without manual setup"),M.BuyPlanningEquipment(3,{-800,1000,20},0,1,E));TestEqual(TEXT("One default route"),M.State.Routes.Num(),1);TestTrue(TEXT("Forklift uses the generated route"),M.State.Drivers.Num()==1&&M.State.Drivers[0].Route==M.State.Routes[0].Id);
 M.State.Programs.Add(FFoundrylineFactoryCatalog::DefaultPrograms()[0]);TestFalse(TEXT("An incomplete machine recipe catalog is rejected"),M.Valid(E));return true;}
#endif
