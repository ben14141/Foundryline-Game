#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
namespace OfficeTests {
struct Fixture {
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);
 AFoundrylineBuildSite* Site=W->SpawnActor<AFoundrylineBuildSite>();
 FFoundrylineFactorySimulation Model;
 Fixture(){Site->DispatchBeginPlay();Site->OfficeRectangle(TEXT("OfficeArea"),{3200,1000,0},{4400,2200,0});Site->OfficeRectangle(TEXT("OfficeArea"),{4500,1000,0},{5700,2200,0});Model.State.bCutPartsStarter=true;Model.State.FloorMin={-2400,0};Model.State.FloorMax={6000,2400};Model.SeedCommerce();}
 ~Fixture(){W->DestroyWorld(false);}
 TArray<FFoundrylineOfficeRoomSpec> Rooms(){return FFoundrylineOffices::Rooms(W,Model.State,FTransform::Identity).FilterByPredicate([](const auto& R){return R.Frame.GetLocation().X>=3200;});}
 bool Assign(const FFoundrylineOfficeRoomSpec& R,int Person,FString& E){return FFoundrylineOffices::Assign(W,Model,FTransform::Identity,R.BuildingId,R.RoomId,R.Floor,Person,E);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficePresidentAssign,"Foundryline.Offices.PresidentFreeFurnishingAndOccupancy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficePresidentAssign::RunTest(const FString&){OfficeTests::Fixture F;FString E;const auto Rooms=F.Rooms();if(!TestTrue(TEXT("Two real owned offices available"),Rooms.Num()>=2))return false;F.Model.State.Commerce.CashCents=0;TestTrue(TEXT("President office furnishes with zero test cash: ")+E,F.Assign(Rooms[0],0,E));if(!TestEqual(TEXT("Exactly one office"),F.Model.State.OfficeAssignments.Num(),1))return false;const auto& A=F.Model.State.OfficeAssignments[0];TestTrue(TEXT("Computer workstation created"),A.DeskId>0&&F.Model.State.OfficeDesks.Num()==1);TestEqual(TEXT("Free testing furniture does not spend cash"),F.Model.State.Commerce.CashCents,int64(0));TestTrue(TEXT("Executive package includes display furniture"),A.Decorations.ContainsByPredicate([](const auto& P){return P.Kind==1;}));TestTrue(TEXT("President desk accessories"),A.Decorations.ContainsByPredicate([](const auto& P){return P.Kind==6;}));const int Next=F.Model.State.NextId;TestFalse(TEXT("Repeated assignment never duplicates furnishing"),F.Assign(Rooms[0],0,E));TestEqual(TEXT("Rejected repeat preserves identity sequence"),F.Model.State.NextId,Next);TestTrue(TEXT("State remains valid"),F.Model.Valid(E));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeEngineerAssign,"Foundryline.Offices.EngineerHiringAndIndependentRoom",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeEngineerAssign::RunTest(const FString&){OfficeTests::Fixture F;FString E;const auto Rooms=F.Rooms();if(Rooms.Num()<2)return false;const FGuid Request=FGuid::NewGuid();TestTrue(TEXT("Hire an engineering employee"),F.Model.HireEmployee(EFoundrylineEmployeeRole::Engineer,Request,E));if(!TestEqual(TEXT("One engineering record"),F.Model.State.OfficeEmployees.Num(),1))return false;const int Person=F.Model.State.OfficeEmployees[0].Id;auto Roster=FFoundrylineEmployeeRoster::Build(F.Model.State);const auto* Before=Roster.FindByPredicate([&](const auto& P){return P.Id==Person;});TestTrue(TEXT("Unassigned engineer requires an office"),Before&&Before->Role==EFoundrylineEmployeeRole::Engineer&&!Before->Assignment&&!Before->bSeated);TestFalse(TEXT("Hiring request deduplicated"),F.Model.HireEmployee(EFoundrylineEmployeeRole::Engineer,Request,E));TestTrue(TEXT("President assignment"),F.Assign(Rooms[0],0,E));TestFalse(TEXT("Engineer cannot take occupied president office"),F.Assign(Rooms[0],Person,E));TestTrue(TEXT("Engineer has separate office"),F.Assign(Rooms[1],Person,E));const auto* A=F.Model.State.OfficeAssignments.FindByPredicate([&](const auto& A){return A.EmployeeId==Person;});TestTrue(TEXT("Engineer blueprint board exists"),A&&A->Decorations.ContainsByPredicate([](const auto& D){return D.Kind==9;}));TestTrue(TEXT("Engineer technical desktop exists"),A&&A->Decorations.ContainsByPredicate([](const auto& D){return D.Kind==7;}));Roster=FFoundrylineEmployeeRoster::Build(F.Model.State);const auto* After=Roster.FindByPredicate([&](const auto& P){return P.Id==Person;});TestTrue(TEXT("Engineer seated at their actual computer"),After&&After->bSeated&&After->Assignment&&After->Position.Z<100);TestTrue(TEXT("Office name is employee name"),A&&A->OccupantName==F.Model.State.OfficeEmployees[0].DisplayName);TestEqual(TEXT("No hidden production operator created"),F.Model.State.Operators.Num(),0);TestTrue(TEXT("Engineering release removes role decoration"),FFoundrylineOffices::Release(F.Model,Person,E));TestEqual(TEXT("Released computer retained"),F.Model.State.OfficeDesks.Num(),2);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeOwnershipSave,"Foundryline.Offices.OwnershipSaveAndInvalidRecords",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeOwnershipSave::RunTest(const FString&){OfficeTests::Fixture F;FString E;const auto Rooms=F.Rooms();if(Rooms.IsEmpty())return false;auto Foreign=F.Model;Foreign.State.CompanyId=FGuid::NewGuid();Foreign.State.PlantId=FGuid::NewGuid();TestFalse(TEXT("Neighbour cannot claim another company office"),FFoundrylineOffices::Assign(F.W,Foreign,FTransform::Identity,Rooms[0].BuildingId,Rooms[0].RoomId,0,0,E));TestTrue(TEXT("Own president assigned"),F.Assign(Rooms[0],0,E));auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=F.Model.State;TArray<uint8> Bytes;TestTrue(TEXT("Office save serialization"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Office save deserializes"),Loaded))return false;TestEqual(TEXT("Office assignment persists"),Loaded->Factory.OfficeAssignments.Num(),1);TestEqual(TEXT("President name persists"),Loaded->Factory.OfficeAssignments[0].OccupantName,F.Model.State.OfficeAssignments[0].OccupantName);auto Bad=F.Model;const auto Duplicate=Bad.State.OfficeAssignments[0];Bad.State.OfficeAssignments.Add(Duplicate);TestFalse(TEXT("Duplicate office/occupant rejected"),Bad.Valid(E));Bad=F.Model;Bad.State.OfficeDesks.Reset();TestFalse(TEXT("Assigned computer cannot disappear"),Bad.Valid(E));TestTrue(TEXT("World property and assignment agree"),FFoundrylineOffices::ValidateProperty(F.Model.State,F.W->GetSubsystem<UFoundrylinePlacementSubsystem>()->GetObjects(),F.W->GetSubsystem<UFoundrylinePlacementSubsystem>(),FTransform::Identity,E));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeProtect,"Foundryline.Offices.AssignedRoomAndDeskProtection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeProtect::RunTest(const FString&){OfficeTests::Fixture F;FString E;const auto Rooms=F.Rooms();if(Rooms.IsEmpty())return false;TestTrue(TEXT("Assign office"),F.Assign(Rooms[0],0,E));auto* Plant=F.W->SpawnActor<AFoundrylineFactoryPlant>();Plant->Model=F.Model;Plant->Snapshot=F.Model.State;auto* Placement=F.W->GetSubsystem<UFoundrylinePlacementSubsystem>();auto O=*Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Rooms[0].BuildingId;});O.Anchor.X+=100;TestFalse(TEXT("Occupied office building cannot move away from furniture"),FFoundrylineOffices::ProtectConstruction(F.W,O,E));O.DefinitionId=NAME_None;TestFalse(TEXT("Assigned office cannot be deleted"),FFoundrylineOffices::ProtectConstruction(F.W,O,E));TestTrue(TEXT("Assigned workstation identified for edit guard"),FFoundrylineOffices::DeskInUse(F.Model.State,F.Model.State.OfficeDesks[0].Id));TestTrue(TEXT("Release president office"),FFoundrylineOffices::Release(F.Model,0,E));Plant->Model=F.Model;TestTrue(TEXT("Unassignment releases construction guard"),FFoundrylineOffices::ProtectConstruction(F.W,O,E));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeInteriorGlass,"Foundryline.Offices.ManualInteriorGlassAndReversibleDecor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeInteriorGlass::RunTest(const FString&){
 OfficeTests::Fixture F;FString E;
 TestTrue(TEXT("Build an actual enclosed office room"),F.Site->OfficeRectangle(TEXT("OfficeRoom"),{3200,1000,0},{3800,1600,0}));
 TestTrue(TEXT("Room access door"),F.Site->OfficeOpening(TEXT("RoomDoor"),{3800,1200,0}));
 auto Rooms=F.Rooms();const auto* Room=Rooms.FindByPredicate([](const auto& R){return R.RoomId.IsValid();});if(!TestNotNull(TEXT("Room exists"),Room))return false;
 TestTrue(TEXT("Assign furnished president room"),F.Assign(*Room,0,E));
 // Fixture artwork on the north partition, deliberately covering the test span.
 FFoundrylineOfficeDecoration Art;Art.Kind=8;Art.Position={3500,1587,130};Art.Quarter=2;F.Model.State.OfficeAssignments[0].Decorations.Add(Art);
 TestTrue(TEXT("Place full-height manual glass on interior wall"),F.Site->OfficeOpening(TEXT("OfficeInteriorGlass"),{3500,1600,0}));
 auto* P=F.W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto* O=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Room->BuildingId;});const auto* R=O->OfficeRooms.FindByPredicate([&](const auto& R){return R.SpaceId==Room->RoomId;});const auto* G=R->Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeInteriorGlass;});
 if(TestNotNull(TEXT("Glass persists as opening"),G)){TestEqual(TEXT("True ceiling-height glass"),G->HeightCm(),300);TestEqual(TEXT("No wall underneath"),G->BottomCm(),0);TestTrue(TEXT("Glass is not a door"),G->IsGlazing());}
 FFoundrylineOffices::ReconcileWallDecor(F.W,F.Model,FTransform::Identity);TestTrue(TEXT("Glass clears covered wall art"),F.Model.State.OfficeAssignments[0].Decorations.Last().HiddenByGlazing);
 F.Site->Undo();FFoundrylineOffices::ReconcileWallDecor(F.W,F.Model,FTransform::Identity);TestFalse(TEXT("Undo restores covered art"),F.Model.State.OfficeAssignments[0].Decorations.Last().HiddenByGlazing);
 TestFalse(TEXT("Interior tool cannot replace exterior walls"),F.Site->OfficeOpening(TEXT("OfficeInteriorGlass"),{3200,1400,0}));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeUpstairsAssignment,"Foundryline.Offices.UpstairsAndSharedGlassEditing",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeUpstairsAssignment::RunTest(const FString&){
 OfficeTests::Fixture F;FString E;
 TestTrue(TEXT("Stairs create second floor"),F.Site->PlaceOfficeStairs({3250,1050,0},0,false));
 TestTrue(TEXT("Upstairs room fits clear of stairs"),F.Site->OfficeRectangle(TEXT("OfficeRoom"),{3800,1000,320},{4400,1600,320}));
 TestTrue(TEXT("Upstairs door"),F.Site->OfficeOpening(TEXT("RoomDoor"),{3800,1300,320}));
 auto Rooms=F.Rooms();const auto* Upper=Rooms.FindByPredicate([](const auto& R){return R.Floor==1&&R.RoomId.IsValid();});if(!TestNotNull(TEXT("Upstairs office available"),Upper))return false;
 if(!TestTrue(TEXT("President assigns upstairs"),F.Assign(*Upper,0,E)))return false;
 TestEqual(TEXT("Desk remains on upstairs floor"),F.Model.State.OfficeAssignments[0].DeskPosition.Z,340.);
 TestTrue(TEXT("Upper glass wall manual"),F.Site->OfficeOpening(TEXT("OfficeInteriorGlass"),{4100,1600,320}));
 // A separate ground-floor pair exercises edit/remove on a shared partition.
 TestTrue(TEXT("Ground room A"),F.Site->OfficeRectangle(TEXT("OfficeRoom"),{4600,1200,0},{5100,1800,0}));
 TestTrue(TEXT("Ground room B"),F.Site->OfficeRectangle(TEXT("OfficeRoom"),{5100,1200,0},{5700,1800,0}));
 TestTrue(TEXT("Shared-wall glass"),F.Site->OfficeOpening(TEXT("OfficeInteriorGlass"),{5100,1400,0}));
 auto Count=[&](){int N=0;for(const auto& O:F.W->GetSubsystem<UFoundrylinePlacementSubsystem>()->GetObjects())for(const auto& R:O.OfficeRooms)for(const auto& V:R.Openings)if(V.Kind==EFoundrylineOpeningKind::OfficeInteriorGlass&&R.OfficeLevel==0)++N;return N;};
 TestEqual(TEXT("Shared panel has paired records"),Count(),2);
 TestTrue(TEXT("Select shared panel"),F.Site->BeginEdit(TEXT("Select"),{5100,1400,0}));
 TestTrue(TEXT("Remove shared panel"),F.Site->CommitEdit(TEXT("Demolish"),{5100,1400,0},{5100,1400,0}));
 TestEqual(TEXT("Both sides removed together"),Count(),0);F.Site->Undo();TestEqual(TEXT("Undo restores both sides"),Count(),2);
 TestTrue(TEXT("Click near adjoining corner targets the actual wall segment"),F.Site->OfficeOpening(TEXT("OfficeInteriorGlass"),{5200,1200,0}));bool CorrectRoom=false;for(const auto& O:F.W->GetSubsystem<UFoundrylinePlacementSubsystem>()->GetObjects())for(const auto& R:O.OfficeRooms)if(R.OfficeLevel==0&&O.Anchor.X+R.Min.X<=5200&&O.Anchor.X+R.Max.X>=5200&&O.Anchor.Y+R.Min.Y==1200)CorrectRoom|=R.Openings.ContainsByPredicate([](const auto& G){return G.Kind==EFoundrylineOpeningKind::OfficeInteriorGlass&&G.Edge==0;});TestTrue(TEXT("Neighbor wall extension cannot steal glass placement"),CorrectRoom);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeWallDecorUpgrade,"Foundryline.Offices.WallDecorUpgradePreservesOffice",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeWallDecorUpgrade::RunTest(const FString&){
 OfficeTests::Fixture F;FString E;const auto Rooms=F.Rooms();if(Rooms.IsEmpty()||!F.Assign(Rooms[0],0,E))return false;
 auto& A=F.Model.State.OfficeAssignments[0];const auto Id=A.Id;const auto Desk=A.DeskId;const auto Name=A.OccupantName;const auto Cash=F.Model.State.Commerce.CashCents;
 A.Decorations.RemoveAll([](const auto& D){return D.Kind>=12;});A.FurnishingVersion=1;
 FFoundrylineOffices::ReconcileWallDecor(F.W,F.Model,FTransform::Identity);
 TestEqual(TEXT("Existing office upgraded"),A.FurnishingVersion,uint8(2));for(uint8 Kind:{12,13,14})TestTrue(FString::Printf(TEXT("Executive wall decoration %d fitted"),Kind),A.Decorations.ContainsByPredicate([&](const auto& D){return D.Kind==Kind;}));
 TestEqual(TEXT("Same assigned room identity"),A.Id,Id);TestEqual(TEXT("Same computer"),A.DeskId,Desk);TestEqual(TEXT("Same occupant"),A.OccupantName,Name);TestEqual(TEXT("No charge for wall decor upgrade"),F.Model.State.Commerce.CashCents,Cash);
 const int Count=A.Decorations.Num();const auto Revision=F.Model.State.Revision;FFoundrylineOffices::ReconcileWallDecor(F.W,F.Model,FTransform::Identity);TestEqual(TEXT("Repeat reconciliation never duplicates decor"),A.Decorations.Num(),Count);TestEqual(TEXT("No repeated state publication"),F.Model.State.Revision,Revision);TestTrue(TEXT("Upgraded state valid for save"),F.Model.Valid(E));return true;
}
#endif
