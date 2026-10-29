#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildingConnections.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineConnectionSnapTest,"Foundryline.Construction.PredictableSectionConnections",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineConnectionSnapTest::RunTest(const FString& Parameters) {
 auto Bounds=[](int32 X,int32 Y,int32 W,int32 D){FFoundrylineGridBounds B;B.Min={X,Y,0};B.Max={X+W,Y+D,620};return B;};
 const auto Shop=Bounds(0,0,2400,1600);
 FIntVector A(2500,100,0),B(4000,1500,0);
 TestTrue(TEXT("Near wall and corners magnetize"),FFoundrylineBuildingConnections::Snap(A,B,{Shop}));
 TestTrue(TEXT("Section joins flush without one metre end slivers"),A==FIntVector(2400,0,0) && B==FIntVector(4000,1600,0));
 A={2500,1500,0};B={4000,2500,0};
 TestFalse(TEXT("One metre corner contact is not a usable shop expansion"),FFoundrylineBuildingConnections::Snap(A,B,{Shop}));
 const auto North=Bounds(2400,2400,1600,1600);
 A={2500,1700,0};B={3900,2300,0};auto C=A,D=B;
 TestTrue(TEXT("Selects a nearby viable wall"),FFoundrylineBuildingConnections::Snap(A,B,{Shop,North}));
 TestTrue(TEXT("Reordered construction records also snap"),FFoundrylineBuildingConnections::Snap(C,D,{North,Shop}));
 TestTrue(TEXT("Same cursor gives the same rectangle regardless of record order"),A==C && B==D);
 A={2500,100,0};B={4000,1500,0};const auto Block=Bounds(2500,0,1500,1600);
 TestFalse(TEXT("Magnet cannot snap across another occupied section"),FFoundrylineBuildingConnections::Snap(A,B,{Shop,Block}));
 A={2400,0,0};B={4000,1600,0};
 TestTrue(TEXT("Already connected sections remain stable"),FFoundrylineBuildingConnections::Snap(A,B,{Shop}));
 A={4000,0,0};B={5600,1600,0};
 TestTrue(TEXT("Rotated world bounds use the same west edge join"),FFoundrylineBuildingConnections::Snap(A,B,{Bounds(2400,0,1600,1600)}));
 return true;
}
#endif
