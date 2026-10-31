#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoredMachinePurchase,"Foundryline.Commerce.StoredMachinePurchaseAndPlacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoredMachinePurchase::RunTest(const FString&){
 FFoundrylineFactorySimulation Model;Model.SeedCutPartsShop();Model.State.Running=false;Model.State.FloorMin={-12000,-12000};Model.State.FloorMax={12000,12000};Model.State.Commerce.CashCents=10000000;
 FString Error;const int32 ActiveSlots=Model.State.Slots.Num();const int64 Cash=Model.State.Commerce.CashCents;
 for(int Kind=1;Kind<=3;++Kind){
  const int BeforeMachines=Model.State.Machines.Num();const int64 Before=Model.State.Commerce.CashCents;
  TestTrue(TEXT("Office can buy equipment without finding a floor location: ")+Error,Model.BuyStoredMachine(Kind,Error));
  if(Model.State.Machines.Num()!=BeforeMachines+1){AddError(Error);return false;}
  const auto Purchased=Model.State.Machines.Last();TestEqual(TEXT("Correct kind purchased"),Purchased.Kind(),Kind);TestTrue(TEXT("New purchase awaits placement"),Purchased.bStored&&!Purchased.Enabled);
  TestEqual(TEXT("Purchase takes exactly one machine price"),Model.State.Commerce.CashCents,Before-FFoundrylinePlanningEquipment::Price(Kind+3,1));
  TestEqual(TEXT("Unplaced racks do not appear on the floor"),Model.State.Slots.Num(),ActiveSlots+(Kind==1?0:Kind==2?6:11));
  TestEqual(TEXT("Purchase retains complete independent input/output storage"),Purchased.StoredSlots.Num(),Kind==1?6:5);
  const FVector At(4500,Kind*3000-6000,20);const int64 Paid=Model.State.Commerce.CashCents;
  TestTrue(TEXT("Owned machine places through the existing planner: ")+Error,Model.EditPlanningItem(1,Purchased.Id,At,false,Error,Kind%4));
  const auto* Placed=Model.State.Machines.FindByPredicate([&](const auto& M){return M.Id==Purchased.Id;});
  TestTrue(TEXT("Placement preserves purchased identity and chosen position"),Placed&&!Placed->bStored&&Placed->Enabled&&Placed->Origin.Equals(At));
  TestEqual(TEXT("Placement does not charge again"),Model.State.Commerce.CashCents,Paid);
  TestTrue(TEXT("Placement retains valid routes and stock ledger: ")+Error,Model.Valid(Error));
 }
 TestTrue(TEXT("Purchase a second independent plasma"),Model.BuyStoredMachine(1,Error));const auto Last=Model.State.Machines.Last();
 TestEqual(TEXT("Second plasma has its own six rack positions"),Last.StoredSlots.Num(),6);TestTrue(TEXT("Separate machine ID"),Last.Id!=Model.State.Machines[Model.State.Machines.Num()-2].Id);
 Model.State.Commerce.CashCents=0;FString Before;FJsonObjectConverter::UStructToJsonObjectString(Model.State,Before);
 TestFalse(TEXT("Insufficient money cannot buy a machine"),Model.BuyStoredMachine(2,Error));FString After;FJsonObjectConverter::UStructToJsonObjectString(Model.State,After);TestEqual(TEXT("Rejected purchase is atomic"),After,Before);
 TestFalse(TEXT("Invalid equipment kind rejected"),Model.BuyStoredMachine(5,Error));
 Model.State.Commerce.CashCents=120000;const int Drivers=Model.State.Drivers.Num();
 TestTrue(TEXT("Forklift bought before choosing a location"),Model.BuyStoredMachine(4,Error));TestEqual(TEXT("Bought forklift waits unplaced"),Model.State.UnplacedForklifts,1);TestEqual(TEXT("Office purchase does not spawn a driver or parked vehicle"),Model.State.Drivers.Num(),Drivers);TestEqual(TEXT("Forklift price charged once"),Model.State.Commerce.CashCents,int64(0));
 TestFalse(TEXT("Invalid forklift placement keeps ownership token"),Model.BuyPlanningEquipment(3,{99999,99999,20},0,1,Error));TestEqual(TEXT("Rejected placement retains paid forklift"),Model.State.UnplacedForklifts,1);
 TestTrue(TEXT("Prepaid forklift places even with no cash left: ")+Error,Model.BuyPlanningEquipment(3,{-4500,4500,20},1,1,Error));TestEqual(TEXT("Placement consumes one token"),Model.State.UnplacedForklifts,0);TestEqual(TEXT("Placement creates exactly one real vehicle"),Model.State.Drivers.Num(),Drivers+1);TestEqual(TEXT("Forklift placement does not charge twice"),Model.State.Commerce.CashCents,int64(0));
 TestFalse(TEXT("Planner cannot create an unpurchased forklift"),Model.BuyPlanningEquipment(3,{-4500,3500,20},0,1,Error));return true;
}
#endif
