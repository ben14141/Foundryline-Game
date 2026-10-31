#include "UI/FoundrylineERPViewModel.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Economy/FoundrylinePayroll.h"
namespace {
 int64 Add(int64 A,int64 B){if(B>0&&A>MAX_int64-B)return MAX_int64;if(B<0&&A<MIN_int64-B)return MIN_int64;return A+B;}
 int64 Sub(int64 A,int64 B){if(B>0&&A<MIN_int64+B)return MIN_int64;if(B<0&&A>MAX_int64+B)return MAX_int64;return A-B;}
 FString Cash(int64 C){return FFoundrylineCommerceCatalog::Money(C);}
 FString Date(double At){return FFoundrylineCommerceCatalog::Date(At);}
 FString BookingRef(int Id){return FString::Printf(TEXT("Booking #%d"),Id);}
 void Line(FFoundrylineERPDocument& D,const FString& Label,const FString& Value){D.Lines.Add({Label,Value});}
 const FFoundrylineCustomerOrder* FindOrder(const FFoundrylineCommerceState& C,int Id){return C.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;});}
 void Event(FFoundrylineERPWorkspace& W,FString Key,double At,FString Type,FString Caption,FString Detail,FString Reference,FString Party,int OrderId,int BookingId){FFoundrylineERPActivity A;A.Key=MoveTemp(Key);A.At=At;A.Type=MoveTemp(Type);A.Caption=MoveTemp(Caption);A.Detail=MoveTemp(Detail);A.Reference=MoveTemp(Reference);A.Party=MoveTemp(Party);A.OrderId=OrderId;A.BookingId=BookingId;W.Activity.Add(MoveTemp(A));}
}
FString FFoundrylineERPViewModel::CompanyName(const UWorld* World,FGuid CompanyId){const auto* R=AFoundrylineBusinessRegistry::Find(World);return R?R->GetCompanyName(CompanyId):FString();}
const TCHAR* FFoundrylineERPViewModel::DocumentKindName(EFoundrylineERPDocumentKind Kind){switch(Kind){case EFoundrylineERPDocumentKind::SalesOrder:return TEXT("Sales order");case EFoundrylineERPDocumentKind::PurchaseOrder:return TEXT("Steel purchase");case EFoundrylineERPDocumentKind::GoodsReceipt:return TEXT("Goods receipt");case EFoundrylineERPDocumentKind::CarrierBooking:return TEXT("Freight booking");case EFoundrylineERPDocumentKind::CustomerPayment:return TEXT("Customer payment");default:return TEXT("Ledger record");}}
FFoundrylineERPWorkspace FFoundrylineERPViewModel::Build(const FFoundrylineCommerceState& C,const FString& Name,int32 MaxGraphBins){
 FFoundrylineERPWorkspace W;W.CompanyName=Name;W.bAvailable=C.Enabled;W.GraphCaption=TEXT("Cash less outstanding costs / recent journal");if(!C.Enabled)return W;
 auto& S=W.Summary;S.CashCents=C.CashCents;S.OutstandingCostsCents=C.UnpaidCents;S.EarnedRevenueCents=C.EarnedRevenueCents;S.TotalSpendingCents=C.CostCents;
 for(const auto& O:C.Orders){
  S.AvailableOffers+=O.Status==EFoundrylineOrderState::Offered;S.ActiveOrders+=O.Status==EFoundrylineOrderState::Accepted;S.CompletedInHistory+=O.Status==EFoundrylineOrderState::Completed;
  if(O.Status==EFoundrylineOrderState::Offered)continue;
  S.UnfulfilledSalesCents=Add(S.UnfulfilledSalesCents,(O.Quantity-O.Delivered)*O.UnitPriceCents);S.DeliveredUnpaidCents=Add(S.DeliveredUnpaidCents,FMath::Max(int64(0),O.Delivered*O.UnitPriceCents-O.PaidCents));if(O.Payment==EFoundrylinePayment::Upfront)S.CustomerAdvancesCents=Add(S.CustomerAdvancesCents,(O.Quantity-O.Delivered)*O.UnitPriceCents);
  FFoundrylineERPDocument D;D.Kind=EFoundrylineERPDocumentKind::SalesOrder;D.Key=TEXT("sale:")+FString::FromInt(O.Id);D.Reference=O.Reference;D.Title=TEXT("Sales order");D.Party=O.Customer;D.OrderId=O.Id;D.Status=O.Status==EFoundrylineOrderState::Completed?(O.Late?TEXT("Completed / late"):TEXT("Completed")):O.Late?TEXT("Accepted / late"):TEXT("Accepted");D.bHasRecordedAt=true;D.RecordedAt=O.AcceptedAt;D.bHasAmount=true;D.AmountCents=O.Quantity*O.UnitPriceCents;D.AmountLabel=TEXT("Contract value");
  Line(D,TEXT("Customer"),O.Customer);Line(D,TEXT("Part"),FFoundrylineFactoryCatalog::Part(EFoundrylinePart(O.Part)).Name);Line(D,TEXT("Ordered quantity"),FString::FromInt(O.Quantity));Line(D,TEXT("Unit price"),Cash(O.UnitPriceCents));Line(D,TEXT("Accepted"),Date(O.AcceptedAt));Line(D,TEXT("Payment terms"),FFoundrylineCommerceCatalog::PaymentName(O.Payment));Line(D,TEXT("Deadline"),O.Deadline>0?Date(O.Deadline):TEXT("No deadline"));Line(D,TEXT("Delivered quantity"),FString::FromInt(O.Delivered));Line(D,TEXT("Payments recorded"),Cash(O.PaidCents));W.Documents.Add(MoveTemp(D));
  Event(W,TEXT("accepted:")+FString::FromInt(O.Id),O.AcceptedAt,TEXT("Sales"),TEXT("Order accepted"),O.Reference+TEXT(" / ")+O.Customer,O.Reference,O.Customer,O.Id,0);
 }
 for(const auto& B:C.Bookings){
  S.OpenBookings+=B.Status!=EFoundrylineBookingState::Delivered;const auto Carrier=FFoundrylineCommerceCatalog::Carrier(B.Carrier);const auto* O=FindOrder(C,B.OrderId);const FString Ref=BookingRef(B.Id);FFoundrylineERPDocument D;D.Kind=EFoundrylineERPDocumentKind::CarrierBooking;D.Key=TEXT("freight:")+FString::FromInt(B.Id);D.Reference=Ref;D.Title=B.Inbound?TEXT("Inbound freight booking"):TEXT("Customer pickup booking");D.Party=Carrier.Name;D.Status=FFoundrylineCommerceCatalog::BookingName(B.Status);D.OrderId=B.OrderId;D.BookingId=B.Id;D.bHasAmount=true;D.AmountCents=B.OriginalFeeCents;D.AmountLabel=TEXT("Original booking fee");
  Line(D,TEXT("Carrier"),Carrier.Name);Line(D,TEXT("Vehicle"),Carrier.Vehicle);Line(D,B.Inbound?TEXT("Supplier"):TEXT("Customer"),B.Customer);Line(D,TEXT("Scheduled arrival"),Date(B.ArrivalAt));Line(D,TEXT("Scheduled dispatch"),Date(B.DispatchAt));Line(D,TEXT("Original fee"),Cash(B.OriginalFeeCents));Line(D,TEXT("Booking time"),TEXT("Unavailable"));if(O)Line(D,TEXT("Sales order"),O->Reference);else if(B.OrderId)Line(D,TEXT("Sales order ID"),FString::FromInt(B.OrderId)+TEXT(" / history not retained"));if(B.BerthAt>0)Line(D,TEXT("Entered bay"),Date(B.BerthAt));if(B.DockedAt>0)Line(D,B.DockedAt>C.Clock?TEXT("Planned dock ready"):TEXT("Dock ready"),Date(B.DockedAt));if(B.DepartedAt>0)Line(D,TEXT("Departure started"),Date(B.DepartedAt));if(B.DeliveredAt>0)Line(D,B.Status==EFoundrylineBookingState::Delivered?B.Inbound?TEXT("Inbound carrier cleared bay"):TEXT("Customer delivery"):TEXT("Estimated customer delivery"),Date(B.DeliveredAt));int Quantity=0;for(const auto& I:B.Manifest)Quantity+=I.Quantity;Line(D,TEXT("Manifest quantity"),FString::FromInt(Quantity));W.Documents.Add(MoveTemp(D));
  // Bought assembly parts for a welding station: priced per unit, delivered to that station's IN port.
  FFoundrylineBoughtPart Bought;if(B.Inbound&&B.Part&&FFoundrylineFactoryCatalog::Bought(EFoundrylinePart(B.Part),&Bought)){
   FFoundrylineERPDocument P;P.Kind=EFoundrylineERPDocumentKind::PurchaseOrder;P.Key=TEXT("purchase:")+FString::FromInt(B.Id);P.Reference=Ref;P.Title=TEXT("Parts purchase record");P.Party=B.Customer;P.Status=B.SpawnedCargo?TEXT("Parts arrived"):TEXT("Delivery scheduled");P.BookingId=B.Id;P.bHasAmount=true;const int64 Material=B.Sheets*Bought.PriceCents;P.AmountCents=Add(Material,B.OriginalFeeCents);P.AmountLabel=TEXT("Parts and freight");
   Line(P,TEXT("Supplier"),B.Customer);Line(P,TEXT("Parts"),FFoundrylineCommerceCatalog::InboundLabel(B));Line(P,TEXT("Unit price"),Cash(Bought.PriceCents));Line(P,TEXT("Weight"),FString::Printf(TEXT("%.1f lb"),B.Sheets*Bought.UnitLb));Line(P,TEXT("Deliver to"),TEXT("Welding station IN port"));
Line(P,TEXT("Original freight fee"),Cash(B.OriginalFeeCents));Line(P,TEXT("Scheduled arrival"),Date(B.ArrivalAt));W.Documents.Add(MoveTemp(P));}
  else if(B.Inbound){
   FFoundrylineERPDocument P;P.Kind=EFoundrylineERPDocumentKind::PurchaseOrder;P.Key=TEXT("purchase:")+FString::FromInt(B.Id);P.Reference=Ref;P.Title=TEXT("Steel purchase record");P.Party=B.Customer;P.Status=B.SpawnedCargo?TEXT("Material arrived"):TEXT("Delivery scheduled");P.BookingId=B.Id;P.bHasAmount=true;const double Weight=B.Sheets*FFoundrylineSteelSheet::PoundsEach();const int64 Material=FMath::RoundToInt64(Weight*100);P.AmountCents=Add(Material,B.OriginalFeeCents);P.AmountLabel=TEXT("Initial purchase and freight");Line(P,TEXT("Supplier"),B.Customer);Line(P,TEXT("Sheet specification"),TEXT("60 x 120 x 1/8 in / carbon steel"));Line(P,TEXT("Whole sheets ordered"),FString::FromInt(B.Sheets));Line(P,TEXT("Steel weight"),FString::Printf(TEXT("%.2f lb"),Weight));Line(P,TEXT("Fixed material price"),TEXT("$1.00 / lb"));Line(P,TEXT("Material value"),Cash(Material));Line(P,TEXT("Original freight fee"),Cash(B.OriginalFeeCents));Line(P,TEXT("Scheduled arrival"),Date(B.ArrivalAt));Line(P,TEXT("Purchase time"),TEXT("Unavailable"));Line(P,TEXT("Rebooking charges"),TEXT("Listed in account journal"));W.Documents.Add(MoveTemp(P));
   if(B.SpawnedCargo){FFoundrylineERPDocument G;G.Kind=EFoundrylineERPDocumentKind::GoodsReceipt;G.Key=TEXT("receipt:")+FString::FromInt(B.Id);G.Reference=Ref;G.Title=TEXT("Raw steel receipt");G.Party=B.Customer;G.Status=B.Status==EFoundrylineBookingState::Departing||B.Status==EFoundrylineBookingState::Delivered?TEXT("Carrier unloaded"):TEXT("Arrived / unloading");G.BookingId=B.Id;double Received=0;int Sheets=0;for(const auto& I:B.Manifest){Received+=I.UnitLb*I.Quantity;Sheets+=I.Quantity;}Line(G,TEXT("Supplier"),B.Customer);Line(G,TEXT("Sheets arrived on carrier"),FString::FromInt(Sheets));Line(G,TEXT("Steel received at bay"),FString::Printf(TEXT("%.2f lb"),Received));Line(G,TEXT("Storage destination"),TEXT("Linked raw-material racking"));Line(G,TEXT("Receipt time"),TEXT("Unavailable"));Line(G,TEXT("Current location"),TEXT("See Inventory"));W.Documents.Add(MoveTemp(G));}
  }
  if(B.BerthAt>0)Event(W,TEXT("berth:")+FString::FromInt(B.Id),B.BerthAt,TEXT("Freight"),TEXT("Carrier entered loading bay"),Ref,Ref,Carrier.Name,B.OrderId,B.Id);
  if(B.DockedAt>0&&B.DockedAt<=C.Clock)Event(W,TEXT("docked:")+FString::FromInt(B.Id),B.DockedAt,TEXT("Freight"),TEXT("Loading bay dock ready"),Ref,Ref,Carrier.Name,B.OrderId,B.Id);
  if(B.DepartedAt>0)Event(W,TEXT("departure:")+FString::FromInt(B.Id),B.DepartedAt,TEXT("Freight"),TEXT("Carrier departure started"),Ref,Ref,Carrier.Name,B.OrderId,B.Id);
  if(B.Status==EFoundrylineBookingState::Delivered&&B.DeliveredAt>0)Event(W,TEXT("delivered:")+FString::FromInt(B.Id),B.DeliveredAt,TEXT("Freight"),B.Inbound?TEXT("Inbound carrier cleared bay"):TEXT("Customer delivery completed"),O?O->Reference:Ref,O?O->Reference:Ref,B.Inbound?FString(Carrier.Name):B.Customer,B.OrderId,B.Id);
 }
 int64 Net=Sub(C.CashCents,C.UnpaidCents);for(const auto& M:C.Money)Net=Sub(Net,M.Cents);W.OpeningRetainedNetCents=Net;
 for(int I=0;I<C.Money.Num();++I){const auto& M=C.Money[I];Net=Add(Net,M.Cents);W.Ledger.Add({I,M.At,M.Label,M.Cents,Net});FFoundrylineERPDocument D;const bool Advance=M.Label.StartsWith(TEXT("Customer advance / "));const bool Payment=M.Label.StartsWith(TEXT("Customer payment / "));D.Kind=Advance||Payment?EFoundrylineERPDocumentKind::CustomerPayment:EFoundrylineERPDocumentKind::LedgerEntry;D.Key=TEXT("ledger:")+FString::FromInt(I);D.Reference=FString::Printf(TEXT("Ledger record %d"),I+1);D.Title=Advance?TEXT("Customer advance receipt"):Payment?TEXT("Customer payment receipt"):TEXT("Ledger posting");D.Status=TEXT("Recorded");D.LedgerIndex=I;D.bHasRecordedAt=true;D.RecordedAt=M.At;D.bHasAmount=true;D.AmountCents=M.Cents;D.AmountLabel=TEXT("Ledger movement");Line(D,TEXT("Posting"),M.Label);Line(D,TEXT("Recorded at"),Date(M.At));Line(D,TEXT("Movement"),Cash(M.Cents));Line(D,TEXT("Net position after record"),Cash(Net));Line(D,TEXT("Net position basis"),TEXT("Cash less outstanding costs"));if(Advance||Payment){const FString Reference=M.Label.RightChop(19);if(const auto* O=C.Orders.FindByPredicate([&](const auto& V){return V.Reference==Reference;})){D.OrderId=O->Id;D.Party=O->Customer;D.Reference=Reference;Line(D,TEXT("Customer"),O->Customer);Line(D,TEXT("Sales order"),Reference);}else{D.Reference=Reference;Line(D,TEXT("Customer"),TEXT("Order history no longer retained"));}Line(D,TEXT("Receipt type"),Advance?TEXT("Upfront customer advance"):TEXT("Customer payment"));}W.Documents.Add(MoveTemp(D));
  Event(W,TEXT("money:")+FString::FromInt(I),M.At,TEXT("Ledger"),M.Label,FString(),W.Documents.Last().Reference,W.Documents.Last().Party,W.Documents.Last().OrderId,0);auto& A=W.Activity.Last();A.bHasAmount=true;A.AmountCents=M.Cents;
 }
 const int32 Limit=FMath::Clamp(MaxGraphBins,1,16);const int32 Size=FMath::Max(1,FMath::DivideAndRoundUp(W.Ledger.Num(),Limit));for(int I=0;I<W.Ledger.Num();I+=Size){FFoundrylineERPGraphBin B;B.FromAt=W.Ledger[I].At;B.ToAt=B.FromAt;B.OpeningNetCents=I?W.Ledger[I-1].NetAfterCents:W.OpeningRetainedNetCents;for(int J=I;J<FMath::Min(I+Size,W.Ledger.Num());++J){const auto& M=W.Ledger[J];B.ToAt=M.At;++B.EntryCount;if(M.MovementCents>=0)B.CreditsCents=Add(B.CreditsCents,M.MovementCents);else B.DebitsCents=Add(B.DebitsCents,M.MovementCents==MIN_int64?MAX_int64:-M.MovementCents);B.ClosingNetCents=M.NetAfterCents;}W.GraphBins.Add(B);}
 W.Activity.Sort([](const auto& A,const auto& B){return A.At==B.At?A.Key<B.Key:A.At>B.At;});return W;
}
FFoundrylineStarterGuide FFoundrylineERPViewModel::Guide(const FFoundrylineFactoryState& S){
 using G=EFoundrylineGuideStep;using Place=EFoundrylinePalletPlace;FFoundrylineStarterGuide Out;const auto& C=S.Commerce;Out.bAvailable=C.Enabled;if(!C.Enabled)return Out;
 const double SheetLb=FFoundrylineSteelSheet::PoundsEach();Out.CashCents=C.CashCents;
 auto Need=[&](const FString& Text,const FString& Action){Out.Problems.Add({Text,Action});};
 // Planning still needs a material estimate before a machine and its installed programs exist.
 const auto StandardPrograms=FFoundrylineFactoryCatalog::DefaultPrograms();
 auto SteelLb=[&](int32 Part){const auto* P=S.Programs.FindByPredicate([&](const auto& V){return V.Machine==1&&int(V.Part)==Part;});if(!P)P=StandardPrograms.FindByPredicate([&](const auto& V){return V.Machine==1&&int(V.Part)==Part;});return P?double(P->SteelLb):0.;};
 // Staffing: an operator assigned to the plasma (machine 1) and a driver on any owned forklift.
 for(const auto& E:FFoundrylineEmployeeRoster::Build(S)){
  if(E.Role==EFoundrylineEmployeeRole::Operator&&E.Assignment==1&&!Out.bOperator){Out.bOperator=true;Out.OperatorName=E.Label;}
  if(E.Role==EFoundrylineEmployeeRole::ForkliftDriver&&E.VehicleId&&!Out.bForkliftDriver){Out.bForkliftDriver=true;Out.DriverName=E.Label;}
  if(E.Role==EFoundrylineEmployeeRole::Welder&&E.Assignment&&S.WeldingStations.ContainsByPredicate([&](const auto& V){return V.Id==E.Assignment;})&&!Out.bWelder){Out.bWelder=true;Out.WelderName=E.Label;}
 }
 // The first welding station builds the business's finished products; its live status explains any hold-up.
 if(S.WeldingStations.Num()){FFoundrylineFactorySimulation View;View.State=S;const auto& St=S.WeldingStations[0];Out.StationId=St.Id;Out.StationName=FFoundrylineWeldingStationLayout::Name(St.Number);Out.StationStatus=View.WeldingStationStatus(St);}
 for(const auto& P:S.Pallets)if(P.Part==EFoundrylinePart::HeadacheRack){if(P.BookingId||P.Place==Place::Outbound||P.Place==Place::InTransit)Out.FinishedOnTrucks+=P.Quantity;else if(P.Place==Place::StationOut)Out.FinishedOnOut+=P.Quantity;else if(P.Place==Place::Pickup)Out.FinishedAtBayProducts+=P.Quantity;else Out.FinishedInStorage+=P.Quantity;}
 // Physical stock. Sheets on the receiving truck already count as on hand; booked pallets are already shipping.
 TMap<int32,int32> FreeByPart;
 for(const auto& P:S.Pallets){
  if(P.Place==Place::InTransit)continue;
  if(P.Part==EFoundrylinePart::Sheet){Out.SteelOnHandLb+=P.Quantity*P.UnitLb;continue;}
  if(P.BookingId||!FFoundrylineCommerceCatalog::Saleable(uint8(P.Part)))continue;
  // Welded products are counted with finished products (below), not as cut parts.
  // Kit parts already in a welding station (IN or its bins) are committed to racks, not stock for sale.
  if(P.Place==Place::StationIn||P.Place==Place::Buffer&&S.WeldingStations.ContainsByPredicate([&](const auto& V){return V.Id==P.Owner;}))continue;
  FreeByPart.FindOrAdd(int(P.Part))+=P.Quantity;if(P.Part==EFoundrylinePart::HeadacheRack)continue;Out.FinishedParts+=P.Quantity;if(P.Place==Place::Pickup)Out.FinishedAtBay+=P.Quantity;
 }
 const auto* Plasma=S.Machines.FindByPredicate([](const auto& M){return M.Id==1;});
 TMap<int32,int32> CuttingByPart;
 // Racks on the bench or still in a station's batch count as in progress, like parts on the plasma.
 for(const auto& St:S.WeldingStations){const int N=(St.Stage?1:0)+(St.Recipe==1?St.BatchRemaining:0);if(N)CuttingByPart.FindOrAdd(int(EFoundrylinePart::HeadacheRack))+=N;}
 if(Plasma){Out.SteelOnHandLb+=FMath::Max(0.f,Plasma->BedLb-Plasma->ReservedLb);if(!Plasma->Auto){Out.PartsInProgress=Plasma->BatchRemaining+(Plasma->Remaining>0?1:0);if(Out.PartsInProgress)CuttingByPart.Add(int(Plasma->Making),Out.PartsInProgress);}}
 for(const auto& B:C.Bookings){if(B.Status==EFoundrylineBookingState::Delivered)continue;if(B.Inbound){++Out.SteelDeliveriesDue;if(!B.SpawnedCargo&&!B.Part)Out.SteelIncomingLb+=B.Sheets*SheetLb;}
else ++Out.PickupsBooked;}
 // Accepted orders, focused order first: the one assigned to the loading bay, else the oldest accepted.
 TArray<const FFoundrylineCustomerOrder*> Accepted;
 for(const auto& O:C.Orders){Out.OffersWaiting+=O.Status==EFoundrylineOrderState::Offered;Out.CompletedOrders+=O.Status==EFoundrylineOrderState::Completed;if(O.Status!=EFoundrylineOrderState::Offered)Out.ReceivedCents+=O.PaidCents;if(O.Status==EFoundrylineOrderState::Accepted){Accepted.Add(&O);Out.OwedToYouCents+=FMath::Max(int64(0),O.Quantity*O.UnitPriceCents-O.PaidCents);}}
 Accepted.Sort([&](const auto& A,const auto& B){const bool AB=A.Id==C.PickupOrder,BB=B.Id==C.PickupOrder;if(AB!=BB)return AB;return A.AcceptedAt==B.AcceptedAt?A.Id<B.Id:A.AcceptedAt<B.AcceptedAt;});
 Out.AcceptedOrders=Accepted.Num();
 // Steel is needed for every accepted order's remaining work, after free finished stock and parts already cutting.
 // The cut in progress already holds its reserved steel, so only queued cuts still need sheet weight.
 int32 QueuedCuts=Plasma&&Plasma->Remaining>0?1:0;
 for(int I=0;I<Accepted.Num();++I){
  const auto& O=*Accepted[I];int OnTrucks=0;for(const auto& B:C.Bookings)if(!B.Inbound&&B.OrderId==O.Id&&B.Status!=EFoundrylineBookingState::Delivered)for(const auto& Item:B.Manifest)OnTrucks+=Item.Quantity;
  const int ToShip=FMath::Max(0,O.Quantity-O.Delivered-OnTrucks);int& Free=FreeByPart.FindOrAdd(O.Part);int& Cutting=CuttingByPart.FindOrAdd(O.Part);
  const int Ready=FMath::Min(Free,ToShip);Free-=Ready;const int InCut=FMath::Min(Cutting,ToShip-Ready);Cutting-=InCut;const int ToMake=ToShip-Ready-InCut;
  const int Started=FMath::Min(QueuedCuts,InCut);QueuedCuts-=Started;Out.SteelNeededLb+=(ToMake+InCut-Started)*SteelLb(O.Part);
  if(I)continue;
  Out.OrderId=O.Id;Out.OrderPart=O.Part;Out.OrderPartName=FFoundrylineFactoryCatalog::Part(EFoundrylinePart(O.Part)).Name;Out.OrderReference=O.Reference;Out.OrderCustomer=O.Customer;Out.OrderRemaining=O.Quantity-O.Delivered;
  Out.PartsToMake=ToMake;Out.PartsCutting=InCut;Out.PartsReady=Ready;Out.PartsOnTrucks=OnTrucks;Out.bAssignedToBay=C.PickupOrder==O.Id;Out.bWeldedOrder=O.Part==uint8(EFoundrylinePart::HeadacheRack);
  for(const auto& P:S.Pallets)if(int(P.Part)==O.Part&&P.Place==Place::Pickup&&!P.BookingId)Out.PartsAtBay+=P.Quantity;
 }
 Out.SheetsToBuy=FMath::Max(0,FMath::CeilToInt((Out.SteelNeededLb-Out.SteelOnHandLb-Out.SteelIncomingLb-.01)/SheetLb));
 const auto* Focus=Accepted.IsEmpty()?nullptr:Accepted[0];
 auto& Done=Out.bStepDone;
 Done[int(G::AcceptOrder)]=Focus&&Out.bAssignedToBay;Done[int(G::BuySteel)]=Focus&&Out.SheetsToBuy==0;Done[int(G::StaffWork)]=Out.bWeldedOrder?Out.bWelder&&Out.bForkliftDriver:Out.bOperator&&Out.bForkliftDriver;
 Done[int(G::ProgramPlasma)]=Focus&&Out.PartsToMake==0;Done[int(G::BookFreight)]=Focus&&Out.PartsToMake+Out.PartsCutting+Out.PartsReady==0;Done[int(G::GetPaid)]=false;
 Out.Step=G::GetPaid;for(int I=0;I<int(G::Count);++I)if(!Done[I]){Out.Step=G(I);break;}
 // Payroll runs every game hour, so idle paid people are a cost: point at the next order. A station idles with no batch and
 // nothing on the bench; the plasma with nothing to cut; a forklift driver with no job.
 Out.PayrollCentsPerHour=FFoundrylinePayroll::PlantCentsPerHour(S);
 if(Out.PayrollCentsPerHour>0){
  const bool StationIdle=S.WeldingStations.ContainsByPredicate([&](const auto& St){return !St.Stage&&(!St.Recipe||St.BatchRemaining<=0)&&S.Walkers.ContainsByPredicate([&](const auto& W){return W.Station==St.Id;});});
  const bool PlasmaIdle=Plasma&&Plasma->Remaining<=0&&Plasma->BatchRemaining<=0&&S.Operators.ContainsByPredicate([](const auto& O){return O.Machine==1;});
  const bool DriverIdle=S.Drivers.ContainsByPredicate([](const auto& D){return D.EmployeeId&&!D.Job;})&&!S.Jobs.Num();
  Out.IdleWhilePaid=StationIdle?TEXT("the welding station waits"):PlasmaIdle?TEXT("the plasma waits"):DriverIdle?TEXT("the forklift waits"):TEXT("");
 }
 const int64 SheetsCost=FMath::RoundToInt64(Out.SheetsToBuy*SheetLb*100)+2500;
 switch(Out.Step){
  case G::AcceptOrder:
   if(!Focus){Out.Title=Out.CompletedOrders?TEXT("Accept your next order"):TEXT("Accept a customer order");Out.Detail=TEXT("Pick a job the plasma cutter can make, such as bracket blanks or plates. The customer pays after the parts are delivered.");Out.ActionLabel=TEXT("Choose an order");Out.Action=TEXT("Page0");}
   else {Out.Title=TEXT("Send this order to the loading bay");Out.Detail=FString::Printf(TEXT("Finished %s for order %s will then be stacked at the loading bay, ready for a pickup truck."),*Out.OrderPartName,*Out.OrderReference);Out.ActionLabel=TEXT("Send to loading bay");Out.Action=FString::Printf(TEXT("AssignOrder%d"),Out.OrderId);}
   break;
  case G::BuySteel:Out.Title=FString::Printf(TEXT("Buy %d sheet%s of steel"),Out.SheetsToBuy,Out.SheetsToBuy==1?TEXT(""):TEXT("s"));Out.Detail=FString::Printf(TEXT("Your orders need about %.0f lb of steel. You have %.0f lb, and %.0f lb is on the way. One sheet weighs %.0f lb."),Out.SteelNeededLb,Out.SteelOnHandLb,Out.SteelIncomingLb,SheetLb);Out.ActionLabel=TEXT("Buy steel");Out.Action=FString::Printf(TEXT("BuySheets%d"),FMath::Clamp(Out.SheetsToBuy,1,10));break;
  case G::StaffWork:if(Out.bWeldedOrder){Out.Title=Out.bWelder?TEXT("Put a driver on the forklift"):TEXT("Assign a welder to the welding station");Out.Detail=TEXT("A welder builds each rack at the station, and a forklift driver takes the finished pallets from OUT to the loading bay or storage. Hire and assign them in Team.");Out.ActionLabel=TEXT("Open Team");Out.Action=TEXT("Page7");break;}
   Out.Title=TEXT("Put people on the plasma and forklift");Out.Detail=TEXT("Someone must run the plasma cutter, and a forklift driver unloads steel and moves finished parts. Assign them in Team.");Out.ActionLabel=TEXT("Open Team");Out.Action=TEXT("Page7");break;
  case G::ProgramPlasma:if(Out.bWeldedOrder){Out.Title=FString::Printf(TEXT("Set %s: make %d %s"),Out.StationName.IsEmpty()?TEXT("a welding station"):*Out.StationName,Out.PartsToMake,*Out.OrderPartName.ToLower());Out.Detail=TEXT("The station orders its frame sections and light kits from the panel, takes plates and brackets from your racks, and builds one rack at a time. Racks already made or in progress are not counted again.");Out.ActionLabel=TEXT("Set the station");Out.Action=FString::Printf(TEXT("StationOrder%d"),Out.OrderId);break;}
   Out.Title=FString::Printf(TEXT("Program the plasma: %d x %s"),Out.PartsToMake,*Out.OrderPartName);Out.Detail=TEXT("This fills in the part and quantity on the plasma screen. Check it, then press Start. Parts already made or cutting are not counted again.");Out.ActionLabel=TEXT("Program plasma");Out.Action=FString::Printf(TEXT("ProgramOrder%d"),Out.OrderId);break;
  case G::BookFreight:
   if(Out.PartsAtBay>0&&Out.PartsAtBay>=Out.PartsReady+Out.PartsCutting){Out.Title=TEXT("Book a pickup truck");Out.Detail=FString::Printf(TEXT("%d %s are waiting at the loading bay. Open the loading tablet, tick the pallets and book a truck that fits them.%s"),Out.PartsAtBay,*Out.OrderPartName,Out.bWeldedOrder?TEXT(" A 6 ft 6 in rack needs a flatbed or larger, and each rack pallet takes two deck spaces."):TEXT(""));Out.ActionLabel=TEXT("Open loading tablet");Out.Action=TEXT("Tablet");}
   else if(Out.bWeldedOrder){Out.Title=Out.PartsCutting?FString::Printf(TEXT("Welding racks: %d left"),Out.PartsCutting):TEXT("Moving racks to the loading bay");Out.Detail=TEXT("The welder builds each rack, and the forklift takes its OUT pallet to the loading bay. Speed up time with 4x or 8x at the top.");Out.ActionLabel=TEXT("View production");Out.Action=TEXT("Page10");}
   else {Out.Title=Out.PartsCutting?FString::Printf(TEXT("Cutting parts: %d left"),Out.PartsCutting):TEXT("Moving parts to the loading bay");Out.Detail=TEXT("The operator cuts the parts and the forklift stacks them at the loading bay. Speed up time with 4x or 8x at the top.");Out.ActionLabel=TEXT("View production");Out.Action=TEXT("Page10");}
   break;
  default:
   // Nothing pending on the focused order while paid people sit idle: the next step is another order.
   if(!Out.IdleWhilePaid.IsEmpty()&&(!Focus||Out.PartsToMake+Out.PartsCutting+Out.PartsReady==0)){Out.Title=TEXT("Accept another order");Out.Detail=FString::Printf(TEXT("Payroll %s/h is running while %s. Overlap orders so your people keep working."),*FFoundrylineCommerceCatalog::Money(Out.PayrollCentsPerHour),*Out.IdleWhilePaid);Out.ActionLabel=TEXT("Find an order");Out.Action=TEXT("Page0");break;}
   Out.Title=TEXT("Truck is on its way - payment follows");Out.Detail=FString::Printf(TEXT("%s: %s once the truck delivers. Nothing else to do for this order; try 8x speed."),*Out.OrderCustomer,FFoundrylineCommerceCatalog::PaymentName(Focus->Payment));Out.ActionLabel=TEXT("Track shipment");Out.Action=TEXT("Page4");break;
 }
 // Everything missing right now, in plain language, each with the screen that fixes it.
 // When the step already points at Orders, the payroll reason leads its detail; otherwise it is listed as a problem.
 if(!Out.IdleWhilePaid.IsEmpty()&&Out.Action==TEXT("Page0")&&!Out.Detail.StartsWith(TEXT("Payroll")))Out.Detail=FString::Printf(TEXT("Payroll %s/h is running while %s. "),*FFoundrylineCommerceCatalog::Money(Out.PayrollCentsPerHour),*Out.IdleWhilePaid)+Out.Detail;
 if(!Out.IdleWhilePaid.IsEmpty()&&Out.Action!=TEXT("Page0"))Need(FString::Printf(TEXT("Payroll %s/h is running while %s. Accept another order."),*FFoundrylineCommerceCatalog::Money(Out.PayrollCentsPerHour),*Out.IdleWhilePaid),TEXT("Page0"));
 if(!Out.bOperator&&!Out.bWeldedOrder)Need(TEXT("No one is running the plasma cutter"),TEXT("Page7"));
 if(Out.bWeldedOrder&&!Out.bWelder)Need(TEXT("No welder at the welding station"),TEXT("Page7"));
 // A station held up (missing parts, OUT full, no welder at work) says why, in its own words.
 if(Out.bWeldedOrder&&(Out.StationStatus.Contains(TEXT("Missing"))||Out.StationStatus.Contains(TEXT("OUT full"))||Out.StationStatus.Contains(TEXT("paused"))||Out.StationStatus.Contains(TEXT("forklift route"))||Out.StationStatus.Contains(TEXT("no free rack slot"))))Need(Out.StationName+TEXT(": ")+Out.StationStatus,TEXT("Page10"));
 if(!Out.bForkliftDriver)Need(TEXT("No forklift driver: steel can't be unloaded or parts moved"),TEXT("Page7"));
 if(Focus&&!Out.bWeldedOrder&&Out.SheetsToBuy>0&&Out.Step!=G::BuySteel)Need(FString::Printf(TEXT("Not enough steel: need %.0f lb, have %.0f lb (+%.0f lb coming)"),Out.SteelNeededLb,Out.SteelOnHandLb,Out.SteelIncomingLb),FString::Printf(TEXT("BuySheets%d"),FMath::Clamp(Out.SheetsToBuy,1,10)));
 if(Focus&&Out.SheetsToBuy>0&&C.CashCents<SheetsCost)Need(FString::Printf(TEXT("Not enough cash for %d sheets (%s)"),Out.SheetsToBuy,*FFoundrylineCommerceCatalog::Money(SheetsCost)),TEXT("Page5"));
 if(Focus&&!Out.bAssignedToBay&&Out.Step!=G::AcceptOrder)Need(TEXT("This order is not at the loading bay yet"),FString::Printf(TEXT("AssignOrder%d"),Out.OrderId));
 if(Focus&&!S.Running)Need(TEXT("Time is paused: press Resume at the top"),TEXT("Run"));
 if(Focus&&Focus->Late)Need(TEXT("This order is past its deadline"),FString::Printf(TEXT("LinkedOrder%d"),Out.OrderId));
 return Out;
}
