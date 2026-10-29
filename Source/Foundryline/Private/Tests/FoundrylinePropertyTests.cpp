#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Companies/FoundrylinePropertyLot.h"
#include "SaveSystem/FoundrylinePlacementSave.h"
#include "Engine/World.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePropertyTest,"Foundryline.Construction.StarterPropertyAndProtectedStreet",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylinePropertyTest::RunTest(const FString&)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);
 auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->bStarterProperty=true;S->DispatchBeginPlay();
 auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Starter property initializes"),S->IsReady());
 TestEqual(TEXT("Street, shop, parking and freight surfaces all placed"),P->GetObjects().Num(),10);
 auto Count=[&](FName Kind){int32 N=0;for(const auto& O:P->GetObjects()) if(P->FindDefinition(O.DefinitionId)->FunctionId==Kind) ++N;return N;};
 TestEqual(TEXT("Small shop and attached office"),Count(TEXT("BuildingShell")),2);
 TestEqual(TEXT("Compact yard and wall-side loading apron"),Count(TEXT("FreightYard")),2);
 const auto* Office=P->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeAnnex;});
 TestTrue(TEXT("Office has room and native furniture"),Office && Office->bAutoFurnished && Office->OfficeRooms.Num()==1);
 TestTrue(TEXT("Office retains outside and shop entrances"),Office && Office->Openings.FilterByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor;}).Num()==2);
 const auto* Shop=P->GetObjects().FindByPredicate([&](const auto& O){return !O.bOfficeAnnex && P->FindDefinition(O.DefinitionId)->FunctionId==TEXT("BuildingShell");});
 const auto* Dock=Shop?Shop->Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::LoadingDock;}):nullptr;
 TestTrue(TEXT("Dock is saved construction with a level floor and 3.5 m opening"),Dock && Dock->BottomCm()==0 && Dock->WidthCm()==350 && Dock->HeightCm()==350);
 TestEqual(TEXT("Starter shop has two loading doors"),Shop?Shop->Openings.FilterByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::LoadingDock;}).Num():0,2);
 const auto* Yard=P->GetObjects().FindByPredicate([&](const auto& O){return P->FindDefinition(O.DefinitionId)->FunctionId==TEXT("FreightYard");});
 TestTrue(TEXT("Starter apron is smaller than a full trailer staging yard"),Yard && P->FindDefinition(Yard->DefinitionId)->Footprint.Max.X==4000);
 FHitResult Hit;
 TestFalse(TEXT("Loading entrance is clear at floor height"),W->LineTraceSingleByChannel(Hit,{-50,2650,60},{-450,2650,60},ECC_Visibility));
 TestFalse(TEXT("Dock opening clears loading height"),W->LineTraceSingleByChannel(Hit,{-50,2650,300},{-450,2650,300},ECC_Visibility));
 TestEqual(TEXT("One editable truck entrance"),Count(TEXT("TruckAccess")),1);
 S->PreviewDoor(TEXT("LoadingDock"),{-400,2600,0});int32 GhostParts=0;
 for(TActorIterator<AActor> It(W);It;++It) if(It->ActorHasTag(TEXT("FoundrylinePlacementPreview")) && !It->IsActorBeingDestroyed())
 {
  TInlineComponentArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
  for(auto* C:Components) {++GhostParts;TestTrue(TEXT("Dock preview has no collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);for(int32 I=0;I<C->GetNumMaterials();++I) TestTrue(TEXT("Every dock part is a hologram"),C->GetMaterial(I) && C->GetMaterial(I)->GetBlendMode()==BLEND_Translucent);}
 }
 TestTrue(TEXT("Loading bay details and opening appear in preview"),GhostParts>=3);S->HidePreview();
 TestTrue(TEXT("Select loading dock for relocation"),S->BeginEdit(TEXT("MoveDoor"),{-400,2600,0}));
 TestTrue(TEXT("Dock snaps to a different exterior wall"),S->CommitEdit(TEXT("MoveDoor"),{-400,2600,0},{-2800,3100,0}));
 S->Undo();
 TestTrue(TEXT("Dock move undo restores original wall"),P->GetObjects().ContainsByPredicate([](const auto& O){return O.Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::LoadingDock && V.Edge==1;});}));
 for(const FVector2D Point:{FVector2D(-3000,-2800),FVector2D(2000,-2450),FVector2D(2000,-2350),FVector2D(-2400,-1000),FVector2D(-2500,200),FVector2D(-2900,1500),FVector2D(-1000,2000)})
 {
  FHitResult Floor;const FVector Top(Point.X,Point.Y,100),Bottom(Point.X,Point.Y,-100);
  TestTrue(TEXT("Connected road, parking, walkway and shop have solid surfaces"),W->LineTraceSingleByChannel(Floor,Top,Bottom,ECC_Visibility));
  TestTrue(TEXT("All finished surfaces stay level at 20 cm"),FMath::IsNearlyEqual(Floor.ImpactPoint.Z,20.,.3));
 }
 TestFalse(TEXT("Building setback enforced"),S->BuildRectangle(TEXT("Building"),{-4000,4000,0},{-3300,4700,0}));
 TestFalse(TEXT("Property edge enforced for paving"),S->BuildRectangle(TEXT("Parking"),{6000,4000,0},{7000,5500,0}));
 TestFalse(TEXT("Owner cannot select town street for demolition"),S->BeginEdit(TEXT("Demolish"),{-5000,-2800,0}));
 const int32 TruckTargets=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetTruckTargetCount();
 TestTrue(TEXT("Smaller yard remains usable as open paving"),S->BuildRectangle(TEXT("FreightYard"),{1000,4000,0},{3000,5300,0}));
 TestEqual(TEXT("Small yard cannot invent truck bays without a turning apron"),W->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetTruckTargetCount(),TruckTargets);
 auto* Snapshot=P->CreateSnapshot();
 TestEqual(TEXT("Owned lot and public corridor saved"),Snapshot->Regions.Num(),2);
 TestTrue(TEXT("Setback is saved with property"),Snapshot->Regions.ContainsByPredicate([](const auto& R){return R.bHasBuildingSetback && R.BuildingBounds.Min.X==-3700;}));
 auto* Restored=NewObject<UFoundrylinePlacementSubsystem>(W);
 for(const auto& O:P->GetObjects()) if(!Restored->FindDefinition(O.DefinitionId)) Restored->RegisterDefinition(const_cast<UFoundrylinePlaceableDefinition*>(P->FindDefinition(O.DefinitionId)));
 TestTrue(TEXT("Property snapshot restores"),Restored->RestoreSnapshot(Snapshot)==EFoundrylinePlacementError::None);
 W->DestroyWorld(false);return true;
}
