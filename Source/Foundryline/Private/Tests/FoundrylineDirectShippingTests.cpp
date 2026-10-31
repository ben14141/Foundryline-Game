#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Logistics/FoundrylineRackAccess.h"
namespace {using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRackEitherFace,"Foundryline.Commerce.DirectShipping.EitherRackFace",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRackEitherFace::RunTest(const FString&){
 for(int Q=0;Q<4;++Q){FFoundrylineFactorySimulation M;M.State.Commerce.Enabled=true;M.State.Commerce.CashCents=1000000;M.State.FloorMin={-3000,-3000};M.State.FloorMax={3000,3000};const FVector At(0,0,20);
  const auto Front=FFoundrylinePlanningEquipment::Service(At,Q,470),Back=FFoundrylinePlanningEquipment::Service(At,(Q+2)%4,470);FString E;
  M.Solids={Front.ExpandBy(-10)};TestFalse(TEXT("Obstructed front face is unavailable"),FFoundrylineRackAccess::Clear(M,Front));TestTrue(TEXT("Back face stays clear"),FFoundrylineRackAccess::Clear(M,Back));TestTrue(TEXT("Purchase needs only one clear face: ")+E,M.CanBuyPlanningEquipment(2,At,Q,2,E));
  M.Solids={Back.ExpandBy(-10)};TestTrue(TEXT("Front face stays clear"),FFoundrylineRackAccess::Clear(M,Front));TestTrue(TEXT("Front-only placement accepted"),M.CanBuyPlanningEquipment(2,At,Q,2,E));
  M.Solids.Add(Front.ExpandBy(-10));TestFalse(TEXT("Both faces blocked rejects placement"),M.CanBuyPlanningEquipment(2,At,Q,2,E));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectRackShipping,"Foundryline.Commerce.DirectShipping.PartialPalletAndLegacyRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDirectRackShipping::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString E;auto& O=M.State.Commerce.Orders[0];O.Quantity=3;const int Order=O.Id;
 FFoundrylineCommerceCommand Cmd;Cmd.Action=Act::AcceptOrder;Cmd.Id=Order;if(!TestTrue(TEXT("Accept three-part order"),M.CommerceCommand(Cmd,E)))return false;
 const auto* Output=M.State.Slots.FindByPredicate([](const auto& S){return S.bMachineOutput;});if(!TestNotNull(TEXT("Machine output exists"),Output))return false;
 FFoundrylineFactoryPallet Stock;Stock.Id=M.State.NextId++;Stock.Part=EFoundrylinePart::Bracket;Stock.Quantity=3;Stock.UnitLb=FFoundrylineFactoryCatalog::Part(Stock.Part).UnitLb;Stock.Place=Place::Rack;Stock.Owner=Output->Id;Stock.Position=Output->Position;M.State.Pallets.Add(Stock);const int Id=Stock.Id;M.State.ReceivedSteelLb+=3*Stock.UnitLb;
 if(!TestTrue(TEXT("Prepare a partial pallet: ")+E,M.PrepareOrderStock(Id,Order,E)))return false;
 TestTrue(TEXT("Preparation targets a finished rack, never floor"),M.State.Jobs.ContainsByPredicate([&](const auto& J){return J.SourcePallet==Id&&J.DestinationPlace==Place::Rack&&FFoundrylineRackAccess::FinishedSlot(M.State,J.Destination);}));
 M.State.Speed=8;for(int I=0;I<6000&&!M.State.Pallets.ContainsByPredicate([&](const auto& P){return P.Id==Id&&FFoundrylineRackAccess::Ready(M.State,P);});++I)M.Step(.1f);
 const auto* P=M.State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!TestTrue(TEXT("Pallet arrives in actual finished-rack slot"),P&&FFoundrylineRackAccess::Ready(M.State,*P)))return false;
 const auto* Slot=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==P->Owner;});TestTrue(TEXT("Pallet seated at shelf, not forklift stop"),Slot&&P->Position.Equals(Slot->Position,.1));
 auto NoRack=M;NoRack.State.StorageRacks.Reset();TestFalse(TEXT("Floor or generic storage cannot be booked"),FFoundrylineRackAccess::Ready(NoRack.State,*P));
 Cmd={};Cmd.Action=Act::BookPickup;Cmd.Id=Order;Cmd.Choice=1;Cmd.Arrival=M.State.Commerce.Clock+80;Cmd.Pallets={Id};Cmd.Quantities={3};if(!TestTrue(TEXT("Book partial load from rack: ")+E,M.CommerceCommand(Cmd,E)))return false;
 const int Booking=M.State.Commerce.Bookings.Last().Id;const auto Cash=M.State.Commerce.CashCents;TestFalse(TEXT("Double booking rejected"),M.CommerceCommand(Cmd,E));TestEqual(TEXT("Duplicate charges no fee"),M.State.Commerce.CashCents,Cash);
 auto Legacy=M;auto* Old=Legacy.State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;});Old->Place=Place::Pickup;Old->Owner=1000;Old->Position=Legacy.ShippingPickupPosition(0);Legacy.PlanFinishedGoods();
 TestTrue(TEXT("Booked legacy floor pallet gets real rack recovery job"),Legacy.State.Jobs.ContainsByPredicate([&](const auto& J){return J.SourcePallet==Id&&J.DestinationPlace==Place::Rack&&J.BookingId==Booking;}));
 bool Floor=false;for(int I=0;I<10000&&M.State.Commerce.Orders[0].Delivered<3;++I){M.Step(.1f);Floor|=M.State.Pallets.ContainsByPredicate([](const auto& V){return V.Place==Place::Pickup;})||M.State.Jobs.ContainsByPredicate([](const auto& J){return J.DestinationPlace==Place::Pickup;});}
 TestFalse(TEXT("No floor-staging state during entire shipment"),Floor);TestEqual(TEXT("Rack-to-truck delivery completes"),M.State.Commerce.Orders[0].Delivered,3);TestTrue(TEXT("Final ledger valid: ")+E,M.Valid(E));return true;
}
#endif
