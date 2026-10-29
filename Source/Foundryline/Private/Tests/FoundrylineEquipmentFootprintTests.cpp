#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineEquipmentFootprintComponent.h"
#include "Construction/FoundrylinePlaceableDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineEquipmentAreaTest,"Foundryline.Production.EquipmentPlacementAreas",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineEquipmentAreaTest::RunTest(const FString&){
 for(const TCHAR* Name:{TEXT("PressBrake"),TEXT("PlasmaTable"),TEXT("SheetRoller")}){
  auto* D=LoadObject<UFoundrylinePlaceableDefinition>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Production/Equipment/Placement/DA_%s.DA_%s"),Name,Name));
  TestTrue(TEXT("Equipment has an authored placement definition"),D!=nullptr);if(!D)continue;
  TestTrue(TEXT("Working area encloses machine footprint"),D->IsDefinitionValid());
  TestEqual(TEXT("Precise placement uses 25 cm grid"),FFoundrylineGrid::SnapStep(D->Snap),25);
  TestTrue(TEXT("Working area includes space beyond the machine"),D->WorkingClearance.Min.X<D->Footprint.Min.X&&D->WorkingClearance.Max.X>D->Footprint.Max.X);
  TestTrue(TEXT("Full machine preview mesh exists"),D->Mesh.LoadSynchronous()!=nullptr);
 }
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AActor>();
 auto* Area=NewObject<UFoundrylineEquipmentFootprintComponent>(A);A->AddInstanceComponent(Area);Area->RegisterComponent();A->DispatchBeginPlay();
 TestTrue(TEXT("Component loads its placement definition"),Area->GetPlacementDefinition()!=nullptr);
 TArray<UInstancedStaticMeshComponent*> Lines;A->GetComponents(Lines);TestEqual(TEXT("Grid and clearance boundaries built"),Lines.Num(),2);
 for(auto* M:Lines){TestTrue(TEXT("Placement grid contains real line geometry"),M->GetInstanceCount()>=4);TestEqual(TEXT("Local grid does not obstruct movement"),M->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}
 Area->ShowPlacementArea(false);for(auto* M:Lines)TestFalse(TEXT("Placement areas can be hidden locally"),M->IsVisible());
 W->DestroyWorld(false);return true;
}
#endif
