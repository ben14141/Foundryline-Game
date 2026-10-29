#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePropertyHistoryTest,"Foundryline.Construction.DuplicateRotateRedoAndConnections",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylinePropertyHistoryTest::RunTest(const FString&)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Create test shop"),S->BuildRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0}));
 const auto SourceRecord=P->GetObjects().Last();
 FFoundrylinePlacementContext Owner;Owner.CompanyId=SourceRecord.CompanyId;Owner.PlantId=SourceRecord.PlantId;Owner.bCanConstruct=true;
 FFoundrylinePlacementRequest Duplicate;Duplicate.Action=EFoundrylinePlacementAction::Duplicate;Duplicate.ObjectId=SourceRecord.ObjectId;Duplicate.Anchor=SourceRecord.Anchor+FIntVector(0,1500,0);Duplicate.ExpectedRevision=P->GetRevision();
 auto Stranger=Owner;Stranger.CompanyId=FGuid::NewGuid();
 TestTrue(TEXT("Another company cannot duplicate a section"),P->Execute(Duplicate,Stranger).Error==EFoundrylinePlacementError::AccessDenied);
 auto Revoked=Owner;Revoked.bCanConstruct=false;TestTrue(TEXT("Revoked construction permission rejects duplicate"),P->Execute(Duplicate,Revoked).Error==EFoundrylinePlacementError::AccessDenied);
 Duplicate.ExpectedRevision--;TestTrue(TEXT("Stale duplicate request rejected"),P->Execute(Duplicate,Owner).Error==EFoundrylinePlacementError::StaleRevision);
 const auto Source=P->GetObjects().Last();S->SelectedObject=Source.ObjectId;S->PaintSelected(0,FLinearColor::Red);
 S->BeginEdit(TEXT("Duplicate"),{3500,1500,0});const auto Revision=P->GetRevision();
 S->PreviewEdit(TEXT("Duplicate"),{3500,1500,0},{3500,3000,0});TestEqual(TEXT("Duplicate ghost cannot mutate"),P->GetRevision(),Revision);
 TestTrue(TEXT("Duplicate into clear space"),S->CommitEdit(TEXT("Duplicate"),{3500,1500,0},{3500,3000,0}));
 const auto CopyId=S->SelectedObject;const auto* Copy=S->GetSelectedObject();
 TestTrue(TEXT("Copy gets new identity and keeps finish"),Copy && CopyId!=Source.ObjectId && Copy->Colors.Exterior==FLinearColor::Red);
 if(Copy) TestTrue(TEXT("Opening IDs are fresh"),!Copy->Openings.ContainsByPredicate([&](const auto& A){return Source.Openings.ContainsByPredicate([&](const auto& B){return A.OpeningId==B.OpeningId;});}));
 S->Undo();TestFalse(TEXT("Undo removes only duplicate"),P->GetObjects().ContainsByPredicate([&](const auto& O){return O.ObjectId==CopyId;}));
 S->Redo();TestTrue(TEXT("Redo restores same duplicate identity"),P->GetObjects().ContainsByPredicate([&](const auto& O){return O.ObjectId==CopyId;}));
 S->BeginEdit(TEXT("Rotate"),{3500,3000,0});TestTrue(TEXT("Rotate clear shop"),S->CommitEdit(TEXT("Rotate"),{3500,3000,0},{3500,3000,0}));
 const auto Rotated=*S->GetSelectedObject();TestEqual(TEXT("Quarter turn"),int32(Rotated.QuarterTurns),1);
 S->Undo();S->Redo();TestEqual(TEXT("Redo rotation"),int32(S->GetSelectedObject()->QuarterTurns),1);
 S->BeginEdit(TEXT("Resize"),{4100,3500,0});TestTrue(TEXT("Resize rotated section"),S->CommitEdit(TEXT("Resize"),{4100,3500,0},{4200,3800,0}));
 S->Undo();TestTrue(TEXT("Resize can redo"),S->CanRedo());S->PaintSelected(2,FLinearColor::Blue);TestFalse(TEXT("New edit clears redo branch"),S->CanRedo());
 W->DestroyWorld(false);
 auto* W2=UWorld::CreateWorld(EWorldType::Game,false);auto* Starter=W2->SpawnActor<AFoundrylineBuildSite>();Starter->bStarterProperty=true;Starter->DispatchBeginPlay();auto* P2=W2->GetSubsystem<UFoundrylinePlacementSubsystem>();
 Starter->BeginEdit(TEXT("Move"),{-2400,2200,0});const auto Before=P2->GetRevision();
 TestFalse(TEXT("Moving shop into occupied starter paving is rejected"),Starter->CommitEdit(TEXT("Move"),{-2400,2200,0},{-2400,1200,0}));TestEqual(TEXT("Rejected occupied move leaves revision intact"),P2->GetRevision(),Before);
 W2->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineLivePropertyRestoreTest,"Foundryline.Save.FreshSessionAndLiveRestore",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineLivePropertyRestoreTest::RunTest(const FString&)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Custom size absent from starter"),Site->BuildRectangle(TEXT("Building"),{1000,4000,0},{1900,5300,0}));
 const auto Custom=P->GetObjects().Last();Site->SelectedObject=Custom.ObjectId;Site->PaintSelected(1,FLinearColor::Blue);
 const FString Slot=TEXT("Foundryline_QA_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 TestTrue(TEXT("Write real save file"),UGameplayStatics::SaveGameToSlot(P->CreateSnapshot(),Slot,0));
 auto* Saved=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!TestNotNull(TEXT("Read real save file"),Saved)) {W->DestroyWorld(false);return false;}
 auto* FreshWorld=UWorld::CreateWorld(EWorldType::Game,false);auto* Fresh=FreshWorld->GetSubsystem<UFoundrylinePlacementSubsystem>();
 for(const auto& O:Saved->Objects) TestTrue(TEXT("Fresh runtime reconstructs approved size"),Fresh->EnsureGeneratedDefinition(O.DefinitionId));
 TestFalse(TEXT("Reject arbitrary asset identifier"),Fresh->EnsureGeneratedDefinition(TEXT("Other_800_800")));
 TestFalse(TEXT("Reject invalid road width"),Fresh->EnsureGeneratedDefinition(TEXT("Road2_700_1200")));
 // Order independence: annex can precede its host in a serialized property.
 Saved->Objects.Sort([](const auto& A,const auto& B){return A.bOfficeAnnex && !B.bOfficeAnnex;});
 TestTrue(TEXT("Fresh world restores custom sizes and annex first"),Fresh->RestoreSnapshot(Saved)==EFoundrylinePlacementError::None);
 TestEqual(TEXT("All objects restored"),Fresh->GetObjects().Num(),P->GetObjects().Num());
 const auto* Restored=Fresh->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Custom.ObjectId;});TestTrue(TEXT("Custom ID and paint survive disk and fresh world"),Restored && Restored->Colors.Interior==FLinearColor::Blue);
 Site->PaintSelected(1,FLinearColor::Red);const auto Before=P->GetRevision();int32 Events=0;const auto Handle=P->OnPlacementChanged.AddLambda([&](const auto&){++Events;});
 TestTrue(TEXT("Load replaces active seeded property"),P->RestoreSnapshot(Saved,true)==EFoundrylinePlacementError::None);
 TestTrue(TEXT("Live revision remains monotonic"),P->GetRevision()>Before);TestTrue(TEXT("Render listeners notified"),Events>0);
 const auto* Loaded=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Custom.ObjectId;});TestTrue(TEXT("Live load restores saved paint"),Loaded && Loaded->Colors.Interior==FLinearColor::Blue);
 const auto DuplicateRecord=Saved->Objects[0];Saved->Objects.Add(DuplicateRecord);const auto GoodRevision=P->GetRevision();Events=0;
 TestTrue(TEXT("Corrupt duplicate ID rejects whole live load"),P->RestoreSnapshot(Saved,true)==EFoundrylinePlacementError::DuplicateId);TestEqual(TEXT("Rejected load retains revision"),P->GetRevision(),GoodRevision);TestEqual(TEXT("Rejected load has no visual events"),Events,0);
 P->OnPlacementChanged.Remove(Handle);UGameplayStatics::DeleteGameInSlot(Slot,0);FreshWorld->DestroyWorld(false);W->DestroyWorld(false);return true;
}
#endif
