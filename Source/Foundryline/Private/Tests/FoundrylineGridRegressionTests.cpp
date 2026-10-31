#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Pathing/FoundrylineRouteNetwork.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "World/FoundrylineViewMath.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineGridRoundTripTest,"Foundryline.Grid.NegativeCoordinatesAndQuarterTurns",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineGridRoundTripTest::RunTest(const FString&) {
 for(auto Snap:{EFoundrylineSnap::Structural100cm,EFoundrylineSnap::Fine25cm,EFoundrylineSnap::Precision10cm,EFoundrylineSnap::Precision5cm}) {
  const int32 Step=FFoundrylineGrid::SnapStep(Snap);
  for(int32 X:{-525,-101,-50,-1,0,1,50,101,525}) {
   const FVector Point(X,-X,17);FIntVector A,B;
   TestTrue(TEXT("Finite point snaps"),FFoundrylineGrid::SnapWorld(Point,Snap,A));
   TestTrue(TEXT("Snapped point satisfies authoritative lattice validation"),FFoundrylineGrid::IsOnSnap(A,Snap));
   TestTrue(TEXT("Nearest cell never moves farther than half a step"),(FVector(A)-Point).GetAbsMax()<=Step*.5);
   TestTrue(TEXT("Snapping again is stable"),FFoundrylineGrid::SnapWorld(FVector(A),Snap,B) && A==B);
   for(uint8 Q=0;Q<4;++Q) TestTrue(TEXT("Integer quarter turns round-trip every grid size"),FFoundrylineGrid::RotateOffset(FFoundrylineGrid::RotateOffset(A,Q),(4-Q)%4)==A);
  }
 }
 FFoundrylineGridBounds Local;Local.Min={-50,25,20};Local.Max={550,1225,620};
 for(uint8 Q=0;Q<4;++Q) {
  const auto B=FFoundrylineGrid::RotateBounds(Local,{-2400,-1800,0},Q);
  TestTrue(TEXT("Rotated footprint retains real dimensions and height"),B.Max.X-B.Min.X==(Q%2?1200:600) && B.Max.Y-B.Min.Y==(Q%2?600:1200) && B.Min.Z==20 && B.Max.Z==620);
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineResizeGridTest,"Foundryline.Construction.ResizeKeepsWorldCornerAndParkingRotation",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineResizeGridTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Square parking section"),S->BuildRectangle(TEXT("Parking"),{-5000,2500,0},{-3800,3700,0}));
 const auto Parking=P->GetObjects().Last();
 TestTrue(TEXT("Select parking resize"),S->BeginEdit(TEXT("Resize"),{-3900,3600,0}));
 S->PreviewEdit(TEXT("Resize"),{-3900,3600,0},{-3800,3100,0});
 const auto Preview=S->PreviewBounds;
 TestTrue(TEXT("Preview preserves opposite corner while turning stall layout"),Preview.Min==FIntVector(-5000,2500,0) && Preview.Max==FIntVector(-3800,3100,20));
 TestTrue(TEXT("Commit rotated parking resize"),S->CommitEdit(TEXT("Resize"),{-3900,3600,0},{-3800,3100,0}));
 const auto* O=P->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Parking.ObjectId;});
 TestTrue(TEXT("Committed rotation and bounds match the hologram"),O && O->QuarterTurns==1 && FFoundrylineGrid::RotateBounds(P->FindDefinition(O->DefinitionId)->Footprint,O->Anchor,O->QuarterTurns).Min==Preview.Min && FFoundrylineGrid::RotateBounds(P->FindDefinition(O->DefinitionId)->Footprint,O->Anchor,O->QuarterTurns).Max==Preview.Max);
 S->Undo();
 TestTrue(TEXT("Test section away from the neighbor"),S->BuildRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0}));
 S->BeginEdit(TEXT("Move"),{3500,1500,0});TestTrue(TEXT("Move close without joining"),S->CommitEdit(TEXT("Move"),{3500,1500,0},{3200,1500,0}));
 S->BeginEdit(TEXT("Resize"),{4000,1900,0});S->PreviewEdit(TEXT("Resize"),{4000,1900,0},{4500,2500,0});
 TestTrue(TEXT("Neighbor magnet cannot move the fixed resize corner"),S->PreviewBounds.Min==FIntVector(2900,1000,0));
 TestTrue(TEXT("Resizing near a neighboring shell commits"),S->CommitEdit(TEXT("Resize"),{4000,1900,0},{4500,2500,0}));
 TestTrue(TEXT("Fixed corner remains fixed after authoritative placement"),P->GetObjects().Last().Anchor==FIntVector(2900,1000,0));
 W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineRouteWorldGridTest,"Foundryline.Driving.OddWidthRoadNodesFollowWorldGrid",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineRouteWorldGridTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 S->DraftTruckWidthCm=1300;S->bExactDraftPlacement=true;
 TestTrue(TEXT("13 m vertical truck access"),S->BuildRectangle(TEXT("TruckAccess"),{-3500,2500,0},{-3500,4100,0}));
 FGuid A,B;
 TestTrue(TEXT("Node on the visible world grid is accepted"),S->EditRouteNetwork(TEXT("NetDraw"),{},{-3500,3300},A));
 auto G=FFoundrylineRouteNetwork::Read(*P);const auto* N=G.Find(A);
 TestTrue(TEXT("Node stays under the world grid intersection, despite half-metre road edges"),N && N->Position.Equals({-3500,3300},.01));
 TestTrue(TEXT("13 m horizontal truck access"),S->BuildRectangle(TEXT("TruckAccess"),{-3500,4500,0},{-1900,4500,0}));
 TestTrue(TEXT("Node on rotated odd-width paving"),S->EditRouteNetwork(TEXT("NetDraw"),{},{-2400,4500},B));
 G=FFoundrylineRouteNetwork::Read(*P);N=G.Find(B);TestTrue(TEXT("Rotated surface does not introduce a half-cell offset"),N && N->Position.Equals({-2400,4500},.01));
 auto* Save=P->CreateSnapshot();TestTrue(TEXT("World-aligned nodes survive save validation"),P->RestoreSnapshot(Save,true)==EFoundrylinePlacementError::None);
 G=FFoundrylineRouteNetwork::Read(*P);N=G.Find(A);
 if(TestNotNull(TEXT("Saved route point restored"),N)) {
  const auto* Host=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==N->Surface;});
  const FGuid RoadId=Host->ObjectId;const FIntVector Fixed=Host->Anchor;
  FFoundrylinePlacementContext C;C.CompanyId=Host->CompanyId;C.PlantId=Host->PlantId;C.bCanConstruct=true;
  FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::SetRouteNodes;R.ObjectId=Host->ObjectId;R.ExpectedRevision=P->GetRevision();R.RouteNodes=Host->RouteNodes;R.RouteNodes[0].Position.X=651;
  TestFalse(TEXT("Server still rejects points off both supported grids"),P->Execute(R,C).IsSuccess());
  R.RouteNodes[0].Position.X=600;TestTrue(TEXT("Legacy local-grid route points remain compatible"),P->Execute(R,C).IsSuccess());
  // Stay below the crossing road; selecting inside two equal-area roads depends on their random IDs.
  TestTrue(TEXT("Select odd-width road corner"),S->BeginEdit(TEXT("Resize"),{-2900,3800,0}));
  TestTrue(TEXT("Resize targets the intended vertical road"),S->SelectedObject==RoadId);
  TestTrue(TEXT("Odd-to-even width edit can preserve its half-metre edge"),S->CommitEdit(TEXT("Resize"),{-2900,3800,0},{-2800,4100,0}));
  const auto* Road=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==RoadId;});
  TestTrue(TEXT("Widened road remains exactly 14 m and keeps the opposite world corner"),Road && Road->Anchor==Fixed && P->FindDefinition(Road->DefinitionId)->Footprint.Max.X==1400);
 }
 W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineCompactSiteTest,"Foundryline.Construction.CompactLoadingSiteCanExpandAndRedo",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineCompactSiteTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->bStarterProperty=true;S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();auto* Paths=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>();
 auto Map=Paths->GetMap();
 TestEqual(TEXT("Exactly two loading destinations, without a large staging yard"),Map->Targets.FilterByPredicate([](const auto& T){return T.Semi;}).Num(),2);
 TestFalse(TEXT("No trailer staging rows are forced into a small starter property"),Map->Targets.ContainsByPredicate([](const auto& T){return T.Semi && !T.Loading;}));
 TestTrue(TEXT("Both grade-level doors have real full-truck stopping clearance"),Map->Targets.FilterByPredicate([](const auto& T){return T.Loading && T.LayoutClear;}).Num()==2);
 TestTrue(TEXT("Select starter yard for expansion"),S->BeginEdit(TEXT("Resize"),{3900,3500,0}));
 TestTrue(TEXT("Yard can grow into a full trailer staging yard"),S->CommitEdit(TEXT("Resize"),{3900,3500,0},{5200,3600,0}));
 TestTrue(TEXT("Larger yard generates optional staging bays"),Paths->GetMap()->Targets.ContainsByPredicate([](const auto& T){return T.Semi && !T.Loading;}));
 S->Undo();TestFalse(TEXT("Undo restores the compact yard"),Paths->GetMap()->Targets.ContainsByPredicate([](const auto& T){return T.Semi && !T.Loading;}));
 S->Redo();TestTrue(TEXT("Redo restores staging expansion"),Paths->GetMap()->Targets.ContainsByPredicate([](const auto& T){return T.Semi && !T.Loading;}));S->Undo();
 const auto* Access=P->GetObjects().FindByPredicate([&](const auto& O){const auto* D=P->FindDefinition(O.DefinitionId);return D->FunctionId==TEXT("TruckAccess") && D->Footprint.Max.X==1200;});
 if(TestNotNull(TEXT("Starter entrance is a narrower 12 m access"),Access)) {
  const FGuid Id=Access->ObjectId;const FIntVector Anchor=Access->Anchor;
  TestTrue(TEXT("Select access width corner"),S->BeginEdit(TEXT("Resize"),{2500,-100,0}));
  TestTrue(TEXT("Truck access may widen independently of its length"),S->CommitEdit(TEXT("Resize"),{2500,-100,0},{2800,0,0}));
  const auto* O=P->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Id;});
  TestTrue(TEXT("Widening preserves the opposite corner and the same construction identity"),O && O->Anchor==Anchor && P->FindDefinition(O->DefinitionId)->Footprint.Max.X==1400 && P->FindDefinition(O->DefinitionId)->Footprint.Max.Y==2400);
  S->Undo();S->Redo();O=P->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Id;});TestTrue(TEXT("Access width changes support redo"),O && P->FindDefinition(O->DefinitionId)->Footprint.Max.X==1400);
 }
 W->DestroyWorld(false);return true;
}
#endif
