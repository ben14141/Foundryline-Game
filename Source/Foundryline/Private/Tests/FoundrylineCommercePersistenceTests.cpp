#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommerceWorldRestore,"Foundryline.Commerce.WorldSaveAccountAndUpgradeRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommerceWorldRestore::RunTest(const FString&){
 const FString Base=TEXT("FoundrylineAutomation_CommerceBase"),Upgrade=TEXT("FoundrylineAutomation_CommerceUpgrade"),Broken=TEXT("FoundrylineAutomation_CommerceInvalid");
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);auto* R=W->SpawnActor<AFoundrylineBusinessRegistry>();auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->bCutPartsStarter=true;Site->DispatchBeginPlay();auto* P=W->SpawnActor<AFoundrylineFactoryPlant>();P->bCutPartsStarter=true;P->DispatchBeginPlay();FString E;
 auto Finish=[&](){for(const auto& Slot:{Base,Upgrade,Broken})UGameplayStatics::DeleteGameInSlot(Slot,0);W->DestroyWorld(false);};
 const int BaseNavigationCount=P->Model.Solids.Num();
 if(!TestTrue(TEXT("Save base to isolated test slot"),P->SaveFactory(E,Base))){AddError(E);Finish();return false;}
 auto* S=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromSlot(Base,0));if(!TestNotNull(TEXT("Read isolated save"),S)){Finish();return false;}
 auto* Property=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(S->Property));if(!TestNotNull(TEXT("Read property"),Property)){Finish();return false;}
 Property->SchemaVersion=999;UGameplayStatics::SaveGameToMemory(Property,S->Property);S->Factory.Commerce.CashCents=123456;UGameplayStatics::SaveGameToSlot(S,Broken,0);
 auto* Placement=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto Revision=Placement->GetRevision();const auto Cash=R->GetLedger().Company(P->BusinessCompanyId)->BalanceCents;const auto Digest=P->GetLedgerDigest();
 TestFalse(TEXT("Invalid property rejects whole load"),P->LoadFactory(E,Broken));TestEqual(TEXT("Cash rolled back after property rejection"),R->GetLedger().Company(P->BusinessCompanyId)->BalanceCents,Cash);TestEqual(TEXT("Property revision untouched"),Placement->GetRevision(),Revision);TestEqual(TEXT("Factory remains unchanged"),P->GetLedgerDigest(),Digest);
 TestTrue(TEXT("Grant test-only upgrade capital"),R->CommitPlantCash(P->BusinessCompanyId,P->BusinessPlantId,Cash,1000000));P->Model.State.Commerce.CashCents=1000000;FFoundrylineCommerceCommand A;A.Action=EFoundrylineCommerceAction::BuyBrake;TestTrue(TEXT("Install brake in fixture"),P->Model.CommerceCommand(A,E));P->EnsureCommerceEquipment();const int UpgradeNavigationCount=P->Model.Solids.Num();TestTrue(TEXT("Upgrade adds real machine and rack blockers"),UpgradeNavigationCount>BaseNavigationCount);TestTrue(TEXT("Save owned upgrade"),P->SaveFactory(E,Upgrade));
 TestTrue(TEXT("Restore earlier base"),P->LoadFactory(E,Base));TestFalse(TEXT("Upgrade removed from ledger"),P->Model.State.Commerce.BrakeOwned);TestEqual(TEXT("Base navigation restored"),P->Model.Solids.Num(),BaseNavigationCount);int Brakes=0;for(TActorIterator<AFoundrylinePressBrakeDisplay> It(W);It;++It)if(IsValid(*It)&&!It->IsActorBeingDestroyed())++Brakes;TestEqual(TEXT("Upgrade actor removed"),Brakes,0);
 TestTrue(TEXT("Restore purchased upgrade"),P->LoadFactory(E,Upgrade));TestTrue(TEXT("Upgrade ledger restored"),P->Model.State.Commerce.BrakeOwned);TestEqual(TEXT("Upgrade navigation restored"),P->Model.Solids.Num(),UpgradeNavigationCount);TestEqual(TEXT("Upgrade cash restored with factory"),R->GetLedger().Company(P->BusinessCompanyId)->BalanceCents,int64(550000));TestTrue(TEXT("Restored ledger valid"),P->Model.Valid(E));if(!E.IsEmpty())AddInfo(E);Finish();return true;
}
#endif
