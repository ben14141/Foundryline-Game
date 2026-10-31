#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Buildings/FoundrylineRoofLayout.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
using Roof=FFoundrylineRoofLayout;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoofLayoutTest,"Foundryline.Construction.Roofs.RealScaleAperturesAndRecessedLights",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRoofLayoutTest::RunTest(const FString&){
 for(bool Office:{false,true})for(const FVector2D Size:{FVector2D(400,400),FVector2D(1200,800),FVector2D(2800,2200)}){
  FFoundrylinePlacedObject O;O.bOfficeAnnex=Office;const auto Windows=Roof::Windows(O,Size.X,Size.Y);const auto Solids=Roof::SolidPanels(Size.X,Size.Y,Windows);double Area=0;
  for(int I=0;I<Solids.Num();++I){const auto& A=Solids[I];Area+=A.GetArea();for(int J=0;J<I;++J){const auto& B=Solids[J];TestFalse(TEXT("Roof solids do not overlap"),A.Min.X<B.Max.X-.01&&A.Max.X>B.Min.X+.01&&A.Min.Y<B.Max.Y-.01&&A.Max.Y>B.Min.Y+.01);}for(const auto& V:Windows){const auto B=V.Bounds();TestFalse(TEXT("Apertures are real holes, with no opaque ceiling beneath"),A.Min.X<B.Max.X-.01&&A.Max.X>B.Min.X+.01&&A.Min.Y<B.Max.Y-.01&&A.Max.Y>B.Min.Y+.01);}}
  for(const auto& V:Windows){Area+=V.Size.X*V.Size.Y;TestTrue(TEXT("Skylight scale fixed across roof sizes"),Office?FMath::IsNearlyEqual(V.Size.GetMin(),91.44,.01)&&FMath::IsNearlyEqual(V.Size.GetMax(),121.92,.01):V.Size==FVector2D(400,400));TestTrue(TEXT("Skylights leave roof-edge clearance"),V.Bounds().Min.GetMin()>=59&&V.Bounds().Max.X<=Size.X-59&&V.Bounds().Max.Y<=Size.Y-59);}
  TestTrue(TEXT("Roof plus holes exactly covers footprint"),FMath::Abs(Area-Size.X*Size.Y)<.1);
  if(Office){const auto Fixtures=Roof::OfficeFixtures(O,Size.X,Size.Y);TestTrue(TEXT("Each tested office has artificial lights"),Fixtures.Num()>0);for(const auto& F:Fixtures){TestTrue(TEXT("Standard 2 by 4 foot panel"),FMath::IsNearlyEqual(F.Size.GetMin(),60.96,.01)&&FMath::IsNearlyEqual(F.Size.GetMax(),121.92,.01));for(const auto& V:Windows)TestFalse(TEXT("Ceiling fixtures never cover skylight glass"),FBox2D(F.Center-F.Size/2,F.Center+F.Size/2).ExpandBy(8).Intersect(V.Bounds().ExpandBy(18)));}}
 }
 FFoundrylinePlacedObject Office;Office.bOfficeAnnex=true;FFoundrylineOfficeSpace Room;Room.Min={200,200};Room.Max={600,600};Office.OfficeRooms.Add(Room);const auto Fixtures=Roof::OfficeFixtures(Office,800,1200);bool LitRoom=false;for(const auto& F:Fixtures){const auto B=FBox2D(F.Center-F.Size/2,F.Center+F.Size/2);if(F.Center.X>200&&F.Center.X<600&&F.Center.Y>200&&F.Center.Y<600){LitRoom=true;TestTrue(TEXT("Room fixture clears partitions"),B.Min.X>=225&&B.Min.Y>=225&&B.Max.X<=575&&B.Max.Y<=575);}}TestTrue(TEXT("Enclosed office receives a light"),LitRoom);TestTrue(TEXT("Narrow hallway has lights"),Roof::OfficeFixtures(FFoundrylinePlacedObject(),200,1200).Num()>0);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeGlassTest,"Foundryline.Construction.OfficeGlass.ManualWallSpanDoorsAndUndo",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeGlassTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestFalse(TEXT("Office glazing cannot be put on a shop"),S->PlaceDoor(TEXT("OfficeGlass"),{1600,-1000,0}));
 TestTrue(TEXT("Office placement"),S->OfficeRectangle(TEXT("OfficeArea"),{3200,1000,0},{4400,2000,0}));
 TestTrue(TEXT("Office entrance already present"),P->GetObjects().Last().Openings.Num()==1);
 const auto Revision=P->GetRevision();S->PreviewDoor(TEXT("OfficeGlass"),{3300,1000,0});TestEqual(TEXT("Hologram does not mutate shared state"),P->GetRevision(),Revision);
 TestTrue(TEXT("Glass fills available span beside door"),S->PlaceDoor(TEXT("OfficeGlass"),{3300,1000,0}));
 const auto* O=P->GetObjects().FindByPredicate([](const auto& V){return V.bOfficeAnnex&&V.Anchor.X==3200;});if(TestNotNull(TEXT("Office remains"),O)){const auto* G=O->Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeGlassWall;});if(TestNotNull(TEXT("Manual glass record"),G)){TestFalse(TEXT("Not an automatic office window"),G->bAutomatic);TestEqual(TEXT("Fixed floor-to-ceiling glass height"),G->HeightCm(),270);TestTrue(TEXT("Stops before doorway, leaves corner post"),G->OffsetCm==25&&G->WidthCm()>=100&&G->OffsetCm+G->WidthCm()<500);}TestTrue(TEXT("Entrance preserved"),O->Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor;}));}
 S->Undo();O=P->GetObjects().FindByPredicate([](const auto& V){return V.bOfficeAnnex&&V.Anchor.X==3200;});TestTrue(TEXT("Undo removes glazing and retains office door"),O&&O->Openings.Num()==1);W->DestroyWorld(false);return true;
}
#endif
