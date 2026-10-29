#include "FoundrylineNetworkTestTools.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FString UFoundrylineNetworkTestTools::ProbePlayerCollision(APlayerController* Player,FVector Start,FVector End){
 TSharedRef<FJsonObject> Out=MakeShared<FJsonObject>();Out->SetBoolField(TEXT("valid"),false);
#if WITH_EDITOR
 auto* Walker=Player?Cast<AFoundrylineWalkCharacter>(Player->GetPawn()):nullptr;
 if(Walker&&Walker->GetWorld()->WorldType==EWorldType::PIE){
  auto* Movement=Walker->GetCharacterMovement();const FTransform Before=Walker->GetActorTransform();const FVector Velocity=Movement->Velocity;
  Walker->SetActorLocation(Start,false,nullptr,ETeleportType::TeleportPhysics);FHitResult Hit;
  Movement->SafeMoveUpdatedComponent(End-Start,Walker->GetActorQuat(),true,Hit);
  const FVector Reached=Walker->GetActorLocation();Out->SetBoolField(TEXT("valid"),true);Out->SetBoolField(TEXT("blocked"),Hit.bBlockingHit);Out->SetBoolField(TEXT("penetrating"),Hit.bStartPenetrating);Out->SetNumberField(TEXT("travel"),FVector::Distance(Start,Reached));Out->SetNumberField(TEXT("requested"),FVector::Distance(Start,End));Out->SetNumberField(TEXT("fraction"),Hit.Time);Out->SetNumberField(TEXT("capsule_radius"),Walker->GetCapsuleComponent()->GetScaledCapsuleRadius());Out->SetNumberField(TEXT("capsule_half_height"),Walker->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());Out->SetStringField(TEXT("actor"),Hit.GetActor()?Hit.GetActor()->GetPathName():TEXT(""));Out->SetStringField(TEXT("component"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():TEXT(""));Out->SetStringField(TEXT("location"),Reached.ToString());Out->SetBoolField(TEXT("authority"),Walker->HasAuthority());
  Walker->SetActorTransform(Before,false,nullptr,ETeleportType::TeleportPhysics);Movement->Velocity=Velocity;
 }
#endif
 FString Result;const auto Writer=TJsonWriterFactory<>::Create(&Result);FJsonSerializer::Serialize(Out,Writer);return Result;
}
