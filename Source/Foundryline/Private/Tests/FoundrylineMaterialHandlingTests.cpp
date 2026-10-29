#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineSheetStockTest,"Foundryline.Inventory.ExactSheetStockAndSimplifiedVisuals",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineSheetStockTest::RunTest(const FString&){
 TestTrue(TEXT("60 x 120 inches retains true centimetres"),FFoundrylineSteelSheet::WidthCm==152.4 && FFoundrylineSteelSheet::LengthCm==304.8);
 TestTrue(TEXT("A 1/8 inch sheet weighs about 255 lb in this specification"),FFoundrylineSteelSheet::PoundsEach()>250 && FFoundrylineSteelSheet::PoundsEach()<260);
 TestEqual(TEXT("Ten real sheets render at most three"),FFoundrylineSteelSheet::VisibleIndices(10).Num(),3);
 TestTrue(TEXT("Visual top sheet follows actual stack height"),FFoundrylineSteelSheet::VisibleIndices(10).Last()==9);
 TestEqual(TEXT("Empty stack has no sheet meshes"),FFoundrylineSteelSheet::VisibleIndices(0).Num(),0);
 TestEqual(TEXT("Two sheets show exactly two meshes"),FFoundrylineSteelSheet::VisibleIndices(2).Num(),2);
 FFoundrylineSheetStock Stock;const double Before=Stock.Pounds();TestTrue(TEXT("Consumes actual stock"),Stock.Consume(1));TestEqual(TEXT("Ten becomes nine"),Stock.Count,9);
 TestTrue(TEXT("Mass changes by one sheet"),FMath::Abs(Before-Stock.Pounds()-FFoundrylineSteelSheet::PoundsEach())<.0001);
 TestFalse(TEXT("Cannot consume more sheets than exist"),Stock.Consume(10));TestFalse(TEXT("Negative consumption cannot manufacture stock"),Stock.Consume(-2));TestEqual(TEXT("Rejected operation preserves count"),Stock.Count,9);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineMaterialCellTest,"Foundryline.Logistics.ForkliftRackAndRollerLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineMaterialCellTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* C=W->SpawnActor<AFoundrylineMaterialHandlingCell>();C->DispatchBeginPlay();
 auto* P=W->SpawnActor<APlayerController>();P->PlayerState=W->SpawnActor<APlayerState>();auto* Other=W->SpawnActor<APlayerController>();Other->PlayerState=W->SpawnActor<APlayerState>();
 TestEqual(TEXT("Two exact ten-sheet loads"),C->TotalSheets(),20);TestEqual(TEXT("Only three sheets visible in load A"),C->VisibleSheetCount(0),3);TestEqual(TEXT("Only three sheets visible in rack load B"),C->VisibleSheetCount(1),3);
 const auto Id=C->Stock[0].Id;TestTrue(TEXT("Driver claim accepted"),C->Claim(P,true));TestFalse(TEXT("Other player cannot steal forklift"),C->Claim(Other,true));
 const auto Initial=C->Pose.Position;C->Drive(Other,1,1,1);for(int32 I=0;I<20;++I)C->Step(1.f/30);TestTrue(TEXT("Foreign input cannot move forklift"),C->Pose.Position.Equals(Initial));
 TestFalse(TEXT("Distant load cannot be picked"),C->PickNearest());
 C->Pose.Position={86,0,0};C->Pose.Yaw=180;TestTrue(TEXT("Nearby floor load can be picked"),C->Command(P,1));for(int32 I=0;I<30;++I)C->Step(1.f/30);
 TestEqual(TEXT("Load A is on forklift"),C->Carried,0);TestEqual(TEXT("Picking preserves inventory"),C->TotalSheets(),20);TestTrue(TEXT("Stock ID survives pickup"),C->Stock[0].Id==Id);
 C->Pose.Position={-414,400,0};C->Pose.Yaw=180;C->Pose.Speed=0;
 TestFalse(TEXT("Occupied middle rack level rejects overlapping load"),C->PlaceCarried(EFoundrylineSheetPlace::Rack,1));
 TestTrue(TEXT("Free top level accepts transfer"),C->PlaceCarried(EFoundrylineSheetPlace::Rack,2));for(int32 I=0;I<150;++I)C->Step(1.f/30);
 TestTrue(TEXT("Load stored at top level"),C->Stock[0].Place==EFoundrylineSheetPlace::Rack && C->Stock[0].RackLevel==2 && C->Carried==INDEX_NONE);
 TestEqual(TEXT("Rack storage preserves sheet count"),C->TotalSheets(),20);
 TestTrue(TEXT("Stored load can be retrieved"),C->PickNearest());for(int32 I=0;I<60;++I)C->Step(1.f/30);TestEqual(TEXT("Retrieval restores carried load"),C->Carried,0);
 C->Pose.Position={-66,-550,0};C->Pose.Yaw=0;C->Pose.Speed=0;
 TestTrue(TEXT("Can place at roller input"),C->PlaceCarried(EFoundrylineSheetPlace::RollerInput));for(int32 I=0;I<150;++I)C->Step(1.f/30);
 TestTrue(TEXT("Roller input position persists"),C->Stock[0].Place==EFoundrylineSheetPlace::RollerInput && C->Carried==INDEX_NONE);
 TestTrue(TEXT("One sheet starts forming"),C->StartRoller());TestEqual(TEXT("Exactly one raw sheet reserved"),C->TotalSheets(),19);TestEqual(TEXT("Nine sheets still use only three visuals"),C->VisibleSheetCount(0),3);
 TestFalse(TEXT("Cannot start duplicate work while roller busy"),C->StartRoller());for(int32 I=0;I<190;++I)C->Step(1.f/30);
 TestEqual(TEXT("One curved blank produced"),C->RolledBlanks,1);TestEqual(TEXT("Material quantity conserved"),C->TotalSheets()+C->RolledBlanks,20);
 C->Stock[0].Count=2;C->OnRep_Stock();TestEqual(TEXT("Two remaining sheets show two visuals"),C->VisibleSheetCount(0),2);
 TestTrue(TEXT("Driver can release forklift"),C->Claim(P,false));W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineReceivingRackTest,"Foundryline.Logistics.RelocatedReceivingRack",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineReceivingRackTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* C=W->SpawnActor<AFoundrylineMaterialHandlingCell>();C->RackOffsetCm=FVector(2900,-1450,0);C->ArrivalPositionCm=FVector(1600,-1050,0);C->SetActorLocation(FVector(-1700,2550,20));C->DispatchBeginPlay();
 TestTrue(TEXT("Raw steel rack is near the loading bays"),C->GetRackWorldPosition(1).Equals(FVector(650,1500,190),.01));
 TestTrue(TEXT("Arrival stock is in receiving zone"),C->Stock[0].FloorPosition.Equals(C->ArrivalPositionCm));
 TestEqual(TEXT("Moving the rack preserves real stock quantities"),C->TotalSheets(),20);
 C->Pose.Position=C->GetRackPosition(0)+FVector(136,0,0);C->Pose.Yaw=180;C->Pose.Lift=170;
 auto* P=W->SpawnActor<APlayerController>();P->PlayerState=W->SpawnActor<APlayerState>();TestTrue(TEXT("Driver can claim receiving forklift"),C->Claim(P,true));
 TestTrue(TEXT("Forklift can collect a load from relocated rack"),C->Command(P,1));
 for(int I=0;I<150;++I)C->Step(1.f/30);
 TestEqual(TEXT("Racked stock was collected"),C->Carried,1);TestEqual(TEXT("Pickup does not create steel"),C->TotalSheets(),20);
 W->DestroyWorld(false);return true;
}
#endif
