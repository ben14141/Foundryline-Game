#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Pathing/FoundrylineFloorLaneGeometry.h"
#include "Construction/FoundrylinePlannerItems.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkliftRectangleAreasTest,"Foundryline.Construction.PlannerRevision.ForkliftRectanglesJoinAndProtectWorkspaces",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkliftRectangleAreasTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);M.State.Running=false;M.State.FloorMin={-5000,-5000};M.State.FloorMax={5000,5000};M.State.Lanes.Reset();M.State.Zones.Reset();M.State.Drivers.Reset();M.State.Operators.Reset();M.State.UnassignedEmployees.Reset();M.Solids.Reset();FString Error;
 const int32 Next=M.State.NextId;const auto Revision=M.State.Revision;
 for(const FVector Size:{FVector(359,600,0),FVector(600,359,0),FVector(100,100,0)})TestFalse(TEXT("Both rectangle dimensions allow two forklifts to pass"),M.PaintForkliftArea({0,0,20},Size+FVector(0,0,20),Error));
 TestEqual(TEXT("Invalid rectangle allocates no identity"),M.State.NextId,Next);TestEqual(TEXT("Invalid rectangle does not advance shared revision"),M.State.Revision,Revision);TestTrue(TEXT("Rejected previews leave no floor area"),M.State.Lanes.IsEmpty());
 TestTrue(TEXT("A minimum 360cm square is legal"),M.PaintForkliftArea({-1000,-1000,20},{-640,-640,20},Error));M.State.Lanes.Reset();
 TestTrue(TEXT("Drag opposite corners in reverse order"),M.PaintForkliftArea({1200,600,20},{0,0,20},Error));
 TestTrue(TEXT("Area stores exactly the two dragged corners"),M.State.Lanes.Last().Min.Equals({0,0})&&M.State.Lanes.Last().Max.Equals({1200,600}));
 TestTrue(TEXT("An adjoining rectangle connects by a shared edge"),M.PaintForkliftArea({1200,0,20},{1800,1200,20},Error));
 TestTrue(TEXT("Forklift navigation travels through the connected rectangular union"),!M.Path({200,300,20},{1500,900,20}).IsEmpty());
 const auto First=M.State.Lanes[0];
 TestTrue(TEXT("Connected edge has no painted separator"),FFoundrylineFloorLaneGeometry::VisibleLine({1200,0},{1200,600},M.State.Lanes,First.Id,{.2,0}).IsEmpty());
 TestEqual(TEXT("Outer perimeter stays painted"),FFoundrylineFloorLaneGeometry::VisibleLine({0,0},{0,600},M.State.Lanes,First.Id,{-.2,0}).Num(),1);
 TestTrue(TEXT("Overlapping forklift areas connect rather than conflict"),M.PaintForkliftArea({1000,200,20},{1600,800,20},Error));
 const int Before=M.State.Lanes.Num();M.Solids.Add(FBox2D({2200,2200},{2300,2300}));TestFalse(TEXT("Forklift area cannot cover machinery or racking"),M.PaintForkliftArea({2000,2000,20},{2600,2600,20},Error));TestEqual(TEXT("Blocked equipment keeps existing areas unchanged"),M.State.Lanes.Num(),Before);
 FFoundrylineFactoryZone Zone;Zone.Min={3000,0,20};Zone.Max={4500,1500,20};Zone.DropMin=Zone.Min;Zone.DropMax=Zone.Max;M.State.Zones.Add(Zone);TestFalse(TEXT("Forklift area cannot cover a work zone"),M.PaintForkliftArea({3200,200,20},{4000,1000,20},Error));
 return true;
}
#endif
