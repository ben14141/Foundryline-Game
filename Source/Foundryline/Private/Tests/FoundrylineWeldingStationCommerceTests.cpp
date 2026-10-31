#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Logistics/FoundrylineRackAccess.h"
#include "FoundrylineWeldingStationTestHelpers.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
#include "UI/FoundrylineERPViewModel.h"
#include "Economy/FoundrylineEconomyTuning.h"
namespace WeldingCommerceTest {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;using Role=EFoundrylineEmployeeRole;
// The shared reorganised starter layout with the station placed.
bool Station(FFoundrylineFactorySimulation& M,FString& E){return FoundrylineWeldingTestHelpers::PaintStarterAndPlaceStation(M,E);}
using FoundrylineWeldingTestHelpers::KitLb;
// Kit stock topped up to exactly two whole kits, so the batch never waits on supplies.
void TopUpBins(FFoundrylineFactorySimulation& M){FoundrylineWeldingTestHelpers::KitStock(M,M.State.WeldingStations[0],true);}
int Racks(const FFoundrylineFactorySimulation& M,Place Where){int N=0;for(const auto& A:M.State.Pallets)if(A.Part==Part::HeadacheRack&&A.Place==Where)N+=A.Quantity;return N;}
bool Welder(FFoundrylineFactorySimulation& M,FString& E){return M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)&&M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,Role::Welder,M.State.WeldingStations[0].Id,E);}
template<class F> bool Pump(FFoundrylineFactorySimulation& M,F Done,int Steps=30000,bool Refill=true){for(int I=0;I<Steps;++I){if(Done())return true;if(Refill&&!M.State.WeldingStations[0].Stage)TopUpBins(M);M.Step(.1f);}return Done();}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationCommerce,"Foundryline.Factory.WeldingStation.Commercial",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationCommerce::RunTest(const FString&){using namespace WeldingCommerceTest;
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);M.State.Commerce.CashCents=1000000;FString E;
 // Rack demand appears only once the business has a station to build racks; part offers are unchanged.
 auto RackOffer=[&](){return M.State.Commerce.Orders.FindByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::HeadacheRack);});};
 const int PartOffers=M.State.Commerce.Orders.FilterByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered;}).Num();
 TestNull(TEXT("No headache-rack offer without a welding station"),RackOffer());
 if(!TestTrue(TEXT("Station places: ")+E,Station(M,E)))return false;const int Id=M.State.WeldingStations[0].Id;
 M.State.Running=true;M.State.Speed=8;M.Step(.1f);
 const auto* Offer=RackOffer();if(!TestNotNull(TEXT("A headache-rack offer appears once the station exists"),Offer))return false;
 TestEqual(TEXT("Rack offers kept open on Sales while there is a station"),M.State.Commerce.Orders.FilterByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::HeadacheRack);}).Num(),FFoundrylineEconomyTuning::RackOffersOnSale);
 TestTrue(TEXT("Rack offer uses the configured quantity range and current catalog price"),Offer->Quantity>=FFoundrylineEconomyTuning::RackOrderMin&&Offer->Quantity<=FFoundrylineEconomyTuning::RackOrderMax&&Offer->UnitPriceCents==FFoundrylineCommerceCatalog::SalePrice(uint8(Part::HeadacheRack)));
 TestTrue(TEXT("Part offers are not displaced"),M.State.Commerce.Orders.FilterByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part!=uint8(Part::HeadacheRack);}).Num()>=PartOffers);
 const int OrderId=Offer->Id,Quantity=Offer->Quantity;const int64 Price=Offer->UnitPriceCents;
 {auto T=M;T.State.WeldingStations.Reset();for(auto& R:T.State.Routes){R.Destinations.Remove(Id);R.Sources.Remove(Id);}FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=OrderId;
  TestFalse(TEXT("A rack order cannot be accepted without a station"),T.CommerceCommand(A,E));TestTrue(TEXT("Refusal names the welding station: ")+E,E.Contains(TEXT("welding station")));}
 FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=OrderId;if(!TestTrue(TEXT("Accept the rack order: ")+E,M.CommerceCommand(A,E)))return false;
 A.Action=Act::AssignPickup;if(!TestTrue(TEXT("Assign it to the pickup zone: ")+E,M.CommerceCommand(A,E)))return false;

 // The Overview recommends the real next action for a rack order: staff the station, then set its batch, then ship.
 using WeldingGuideG=EFoundrylineGuideStep;auto Guide=FFoundrylineERPViewModel::Guide(M.State);
 TestTrue(TEXT("Overview focuses the rack order"),Guide.OrderId==OrderId&&Guide.bWeldedOrder&&Guide.StationId==Id);
 TestTrue(TEXT("Overview asks for a welder first: ")+Guide.Title,Guide.Step==WeldingGuideG::StaffWork&&Guide.Title.Contains(TEXT("welder"))&&Guide.Problems.ContainsByPredicate([](const auto& P){return P.Text.Contains(TEXT("No welder"));}));
 TestTrue(TEXT("No steel is asked for a rack order"),Guide.bStepDone[int(WeldingGuideG::BuySteel)]&&!Guide.Problems.ContainsByPredicate([](const auto& P){return P.Text.Contains(TEXT("steel"));}));
 if(!TestTrue(TEXT("Hire and assign a welder: ")+E,Welder(M,E)))return false;Guide=FFoundrylineERPViewModel::Guide(M.State);
 TestTrue(TEXT("Paid welder with an idle station: the Overview says payroll is running and points at Orders"),Guide.PayrollCentsPerDay>0&&Guide.IdleWhilePaid.Contains(TEXT("welding station"))&&Guide.Problems.ContainsByPredicate([](const auto& P){return P.Text.StartsWith(TEXT("Payroll"))&&P.Action==TEXT("Page0");}));
 TestTrue(TEXT("Then it asks to set the station for the order: ")+Guide.Title,Guide.Step==WeldingGuideG::ProgramPlasma&&Guide.PartsToMake==Quantity&&Guide.Action==FString::Printf(TEXT("StationOrder%d"),OrderId));
 // Build exactly the order. Each finished rack is wanted by the order, so its OUT pallet goes whole to the pickup zone.
 TestTrue(TEXT("Batch for the order: ")+E,M.ConfigureWeldingStation(Id,1,Guide.PartsToMake,E));TopUpBins(M);Guide=FFoundrylineERPViewModel::Guide(M.State);
 TestTrue(TEXT("With the batch set, the Overview follows the welding: ")+Guide.Title,Guide.Step==WeldingGuideG::BookFreight&&Guide.PartsToMake==0&&Guide.PartsCutting==Quantity&&Guide.Title.Contains(TEXT("Welding racks")));
 bool SawOutReserved=false;auto Staged=[&](){for(const auto& P:M.State.Pallets)SawOutReserved|=P.Place==Place::StationOut&&P.Reserved>0;int N=0;for(const auto& P:M.State.Pallets)if(FFoundrylineRackAccess::Ready(M.State,P)&&P.OrderId==OrderId)N+=P.Quantity;return N==Quantity;};
 if(!TestTrue(TEXT("Racks are built and prepared in finished-goods racking"),Pump(M,Staged)))return false;
 TestTrue(TEXT("A forklift collected from OUT"),SawOutReserved);TestEqual(TEXT("OUT is clear"),Racks(M,Place::StationOut),0);TestEqual(TEXT("Finished products wait in actual rack slots"),Racks(M,Place::Rack),Quantity);
 TestTrue(TEXT("Each staged pallet carries whole kits' mass"),!M.State.Pallets.ContainsByPredicate([&](const auto& P){return FFoundrylineRackAccess::Ready(M.State,P)&&!FMath::IsNearlyEqual(P.UnitLb,KitLb,.01f);}));
 TestTrue(TEXT("Status after collection: ")+M.WeldingStationStatus(M.State.WeldingStations[0]),M.WeldingStationStatus(M.State.WeldingStations[0]).Contains(TEXT("Choose a product")));
 TestTrue(TEXT("Staged racks validate: ")+E,M.Valid(E));
 Guide=FFoundrylineERPViewModel::Guide(M.State);TestTrue(TEXT("Overview says book a truck, and that a van won't do: ")+Guide.Detail,Guide.Title==TEXT("Book a pickup truck")&&Guide.Detail.Contains(TEXT("flatbed or larger")));
 TestEqual(TEXT("Finished products at the bay are listed apart from parts"),Guide.FinishedAtBayProducts,Quantity);
 TestEqual(TEXT("Racks are not counted as finished cut parts at the bay"),Guide.FinishedAtBay,0);

 // Carrier footprint and weight: a 6 ft 6 in rack does not fit a pallet van; a pallet of racks takes two deck spaces.
 A={};A.Action=Act::BookPickup;A.Arrival=M.State.Commerce.Clock+60;for(const auto& P:M.State.Pallets)if(FFoundrylineRackAccess::Ready(M.State,P)&&P.OrderId==OrderId){A.Pallets.Add(P.Id);A.Quantities.Add(P.Quantity);}
 A.Choice=0;TestFalse(TEXT("The pallet van is refused"),M.CommerceCommand(A,E));TestTrue(TEXT("Refusal says it won't fit: ")+E,E.Contains(TEXT("won't fit")));
 A.Choice=1;A.Arrival=M.State.Commerce.Clock+60;TestEqual(TEXT("Each rack pallet needs two deck spaces"),FFoundrylineCommerceCatalog::PalletSpaces(uint8(Part::HeadacheRack)),2);
 if(A.Pallets.Num()>1)TestFalse(TEXT("Two rack pallets exceed the two-pallet flatbed"),M.CommerceCommand(A,E));
 A.Choice=2;A.Arrival=M.State.Commerce.Clock+60;const int64 Before=M.State.Commerce.EarnedRevenueCents;if(!TestTrue(TEXT("The rigid truck takes the order: ")+E,M.CommerceCommand(A,E)))return false;
 TestTrue(TEXT("Booked manifest validates: ")+E,M.Valid(E));

 // Mid-delivery save, then delivery and payment exactly once.
 TArray<uint8> Bytes;{auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TestTrue(TEXT("Save with the booking"),UGameplayStatics::SaveGameToMemory(Save,Bytes));}
 auto Completed=[&](const FFoundrylineFactorySimulation& X){return X.State.Commerce.Orders.ContainsByPredicate([&](const auto& O){return O.Id==OrderId&&O.Status==EFoundrylineOrderState::Completed;});};
 if(!TestTrue(TEXT("Carrier delivers and the order completes"),Pump(M,[&]{return Completed(M);},30000,false)))return false;
 for(int I=0;I<3000;++I)M.Step(.1f);
 const auto* Done=M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==OrderId;});
 TestTrue(TEXT("Paid exactly once for every rack"),Done&&Done->PaidCents==int64(Quantity)*Price&&Done->Delivered==Quantity);TestEqual(TEXT("Revenue counted once"),M.State.Commerce.EarnedRevenueCents-Before,Done&&Done->Payment!=EFoundrylinePayment::Upfront?int64(Quantity)*Price:int64(0));
 TestEqual(TEXT("No rack left anywhere in the plant"),Racks(M,Place::Pickup)+Racks(M,Place::Outbound)+Racks(M,Place::Forks)+Racks(M,Place::Rack)+Racks(M,Place::StationOut),0);
 TestTrue(TEXT("Plant validates after delivery: ")+E,M.Valid(E));
 if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){FFoundrylineFactorySimulation R;R.State=Loaded->Factory;R.Solids=M.Solids;TestTrue(TEXT("Mid-delivery save validates: ")+E,R.Valid(E));
  if(TestTrue(TEXT("Restored delivery completes"),Pump(R,[&]{return Completed(R);},30000,false))){for(int I=0;I<3000;++I)R.Step(.1f);const auto* X=R.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==OrderId;});TestTrue(TEXT("Restored order paid exactly once"),X&&X->PaidCents==int64(Quantity)*Price);}}
 else AddError(TEXT("Mid-delivery save did not load"));

 // Without an order: a pallet is collected to finished-goods storage once full; the last, partial pallet once the batch ends.
 TestTrue(TEXT("Six racks for stock: ")+E,M.ConfigureWeldingStation(Id,1,Layout::RacksPerOutPallet+1,E));
 int MaxOut=0;auto Stocked=[&](){MaxOut=FMath::Max(MaxOut,Racks(M,Place::StationOut));return Racks(M,Place::Rack)==Layout::RacksPerOutPallet+1&&!M.State.Jobs.ContainsByPredicate([](const auto& J){return J.Part==Part::HeadacheRack;});};
 if(!TestTrue(TEXT("All six reach finished-goods storage"),Pump(M,Stocked,60000)))return false;
 TestTrue(TEXT("OUT never held more than a pallet before collection"),MaxOut<=Layout::RacksPerOutPallet+1);TestEqual(TEXT("OUT is clear"),Racks(M,Place::StationOut),0);
 TArray<int> Stacks;for(const auto& P:M.State.Pallets)if(P.Part==Part::HeadacheRack&&P.Place==Place::Rack)Stacks.Add(P.Quantity);Stacks.Sort();
 TestTrue(TEXT("Stored as a full pallet of five and a pallet of one, in rack slots"),Stacks==TArray<int>({1,Layout::RacksPerOutPallet}));
 TestEqual(TEXT("Eight units made in all"),M.State.WeldingStations[0].UnitsCompleted,Quantity+Layout::RacksPerOutPallet+1);
 TestTrue(TEXT("Stored racks validate: ")+E,M.Valid(E));
 for(const auto& P:M.State.Pallets)if(P.Part==Part::HeadacheRack&&P.Place==Place::Rack)if(const auto* Slot=M.State.Slots.FindByPredicate([&](const auto& V){return V.Id==P.Owner;})){double Room=1e9;for(const auto& O:M.State.Slots)if(O.Id!=Slot->Id&&FVector::Dist2D(O.Position,Slot->Position)<60&&O.Position.Z>Slot->Position.Z+1)Room=FMath::Min(Room,O.Position.Z-Slot->Position.Z-10);
  TestTrue(FString::Printf(TEXT("A stored stack of %d fits under the shelf above it"),P.Quantity),Layout::PalletTopCm+P.Quantity*Layout::RackSizeCm().Z<=Room);}
 Guide=FFoundrylineERPViewModel::Guide(M.State);TestEqual(TEXT("Stock lists the stored racks as finished products"),Guide.FinishedInStorage,Layout::RacksPerOutPallet+1);
 // Bracket and plate sales keep working alongside: Foundryline.Commerce.TwoRecipeRepeatSales.
 return true;
}
#endif
