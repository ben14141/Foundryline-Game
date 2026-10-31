#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "JsonObjectConverter.h"

namespace MachineQueueTest {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Action=EFoundrylineCommerceAction;
FString Snapshot(const FFoundrylineFactorySimulation& M){FString Out;FJsonObjectConverter::UStructToJsonObjectString(M.State,Out);return Out;}
int Accept(FFoundrylineFactorySimulation& M,Part P,int Quantity,FString& E){
 auto* O=M.State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Status==EFoundrylineOrderState::Offered&&V.Part==uint8(P);});if(!O){E=TEXT("Test requires an offered compatible part");return 0;}
 O->Quantity=Quantity;O->Payment=EFoundrylinePayment::OnCompletion;const int Id=O->Id;FFoundrylineCommerceCommand C;C.Action=Action::AcceptOrder;C.Id=Id;return M.CommerceCommand(C,E)?Id:0;
}
bool Pump(FFoundrylineFactorySimulation& M,TFunctionRef<bool()> Done,int Limit=14000){M.State.Running=true;M.State.Speed=8;for(int I=0;I<Limit&&!Done();++I)M.Step(.1f);return Done();}
int Pickup(const FFoundrylineFactorySimulation& M,int Order){int N=0;for(const auto& P:M.State.Pallets)if(P.Place==Place::Pickup&&P.OrderId==Order)N+=P.Quantity;return N;}
int Stock(FFoundrylineFactorySimulation& M,int Slot,int Quantity){
 auto* S=M.State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});if(!S)return 0;S->Part=Part::Bracket;
 FFoundrylineFactoryPallet P;P.Id=M.State.NextId++;P.Part=Part::Bracket;P.Quantity=Quantity;P.UnitLb=FFoundrylineFactoryCatalog::Part(P.Part).UnitLb;P.Place=Place::Rack;P.Owner=Slot;P.Position=S->Position;M.State.Pallets.Add(P);M.State.ReceivedSteelLb+=P.Quantity*P.UnitLb;return P.Id;
}
FFoundrylineCommerceCommand Stage(int Order,int Pallet){FFoundrylineCommerceCommand C;C.Action=Action::StagePallet;C.Id=Order;C.Pallets.Add(Pallet);return C;}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMachineJobQueueFlow,"Foundryline.Commerce.MachineJobs.QueueRemainingDemandAndFIFO",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMachineJobQueueFlow::RunTest(const FString&){
 using namespace MachineQueueTest;FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString Error;
 const int Brackets=Accept(M,Part::Bracket,3,Error),Plates=Accept(M,Part::MountingPlate,2,Error);
 if(!TestTrue(TEXT("Accept bracket and plate contracts"),Brackets>0&&Plates>0)){AddError(Error);return false;}
 if(!TestTrue(TEXT("Queue the first bracket"),M.QueueMachineJob(1,Brackets,1,Error))||!TestTrue(TEXT("Queue only the remaining two brackets"),M.QueueMachineJob(1,Brackets,0,Error))){AddError(Error);return false;}
 TestEqual(TEXT("Second entry is remaining demand, not another full order"),M.State.Machines[0].Queue[1].Quantity,2);
 const FString Allocated=Snapshot(M);TestFalse(TEXT("Already allocated contract cannot be queued twice"),M.QueueMachineJob(1,Brackets,0,Error));TestEqual(TEXT("Duplicate queue does not mutate state"),Snapshot(M),Allocated);
 TestTrue(TEXT("Queue the plate contract after brackets"),M.QueueMachineJob(1,Plates,2,Error));
 TestEqual(TEXT("Three explicit FIFO batch entries"),M.State.Machines[0].Queue.Num(),3);
 FFoundrylineCommerceCommand Buy;Buy.Action=Action::BuySteel;Buy.Quantity=1;
 if(!TestTrue(TEXT("Purchase a real sheet and supplier delivery"),M.CommerceCommand(Buy,Error))){AddError(Error);return false;}
 bool SawBracket=false,SawPlate=false,PlateStartedBeforeBracketsFinished=false;
 M.State.Running=true;M.State.Speed=8;
 for(int I=0;I<14000&&(M.Total(Part::Bracket)!=3||M.Total(Part::MountingPlate)!=2);++I){M.Step(.1f);const auto& Machine=M.State.Machines[0];
  if(Machine.Remaining>0){SawBracket|=Machine.Making==Part::Bracket;SawPlate|=Machine.Making==Part::MountingPlate;if(Machine.Making==Part::MountingPlate&&M.Total(Part::Bracket)<3)PlateStartedBeforeBracketsFinished=true;}
 }
 TestTrue(TEXT("Both recipes actually animate through processing cycles"),SawBracket&&SawPlate);TestFalse(TEXT("Plate processing never overtakes earlier bracket batches"),PlateStartedBeforeBracketsFinished);
 TestEqual(TEXT("Exactly the bracket contract quantity is produced"),M.Total(Part::Bracket),3);TestEqual(TEXT("Exactly the plate contract quantity is produced"),M.Total(Part::MountingPlate),2);TestTrue(TEXT("FIFO queue drains"),M.State.Machines[0].Queue.IsEmpty());
 int OutputSlots=0;for(const auto& S:M.State.Slots)OutputSlots+=S.MachineId==1&&S.bMachineOutput;TestEqual(TEXT("Plasma has four designated output positions"),OutputSlots,4);
 for(const auto& P:M.State.Pallets)if(P.Part==Part::Bracket||P.Part==Part::MountingPlate){const auto* Slot=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==P.Owner&&S.MachineId==1&&S.bMachineOutput;});TestTrue(TEXT("Produced pallet is directly on the machine output rack"),P.Place==Place::Rack&&Slot&&P.Position.Equals(Slot->Position,.01));TestEqual(TEXT("Production does not preassign stock to a shipping contract"),P.OrderId,0);}
 TestTrue(TEXT("Steel is conserved across both queued recipes"),FMath::IsNearlyEqual(M.SteelMass(),M.State.ReceivedSteelLb,.2f));if(!TestTrue(TEXT("Completed mixed queue validates"),M.Valid(Error)))AddError(Error);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMachineFullOutputRack,"Foundryline.Commerce.MachineJobs.FullRackStopsBeforeSteelConsumption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMachineFullOutputRack::RunTest(const FString&){
 using namespace MachineQueueTest;FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString Error;TArray<int> Slots;for(const auto& S:M.State.Slots)if(S.MachineId==1&&S.bMachineOutput)Slots.Add(S.Id);
 if(!TestEqual(TEXT("Output fixture has four slots"),Slots.Num(),4))return false;
 for(int Slot:Slots)Stock(M,Slot,FFoundrylineFactoryCatalog::Part(Part::Bracket).Capacity);
 M.State.Machines[0].BedLb=50;M.State.ReceivedSteelLb+=50;
 TestTrue(TEXT("Full rack fixture has a valid ledger"),M.Valid(Error));TestTrue(TEXT("Request one additional bracket"),M.Program(1,Part::Bracket,1,false,Error));
 const float Mass=M.SteelMass(),Bed=M.State.Machines[0].BedLb;const int Count=M.Total(Part::Bracket);M.State.Running=true;M.State.Speed=8;for(int I=0;I<600;++I)M.Step(.1f);
 TestEqual(TEXT("Full rack does not overfill a pallet"),M.Total(Part::Bracket),Count);TestEqual(TEXT("No steel deducted from bed while blocked"),M.State.Machines[0].BedLb,Bed);TestEqual(TEXT("No unplaced material is reserved"),M.State.Machines[0].ReservedLb,0.f);TestEqual(TEXT("Requested unit remains pending"),M.State.Machines[0].BatchRemaining,1);TestEqual(TEXT("No cutting cycle starts without output capacity"),M.State.Machines[0].Remaining,0.f);TestTrue(TEXT("All steel remains accounted for"),FMath::IsNearlyEqual(M.SteelMass(),Mass,.001f));TestTrue(TEXT("Blocked output reports actionable status"),M.State.Status.Contains(TEXT("output rack is full")));if(!TestTrue(TEXT("Blocked plant remains valid"),M.Valid(Error)))AddError(Error);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStagePartialStockPallet,"Foundryline.Commerce.MachineJobs.StagePartialStockViaForklift",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStagePartialStockPallet::RunTest(const FString&){
 using namespace MachineQueueTest;FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString Error;const int Contract=Accept(M,Part::Bracket,25,Error),Other=Accept(M,Part::Bracket,10,Error);
 if(!TestTrue(TEXT("Two accepted contracts available"),Contract>0&&Other>0)){AddError(Error);return false;}
 const int First=Stock(M,107,25),Second=Stock(M,108,30);if(!TestTrue(TEXT("Unassigned rack stock fixture validates"),First>0&&Second>0&&M.Valid(Error))){AddError(Error);return false;}
 auto Disabled=M;for(auto& D:Disabled.State.Drivers)D.Priorities[2]=-1;const FString DisabledBefore=Snapshot(Disabled);
 TestFalse(TEXT("Disabled Shipping cannot silently create a staging request"),Disabled.CommerceCommand(Stage(Contract,First),Error));TestEqual(TEXT("Disabled-driver failure is atomic"),Snapshot(Disabled),DisabledBefore);TestTrue(TEXT("Driver guidance mentions Shipping"),Error.Contains(TEXT("Shipping")));
 if(!TestTrue(TEXT("Stage unassigned half-capacity pallet for its contract"),M.CommerceCommand(Stage(Contract,First),Error))){AddError(Error);return false;}
 const auto* Job=M.State.Jobs.FindByPredicate([&](const auto& J){return J.SourcePallet==First&&J.DestinationPlace==Place::Pickup&&J.OrderId==Contract;});TestTrue(TEXT("Request creates an actual reserved forklift load of 25"),Job&&Job->Quantity==25);
 const FString Reserved=Snapshot(M);TestFalse(TEXT("Duplicate stage click rejected"),M.CommerceCommand(Stage(Contract,First),Error));TestEqual(TEXT("Duplicate click cannot reserve stock twice"),Snapshot(M),Reserved);
 TestFalse(TEXT("Other contract cannot take over an occupied bay"),M.CommerceCommand(Stage(Other,Second),Error));TestEqual(TEXT("Conflicting contract rejection is atomic"),Snapshot(M),Reserved);
 TestTrue(TEXT("Forklift physically carries the half pallet to pickup"),Pump(M,[&](){return Pickup(M,Contract)==25;}));TestEqual(TEXT("Bay quantity equals contract rather than full pallet capacity"),Pickup(M,Contract),25);const auto* Unrelated=M.State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Second;});TestTrue(TEXT("The unrelated stock remains unassigned"),Unrelated&&Unrelated->OrderId==0);
 if(!TestTrue(TEXT("Partial staging preserves valid quantities and reservations"),M.Valid(Error)))AddError(Error);
 FFoundrylineFactorySimulation Tail;Tail.SeedCutPartsShop();const int Ten=Accept(Tail,Part::Bracket,10,Error),Thirty=Stock(Tail,107,30);
 if(!TestTrue(TEXT("Stage only remaining ten from a larger pallet"),Tail.CommerceCommand(Stage(Ten,Thirty),Error))){AddError(Error);return false;}
 const auto* Partial=Tail.State.Jobs.FindByPredicate([&](const auto& J){return J.SourcePallet==Thirty;});TestTrue(TEXT("Only ten units are reserved"),Partial&&Partial->Quantity==10);TestTrue(TEXT("Actual forklift route completes partial load"),Pump(Tail,[&](){return Pickup(Tail,Ten)==10;}));TestEqual(TEXT("Twenty units stay in rack stock"),Tail.Available(Part::Bracket,Place::Rack,107),20);TestEqual(TEXT("Ten units arrive at shipping"),Pickup(Tail,Ten),10);TestTrue(TEXT("Splitting stock conserves steel"),FMath::IsNearlyEqual(Tail.SteelMass(),Tail.State.ReceivedSteelLb,.001f));if(!TestTrue(TEXT("Split-pallet ledger validates"),Tail.Valid(Error)))AddError(Error);return true;
}
#endif
