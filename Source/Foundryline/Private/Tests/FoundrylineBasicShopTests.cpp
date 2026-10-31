#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicShopBackendTest,"Foundryline.Commerce.BasicShop.EmptyBusinessCanPurchase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBasicShopBackendTest::RunTest(const FString&){FFoundrylineFactorySimulation M;M.State.bCutPartsStarter=true;M.SeedCommerce();M.State.Lanes.Reset();M.State.FloorMin={-2370,30};M.State.FloorMax={-30,1770};FString E;
 const bool InitialValid=M.Valid(E);if(!InitialValid)AddError(E);TestTrue(TEXT("Empty owned shop backend is valid"),InitialValid);auto Assembly=M;TestTrue(TEXT("Empty shop allows a workstation before lanes or forklifts"),Assembly.BuyPlanningEquipment(0,{-1300,900,20},0,1,E));TestTrue(TEXT("Machine-less workstation state saves and validates"),Assembly.Valid(E));
 TestTrue(TEXT("Racks can be purchased without a preinstalled machine"),M.BuyPlanningEquipment(1,{-1700,700,20},0,1,E));TestTrue(TEXT("Purchase keeps owned shop state valid"),M.Valid(E));TestEqual(TEXT("No machines are silently added"),M.State.Machines.Num(),0);TestEqual(TEXT("One rack purchased"),M.State.StorageRacks.Num(),1);
 TestTrue(TEXT("Office buys the first forklift before choosing its parking position"),M.BuyStoredMachine(4,E));
 TestTrue(TEXT("First forklift creates its shop route without manual setup"),M.BuyPlanningEquipment(3,{-800,1000,20},0,1,E));TestEqual(TEXT("One default route"),M.State.Routes.Num(),1);TestTrue(TEXT("Forklift uses the generated route"),M.State.Drivers.Num()==1&&M.State.Drivers[0].Route==M.State.Routes[0].Id);
 M.State.Programs.Add(FFoundrylineFactoryCatalog::DefaultPrograms()[0]);TestFalse(TEXT("An incomplete machine recipe catalog is rejected"),M.Valid(E));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstShopDoorsTest,"Foundryline.Commerce.FirstShop.LayoutAndDoorClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirstShopDoorsTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false,TEXT("L_FirstShopStarter"));
 auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->bStarterProperty=true;S->bCutPartsStarter=true;S->bFirstShopStarter=true;S->DispatchBeginPlay();
 auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();int Loading=0,Garage=0;
 for(const auto& O:P->GetObjects())for(const auto& V:O.Openings){if(V.Kind==EFoundrylineOpeningKind::LoadingDock){++Loading;TestEqual(TEXT("Starter shipping bay is small-truck tier"),int(V.LoadingTier),1);}if(V.Kind==EFoundrylineOpeningKind::Garage)++Garage;}
 TestEqual(TEXT("One shipping bay"),Loading,1);TestEqual(TEXT("Separate forklift garage"),Garage,1);
 const auto* C=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();const auto Shape=FCollisionShape::MakeCapsule(C->GetUnscaledCapsuleRadius(),C->GetUnscaledCapsuleHalfHeight());
 int Doors=0;for(TActorIterator<AActor> It(W);It;++It){TArray<UFoundrylinePersonnelDoorComponent*> Leaves;It->GetComponents(Leaves);for(auto* D:Leaves){++Doors;TestTrue(TEXT("Starter personnel door begins open"),D->IsOpen());}}
 TestTrue(TEXT("Office outside, shop connection and shop outside doors exist"),Doors>=3);
 for(const auto& O:P->GetObjects())if(const auto* Def=P->FindDefinition(O.DefinitionId))for(const auto& V:O.Openings){
  if(V.IsGlazing())continue;
  // Use the actual snapped opening centre rather than the cursor used to author it.
  const double U=V.OffsetCm+V.WidthCm()*.5;const double Width=Def->Footprint.Max.X,Depth=Def->Footprint.Max.Y;
  const FVector Local=V.Edge==0?FVector(U,0,112):V.Edge==1?FVector(Width,U,112):V.Edge==2?FVector(U,Depth,112):FVector(0,U,112);
  const FVector Normal=V.Edge==0?FVector(0,-1,0):V.Edge==1?FVector(1,0,0):V.Edge==2?FVector(0,1,0):FVector(-1,0,0);
  const FRotator Rotation(0,O.QuarterTurns*90,0);const FVector At=FVector(O.Anchor)+Rotation.RotateVector(Local),Across=Rotation.RotateVector(Normal)*130;
  FHitResult Hit;FCollisionQueryParams Q;const bool Blocked=W->SweepSingleByChannel(Hit,At-Across,At+Across,FQuat::Identity,ECC_Pawn,Shape,Q);
  TestFalse(*FString::Printf(TEXT("Door %s at %s clears walking capsule"),*V.OpeningId.ToString(),*At.ToString()),Blocked);
  if(Blocked)AddInfo(FString::Printf(TEXT("Door sweep hit %s at %s"),*GetPathNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString()));
 }
 W->DestroyWorld(false);FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString E;const bool Valid=M.Valid(E);if(!Valid)AddError(E);TestTrue(TEXT("Plasma starter operations validate"),Valid);TestEqual(TEXT("Exactly one production machine"),M.State.Machines.Num(),1);TestTrue(TEXT("No fabrication bench or assembly work zones"),M.State.WeldingStations.IsEmpty()&&M.State.Zones.IsEmpty());return true;
}
#endif
