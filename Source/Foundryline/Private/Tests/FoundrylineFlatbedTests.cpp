#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
namespace {using Act=EFoundrylineCommerceAction;using Place=EFoundrylinePalletPlace;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlatbedOrderFlow,"Foundryline.Flatbed.FirstOrderDelivery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlatbedOrderFlow::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString E;
 if(!TestTrue(TEXT("Fresh commerce ledger valid: ")+E,M.Valid(E))) {AddError(E);return false;}
 TestEqual(TEXT("No free steel"),M.Total(EFoundrylinePart::Sheet),0);
 TestFalse(TEXT("Legacy free receiving blocked"),M.ReceiveSteel(1,E));
 const int Order=M.State.Commerce.Orders[0].Id;
 FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=Order;
 TestTrue(TEXT("Accept fixed order"),M.CommerceCommand(A,E));
 TestFalse(TEXT("Cannot accept twice"),M.CommerceCommand(A,E));
 A={};A.Action=Act::AssignPickup;A.Id=Order;TestTrue(TEXT("Assign pickup"),M.CommerceCommand(A,E));
 A={};A.Action=Act::BuySteel;A.Quantity=1;TestTrue(TEXT("Purchase sheet with scheduled carrier"),M.CommerceCommand(A,E));
 TestTrue(TEXT("Set 40 brackets"),M.Program(1,EFoundrylinePart::Bracket,40,false,E));
 M.State.Running=true;M.State.Speed=8;
 auto Staged=[&](){int Q=0;for(const auto& P:M.State.Pallets)if(P.Place==Place::Pickup)Q+=P.Quantity;return Q;};
 for(int I=0;I<12000&&Staged()<40;++I){M.Step(.1f);if(!M.Valid(E)){AddError(TEXT("Live receipt/cutting ledger: ")+E);return false;}}
 TestEqual(TEXT("Forty cut parts staged automatically"),Staged(),40);
 if(Staged()!=40){AddError(FString::Printf(TEXT("Clock %.1f jobs %d pallets %d machine remaining %.1f"),M.State.Commerce.Clock,M.State.Jobs.Num(),M.State.Pallets.Num(),M.State.Machines[0].Remaining));return false;}
 if(!TestTrue(TEXT("Valid staged inventory"),M.Valid(E))){AddError(E);return false;}
 A={};A.Action=Act::BookPickup;A.Choice=4;A.Arrival=M.State.Commerce.Clock+120;
 for(const auto& P:M.State.Pallets)if(P.Place==Place::Pickup){A.Pallets.Add(P.Id);A.Quantities.Add(P.Quantity);}
 TestTrue(TEXT("Book exact selected pallets"),M.CommerceCommand(A,E));const int Booking=M.State.Commerce.Bookings.Last().Id;
 TestFalse(TEXT("Locked pallets cannot be booked twice"),M.CommerceCommand(A,E));
 TestEqual(TEXT("No payment before delivery"),M.State.Commerce.EarnedRevenueCents,int64(0));
 for(int I=0;I<12000&&M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Order;})->Delivered<40;++I)M.Step(.1f);
 const auto* O=M.State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Id==Order;});
 TestEqual(TEXT("Customer receives exact quantity"),O->Delivered,40);TestEqual(TEXT("Completion pays exactly once"),O->PaidCents,int64(O->Quantity)*O->UnitPriceCents);
 const int64 Paid=O->PaidCents;for(int I=0;I<100;++I)M.Step(.1f);TestEqual(TEXT("Payment not replayed"),M.State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Id==Order;})->PaidCents,Paid);
 TestTrue(TEXT("Cash reconciles purchases fees wages and payment"),M.State.Commerce.CashCents-M.State.Commerce.UnpaidCents==400000+Paid-M.State.Commerce.CostCents);
 TestTrue(TEXT("Mass conserved through shipping"),FMath::Abs(M.SteelMass()-M.State.ReceivedSteelLb)<.2f);
 TestTrue(TEXT("Final valid ledger"),M.Valid(E));if(!E.IsEmpty())AddInfo(E);return true;
}

#endif
