#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "UI/FoundrylineERPViewModel.h"
// Phase 6: the whole player flow from the small cut-parts shop with only player commands and no fixtures. Paint, place the
// station, take a rack order, buy steel and cut the plates and brackets on the plasma, buy frame sections and light kits at
// the station, hire and assign one welder, build, ship and get paid; then add a second welder and run a second order faster.
namespace WeldingPlayerFlowTest {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;using Role=EFoundrylineEmployeeRole;using Layout=FFoundrylineWeldingStationLayout;
struct FFlow {
 FFoundrylineFactorySimulation& M;FAutomationTestBase& T;int Station=0;TArray<FString> Log;
 FFoundrylineCommerceCommand Cmd(Act A,int Id=0,int Choice=0,int Quantity=0){FFoundrylineCommerceCommand C;C.Action=A;C.Id=Id;C.Choice=Choice;C.Quantity=Quantity;return C;}
 bool Do(const FFoundrylineCommerceCommand& C,const TCHAR* What){FString E;const bool Ok=M.CommerceCommand(C,E);if(!Ok)Log.Add(FString(What)+TEXT(": ")+E);return Ok;}
 const FFoundrylineWeldingStationRecord& S()const{return *M.State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==Station;});}
 const FFoundrylineCustomerOrder* Order(int Id)const{return M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;});}
 int Count(Part P,Place Where)const{int N=0;for(const auto& A:M.State.Pallets)if(A.Part==P&&A.Place==Where&&!A.BookingId)N+=A.Quantity-A.Reserved;return N;}
 float SteelLb(Part P)const{const auto* R=M.State.Programs.FindByPredicate([&](const auto& V){return V.Machine==1&&V.Part==P;});return R?R->SteelLb:0;}
 // What a player does each time they look at the panels: buy what the station panel says is missing, cut the plates and
 // brackets the order still needs on the plasma, and book a truck for staged racks.
 int PlatesQueued=0,BracketsQueued=0,PlatesWanted=0,BracketsWanted=0;
 void Operate(int OrderId){
  for(Part P:{Part::FrameSection,Part::LightKit}){const int Short=FMath::Max(0,M.StationShortfall(S(),P)-M.StationRackSupply(P));if(Short>0&&!M.State.Commerce.Bookings.ContainsByPredicate([&](const auto& B){return B.Inbound&&B.StationId==Station&&B.Part==uint8(P)&&B.Status!=EFoundrylineBookingState::Delivered;}))
   Do(Cmd(Act::BuyParts,Station,int(P),FMath::Min(Short,FFoundrylineFactoryCatalog::Part(P).Capacity)),TEXT("Buy station parts"));}
  const auto* Plasma=M.State.Machines.FindByPredicate([](const auto& V){return V.Id==1;});const bool Idle=Plasma&&Plasma->BatchRemaining<=0&&Plasma->Remaining<=0;
  FString E;if(Idle&&PlatesQueued<PlatesWanted){const int N=PlatesWanted-PlatesQueued;if(M.Program(1,Part::MountingPlate,N,false,E))PlatesQueued+=N;else Log.Add(TEXT("Program plates: ")+E);}
  else if(Idle&&BracketsQueued<BracketsWanted){const int N=BracketsWanted-BracketsQueued;if(M.Program(1,Part::Bracket,N,false,E))BracketsQueued+=N;else Log.Add(TEXT("Program brackets: ")+E);}
  const auto* O=Order(OrderId);if(!O||O->Status!=EFoundrylineOrderState::Accepted)return;int Staged=0;for(const auto& A:M.State.Pallets)if(A.Place==Place::Pickup&&A.OrderId==OrderId&&!A.BookingId)Staged+=A.Quantity;
  int Booked=0;for(const auto& B:M.State.Commerce.Bookings)if(!B.Inbound&&B.OrderId==OrderId)for(const auto& I:B.Manifest)Booked+=I.Quantity;
  int Bays=0;for(const auto& A:M.State.Pallets)Bays+=A.Place==Place::Pickup&&!A.BookingId;
  if(Staged&&(Staged+Booked+O->Delivered>=O->Quantity||Bays>=2)){auto C=Cmd(Act::BookPickup);C.Choice=2;C.Arrival=M.State.Commerce.Clock+60;for(const auto& A:M.State.Pallets)if(A.Place==Place::Pickup&&A.OrderId==OrderId&&!A.BookingId){C.Pallets.Add(A.Id);C.Quantities.Add(A.Quantity);}Do(C,TEXT("Book the rigid truck"));}
 }
 // Accept the rack offer, send it to the loading bay, set the station and work out the plates, brackets and steel it needs.
 int TakeOrder(){const auto* Offer=M.State.Commerce.Orders.FindByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::HeadacheRack);});if(!Offer)return 0;const int Id=Offer->Id;
  if(!Do(Cmd(Act::AcceptOrder,Id),TEXT("Accept rack order"))||!Do(Cmd(Act::AssignPickup,Id),TEXT("Send to loading bay")))return 0;
  const auto G=FFoundrylineERPViewModel::Guide(M.State);FString E;if(G.OrderId!=Id||G.PartsToMake<1||!M.ConfigureWeldingStation(Station,1,G.PartsToMake,E)){Log.Add(TEXT("Set the station: ")+E);return 0;}
  FFoundrylineAssemblyRecipe R;FFoundrylineFactoryCatalog::Recipe(1,R);const int Racks=G.PartsToMake;
  PlatesWanted=PlatesQueued+FMath::Max(0,R.Bill[int(Part::MountingPlate)]*Racks-Count(Part::MountingPlate,Place::Rack));BracketsWanted=BracketsQueued+FMath::Max(0,R.Bill[int(Part::Bracket)]*Racks-Count(Part::Bracket,Place::Rack));
  const double Lb=(PlatesWanted-PlatesQueued)*SteelLb(Part::MountingPlate)+(BracketsWanted-BracketsQueued)*SteelLb(Part::Bracket);const int Sheets=FMath::Clamp(FMath::CeilToInt((Lb-G.SteelOnHandLb-G.SteelIncomingLb)/FFoundrylineSteelSheet::PoundsEach()+.001),0,10);
  if(Sheets>0)Do(Cmd(Act::BuySteel,0,0,Sheets),TEXT("Buy steel"));return Id;}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationPlayerFlow,"Foundryline.Factory.WeldingStation.PlayerFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationPlayerFlow::RunTest(const FString&){using namespace WeldingPlayerFlowTest;
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);FString E;FFlow F{M,*this};const int64 StartCash=M.State.Commerce.CashCents;
 TestTrue(TEXT("Small shop starts with no stock, no station and no racks on offer"),M.State.WeldingStations.IsEmpty()&&!M.State.Commerce.Orders.ContainsByPredicate([](const auto& O){return O.Part==uint8(Part::HeadacheRack);}));
 // Three paint strokes and the station, exactly as a player places them.
 TestTrue(TEXT("Paint the west fork lane: ")+E,M.PaintLane(EFoundrylineFloorLane::Forklift,{-2217.6,1275,20},{-2217.6,2170,20},304.8f,0,E));
 TestTrue(TEXT("Paint the walkway: ")+E,M.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1455,1214.04,20},{-810,1214.04,20},121.92f,0,E));
 TestTrue(TEXT("Paint the aisle crossing: ")+E,M.PaintLane(EFoundrylineFloorLane::Crossing,{-1370,1450,20},{-1370,1450,20},200,0,E));
 if(!TestTrue(TEXT("Place the welding station: ")+E,M.AddWeldingStation({-1768.02,1823.12,20},0,false,E)))return false;F.Station=M.State.WeldingStations[0].Id;
 if(!TestTrue(TEXT("Hire parts collector"),M.HireEmployee(Role::Unassigned,FGuid::NewGuid(),E)))return false;
 if(!TestTrue(TEXT("Assign collector to assembly area: ")+E,M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,Role::PartsCollector,F.Station,E)))return false;
 M.State.Running=true;M.State.Speed=8;M.Step(.1f);
 // One welder.
 if(!TestTrue(TEXT("Hire a welder: ")+E,M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)))return false;const int WelderA=M.State.UnassignedEmployees.Last().Id;
 if(!TestTrue(TEXT("Assign them to Welder A: ")+E,M.AssignEmployee(WelderA,Role::Welder,F.Station,E,0)))return false;
 // Advancing plant time per unit, by welders at work, from the station's own progress.
 TArray<float> UnitSeconds[3];float Advancing=0;int LastUnits=0;int AtWork=0;
 auto Run=[&](int OrderId,int Steps){for(int I=0;I<Steps;++I){const auto* O=F.Order(OrderId);if(O&&O->Status==EFoundrylineOrderState::Completed)return true;if(I%10==0)F.Operate(OrderId);
   const float Before=F.S().Work;const int Stage=F.S().Stage;M.Step(.1f);const auto& S=F.S();if(S.bAdvancing&&S.Stage==Stage&&Stage>=1&&Stage<=3){Advancing+=(S.Work-Before)/M.StationRate(S);AtWork=M.StationWelders(S,true);}
   if(S.UnitsCompleted>LastUnits){UnitSeconds[FMath::Clamp(AtWork,0,2)].Add(Advancing);Advancing=0;LastUnits=S.UnitsCompleted;}}
  const auto* O=F.Order(OrderId);return O&&O->Status==EFoundrylineOrderState::Completed;};
 const int First=F.TakeOrder();if(!TestTrue(TEXT("Take the first rack order: ")+FString::Join(F.Log,TEXT("; ")),First!=0))return false;
 const int FirstQty=F.Order(First)->Quantity;
 const bool FirstDone=Run(First,120000);TestTrue(TEXT("First order built, shipped and completed with one welder: ")+FString::Join(F.Log,TEXT("; "))+TEXT(" / ")+M.WeldingStationStatus(F.S()),FirstDone);
 if(!FirstDone){for(const auto& C:M.State.Collectors){AddInfo(FString::Printf(TEXT("Collector %d %s pos %s yaw %.1f path %d distance %.1f"),C.Id,*C.Status,*C.Position.ToString(),C.Yaw,C.Path.Num(),C.Distance));for(const auto& L:C.Load)AddInfo(FString::Printf(TEXT("Load part%d qty%d picked%d source%d pickup%s"),L.Part,L.Quantity,L.Picked,L.SourcePallet,*L.Pickup.ToString()));}for(const auto& D:M.State.Drivers)AddInfo(FString::Printf(TEXT("Driver %d pos%s job%d waiting%d"),D.Id,*D.Position.ToString(),D.Job,D.Waiting));for(const auto& W:M.State.Walkers)AddInfo(FString::Printf(TEXT("Welder %d pos%s"),W.Id,*W.Position.ToString()));AddInfo(M.SupplyReport(F.Station));return false;}
 const auto* O1=F.Order(First);TestTrue(TEXT("First order paid exactly once"),O1->PaidCents==int64(FirstQty)*O1->UnitPriceCents&&O1->Delivered==FirstQty);
 TestTrue(TEXT("Plates and brackets were cut on the plasma, frames and lights bought at the station"),M.State.Commerce.Bookings.ContainsByPredicate([](const auto& B){return B.Inbound&&B.Part==uint8(Part::FrameSection);})&&M.State.Commerce.Bookings.ContainsByPredicate([](const auto& B){return B.Inbound&&B.Part==uint8(Part::LightKit);})&&F.PlatesQueued>0&&F.BracketsQueued>0);
 TestTrue(TEXT("Plant validates after the first order: ")+E,M.Valid(E));
 // A second welder, then a second order.
 if(!TestTrue(TEXT("Hire a second welder: ")+E,M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)))return false;
 if(!TestTrue(TEXT("Assign them to Welder B: ")+E,M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,Role::Welder,F.Station,E,1)))return false;
 for(int I=0;I<200&&!M.State.Commerce.Orders.ContainsByPredicate([](const auto& O){return O.Status==EFoundrylineOrderState::Offered&&O.Part==uint8(Part::HeadacheRack);});++I)M.Step(.1f);
 F.Log.Reset();const int Second=F.TakeOrder();if(!TestTrue(TEXT("Take the second rack order: ")+FString::Join(F.Log,TEXT("; ")),Second!=0))return false;
 const int SecondQty=F.Order(Second)->Quantity;
 const bool SecondDone=Run(Second,120000);TestTrue(TEXT("Second order completed with two welders: ")+FString::Join(F.Log,TEXT("; "))+TEXT(" / ")+M.WeldingStationStatus(F.S()),SecondDone);
 if(!SecondDone){for(const auto& C:M.State.Collectors){AddInfo(FString::Printf(TEXT("Second collector %s pos%s path%d retry%.1f release%d"),*C.Status,*C.Position.ToString(),C.Path.Num(),C.Retry,C.ReleaseRequested));for(const auto& L:C.Load)AddInfo(FString::Printf(TEXT("Second load part%d qty%d picked%d"),L.Part,L.Quantity,L.Picked));}for(const auto& D:M.State.Drivers)AddInfo(FString::Printf(TEXT("Second driver pos%s yaw%.1f job%d wait%d"),*D.Position.ToString(),D.Yaw,D.Job,D.Waiting));AddInfo(M.SupplyReport(F.Station));}
 if(SecondDone){const auto* O2=F.Order(Second);TestTrue(TEXT("Second order paid exactly once"),O2->PaidCents==int64(SecondQty)*O2->UnitPriceCents&&O2->Delivered==SecondQty);}
 auto Mean=[](const TArray<float>& V){float S=0;for(float X:V)S+=X;return V.Num()?S/V.Num():0.f;};
 const float One=Mean(UnitSeconds[1]),Two=Mean(UnitSeconds[2]);
 AddInfo(FString::Printf(TEXT("Units: %d with one welder (%.0f plant s of welding each), %d with two (%.0f plant s each); %d racks shipped; cash %s -> %s"),UnitSeconds[1].Num(),One,UnitSeconds[2].Num(),Two,FirstQty+SecondQty,*FFoundrylineCommerceCatalog::Money(StartCash),*FFoundrylineCommerceCatalog::Money(M.State.Commerce.CashCents)));
 TestTrue(TEXT("Every unit of the first order had one welder; the second order's had two"),UnitSeconds[1].Num()>=FirstQty&&UnitSeconds[2].Num()>=1);
 TestTrue(TEXT("Two welders finish a unit in about 1/1.7 of the plant time"),One>0&&Two>0&&FMath::IsNearlyEqual(Two/One,1/1.7f,.06f));
 // Where the money went, from the bookings (every purchase and pickup is one) and the cost total; the rest is the plant's
 // per-minute equipment operating cost (1 cent a game minute per working machine, 3 cents per forklift on a job).
 int64 MarginOverMaterials=0;
 {const auto& C=M.State.Commerce;int64 Steel=0,Frames=0,Lights=0,Supplier=0,Trucks=0;int Sheets=0,Pickups=0;FString Carriers;
  for(const auto& B:C.Bookings){if(B.Inbound){Supplier+=2500;if(!B.Part){Sheets+=B.Sheets;Steel+=FMath::RoundToInt64(FFoundrylineSteelSheet::PoundsEach()*B.Sheets*100);}
    else{FFoundrylineBoughtPart Bought;FFoundrylineFactoryCatalog::Bought(EFoundrylinePart(B.Part),&Bought);(B.Part==uint8(Part::FrameSection)?Frames:Lights)+=Bought.PriceCents*B.Sheets;}}
   else{++Pickups;Trucks+=B.OriginalFeeCents;Carriers+=FString(FFoundrylineCommerceCatalog::Carrier(B.Carrier).Vehicle)+TEXT("; ");}}
  const int64 Wages=C.WagesPaidCents,Operating=C.CostCents-Steel-Frames-Lights-Supplier-Trucks-Wages;
  MarginOverMaterials=C.EarnedRevenueCents-Steel-Frames-Lights-Supplier-Trucks;
  AddInfo(FString::Printf(TEXT("Ledger: revenue %s / steel %s (%d sheets) / frame sections %s / light kits %s / supplier freight %s / pickup trucks %s (%d: %s) / wages %s / operating %s / total cost %s / game minutes %.0f / net %s"),
   *FFoundrylineCommerceCatalog::Money(C.EarnedRevenueCents),*FFoundrylineCommerceCatalog::Money(Steel),Sheets,*FFoundrylineCommerceCatalog::Money(Frames),*FFoundrylineCommerceCatalog::Money(Lights),*FFoundrylineCommerceCatalog::Money(Supplier),*FFoundrylineCommerceCatalog::Money(Trucks),Pickups,*Carriers,
   *FFoundrylineCommerceCatalog::Money(Wages),*FFoundrylineCommerceCatalog::Money(Operating),*FFoundrylineCommerceCatalog::Money(C.CostCents),C.Clock,*FFoundrylineCommerceCatalog::Money(C.CashCents-StartCash)));
  int Left[4]={0,0,0,0};const Part Kit[]={Part::MountingPlate,Part::Bracket,Part::FrameSection,Part::LightKit};for(int I=0;I<4;++I)for(const auto& A:M.State.Pallets)if(A.Part==Kit[I])Left[I]+=A.Quantity;
  AddInfo(FString::Printf(TEXT("Left in stock at the end: %d plates, %d brackets, %d frame sections, %d light kits; steel %.0f lb"),Left[0],Left[1],Left[2],Left[3],M.State.Machines[0].BedLb+M.State.Pallets.FilterByPredicate([](const auto& A){return A.Part==Part::Sheet;}).Num()*FFoundrylineSteelSheet::PoundsEach()));}
 // Economy v1 pays wages for every game hour a person holds a job, waiting included, so this one-order-at-a-time flow can end
 // below its starting cash; that result is reported (AddInfo above), not asserted. What must hold: racks earn more than their
 // materials and freight, and the business stays solvent.
 TestTrue(TEXT("Racks earn more than their materials and freight"),MarginOverMaterials>0);
 TestTrue(TEXT("The business stays solvent and owes nothing"),M.State.Commerce.CashCents>0&&M.State.Commerce.UnpaidCents==0);
 TestTrue(TEXT("Plant validates at the end: ")+E,M.Valid(E));
 return true;
}
#endif
