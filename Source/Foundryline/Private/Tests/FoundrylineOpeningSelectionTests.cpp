#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineOpeningSelectionTest,"Foundryline.Construction.OpeningSelectionMoveDeleteUndo",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineOpeningSelectionTest::RunTest(const FString&)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();
 S->SelectionOutlines.Reset();S->RecordSelection({390,-940,0},true);
 TestTrue(TEXT("Hover outside wall catches the door rather than empty ground"),S->SelectionOutlines.Num()==1&&S->SelectionOutlines[0].Height==260);
 TestTrue(TEXT("Door selectable from its physical outside face"),S->BeginEdit(TEXT("Select"),{390,-940,0}));
 if(!TestNotNull(TEXT("Door, not parent shell, is selected"),S->GetSelectedOpening())){W->DestroyWorld(false);return false;}
 const auto DoorId=S->GetSelectedOpening()->OpeningId,BuildingId=S->GetSelectedObject()->ObjectId;
 TestEqual(TEXT("Inspector identifies the item"),S->GetSelectedItemLabel(),FString(TEXT("PERSONNEL DOOR")));
 TestTrue(TEXT("Generic move targets the door"),S->BeginEdit(TEXT("Move"),{390,-940,0}));
 TestTrue(TEXT("Move through validated replacement succeeds"),S->CommitEdit(TEXT("Move"),{390,-940,0},{400,-500,0}));
 TestTrue(TEXT("Moved door retains identity and shell"),S->GetSelectedOpening()&&S->GetSelectedOpening()->OpeningId==DoorId&&S->GetSelectedObject()->ObjectId==BuildingId);
 TestTrue(TEXT("Relocated opening is selectable"),S->BeginEdit(TEXT("Select"),{400,-540,0}));
 TestTrue(TEXT("Delete only selected opening"),S->CommitEdit(TEXT("Demolish"),{400,-540,0},{400,-540,0}));
 TestNull(TEXT("Deletion clears selection so a second Delete cannot remove the parent"),S->GetSelectedObject());
 S->BeginEdit(TEXT("Select"),{1400,-400,0});TestTrue(TEXT("Building still exists"),S->GetSelectedObject()&&S->GetSelectedObject()->ObjectId==BuildingId);
 if(S->GetSelectedObject())TestFalse(TEXT("Only that door removed"),S->GetSelectedObject()->Openings.ContainsByPredicate([&](const auto& V){return V.OpeningId==DoorId;}));
 S->Undo();S->BeginEdit(TEXT("Select"),{400,-540,0});TestTrue(TEXT("Undo restores the same door"),S->GetSelectedOpening()&&S->GetSelectedOpening()->OpeningId==DoorId);
 const auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto Objects=P->GetObjects();
 bool WindowTested=false;
 for(const auto& O:Objects)if(O.ObjectId==BuildingId)for(const auto& V:O.Openings)if(V.Kind==EFoundrylineOpeningKind::ShopWindow)
 {
  const auto* D=P->FindDefinition(O.DefinitionId);const double U=V.OffsetCm+V.WidthCm()*.5;
  const FVector Local=V.Edge%2==0?FVector(U,V.Edge==0?0:D->Footprint.Max.Y,0):FVector(V.Edge==3?0:D->Footprint.Max.X,U,0);
  const FVector At=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector(Local);const FIntVector Cursor(FMath::RoundToInt(At.X),FMath::RoundToInt(At.Y),0);
  S->BeginEdit(TEXT("Select"),Cursor);if(!S->GetSelectedOpening()||S->GetSelectedOpening()->OpeningId!=V.OpeningId)continue;
  TestTrue(TEXT("Windows are individually selectable and removable"),S->CommitEdit(TEXT("Demolish"),Cursor,Cursor));WindowTested=true;break;
 }
 TestTrue(TEXT("A window edit was exercised"),WindowTested);W->DestroyWorld(false);return true;
}
#endif
