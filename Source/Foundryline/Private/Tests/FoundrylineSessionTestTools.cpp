#include "FoundrylineNetworkTestTools.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "UI/FoundrylinePlayerController.h"
#include "World/FoundrylineCameraPawn.h"
#include "World/FoundrylineWalkCharacter.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {
UWorld* ListenServerWorld(){if(GEngine)for(const auto& C:GEngine->GetWorldContexts())if(auto* W=C.World();W&&W->GetNetMode()==NM_ListenServer)return W;return nullptr;}
APlayerController* ServerSide(const APlayerController* Client){auto* W=ListenServerWorld();if(!W||!Client||!Client->PlayerState)return nullptr;for(TActorIterator<APlayerController> It(W);It;++It)if(It->PlayerState&&It->PlayerState->GetPlayerId()==Client->PlayerState->GetPlayerId())return *It;return nullptr;}
template<typename F> bool OnNextTick(APlayerController* Player,F Action){auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action](float){if(Weak.IsValid())Action(Weak.Get());return false;}));return true;}
}
bool UFoundrylineNetworkTestTools::KickClientForTest(APlayerController* Client){
#if WITH_EDITOR
 auto* Server=ServerSide(Client);if(!Server||Server->IsLocalController())return false;auto* Mode=Server->GetWorld()->GetAuthGameMode();return Mode&&Mode->GameSession&&Mode->GameSession->KickPlayer(Server,FText::FromString(TEXT("Foundryline session test")));
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueuePropertyView(APlayerController* Player){
#if WITH_EDITOR
 return OnNextTick(Player,[](AFoundrylinePlayerController* PC){PC->OpenOperations(1);PC->EnterPropertyView();});
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueueFreightDriveToggle(APlayerController* Player){
#if WITH_EDITOR
 return OnNextTick(Player,[](AFoundrylinePlayerController* PC){PC->ToggleFreightDrive();});
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SpawnFreightRigForTest(APlayerController* Host,FVector Location){
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||!Host->IsLocalController())return false;const FTransform T(FRotator::ZeroRotator,Location);auto* Rig=Host->GetWorld()->SpawnActorDeferred<AFoundrylineFreightRig>(AFoundrylineFreightRig::StaticClass(),T);if(!Rig)return false;Rig->bTractorOnly=true;Rig->State.Tractor=FVector::ZeroVector;Rig->FinishSpawning(T);return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::SessionReport(APlayerController* Player){
 auto J=MakeShared<FJsonObject>();if(!Player)return TEXT("{}");UWorld* W=Player->GetWorld();
 if(auto* R=AFoundrylineBusinessRegistry::Find(W)){J->SetNumberField(TEXT("active_members"),R->GetActiveMemberCount());J->SetBoolField(TEXT("has_company"),R->CompanyFor(Player).IsValid());if(W->GetNetMode()!=NM_Client)J->SetNumberField(TEXT("ledger_members"),R->GetLedger().State.Members.Num());}
 if(W->GetNetMode()!=NM_Client){int32 Players=0,Walking=0,Unpossessed=0;for(TActorIterator<AFoundrylinePlayerController> It(W);It;++It){++Players;if(It->IsWalking())++Walking;}
  for(TActorIterator<APawn> It(W);It;++It)if((It->IsA<AFoundrylineCameraPawn>()||It->IsA<AFoundrylineWalkCharacter>())&&!It->GetController())++Unpossessed;
  // Each walking player parks exactly one top-down camera; anything beyond that is a leaked pawn.
  J->SetNumberField(TEXT("players"),Players);J->SetNumberField(TEXT("walking"),Walking);J->SetNumberField(TEXT("unpossessed_pawns"),Unpossessed);}
 for(TActorIterator<AFoundrylineFreightRig> It(W);It;++It){J->SetBoolField(TEXT("rig"),true);J->SetStringField(TEXT("rig_driver"),IsValid(It->Driver)?FString::FromInt(It->Driver->GetPlayerId()):FString());break;}
 J->SetStringField(TEXT("player_id"),Player->PlayerState?FString::FromInt(Player->PlayerState->GetPlayerId()):FString());J->SetBoolField(TEXT("walking"),Cast<AFoundrylinePlayerController>(Player)&&Cast<AFoundrylinePlayerController>(Player)->IsWalking());
 FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);return Out;
}
