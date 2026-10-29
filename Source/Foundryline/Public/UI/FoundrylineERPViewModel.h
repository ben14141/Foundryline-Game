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
// Read-only projection of actual retained simulation records. No RPCs or synthetic history.
struct FOUNDRYLINE_API FFoundrylineERPViewModel {
 static FString CompanyName(const UWorld* World,FGuid CompanyId);
 static FFoundrylineERPWorkspace Build(const FFoundrylineCommerceState& State,const FString& CompanyName,int32 MaxGraphBins=8);
 static const TCHAR* DocumentKindName(EFoundrylineERPDocumentKind Kind);
};
