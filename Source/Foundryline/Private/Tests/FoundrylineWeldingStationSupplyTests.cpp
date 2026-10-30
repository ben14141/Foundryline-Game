#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "FoundrylineWeldingStationTestHelpers.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
namespace {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;
// The shared reorganised starter with the station placed.
bool StarterStation(FFoundrylineFactorySimulation& M,FString& E){if(!FoundrylineWeldingTestHelpers::StarterStation(M,E)||!M.HireEmployee(EFoundrylineEmployeeRole::Unassigned,FGuid::NewGuid(),E))return false;return M.AssignCollector(M.State.UnassignedEmployees.Last().Id,M.State.WeldingStations[0].Id,E);}
// Cut plates and brackets already in the finished-parts racks, as the plasma table leaves them.
void RackStock(FFoundrylineFactorySimulation& M,Part P,int Slot,int Quantity,float UnitLb){auto* S=M.State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});S->Part=P;FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=Quantity;A.Place=Place::Rack;A.Owner=Slot;A.Position=S->Position;A.UnitLb=UnitLb;M.State.Pallets.Add(A);}
bool Buy(FFoundrylineFactorySimulation& M,int Station,Part P,int Quantity,FString& E){FFoundrylineCommerceCommand A;A.Action=Act::BuyParts;A.Id=Station;A.Choice=int(P);A.Quantity=Quantity;return M.CommerceCommand(A,E);}
int Bin(const FFoundrylineFactorySimulation& M,int Station,Part P){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==Place::Buffer&&A.Part==P)V+=A.Quantity;return V;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationSupplies,"Foundryline.Factory.WeldingStation.Supplies",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationSupplies::RunTest(const FString&){
 FFoundrylineAssemblyRecipe R;TestTrue(TEXT("Headache rack recipe is product data"),FFoundrylineFactoryCatalog::Recipe(1,R));
 TestTrue(TEXT("Rack bill: 2 plates, 4 brackets, 6 frame sections, 2 light kits"),R.Bill[int(Part::MountingPlate)]==2&&R.Bill[int(Part::Bracket)]==4&&R.Bill[int(Part::FrameSection)]==6&&R.Bill[int(Part::LightKit)]==2&&R.Output==Part::HeadacheRack);
 int Other=0;for(int I=0;I<R.Bill.Num();++I)Other+=R.Bill[I];TestEqual(TEXT("No other part types in the rack bill"),Other,14);
 FFoundrylineBoughtPart Frame,Light;TestTrue(TEXT("Frame sections are bought with fixed weight and price"),FFoundrylineFactoryCatalog::Bought(Part::FrameSection,&Frame)&&Frame.UnitLb>0&&Frame.PriceCents>0);
 TestTrue(TEXT("Light kits are bought with fixed weight and price"),FFoundrylineFactoryCatalog::Bought(Part::LightKit,&Light)&&Light.UnitLb>0&&Light.PriceCents>0);
 TestFalse(TEXT("Cut plates and brackets are not bought"),FFoundrylineFactoryCatalog::Bought(Part::MountingPlate)||FFoundrylineFactoryCatalog::Bought(Part::Bracket));
 TestTrue(TEXT("Bundle and crate capacities hold at least one rack; a finished-rack pallet holds one OUT stack"),FFoundrylineFactoryCatalog::Part(Part::FrameSection).Capacity>=6&&FFoundrylineFactoryCatalog::Part(Part::LightKit).Capacity>=2&&FFoundrylineFactoryCatalog::Part(Part::HeadacheRack).Capacity==FFoundrylineWeldingStationLayout::RacksPerOutPallet);

 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station places: ")+E,StarterStation(M,E)))return false;
 const int Id=M.State.WeldingStations[0].Id;
 TestTrue(TEXT("New station joins the forklift route"),M.State.Routes[0].Destinations.Contains(Id));
 TestEqual(TEXT("Unconfigured station requests nothing"),M.StationTarget(M.State.WeldingStations[0],Part::FrameSection),0);
 TestFalse(TEXT("Parts cannot be bought for an unconfigured station"),Buy(M,Id,Part::FrameSection,6,E));
 TestFalse(TEXT("Unknown recipe rejected"),M.ConfigureWeldingStation(Id,7,10,E));
 TestTrue(TEXT("Choose headache racks x10: ")+E,M.ConfigureWeldingStation(Id,1,10,E));
 const auto& S=M.State.WeldingStations[0];
 TestTrue(TEXT("Stock target is two kits within the IN capacity"),M.StationTarget(S,Part::MountingPlate)==4&&M.StationTarget(S,Part::Bracket)==8&&M.StationTarget(S,Part::FrameSection)==12&&M.StationTarget(S,Part::LightKit)==4);
 TestTrue(TEXT("Missing bought parts produce a Buy parts status: ")+M.WeldingStationStatus(S),M.WeldingStationStatus(S).Contains(TEXT("buy parts"))||M.WeldingStationStatus(S).Contains(TEXT("plasma")));

 RackStock(M,Part::Bracket,107,20,5);RackStock(M,Part::MountingPlate,108,10,2);
 const int64 Cash=M.State.Commerce.CashCents;
 TestFalse(TEXT("Cannot buy more than the IN stock lacks"),Buy(M,Id,Part::FrameSection,13,E));
 TestTrue(TEXT("Buy 12 frame sections: ")+E,Buy(M,Id,Part::FrameSection,12,E));TestTrue(TEXT("Buy 4 light kits: ")+E,Buy(M,Id,Part::LightKit,4,E));
 TestEqual(TEXT("Paid parts plus two supplier freight bookings"),Cash-M.State.Commerce.CashCents,12*Frame.PriceCents+4*Light.PriceCents+2*FFoundrylineEconomyTuning::SupplierFreightCents);
 TestFalse(TEXT("Booked parts count; a second purchase is refused"),Buy(M,Id,Part::LightKit,1,E));
 TestTrue(TEXT("Configured station with bookings validates: ")+E,M.Valid(E));
 TestFalse(TEXT("A station expecting deliveries cannot be removed"),M.RemoveWeldingStation(Id,E));

 // Run the real plant: trucks, forklifts, pallets and the IN port. Every part type goes through the one IN port, one at a time.
 const float Mass=M.SteelMass()-M.State.ReceivedSteelLb;M.State.Running=true;M.State.Speed=8;
 TSet<int> Seen,CartTypes;TArray<int> Order;int MaxOnIn=0,MaxJobs=0;bool Saved=false;TArray<uint8> Bytes;
 for(int I=0;I<40000;++I){M.Step(.1f);int OnIn=0,Jobs=0;for(const auto& Collector:M.State.Collectors)for(const auto& Load:Collector.Load)if(Load.Picked)CartTypes.Add(Load.Part);
  for(const auto& A:M.State.Pallets)if(A.Place==Place::StationIn&&A.Owner==Id){++OnIn;if(!Seen.Contains(int(A.Part))){Seen.Add(int(A.Part));Order.Add(int(A.Part));}}
  for(const auto& J:M.State.Jobs)if(J.Destination==Id)++Jobs;MaxOnIn=FMath::Max(MaxOnIn,OnIn);MaxJobs=FMath::Max(MaxJobs,Jobs);
  if(I%500==0&&!M.Valid(E)){AddError(TEXT("Live state invalid: ")+E);break;}
  // Persist once while a delivery sits on the IN port.
  if(!Saved&&OnIn){auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;Saved=UGameplayStatics::SaveGameToMemory(Save,Bytes);}
  bool Full=true;for(Part P:FFoundrylineFactoryCatalog::AssemblyParts())Full&=Bin(M,Id,P)==M.StationTarget(M.State.WeldingStations[0],P);if(Full)break;}
 AddInfo(FString::Printf(TEXT("IN port order (part ids): %s after %.0f s plant time"),*FString::JoinBy(Order,TEXT(", "),[](int V){return FString::FromInt(V);}),M.State.Commerce.Clock-480));
 TestTrue(TEXT("Only bulky frame sections use the forklift IN port"),Seen.Num()==1&&Seen.Contains(int(Part::FrameSection)));TestEqual(TEXT("Three small ingredient types arrive by cart"),CartTypes.Num(),3);
 TestTrue(TEXT("Never more than one pallet on IN"),MaxOnIn<=1);TestTrue(TEXT("Never more than one delivery claims the IN dock"),MaxJobs<=1);
 TestTrue(TEXT("Bins hold two complete kits"),Bin(M,Id,Part::MountingPlate)==4&&Bin(M,Id,Part::Bracket)==8&&Bin(M,Id,Part::FrameSection)==12&&Bin(M,Id,Part::LightKit)==4);
 TestFalse(TEXT("IN port is clear after unloading"),M.State.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::StationIn;}));
 TestTrue(TEXT("Station reports supplies ready: ")+M.WeldingStationStatus(M.State.WeldingStations[0]),M.WeldingStationStatus(M.State.WeldingStations[0]).StartsWith(TEXT("Supplies ready")));
 TestTrue(TEXT("Exactly the delivered plates and brackets left the racks"),M.Available(Part::Bracket,Place::Rack)==12&&M.Available(Part::MountingPlate,Place::Rack)==6);
 TestTrue(TEXT("Material mass is conserved"),FMath::Abs(M.SteelMass()-M.State.ReceivedSteelLb-Mass)<.1f);
 TestEqual(TEXT("Station bin stock is not counted as free plant stock"),M.Total(Part::Bracket),12);
 TestTrue(TEXT("Full station validates: ")+E,M.Valid(E));
 for(int I=0;I<300;++I)M.Step(.1f);TestFalse(TEXT("A full station requests nothing more"),M.State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==Id;}));
 TestFalse(TEXT("A station holding supplies cannot be removed"),M.RemoveWeldingStation(Id,E));

 // The mid-delivery save restores and completes the same supplies.
 if(TestTrue(TEXT("Saved with a frame delivery on IN"),Saved))if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){
  FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Restored.Solids=M.Solids;TestTrue(TEXT("Mid-delivery save validates: ")+E,Restored.Valid(E));
  TestTrue(TEXT("Restored station keeps its recipe and batch"),Restored.State.WeldingStations[0].Recipe==1&&Restored.State.WeldingStations[0].BatchRemaining==10);
  for(int I=0;I<40000;++I){Restored.Step(.1f);bool Full=true;for(Part P:FFoundrylineFactoryCatalog::AssemblyParts())Full&=Bin(Restored,Id,P)==Restored.StationTarget(Restored.State.WeldingStations[0],P);if(Full)break;}
  for(Part P:FFoundrylineFactoryCatalog::AssemblyParts())TestEqual(TEXT("Restored plant fills the same bins"),Bin(Restored,Id,P),Bin(M,Id,P));}

 // Accepted bracket sales keep first claim on rack stock: the station only takes the surplus.
 {FFoundrylineFactorySimulation T;if(StarterStation(T,E)){const int Station=T.State.WeldingStations[0].Id;T.ConfigureWeldingStation(Station,1,10,E);RackStock(T,Part::Bracket,107,20,5);
  auto* O=T.State.Commerce.Orders.FindByPredicate([](const auto& V){return V.Part==uint8(Part::Bracket);});
  if(O){FFoundrylineCommerceCommand A;A.Action=Act::AcceptOrder;A.Id=O->Id;TestTrue(TEXT("Accept a bracket order: ")+E,T.CommerceCommand(A,E));
   TestEqual(TEXT("Stock promised to the order is not offered to the station"),T.StationRackSupply(Part::Bracket),FMath::Max(0,20-O->Quantity));
   if(O->Quantity>=20)TestTrue(TEXT("Station asks for more cutting instead"),T.WeldingStationStatus(T.State.WeldingStations[0]).Contains(TEXT("plasma"))||T.WeldingStationStatus(T.State.WeldingStations[0]).Contains(TEXT("buy")));}
  else AddError(TEXT("Starter offers no bracket order"));}}

 // Stopping the batch stops requests but keeps parts in the IN stock.
 TestTrue(TEXT("Stop the batch"),M.ConfigureWeldingStation(Id,0,0,E));TestEqual(TEXT("Stopped station targets nothing"),M.StationTarget(M.State.WeldingStations[0],Part::FrameSection),0);TestEqual(TEXT("Bins keep their parts"),Bin(M,Id,Part::FrameSection),12);
 {auto T=M;T.State.WeldingStations[0].BatchRemaining=5;TestFalse(TEXT("Batch without a recipe rejected"),T.Valid(E));}
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Part==Part::FrameSection)A.Quantity=13;TestFalse(TEXT("Overfilled bin rejected"),T.Valid(E));}
 // Schema 10 plant: zone part ledgers grow and saved stations join the route.
 {auto T=M;T.State.Version=10;T.State.SupplyBins.Reset();T.State.Collectors.Reset();for(auto& X:T.State.WeldingStations)X.Center-=FFoundrylineWeldingStationLayout::Schema14Shift(X);for(auto& Route:T.State.Routes)Route.Destinations.Remove(Id);T.MigrateWeldingStations();TestEqual(TEXT("Schema 10 migrates to 14"),T.State.Version,15);TestTrue(TEXT("Migrated station is a route destination"),T.State.Routes[0].Destinations.Contains(Id));TestTrue(TEXT("Migrated schema 10 plant validates: ")+E,T.Valid(E));}
 return true;
}
#endif
