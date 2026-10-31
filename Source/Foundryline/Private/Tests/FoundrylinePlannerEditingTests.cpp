#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePlannerSafetyTest,"Foundryline.Construction.PlannerEquipmentSafety",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylinePlannerSafetyTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed();M.State.Running=false;FString E;
 TestTrue(TEXT("Seed model valid"),M.Valid(E));const int InitialPallets=M.State.Pallets.Num();const auto Revision=M.State.Revision;
 TestFalse(TEXT("Loaded rack cannot be removed"),M.EditPlanningItem(3,100,{0,0,20},true,E));TestEqual(TEXT("Rejected edit leaves stock untouched"),M.State.Pallets.Num(),InitialPallets);TestEqual(TEXT("Rejected edit leaves revision untouched"),M.State.Revision,Revision);
 TestTrue(TEXT("Empty equipment can be stored"),M.EditPlanningItem(1,3,{0,0,20},true,E));TestTrue(TEXT("Stored machine keeps its identity"),M.State.Machines[2].bStored);TestTrue(TEXT("Stored state valid"),M.Valid(E));
 FString Json;FJsonObjectConverter::UStructToJsonObjectString(M.State,Json);FFoundrylineFactoryState Reload;TestTrue(TEXT("Stored equipment round trips through save payload"),FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Reload));TestTrue(TEXT("Stored flag preserved"),Reload.Machines[2].bStored);
 M.State=Reload;const FVector Before=M.State.Machines[2].Origin;TestFalse(TEXT("Equipment cannot be restored outside floor"),M.EditPlanningItem(1,3,{99999,99999,20},false,E));TestTrue(TEXT("Failed placement keeps stored equipment intact"),M.State.Machines[2].bStored&&M.State.Machines[2].Origin.Equals(Before));
 const auto Items=FFoundrylinePlannerItems::List(M.State);TestTrue(TEXT("Stored equipment remains listed for placement"),Items.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylinePlannerItem::Machine&&V.Id==3&&V.Stored;}));
 TestTrue(TEXT("Stored machine can be restored at a valid position: ")+E,M.EditPlanningItem(1,3,Before,false,E));TestFalse(TEXT("Restored machine visible again"),M.State.Machines[2].bStored);
 const auto* OldSlot=M.State.Slots.FindByPredicate([](const auto& V){return V.Id==107;});const FVector OldDock=OldSlot?OldSlot->Dock:FVector::ZeroVector;
 TestFalse(TEXT("Machine output rack stays attached to its workspace"),M.EditPlanningItem(3,107,{-700,4700,20},false,E));const auto* NewSlot=M.State.Slots.FindByPredicate([](const auto& V){return V.Id==107;});TestTrue(TEXT("Rejected independent rack move preserves forklift dock"),NewSlot&&NewSlot->Dock.Equals(OldDock,.1));
 const int ScrapId=M.State.Pallets[0].Id;const float ScrapBefore=M.State.ScrapLb;TestTrue(TEXT("Unreserved stock can be scrapped"),M.EditPlanningItem(7,ScrapId,{0,0,20},true,E));TestTrue(TEXT("Scrapped stock remains accounted for"),M.State.ScrapLb>ScrapBefore&&M.Valid(E));
 M.State.Running=true;TestFalse(TEXT("Running production blocks layout mutations"),M.EditPlanningItem(1,2,{0,0,20},true,E));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineManufacturingSignTest,"Foundryline.Construction.ManufacturingSign",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineManufacturingSignTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
 TestTrue(TEXT("Sign can be placed"),Site->BuildRectangle(TEXT("Sign"),{1000,4000,0},{1000,4000,0}));
 TestTrue(TEXT("Sign can be selected"),Site->BeginEdit(TEXT("Select"),{1000,4000,0}));const auto* O=Site->GetSelectedObject();TestTrue(TEXT("Selected record is the sign"),O&&O->DefinitionId==TEXT("ManufacturingSign"));
 if(O){const auto Id=O->ObjectId;auto Colors=O->Colors;Colors.Exterior=FLinearColor(.2,.3,.4);TestTrue(TEXT("Name rotation and color editable"),Site->SetSelectedSign(TEXT("IRON VALLEY WORKS"),45,Colors));
  O=Site->GetSelectedObject();TestTrue(TEXT("Edits retain sign identity and physical scale"),O&&O->ObjectId==Id&&O->SignName==TEXT("IRON VALLEY WORKS")&&O->SignFacingDegrees==45&&O->Colors.Exterior.Equals(Colors.Exterior));
  TestFalse(TEXT("Empty names rejected"),Site->SetSelectedSign(TEXT(""),0,Colors));TestFalse(TEXT("Markup rejected"),Site->SetSelectedSign(TEXT("<bad>"),0,Colors));
  FString Json;FJsonObjectConverter::UStructToJsonObjectString(*Site->GetSelectedObject(),Json);FFoundrylinePlacedObject Reload;TestTrue(TEXT("Sign saves"),FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Reload));TestEqual(TEXT("Company name survives save"),Reload.SignName,FString(TEXT("IRON VALLEY WORKS")));TestEqual(TEXT("Fine rotation survives save"),Reload.SignFacingDegrees,45.f);
  Site->Undo();TestTrue(TEXT("Appearance change supports undo"),Site->GetSelectedObject()&&Site->GetSelectedObject()->SignName!=TEXT("IRON VALLEY WORKS"));
 }
 W->DestroyWorld(false);return true;
}
#endif
