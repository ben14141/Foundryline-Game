#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../UI/FoundrylineOperationFeedback.h"
#include "UI/FoundrylinePlayerController.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSteelPurchaseReceipt,"Foundryline.UI.Feedback.SteelReceiptUsesCommittedPurchase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSteelPurchaseReceipt::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Commerce.Clock=1500;const int64 Cash=M.State.Commerce.CashCents;FString Error;FFoundrylineCommerceCommand C;C.Action=EFoundrylineCommerceAction::BuySteel;C.Quantity=2;C.Arrival=0;
 if(!TestTrue(TEXT("Commit actual steel purchase"),M.CommerceCommand(C,Error))){AddError(Error);return false;}
 const auto& Booking=M.State.Commerce.Bookings.Last();const FString Receipt=FoundrylineOperationFeedback::CommerceReceipt(C,M.State);
 TestTrue(TEXT("Receipt contains actual sheet count"),Receipt.Contains(TEXT("2 steel sheets")));TestTrue(TEXT("Receipt contains actual material plus freight charge"),Receipt.Contains(FFoundrylineCommerceCatalog::Money(Cash-M.State.Commerce.CashCents)));TestTrue(TEXT("Default arrival comes from the committed booking, not request zero"),Receipt.Contains(FFoundrylineCommerceCatalog::Date(Booking.ArrivalAt)));TestTrue(TEXT("Receipt points to Trucks for tracking"),Receipt.Contains(TEXT("Trucks")));TestTrue(TEXT("Default arrival follows the current business clock rather than request zero"),Booking.ArrivalAt>1500);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoredEquipmentReceipt,"Foundryline.UI.Feedback.PurchasedEquipmentReceiptsDescribeOwnedStorage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoredEquipmentReceipt::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Commerce.CashCents=2000000;FString Error;FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::BuyStoredMachine;C.Id=1;const int64 Cash=M.State.Commerce.CashCents;
 if(!TestTrue(TEXT("Buy an owned stored plasma"),M.BuyStoredMachine(1,Error))){AddError(Error);return false;}
 auto Receipt=FoundrylineOperationFeedback::FactoryReceipt(C,M.State);TestTrue(TEXT("Purchase has a receipt"),FoundrylineOperationFeedback::FactoryHasReceipt(C));TestTrue(TEXT("Receipt gives actual equipment charge"),Receipt.Contains(FFoundrylineCommerceCatalog::Money(Cash-M.State.Commerce.CashCents)));TestTrue(TEXT("Receipt describes later placement"),Receipt.Contains(TEXT("waiting to be placed"))&&Receipt.Contains(TEXT("Planning")));
 C.Id=4;const int Drivers=M.State.Drivers.Num();TestTrue(TEXT("Buy an owned forklift token"),M.BuyStoredMachine(4,Error));Receipt=FoundrylineOperationFeedback::FactoryReceipt(C,M.State);TestTrue(TEXT("Forklift receipt describes owned unplaced equipment"),Receipt.Contains(TEXT("forklift"))&&Receipt.Contains(TEXT("1 owned")));TestEqual(TEXT("Purchase receipt does not imply a spawned driver"),M.State.Drivers.Num(),Drivers);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOperationReplyFeedback,"Foundryline.UI.Feedback.RejectionsAndStaleRepliesNeverConfirm",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOperationReplyFeedback::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* P=World->SpawnActor<AFoundrylinePlayerController>();if(!TestNotNull(TEXT("Local feedback fixture"),P)){World->DestroyWorld(false);return false;}
 P->OperationsSequence=7;P->bOperationsPending=true;P->OperationsPendingConfirm=TEXT("Client prediction must not be used");const FString Receipt=TEXT("Ordered 2 steel sheets. Total $536.00. Arrival: Day 2 02:00.");P->ClientOperationsResult_Implementation(7,true,Receipt);TestEqual(TEXT("Successful response shows authoritative server receipt"),P->OperationsConfirm,Receipt);TestFalse(TEXT("Successful reply clears pending state"),P->bOperationsPending);TestTrue(TEXT("Success is distinct from an error"),P->OperationsStatus.IsEmpty());
 P->ClientOperationsResult_Implementation(6,true,TEXT("Stale success"));TestEqual(TEXT("Stale success cannot overwrite current receipt"),P->OperationsConfirm,Receipt);
 P->OperationsSequence=8;P->OperationsConfirm.Reset();P->bOperationsPending=true;P->ClientOperationsResult_Implementation(8,false,TEXT("Insufficient available cash"));TestTrue(TEXT("Rejected request never confirms a purchase"),P->OperationsConfirm.IsEmpty());TestEqual(TEXT("Failure retains actionable server explanation"),P->OperationsStatus,FString(TEXT("Insufficient available cash")));TestFalse(TEXT("Rejected request is no longer pending"),P->bOperationsPending);
 P->ClientFactoryReply_Implementation(TEXT("FL_OK|Purchased forklift. Open Planning to place it."));TestTrue(TEXT("Factory success protocol becomes a visible receipt"),P->OperationsConfirm.Contains(TEXT("Purchased forklift")));TestTrue(TEXT("Factory success is not displayed as an error"),P->FactoryStatus.IsEmpty());P->OperationsConfirm.Reset();P->ClientFactoryReply_Implementation(TEXT("No space for that machine"));TestTrue(TEXT("Rejected equipment action never confirms a purchase"),P->OperationsConfirm.IsEmpty());TestEqual(TEXT("Factory rejection has a distinct error message"),P->OperationsStatus,FString(TEXT("No space for that machine")));World->DestroyWorld(false);return true;
}
#endif
