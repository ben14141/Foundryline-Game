#include "Companies/FoundrylineBusinessRegistry.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Misc/Crc.h"
namespace {void ReleaseControls(APlayerController* P){if(!P)return;for(TActorIterator<AFoundrylineFreightRig> It(P->GetWorld());It;++It)It->Claim(P,false);for(TActorIterator<AFoundrylineMaterialHandlingCell> It(P->GetWorld());It;++It)It->Claim(P,false);}}
AFoundrylineBusinessRegistry::AFoundrylineBusinessRegistry(){bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);Ledger.Seed();}
AFoundrylineBusinessRegistry* AFoundrylineBusinessRegistry::Find(const UWorld* W){if(W)for(TActorIterator<AFoundrylineBusinessRegistry> It(W);It;++It)return *It;return nullptr;}
void AFoundrylineBusinessRegistry::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineBusinessRegistry,Companies);DOREPLIFETIME(AFoundrylineBusinessRegistry,ActiveMembers);}
bool AFoundrylineBusinessRegistry::Connected(const APlayerController* P)const{return P&&P->GetWorld()==GetWorld()&&P->PlayerState&&!P->PlayerState->IsOnlyASpectator()&&GetWorld()->GetGameState()&&GetWorld()->GetGameState()->PlayerArray.Contains(P->PlayerState);}
FString AFoundrylineBusinessRegistry::Account(const APlayerController* P)const{if(!Connected(P))return {};const auto* Key=Accounts.Find(P->PlayerState);return Key?*Key:FString();}
bool AFoundrylineBusinessRegistry::RegisterPlayer(APlayerController* P){if(!HasAuthority()||!Connected(P))return false;if(!Accounts.Contains(P->PlayerState)){Accounts.Add(P->PlayerState,TEXT("session:")+FGuid::NewGuid().ToString());}FString E;const bool Ok=Ledger.EnrolCoop(Account(P),E);RefreshDirectory();return Ok;}
void AFoundrylineBusinessRegistry::UnregisterPlayer(APlayerController* P){if(!HasAuthority()||!P)return;ReleaseControls(P);FString Key;if(P->PlayerState&&Accounts.RemoveAndCopyValue(P->PlayerState,Key))Ledger.ForgetSession(Key);RefreshDirectory();}
FGuid AFoundrylineBusinessRegistry::CompanyFor(const APlayerController* P)const{if(!Connected(P))return {};if(HasAuthority())return Ledger.CompanyFor(Account(P));const auto* M=ActiveMembers.FindByPredicate([&](const auto& V){return V.PlayerId==P->PlayerState->GetPlayerId();});return M?M->CompanyId:FGuid();}
bool AFoundrylineBusinessRegistry::CanOperate(const APlayerController* P,FGuid C,FGuid Plant)const{if(!C.IsValid()||CompanyFor(P)!=C)return false;if(HasAuthority()){const auto* V=Ledger.Company(C);return V&&V->PlantId==Plant;}return Companies.ContainsByPredicate([&](const auto& V){return V.Id==C&&V.PlantId==Plant;});}
bool AFoundrylineBusinessRegistry::ConstructionContext(const APlayerController* P,FFoundrylinePlacementContext& Out)const{Out={};const auto Id=CompanyFor(P);if(!Id.IsValid())return false;if(HasAuthority()){const auto* C=Ledger.Company(Id);if(!C)return false;Out.CompanyId=C->Id;Out.PlantId=C->PlantId;}else{const auto* C=Companies.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!C)return false;Out.CompanyId=C->Id;Out.PlantId=C->PlantId;}Out.bCanConstruct=true;return true;}
void AFoundrylineBusinessRegistry::RefreshDirectory(){if(!HasAuthority())return;Companies.Reset();for(const auto& C:Ledger.State.Companies){FFoundrylineBusinessPublicCompany V;V.Id=C.Id;V.PlantId=C.PlantId;V.PlotId=C.PlotId;V.Name=C.Name;Companies.Add(V);}ActiveMembers.Reset();for(auto It=Accounts.CreateIterator();It;++It){if(!It.Key().IsValid()){It.RemoveCurrent();continue;}auto* PS=It.Key().Get();if(!GetWorld()->GetGameState()||!GetWorld()->GetGameState()->PlayerArray.Contains(PS))continue;FFoundrylineBusinessPublicMember M;M.PlayerId=PS->GetPlayerId();M.CompanyId=Ledger.CompanyFor(It.Value());ActiveMembers.Add(M);}ForceNetUpdate();}
bool AFoundrylineBusinessRegistry::CreateBusiness(APlayerController* P,const FString& Name,FGuid Plot,FString& E){if(!HasAuthority()||!bWorldBound||Account(P).IsEmpty()){E=TEXT("A registered server-side player is required");return false;}auto Copy=Ledger;if(!Copy.Create(Account(P),Name,Plot,E)||!Copy.Valid(E))return false;auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();if(!Placement||Placement->RegisterBusinessRegion(Copy.Region(Plot))!=EFoundrylinePlacementError::None){E=TEXT("Plot registration failed; business unchanged");return false;}Ledger=MoveTemp(Copy);ReleaseControls(P);RefreshDirectory();for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)It->PublishNetworkState();return true;}
bool AFoundrylineBusinessRegistry::Invite(APlayerController* O,APlayerController* G,FString& E){if(!HasAuthority()||Account(O).IsEmpty()||Account(G).IsEmpty())return false;return Ledger.Invite(Account(O),Account(G),E);}
bool AFoundrylineBusinessRegistry::Accept(APlayerController* G,FGuid C,FString& E){if(!HasAuthority()||Account(G).IsEmpty()||!Ledger.Accept(Account(G),C,E))return false;ReleaseControls(G);RefreshDirectory();return true;}
bool AFoundrylineBusinessRegistry::RemoveMember(APlayerController* O,APlayerController* G,FString& E){if(!HasAuthority()||Account(O).IsEmpty()||Account(G).IsEmpty()||!Ledger.RemoveMember(Account(O),Account(G),E))return false;ReleaseControls(G);RefreshDirectory();return true;}
bool AFoundrylineBusinessRegistry::AdjustBalance(FGuid C,int64 Delta,FString& E){return HasAuthority()&&Ledger.AdjustBalance(C,Delta,E);}
UFoundrylineBusinessSave* AFoundrylineBusinessRegistry::CreateSnapshot()const{if(!HasAuthority())return nullptr;auto* S=NewObject<UFoundrylineBusinessSave>();S->Businesses=Ledger.State;return S;}
bool AFoundrylineBusinessRegistry::RestoreBeforeWorldBinding(const UFoundrylineBusinessSave* S,FString& E){if(!HasAuthority()||bWorldBound||!Accounts.IsEmpty()||!S){E=TEXT("Restore business identity before login and world binding");return false;}if(!Ledger.Restore(S->Businesses,E))return false;RefreshDirectory();return true;}
FString AFoundrylineBusinessRegistry::GetBusinessDigest()const{FString S;for(const auto& C:Companies)S+=C.Id.ToString()+C.PlantId.ToString()+C.PlotId.ToString()+C.Name;for(const auto& M:ActiveMembers)S+=FString::FromInt(M.PlayerId)+M.CompanyId.ToString();return FString::Printf(TEXT("%08X"),FCrc::StrCrc32(*S));}

bool AFoundrylineBusinessRegistry::BindWorld(){if(!HasAuthority())return false;auto* P=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();if(!P||P->GetRegions().IsEmpty())return false;TArray<FFoundrylineBuildRegion> Regions;for(const auto& Plot:Ledger.State.Plots)if(Plot.Company.IsValid())Regions.Add(Ledger.Region(Plot.Id));if(P->RegisterBusinessRegions(Regions)!=EFoundrylinePlacementError::None)return false;bWorldBound=true;RefreshDirectory();return true;}

bool AFoundrylineBusinessRegistry::RegisterVerifiedAccount(APlayerController* P,const FString& Verified,FString& E){
 if(!HasAuthority()||!Connected(P)||Verified.TrimStartAndEnd().IsEmpty()||Verified.Len()>200){E=TEXT("Server-verified account required");return false;}const FString Key=TEXT("verified:")+Verified;
 if(const auto* Existing=Accounts.Find(P->PlayerState);Existing&&*Existing!=Key){E=TEXT("Bind a verified identity before default session enrolment");return false;}
 Accounts.Add(P->PlayerState,Key);const bool Ok=Ledger.EnrolCoop(Key,E);RefreshDirectory();return Ok;
}

int64 AFoundrylineBusinessRegistry::InitialiseEconomy(FGuid Company,FGuid Plant,int64 Initial){if(!HasAuthority())return 0;auto* C=Ledger.State.Companies.FindByPredicate([&](const auto& V){return V.Id==Company&&V.PlantId==Plant;});if(!C)return 0;if(!C->EconomyInitialised){if(!C->BalanceCents)C->BalanceCents=FMath::Max(int64(0),Initial);C->EconomyInitialised=true;}return C->BalanceCents;}
bool AFoundrylineBusinessRegistry::CommitPlantCash(FGuid Company,FGuid Plant,int64 Expected,int64 Cash){if(!HasAuthority()||Cash<0||Cash>1000000000000LL)return false;auto* C=Ledger.State.Companies.FindByPredicate([&](const auto& V){return V.Id==Company&&V.PlantId==Plant;});if(!C||C->BalanceCents!=Expected)return false;C->BalanceCents=Cash;C->EconomyInitialised=true;return true;}

FString AFoundrylineBusinessRegistry::GetCompanyName(FGuid Company)const{if(const auto* C=Companies.FindByPredicate([&](const auto& V){return V.Id==Company;}))return C->Name;if(HasAuthority())if(const auto* C=Ledger.Company(Company))return C->Name;return {}; }

FString AFoundrylineBusinessRegistry::OwnerDisplayName(FGuid Company)const{const auto* C=Ledger.Company(Company);if(C)for(const auto& P:Accounts)if(P.Key.IsValid()&&P.Value==C->OwnerAccount)return P.Key->GetPlayerName().Left(64);return TEXT("Business owner");}
