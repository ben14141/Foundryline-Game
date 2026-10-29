#include "FoundrylineNetworkTestTools.h"
#include "UI/FoundrylinePlayerController.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Networking/FoundrylineOperationsState.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {FString Json(const TSharedRef<FJsonObject>& J){FString S;auto W=TJsonWriterFactory<>::Create(&S);FJsonSerializer::Serialize(J,W);return S;}}
bool UFoundrylineNetworkTestTools::QueueCommerceAction(APlayerController* Player,const FString& Action){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action](float){if(Weak.IsValid())Weak->OperationsAction(FName(*Action));return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueueCommerceForeignTarget(APlayerController* Player,FGuid PlantId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,PlantId](float){if(Weak.IsValid()){FFoundrylineCommerceCommand C;C.Action=EFoundrylineCommerceAction::BuySteel;C.Quantity=1;C.PlantId=PlantId;Weak->bOperationsPending=true;Weak->ServerOperationsCommand(C,++Weak->OperationsSequence);}return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::AdvanceCommerceTest(APlayerController* Host,int32 Steps){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Host);if(!PC||!PC->HasAuthority()||PC->GetWorld()->WorldType!=EWorldType::PIE||Steps<1||Steps>2000)return false;auto* P=PC->FactoryPlant();if(!P||!P->Model.State.Commerce.Enabled)return false;
 // Exercise real simulation ticks and its real account synchronisation. No free inventory.
 for(int I=0;I<Steps;++I)P->Tick(.1f);return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::CommerceReport(APlayerController* Player){auto J=MakeShared<FJsonObject>();
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||PC->GetWorld()->WorldType!=EWorldType::PIE)return TEXT("{}");auto* P=PC->FactoryPlant();if(!P)return TEXT("{}");const auto& C=P->Snapshot.Commerce;
 J->SetStringField(TEXT("company"),P->BusinessCompanyId.ToString());J->SetStringField(TEXT("plant"),P->BusinessPlantId.ToString());J->SetStringField(TEXT("digest"),P->GetLedgerDigest());J->SetBoolField(TEXT("enabled"),C.Enabled);J->SetNumberField(TEXT("cash"),C.CashCents);J->SetNumberField(TEXT("cost"),C.CostCents);J->SetNumberField(TEXT("revenue"),C.EarnedRevenueCents);J->SetNumberField(TEXT("clock"),C.Clock);J->SetNumberField(TEXT("pickup_order"),C.PickupOrder);J->SetBoolField(TEXT("pending"),PC->bOperationsPending);J->SetStringField(TEXT("status"),PC->OperationsStatus);J->SetBoolField(TEXT("running"),P->Snapshot.Running);J->SetNumberField(TEXT("jobs"),P->Snapshot.Jobs.Num());
 if(PC->HasAuthority()){FString Error;J->SetBoolField(TEXT("authoritative_valid"),P->Model.Valid(Error));J->SetStringField(TEXT("validation_error"),Error);J->SetNumberField(TEXT("mass_delta"),P->Model.SteelMass()-P->Model.State.ReceivedSteelLb);const auto* R=AFoundrylineBusinessRegistry::Find(PC->GetWorld());const auto* Company=R?R->GetLedger().Company(P->BusinessCompanyId):nullptr;J->SetBoolField(TEXT("cash_matches_registry"),Company&&Company->BalanceCents==P->Model.State.Commerce.CashCents);}
 TArray<TSharedPtr<FJsonValue>> Orders,Bookings,Pallets,Private,Drivers,Machines;
 for(const auto& D:P->Snapshot.Drivers){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),D.Id);V->SetNumberField(TEXT("job"),D.Job);V->SetNumberField(TEXT("x"),D.Position.X);V->SetNumberField(TEXT("y"),D.Position.Y);V->SetNumberField(TEXT("z"),D.Position.Z);V->SetNumberField(TEXT("path_points"),D.Path.Num());V->SetBoolField(TEXT("waiting"),D.Waiting);Drivers.Add(MakeShared<FJsonValueObject>(V));}
 for(const auto& M:P->Snapshot.Machines){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),M.Id);V->SetNumberField(TEXT("part"),int(M.Making));V->SetNumberField(TEXT("remaining"),M.Remaining);V->SetNumberField(TEXT("duration"),M.Duration);V->SetNumberField(TEXT("batch_remaining"),M.BatchRemaining);V->SetNumberField(TEXT("sheet_lb"),M.BedLb);V->SetBoolField(TEXT("enabled"),M.Enabled);Machines.Add(MakeShared<FJsonValueObject>(V));}
 J->SetArrayField(TEXT("drivers"),Drivers);J->SetArrayField(TEXT("machines"),Machines);
 for(const auto& O:C.Orders){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),O.Id);V->SetNumberField(TEXT("part"),O.Part);V->SetNumberField(TEXT("quantity"),O.Quantity);V->SetNumberField(TEXT("delivered"),O.Delivered);V->SetNumberField(TEXT("paid"),O.PaidCents);V->SetNumberField(TEXT("status"),int(O.Status));V->SetNumberField(TEXT("payment"),int(O.Payment));Orders.Add(MakeShared<FJsonValueObject>(V));}
 for(const auto& B:C.Bookings){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),B.Id);V->SetNumberField(TEXT("status"),int(B.Status));V->SetNumberField(TEXT("order"),B.OrderId);V->SetBoolField(TEXT("inbound"),B.Inbound);V->SetNumberField(TEXT("arrival"),B.ArrivalAt);TArray<TSharedPtr<FJsonValue>> Manifest;for(const auto& I:B.Manifest){auto X=MakeShared<FJsonObject>();X->SetNumberField(TEXT("pallet"),I.PalletId);X->SetNumberField(TEXT("quantity"),I.Quantity);Manifest.Add(MakeShared<FJsonValueObject>(X));}V->SetArrayField(TEXT("manifest"),Manifest);Bookings.Add(MakeShared<FJsonValueObject>(V));}
 for(const auto& A:P->Snapshot.Pallets){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),A.Id);V->SetNumberField(TEXT("place"),int(A.Place));V->SetNumberField(TEXT("part"),int(A.Part));V->SetNumberField(TEXT("quantity"),A.Quantity);V->SetNumberField(TEXT("booking"),A.BookingId);V->SetNumberField(TEXT("order"),A.OrderId);V->SetNumberField(TEXT("owner"),A.Owner);V->SetNumberField(TEXT("unit_lb"),A.UnitLb);V->SetNumberField(TEXT("reserved"),A.Reserved);V->SetNumberField(TEXT("x"),A.Position.X);V->SetNumberField(TEXT("y"),A.Position.Y);V->SetNumberField(TEXT("z"),A.Position.Z);Pallets.Add(MakeShared<FJsonValueObject>(V));}
 for(TActorIterator<AFoundrylineOperationsState> It(PC->GetWorld());It;++It){auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("company"),It->CompanyId.ToString());V->SetStringField(TEXT("plant"),It->PlantId.ToString());V->SetBoolField(TEXT("enabled"),It->Data.Enabled);Private.Add(MakeShared<FJsonValueObject>(V));}
 J->SetArrayField(TEXT("orders"),Orders);J->SetArrayField(TEXT("bookings"),Bookings);J->SetArrayField(TEXT("pallets"),Pallets);J->SetArrayField(TEXT("private_actors"),Private);
#endif
 return Json(J);
}
