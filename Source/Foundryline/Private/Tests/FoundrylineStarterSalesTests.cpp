#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
namespace {using Part=EFoundrylinePart;using Act=EFoundrylineCommerceAction;using Place=EFoundrylinePalletPlace;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterOfferCapability,"Foundryline.Commerce.StarterOfferCapability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterOfferCapability::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString E;
 for(const auto& O:M.State.Commerce.Orders)TestTrue(TEXT("Initial offers need only installed plasma"),O.Part!=uint8(Part::BentBracket)&&O.Part!=uint8(Part::SupportChannel));
 auto Stale=M.State.Commerce.Orders[0];Stale.Id=M.State.NextId++;Stale.Part=uint8(Part::BentBracket);M.State.Commerce.Orders.Add(Stale);
 FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=Stale.Id;const int64 Cash=M.State.Commerce.CashCents;TestFalse(TEXT("Stale advanced acceptance denied by server"),M.CommerceCommand(A,E));TestEqual(TEXT("Rejected order pays no advance"),M.State.Commerce.CashCents,Cash);
 M.State.Commerce.CashCents=1000000;A={};A.Action=Act::BuyBrake;TestTrue(TEXT("Install actual upgrade"),M.CommerceCommand(A,E));A={};A.Action=Act::AcceptOrder;A.Id=Stale.Id;TestFalse(TEXT("Unstaffed upgrade still cannot accept"),M.CommerceCommand(A,E));
 TestTrue(TEXT("Hire person"),M.HireEmployee(EFoundrylineEmployeeRole::Unassigned,FGuid::NewGuid(),E));TestTrue(TEXT("Assign actual brake"),M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,EFoundrylineEmployeeRole::Operator,2,E));TestTrue(TEXT("Staffed installed upgrade accepts advanced contract"),M.CommerceCommand(A,E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterRepeatSales,"Foundryline.Commerce.TwoRecipeRepeatSales",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterRepeatSales::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Running=true;M.State.Speed=8;FString E;
 auto Pump=[&](auto Done){for(int I=0;I<16000&&!Done();++I)M.Step(.1f);return Done();};
 int64 ExpectedRevenue=0;
 for(int Cycle=0;Cycle<2;++Cycle){const Part P=Cycle?Part::MountingPlate:Part::Bracket;const int Q=Cycle?60:40;
  const auto* Offer=M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Part==uint8(P)&&O.Status==EFoundrylineOrderState::Offered;});if(!TestNotNull(TEXT("Next recipe has a real offer"),Offer))return false;const int Id=Offer->Id;
  FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=Id;if(!TestTrue(TEXT("Accept real order"),M.CommerceCommand(A,E)))return false;A.Action=Act::AssignPickup;if(!TestTrue(TEXT("Assign next shipment"),M.CommerceCommand(A,E)))return false;
  A={};A.Action=Act::BuySteel;A.Quantity=1;if(!TestTrue(TEXT("Buy paid material"),M.CommerceCommand(A,E)))return false;TestTrue(TEXT("Set exact recipe and quantity"),M.Program(1,P,Q,false,E));
  auto Staged=[&](){int N=0;for(const auto& X:M.State.Pallets)if(X.Place==Place::Pickup&&X.OrderId==Id)N+=X.Quantity;return N==Q;};if(!TestTrue(TEXT("Employees cut and automatically rack/stage full order"),Pump(Staged)))return false;
  A={};A.Action=Act::BookPickup;A.Choice=1;A.Arrival=M.State.Commerce.Clock+60;for(const auto& X:M.State.Pallets)if(X.Place==Place::Pickup&&X.OrderId==Id){A.Pallets.Add(X.Id);A.Quantities.Add(X.Quantity);}if(!TestTrue(TEXT("Book staged real pallets"),M.CommerceCommand(A,E)))return false;
  if(!TestTrue(TEXT("Carrier delivers and completes order"),Pump([&](){return M.State.Commerce.Orders.ContainsByPredicate([&](const auto& O){return O.Id==Id&&O.Status==EFoundrylineOrderState::Completed;});})))return false;
  const auto* Done=M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;});ExpectedRevenue+=Done->Quantity*Done->UnitPriceCents;TestEqual(TEXT("Exact invoice payment"),Done->PaidCents,int64(Done->Quantity)*Done->UnitPriceCents);TestTrue(TEXT("Ledger remains valid across recipe transition: ")+E,M.Valid(E));
 }
 TestEqual(TEXT("Two distinct starter recipes paid"),ExpectedRevenue,int64(74000));TestEqual(TEXT("Earned revenue reconciles"),M.State.Commerce.EarnedRevenueCents,ExpectedRevenue);TestTrue(TEXT("Repeat flow stays cash positive without debt"),M.State.Commerce.CashCents>0&&M.State.Commerce.UnpaidCents==0);TestTrue(TEXT("More basic offers remain available"),M.State.Commerce.Orders.ContainsByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::Bracket);}));AddInfo(FString::Printf(TEXT("After two orders cash $%.2f, costs $%.2f, remaining bed %.3flb"),M.State.Commerce.CashCents/100.,M.State.Commerce.CostCents/100.,M.State.Machines[0].BedLb));return true;
}
#endif
