#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Pathing/FoundrylineFloorLaneGeometry.h"
#include "Production/FoundrylineFactorySimulation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkLaneJoinTest,"Foundryline.Factory.ConnectedLaneMarkings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkLaneJoinTest::RunTest(const FString&){
 FFoundrylineFactoryLane H;H.Id=1;H.Min={-1000,-200};H.Max={1000,200};FFoundrylineFactoryLane V;V.Id=2;V.Min={-200,-1000};V.Max={200,1000};
 auto Segments=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,200},{1000,200},{H,V},1,{0,.2});TestEqual(TEXT("Cross junction removes the internal horizontal boundary"),Segments.Num(),2);if(Segments.Num()==2){TestEqual(TEXT("First edge stops at the connected corridor"),Segments[0].Value.X,-200.);TestEqual(TEXT("Boundary resumes after the corridor"),Segments[1].Key.X,200.);}
 V.Min={0,-200};V.Max={2000,200};Segments=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,200},{1000,200},{H,V},1,{0,.2});TestEqual(TEXT("Overlapping straight roads retain their outside edge"),Segments.Num(),1);
 V.Min={-75,-1000};V.Max={75,1000};V.Kind=EFoundrylineFloorLane::Pedestrian;auto Crossings=FFoundrylineFloorLaneGeometry::Crossings({H,V});TestEqual(TEXT("Crossing appears automatically where people cross forklifts"),Crossings.Num(),1);if(Crossings.Num())TestFalse(TEXT("Stripes cross the actual direction of walking"),Crossings[0].WalkingX);
 FFoundrylineFactoryLane Explicit;Explicit.Id=3;Explicit.Kind=EFoundrylineFloorLane::Crossing;Explicit.Min={-75,-200};Explicit.Max={75,200};TestEqual(TEXT("Explicit paint does not duplicate the automatic crossing"),FFoundrylineFloorLaneGeometry::Crossings({H,V,Explicit}).Num(),1);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPersonnelJoinTest,"Foundryline.Factory.SolidPersonnelJunctions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPersonnelJoinTest::RunTest(const FString&){
 FFoundrylineFactoryLane A;A.Id=1;A.Kind=EFoundrylineFloorLane::Pedestrian;A.Min={-1000,-75};A.Max={1000,75};auto B=A;B.Id=2;B.Min={-75,-1000};B.Max={75,1000};
 auto Lines=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,75},{1000,75},{A,B},1,{0,.2},EFoundrylineFloorLane::Pedestrian);TestEqual(TEXT("Personnel cross junction opens both arms"),Lines.Num(),2);
 B.Min={-75,75};Lines=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,75},{1000,75},{A,B},1,{0,.2},EFoundrylineFloorLane::Pedestrian);TestEqual(TEXT("Touching personnel T junction opens at the meeting edge"),Lines.Num(),2);
 B.Min={0,-75};B.Max={2000,75};Lines=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,75},{1000,75},{A,B},1,{0,.2},EFoundrylineFloorLane::Pedestrian);TestEqual(TEXT("Aligned extensions retain a solid outside edge"),Lines.Num(),1);
 B.Kind=EFoundrylineFloorLane::Forklift;B.Min={-200,-1000};B.Max={200,1000};Lines=FFoundrylineFloorLaneGeometry::VisibleLine({-1000,75},{1000,75},{A,B},1,{0,.2},EFoundrylineFloorLane::Pedestrian);TestEqual(TEXT("Green boundaries stop through the forklift crossing"),Lines.Num(),2);
 auto F=B;F.Min={-1000,-200};F.Max={0,200};auto F2=F;F2.Id=3;F2.Min={0,-200};F2.Max={1000,200};A.Min={-75,-1000};A.Max={75,1000};const auto C=FFoundrylineFloorLaneGeometry::Crossings({A,F,F2});TestEqual(TEXT("A split forklift road makes one complete automatic crossing"),C.Num(),1);if(C.Num())TestTrue(TEXT("Crossing covers both road sections"),C[0].Bounds.Min.Equals({-75,-200},.01)&&C[0].Bounds.Max.Equals({75,200},.01));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPeopleYieldTest,"Foundryline.Factory.PeopleYieldToForklifts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPeopleYieldTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed();M.Solids.Reset();M.State.Machines.Reset();M.State.Zones.Reset();M.State.Jobs.Reset();M.State.Pallets.Reset();M.State.Routes.Reset();M.State.Walkers.Reset();M.State.Drivers.Reset();M.State.Lanes.Reset();M.State.FloorMin={-2000,-2000};M.State.FloorMax={2000,2000};M.State.Running=true;
 FFoundrylineFactoryLane F;F.Id=1;F.Min={-1000,-200};F.Max={1000,200};M.State.Lanes.Add(F);FFoundrylineFactoryLane P;P.Id=2;P.Kind=EFoundrylineFloorLane::Pedestrian;P.Min={-75,-1000};P.Max={75,1000};M.State.Lanes.Add(P);
 FFoundrylineFactoryDriver D;D.Id=3;D.EmployeeId=D.Id;D.DisplayName=FString::Printf(TEXT("Test Driver %d"),D.Id);D.Position={-600,0,20};D.Parking={600,0,20};D.Path={D.Position,D.Parking};M.State.Drivers.Add(D);
 FFoundrylineFactoryWalker W;W.Id=4;W.Position={0,-270,20};W.Path={W.Position,FVector(0,600,20)};M.State.Walkers.Add(W);
 M.Step(.1f);TestTrue(TEXT("Person waits outside the stripes for an approaching forklift"),M.State.Walkers[0].Waiting&&M.State.Walkers[0].Position.Y==-270);TestTrue(TEXT("Forklift keeps right of way"),M.State.Drivers[0].Position.X> -600&&!M.State.Drivers[0].Waiting);
 for(int I=0;I<140;++I)M.Step(.1f);TestTrue(TEXT("Person crosses after the forklift clears"),M.State.Walkers[0].Position.Y>200);TestTrue(TEXT("Forklift reaches its destination"),M.State.Drivers[0].Position.X>580);
 TestEqual(TEXT("Derived crossing is cached"),M.FloorCrossings().Num(),1);M.State.Lanes.RemoveAt(1);TestEqual(TEXT("Removing walkway removes its crossing"),M.FloorCrossings().Num(),0);return true;
}
#endif
