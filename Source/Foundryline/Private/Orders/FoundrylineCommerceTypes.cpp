#include "Orders/FoundrylineCommerceTypes.h"
#include "Production/FoundrylineFactoryTypes.h"
FFoundrylineCarrierSpec FFoundrylineCommerceCatalog::Carrier(int32 I){static const FFoundrylineCarrierSpec S[]={{TEXT("Swift Van Couriers"),TEXT("Pallet van"),1,900,2500,15,60},{TEXT("Foundry Lane Haulage"),TEXT("Two-pallet flatbed"),2,2000,4000,20,90},{TEXT("County Freight"),TEXT("Rigid delivery truck"),6,6000,9000,40,120},{TEXT("Interstate Pallet Lines"),TEXT("Box semi"),20,24000,18000,60,240}};return S[FMath::Clamp(I,0,3)];}
int64 FFoundrylineCommerceCatalog::SalePrice(uint8 P){switch(EFoundrylinePart(P)){case EFoundrylinePart::MountingPlate:return 500;case EFoundrylinePart::GussetPlate:return 700;case EFoundrylinePart::Bracket:return 1100;case EFoundrylinePart::ReinforcementPlate:return 1700;case EFoundrylinePart::BentBracket:return 1900;case EFoundrylinePart::SupportChannel:return 3000;default:return 0;}}
bool FFoundrylineCommerceCatalog::Saleable(uint8 P){return SalePrice(P)>0;}
FString FFoundrylineCommerceCatalog::InboundLabel(const FFoundrylineFreightBooking& B){if(!B.Part)return FString::Printf(TEXT("%d sheet%s of steel"),B.Sheets,B.Sheets==1?TEXT(""):TEXT("s"));FString Name=FString(FFoundrylineFactoryCatalog::Part(EFoundrylinePart(B.Part)).Name).ToLower();if(B.Sheets==1)Name.RemoveFromEnd(TEXT("s"));return FString::Printf(TEXT("%d %s"),B.Sheets,*Name);}


const TCHAR* FFoundrylineCommerceCatalog::PaymentName(EFoundrylinePayment P){return P==EFoundrylinePayment::Upfront?TEXT("Paid upfront"):P==EFoundrylinePayment::PerDelivery?TEXT("Paid per delivery"):TEXT("Paid on completion");}
const TCHAR* FFoundrylineCommerceCatalog::BookingName(EFoundrylineBookingState S){static const TCHAR* N[]={TEXT("Booked"),TEXT("On the way"),TEXT("Waiting for bay"),TEXT("Loading / unloading"),TEXT("Departing"),TEXT("In transit"),TEXT("Delivered")};return N[FMath::Clamp(int(S),0,6)];}
FString FFoundrylineCommerceCatalog::Date(double T){const int64 M=FMath::Max(int64(0),int64(T));return FString::Printf(TEXT("Day %lld  %02lld:%02lld"),M/1440+1,M/60%24,M%60);}
FString FFoundrylineCommerceCatalog::Money(int64 C){return FString::Printf(TEXT("%s$%lld.%02lld"),C<0?TEXT("-"):TEXT(""),FMath::Abs(C)/100,FMath::Abs(C)%100);}
int32 FFoundrylineCommerceCatalog::PalletSpaces(uint8 P){return EFoundrylinePart(P)==EFoundrylinePart::Sheet?2:1;}
double FFoundrylineCommerceCatalog::PalletWeight(uint8 P,int32 Q,double Lb){return Q*Lb+60*PalletSpaces(P);}
