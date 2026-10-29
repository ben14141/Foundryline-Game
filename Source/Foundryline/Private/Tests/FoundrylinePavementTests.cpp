#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylineSizing.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePavementLifecycleTest,"Foundryline.Construction.AllPavementOptionsLifecycle",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylinePavementLifecycleTest::RunTest(const FString&) {
 for(const FName Tool:{FName(TEXT("Service")),FName(TEXT("Road2")),FName(TEXT("Road4")),FName(TEXT("Driveway")),FName(TEXT("TruckAccess")),FName(TEXT("Parking")),FName(TEXT("FreightYard")),FName(TEXT("Walkway"))}) {
  auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
  auto Check=[&](const TCHAR* Label,bool Value){return TestTrue(Tool.ToString()+TEXT(" / ")+Label,Value);};
  const FIntVector Start(-3500,2800,0),End=Start+(FFoundrylineSizing::Road(Tool)?FIntVector(0,1600,0):FIntVector(1000,1600,0));
  const auto Revision=P->GetRevision();S->PreviewRectangle(Tool,Start,End);const auto Preview=S->PreviewBounds;
  Check(TEXT("preview is valid and never mutates"),S->bHasPreviewBounds && !S->IsConstructionFeedbackError() && P->GetRevision()==Revision);
  if(!Check(TEXT("commit"),S->BuildRectangle(Tool,Start,End))) {W->DestroyWorld(false);continue;}
  const auto Original=P->GetObjects().Last();const FGuid Id=Original.ObjectId;
  auto Current=[&](){return P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Id;});};
  auto Bounds=[&](){const auto* O=Current();return FFoundrylineGrid::RotateBounds(P->FindDefinition(O->DefinitionId)->Footprint,O->Anchor,O->QuarterTurns);};
  auto B=Bounds();Check(TEXT("committed footprint matches hologram"),B.Min==Preview.Min && B.Max==Preview.Max);
  FHitResult Hit;const FVector Center((B.Min.X+B.Max.X)/2.,(B.Min.Y+B.Max.Y)/2.,0);
  Check(TEXT("surface is solid and level with the 20 cm shop floor"),W->LineTraceSingleByChannel(Hit,Center+FVector(0,0,100),Center-FVector(0,0,100),ECC_Visibility) && FMath::IsNearlyEqual(Hit.ImpactPoint.Z,20.,.1));
  FIntVector Pick(FMath::RoundToInt(Center.X),FMath::RoundToInt(Center.Y),0);
  Check(TEXT("select and move"),S->BeginEdit(TEXT("Move"),Pick) && S->CommitEdit(TEXT("Move"),Pick,Pick+FIntVector(300,0,0)));
  Check(TEXT("move retains identity and dimensions"),Current() && Current()->Anchor==Original.Anchor+FIntVector(300,0,0) && Current()->DefinitionId==Original.DefinitionId);
  Pick+=FIntVector(300,0,0);
  Check(TEXT("quarter-turn"),S->BeginEdit(TEXT("Rotate"),Pick) && S->CommitEdit(TEXT("Rotate"),Pick,Pick));
  const auto Rotated=*Current();const auto* D=P->FindDefinition(Rotated.DefinitionId);
  auto WorldPoint=[&](FIntVector Local){return Rotated.Anchor+FFoundrylineGrid::RotateOffset(Local,Rotated.QuarterTurns);};
  const auto Corner=WorldPoint(D->Footprint.Max-FIntVector(100,100,D->Footprint.Max.Z));
  const auto Target=WorldPoint({D->Footprint.Max.X+200,D->Footprint.Max.Y+300,0});
  Check(TEXT("select rotated corner"),S->BeginEdit(TEXT("Resize"),Corner));S->PreviewEdit(TEXT("Resize"),Corner,Target);const auto ResizedPreview=S->PreviewBounds;
  Check(TEXT("resize"),S->CommitEdit(TEXT("Resize"),Corner,Target));
  B=Bounds();Check(TEXT("resized footprint matches preview"),B.Min==ResizedPreview.Min && B.Max==ResizedPreview.Max);
  const auto* RD=P->FindDefinition(Current()->DefinitionId);
  Check(TEXT("real width and depth are preserved"),RD->Footprint.Max.X==D->Footprint.Max.X+(FFoundrylineSizing::FixedWidth(Tool)?0:200) && RD->Footprint.Max.Y==D->Footprint.Max.Y+300 && RD->Footprint.Max.Z==20);
  S->Undo();Check(TEXT("undo resize"),Current()->DefinitionId==Rotated.DefinitionId && Current()->Anchor==Rotated.Anchor);
  S->Redo();Check(TEXT("redo resize"),Bounds().Min==B.Min && Bounds().Max==B.Max);
  Pick={(B.Min.X+B.Max.X)/2,(B.Min.Y+B.Max.Y)/2,0};
  Check(TEXT("demolish"),S->BeginEdit(TEXT("Demolish"),Pick) && S->CommitEdit(TEXT("Demolish"),Pick,Pick));Check(TEXT("section removed"),!Current());
  S->Undo();Check(TEXT("undo demolition restores original identity"),Current() && Bounds().Min==B.Min && Bounds().Max==B.Max);
  auto* Saved=P->CreateSnapshot();auto* Fresh=NewObject<UFoundrylinePlacementSubsystem>(W);
  bool Definitions=true;for(const auto& O:Saved->Objects) Definitions&=Fresh->EnsureGeneratedDefinition(O.DefinitionId);
  Check(TEXT("save reconstructs approved definitions and restores edited pavement"),Definitions && Fresh->RestoreSnapshot(Saved)==EFoundrylinePlacementError::None);
  const auto BeforeInvalid=P->GetRevision();
  Check(TEXT("undersized draft rejected"),!S->BuildRectangle(Tool,{-5000,3000,0},{-4900,3100,0}));
  Check(TEXT("out-of-property draft rejected"),!S->BuildRectangle(Tool,{20000,20000,0},{21600,21600,0}));
  Check(TEXT("invalid requests do not change the property"),P->GetRevision()==BeforeInvalid);
  W->DestroyWorld(false);
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePavementRoutingResizeTest,"Foundryline.Construction.PaintedPavementResizePreservesRouting",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylinePavementRoutingResizeTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Pave test yard"),S->BuildRectangle(TEXT("FreightYard"),{-5000,2800,0},{-3800,4400,0}));const FGuid Id=P->GetObjects().Last().ObjectId;
 TestTrue(TEXT("Paint permitted truck driving area"),S->EditTruckRouting({-5000,2800,0},{-3800,4400,0}));
 FGuid Kept,Outside,Out;TestTrue(TEXT("Place retained route point"),S->EditRouteNetwork(TEXT("NetDraw"),{},{-4500,3400},Kept));TestTrue(TEXT("Place route point near old edge"),S->EditRouteNetwork(TEXT("NetDraw"),{},{-3900,4300},Outside));
 S->BeginEdit(TEXT("Resize"),{-3900,4300,0});const auto Revision=P->GetRevision();
 TestFalse(TEXT("Shrinking cannot silently lose a route point"),S->CommitEdit(TEXT("Resize"),{-3900,4300,0},{-4000,4000,0}));TestEqual(TEXT("Rejected shrink is atomic"),P->GetRevision(),Revision);
 TestTrue(TEXT("Remove the outlying route point"),S->EditRouteNetwork(TEXT("NetDelete"),Outside,{},Out));
 S->BeginEdit(TEXT("Resize"),{-3900,4300,0});TestTrue(TEXT("Painted pavement can shrink after moving or removing outlying points"),S->CommitEdit(TEXT("Resize"),{-3900,4300,0},{-4000,4000,0}));
 S->BeginEdit(TEXT("Resize"),{-4900,2900,0});TestTrue(TEXT("Opposite edge can also be adjusted without moving routing"),S->CommitEdit(TEXT("Resize"),{-4900,2900,0},{-4800,3000,0}));
 const auto* O=P->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Id;});
 TestTrue(TEXT("Driving permissions are clipped to the resized pavement"),O && O->TruckZones.Num()==1 && O->TruckZones[0].Min==FIntPoint(0,0) && O->TruckZones[0].Max==FIntPoint(800,1000));
 if(O && O->RouteNodes.Num()==1) {const auto& N=O->RouteNodes[0];TestTrue(TEXT("Route point stays at its original world location"),O->Anchor+FFoundrylineGrid::RotateOffset({N.Position.X,N.Position.Y,0},O->QuarterTurns)==FIntVector(-4500,3400,0));}
 else AddError(TEXT("Retained route point was lost"));
 S->Undo();S->Redo();O=P->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Id;});TestTrue(TEXT("Redo keeps the clipped permissions and route identity"),O && O->RouteNodes.Num()==1 && O->RouteNodes[0].Id==Kept && O->TruckZones[0].Max==FIntPoint(800,1000));
 W->DestroyWorld(false);return true;
}
#endif
