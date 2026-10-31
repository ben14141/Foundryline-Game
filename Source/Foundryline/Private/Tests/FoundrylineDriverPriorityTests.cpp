#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkliftTaskPriorities,"Foundryline.Commerce.DriverTaskCheckboxes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkliftTaskPriorities::RunTest(const FString&){
 FFoundrylineFactorySimulation Sim;Sim.SeedCutPartsShop();FString Error;
 if(!TestTrue(TEXT("Starter has a forklift"),!Sim.State.Drivers.IsEmpty()))return false;
 const int Driver=Sim.State.Drivers[0].Id;
 auto Set=[&](int Category,int Rank){FFoundrylineCommerceCommand C;C.Action=EFoundrylineCommerceAction::DriverPriority;C.Id=Driver;C.Choice=Category;C.Quantity=Rank;return Sim.CommerceCommand(C,Error);};
 TestTrue(TEXT("Receiving can be disabled"),Set(0,-1));
 TestEqual(TEXT("Disabled persists"),Sim.State.Drivers[0].Priorities[0],-1);
 TestTrue(TEXT("Production can share shipping priority"),Set(1,2));
 TestEqual(TEXT("Changing one category preserves another"),Sim.State.Drivers[0].Priorities[2],2);
 TestTrue(TEXT("Receiving re-enabled at first priority"),Set(0,0));
 TestTrue(TEXT("Production second"),Set(1,1));
 TestTrue(TEXT("Shipping third"),Set(2,2));
 TestTrue(TEXT("All three enabled and ledger valid: ")+Error,Sim.Valid(Error));
 TestFalse(TEXT("Out of range priority rejected"),Set(0,3));
 TestEqual(TEXT("Rejected edit preserves setting"),Sim.State.Drivers[0].Priorities[0],0);
 TestTrue(TEXT("Disable receiving"),Set(0,-1));TestTrue(TEXT("Disable production"),Set(1,-1));TestTrue(TEXT("Disable shipping"),Set(2,-1));
 TestTrue(TEXT("Parked driver with no tasks is valid: ")+Error,Sim.Valid(Error));
 FFoundrylineFactoryJob Job;Job.Task=EFoundrylineForkTask::Truck;Job.SourcePlace=EFoundrylinePalletPlace::Inbound;Job.DestinationPlace=EFoundrylinePalletPlace::Rack;
 TestEqual(TEXT("Inbound unloading is receiving"),Sim.ForkPriorityGroup(Job),0);
 Job.Task=EFoundrylineForkTask::Storage;Job.SourcePlace=EFoundrylinePalletPlace::MachineOutput;
 TestEqual(TEXT("Storing output is production regardless of saleability"),Sim.ForkPriorityGroup(Job),1);
 Job.SourcePlace=EFoundrylinePalletPlace::Rack;Job.DestinationPlace=EFoundrylinePalletPlace::Pickup;
 TestEqual(TEXT("Staging a sold component is shipping"),Sim.ForkPriorityGroup(Job),2);
 return true;
}
#endif
