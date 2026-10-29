#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "World/FoundrylineViewMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineGroundPickingTest, "Foundryline.View.GroundPicking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineGroundPickingTest::RunTest(const FString& Parameters)
{
	FIntVector Anchor;
	TestTrue(TEXT("Downward ray finds the ground"), FFoundrylineViewMath::GroundAnchor(
		FVector(38, -38, 1000), FVector(0, 0, -1), EFoundrylineSnap::Fine25cm, Anchor));
	TestTrue(TEXT("Ground pick uses active snap and zero height"), Anchor == FIntVector(50, -50, 0));
	TestFalse(TEXT("Ray above ground pointing away cannot select"), FFoundrylineViewMath::GroundAnchor(
		FVector(0, 0, 1000), FVector(0, 0, 1), EFoundrylineSnap::Structural100cm, Anchor));
	TestFalse(TEXT("Parallel ray cannot select"), FFoundrylineViewMath::GroundAnchor(
		FVector(0, 0, 1000), FVector(1, 0, 0), EFoundrylineSnap::Structural100cm, Anchor));
	TestTrue(TEXT("Oblique ray intersects correctly"), FFoundrylineViewMath::GroundAnchor(
		FVector(0, 0, 100), FVector(1, 0, -1), EFoundrylineSnap::Structural100cm, Anchor));
	TestTrue(TEXT("Intersection uses ray direction rather than camera origin"), Anchor == FIntVector(100, 0, 0));
	return true;
}

#endif
