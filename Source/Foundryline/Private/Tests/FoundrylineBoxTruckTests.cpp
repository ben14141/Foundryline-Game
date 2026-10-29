#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Logistics/FoundrylineLogisticsTrip.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineCompactBoxTripTest,"Foundryline.Driving.CompactBoxTruckTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineCompactBoxTripTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->bStarterProperty=true;S->DispatchBeginPlay();
 auto* Paths=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>();auto M=Paths->GetMap();
 const auto V=FFoundrylineVehicleProfile::BoxTruck();
 TestTrue(TEXT("Rigid delivery truck has its own real clearance and no trailer"),!V.Semi && V.IsFreight() && V.Front+V.Rear==800 && V.TurnRadius==700);
 const auto* First=M->Targets.FindByPredicate([](const auto& T){return T.Loading;});
 if(First) TestFalse(TEXT("Box trucks still require permitted truck zones"),FFoundrylineVehiclePath::PoseClear(*M,V,FFoundrylineVehiclePath::TargetPose(*First,V),true,First->Id));
 S->EditTruckRouting({-4000,-2400,0},{6000,5300,0});M=Paths->GetMap();const auto Gates=FFoundrylineLogisticsTrip::Gates(*M,true,true);
 TestEqual(TEXT("Starter box truck uses the genuine compact entrance"),Gates.Num(),1);
 int32 Checked=0;
 for(const auto& T:M->Targets) if(T.Loading && !Gates.IsEmpty()) {
  FFoundrylineLogisticsTrip Trip;Trip.SmallTruck=true;Trip.Semi=true;Trip.HasEntry=Trip.HasExit=true;Trip.Entrance=Trip.Departure=Gates[0];Trip.TargetId=T.Id;
  TestTrue(TEXT("Box truck fits the actual loading door"),T.SmallTruckClear);
  const auto R=Trip.Test(*M,Trip);
  AddInfo(FString::Printf(TEXT("Compact door %d: %s / %.0f %.0f ms"),++Checked,*R.Reason,R.Inbound.Milliseconds,R.Outbound.Milliseconds));
  TestTrue(TEXT("Box truck can arrive, load and depart the starter site"),R.Success);
 }
 TestEqual(TEXT("Both starter loading doors were tested"),Checked,2);W->DestroyWorld(false);return true;
}
#endif
