#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Construction/FoundrylineBuildSite.h"
#include "SaveSystem/FoundrylinePlacementSave.h"
#include "Engine/World.h"
#include "UI/FoundrylinePlayerController.h"

namespace {
 bool ExpandForSemis(AFoundrylineBuildSite* S) {
  return S->BeginEdit(TEXT("Resize"),{2500,-100,0}) && S->CommitEdit(TEXT("Resize"),{2500,-100,0},{3800,0,0})
   && S->BeginEdit(TEXT("Move"),{2000,-1200,0}) && S->CommitEdit(TEXT("Move"),{2000,-1200,0},{1400,-1200,0})
   && S->BeginEdit(TEXT("Resize"),{3900,3500,0}) && S->CommitEdit(TEXT("Resize"),{3900,3500,0},{5600,3600,0});
 }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineDrivingGeometryTest,"Foundryline.Driving.LayoutAndZones",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineDrivingGeometryTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->DispatchBeginPlay();
 auto* Paths=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto Map=Paths->GetMap();
 TestTrue(TEXT("Car bays generated"),Paths->GetParkingTargetCount()>0);TestTrue(TEXT("Truck bays plus loading target generated"),Paths->GetTruckTargetCount()>1);
 const auto* Parking=Map->Surfaces.FindByPredicate([](const auto& S){return S.Function==TEXT("Parking");});
 if(TestNotNull(TEXT("Parking surface"),Parking)) {
  TestFalse(TEXT("Real driveway creates a parking entrance"),Parking->Entrances.IsEmpty());
  for(const auto& B:FFoundrylineDrivingLayout::Parking(Parking->Size.X,Parking->Size.Y,Parking->Entrances))
   TestFalse(TEXT("No stalls or target bays across driveway"),Parking->Entrances.ContainsByPredicate([&](const auto& E){return E.Overlaps(B.Bounds);}));
 }
 const auto* Bay=Map->Targets.FindByPredicate([](const auto& T){return !T.Semi;});
 if(Bay) {TestTrue(TEXT("Car fits its real parking target"),FFoundrylineVehiclePath::PoseClear(*Map,FFoundrylineVehicleProfile::Car(),Bay->Pose,false,Bay->Id));const FGuid A=FGuid::NewGuid(),B=FGuid::NewGuid();TestTrue(TEXT("First vehicle takes spot"),Paths->ReserveTarget(Bay->Id,A));TestFalse(TEXT("Second vehicle cannot double park"),Paths->ReserveTarget(Bay->Id,B));Paths->ReleaseTarget(Bay->Id,B);TestFalse(TEXT("Other vehicle cannot free a space"),Paths->IsTargetAvailable(Bay->Id));Paths->ReleaseTarget(Bay->Id,A);TestTrue(TEXT("Owner can release spot"),Paths->IsTargetAvailable(Bay->Id));}
 TestTrue(TEXT("Paint spanning owned paving uses one atomic command"),Site->EditTruckRouting({-4000,-2400,0},{6000,5300,0}));
 int32 Zones=0;for(const auto& O:P->GetObjects()) Zones+=O.TruckZones.Num();TestTrue(TEXT("Truck permissions stored in records"),Zones>0);
 const auto* Snapshot=P->CreateSnapshot();int32 SavedZones=0;for(const auto& O:Snapshot->Objects) SavedZones+=O.TruckZones.Num();TestEqual(TEXT("Zones participate in property saves"),SavedZones,Zones);
 TestTrue(TEXT("Partial erase works"),Site->EditTruckRouting({1000,1000,0},{2000,2000,0},true));
 auto* PC=W->SpawnActor<AFoundrylinePlayerController>(); // Overlay settings must stay player-local; native network test checks actual client UI later.
 TestTrue(TEXT("Existing placement authority still available"),Site->IsReady());
 W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineVehiclePlannerTest,"Foundryline.Driving.KinematicRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineVehiclePlannerTest::RunTest(const FString& Parameters)
{
 FFoundrylineDrivingMap M;FFoundrylineDriveSurface Yard;Yard.Id=FGuid::NewGuid();Yard.Function=TEXT("FreightYard");Yard.Transform=FTransform::Identity;Yard.Size={7000,7000};Yard.TruckZones.Add({{0,0},Yard.Size});M.Surfaces.Add(Yard);
 const auto Car=FFoundrylineVehicleProfile::Car(),Truck=FFoundrylineVehicleProfile::Truck();
 FFoundrylineVehiclePose A;A.Position={2500,2500};auto B=A;B.Position={4500,2500};
 const auto C=FFoundrylineVehiclePath::Find(M,Car,A,B);TestTrue(TEXT("Car follows connected paved area"),C.Success);
 const auto T=FFoundrylineVehiclePath::Find(M,Truck,A,B);TestTrue(TEXT("Semi and trailer follow legal corridor"),T.Success);
 for(const auto& P:T.Poses) TestTrue(TEXT("Every returned trailer pose fits"),FFoundrylineVehiclePath::PoseClear(M,Truck,P));
 M.Surfaces[0].TruckZones.Reset();TestFalse(TEXT("Semi does not ignore truck permission zones"),FFoundrylineVehiclePath::Find(M,Truck,A,B).Success);
 TestTrue(TEXT("Car does not require a semi zone"),FFoundrylineVehiclePath::Find(M,Car,A,B).Success);
 M.Surfaces[0].TruckZones.Add({{0,0},Yard.Size});M.Surfaces[0].Islands.Add({{3300,0},{3500,7000}});
 TestFalse(TEXT("Disconnected paving cannot be crossed"),FFoundrylineVehiclePath::Find(M,Car,A,B,true,{},1500).Success);
 const auto Curved=FFoundrylineVehiclePath::Advance(A,Truck,1000,1/Truck.TurnRadius);TestTrue(TEXT("Trailer has genuine offtracking during a turn"),FMath::Abs(Curved.Yaw-Curved.TrailerYaw)>.2f);
 auto Jackknifed=A;Jackknifed.TrailerYaw=PI/2;TestFalse(TEXT("Jackknife pose rejected"),FFoundrylineVehiclePath::PoseClear(M,Truck,Jackknifed));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineStarterRoutesTest,"Foundryline.Driving.StarterRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineStarterRoutesTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->DispatchBeginPlay();TestTrue(TEXT("Property can expand for full semi deliveries"),ExpandForSemis(Site));Site->EditTruckRouting({-4000,-2400,0},{6000,5300,0});
 const auto Map=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();
 for(const auto& S:Map->Surfaces) AddInfo(FString::Printf(TEXT("Surface %s at %s size %.0f %.0f"),*S.Function.ToString(),*S.Transform.GetLocation().ToString(),S.Size.X,S.Size.Y));
 FFoundrylineVehiclePose Entry;Entry.Position={-5000,-3000};
 const auto* CarBay=Map->Targets.FindByPredicate([](const auto& T){return !T.Semi;});
 const auto* TruckBay=Map->Targets.FindByPredicate([](const auto& T){return T.Semi && !T.Loading && T.Pose.Position.Y>2500 && T.Pose.Position.Y<2700;});
 const auto* Dock=Map->Targets.FindByPredicate([](const auto& T){return T.Loading;});
 auto Check=[&](const TCHAR* Name,const auto& Profile,auto Start,auto End,FGuid Target){const auto Route=FFoundrylineVehiclePath::Find(*Map,Profile,Start,End,true,Target);AddInfo(FString::Printf(TEXT("%s %s | %.1f ms | %d expansions | %d samples"),Name,*Route.Reason,Route.Milliseconds,Route.Expanded,Route.Poses.Num()));TestTrue(Name,Route.Success);return Route.Success;};
 if(CarBay) Check(TEXT("Public road through driveway into car parking"),FFoundrylineVehicleProfile::Car(),Entry,CarBay->Pose,CarBay->Id);
 FFoundrylineVehiclePose Ready;Ready.Position={4000,2600};
 if(TruckBay) Check(TEXT("Semi reverse parks in marked freight bay"),FFoundrylineVehicleProfile::Truck(),Ready,TruckBay->Pose,TruckBay->Id);
 if(Dock) {Ready.Position={3000,Dock->Pose.Position.Y};Check(TEXT("Semi reverse approach to real loading opening"),FFoundrylineVehicleProfile::Truck(),Ready,Dock->Pose,Dock->Id);}
 if(Dock) Check(TEXT("Semi incoming from public street to loading"),FFoundrylineVehicleProfile::Truck(),Entry,Dock->Pose,Dock->Id);
 if(Dock) {FFoundrylineVehiclePose Exit;Exit.Position={-2000,-2600};Exit.Yaw=PI;Exit.TrailerYaw=PI;Check(TEXT("Semi departing loading to public street"),FFoundrylineVehicleProfile::Truck(),Dock->Pose,Exit,{});}
 W->DestroyWorld(false);return true;
}

#include "Logistics/FoundrylineLogisticsTrip.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineGuidedTripTest,"Foundryline.Driving.GuidedTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineGuidedTripTest::RunTest(const FString& Parameters) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->DispatchBeginPlay();TestTrue(TEXT("Property can expand for full semi deliveries"),ExpandForSemis(Site));Site->EditTruckRouting({-4000,-2400,0},{6000,5300,0});
 const auto M=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();FString Message;
 for(bool Semi:{true,false}) {
  FFoundrylineLogisticsTrip Trip;Trip.Semi=Semi;const auto Gates=Trip.Gates(*M,Semi);
  TestEqual(Semi?TEXT("One genuine truck street connection"):TEXT("One car driveway connection"),Gates.Num(),1);if(Gates.IsEmpty()) continue;
  TestFalse(TEXT("Cannot test without selections"),Trip.Test(*M,Trip).Success);
  TestFalse(TEXT("Cannot select grass as a gate"),Trip.Select(*M,TEXT("TripEntrance"),{30000,30000},Message));
  TestTrue(TEXT("Select physical property entrance"),Trip.Select(*M,TEXT("TripEntrance"),Gates[0].Portal,Message));
  const auto* Target=M->Targets.FindByPredicate([&](const auto& T){return Semi?T.Loading:!T.Semi;});if(!TestNotNull(TEXT("Matching destination"),Target)) continue;
  TestTrue(TEXT("Can click actual loading door or real parking marker"),Trip.Select(*M,TEXT("TripDestination"),Trip.TargetMarker(*Target),Message));
  TestTrue(TEXT("Select two-way exit at same property gate"),Trip.Select(*M,TEXT("TripExit"),Gates[0].Portal,Message));
  TestTrue(TEXT("All trip selections complete"),Trip.Ready(*M));
  const auto R=Trip.Test(*M,Trip);AddInfo(FString::Printf(TEXT("%s complete trip: %s; arrival %.1f ms / departure %.1f ms"),Semi?TEXT("Semi"):TEXT("Car"),*R.Reason,R.Inbound.Milliseconds,R.Outbound.Milliseconds));
  TestTrue(TEXT("Both arrival and departure pass"),R.Success);
  if(R.Success) {TestTrue(TEXT("Departure starts exactly where arrival ended"),R.Inbound.Poses.Last().Position.Equals(R.Outbound.Poses[0].Position,.001));TestEqual(TEXT("Continuous trailer heading at loading"),R.Inbound.Poses.Last().TrailerYaw,R.Outbound.Poses[0].TrailerYaw);}
 }
 W->DestroyWorld(false);return true;
}
#endif
