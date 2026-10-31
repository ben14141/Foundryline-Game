#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "Buildings/FoundrylineOfficeStoreyLink.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "SaveSystem/FoundrylinePlacementSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace OfficeStoreyTest {
struct FFixture {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 UFoundrylinePlacementSubsystem* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
 FFoundrylinePlacementContext Context;
 FGuid OfficeId;
 bool Ready=false;
 FFixture(){
  Context.CompanyId=FGuid::NewGuid();Context.PlantId=FGuid::NewGuid();Context.bCanConstruct=true;
  FFoundrylineBuildRegion Region;Region.RegionId=FGuid::NewGuid();Region.CompanyId=Context.CompanyId;Region.PlantId=Context.PlantId;Region.SurfaceId=TEXT("StarterLand");Region.Bounds.Min={-10000,-10000,0};Region.Bounds.Max={10000,10000,5};
  if(Placement->InitializeRegions({Region})!=EFoundrylinePlacementError::None||!Placement->EnsureGeneratedDefinition(TEXT("Office_1200_1200")))return;
  FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::Build;R.ExpectedRevision=Placement->GetRevision();R.DefinitionId=TEXT("Office_1200_1200");R.bOfficeAnnex=true;
  FFoundrylineOfficeSpace Area;Area.SpaceId=FGuid::NewGuid();Area.Max={1200,1200};R.OfficeAreas={Area};
  FFoundrylineOpening Door;Door.OpeningId=FGuid::NewGuid();Door.Edge=0;Door.OffsetCm=500;Door.Kind=EFoundrylineOpeningKind::OfficeDoor;R.Openings={Door};
  const auto Built=Placement->Execute(R,Context);OfficeId=Built.ObjectId;Ready=Built.IsSuccess();
 }
 ~FFixture(){World->DestroyWorld(false);}
 FFoundrylinePlacedObject Office()const {const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==OfficeId;});return O?*O:FFoundrylinePlacedObject();}
 FFoundrylinePlacementRequest Replace(const FFoundrylinePlacedObject& O)const{FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=OfficeId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=O;return R;}
 FFoundrylinePlacedObject TwoFloor(int Q=0)const{auto O=Office();O.bOfficeSecondFloor=true;O.OfficeStairOrigin={600,600};O.OfficeStairQuarter=Q;return O;}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeStoreySchema,"Foundryline.Construction.OfficeStoreys.ValidatedOwnershipAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeStoreySchema::RunTest(const FString&){
 using namespace OfficeStoreyTest;FFixture F;if(!TestTrue(TEXT("Owned single-floor office fixture builds"),F.Ready))return false;
 TestFalse(TEXT("Existing office records default to one floor"),F.Office().bOfficeSecondFloor);
 auto* Legacy=F.Placement->CreateSnapshot();TArray<uint8> LegacyBytes;TestTrue(TEXT("Serialize original one-floor save"),UGameplayStatics::SaveGameToMemory(Legacy,LegacyBytes));auto* LoadedLegacy=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(LegacyBytes));if(TestNotNull(TEXT("Original save loads"),LoadedLegacy))TestFalse(TEXT("Old-style office remains single-floor"),LoadedLegacy->Objects[0].bOfficeSecondFloor);
 for(int Q=0;Q<4;++Q){const auto Before=F.Placement->GetRevision();const auto Valid=F.Placement->Execute(F.Replace(F.TwoFloor(Q)),F.Context);TestTrue(*FString::Printf(TEXT("Quarter %d staircase fits and commits"),Q),Valid.IsSuccess());TestEqual(TEXT("Each genuine shared edit advances revision"),F.Placement->GetRevision(),Before+1);}
 auto Outside=F.TwoFloor();Outside.OfficeStairOrigin={1100,1100};const auto Before=F.Placement->GetRevision();TestFalse(TEXT("Stair outside host walls rejected"),F.Placement->Execute(F.Replace(Outside),F.Context).IsSuccess());TestEqual(TEXT("Rejected stair does not alter property"),F.Placement->GetRevision(),Before);
 auto Invalid=F.TwoFloor();Invalid.OfficeStairQuarter=4;TestFalse(TEXT("Invalid orientation rejected"),F.Placement->Validate(F.Replace(Invalid),F.Context).IsSuccess());Invalid=F.TwoFloor();Invalid.bOfficeAnnex=false;TestFalse(TEXT("Non-office host cannot carry an office storey"),F.Placement->Validate(F.Replace(Invalid),F.Context).IsSuccess());
 auto Foreign=F.Context;Foreign.CompanyId=FGuid::NewGuid();Foreign.PlantId=FGuid::NewGuid();TestEqual(TEXT("Another business cannot modify this stair"),F.Placement->Execute(F.Replace(F.TwoFloor()),Foreign).Error,EFoundrylinePlacementError::AccessDenied);
 auto WithWindows=F.TwoFloor();for(int Level=0;Level<2;++Level){FFoundrylineOpening V;V.OpeningId=FGuid::NewGuid();V.Kind=EFoundrylineOpeningKind::ShopWindow;V.Edge=1;V.OffsetCm=200;V.OfficeLevel=Level;WithWindows.Openings.Add(V);}TestTrue(TEXT("Windows at same XY on different floors are independent"),F.Placement->Execute(F.Replace(WithWindows),F.Context).IsSuccess());
 auto WithRooms=F.Office();for(int Level=0;Level<2;++Level){FFoundrylineOfficeSpace Room;Room.SpaceId=FGuid::NewGuid();Room.Min={100,100};Room.Max={500,500};Room.OfficeLevel=Level;WithRooms.OfficeRooms.Add(Room);}TestTrue(TEXT("Independent rooms can share XY on different floors"),F.Placement->Execute(F.Replace(WithRooms),F.Context).IsSuccess());
 auto BadRoom=F.Office();BadRoom.OfficeRooms[1].Min={500,500};BadRoom.OfficeRooms[1].Max={900,900};TestFalse(TEXT("Upstairs room cannot seal off staircase"),F.Placement->Validate(F.Replace(BadRoom),F.Context).IsSuccess());
 auto BadDoor=F.Office();FFoundrylineOpening Drop;Drop.OpeningId=FGuid::NewGuid();Drop.Kind=EFoundrylineOpeningKind::OfficeDoor;Drop.OfficeLevel=1;Drop.Edge=3;Drop.OffsetCm=200;BadDoor.Openings.Add(Drop);TestFalse(TEXT("Upper exterior door cannot open onto a drop"),F.Placement->Validate(F.Replace(BadDoor),F.Context).IsSuccess());
 auto* Save=F.Placement->CreateSnapshot();TArray<uint8> Bytes;TestTrue(TEXT("Storeys serialize through actual property save"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Two-floor property deserializes"),Loaded))return false;
 TestTrue(TEXT("Saved upper floor flag"),Loaded->Objects[0].bOfficeSecondFloor);TestTrue(TEXT("Saved staircase origin"),Loaded->Objects[0].OfficeStairOrigin==F.Office().OfficeStairOrigin);TestEqual(TEXT("Saved upper room level"),int32(Loaded->Objects[0].OfficeRooms[1].OfficeLevel),1);
 auto* OtherWorld=UWorld::CreateWorld(EWorldType::Game,false);auto* Other=OtherWorld->GetSubsystem<UFoundrylinePlacementSubsystem>();Other->EnsureGeneratedDefinition(TEXT("Office_1200_1200"));const auto Restored=Other->RestoreSnapshot(Loaded);TestEqual(TEXT("Fresh world restores complete property"),Restored,EFoundrylinePlacementError::None);if(!Other->GetObjects().IsValidIndex(0)){OtherWorld->DestroyWorld(false);return false;}TestEqual(TEXT("Restored upstairs windows survive"),Other->GetObjects()[0].Openings.FilterByPredicate([](const auto& V){return V.OfficeLevel==1;}).Num(),1);OtherWorld->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeStoreyDeskCoverage,"Foundryline.Construction.OfficeStoreys.OwnedFloorAndDeskHeights",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeStoreyDeskCoverage::RunTest(const FString&){
 using namespace OfficeStoreyTest;FFixture F;if(!TestTrue(TEXT("Office fixture"),F.Ready))return false;
 FFoundrylineFactorySimulation M;M.State.bCutPartsStarter=true;M.SeedCommerce();M.State.CompanyId=F.Context.CompanyId;M.State.PlantId=F.Context.PlantId;M.State.FloorMin={-2000,-2000};M.State.FloorMax={2000,2000};FString Error;
 const FBox2D Clear({110,260},{290,420});
 auto Before=M.State;TestFalse(TEXT("Single-floor office does not grant floating upstairs furniture"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Before,F.World,FTransform::Identity,Clear,Error,0,340));
 if(!TestTrue(TEXT("Build actual office upper floor"),F.Placement->Execute(F.Replace(F.TwoFloor()),F.Context).IsSuccess()))return false;
 TestTrue(TEXT("Full desk footprint supported upstairs"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(M.State,F.World,FTransform::Identity,Clear,Error,0,340));TestTrue(TEXT("Buy upstairs desk at real340cm floor"),M.BuyPlanningEquipment(7,{200,300,340},0,1,Error));
 TestTrue(TEXT("Ground desk may share the upstairs desk's XY"),M.BuyPlanningEquipment(7,{200,300,20},0,1,Error));TestEqual(TEXT("Both floors retain one desk"),M.State.OfficeDesks.Num(),2);TestFalse(TEXT("Same-floor desk overlap still rejected"),M.BuyPlanningEquipment(7,{200,300,340},0,1,Error));
 auto Hole=M.State;TestFalse(TEXT("Desk cannot float over actual floor hole"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Hole,F.World,FTransform::Identity,FBox2D({610,760},{790,920}),Error,0,340));
 auto Landing=M.State;TestFalse(TEXT("Upper landing remains clear even outside hole"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Landing,F.World,FTransform::Identity,FBox2D({610,610},{790,690}),Error,0,340));
 auto OtherBusiness=M.State;OtherBusiness.CompanyId=FGuid::NewGuid();OtherBusiness.PlantId=FGuid::NewGuid();TestFalse(TEXT("Neighbour cannot use the office floor"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(OtherBusiness,F.World,FTransform::Identity,Clear,Error,0,340));
 TestFalse(TEXT("Unsupported third office level rejected"),FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Hole,F.World,FTransform::Identity,Clear,Error,0,660));TestTrue(TEXT("Two-floor furniture ledger remains valid"),M.Valid(Error));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeStoreyRenderedFloor,"Foundryline.Construction.OfficeStoreys.PhysicalFloorHoleAndCleanup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeStoreyRenderedFloor::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
 if(!TestTrue(TEXT("Create separate office shell"),Site->OfficeRectangle(TEXT("OfficeArea"),{3200,1000,0},{4400,2200,0}))){W->DestroyWorld(false);return false;}
 if(!TestTrue(TEXT("Place staircase and upper floor through public builder"),Site->PlaceOfficeStairs({3800,1600,0},0))){AddInfo(Site->Status);W->DestroyWorld(false);return false;}
 auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto* Office=P->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeSecondFloor;});if(!TestNotNull(TEXT("Saved office has upper floor"),Office)){W->DestroyWorld(false);return false;}
 const FTransform T(FRotator(0,Office->QuarterTurns*90,0),FVector(Office->Anchor));
 auto Trace=[&](FVector XY,double Top,double Bottom){FHitResult Hit;const FVector At=T.TransformPosition(XY);const bool Found=W->LineTraceSingleByChannel(Hit,{At.X,At.Y,Top},{At.X,At.Y,Bottom},ECC_Visibility);TestTrue(TEXT("Authored office has physical floor collision"),Found);return Hit;};
 const auto Upper=Trace({200,200,0},450,300);TestTrue(TEXT("Upper slab and thin finish meet the340cm floor"),FMath::Abs(Upper.ImpactPoint.Z-340)<=2.1);
 const auto Hole=Trace({668,856,0},480,0);TestTrue(*FString::Printf(TEXT("Stair opening reaches lower flight, hit %s at %s"),*GetNameSafe(Hole.GetComponent()),*Hole.ImpactPoint.ToCompactString()),FMath::Abs(Hole.ImpactPoint.Z-100)<.2);
 // Real controlled pawn upstairs prevents every destructive transform route.
 auto* Controller=W->SpawnActor<APlayerController>();auto* Walker=W->SpawnActor<ACharacter>(FVector(3400,1200,430),FRotator::ZeroRotator);Controller->Possess(Walker);
 FString Occupied;for(const auto Action:{EFoundrylinePlacementAction::Move,EFoundrylinePlacementAction::Rotate,EFoundrylinePlacementAction::Remove}){FFoundrylinePlacementRequest Edit;Edit.ObjectId=Office->ObjectId;Edit.Action=Action;Edit.Anchor=Office->Anchor+FIntVector(100,0,0);Edit.QuarterTurns=(Office->QuarterTurns+1)%4;TestFalse(TEXT("Controlled upstairs occupant blocks move, rotate and removal"),Site->OfficeStoreyRequestsClear({Edit},Occupied));TestFalse(TEXT("Occupied-floor block explains the reason"),Occupied.IsEmpty());}
 Controller->UnPossess();Walker->Destroy();Controller->Destroy();
 int Links=0;for(TActorIterator<AActor> It(W);It;++It)if(auto* Link=It->FindComponentByClass<UFoundrylineOfficeStoreyLink>()){++Links;TestTrue(TEXT("Upper office visual is owned for cleanup"),IsValid(Link->UpperActor));}
 TestEqual(TEXT("Exactly one linked upper office"),Links,1);Site->Undo();int Left=0;for(TActorIterator<AActor> It(W);It;++It)if(!It->IsActorBeingDestroyed()&&It->FindComponentByClass<UFoundrylineOfficeStoreyLink>())++Left;TestEqual(TEXT("Undo does not orphan upper visual"),Left,0);W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStraightOfficeSave,"Foundryline.Construction.OfficeStoreys.StraightSelectionAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStraightOfficeSave::RunTest(const FString&){using namespace OfficeStoreyTest;FFixture F;if(!TestTrue(TEXT("Owned office fixture"),F.Ready))return false;
 for(int Q=0;Q<4;++Q){auto O=F.TwoFloor(Q);O.OfficeStairStyle=1;O.OfficeStairOrigin=Q==0?FIntPoint(100,100):Q==1?FIntPoint(1000,100):Q==2?FIntPoint(1000,1000):FIntPoint(100,1000);TestTrue(TEXT("Straight stairs commit in all four rotations"),F.Placement->Execute(F.Replace(O),F.Context).IsSuccess());}
 auto Bad=F.Office();Bad.OfficeStairStyle=2;TestFalse(TEXT("Unknown stair style rejected"),F.Placement->Validate(F.Replace(Bad),F.Context).IsSuccess());
 auto* Save=F.Placement->CreateSnapshot();TArray<uint8> Bytes;TestTrue(TEXT("Save straight stair choice"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Straight save loads"),Loaded))return false;TestEqual(TEXT("Straight choice survives serialization"),int(Loaded->Objects[0].OfficeStairStyle),1);TestEqual(TEXT("Existing default retains turning staircase"),int(FFoundrylinePlacedObject().OfficeStairStyle),0);return true;}
#endif
