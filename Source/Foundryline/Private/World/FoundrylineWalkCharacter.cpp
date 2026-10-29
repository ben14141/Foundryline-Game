#include "World/FoundrylineWalkCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
AFoundrylineWalkCharacter::AFoundrylineWalkCharacter()
{
 bReplicates=true;GetCapsuleComponent()->InitCapsuleSize(30,90);BaseEyeHeight=80;
 GetCharacterMovement()->MaxWalkSpeed=350;GetCharacterMovement()->MaxStepHeight=25;
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));Camera->SetupAttachment(GetCapsuleComponent());Camera->SetRelativeLocation(FVector(0,0,80));Camera->bUsePawnControlRotation=true;Camera->FieldOfView=85;
}
void AFoundrylineWalkCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);Input->BindAxis(TEXT("CameraForward"),this,&AFoundrylineWalkCharacter::Forward);Input->BindAxis(TEXT("CameraRight"),this,&AFoundrylineWalkCharacter::Right);Input->BindAxis(TEXT("WalkLookX"),this,&AFoundrylineWalkCharacter::LookX);Input->BindAxis(TEXT("WalkLookY"),this,&AFoundrylineWalkCharacter::LookY);
}
void AFoundrylineWalkCharacter::Forward(float V) {if(Controller) AddMovementInput(FRotator(0,Controller->GetControlRotation().Yaw,0).Vector(),V);}
void AFoundrylineWalkCharacter::Right(float V) {if(Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
void AFoundrylineWalkCharacter::LookX(float V) {AddControllerYawInput(V);}
void AFoundrylineWalkCharacter::LookY(float V) {AddControllerPitchInput(-V);}

#include "Interfaces/MovementBaseInterface.h"
void AFoundrylineWalkCharacter::SetBase(FMovementBaseInterfaceData* Base,const FName BoneName,bool bNotifyActor)
{
 // Generated floors and pooled physical object shapes exist separately on every
 // peer. Keep their collision and server movement validation, but never encode
 // these local, non-replicated components as relative network movement bases.
 // Real moving platforms retain the normal Unreal based-movement path.
 const auto* Component=Base?Cast<UPrimitiveComponent>(Base->PhysicsObjectOwner.Get()):nullptr;
 if(Component && (Component->ComponentHasTag(TEXT("FoundrylinePhysicalCollision")) || (Component->Mobility==EComponentMobility::Static && Component->GetOwner() && Component->GetOwner()->ActorHasTag(TEXT("FoundrylinePropertyVisual")))))
  Super::SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr),NAME_None,bNotifyActor);
 else Super::SetBase(Base,BoneName,bNotifyActor);
}
