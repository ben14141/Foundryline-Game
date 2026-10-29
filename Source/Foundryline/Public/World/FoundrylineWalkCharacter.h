#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FoundrylineWalkCharacter.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylineWalkCharacter : public ACharacter
{
 GENERATED_BODY()
public:
 AFoundrylineWalkCharacter();
 using ACharacter::SetBase;
 virtual void SetBase(FMovementBaseInterfaceData* Base,const FName BoneName=NAME_None,bool bNotifyActor=true) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
 void Forward(float V);void Right(float V);void LookX(float V);void LookY(float V);
 UPROPERTY() TObjectPtr<class UCameraComponent> Camera;
};
