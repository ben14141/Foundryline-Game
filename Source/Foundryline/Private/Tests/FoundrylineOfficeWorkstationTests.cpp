#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeDeskPurchase,"Foundryline.Commerce.OfficeDesks.PurchaseMoveSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeDeskPurchase::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.State.bCutPartsStarter=true;M.SeedCommerce();M.State.FloorMin={-2000,-2000};M.State.FloorMax={2000,2000};M.State.Running=false;FString E;const int64 Before=M.State.Commerce.CashCents;
 if(!TestTrue(TEXT("Buy first desk"),M.BuyPlanningEquipment(7,{-800,600,20},0,1,E)))return false;
 TestTrue(TEXT("Buy second desk rotated"),M.BuyPlanningEquipment(7,{-300,600,20},1,1,E));
 TestEqual(TEXT("Each complete workstation costs $500"),M.State.Commerce.CashCents,Before-2*FFoundrylineEconomyTuning::OfficeWorkstationCents);
 const int64 Paid=M.State.Commerce.CashCents;const int Id=M.State.OfficeDesks[0].Id;
 TestFalse(TEXT("Overlapping purchase rejected"),M.BuyPlanningEquipment(7,{-800,600,20},0,1,E));TestEqual(TEXT("Rejected purchase has no charge"),M.State.Commerce.CashCents,Paid);
 TestTrue(TEXT("Move and rotate purchased desk"),M.EditPlanningItem(int(EFoundrylinePlannerItem::OfficeDesk),Id,{-800,1100,20},false,E,3));TestEqual(TEXT("Moving has no second charge"),M.State.Commerce.CashCents,Paid);
 auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TArray<uint8> Bytes;TestTrue(TEXT("Save desk records"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Restored=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Load desk records"),Restored))return false;
 TestEqual(TEXT("Both workstations persist"),Restored->Factory.OfficeDesks.Num(),2);TestEqual(TEXT("Saved rotated placement"),Restored->Factory.OfficeDesks[0].QuarterTurns,3);
 auto Poor=M;Poor.State.Commerce.CashCents=0;TestFalse(TEXT("Cannot buy without money"),Poor.BuyPlanningEquipment(7,{500,600,20},0,1,E));TestEqual(TEXT("Failed buy adds no record"),Poor.State.OfficeDesks.Num(),2);
 TestTrue(TEXT("Remove selected workstation"),M.EditPlanningItem(int(EFoundrylinePlannerItem::OfficeDesk),Id,{-800,1100,20},true,E));TestEqual(TEXT("Removal leaves other desk"),M.State.OfficeDesks.Num(),1);TestTrue(TEXT("Final state valid"),M.Valid(E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeDeskActorLifecycle,"Foundryline.Commerce.OfficeDesks.TerminalLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeDeskActorLifecycle::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);
 auto* P=W->SpawnActor<AFoundrylineFactoryPlant>();P->BusinessCompanyId=FGuid::NewGuid();P->BusinessPlantId=FGuid::NewGuid();P->Model.State.bCutPartsStarter=true;P->Model.SeedCommerce();P->Model.State.CompanyId=P->BusinessCompanyId;P->Model.State.PlantId=P->BusinessPlantId;P->Model.State.FloorMin={-2000,-2000};P->Model.State.FloorMax={2000,2000};FString E;
 TestTrue(TEXT("Purchase desk for this business"),P->Model.BuyPlanningEquipment(7,{500,600,20},1,1,E));P->EnsureCommerceEquipment();TestEqual(TEXT("One replicated terminal spawned"),P->OfficeDeskActors.Num(),1);
 if(P->OfficeDeskActors.Num()){auto* D=P->OfficeDeskActors.CreateIterator().Value().Get();D->DispatchBeginPlay();TestTrue(TEXT("BeginPlay preserves planned desk rotation"),FMath::IsNearlyEqual(D->GetActorRotation().Yaw,90.,.01));TestEqual(TEXT("Terminal has correct company"),D->CompanyId,P->BusinessCompanyId);TestEqual(TEXT("Terminal has correct plant"),D->PlantId,P->BusinessPlantId);TestTrue(TEXT("Desk replicates and moves over network"),D->GetIsReplicated()&&D->IsReplicatingMovement());
 TArray<UStaticMeshComponent*> Meshes;D->GetComponents(Meshes);bool Computer=false,Chair=false;for(auto* C:Meshes)if(auto* Mesh=C->GetStaticMesh().Get()){Computer|=Mesh->GetName()==TEXT("SM_OfficeWorkstation");Chair|=Mesh->GetName()==TEXT("SM_Chairs_01");if(Mesh->GetName()==TEXT("SM_Chairs_01"))TestTrue(TEXT("Chair stays on the seating side after rotation"),C->GetComponentLocation().Equals(D->GetActorLocation()+D->GetActorQuat().RotateVector(FVector(0,85,0)),.1));}TestTrue(TEXT("Actual computer model present"),Computer);TestTrue(TEXT("Actual chair model present"),Chair);
 P->Model.State.OfficeDesks[0].Center={900,1100,20};P->EnsureCommerceEquipment();TestTrue(TEXT("Move updates existing actor"),D->GetActorLocation().Equals({900,1100,20},.01));P->Model.State.OfficeDesks.Reset();P->EnsureCommerceEquipment();TestTrue(TEXT("Removal destroys terminal"),D->IsActorBeingDestroyed());TestEqual(TEXT("No orphan desk records"),P->OfficeDeskActors.Num(),0);
 }
 W->DestroyWorld(false);return true;
}
#endif
