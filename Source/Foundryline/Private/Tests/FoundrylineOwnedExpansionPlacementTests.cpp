#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFabStationOwnedExpansionTest,"Foundryline.Construction.PlannerRevision.FabStationUsesOwnedExpansionFloor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFabStationOwnedExpansionTest::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
 FFoundrylineFactorySimulation Model;Model.SeedCutPartsShop(true);Model.State.Running=false;Model.State.Commerce.CashCents=10000000;
 Model.Solids.Reset();Model.State.Lanes.Reset();Model.State.Operators.Reset();Model.State.Walkers.Reset();Model.State.UnassignedEmployees.Reset();Model.State.Pallets.Reset();Model.State.Jobs.Reset();
 const FVector At(1600,-400,20);FString Error;
 auto Lane=[&](EFoundrylineFloorLane Kind,FVector2D A,FVector2D B){FFoundrylineFactoryLane L;L.Id=Model.State.NextId++;L.Kind=Kind;L.Min=A+FVector2D(At);L.Max=B+FVector2D(At);Model.State.Lanes.Add(L);};
 Lane(EFoundrylineFloorLane::Forklift,{-800,400},{800,760});Lane(EFoundrylineFloorLane::Forklift,{-700,-100},{-340,760});Lane(EFoundrylineFloorLane::Forklift,{340,-100},{700,760});
 Lane(EFoundrylineFloorLane::Pedestrian,{-1200,-320},{250,-198.12});Lane(EFoundrylineFloorLane::Pedestrian,{-1200,-320},{-1090,320});Lane(EFoundrylineFloorLane::Pedestrian,{-1200,198.12},{250,320});
 auto& Driver=Model.State.Drivers[0];Driver.Position=Driver.Parking=At+FVector(-520,600,0);Driver.Job=0;Driver.Path.Reset();
 FFoundrylineStandbyDriver Welder;Welder.Id=Model.State.NextId++;Welder.DisplayName=TEXT("Expansion Welder");Welder.Home=Welder.Position=At+FVector(-1145,-260,0);Model.State.UnassignedEmployees.Add(Welder);
 const auto Required=FFoundrylinePlannerItems::StationWorkspaceBounds(At,0);
 TestTrue(TEXT("Real owned second building covers station and four access strips"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Model.State,World,FTransform::Identity,Required,Error));
 TestTrue(TEXT("Owned floor grows beyond obsolete starter X370 wall"),Model.State.FloorMax.X>370);
 const auto BeforeCash=Model.State.Commerce.CashCents;
 const bool Bought=Model.BuyPlanningEquipment(0,At,0,1,Error);TestTrue(TEXT("Fabrication station purchases in an owned expansion: ")+Error,Bought);
 if(Bought){TestTrue(TEXT("New station's expanded-floor record remains valid: ")+Error,Model.WeldingStationsValid(Error));TestTrue(TEXT("Purchase charges the business once"),Model.State.Commerce.CashCents<BeforeCash);}
 auto Rejected=Model.State;const auto BeforeMin=Rejected.FloorMin,BeforeMax=Rejected.FloorMax;
 TestFalse(TEXT("Clear ground between buildings cannot masquerade as interior floor"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Rejected,World,FTransform::Identity,FFoundrylinePlannerItems::StationWorkspaceBounds({3300,1500,20},0),Error));
 TestTrue(TEXT("Rejected coverage does not grow bounds"),Rejected.FloorMin==BeforeMin&&Rejected.FloorMax==BeforeMax);
 auto Foreign=Model.State;Foreign.CompanyId=FGuid::NewGuid();TestFalse(TEXT("Another company's building cannot receive this workspace"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Foreign,World,FTransform::Identity,Required,Error));
 auto Overlap=Model;TestFalse(TEXT("Installed station still prevents duplicate overlap"),Overlap.BuyPlanningEquipment(0,At,0,1,Error));
 World->DestroyWorld(false);return true;
}
#endif
