#pragma once
#include "CoreMinimal.h"
#include "Orders/FoundrylineCommerceTypes.h"
class UWorld;

enum class EFoundrylineERPDocumentKind:uint8 {SalesOrder,PurchaseOrder,GoodsReceipt,CarrierBooking,CustomerPayment,LedgerEntry};
struct FFoundrylineERPDocumentLine {FString Label,Value;};
struct FFoundrylineERPDocument {
 EFoundrylineERPDocumentKind Kind=EFoundrylineERPDocumentKind::LedgerEntry;
 FString Key,Reference,Title,Party,Status,AmountLabel;
 int32 OrderId=0,BookingId=0,LedgerIndex=INDEX_NONE;
 bool bHasRecordedAt=false,bHasAmount=false;
 double RecordedAt=0;
 int64 AmountCents=0;
 TArray<FFoundrylineERPDocumentLine> Lines;
};
struct FFoundrylineERPActivity {
 FString Key,Type,Caption,Detail,Reference,Party;
 double At=0;
 bool bHasAmount=false;
 int64 AmountCents=0;
 int32 OrderId=0,BookingId=0;
};
struct FFoundrylineERPLedgerEntry {
 // Relative to the retained journal; this is not a persistent invoice ID.
 int32 SourceIndex=INDEX_NONE;
 double At=0;
 FString Label;
 int64 MovementCents=0,NetAfterCents=0;
};
// Bins group retained journal records, not invented hourly/daily observations.
struct FFoundrylineERPGraphBin {
 double FromAt=0,ToAt=0;
 int32 EntryCount=0;
 int64 CreditsCents=0,DebitsCents=0,OpeningNetCents=0,ClosingNetCents=0;
};
struct FFoundrylineERPSummary {
 int64 CashCents=0,OutstandingCostsCents=0,EarnedRevenueCents=0,TotalSpendingCents=0;
 int64 UnfulfilledSalesCents=0,DeliveredUnpaidCents=0,CustomerAdvancesCents=0;
 int32 ActiveOrders=0,CompletedInHistory=0,AvailableOffers=0,OpenBookings=0;
};
struct FFoundrylineERPWorkspace {
 bool bAvailable=false;
 FString CompanyName,GraphCaption;
 FFoundrylineERPSummary Summary;
 int64 OpeningRetainedNetCents=0;
 TArray<FFoundrylineERPDocument> Documents;
 TArray<FFoundrylineERPActivity> Activity;
 TArray<FFoundrylineERPLedgerEntry> Ledger;
 TArray<FFoundrylineERPGraphBin> GraphBins;
};
// Starter-loop guidance: accept order -> buy steel -> staff plasma & forklift -> program plasma -> book freight -> get paid.
enum class EFoundrylineGuideStep:uint8 {AcceptOrder,BuySteel,StaffWork,ProgramPlasma,BookFreight,GetPaid,Count};
struct FFoundrylineGuideProblem {FString Text,Action;};
struct FFoundrylineStarterGuide {
 bool bAvailable=false;
 // Focused accepted order (the one sent to the loading bay first, else the oldest accepted).
 int32 OrderId=0,OrderPart=0,OrderRemaining=0;
 FString OrderReference,OrderCustomer,OrderPartName;
 EFoundrylineGuideStep Step=EFoundrylineGuideStep::AcceptOrder;
 bool bStepDone[int(EFoundrylineGuideStep::Count)]={};
 FString Title,Detail,ActionLabel,Action;
 TArray<FFoundrylineGuideProblem> Problems;
 // Focused order breakdown.
 int32 PartsToMake=0,PartsCutting=0,PartsReady=0,PartsAtBay=0,PartsOnTrucks=0,SheetsToBuy=0;
 double SteelNeededLb=0;
 bool bAssignedToBay=false,bOperator=false,bForkliftDriver=false;
 FString OperatorName,DriverName;
 // Whole-business glance (all accepted orders).
 int32 AcceptedOrders=0,OffersWaiting=0,CompletedOrders=0,PartsInProgress=0,FinishedParts=0,FinishedAtBay=0,PickupsBooked=0,SteelDeliveriesDue=0;
 double SteelOnHandLb=0,SteelIncomingLb=0;
 int64 CashCents=0,OwedToYouCents=0,ReceivedCents=0;
};
// Read-only projection of actual retained simulation records. No RPCs or synthetic history.
struct FOUNDRYLINE_API FFoundrylineERPViewModel {
 static FString CompanyName(const UWorld* World,FGuid CompanyId);
 static FFoundrylineERPWorkspace Build(const FFoundrylineCommerceState& State,const FString& CompanyName,int32 MaxGraphBins=8);
 // Pure function of one plant's replicated snapshot; identical for every co-op player viewing that plant.
 static FFoundrylineStarterGuide Guide(const struct FFoundrylineFactoryState& State);
 static const TCHAR* DocumentKindName(EFoundrylineERPDocumentKind Kind);
};
