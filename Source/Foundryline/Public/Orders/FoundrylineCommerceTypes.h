#pragma once
#include "CoreMinimal.h"
#include "FoundrylineCommerceTypes.generated.h"
UENUM() enum class EFoundrylinePayment:uint8 {PerDelivery,OnCompletion,Upfront};
UENUM() enum class EFoundrylineOrderState:uint8 {Offered,Accepted,Completed};
UENUM() enum class EFoundrylineBookingState:uint8 {Booked,Approaching,Waiting,Loading,Departing,InTransit,Delivered};
UENUM() enum class EFoundrylineForkTask:uint8 {Truck,Machine,Storage};
UENUM() enum class EFoundrylineCommerceAction:uint8 {AcceptOrder,BuySteel,AssignPickup,BookPickup,Reschedule,DriverPriority,BuyBrake,BuyForklift,BuyParts};
USTRUCT() struct FFoundrylineCustomerOrder {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString Customer;
 UPROPERTY(SaveGame) FString Reference;
 UPROPERTY(SaveGame) uint8 Part=1;
 UPROPERTY(SaveGame) int32 Quantity=40;
 UPROPERTY(SaveGame) int32 Delivered=0;
 UPROPERTY(SaveGame) int64 UnitPriceCents=1100;
 UPROPERTY(SaveGame) int64 PaidCents=0;
 UPROPERTY(SaveGame) EFoundrylinePayment Payment=EFoundrylinePayment::OnCompletion;
 UPROPERTY(SaveGame) EFoundrylineOrderState Status=EFoundrylineOrderState::Offered;
 UPROPERTY(SaveGame) bool Email=false;
 UPROPERTY(SaveGame) bool Rush=false;
 UPROPERTY(SaveGame) bool Late=false;
 UPROPERTY(SaveGame) double AllowedMinutes=2880;
 UPROPERTY(SaveGame) double AcceptedAt=0;
 UPROPERTY(SaveGame) double Deadline=0;
};
USTRUCT() struct FFoundrylineFreightItem {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 PalletId=0;
 UPROPERTY(SaveGame) uint8 Part=1;
 UPROPERTY(SaveGame) int32 Quantity=0;
 UPROPERTY(SaveGame) float UnitLb=0;
};
USTRUCT() struct FFoundrylineFreightBooking {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) int32 OrderId=0;
 UPROPERTY(SaveGame) bool Inbound=false;
 // Inbound quantity: steel sheets, or units of Part for a bought-parts delivery.
 UPROPERTY(SaveGame) int32 Sheets=0;
 // Schema 11: inbound part (0 = steel sheet). Bought assembly parts go straight to StationId's IN port.
 UPROPERTY(SaveGame) uint8 Part=0;
 UPROPERTY(SaveGame) int32 StationId=0;
 UPROPERTY(SaveGame) int32 Carrier=1;
 UPROPERTY(SaveGame) int64 OriginalFeeCents=4000;
 UPROPERTY(SaveGame) double ArrivalAt=0;
 UPROPERTY(SaveGame) double DispatchAt=0;
 UPROPERTY(SaveGame) double BerthAt=0;
 UPROPERTY(SaveGame) double DockedAt=0;
 UPROPERTY(SaveGame) double DepartedAt=0;
 UPROPERTY(SaveGame) double DeliveredAt=0;
 UPROPERTY(SaveGame) EFoundrylineBookingState Status=EFoundrylineBookingState::Booked;
 UPROPERTY(SaveGame) bool SpawnedCargo=false;
 UPROPERTY(SaveGame) TArray<FFoundrylineFreightItem> Manifest;
 UPROPERTY(SaveGame) FString Customer;
};
USTRUCT() struct FFoundrylineMoneyEntry {
 GENERATED_BODY()
 UPROPERTY(SaveGame) double At=0;
 UPROPERTY(SaveGame) int64 Cents=0;
 UPROPERTY(SaveGame) FString Label;
};
USTRUCT() struct FFoundrylineCommerceState {
 GENERATED_BODY()
 UPROPERTY(SaveGame) bool Enabled=false;
 UPROPERTY(SaveGame) double Clock=480;
 UPROPERTY(SaveGame) int64 CashCents=400000;
 UPROPERTY(SaveGame) int64 UnpaidCents=0;
 UPROPERTY(SaveGame) int64 EarnedRevenueCents=0;
 UPROPERTY(SaveGame) int64 CostCents=0;
 UPROPERTY(SaveGame) double ExpenseFraction=0;
 // Wages owed but not yet a whole cent and the wages paid so far (FFoundrylinePayroll), and equipment still being
 // installed (game minutes left; times in FFoundrylineEconomyTuning).
 UPROPERTY(SaveGame) double WageFraction=0;
 UPROPERTY(SaveGame) int64 WagesPaidCents=0;
 UPROPERTY(SaveGame) float BrakeInstallMinutes=0;
 UPROPERTY(SaveGame) float ForkliftInstallMinutes=0;
 UPROPERTY(SaveGame) int32 ForkliftInstallDriver=0;
 UPROPERTY(SaveGame) bool BrakeOwned=false;
 UPROPERTY(SaveGame) int32 PickupOrder=0;
 UPROPERTY(SaveGame) int32 ActiveBooking=0;
 UPROPERTY(SaveGame) int32 OfferSerial=0;
 // Headache-rack offers keep their own serial so the cut-part offer rotation is unchanged.
 UPROPERTY(SaveGame) int32 RackOfferSerial=0;
 UPROPERTY(SaveGame) TArray<FFoundrylineCustomerOrder> Orders;
 UPROPERTY(SaveGame) TArray<FFoundrylineFreightBooking> Bookings;
 UPROPERTY(SaveGame) TArray<FFoundrylineMoneyEntry> Money;
};
USTRUCT() struct FFoundrylineCommerceCommand {
 GENERATED_BODY()
 UPROPERTY() EFoundrylineCommerceAction Action=EFoundrylineCommerceAction::AcceptOrder;
 UPROPERTY() FGuid PlantId;
 UPROPERTY() int32 Id=0;
 UPROPERTY() int32 Choice=0;
 UPROPERTY() int32 Quantity=1;
 UPROPERTY() double Arrival=0;
 UPROPERTY() TArray<int32> Pallets;
 UPROPERTY() TArray<int32> Quantities;
};
// DeckCm: the longest item the deck carries lying flat across it (a pallet van's box is too narrow for a 6 ft 6 in rack).
struct FFoundrylineCarrierSpec {const TCHAR* Name;const TCHAR* Vehicle;int32 Spaces;double PayloadLb;int64 FeeCents;double LeadMinutes;double TransitMinutes;double DeckCm=240;};
struct FOUNDRYLINE_API FFoundrylineCommerceCatalog {
 static FFoundrylineCarrierSpec Carrier(int32 Id);
 static int64 SalePrice(uint8 Part);
 static const TCHAR* PaymentName(EFoundrylinePayment Payment);
 static const TCHAR* BookingName(EFoundrylineBookingState Status);
 static FString Date(double Minutes);
 static FString Money(int64 Cents);
 static int32 PalletSpaces(uint8 Part);
 static double PalletWeight(uint8 Part,int32 Quantity,double UnitLb);
 static bool Saleable(uint8 Part);
 // Longest dimension of one shipped item (0 for parts that sit within a pallet), and whether a carrier's deck takes it.
 static double ItemLengthCm(uint8 Part);
 static bool CarrierFits(int32 Vehicle,uint8 Part){return ItemLengthCm(Part)<=Carrier(Vehicle).DeckCm;}
 // "4 sheets of steel" / "6 frame sections" for an inbound delivery.
 static FString InboundLabel(const FFoundrylineFreightBooking& Booking);

};
