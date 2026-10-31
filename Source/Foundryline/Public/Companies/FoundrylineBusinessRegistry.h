#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Companies/FoundrylineBusinessLedger.h"
#include "Networking/FoundrylineActionContext.h"
#include "FoundrylineBusinessRegistry.generated.h"
class APlayerController;
class APlayerState;
USTRUCT()
struct FFoundrylineBusinessPublicCompany {
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FGuid PlantId;
 UPROPERTY() FGuid PlotId;
 UPROPERTY() FString Name;
};
USTRUCT()
struct FFoundrylineBusinessPublicMember {
 GENERATED_BODY()
 UPROPERTY() int32 PlayerId=-1;
 UPROPERTY() FGuid CompanyId;
};
// All mutation APIs are trusted server C++ entry points; deliberately no client RPCs.
UCLASS()
class FOUNDRYLINE_API AFoundrylineBusinessRegistry : public AActor {
 GENERATED_BODY()
public:
 AFoundrylineBusinessRegistry();
 static AFoundrylineBusinessRegistry* Find(const UWorld* World);
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 bool RegisterPlayer(APlayerController* Player);
 // Logout: release vehicle claims and drop the departing session before the directory refresh.
 void UnregisterPlayer(APlayerController* Player);
 // A future authentication adapter calls this before default enrolment. Never pass a client claim.
 bool RegisterVerifiedAccount(APlayerController* Player,const FString& VerifiedAccount,FString& Error);
 void RefreshDirectory();
 bool BindWorld();
 bool IsWorldBound()const{return bWorldBound;}
 FGuid CompanyFor(const APlayerController* Player)const;
 UFUNCTION(BlueprintPure,Category="Companies") FString GetCompanyName(FGuid Company)const;
 bool CanOperate(const APlayerController* Player,FGuid Company,FGuid Plant)const;
 bool ConstructionContext(const APlayerController* Player,FFoundrylinePlacementContext& Out)const;
 bool CreateBusiness(APlayerController* Player,const FString& Name,FGuid Plot,FString& Error);
 bool Invite(APlayerController* Owner,APlayerController* Guest,FString& Error);
 bool Accept(APlayerController* Guest,FGuid Company,FString& Error);
 bool RemoveMember(APlayerController* Owner,APlayerController* Guest,FString& Error);
 bool AdjustBalance(FGuid Company,int64 Cents,FString& Error);
 int64 InitialiseEconomy(FGuid Company,FGuid Plant,int64 InitialCash);
 bool CommitPlantCash(FGuid Company,FGuid Plant,int64 Expected,int64 Cash);
 FString OwnerDisplayName(FGuid Company)const;
 bool HasMultipleBusinesses()const{return Ledger.State.Companies.Num()>1;}
 const FFoundrylineBusinessLedger& GetLedger()const{return Ledger;}
 UFoundrylineBusinessSave* CreateSnapshot()const;
 bool RestoreBeforeWorldBinding(const UFoundrylineBusinessSave* Save,FString& Error);
 UFUNCTION(BlueprintPure,Category="Companies") FString GetBusinessDigest()const;
 UFUNCTION(BlueprintPure,Category="Companies") int32 GetCompanyCount()const{return Companies.Num();}
 UFUNCTION(BlueprintPure,Category="Companies") int32 GetActiveMemberCount()const{return ActiveMembers.Num();}
private:
 UPROPERTY(Replicated) TArray<FFoundrylineBusinessPublicCompany> Companies;
 UPROPERTY(Replicated) TArray<FFoundrylineBusinessPublicMember> ActiveMembers;
 FFoundrylineBusinessLedger Ledger;
 TMap<TWeakObjectPtr<APlayerState>,FString> Accounts;
 bool bWorldBound=false;
 FString Account(const APlayerController* Player)const;
 bool Connected(const APlayerController* Player)const;
};
