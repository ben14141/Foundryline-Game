#include "Materials/Material.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineEditingTest,"Foundryline.Construction.EditResizeDoorDemolitionUndo",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineEditingTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();
 auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("New test section"),S->BuildRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0}));
 const auto Original=P->GetObjects().Last();
 auto Current=[&]()->const FFoundrylinePlacedObject& {return *P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Original.ObjectId;});};
 TestTrue(TEXT("Select for move"),S->BeginEdit(TEXT("Move"),{3500,1500,0}));
 const auto Revision=P->GetRevision();S->PreviewEdit(TEXT("Move"),{3500,1500,0},{4000,2000,0});
 TestEqual(TEXT("Edit preview has no mutation"),P->GetRevision(),Revision);
 TestTrue(TEXT("Move section"),S->CommitEdit(TEXT("Move"),{3500,1500,0},{4000,2000,0}));
 TestTrue(TEXT("Identity and dimensions retained"),Current().ObjectId==Original.ObjectId && Current().Anchor==FIntVector(3700,1500,0) && Current().DefinitionId==Original.DefinitionId);
 S->Undo();TestTrue(TEXT("Undo returns original anchor"),Current().Anchor==Original.Anchor);
 S->BeginEdit(TEXT("Resize"),{4300,1900,0});
 TestTrue(TEXT("Corner resize"),S->CommitEdit(TEXT("Resize"),{4300,1900,0},{4600,2400,0}));
 TestEqual(TEXT("Resize width at real scale"),P->FindDefinition(Current().DefinitionId)->Footprint.Max.X,1400);
 TestEqual(TEXT("Resize depth at real scale"),P->FindDefinition(Current().DefinitionId)->Footprint.Max.Y,1400);
 S->Undo();TestTrue(TEXT("Undo restores original definition"),Current().DefinitionId==Original.DefinitionId);
 TestTrue(TEXT("Door installation"),S->PlaceDoor(TEXT("Personnel"),{3200,1200,0}));
 const auto Door=*Current().Openings.FindByPredicate([](const auto& O){return O.Kind==EFoundrylineOpeningKind::Personnel;});
 TestTrue(TEXT("Select actual door"),S->BeginEdit(TEXT("MoveDoor"),{3200,1200,0}));
 TestTrue(TEXT("Relocate opening atomically"),S->CommitEdit(TEXT("MoveDoor"),{3200,1200,0},{3200,1800,0}));
 const auto* Moved=Current().Openings.FindByPredicate([&](const auto& O){return O.OpeningId==Door.OpeningId;});
 TestTrue(TEXT("Door identity preserved and old hole closes"),Moved && Moved->OffsetCm==Door.OffsetCm+600 && Current().Openings.FilterByPredicate([](const auto& O){return O.Kind==EFoundrylineOpeningKind::Personnel;}).Num()==1);
 S->Undo();TestTrue(TEXT("Door undo restores offset"),Current().Openings.ContainsByPredicate([&](const auto& O){return O.OpeningId==Door.OpeningId && O.OffsetCm==Door.OffsetCm;}));
 S->BeginEdit(TEXT("Move"),{3500,1500,0});
 const auto Before=P->GetRevision();TestFalse(TEXT("Move into existing factory rejected"),S->CommitEdit(TEXT("Move"),{3500,1500,0},{1500,0,0}));
 TestEqual(TEXT("Rejected edit atomic"),P->GetRevision(),Before);
 S->BeginEdit(TEXT("Demolish"),{3500,1500,0});TestTrue(TEXT("Demolish selected section"),S->CommitEdit(TEXT("Demolish"),{3500,1500,0},{3500,1500,0}));
 TestFalse(TEXT("Record removed"),P->GetObjects().ContainsByPredicate([&](const auto& O){return O.ObjectId==Original.ObjectId;}));
 S->Undo();TestTrue(TEXT("Undo demolition restores ID and door metadata"),Current().ObjectId==Original.ObjectId && Current().Openings.ContainsByPredicate([&](const auto& O){return O.OpeningId==Door.OpeningId;}));
 TestTrue(TEXT("Remove parking grass and restore"),S->ApplyGroundTool(TEXT("Landscape"),{-2000,0,0}));
 const auto* Park=P->GetObjects().FindByPredicate([&](const auto& O){return P->FindDefinition(O.DefinitionId)->FunctionId==TEXT("Parking");});const auto ParkId=Park->ObjectId;
 TestFalse(TEXT("Landscaping flag stored"),Park->bLandscaping);S->Undo();
 TestTrue(TEXT("Landscaping undo"),P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==ParkId;})->bLandscaping);
 W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineJunctionTest,"Foundryline.Construction.JunctionCrosswalkAndEntrance",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineJunctionTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 TestTrue(TEXT("Vertical approach"),S->BuildRectangle(TEXT("Road2"),{3200,-4000,0},{3200,-1000,0}));
 const auto Road=P->GetObjects().Last();
 TestTrue(TEXT("Full X intersection across existing road"),S->BuildRectangle(TEXT("Road2"),{1800,-2500,0},{5000,-2500,0}));
 FHitResult Hit;
 TestFalse(TEXT("No curb barrier through junction"),W->LineTraceSingleByChannel(Hit,{3600,-2500,30},{3800,-2500,30},ECC_Visibility));
 int32 Cover=0;
 for(TActorIterator<AActor> It(W);It;++It) {TInlineComponentArray<UInstancedStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) if(C->GetMaterial(0) && C->GetMaterial(0)->GetName()==TEXT("M_PavementAsphalt")) for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T,true);const auto Local=T.InverseTransformPosition({3230,-2470,10});if(Local.GetAbs().GetMax()<49.99) ++Cover;}}
 TestEqual(TEXT("Single asphalt surface at intersection avoids z fighting"),Cover,1);
 const auto Revision=P->GetRevision();S->ApplyGroundTool(TEXT("Crosswalk"),{3200,-3500,0},true);TestEqual(TEXT("Crosswalk preview leaves state intact"),P->GetRevision(),Revision);
 TestTrue(TEXT("Crosswalk placement"),S->ApplyGroundTool(TEXT("Crosswalk"),{3200,-3500,0}));
 const auto* R=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Road.ObjectId;});TestEqual(TEXT("Saved crossing location"),R->Crosswalks.Num(),1);
 TestFalse(TEXT("Curb removed for ramp"),W->LineTraceSingleByChannel(Hit,{2770,-3500,30},{2810,-3500,30},ECC_Visibility));
 S->Undo();TestEqual(TEXT("Undo crossing"),P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Road.ObjectId;})->Crosswalks.Num(),0);
 TestTrue(TEXT("Dedicated six-metre parking driveway"),S->BuildRectangle(TEXT("Driveway"),{-5000,0,0},{-3600,0,0}));
 TestFalse(TEXT("Parking entrance curbs open"),W->LineTraceSingleByChannel(Hit,{-3620,0,30},{-3580,0,30},ECC_Visibility));
 TestFalse(TEXT("Road still cannot pass through factory"),S->BuildRectangle(TEXT("Road2"),{1000,-2000,0},{1000,2000,0}));
 W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineEditAuthorityTest,"Foundryline.Placement.EditOwnershipAndUndoAuthority",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineEditAuthorityTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto O=P->GetObjects().Last();
 FFoundrylinePlacementContext C;C.CompanyId=O.CompanyId;C.PlantId=O.PlantId;C.bCanConstruct=true;
 FFoundrylinePlacementRequest R;R.ObjectId=O.ObjectId;R.ExpectedRevision=P->GetRevision();R.Action=EFoundrylinePlacementAction::ReplaceObject;R.Replacement=O;R.Replacement.CompanyId=FGuid::NewGuid();
 TestFalse(TEXT("Replacement cannot transfer ownership"),P->Execute(R,C).IsSuccess());
 R.Action=EFoundrylinePlacementAction::Remove;TestTrue(TEXT("Owner may demolish"),P->Execute(R,C).IsSuccess());
 R.Action=EFoundrylinePlacementAction::RestoreRemoved;R.ExpectedRevision=P->GetRevision();auto Stranger=C;Stranger.CompanyId=FGuid::NewGuid();
 TestFalse(TEXT("Different company cannot restore tombstone"),P->Execute(R,Stranger).IsSuccess());
 TestTrue(TEXT("Owner may restore exact server record"),P->Execute(R,C).IsSuccess());
 R.ExpectedRevision=P->GetRevision();TestFalse(TEXT("Cannot duplicate restored identity"),P->Execute(R,C).IsSuccess());
 R.ObjectId=FGuid::NewGuid();TestFalse(TEXT("Cannot inject an arbitrary undo identity"),P->Execute(R,C).IsSuccess());
 const auto Snapshot=P->CreateSnapshot();TestEqual(TEXT("Snapshot contains all restored objects"),Snapshot->Objects.Num(),P->GetObjects().Num());
 W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineOfficeTest,"Foundryline.Construction.OfficeZonesRoomsAndActualDoors",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineOfficeTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->bLargeShowcase=true;S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 const auto* Host=P->GetObjects().FindByPredicate([](const auto& O){return !O.OfficeAreas.IsEmpty();});
 if(!TestNotNull(TEXT("Showcase has open office area"),Host)) {W->DestroyWorld(false);return false;}
 const auto HostId=Host->ObjectId;
 TestTrue(TEXT("Office begins with an exterior entrance and a shop door"),Host->Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==3;}) && Host->Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==1;}));

 TestTrue(TEXT("Office is an exterior annex"),Host->bOfficeAnnex && Host->Anchor.X==5200);
 TestFalse(TEXT("Office placement inside shop rejected"),S->OfficeRectangle(TEXT("OfficeArea"),{700,500,0},{2000,1700,0}));
 TestTrue(TEXT("Detached office accepted"),S->OfficeRectangle(TEXT("OfficeArea"),{9000,3000,0},{11000,5000,0})); S->Undo(); Host=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==HostId;});
 TestFalse(TEXT("Rooms cannot be placed inside shop"),S->OfficeRectangle(TEXT("OfficeRoom"),{700,500,0},{2000,1700,0}));
 TestFalse(TEXT("Sub-four-metre rooms rejected for furniture clearance"),S->OfficeRectangle(TEXT("OfficeRoom"),{7200,-1300,0},{7500,-900,0}));
 for(const auto& Room:Host->OfficeRooms) TestTrue(TEXT("Sample rooms reserve realistic desk and circulation space"),Room.Max.X-Room.Min.X>=700 && Room.Max.Y-Room.Min.Y>=800);
 const auto Shop=*P->GetObjects().FindByPredicate([&](const auto& O){return !O.bOfficeAnnex && O.Anchor==FIntVector(400,-2400,0);});
 FFoundrylinePlacementContext Owner;Owner.CompanyId=Shop.CompanyId;Owner.PlantId=Shop.PlantId;Owner.bCanConstruct=true;
 FFoundrylinePlacementRequest Remove;Remove.Action=EFoundrylinePlacementAction::Remove;Remove.ObjectId=Shop.ObjectId;Remove.ExpectedRevision=P->GetRevision();
 TestTrue(TEXT("Office remains valid independently when shop is removed"),P->PreviewBatch({Remove},Owner).IsSuccess());
 FFoundrylinePlacementRequest Change;Change.Action=EFoundrylinePlacementAction::ReplaceObject;Change.ObjectId=HostId;Change.ExpectedRevision=P->GetRevision();Change.Replacement=*Host;
 Change.Replacement.Openings.RemoveAll([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==3;});
 TestFalse(TEXT("Required shop door cannot be removed"),P->Execute(Change,Owner).IsSuccess());
 Change.Replacement=*Host;Change.Replacement.Openings.RemoveAll([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==1;});
 TestFalse(TEXT("Required outside door cannot be removed"),P->Execute(Change,Owner).IsSuccess());
 FHitResult DoorHit;
 TestFalse(TEXT("Shop doorway is physically open"),W->LineTraceSingleByChannel(DoorHit,{5170,-1250,100},{5230,-1250,100},ECC_Visibility));
 TestTrue(TEXT("Office partition retained beside shop doorway"),W->LineTraceSingleByChannel(DoorHit,{5170,-900,100},{5230,-900,100},ECC_Visibility));
 TestTrue(TEXT("Shop-to-office transition closes immediately above the office ceiling"),W->LineTraceSingleByChannel(DoorHit,{5170,-900,321},{5230,-900,321},ECC_Visibility));
 TestTrue(TEXT("Shop-to-office upper wall remains structural with roofs hidden"),W->LineTraceSingleByChannel(DoorHit,{5170,-900,619},{5230,-900,619},ECC_Visibility));
 TestFalse(TEXT("Exterior doorway is physically open"),W->LineTraceSingleByChannel(DoorHit,{7970,-1250,100},{8030,-1250,100},ECC_Visibility));
 auto Current=[&]()->const FFoundrylinePlacedObject& {return *P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==HostId;});};
 TestEqual(TEXT("Sample offices and meeting room"),Current().OfficeRooms.Num(),3);
 TestTrue(TEXT("Corner office reuses two perimeter walls"),Current().OfficeRooms.Num()==3 && Current().OfficeRooms[0].Min==FIntPoint(0,0));
 auto WallsAt=[&](FVector Point)
 {
  int32 Count=0;
  for(TActorIterator<AActor> It(W);It;++It) if(!It->IsActorBeingDestroyed() && !It->ActorHasTag(TEXT("FoundrylinePlacementPreview"))) {TInlineComponentArray<UInstancedStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) if(C->GetMaterial(0) && (C->GetMaterial(0)->GetBaseMaterial()->GetName()==TEXT("M_Wall") || C->GetMaterial(0)->GetBaseMaterial()->GetName()==TEXT("M_WallCladding") || C->GetMaterial(0)->GetBaseMaterial()->GetName()==TEXT("M_InteriorFinish"))) for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T,true);if(T.InverseTransformPosition(Point).GetAbs().GetMax()<49.99) ++Count;}}
  return Count;
 };
 TestEqual(TEXT("Exactly one wall at reused outer perimeter"),WallsAt({5215,-1900,120}),1);
 TestTrue(TEXT("Room may snap flush against outside wall"),S->OfficeRectangle(TEXT("OfficeRoom"),{7200,-2200,0},{8000,-1400,0}));
 TestTrue(TEXT("Adjacent room shares a partition"),S->OfficeRectangle(TEXT("OfficeRoom"),{7200,-1400,0},{8000,-600,0}));
 TestEqual(TEXT("Adjacent rooms have a single partition, not doubled walls"),WallsAt({7500,-1400,120}),1);
 S->Undo();S->Undo();

 TestEqual(TEXT("Open office conversion stays one host record"),Current().OfficeAreas.Num(),1);
 int32 Doors=0,Windows=0;for(const auto& R:Current().OfficeRooms) for(const auto& O:R.Openings) {Doors+=O.Kind==EFoundrylineOpeningKind::OfficeDoor;Windows+=O.Kind==EFoundrylineOpeningKind::ShopWindow;}
 TestEqual(TEXT("Each sample room has an office door"),Doors,3);TestEqual(TEXT("Office windows are manual only, including showcase"),Windows,0);
 TestFalse(TEXT("No automatic exterior office windows"),Current().Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::ShopWindow;}));
 TestTrue(TEXT("Manual open-office exterior window"),S->OfficeOpening(TEXT("RoomWindow"),{8000,-1900,0}));S->Undo();
 bool NavChange=false;auto PaintHandle=P->OnPlacementChanged.AddLambda([&](const auto& V){NavChange|=V.bAffectsNavigation;});
 S->SelectedObject=HostId;TestTrue(TEXT("Paint office exterior"),S->PaintSelected(0,FLinearColor::Red));
 TestFalse(TEXT("Painting does not invalidate navigation"),NavChange);P->OnPlacementChanged.Remove(PaintHandle);
 FFoundrylinePlacementRequest Paint;Paint.Action=EFoundrylinePlacementAction::SetBuildingColors;Paint.ObjectId=HostId;Paint.ExpectedRevision=P->GetRevision();Paint.Colors=Current().Colors;
 auto Stranger=Owner;Stranger.CompanyId=FGuid::NewGuid();TestFalse(TEXT("Cannot paint another company building"),P->Execute(Paint,Stranger).IsSuccess());
 Paint.Colors.Exterior.R=2;TestFalse(TEXT("Invalid paint channel rejected"),P->Execute(Paint,Owner).IsSuccess());
 TestTrue(TEXT("Exterior independent of interior"),Current().Colors.Exterior==FLinearColor::Red && Current().Colors.Interior!=FLinearColor::Red);
 TestTrue(TEXT("Paint office interior"),S->PaintSelected(1,FLinearColor::Blue));
 TestTrue(TEXT("Paint office trim"),S->PaintSelected(2,FLinearColor::Green));
 auto Painted=P->CreateSnapshot();TestTrue(TEXT("Snapshot includes separate colors"),Painted->Objects.FindByPredicate([&](const auto& V){return V.ObjectId==HostId;})->Colors.Trim==FLinearColor::Green);
 TArray<uint8> PaintBytes;TestTrue(TEXT("Paint serializes"),UGameplayStatics::SaveGameToMemory(Painted,PaintBytes));
 auto* PaintSave=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(PaintBytes));const auto* PaintRecord=PaintSave?PaintSave->Objects.FindByPredicate([&](const auto& V){return V.ObjectId==HostId;}):nullptr;
 TestTrue(TEXT("Three paint channels survive serialization"),PaintRecord && PaintRecord->Colors.Exterior==FLinearColor::Red && PaintRecord->Colors.Interior==FLinearColor::Blue && PaintRecord->Colors.Trim==FLinearColor::Green);
 S->Undo();S->Undo();S->Undo();TestTrue(TEXT("Paint undo restores Foundryline defaults"),Current().Colors.Exterior.Equals(FFoundrylineBuildingColors().Exterior));
 const auto Revision=P->GetRevision();S->OfficeRectangle(TEXT("OfficeRoom"),{7200,-2200,0},{7800,-1400,0},true);TestEqual(TEXT("Room hologram does not mutate state"),P->GetRevision(),Revision);
 int32 Ghosts=0;
 for(TActorIterator<AActor> It(W);It;++It) if(It->ActorHasTag(TEXT("FoundrylinePlacementPreview")) && !It->IsActorBeingDestroyed()) {TInlineComponentArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) {++Ghosts;TestTrue(TEXT("Office preview has no collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);for(int32 I=0;I<C->GetNumMaterials();++I) TestTrue(TEXT("Office preview all materials holographic"),C->GetMaterial(I) && C->GetMaterial(I)->GetBlendMode()==BLEND_Translucent);}}
 TestTrue(TEXT("Room ghost rendered"),Ghosts>0);S->HidePreview();
 TestFalse(TEXT("Room cannot leave office area"),S->OfficeRectangle(TEXT("OfficeRoom"),{7600,-1000,0},{8300,-300,0}));
 TestFalse(TEXT("Rooms cannot overlap"),S->OfficeRectangle(TEXT("OfficeRoom"),{5500,-2100,0},{6000,-1600,0}));
 TestTrue(TEXT("Drag a new room"),S->OfficeRectangle(TEXT("OfficeRoom"),{7200,-2200,0},{7800,-1400,0}));
 TestTrue(TEXT("Place actual office door"),S->OfficeOpening(TEXT("RoomDoor"),{7800,-1800,0}));
 FHitResult Hit;TestFalse(TEXT("Door path stays open above floor"),W->LineTraceSingleByChannel(Hit,{7770,-1750,100},{7830,-1750,100},ECC_Visibility));
 TestTrue(TEXT("Window after room creation"),S->OfficeOpening(TEXT("RoomWindow"),{7500,-1400,0}));
 TestFalse(TEXT("Duplicate window blocked"),S->OfficeOpening(TEXT("RoomWindow"),{7500,-1400,0}));
 S->Undo();S->Undo();S->Undo();TestEqual(TEXT("Undo room and openings leaves sample rooms"),Current().OfficeRooms.Num(),3);
 bool Desk=false,Chair=false;
 for(TActorIterator<AActor> It(W);It;++It) if(!It->IsActorBeingDestroyed() && !It->ActorHasTag(TEXT("FoundrylinePlacementPreview"))) {TInlineComponentArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) if(C->GetStaticMesh()) {const auto Name=C->GetStaticMesh()->GetName();if(Name==TEXT("SM_Table_01") || Name==TEXT("SM_Chairs_01")) {Desk|=Name==TEXT("SM_Table_01");Chair|=Name==TEXT("SM_Chairs_01");TestTrue(TEXT("Office furniture stays native 1:1 scale"),C->GetComponentScale().Equals(FVector::OneVector,.001));}}}
 TestTrue(TEXT("Sample furnished with real desk and chair for space review"),Desk && Chair);
 bool OfficeLeaf=false,ShopLeaf=false;
 for(TActorIterator<AActor> It(W);It;++It) if(!It->IsActorBeingDestroyed() && !It->ActorHasTag(TEXT("FoundrylinePlacementPreview"))) {TInlineComponentArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) if(C->GetStaticMesh()) {const auto Name=C->GetStaticMesh()->GetName();if(Name==TEXT("SM_Door_B2_Right") || Name==TEXT("SM_Door_A1")) {OfficeLeaf|=Name==TEXT("SM_Door_B2_Right");ShopLeaf|=Name==TEXT("SM_Door_A1");TestTrue(TEXT("Vendor door geometry retained at native 1:1 scale"),C->GetComponentScale().Equals(FVector::OneVector,.001));}}}
 TestTrue(TEXT("Real factory office and shop door meshes visible"),OfficeLeaf && ShopLeaf);
 TArray<uint8> Bytes;TestTrue(TEXT("Office data serializes"),UGameplayStatics::SaveGameToMemory(P->CreateSnapshot(),Bytes));auto* Save=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(Bytes));const auto* Saved=Save?Save->Objects.FindByPredicate([&](const auto& O){return O.ObjectId==HostId;}):nullptr;
 TestTrue(TEXT("Rooms and openings survive serialization"),Saved && Saved->OfficeRooms.Num()==3 && Saved->OfficeRooms[0].Openings.Num()==1);
 W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineSimpleConnectionsTest,"Foundryline.Construction.ExtendResizeOfficesAndRoadEnds",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineSimpleConnectionsTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* S=W->SpawnActor<AFoundrylineBuildSite>();S->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();
 auto Size=[&](const FFoundrylinePlacedObject& O){const auto* D=P->FindDefinition(O.DefinitionId);return D?FIntPoint(D->Footprint.Max.X,D->Footprint.Max.Y):FIntPoint::ZeroValue;};
 // The starter shop spans 400..2800 x -1200..400. Start inside it and pull out of the south wall.
 TestTrue(TEXT("Drag out of a building extends it"),S->BuildRectangle(TEXT("Building"),{1600,-400,0},{1700,-2400,0}));
 const auto Extension=P->GetObjects().Last();
 TestTrue(TEXT("Extension spans the whole south wall"),Extension.Anchor==FIntVector(400,-2400,0) && Size(Extension)==FIntPoint(2400,1200));
 TestTrue(TEXT("Extension reports the shared wall"),S->Status.Contains(TEXT("expanded")));
 S->Undo();
 TestFalse(TEXT("A drag that stays inside is refused"),S->BuildRectangle(TEXT("Building"),{1200,-400,0},{2000,0,0}));

 // Grabbing the middle of a wall moves only that wall; doors keep their place in the world.
 TestTrue(TEXT("Section for wall resize"),S->BuildRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0}));
 const auto Id=P->GetObjects().Last().ObjectId;
 auto Current=[&]()->const FFoundrylinePlacedObject& {return *P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Id;});};
 TestTrue(TEXT("North wall door"),S->PlaceDoor(TEXT("Personnel"),{3800,2000,0}));
 const auto* Door=Current().Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::Personnel;});
 if(!TestNotNull(TEXT("Door recorded"),Door)) {W->DestroyWorld(false);return false;}
 const auto DoorId=Door->OpeningId;const int32 DoorX=Current().Anchor.X+Door->OffsetCm;
 TestTrue(TEXT("Select west wall"),S->BeginEdit(TEXT("Resize"),{3200,1500,0}));
 TestTrue(TEXT("Pull west wall out"),S->CommitEdit(TEXT("Resize"),{3200,1500,0},{3000,1700,0}));
 TestTrue(TEXT("Only the west wall moved"),Current().Anchor==FIntVector(3000,1000,0) && Size(Current())==FIntPoint(1400,1000));
 const auto* Kept=Current().Openings.FindByPredicate([&](const auto& V){return V.OpeningId==DoorId;});
 TestTrue(TEXT("Door stays at the same world position"),Kept && Current().Anchor.X+Kept->OffsetCm==DoorX);
 S->Undo();TestTrue(TEXT("Undo restores the wall"),Current().Anchor==FIntVector(3200,1000,0));

 // Offices joined to offices share one open floor; each still gets an outside door.
 TestTrue(TEXT("First office"),S->BuildRectangle(TEXT("Office"),{3200,3600,0},{4200,4600,0}));
 const auto FirstId=P->GetObjects().Last().ObjectId;
 TestTrue(TEXT("First office starts with its door on the south wall"),P->GetObjects().Last().Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==0;}));
 TestTrue(TEXT("Second office drawn off its south wall"),S->BuildRectangle(TEXT("Office"),{3700,4000,0},{3800,2600,0}));
 const auto Second=P->GetObjects().Last();
 TestTrue(TEXT("Joined office spans the whole wall"),Second.bOfficeAnnex && Second.Anchor==FIntVector(3200,2600,0) && Size(Second)==FIntPoint(1000,1000));
 TestTrue(TEXT("Joined office keeps an outside entrance"),Second.Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge!=2;}));
 const auto* First=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==FirstId;});
 TestTrue(TEXT("First office gets a new outside door when its old one now opens into the second"),First && First->Openings.ContainsByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge!=0;}));
 FHitResult Hit;
 TestFalse(TEXT("Office to office wall is open"),W->LineTraceSingleByChannel(Hit,{3350,3550,120},{3350,3650,120},ECC_Visibility));
 S->Undo();S->Undo();

 // Road ends reach the wall they nearly touch.
 TestTrue(TEXT("Road toward a building"),S->BuildRectangle(TEXT("Service"),{1600,-3500,0},{1600,-1400,0}));
 const auto Road=P->GetObjects().Last();
 TestTrue(TEXT("Road end snaps flush to the wall"),Road.Anchor.Y+Size(Road).Y==-1200);
 W->DestroyWorld(false);return true;
}
#endif
