#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "JsonObjectConverter.h"

namespace PlannerRevisionTest {
FFoundrylineFactorySimulation RoomyShop(){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Running=false;
 M.State.Commerce.CashCents=10000000;M.State.FloorMin={-12000,-12000};M.State.FloorMax={12000,12000};
 M.State.Lanes.Reset();M.Solids=FFoundrylineWorkshopLayout::PlantSolids(M.State);return M;
}
FString Snapshot(const FFoundrylineFactorySimulation& M){FString Out;FJsonObjectConverter::UStructToJsonObjectString(M.State,Out);return Out;}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlannerLaneRevisionTest,"Foundryline.Construction.PlannerRevision.LaneRotationResizeAndAtomicity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlannerLaneRevisionTest::RunTest(const FString&){
 using namespace PlannerRevisionTest;auto M=RoomyShop();FString Error;
 FFoundrylineFactoryLane Lane;Lane.Id=M.State.NextId++;Lane.Kind=EFoundrylineFloorLane::Forklift;Lane.Min={4000,4000};Lane.Max={5200,4300};M.State.Lanes.Add(Lane);
 const FVector Center((Lane.Min+Lane.Max)/2,20);const int Kind=int(EFoundrylinePlannerItem::Lane);
 if(!TestTrue(TEXT("Lane fixture is a valid owned shop"),M.Valid(Error))){AddError(Error);return false;}
 // Both one-way signs must follow actual rotation from either starting axis.
 for(bool AlongX:{true,false})for(int Direction:{1,2})for(int Quarter=0;Quarter<4;++Quarter){
  auto OneWay=M;auto& Original=OneWay.State.Lanes[0];Original.Direction=Direction;
  Original.Min=FVector2D(Center)-(AlongX?FVector2D(600,150):FVector2D(150,600));
  Original.Max=FVector2D(Center)+(AlongX?FVector2D(600,150):FVector2D(150,600));
  const FVector Before=(AlongX?FVector::ForwardVector:FVector::RightVector)*(Direction==1?1.:-1.);
  const FVector Expected=FRotator(0,Quarter*90,0).RotateVector(Before);
  if(!TestTrue(FString::Printf(TEXT("Rotate %s one-way sign%d quarter%d"),AlongX?TEXT("X"):TEXT("Y"),Direction,Quarter),OneWay.EditPlanningItem(Kind,Lane.Id,Center,false,Error,Quarter))){AddError(Error);continue;}
  const auto& Saved=OneWay.State.Lanes[0];const bool IsX=Saved.Max.X-Saved.Min.X>Saved.Max.Y-Saved.Min.Y;
  const FVector Actual=(IsX?FVector::ForwardVector:FVector::RightVector)*(Saved.Direction==1?1.:-1.);
  TestTrue(TEXT("One-way arrows and navigation flow retain rotated direction"),Actual.Equals(Expected,.001));
 }
 if(!TestTrue(TEXT("Quarter turn rotates an existing lane"),M.EditPlanningItem(Kind,Lane.Id,Center,false,Error,1))){AddError(Error);return false;}
 TestTrue(TEXT("Rotation swaps lane length and width"),(M.State.Lanes[0].Max-M.State.Lanes[0].Min).Equals({300,1200},.01));
 const FString Rotated=Snapshot(M);const auto Revision=M.State.Revision;
 TestFalse(TEXT("Too-small lane resize rejected"),M.EditPlanningItem(Kind,Lane.Id,Center,false,Error,-1,{50,300,0}));
 TestEqual(TEXT("Invalid resize leaves the entire state unchanged"),Snapshot(M),Rotated);
 TestEqual(TEXT("Invalid resize does not advance revision"),M.State.Revision,Revision);
 for(const FVector TooNarrow:{FVector(359,1200,0),FVector(1200,359,0),FVector(100,100,0)}){
  TestFalse(TEXT("Resize cannot bypass the painted forklift lane minimum"),M.EditPlanningItem(Kind,Lane.Id,Center,false,Error,-1,TooNarrow));
  TestEqual(TEXT("Narrow lane rejection preserves layout and reservations"),Snapshot(M),Rotated);
 }
 auto Minimum=M;
 TestTrue(TEXT("A forklift area with 360cm clear width remains valid"),Minimum.EditPlanningItem(Kind,Lane.Id,Center,false,Error,-1,{360,1200,0}));
 const FVector Machine=M.State.Machines[0].Origin;
 TestFalse(TEXT("Lane cannot move over machinery"),M.EditPlanningItem(Kind,Lane.Id,Machine,false,Error));
 TestEqual(TEXT("Rejected overlap leaves the entire state unchanged"),Snapshot(M),Rotated);
 // The planner previews on a copy. Discarding that copy is cancellation, not a committed edit.
 auto Preview=M;
 if(!TestTrue(TEXT("A valid move can be previewed"),Preview.EditPlanningItem(Kind,Lane.Id,Center+FVector(600,0,0),false,Error))){AddError(Error);return false;}
 TestEqual(TEXT("Cancelled preview never changes the live shop"),Snapshot(M),Rotated);
 TestTrue(TEXT("Explicit rectangular lane resize succeeds"),M.EditPlanningItem(Kind,Lane.Id,Center,false,Error,-1,{600,1600,0}));
 TestTrue(TEXT("Requested rectangle is the saved rectangle"),(M.State.Lanes[0].Max-M.State.Lanes[0].Min).Equals({600,1600},.01));
 TestTrue(TEXT("Existing lane can be removed"),M.EditPlanningItem(Kind,Lane.Id,Center,true,Error));
 TestFalse(TEXT("Removed lane is no longer selectable"),FFoundrylinePlannerItems::List(M.State).ContainsByPredicate([&](const auto& I){return I.Kind==EFoundrylinePlannerItem::Lane&&I.Id==Lane.Id;}));
 TestTrue(TEXT("Lane edits leave a valid ledger"),M.Valid(Error));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlannerRackRevisionTest,"Foundryline.Construction.PlannerRevision.RackRotationMovesSlotsAndDocks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlannerRackRevisionTest::RunTest(const FString&){
 using namespace PlannerRevisionTest;auto M=RoomyShop();FString Error;const FVector Before(4500,4500,20),After(5000,4500,20);const FRotator Rotation(0,90,0);
 if(!TestTrue(TEXT("Purchase a clear two-level parts rack"),M.BuyPlanningEquipment(1,Before,0,2,Error))){AddError(Error);return false;}
 const auto Rack=M.State.StorageRacks.Last();const auto Slots=M.State.Slots.FilterByPredicate([&](const auto& S){return Rack.Slots.Contains(S.Id);});
 TestEqual(TEXT("Two levels provide four pallet positions"),Slots.Num(),4);
 if(!TestTrue(TEXT("Move and rotate an empty rack in one action"),M.EditPlanningItem(int(EFoundrylinePlannerItem::StorageRack),Rack.Id,After,false,Error,1))){AddError(Error);return false;}
 const auto* Saved=M.State.StorageRacks.FindByPredicate([&](const auto& R){return R.Id==Rack.Id;});
 TestTrue(TEXT("Rack saves new center and quarter turn"),Saved&&Saved->Center.Equals(After)&&Saved->QuarterTurns==1);
 for(const auto& Old:Slots){const auto* New=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==Old.Id;});
  TestTrue(FString::Printf(TEXT("Slot %d keeps identity and rotates with rack"),Old.Id),New&&New->Position.Equals(After+Rotation.RotateVector(Old.Position-Before),.01));
  TestTrue(FString::Printf(TEXT("Slot %d forklift dock follows rack rotation"),Old.Id),New&&New->Dock.Equals(After+Rotation.RotateVector(Old.Dock-Before),.01));
 }
 const FString Stable=Snapshot(M);
 TestFalse(TEXT("Invalid quarter turn is rejected"),M.EditPlanningItem(int(EFoundrylinePlannerItem::StorageRack),Rack.Id,After,false,Error,4));
 TestEqual(TEXT("Invalid rotation is atomic"),Snapshot(M),Stable);
 TestFalse(TEXT("Rack cannot be moved through a machine"),M.EditPlanningItem(int(EFoundrylinePlannerItem::StorageRack),Rack.Id,M.State.Machines[0].Origin,false,Error,1));
 TestEqual(TEXT("Overlap rejection preserves rack, slots, and cash"),Snapshot(M),Stable);
 TestTrue(TEXT("Rotated rack layout validates"),M.Valid(Error));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlannerMachineInstancesTest,"Foundryline.Construction.PlannerRevision.IndependentMachinePurchases",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlannerMachineInstancesTest::RunTest(const FString&){
 using namespace PlannerRevisionTest;auto M=RoomyShop();FString Error;
 struct Purchase {int Item;FVector At;int Quarter;};
 const Purchase Choices[]={{4,{4000,4000,20},0},{4,{7000,4000,20},1},{5,{4000,7500,20},2},{6,{8000,7500,20},3}};
 TArray<int32> Purchased;TSet<int32> AllIds;for(const auto& Machine:M.State.Machines)AllIds.Add(Machine.Id);for(const auto& Slot:M.State.Slots)AllIds.Add(Slot.Id);
 for(const auto& Choice:Choices){const int Count=M.State.Machines.Num();const int64 Cash=M.State.Commerce.CashCents;
  if(!TestTrue(FString::Printf(TEXT("Purchase and place equipment item %d"),Choice.Item),M.BuyPlanningEquipment(Choice.Item,Choice.At,Choice.Quarter,1,Error))){AddError(Error);return false;}
  TestEqual(TEXT("Each purchase creates exactly one independent machine"),M.State.Machines.Num(),Count+1);
  const auto& Machine=M.State.Machines.Last();TestFalse(TEXT("Machine identity does not reuse any machine or rack slot"),AllIds.Contains(Machine.Id));AllIds.Add(Machine.Id);Purchased.Add(Machine.Id);
  TestEqual(TEXT("Instance retains equipment kind independently of ID"),Machine.Kind(),Choice.Item-3);TestEqual(TEXT("Placement keeps purchased rotation"),Machine.QuarterTurns,Choice.Quarter);TestTrue(TEXT("Placement keeps chosen position"),Machine.Origin.Equals(Choice.At));TestTrue(TEXT("Purchase charges the business"),M.State.Commerce.CashCents<Cash);
  int Outputs=0,Inputs=0;for(const auto& Slot:M.State.Slots)if(Slot.MachineId==Machine.Id){Outputs+=Slot.bMachineOutput;Inputs+=Slot.bInputSupply;TestFalse(TEXT("Attached rack slot identity is unique"),AllIds.Contains(Slot.Id));AllIds.Add(Slot.Id);}
  TestEqual(TEXT("Each machine receives four output positions"),Outputs,4);TestEqual(TEXT("Plasma receives two sheet positions; other machines receive one input"),Inputs,Machine.Kind()==1?2:1);
  const auto Expected=FFoundrylineFactoryCatalog::MachinePrograms(Machine.Id,Machine.Kind(),true);const auto Programs=M.State.Programs.FilterByPredicate([&](const auto& P){return P.Machine==Machine.Id;});TestEqual(TEXT("Instance owns its equipment's available recipes"),Programs.Num(),Expected.Num());
  const auto* Route=M.State.Routes.FindByPredicate([&](const auto& R){return R.Sources.Contains(Machine.Id)&&R.Destinations.Contains(Machine.Id);});TestNotNull(TEXT("Instance is connected to this shop's logistics route"),Route);
 }
 TestTrue(TEXT("First added plasma accepts its own batch"),M.Program(Purchased[0],EFoundrylinePart::Bracket,7,false,Error));
 TestTrue(TEXT("Second added plasma accepts a different batch"),M.Program(Purchased[1],EFoundrylinePart::MountingPlate,3,false,Error));
 const auto* First=M.State.Machines.FindByPredicate([&](const auto& X){return X.Id==Purchased[0];});const auto* Second=M.State.Machines.FindByPredicate([&](const auto& X){return X.Id==Purchased[1];});
 TestTrue(TEXT("One plasma's settings do not overwrite another"),First&&Second&&First->Making==EFoundrylinePart::Bracket&&First->BatchRemaining==7&&Second->Making==EFoundrylinePart::MountingPlate&&Second->BatchRemaining==3);
 const FString Stable=Snapshot(M);TestFalse(TEXT("Overlapping machine purchase rejected"),M.BuyPlanningEquipment(4,Choices[0].At,0,1,Error));TestEqual(TEXT("Rejected machine purchase preserves money and identity counters"),Snapshot(M),Stable);
 TestTrue(TEXT("Multiple instances and their recipes validate"),M.Valid(Error));return true;
}
#endif
