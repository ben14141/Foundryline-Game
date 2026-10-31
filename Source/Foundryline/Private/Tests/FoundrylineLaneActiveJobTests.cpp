#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "JsonObjectConverter.h"

namespace LaneActiveJobTest {
using Place=EFoundrylinePalletPlace;
struct Fixture {
 FFoundrylineFactorySimulation M;int Left=0,Bridge=0,Right=0,Source=0,Target=0,Pallet=0,Job=0;
 Fixture(){
  M.SeedCutPartsShop(false);auto& S=M.State;S.Running=false;S.FloorMin={-12000,-12000};S.FloorMax={12000,12000};S.Lanes.Reset();
  for(auto& Machine:S.Machines)Machine.Enabled=false;
  auto Lane=[&](FVector2D Min,FVector2D Max){FFoundrylineFactoryLane L;L.Id=S.NextId++;L.Kind=EFoundrylineFloorLane::Forklift;L.Min=Min;L.Max=Max;S.Lanes.Add(L);return L.Id;};
  Left=Lane({3000,4700},{5000,5300});Bridge=Lane({4800,4700},{6800,5300});Right=Lane({6600,4700},{8500,5300});
  auto Slot=[&](double X){FFoundrylineFactorySlot V;V.Id=S.NextId++;V.Part=EFoundrylinePart::Bracket;V.Position={X,4500,20};V.Dock={X,5000,20};S.Slots.Add(V);S.Routes[0].Sources.Add(V.Id);S.Routes[0].Destinations.Add(V.Id);return V.Id;};
  Source=Slot(4000);Target=Slot(8000);
  FFoundrylineFactoryPallet P;P.Id=S.NextId++;P.Part=EFoundrylinePart::Bracket;P.Quantity=5;P.UnitLb=5;P.Place=Place::Rack;P.Owner=Source;P.Position={4000,4500,20};Pallet=P.Id;S.Pallets.Add(P);
  S.Drivers[0].Position=S.Drivers[0].Parking={3500,5000,20};S.Drivers[0].Path.Reset();S.Drivers[0].Distance=0;
  M.Solids=FFoundrylineWorkshopLayout::PlantSolids(S);FString Error;
  if(M.RequestPlanningPalletMove(Pallet,Target,Error))Job=S.Jobs.Last().Id;
 }
 const FFoundrylineFactoryJob* Current()const{return M.State.Jobs.FindByPredicate([&](const auto& J){return J.Id==Job;});}
 bool Phase(int Wanted){M.State.Running=true;for(int I=0;I<4000;++I){const auto* J=Current();if(J&&J->Phase==Wanted){M.State.Running=false;return true;}M.Step(.1f);}M.State.Running=false;return false;}
 bool Widen(int Lane,FVector Center,FString& Error){return M.EditPlanningItem(int(EFoundrylinePlannerItem::Lane),Lane,Center,false,Error,0,{2000,800,0});}
};
FString Snapshot(const FFoundrylineFactorySimulation& M){FString S;FJsonObjectConverter::UStructToJsonObjectString(M.State,S);return S;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanePausedActiveJob,"Foundryline.Construction.PlannerRevision.PausedLaneActiveDelivery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLanePausedActiveJob::RunTest(const FString&){
 using namespace LaneActiveJobTest;LaneActiveJobTest::Fixture F;FString Error;
 if(!TestTrue(TEXT("Public pallet move creates a real reserved job"),F.Job>0&&F.M.Valid(Error))){AddError(Error);return false;}
 const float Steel=F.M.SteelMass();
 if(!TestTrue(TEXT("Driver begins real pickup travel"),F.Phase(1)))return false;
 const auto* J=F.Current();const auto Pickup=J->Pickup;const int Driver=J->Driver;
 if(!TestTrue(TEXT("Paused lane containing active forklift can widen safely: ")+Error,F.Widen(F.Left,{4000,5000,20},Error)))return false;
 J=F.Current();const auto* D=F.M.State.Drivers.FindByPredicate([&](const auto& V){return V.Id==Driver;});
 TestTrue(TEXT("Pickup travel keeps job and reservation, path starts at current vehicle"),J&&J->Phase==1&&D&&D->Job==F.Job&&D->Path.Num()>0&&D->Path[0].Equals(D->Position,.01)&&D->Path.Last().Equals(Pickup,.01)&&D->Distance==0);
 const auto* Stock=F.M.State.Pallets.FindByPredicate([&](const auto& P){return P.Id==F.Pallet;});TestTrue(TEXT("Five reserved parts are not cancelled or duplicated"),Stock&&Stock->Reserved==5&&Stock->Quantity==5);
 if(!TestTrue(TEXT("Pickup handling phase reached"),F.Phase(2)))return false;
 const float Handling=F.Current()->Handling;
 TestTrue(TEXT("Lane edit during handling is allowed"),F.Widen(F.Left,{4000,5000,20},Error));
 TestTrue(TEXT("Pickup handling phase and timer preserved"),F.Current()&&F.Current()->Phase==2&&F.Current()->Handling==Handling);
 if(!TestTrue(TEXT("Real pallet is loaded for delivery"),F.Phase(3)))return false;
 const int Carry=F.Current()->CarryPallet;
 TestTrue(TEXT("Loaded delivery can replan on widened lane"),F.Widen(F.Right,{7550,5000,20},Error));
 J=F.Current();D=F.M.State.Drivers.FindByPredicate([&](const auto& V){return V.Id==Driver;});
 TestTrue(TEXT("Loaded leg targets drop and retains same carried pallet"),J&&J->Phase==3&&J->CarryPallet==Carry&&D&&!D->Path.IsEmpty()&&D->Path.Last().Equals(J->Drop,.01));
 const FString Before=Snapshot(F.M);
 TestFalse(TEXT("Removing required bridge rejects a loaded delivery"),F.M.EditPlanningItem(int(EFoundrylinePlannerItem::Lane),F.Bridge,{5800,5000,20},true,Error));
 TestTrue(TEXT("Blocked edit explains forklift connectivity"),Error.Contains(TEXT("route"))||Error.Contains(TEXT("connect"))||Error.Contains(TEXT("strand")));
 TestEqual(TEXT("Rejected disconnection leaves all state, load, paths and revision unchanged"),Snapshot(F.M),Before);
 if(!TestTrue(TEXT("Unloading phase reached after resume"),F.Phase(4)))return false;
 const float Unload=F.Current()->Handling;
 TestTrue(TEXT("Safe lane edit during unloading succeeds"),F.Widen(F.Right,{7550,5000,20},Error));
 TestTrue(TEXT("Unload phase and timer preserved"),F.Current()&&F.Current()->Phase==4&&F.Current()->Handling==Unload);
 F.M.State.Running=true;for(int I=0;I<4000&&F.Current();++I)F.M.Step(.1f);
 TestTrue(TEXT("Original real delivery completes"),!F.Current());TestEqual(TEXT("Exact five parts arrive once"),F.M.Available(EFoundrylinePart::Bracket,Place::Rack,F.Target),5);
 TestTrue(TEXT("Lane edits conserve material"),FMath::IsNearlyEqual(F.M.SteelMass(),Steel,.01f));TestTrue(TEXT("Final state valid: ")+Error,F.M.Valid(Error));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanePausedPendingJob,"Foundryline.Construction.PlannerRevision.PausedLaneQueuedReservation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLanePausedPendingJob::RunTest(const FString&){
 using namespace LaneActiveJobTest;LaneActiveJobTest::Fixture F;FString Error;if(!TestTrue(TEXT("Queued fixture valid"),F.Job>0&&F.M.Valid(Error)))return false;
 TestTrue(TEXT("Queued unassigned reservation permits a connected edit"),F.Widen(F.Bridge,{5800,5000,20},Error));
 TestTrue(TEXT("Pending job remains queued without assigning or cancelling it"),F.Current()&&F.Current()->Phase==0&&!F.Current()->Driver);
 const FString Before=Snapshot(F.M);TestFalse(TEXT("Deleting required lane with queued job rejected"),F.M.EditPlanningItem(int(EFoundrylinePlannerItem::Lane),F.Bridge,{5800,5000,20},true,Error));TestEqual(TEXT("Queued reservation disconnection rejection is atomic"),Snapshot(F.M),Before);return true;
}
#endif
