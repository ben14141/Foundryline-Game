#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineFloorPaintingTest,"Foundryline.Production.AccurateFloorPainting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineFloorPaintingTest::RunTest(const FString&){
 FVector P;TestTrue(TEXT("Cursor intersects the real raised slab"),FFoundrylineFloorPlacement::FloorPoint({100,-200,1000},{.2,.6,-1},P));TestTrue(TEXT("No offset from picking the ground below the slab"),P.Equals({296,388,20},.0001));TestFalse(TEXT("Upward rays cannot paint behind the camera"),FFoundrylineFloorPlacement::FloorPoint({0,0,100},{0,0,1},P));
 const FBox2D Floor({-10000,-10000},{10000,10000});FFoundrylineFactorySimulation M;M.State.FloorMin=Floor.Min;M.State.FloorMax=Floor.Max;FFoundrylineFactoryRoute R;R.Id=1;M.State.Routes.Add(R);FString E;
 TestTrue(TEXT("First work zone placed"),M.AddPresetZone({0,0,20},2,0,1,1,E));const auto Size=FFoundrylineWorkZoneLayout::TotalSize(2);bool Snapped=false;const TArray<FBox2D> Targets={FFoundrylineWorkZoneLayout::Bounds(M.State.Zones[0])};
 auto Center=FFoundrylineFloorPlacement::Snap({Size.X+8,3},Size/2,30.48,Floor,Targets,0,true,Snapped);TestTrue(TEXT("Nearby zone edges align exactly, even between grid lines"),Snapped&&Center.Equals({Size.X,0},.001));
 TestTrue(TEXT("Authoritative placement permits touching edges without overlapping"),M.AddPresetZone(FVector(Center,20),2,0,1,1,E));
 FFoundrylineFactoryZone Bad;FFoundrylineWorkZoneLayout::Create({Size.X-.2,0,20},2,0,1,Bad);TestFalse(TEXT("A real overlap is still rejected"),M.CanPlaceZone(Bad,E));
 Center=FFoundrylineFloorPlacement::Snap({Size.X+15.24+4,2},Size/2,30.48,Floor,Targets,15.24,true,Snapped);TestTrue(TEXT("Six-inch spacing is exact"),FMath::Abs(Center.X-Size.X-15.24)<.001);
 Center=FFoundrylineFloorPlacement::Snap({Floor.Max.X-Size.X/2+6,3000},Size/2,30.48,Floor,{},0,true,Snapped);TestTrue(TEXT("Zone edge snaps accurately inside the floor boundary"),FMath::Abs(Center.X+Size.X/2-Floor.Max.X)<.001);
 const TArray<FBox2D> LaneTargets={FBox2D({123.456,1000},{500,1500})};const auto End=FFoundrylineFloorPlacement::Snap({130,1200},{0,180},30.48,Floor,LaneTargets,0,true,Snapped);TestTrue(TEXT("Lane endpoint aligns with a nearby non-grid edge"),FMath::Abs(End.X-123.456)<.001);
 FFoundrylineFactorySimulation L;L.State.FloorMin=Floor.Min;L.State.FloorMax=Floor.Max;
 TestTrue(TEXT("Feet and inch lane placement accepted"),L.PaintLane(EFoundrylineFloorLane::Forklift,{6.35,38.1,20},{600.71,38.1,20},365.76,0,E));const auto& Lane=L.State.Lanes.Last();TestTrue(TEXT("Server preserves preview endpoints instead of rounding to 25 cm"),FMath::Abs(Lane.Min.X-6.35)<.001&&FMath::Abs(Lane.Max.X-600.71)<.001);TestTrue(TEXT("Painted width is exactly twelve feet"),FMath::Abs(Lane.Max.Y-Lane.Min.Y-365.76)<.001);
 TestTrue(TEXT("Crossing stamp accepts exact center"),L.PaintLane(EFoundrylineFloorLane::Crossing,{123.456,789.123,20},{0,0,20},365.76,0,E));TestTrue(TEXT("Crossing stamp preview and final footprint share a center"),L.State.Lanes.Last().Min.Equals({23.456,539.123},.001));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkZoneLaneExclusionTest,"Foundryline.Production.WorkZonesExcludeLanes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorkZoneLaneExclusionTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed(true);FString E;TestTrue(TEXT("Starter layout passes save validation"),M.Valid(E));
 for(const auto& Z:M.State.Zones){for(const auto& L:M.State.Lanes)TestFalse(TEXT("No starter lane overlaps either tanker zone"),FFoundrylineFloorPlacement::Overlap(FFoundrylineWorkZoneLayout::Bounds(Z),FBox2D(L.Min,L.Max)));
  TestTrue(TEXT("Employees reach their work point from the outside walkway"),M.Path({400,FFoundrylineWorkZoneLayout::WorkPoint(Z).Y,20},FFoundrylineWorkZoneLayout::WorkPoint(Z),true).Num()>1);
 }
 FFoundrylineFactorySimulation X;X.State.FloorMin={-10000,-10000};X.State.FloorMax={10000,10000};FFoundrylineFactoryRoute R;R.Id=1;X.State.Routes.Add(R);
 TestTrue(TEXT("Place personnel lane first"),X.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1000,0,20},{1000,0,20},121.92,0,E));TestFalse(TEXT("Work zone cannot be placed over a personnel lane"),X.AddPresetZone({0,0,20},2,0,1,1,E));X.State.Lanes.Reset();
 TestTrue(TEXT("Place work zone first"),X.AddPresetZone({0,0,20},2,0,1,1,E));TestFalse(TEXT("Forklift lane cannot be painted through a work zone"),X.PaintLane(EFoundrylineFloorLane::Forklift,{-1000,0,20},{1000,0,20},304.8,0,E));TestFalse(TEXT("Personnel lane cannot be painted through a work zone"),X.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1000,0,20},{1000,0,20},121.92,0,E));TestFalse(TEXT("Crossing stamps cannot overlap work zones"),X.PaintLane(EFoundrylineFloorLane::Crossing,{0,0,20},{0,0,20},121.92,0,E));
 const double Edge=FFoundrylineWorkZoneLayout::TotalSize(2).X/2;TestTrue(TEXT("Lanes may connect exactly at the zone edge"),X.PaintLane(EFoundrylineFloorLane::Pedestrian,{Edge,0,20},{Edge+500,0,20},121.92,0,E));
 return true;
}
#endif
