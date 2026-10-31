#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Logistics/FoundrylineRackAccess.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace EconomyBenchmark {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Act=EFoundrylineCommerceAction;
using Obj=TSharedPtr<FJsonObject>;
struct Mark {int64 Cash,Cost,Revenue,Wages,Accrued;double Clock;};
Mark At(const FFoundrylineFactorySimulation& M){const auto& C=M.State.Commerce;return {C.CashCents,C.CostCents,C.EarnedRevenueCents,C.WagesPaidCents,C.PayrollAccruedCents,C.Clock};}
double RawLb(const FFoundrylineFactorySimulation& M){double N=0;for(const auto& P:M.State.Pallets)if(P.Part==Part::Sheet)N+=P.Quantity*P.UnitLb;for(const auto& A:M.State.Machines)N+=A.BedLb;return N;}
Obj Delta(const FFoundrylineFactorySimulation& M,Mark B){auto J=MakeShared<FJsonObject>();const auto A=At(M);J->SetNumberField(TEXT("cash_before_cents"),B.Cash);J->SetNumberField(TEXT("cash_after_cents"),A.Cash);J->SetNumberField(TEXT("cash_gain_cents"),A.Cash-B.Cash);J->SetNumberField(TEXT("cost_cents"),A.Cost-B.Cost);J->SetNumberField(TEXT("revenue_cents"),A.Revenue-B.Revenue);J->SetNumberField(TEXT("wages_cents"),A.Wages-B.Wages);J->SetNumberField(TEXT("wages_earned_cents"),A.Wages-B.Wages+A.Accrued-B.Accrued);J->SetNumberField(TEXT("economic_cost_cents"),A.Cost-B.Cost+A.Accrued-B.Accrued);J->SetNumberField(TEXT("game_minutes"),A.Clock-B.Clock);J->SetNumberField(TEXT("raw_steel_left_lb"),RawLb(M));J->SetNumberField(TEXT("raw_steel_asset_cents"),FMath::RoundToInt64(RawLb(M)*FFoundrylineEconomyTuning::SteelCentsPerLb));J->SetNumberField(TEXT("payroll_accrued_cents"),A.Accrued);J->SetNumberField(TEXT("net_after_payroll_accrual_cents"),A.Cash-B.Cash-(A.Accrued-B.Accrued));J->SetNumberField(TEXT("unpaid_cents"),M.State.Commerce.UnpaidCents);return J;}
struct Runner {
 FAutomationTestBase& T;FFoundrylineFactorySimulation M;FString Error;TSet<int> Seen;int BlankTrips=0,BlankUnits=0,MaxBlankBatch=0;
 explicit Runner(FAutomationTestBase& In):T(In){M.SeedCutPartsShop();M.State.Running=true;M.State.Speed=8;}
 bool Good(bool B,const FString& Label){if(!B)T.AddError(Label+TEXT(": ")+Error);return B;}
 bool Command(FFoundrylineCommerceCommand C){return Good(M.CommerceCommand(C,Error),TEXT("Actual commerce command"));}
 bool TickUntil(TFunctionRef<bool()> Done,const FString& Label){for(int I=0;I<24000&&!Done();++I){M.Step(.1f);for(const auto& J:M.State.Jobs)if(!Seen.Contains(J.Id)){Seen.Add(J.Id);if(J.Part==Part::Bracket&&(J.Destination==2||M.State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==J.Destination&&S.MachineId==2;}))){++BlankTrips;BlankUnits+=J.Quantity;MaxBlankBatch=FMath::Max(MaxBlankBatch,J.Quantity);}}}return Good(Done(),Label+TEXT(" / ")+M.State.Status);}
 int Accept(Part P,int Quantity=0){const auto* O=M.State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Part==uint8(P)&&V.Status==EFoundrylineOrderState::Offered&&(!Quantity||V.Quantity==Quantity)&&!V.Rush&&V.Payment!=EFoundrylinePayment::Upfront;});if(!O){Error=TEXT("No ordinary actual offered order");Good(false,TEXT("Offer"));return 0;}const int Id=O->Id;FFoundrylineCommerceCommand C;C.Action=Act::AcceptOrder;C.Id=Id;return Command(C)?Id:0;}
 const FFoundrylineCustomerOrder* Order(int Id)const{return M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;});}
 bool Steel(double NeedLb){const int Sheets=FMath::Max(0,FMath::CeilToInt((NeedLb-RawLb(M)-.0001)/FFoundrylineSteelSheet::PoundsEach()));if(!Sheets)return true;FFoundrylineCommerceCommand C;C.Action=Act::BuySteel;C.Quantity=Sheets;return Command(C);}
 bool Queue(int Id){const auto* O=Order(Id);if(!O)return false;if(!Good(M.QueueMachineJob(1,Id,0,Error),TEXT("Actual plasma queue")))return false;return O->Part!=uint8(Part::BentBracket)||Good(M.QueueMachineJob(2,Id,0,Error),TEXT("Actual brake queue"));}
 bool Ship(int Id){const auto* O=Order(Id);if(!O)return false;const Part P=Part(O->Part);const int Quantity=O->Quantity;
  if(!TickUntil([&](){int N=0;for(const auto& A:M.State.Pallets)if(A.Part==P&&!A.Reserved&&!A.BookingId&&A.Place==Place::Rack)N+=A.Quantity;return N>=Quantity;},TEXT("Real production completes")))return false;
  // Stage actual available stock; a partial final pallet remains valid.
  TArray<int> Pallets;for(const auto& A:M.State.Pallets)if(A.Part==P&&!A.Reserved&&!A.BookingId&&A.Place==Place::Rack)Pallets.Add(A.Id);
  int ToStage=Quantity;for(int Pallet:Pallets){if(ToStage<=0)break;const auto* Stock=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Id==Pallet;});if(!Stock)continue;const int Take=FMath::Min(ToStage,Stock->Quantity);FFoundrylineCommerceCommand C;C.Action=Act::StagePallet;C.Id=Id;C.Pallets={Pallet};if(!Command(C))return false;ToStage-=Take;}
  if(!TickUntil([&](){int N=0;for(const auto& A:M.State.Pallets)if(FFoundrylineRackAccess::Ready(M.State,A)&&A.OrderId==Id)N+=A.Quantity;return N==Quantity;},TEXT("Forklift physically stages order")))return false;
  FFoundrylineCommerceCommand C;C.Action=Act::BookPickup;int Spaces=0;double Weight=0;for(const auto& A:M.State.Pallets)if(FFoundrylineRackAccess::Ready(M.State,A)&&A.OrderId==Id){C.Pallets.Add(A.Id);C.Quantities.Add(A.Quantity);Spaces+=FFoundrylineCommerceCatalog::PalletSpaces(uint8(A.Part));Weight+=FFoundrylineCommerceCatalog::PalletWeight(uint8(A.Part),A.Quantity,A.UnitLb);}
  int Carrier=0;for(;Carrier<5;++Carrier){const auto S=FFoundrylineCommerceCatalog::Carrier(Carrier);if(S.Spaces>=Spaces&&S.PayloadLb>=Weight&&FFoundrylineCommerceCatalog::CarrierFits(Carrier,uint8(P)))break;}if(!Good(Carrier<5,TEXT("A real carrier fits")))return false;
  C.Choice=Carrier;C.Arrival=M.State.Commerce.Clock+FFoundrylineCommerceCatalog::Carrier(Carrier).LeadMinutes+10;if(!Command(C))return false;
  if(!TickUntil([&](){const auto* A=Order(Id);return A&&A->Status==EFoundrylineOrderState::Completed;},TEXT("Customer delivery/payment")))return false;
  return Good(M.Valid(Error),TEXT("Completed ledger validates"))&&Good(M.State.Commerce.UnpaidCents==0,TEXT("No unpaid operating costs"));
 }
 bool Brake(){FFoundrylineCommerceCommand C;C.Action=Act::BuyBrake;if(!Command(C))return false;if(!Good(M.HireEmployee(EFoundrylineEmployeeRole::Unassigned,FGuid::NewGuid(),Error),TEXT("Hire brake employee")))return false;return Good(M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,EFoundrylineEmployeeRole::Operator,2,Error),TEXT("Assign paid brake operator"));}
 Obj Report(Mark B){auto J=Delta(M,B);J->SetNumberField(TEXT("brake_feed_jobs"),BlankTrips);J->SetNumberField(TEXT("brake_feed_units"),BlankUnits);J->SetNumberField(TEXT("largest_brake_feed_batch"),MaxBlankBatch);J->SetNumberField(TEXT("average_brake_feed_batch"),BlankTrips?double(BlankUnits)/BlankTrips:0);return J;}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyProgressionBenchmark,"Foundryline.Economy.ProgressionBenchmark",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEconomyProgressionBenchmark::RunTest(const FString&){
 using namespace EconomyBenchmark;auto Root=MakeShared<FJsonObject>();Root->SetStringField(TEXT("units"),TEXT("Integer cents, game minutes, steel pounds. Prices/times/wages unchanged by this test."));
 Root->SetNumberField(TEXT("bracket_price_cents"),FFoundrylineEconomyTuning::SalePriceCents(Part::Bracket));Root->SetNumberField(TEXT("plate_price_cents"),FFoundrylineEconomyTuning::SalePriceCents(Part::MountingPlate));Root->SetNumberField(TEXT("formed_price_cents"),FFoundrylineEconomyTuning::SalePriceCents(Part::BentBracket));
 bool Ok=true;TArray<int> SerialIds;TArray<TSharedPtr<FJsonValue>> Jobs;
 Runner Serial(*this);const Mark Start=At(Serial.M);int FirstAffordable=0;
 for(int I=0;I<8;++I){const Mark Before=At(Serial.M);Part Next=I==0?Part::Bracket:I<3?Part::MountingPlate:I%2?Part::MountingPlate:Part::Bracket;
  // The first three remain identical to native play. Later jobs use an actual
  // available ordinary basic offer rather than requiring an unavailable recipe.
  if(I>=3&&!Serial.M.State.Commerce.Orders.ContainsByPredicate([&](const auto& O){return O.Part==uint8(Next)&&O.Status==EFoundrylineOrderState::Offered&&!O.Rush&&O.Payment!=EFoundrylinePayment::Upfront;})){
   const auto* Alternative=Serial.M.State.Commerce.Orders.FindByPredicate([](const auto& O){return (O.Part==uint8(Part::Bracket)||O.Part==uint8(Part::MountingPlate))&&O.Status==EFoundrylineOrderState::Offered&&!O.Rush&&O.Payment!=EFoundrylinePayment::Upfront;});if(Alternative)Next=Part(Alternative->Part);
  }
  const int Id=Serial.Accept(Next,I==0?40:I<3?60:0);if(!Id){Ok=false;break;}SerialIds.Add(Id);const auto Order=*Serial.Order(Id);const double Steel=Order.Quantity*(Order.Part==uint8(Part::MountingPlate)?2:5);
  if(!Serial.Steel(Steel)||!Serial.Queue(Id)||!Serial.Ship(Id)){Ok=false;break;}auto J=Serial.Report(Before);J->SetNumberField(TEXT("quantity"),Order.Quantity);J->SetNumberField(TEXT("part"),Order.Part);J->SetNumberField(TEXT("order_id"),Id);Jobs.Add(MakeShared<FJsonValueObject>(J));
  if(!FirstAffordable&&Serial.M.State.Commerce.CashCents-Serial.M.State.Commerce.PayrollAccruedCents>=FFoundrylineEconomyTuning::PressBrakeCents+100000)FirstAffordable=I+1;
  if(I==2)Root->SetObjectField(TEXT("serial_first_three"),Serial.Report(Start));
 }
 Root->SetArrayField(TEXT("serial_jobs"),Jobs);Root->SetObjectField(TEXT("serial_eight_total"),Serial.Report(Start));Root->SetNumberField(TEXT("first_job_count_affording_brake_plus_1000_reserve"),FirstAffordable);
 TestTrue(TEXT("Real starter progression starts with default $4000"),Start.Cash==400000);
 // Assert the agreed tuning targets against real purchases, freight and payroll.
 if(Jobs.Num()>=3){const auto Three=Root->GetObjectField(TEXT("serial_first_three"));const double Average=Three->GetNumberField(TEXT("net_after_payroll_accrual_cents"))/3;Root->SetNumberField(TEXT("ordinary_first_three_average_net_cents"),Average);Root->SetBoolField(TEXT("target_600_to_800_average_net_met"),Average>=60000&&Average<=80000);TestTrue(TEXT("Ordinary first three jobs average $600-$800 net cash"),Average>=60000&&Average<=80000);}
 Root->SetBoolField(TEXT("target_brake_after_3_to_5_jobs_with_1000_reserve_met"),FirstAffordable>=3&&FirstAffordable<=5);
 TestTrue(TEXT("Eight genuine ordinary basic orders remain playable"),Jobs.Num()==8);
 TestTrue(TEXT("Brake becomes affordable after 3-5 jobs with $1000 reserve"),FirstAffordable>=3&&FirstAffordable<=5);
 Runner Queued(*this);const Mark QStart=At(Queued.M);TArray<int> QIds;double TotalSteel=0;
 for(int I=0;I<3;++I){const int Id=Queued.Accept(I==0?Part::Bracket:Part::MountingPlate,I==0?40:60);if(!Id){Ok=false;break;}QIds.Add(Id);const auto* O=Queued.Order(Id);TotalSteel+=O->Quantity*(O->Part==uint8(Part::MountingPlate)?2:5);}
 if(QIds.Num()==3&&Queued.Steel(TotalSteel)){for(int Id:QIds)if(!Queued.Queue(Id))Ok=false;for(int Id:QIds)if(!Queued.Ship(Id)){Ok=false;break;}}else Ok=false;
 Root->SetObjectField(TEXT("continuously_queued_three"),Queued.Report(QStart));
 if(Jobs.Num()>=3&&QIds.Num()==3){int SerialUnits=0,QueuedUnits=0;for(int I=0;I<3;++I)SerialUnits+=int(Jobs[I]->AsObject()->GetNumberField(TEXT("quantity")));for(int Id:QIds)if(const auto* O=Queued.Order(Id))QueuedUnits+=O->Quantity;Root->SetNumberField(TEXT("serial_first_three_units"),SerialUnits);Root->SetNumberField(TEXT("queued_three_units"),QueuedUnits);TestEqual(TEXT("Serial and queued comparison completes identical 160 parts"),QueuedUnits,SerialUnits);TestEqual(TEXT("Serial and queued comparison invoices match"),Queued.M.State.Commerce.EarnedRevenueCents,int64(Root->GetObjectField(TEXT("serial_first_three"))->GetNumberField(TEXT("revenue_cents"))));}
 // Controlled upgrade comparison is clearly separate from the real starter progression above.
 double BlankRate=0,FormedRate=0;
 for(int Upgrade=0;Upgrade<2;++Upgrade){
  Runner Control(*this);Control.M.State.Commerce.CashCents=1000000;const Mark CapitalStart=At(Control.M);
  bool Ready=!Upgrade||Control.Brake();const Mark WarmupStart=At(Control.M);
  const int64 UpgradeCapital=WarmupStart.Cost-CapitalStart.Cost;
  TArray<TSharedPtr<FJsonValue>> WarmupJobs;
  if(Upgrade&&Ready){
   Control.M.RefreshCommerceOffers();
   auto FormedAvailable=[&](){return Control.M.State.Commerce.Orders.ContainsByPredicate([](const auto& O){return O.Part==uint8(Part::BentBracket)&&O.Quantity==30&&O.Status==EFoundrylineOrderState::Offered&&!O.Rush&&O.Payment!=EFoundrylinePayment::Upfront;});};
   for(int Warmup=0;Warmup<8&&Ready&&!FormedAvailable();++Warmup){
    // Finish genuine ordinary basic contracts to advance offer generation without
    // filling the six-active-contract limit or accepting free customer advances.
    const auto* Offer=Control.M.State.Commerce.Orders.FindByPredicate([](const auto& O){return (O.Part==uint8(Part::Bracket)||O.Part==uint8(Part::MountingPlate))&&O.Status==EFoundrylineOrderState::Offered&&!O.Rush&&O.Payment!=EFoundrylinePayment::Upfront;});
    if(!Offer){Ready=Control.Good(false,TEXT("Actual ordinary warmup offer exists"));break;}
    const Part P=Part(Offer->Part);const int Quantity=Offer->Quantity;const Mark Before=At(Control.M);const int WarmupId=Control.Accept(P,Quantity);
    Ready=WarmupId&&Control.Steel(Quantity*(P==Part::MountingPlate?2:5))&&Control.Queue(WarmupId)&&Control.Ship(WarmupId);
    auto W=Control.Report(Before);W->SetNumberField(TEXT("quantity"),Quantity);W->SetNumberField(TEXT("part"),uint8(P));W->SetNumberField(TEXT("order_id"),WarmupId);WarmupJobs.Add(MakeShared<FJsonValueObject>(W));
   }
   Ready&=Control.Good(FormedAvailable(),TEXT("Ordinary 30 formed-bracket offer generated after completed warmups"));
  }
  const Mark OperatingStart=At(Control.M);const double StartingRaw=RawLb(Control.M);const int Id=Ready?Control.Accept(Upgrade?Part::BentBracket:Part::Bracket,Upgrade?30:40):0;
  if(Id){const auto Order=*Control.Order(Id);Ready=Control.Steel(Order.Quantity*5)&&Control.Queue(Id)&&Control.Ship(Id);auto J=Control.Report(OperatingStart);
   J->SetStringField(TEXT("fixture"),TEXT("CONTROL ONLY: $10000 initial cash; genuine ordinary orders (40 blank / 30 formed), compared by inventory-adjusted net per game minute. Paid upgrade capital and completed offer-generation warmups excluded from measured order margin and reported separately. Raw-steel assets show any warmup inventory benefit."));
   J->SetNumberField(TEXT("quantity"),Order.Quantity);J->SetNumberField(TEXT("upgrade_capital_cents"),UpgradeCapital);
   J->SetArrayField(TEXT("completed_warmup_jobs"),WarmupJobs);
   // This separate mark excludes the measured order from the warmup figures.
   auto Warmup=MakeShared<FJsonObject>();Warmup->SetNumberField(TEXT("cash_gain_cents"),OperatingStart.Cash-WarmupStart.Cash);Warmup->SetNumberField(TEXT("cost_cents"),OperatingStart.Cost-WarmupStart.Cost);Warmup->SetNumberField(TEXT("revenue_cents"),OperatingStart.Revenue-WarmupStart.Revenue);Warmup->SetNumberField(TEXT("wages_cents"),OperatingStart.Wages-WarmupStart.Wages);Warmup->SetNumberField(TEXT("game_minutes"),OperatingStart.Clock-WarmupStart.Clock);J->SetObjectField(TEXT("warmup_financial_delta"),Warmup);
   J->SetNumberField(TEXT("starting_raw_steel_lb"),StartingRaw);J->SetNumberField(TEXT("cash_gain_including_capital_and_warmups_cents"),Control.M.State.Commerce.CashCents-CapitalStart.Cash);
   const auto End=At(Control.M);const double Minutes=FMath::Max(1.,End.Clock-OperatingStart.Clock);const double Rate=(End.Cash-OperatingStart.Cash-(End.Accrued-OperatingStart.Accrued)+FMath::RoundToInt64((RawLb(Control.M)-StartingRaw)*FFoundrylineEconomyTuning::SteelCentsPerLb))/Minutes;
   J->SetNumberField(TEXT("operating_net_cents_per_game_minute"),Rate);
   J->SetNumberField(TEXT("inventory_adjusted_net_cents"),End.Cash-OperatingStart.Cash-(End.Accrued-OperatingStart.Accrued)+FMath::RoundToInt64((RawLb(Control.M)-StartingRaw)*FFoundrylineEconomyTuning::SteelCentsPerLb));
   if(Upgrade)FormedRate=Rate;else BlankRate=Rate;Root->SetObjectField(Upgrade?TEXT("formed_upgrade_control"):TEXT("blank_control"),J);
  }else Ready=false;
  Ok&=Ready;
 }
 Root->SetBoolField(TEXT("formed_improves_operating_net_per_game_minute"),FormedRate>BlankRate);Root->SetBoolField(TEXT("all_real_flows_completed"),Ok);
 TestTrue(TEXT("Paid brake/operator improve inventory-adjusted net per game minute"),FormedRate>BlankRate&&BlankRate>0);
 const FString Folder=FPaths::ProjectSavedDir()/TEXT("Logs");IFileManager::Get().MakeDirectory(*Folder,true);FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));TestTrue(TEXT("Write actual economy benchmark JSON"),FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("EconomyProgressionBenchmark.json"))));
 AddInfo(FString::Printf(TEXT("Actual benchmark: first brake+reserve job %d; blank %.2f vs formed %.2f cents/game minute; see Saved/Logs/EconomyProgressionBenchmark.json"),FirstAffordable,BlankRate,FormedRate));
 return Ok;
}
#endif
