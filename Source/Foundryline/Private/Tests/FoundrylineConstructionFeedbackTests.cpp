#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Construction/FoundrylineBuildSite.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineConstructionFeedbackTest,"Foundryline.Construction.PlacementFeedback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineConstructionFeedbackTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);
 auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
 const FIntVector A(-5000,3000,0),B(-4000,4000,0);
 Site->PreviewRectangle(TEXT("Building"),A,B);
 TestEqual(TEXT("Clear valid draft is ready"),Site->GetConstructionFeedbackTitle(),FString(TEXT("READY TO PLACE")));
 TestFalse(TEXT("Valid draft has no error"),Site->IsConstructionFeedbackError());
 TestTrue(TEXT("Authoritative construction accepted"),Site->BuildRectangle(TEXT("Building"),A,B));
 TestFalse(TEXT("Successful construction has no confirmation banner"),Site->GetConstructionFeedbackTitle()==TEXT("CHANGE APPLIED"));
 TestFalse(TEXT("Overlapping construction rejected"),Site->BuildRectangle(TEXT("Building"),A,B));
 const FString Rejection=Site->GetConstructionFeedbackText();
 TestTrue(TEXT("Rejection explains overlap"),Rejection.Contains(TEXT("Overlaps")));
 Site->PreviewRectangle(TEXT("Building"),{3000,3000,0},{4000,4000,0});
 TestEqual(TEXT("Hover does not erase rejected click reason"),Site->GetConstructionFeedbackText(),Rejection);
 Site->ClearConstructionFeedback();
 Site->PreviewRectangle(TEXT("Building"),{20000,20000,0},{21000,21000,0});
 TestEqual(TEXT("Outside plot is blocked"),Site->GetConstructionFeedbackTitle(),FString(TEXT("PLACEMENT BLOCKED")));
 TestTrue(TEXT("Outside plot has readable reason"),Site->GetConstructionFeedbackText().Contains(TEXT("build area")));
 Site->ClearConstructionFeedback();Site->PreviewRectangle(TEXT("Building"),A,{A.X+100,A.Y+100,0});
 TestTrue(TEXT("Too-small rectangle explains minimum dimensions"),Site->GetConstructionFeedbackText().Contains(TEXT("Min 4 x 4 m")));
 TestTrue(TEXT("Too-small rectangle is marked blocked"),Site->IsConstructionFeedbackError());
 TestTrue(TEXT("Stale network revision prompts retry"),AFoundrylineBuildSite::PlacementErrorText(EFoundrylinePlacementError::StaleRevision).Contains(TEXT("property changed")));
 TestTrue(TEXT("Client authority rejection is explicit"),AFoundrylineBuildSite::PlacementErrorText(EFoundrylinePlacementError::NotAuthority).Contains(TEXT("server")));
 W->DestroyWorld(false);return true;
}
#endif
