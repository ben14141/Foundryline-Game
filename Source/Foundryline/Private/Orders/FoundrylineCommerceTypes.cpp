#include "Orders/FoundrylineCommerceTypes.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "Economy/FoundrylineEconomyTuning.h"
FFoundrylineCarrierSpec FFoundrylineCommerceCatalog::Carrier(int32 I){return FFoundrylineEconomyTuning::Carrier(I);}
int64 FFoundrylineCommerceCatalog::SalePrice(uint8 P){return FFoundrylineEconomyTuning::SalePriceCents(EFoundrylinePart(P));}
bool FFoundrylineCommerceCatalog::Saleable(uint8 P){return SalePrice(P)>0;}
// A finished headache rack ships lying flat across two pallets: 6 ft 6 in wide (SM_HeadacheRack bounds).
double FFoundrylineCommerceCatalog::ItemLengthCm(uint8 P){return EFoundrylinePart(P)==EFoundrylinePart::HeadacheRack?198.12:0;}
FString FFoundrylineCommerceCatalog::InboundLabel(const FFoundrylineFreightBooking& B){if(!B.Part)return FString::Printf(TEXT("%d sheet%s of steel"),B.Sheets,B.Sheets==1?TEXT(""):TEXT("s"));FString Name=FString(FFoundrylineFactoryCatalog::Part(EFoundrylinePart(B.Part)).Name).ToLower();if(B.Sheets==1)Name.RemoveFromEnd(TEXT("s"));return FString::Printf(TEXT("%d %s"),B.Sheets,*Name);}


const TCHAR* FFoundrylineCommerceCatalog::PaymentName(EFoundrylinePayment P){return P==EFoundrylinePayment::Upfront?TEXT("Paid upfront"):P==EFoundrylinePayment::PerDelivery?TEXT("Paid per delivery"):TEXT("Paid on completion");}
const TCHAR* FFoundrylineCommerceCatalog::BookingName(EFoundrylineBookingState S){static const TCHAR* N[]={TEXT("Booked"),TEXT("On the way"),TEXT("Waiting for bay"),TEXT("Loading / unloading"),TEXT("Departing"),TEXT("In transit"),TEXT("Delivered")};return N[FMath::Clamp(int(S),0,6)];}
FString FFoundrylineCommerceCatalog::Date(double T){const int64 M=FMath::Max(int64(0),int64(T));return FString::Printf(TEXT("Day %lld  %02lld:%02lld"),M/1440+1,M/60%24,M%60);}
FString FFoundrylineCommerceCatalog::Money(int64 C){return FString::Printf(TEXT("%s$%lld.%02lld"),C<0?TEXT("-"):TEXT(""),FMath::Abs(C)/100,FMath::Abs(C)%100);}
int32 FFoundrylineCommerceCatalog::PalletSpaces(uint8 P){return EFoundrylinePart(P)==EFoundrylinePart::Sheet||EFoundrylinePart(P)==EFoundrylinePart::HeadacheRack?2:1;}
double FFoundrylineCommerceCatalog::PalletWeight(uint8 P,int32 Q,double Lb){return Q*Lb+60*PalletSpaces(P);}
