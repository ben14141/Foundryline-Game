#include "FoundrylineOperationFeedback.h"
#include "FoundrylineUISurface.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace FoundrylineOperationFeedback {
namespace {
using FactoryAction=EFoundrylineFactoryAction;using CommerceAction=EFoundrylineCommerceAction;
struct FLocalFeedback {FString Message,SeenError,SeenConfirm;double At=-100,SeenConfirmAt=-100,SeenErrorAt=-100,PendingAt=-100;bool Failed=false,Pending=false;};
TMap<TWeakObjectPtr<AFoundrylinePlayerController>,FLocalFeedback> LocalFeedback;
const TCHAR* Equipment(int Kind){return Kind==1?TEXT("plasma cutter"):Kind==2?TEXT("press brake"):Kind==3?TEXT("sheet roller"):TEXT("forklift");}
}
bool FactoryHasReceipt(const FFoundrylineFactoryCommand& C){return C.Action==FactoryAction::BuyStoredMachine||C.Action==FactoryAction::BuyPlanningEquipment||C.Action==FactoryAction::AddWeldingStation||C.Action==FactoryAction::EditPlanningItem||C.Action==FactoryAction::HireEmployee||C.Action==FactoryAction::AssignEmployee;}
FString FactoryReceipt(const FFoundrylineFactoryCommand& C,const FFoundrylineFactoryState& S){
 if(C.Action==FactoryAction::BuyStoredMachine){const int Kind=C.Id;const int Stored=Kind==4?S.UnplacedForklifts:S.Machines.FilterByPredicate([&](const auto& M){return M.Kind()==Kind&&M.bStored;}).Num();return FString::Printf(TEXT("Purchased %s for %s. %d owned and waiting to be placed. Open Planning and choose it from the catalogue."),Equipment(Kind),*FFoundrylineCommerceCatalog::Money(FFoundrylinePlanningEquipment::Price(Kind==4?3:Kind+3,1)),Stored);}
 if(C.Action==FactoryAction::HireEmployee){const FString Name=S.UnassignedEmployees.IsEmpty()?FString(TEXT("New team member")):S.UnassignedEmployees.Last().DisplayName;return Name+TEXT(" hired. Open Team / People & jobs to assign a machine or forklift.");}
 if(C.Action==FactoryAction::AssignEmployee){const auto People=FFoundrylineEmployeeRoster::Build(S);const auto* E=People.FindByPredicate([&](const auto& V){return V.Id==C.Id;});return E?E->Assignment?FString::Printf(TEXT("%s assigned: %s."),*E->Label,*E->CurrentJob):E->Label+TEXT(" is now unassigned and available for another job."):FString(TEXT("Employee assignment updated."));}
 if(C.Action==FactoryAction::EditPlanningItem){if(C.Flag)return C.Count==1?TEXT("Machine moved to owned equipment storage. Place it again from the planning catalogue."):C.Count==7?TEXT("Pallet scrapped. Its material remains accounted for in the stock ledger."):TEXT("Item removed from this business layout.");return TEXT("Layout updated. The item's position, rotation and connected workspace have been saved to this business.");}
 if(C.Action==FactoryAction::AddWeldingStation)return TEXT("Fabrication workstation placed. Assign a welder in Team to begin assembly.");
 if(C.Action==FactoryAction::BuyPlanningEquipment){if(C.Id==7)return TEXT("Office workstation purchased and placed. Walk to its computer and press E.");if(C.Id==3)return TEXT("Owned forklift placed. Assign its driver in Team / People & jobs.");if(C.Id>=4)return FString::Printf(TEXT("Owned %s placed. Assign an operator in Team to run it."),Equipment(C.Id-3));if(C.Id==0)return TEXT("Fabrication workstation purchased and placed. Assign a welder in Team.");return FString::Printf(TEXT("%d-level %s rack purchased and placed. Its pallet slots and loading aisle are ready."),FMath::Clamp(C.Count,1,3),C.Id==2?TEXT("finished-goods"):TEXT("parts"));}
 return TEXT("Business action confirmed.");
}
FString CommerceReceipt(const FFoundrylineCommerceCommand& C,const FFoundrylineFactoryState& S){
 const auto& Finance=S.Commerce;const auto* Order=Finance.Orders.FindByPredicate([&](const auto& O){return O.Id==C.Id;});const auto* Booking=Finance.Bookings.IsEmpty()?nullptr:&Finance.Bookings.Last();
 if(C.Action==CommerceAction::BuySteel&&Booking){const int64 Material=FMath::RoundToInt64(FFoundrylineSteelSheet::PoundsEach()*Booking->Sheets*FFoundrylineEconomyTuning::SteelCentsPerLb);return FString::Printf(TEXT("Ordered %d steel sheet%s. Total %s including freight. Arrival: %s. Track the delivery in Trucks."),Booking->Sheets,Booking->Sheets==1?TEXT(""):TEXT("s"),*FFoundrylineCommerceCatalog::Money(Material+Booking->OriginalFeeCents),*FFoundrylineCommerceCatalog::Date(Booking->ArrivalAt));}
 if(C.Action==CommerceAction::BookPickup&&Booking)return FString::Printf(TEXT("Pickup booked with %s: %d pallet%s, fee %s. Arrival: %s. View the booking in Trucks."),FFoundrylineCommerceCatalog::Carrier(Booking->Carrier).Name,Booking->Manifest.Num(),Booking->Manifest.Num()==1?TEXT(""):TEXT("s"),*FFoundrylineCommerceCatalog::Money(Booking->OriginalFeeCents),*FFoundrylineCommerceCatalog::Date(Booking->ArrivalAt));
 if(C.Action==CommerceAction::Reschedule){const auto* B=Finance.Bookings.FindByPredicate([&](const auto& V){return V.Id==C.Id;});if(B)return FString::Printf(TEXT("Truck rescheduled to %s. Rebooking fee: %s. Check the calendar in Trucks."),*FFoundrylineCommerceCatalog::Date(B->ArrivalAt),*FFoundrylineCommerceCatalog::Money(B->OriginalFeeCents/2));}
 if(C.Action==CommerceAction::AcceptOrder&&Order)return FString::Printf(TEXT("Contract %s accepted: %d x %s. %s"),*Order->Reference,Order->Quantity,FFoundrylineFactoryCatalog::Part(EFoundrylinePart(Order->Part)).Name,*(Order->Deadline>0?TEXT("Due ")+FFoundrylineCommerceCatalog::Date(Order->Deadline):FString(TEXT("No delivery deadline."))));
 if(C.Action==CommerceAction::StagePallet){const auto* J=S.Jobs.FindByPredicate([&](const auto& V){return V.OrderId==C.Id&&V.DestinationPlace==EFoundrylinePalletPlace::Pickup&&C.Pallets.Contains(V.SourcePallet);});return J?FString::Printf(TEXT("Forklift requested: %d matching parts to the loading bay for %s. Book the carrier once staged."),J->Quantity,Order?*Order->Reference:TEXT("this contract")):FString(TEXT("Pallet movement requested for the selected contract."));}
 if(C.Action==CommerceAction::AssignPickup)return TEXT("Contract assigned to this loading bay. This assignment does not create parts or book a truck.");
 if(C.Action==CommerceAction::BuyParts&&Booking)return FString::Printf(TEXT("Ordered %s. Arrival: %s. Track the delivery in Trucks."),*FFoundrylineCommerceCatalog::InboundLabel(*Booking),*FFoundrylineCommerceCatalog::Date(Booking->ArrivalAt));
 if(C.Action==CommerceAction::DriverPriority)return TEXT("Forklift task preferences saved. They apply after the current trip.");
 return TEXT("Business request confirmed.");
}
void BeginFactory(AFoundrylinePlayerController* P,const FFoundrylineFactoryCommand& C){if(P&&P->IsLocalController()&&FactoryHasReceipt(C)){auto& F=LocalFeedback.FindOrAdd(P);F.Pending=true;F.PendingAt=P->GetWorld()->GetRealTimeSeconds();P->OperationsStatus.Reset();}}
void FinishFactory(AFoundrylinePlayerController* P){if(P)if(auto* F=LocalFeedback.Find(P))F->Pending=false;}
void Draw(AFoundrylineBuildHUD* HUD,AFoundrylinePlayerController* P,UCanvas* Canvas){
 if(!HUD||!P||!Canvas||!P->IsLocalController())return;auto* Draw=static_cast<AHUD*>(HUD);for(auto It=LocalFeedback.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();auto& F=LocalFeedback.FindOrAdd(P);const double Now=P->GetWorld()->GetRealTimeSeconds();
 // Time controls use replicated state; purchases/hiring/layout use explicit accepted-server receipts.
 if(P->OperationsWatch==3){if(auto* Plant=P->FactoryPlant();Plant&&Plant->Snapshot.Running==(P->OperationsWatchTarget!=0))P->ConfirmOperations(Plant->Snapshot.Running?TEXT("Time resumed."):TEXT("Time paused."));}
 else if(P->OperationsWatch==4){if(auto* Plant=P->FactoryPlant();Plant&&FMath::RoundToInt(Plant->Snapshot.Speed)==P->OperationsWatchTarget)P->ConfirmOperations(FString::Printf(TEXT("Speed set to %dx."),P->OperationsWatchTarget));}
 if(P->OperationsWatch&&Now-P->OperationsWatchAt>10)P->OperationsWatch=0;
 if(!P->OperationsConfirm.IsEmpty()&&(P->OperationsConfirmAt!=F.SeenConfirmAt||P->OperationsConfirm!=F.SeenConfirm)){F.SeenConfirm=P->OperationsConfirm;F.SeenConfirmAt=P->OperationsConfirmAt;F.Message=P->OperationsConfirm;F.At=Now;F.Failed=false;F.Pending=false;}
 if(!P->OperationsStatus.IsEmpty()&&(P->OperationsStatus!=F.SeenError||P->OperationsStatusAt>F.SeenErrorAt)){F.SeenError=P->OperationsStatus;F.SeenErrorAt=P->OperationsStatusAt;F.Message=P->OperationsStatus;F.At=Now;F.Failed=true;F.Pending=false;}else if(P->OperationsStatus.IsEmpty())F.SeenError.Reset();
 const bool Pending=(F.Pending&&Now-F.PendingAt<12)||P->bOperationsPending;const double Age=Now-F.At;if(!Pending&&(F.Message.IsEmpty()||Age>12))return;
 const float Scale=FMath::Clamp(FMath::Min(Canvas->ClipX/1920.f,Canvas->ClipY/1080.f),.75f,1.25f),W=FMath::Min(610.f*Scale,Canvas->ClipX-32*Scale),High=126*Scale,X=Canvas->ClipX-W-24*Scale,Y=Canvas->ClipY-High-22*Scale,Fade=Pending?1.f:FMath::Clamp(float(12-Age)/2,0.f,1.f);
 auto Color=[&](const TCHAR* Hex){auto C=FLinearColor(FColor::FromHex(Hex));C.A*=Fade;return C;};const auto Ink=Color(TEXT("1F2327")),Paper=Color(TEXT("F7F5F1")),Accent=Color(F.Failed&&!Pending?TEXT("B23B36"):TEXT("F07A2F"));
 FoundrylineUISurface::Panel(Canvas,X,Y,W,High,Scale,F.Failed&&!Pending?Paper:Ink,F.Failed&&!Pending?Paper:Color(TEXT("2D3237")),Accent,3,9);Draw->DrawRect(Accent,X+1*Scale,Y+13*Scale,3*Scale,High-26*Scale);
 Draw->DrawText(Pending?TEXT("REQUEST SENT"):F.Failed?TEXT("ACTION NOT COMPLETED"):TEXT("CONFIRMED"),Accent,X+20*Scale,Y+16*Scale,GEngine->GetMediumFont(),1.05f*Scale);
 const FString Message=Pending?FString(TEXT("Waiting for the business server to confirm your request...")):F.Message;TArray<FString> Words;Message.ParseIntoArray(Words,TEXT(" "));FString Line;int Row=0;const auto TextColor=F.Failed&&!Pending?Ink:Paper;
 for(const auto& Word:Words){const FString Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float TW=0,TH=0;Draw->GetTextSize(Next,TW,TH,GEngine->GetMediumFont(),Scale);if(TW>W-40*Scale&&!Line.IsEmpty()){Draw->DrawText(Line,TextColor,X+20*Scale,Y+(46+Row*21)*Scale,GEngine->GetMediumFont(),Scale);Line=Word;if(++Row>=3)break;}else Line=Next;}if(!Line.IsEmpty()&&Row<3)Draw->DrawText(Line,TextColor,X+20*Scale,Y+(46+Row*21)*Scale,GEngine->GetMediumFont(),Scale);
}
}
