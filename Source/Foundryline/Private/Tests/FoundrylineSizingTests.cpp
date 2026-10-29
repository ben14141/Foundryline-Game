#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "UI/FoundrylinePlayerController.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineSizingWorkflowTest,"Foundryline.Construction.ExactSurfaceSizingAndRotatedParking",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineSizingWorkflowTest::RunTest(const FString& Parameters) {
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);
 auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
 auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
 auto* PC=World->SpawnActor<AFoundrylinePlayerController>();PC->SetAsLocalPlayerController();
 PC->BuildTool=TEXT("Parking");PC->SetConstructionSize(6,12,true);
 const FIntVector Start(-5000,2500,0);
 TestTrue(TEXT("Exact parking preserves 6 x 12 metre clearance"),PC->ExactSurfaceEnd(Start)==Start+FIntVector(600,1200,0));
 PC->SurfaceSizingAction(TEXT("SizeRotate"));
 TestTrue(TEXT("Rotation swaps world axes, not car scale"),PC->ExactSurfaceEnd(Start)==Start+FIntVector(1200,600,0));
 TestTrue(TEXT("Rotated exact footprint commits"),Site->BuildRectangle(TEXT("Parking"),Start,PC->ExactSurfaceEnd(Start)));
 const auto Parking=Placement->GetObjects().Last();const auto* Def=Placement->FindDefinition(Parking.DefinitionId);
 TestTrue(TEXT("Saved parking retains local stall layout"),Def && Def->Footprint.Max.X==600 && Def->Footprint.Max.Y==1200 && Parking.QuarterTurns==1);
 Site->Undo();Site->DraftAreaRotation=0;
 TestTrue(TEXT("Reverse-oriented drag also accepts the same physical parking footprint"),Site->BuildRectangle(TEXT("Parking"),Start,Start+FIntVector(1200,600,0)));
 Site->Undo();
 TestFalse(TEXT("Too shallow for real parking stalls is rejected"),Site->BuildRectangle(TEXT("Parking"),Start,Start+FIntVector(600,600,0)));
 PC->BuildTool=TEXT("TruckAccess");PC->bRotateSurface=false;PC->SetConstructionSize(13,16,true);
 const FIntVector TruckStart(-3500,2500,0);
 TestTrue(TEXT("Truck access supports custom 13 m width"),Site->BuildRectangle(TEXT("TruckAccess"),TruckStart,PC->ExactSurfaceEnd(TruckStart)));
 Def=Placement->FindDefinition(Placement->GetObjects().Last().DefinitionId);
 TestTrue(TEXT("Custom width and length survive definition encoding"),Def && Def->Footprint.Max.X==1300 && Def->Footprint.Max.Y==1600);
 TestTrue(TEXT("A second world can reconstruct custom width from saved ID"),NewObject<UFoundrylinePlacementSubsystem>(World)->EnsureGeneratedDefinition(TEXT("TruckAccess_1300_1600")));
 TestFalse(TEXT("Network input cannot change a marked road lane width"),Placement->EnsureGeneratedDefinition(TEXT("Road2_900_1600")));
 Site->Undo();
 TestTrue(TEXT("Small unmarked freight paving can be built"),Site->BuildRectangle(TEXT("FreightYard"),Start,Start+FIntVector(800,800,0)));
 PC->ChooseTool(TEXT("Parking"));
 TestFalse(TEXT("Selecting pavement returns to drag placement"),PC->bExactSurface);
 PC->SurfaceSizingAction(TEXT("SizeSmall"));
 TestFalse(TEXT("Legacy size actions cannot enable stamp placement"),PC->bExactSurface);
 const FIntVector OfficeStart(-5000,0,0);
 Site->PreviewRectangle(TEXT("Office"),OfficeStart,OfficeStart+FIntVector(400,400,0));
 TestTrue(TEXT("Small standalone office shows a hologram"),Site->bHasPreviewBounds);
 TestTrue(TEXT("4 x 4 m standalone office is accepted"),Site->BuildRectangle(TEXT("Office"),OfficeStart,OfficeStart+FIntVector(400,400,0)));
 TestTrue(TEXT("Standalone office has an exterior door"),Placement->GetObjects().Last().Openings.Num()>0);
 const auto OfficeId=Placement->GetObjects().Last().ObjectId;
 Site->PreviewRectangle(TEXT("Building"),OfficeStart+FIntVector(0,400,0),OfficeStart+FIntVector(400,800,0));
 TestFalse(TEXT("Shop against an office has a valid preview"),Site->IsConstructionFeedbackError());
 TestTrue(TEXT("Shop attaches after office exists"),Site->BuildRectangle(TEXT("Building"),OfficeStart+FIntVector(0,400,0),OfficeStart+FIntVector(400,800,0)));
 const auto* Joined=Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==OfficeId;});
 TestTrue(TEXT("Office gains shop doorway on shared edge"),Joined && Joined->Openings.ContainsByPredicate([](const auto& O){return O.Edge==2 && O.Kind==EFoundrylineOpeningKind::OfficeDoor;}));
 TestTrue(TEXT("Office retains exterior entrance"),Joined && Joined->Openings.ContainsByPredicate([](const auto& O){return O.Edge==0 && O.Kind==EFoundrylineOpeningKind::OfficeDoor;}));
 World->DestroyWorld(false);return true;
}
#endif
