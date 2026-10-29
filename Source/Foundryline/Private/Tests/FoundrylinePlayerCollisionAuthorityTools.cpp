#include "FoundrylineNetworkTestTools.h"
#include "UI/FoundrylinePlayerController.h"
#include "World/FoundrylineWalkCharacter.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {
AFoundrylinePlayerController* CollisionAuthority(APlayerController* Player){
#if WITH_EDITOR
 if(!GEngine||!Player||!Player->PlayerState||!Player->GetWorld()||Player->GetWorld()->WorldType!=EWorldType::PIE)return nullptr;const int32 Id=Player->PlayerState->GetPlayerId();if(Id<0)return nullptr;
 for(const auto& Context:GEngine->GetWorldContexts()){auto* W=Context.World();if(!W||W->WorldType!=EWorldType::PIE||W->GetNetMode()==NM_Client)continue;for(TActorIterator<AFoundrylinePlayerController> It(W);It;++It)if(It->HasAuthority()&&It->PlayerState&&It->PlayerState->GetPlayerId()==Id)return *It;}
#endif
 return nullptr;
}
TArray<TSharedPtr<FJsonValue>> CollisionPosition(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}
}
bool UFoundrylineNetworkTestTools::PlacePlayerCollisionReview(APlayerController* Player,FVector Position,float Yaw){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||Position.ContainsNaN()||!FMath::IsFinite(Yaw)||Position.GetAbsMax()>1000000)return false;auto* Server=CollisionAuthority(PC);auto* LocalWalker=Cast<AFoundrylineWalkCharacter>(PC->GetPawn());auto* ServerWalker=Server?Cast<AFoundrylineWalkCharacter>(Server->GetPawn()):nullptr;if(!LocalWalker||!ServerWalker)return false;
 const FRotator Facing(0,Yaw,0);auto Place=[&](AFoundrylinePlayerController* Owner,AFoundrylineWalkCharacter* Walker){Walker->GetCharacterMovement()->StopMovementImmediately();Walker->SetActorLocationAndRotation(Position,Facing,false,nullptr,ETeleportType::TeleportPhysics);Owner->SetControlRotation(Facing);Walker->ForceNetUpdate();};Place(Server,ServerWalker);if(LocalWalker!=ServerWalker)Place(PC,LocalWalker);return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::AuthoritativePlayerCollisionPosition(APlayerController* Player){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("found"),false);
#if WITH_EDITOR
 if(auto* Server=CollisionAuthority(Player))if(auto* Walker=Cast<AFoundrylineWalkCharacter>(Server->GetPawn())){J->SetBoolField(TEXT("found"),true);J->SetNumberField(TEXT("player_id"),Server->PlayerState->GetPlayerId());J->SetArrayField(TEXT("position"),CollisionPosition(Walker->GetActorLocation()));J->SetStringField(TEXT("pawn"),Walker->GetName());J->SetStringField(TEXT("world"),Server->GetWorld()->GetName());J->SetNumberField(TEXT("radius"),Walker->GetCapsuleComponent()->GetScaledCapsuleRadius());J->SetNumberField(TEXT("half_height"),Walker->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());J->SetNumberField(TEXT("max_step"),Walker->GetCharacterMovement()->MaxStepHeight);if(Player&&Player->GetPawn())J->SetArrayField(TEXT("local_position"),CollisionPosition(Player->GetPawn()->GetActorLocation()));}
#endif
 FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);return Text;
}
