#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
#include "Grid/FoundrylineGrid.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCutShopFlowTest,"Foundryline.Factory.CutPartsStarterFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCutShopFlowTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(false);FString E;TestTrue(TEXT("Starter ledger valid"),M.Valid(E));AddInfo(E);
 TestEqual(TEXT("Only burn table"),M.State.Machines.Num(),1);TestEqual(TEXT("One forklift"),M.State.Drivers.Num(),1);TestEqual(TEXT("No assembly bays"),M.State.Zones.Num(),0);TestEqual(TEXT("Six compatible plasma recipes in legacy fixture"),M.State.Programs.Num(),6);
 TestTrue(TEXT("Forklift can reach raw rack"),!M.Path(M.State.Drivers[0].Position,M.State.Slots[0].Dock).IsEmpty());
 TestTrue(TEXT("Output is connected to finished rack"),!M.Path(M.State.Machines[0].OutputDock,M.State.Slots[1].Dock).IsEmpty());
 const float Mass=M.SteelMass();M.State.Running=true;M.State.Speed=8;
 for(int I=0;I<4000&&M.Available(EFoundrylinePart::Bracket,EFoundrylinePalletPlace::Rack,107)<10;++I)M.Step(.1f);
 TestEqual(TEXT("Ten parts cut and delivered to finished rack"),M.Available(EFoundrylinePart::Bracket,EFoundrylinePalletPlace::Rack,107),10);
 TestTrue(TEXT("Steel conserved"),FMath::Abs(M.SteelMass()-Mass)<.1);TestTrue(TEXT("Finished ledger remains valid"),M.Valid(E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCutShopLayoutTest,"Foundryline.Construction.CutPartsStarterLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCutShopLayoutTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->bCutPartsStarter=true;Site->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();int Shops=0,Rooms=0,TruckDoors=0;
 TestTrue(TEXT("Starter construction initializes"),Site->IsReady());for(const auto& O:P->GetObjects()){const auto* D=P->FindDefinition(O.DefinitionId);if(D&&D->FunctionId==TEXT("BuildingShell")){if(O.bOfficeAnnex){Rooms+=O.OfficeRooms.Num();int Doors=0;for(const auto& A:O.Openings)if(A.Kind==EFoundrylineOpeningKind::OfficeDoor)++Doors;TestTrue(TEXT("Office has shop and outside access"),Doors>=2);}else{++Shops;TestEqual(TEXT("28 metre starter frontage"),D->Footprint.Max.X,2800);TestEqual(TEXT("22 metre starter depth"),D->Footprint.Max.Y,2200);}}for(const auto& A:O.Openings)if(A.Kind==EFoundrylineOpeningKind::TruckGarage)++TruckDoors;}

 auto Paved=[&](FIntVector Point){for(const auto& O:P->GetObjects()){const auto* D=P->FindDefinition(O.DefinitionId);if(!D||(D->FunctionId!=TEXT("Road2")&&D->FunctionId!=TEXT("TruckAccess")&&D->FunctionId!=TEXT("FreightYard")))continue;const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,O.Anchor,O.QuarterTurns);if(Point.X>=B.Min.X&&Point.X<=B.Max.X&&Point.Y>=B.Min.Y&&Point.Y<=B.Max.Y)return true;}return false;};
 int Access=0;for(const auto& O:P->GetObjects())if(const auto* D=P->FindDefinition(O.DefinitionId);D&&D->FunctionId==TEXT("TruckAccess"))++Access;
 TestEqual(TEXT("Freight driveway survives placement validation"),Access,1);bool Connected=true;for(int Y=-2400;Y<=0;Y+=100)Connected&=Paved({4200,Y,0});TestTrue(TEXT("Continuous pavement from public street through freight driveway into apron"),Connected);TestTrue(TEXT("Eastbound carrier exit remains on protected paved street"),Paved({9960,-3000,0}));
 TestEqual(TEXT("One small shop"),Shops,1);TestEqual(TEXT("Two private offices"),Rooms,2);TestEqual(TEXT("One ground-level loading door"),TruckDoors,1);W->DestroyWorld(false);return true;
}
#endif
