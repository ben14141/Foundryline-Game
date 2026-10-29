#pragma once
#include "CoreMinimal.h"
#include "Companies/FoundrylineBuildRegion.h"
#include "GameFramework/SaveGame.h"
#include "FoundrylineBusinessLedger.generated.h"

USTRUCT()
struct FFoundrylineBusinessCompany {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid Id;
 UPROPERTY(SaveGame) FGuid PlantId;
 UPROPERTY(SaveGame) FGuid PlotId;
 UPROPERTY(SaveGame) FString Name;
 UPROPERTY(SaveGame) FString OwnerAccount;
 UPROPERTY(SaveGame) int64 BalanceCents=0;
 UPROPERTY(SaveGame) bool EconomyInitialised=false;
};
USTRUCT()
struct FFoundrylineBusinessMembership {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FString Account;
 UPROPERTY(SaveGame) FGuid Company;
};
USTRUCT()
struct FFoundrylineBusinessPlot {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid Id;
 UPROPERTY(SaveGame) FFoundrylineGridBounds Bounds;
 UPROPERTY(SaveGame) FFoundrylineGridBounds BuildingBounds;
 UPROPERTY(SaveGame) FGuid Company;
};
USTRUCT()
struct FFoundrylineBusinessInvitation {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FString Account;
 UPROPERTY(SaveGame) FGuid Company;
};
USTRUCT()
struct FFoundrylineBusinessState {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Version=1;
 UPROPERTY(SaveGame) TArray<FFoundrylineBusinessCompany> Companies;
 UPROPERTY(SaveGame) TArray<FFoundrylineBusinessMembership> Members;
 UPROPERTY(SaveGame) TArray<FFoundrylineBusinessPlot> Plots;
 UPROPERTY(SaveGame) TArray<FFoundrylineBusinessInvitation> Invitations;
};
UCLASS()
class FOUNDRYLINE_API UFoundrylineBusinessSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) FFoundrylineBusinessState Businesses;
};

// Server-owned data only. Account strings must come from the authenticated connection,
// never from a client-supplied claim. Every failure leaves the ledger unchanged.
struct FOUNDRYLINE_API FFoundrylineBusinessLedger {
 FFoundrylineBusinessState State;
 static FGuid StarterCompany(){return FGuid(0xF001,1,1,1);}
 static FGuid StarterPlant(){return FGuid(0xF001,2,1,1);}
 void Seed();
 FGuid CompanyFor(const FString& Account)const;
 const FFoundrylineBusinessCompany* Company(FGuid Id)const;
 bool EnrolCoop(const FString& Account,FString& Error);
 bool Create(const FString& Account,const FString& Name,FGuid Plot,FString& Error);
 bool Invite(const FString& Owner,const FString& Invitee,FString& Error);
 bool Accept(const FString& Account,FGuid Company,FString& Error);
 bool RemoveMember(const FString& Owner,const FString& Account,FString& Error);
 // Anonymous session identities never reconnect, so a departing non-owner's membership is dropped.
 bool ForgetSession(const FString& Account);
 bool AdjustBalance(FGuid Company,int64 Delta,FString& Error);
 bool Valid(FString& Error)const;
 bool Restore(const FFoundrylineBusinessState& Snapshot,FString& Error);
 FFoundrylineBuildRegion Region(FGuid Plot)const;
};
