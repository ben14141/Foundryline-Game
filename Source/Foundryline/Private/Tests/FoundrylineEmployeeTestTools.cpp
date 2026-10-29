#include "FoundrylineNetworkTestTools.h"
#include "UI/FoundrylinePlayerController.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
FString UFoundrylineNetworkTestTools::EmployeeReport(APlayerController* Player){auto J=MakeShared<FJsonObject>();
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||PC->GetWorld()->WorldType!=EWorldType::PIE)return TEXT("{}");auto* Registry=AFoundrylineBusinessRegistry::Find(PC->GetWorld());if(!Registry)return TEXT("{}");const FGuid Company=Registry->CompanyFor(PC);AFoundrylineFactoryPlant* P=nullptr;for(TActorIterator<AFoundrylineFactoryPlant> It(PC->GetWorld());It;++It)if(Company.IsValid()&&It->BusinessCompanyId==Company){P=*It;break;}if(!P)return TEXT("{}");
 const auto& S=P->Snapshot;J->SetStringField(TEXT("company"),S.CompanyId.ToString());J->SetStringField(TEXT("plant"),S.PlantId.ToString());J->SetNumberField(TEXT("cash"),S.Commerce.CashCents);J->SetBoolField(TEXT("running"),S.Running);J->SetNumberField(TEXT("operators"),FFoundrylineEmployeeRoster::Build(S).FilterByPredicate([](const auto& E){return E.Role==EFoundrylineEmployeeRole::Operator;}).Num());J->SetNumberField(TEXT("welders"),FFoundrylineEmployeeRoster::Build(S).FilterByPredicate([](const auto& E){return E.Role==EFoundrylineEmployeeRole::Welder;}).Num());J->SetNumberField(TEXT("drivers"),S.Drivers.FilterByPredicate([](const auto& D){return D.EmployeeId>0;}).Num());J->SetNumberField(TEXT("unassigned"),FFoundrylineEmployeeRoster::Build(S).FilterByPredicate([](const auto& E){return E.Role==EFoundrylineEmployeeRole::Unassigned;}).Num());J->SetNumberField(TEXT("forklifts"),S.Drivers.Num());if(PC->HasAuthority()){FString E;J->SetBoolField(TEXT("valid"),P->Model.Valid(E));J->SetStringField(TEXT("error"),E);}
 auto Vec=[](FVector V){TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(V.X));A.Add(MakeShared<FJsonValueNumber>(V.Y));A.Add(MakeShared<FJsonValueNumber>(V.Z));return A;};TArray<TSharedPtr<FJsonValue>> People,Machines;
 for(const auto& E:FFoundrylineEmployeeRoster::Build(S)){auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("key"),E.Key);V->SetStringField(TEXT("name"),E.Label);V->SetStringField(TEXT("job"),E.CurrentJob);V->SetNumberField(TEXT("vehicle"),E.VehicleId);V->SetNumberField(TEXT("id"),E.Id);V->SetNumberField(TEXT("role"),int(E.Role));V->SetNumberField(TEXT("assignment"),E.Assignment);V->SetArrayField(TEXT("position"),Vec(E.Position));V->SetArrayField(TEXT("home"),Vec(E.Home));V->SetArrayField(TEXT("workpoint"),Vec(E.WorkPoint));V->SetNumberField(TEXT("yaw"),E.Yaw);V->SetNumberField(TEXT("speed"),E.Speed);V->SetNumberField(TEXT("distance"),E.Distance);V->SetNumberField(TEXT("phase"),E.WorkPhase);V->SetStringField(TEXT("activity"),FFoundrylineEmployeeRoster::ActivityName(E.Activity));V->SetStringField(TEXT("wait"),FFoundrylineEmployeeRoster::WaitName(E.Wait));V->SetBoolField(TEXT("moving"),E.bMoving);V->SetBoolField(TEXT("working"),E.bWorking);V->SetBoolField(TEXT("seated"),E.bSeated);TArray<TSharedPtr<FJsonValue>> Path;for(auto Point:E.Path)Path.Add(MakeShared<FJsonValueArray>(Vec(Point)));V->SetArrayField(TEXT("path"),Path);People.Add(MakeShared<FJsonValueObject>(V));}
 for(const auto& M:S.Machines){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),M.Id);V->SetNumberField(TEXT("remaining"),M.Remaining);V->SetNumberField(TEXT("batch"),M.BatchRemaining);V->SetBoolField(TEXT("enabled"),M.Enabled);V->SetBoolField(TEXT("staff_ready"),S.Operators.ContainsByPredicate([&](const auto& O){return O.Machine==M.Id&&FVector::Dist2D(O.Position,O.WorkPoint)<=1;}));Machines.Add(MakeShared<FJsonValueObject>(V));}J->SetNumberField(TEXT("count"),People.Num());J->SetArrayField(TEXT("employees"),People);J->SetArrayField(TEXT("machines"),Machines);TArray<TSharedPtr<FJsonValue>> Requests,Vehicles;for(const auto& Id:S.HireRequests)Requests.Add(MakeShared<FJsonValueString>(Id.ToString()));J->SetArrayField(TEXT("hire_requests"),Requests);for(const auto& D:S.Drivers){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),D.Id);V->SetNumberField(TEXT("employee"),D.EmployeeId);V->SetNumberField(TEXT("job"),D.Job);V->SetBoolField(TEXT("parked"),FVector::Dist2D(D.Position,D.Parking)<20);Vehicles.Add(MakeShared<FJsonValueObject>(V));}J->SetArrayField(TEXT("vehicles"),Vehicles);TArray<TSharedPtr<FJsonValue>> Zones;for(const auto& Z:S.Zones){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("id"),Z.Id);V->SetBoolField(TEXT("ordered"),Z.Ordered);V->SetNumberField(TEXT("active_step"),Z.ActiveStep);V->SetNumberField(TEXT("work"),Z.Work);V->SetNumberField(TEXT("completed"),Z.CompletedMask);V->SetNumberField(TEXT("installed_lb"),Z.InstalledLb);V->SetNumberField(TEXT("welders"),Z.Welders);Zones.Add(MakeShared<FJsonValueObject>(V));}J->SetArrayField(TEXT("zones"),Zones);
#endif
 FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);return Out;
}
bool UFoundrylineNetworkTestTools::QueueEmployeeAssignment(APlayerController* Player,int32 Id,int32 Role,int32 Target,FGuid Plant,int32 Slot){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Id,Role,Target,Plant,Slot](float){if(Weak.IsValid()){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::AssignEmployee;C.Id=Id;C.Count=Role;C.Other=Target;C.QuarterTurns=Slot;C.TargetPlantId=Plant;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}

bool UFoundrylineNetworkTestTools::QueueEmployeeHire(APlayerController* Player,int32 Role,FGuid Request,FGuid Plant){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Role,Request,Plant](float){if(Weak.IsValid()){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::HireEmployee;C.Count=Role;C.RequestId=Request;C.TargetPlantId=Plant;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}

bool UFoundrylineNetworkTestTools::QueueFactoryOrderZone(APlayerController* Player,int32 ZoneId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,ZoneId](float){if(Weak.IsValid()){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::Order;C.Id=ZoneId;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}
