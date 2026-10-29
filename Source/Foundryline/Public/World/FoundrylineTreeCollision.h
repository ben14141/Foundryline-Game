#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineTreeCollision.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylineTreeCollision:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineTreeCollision();
 virtual void BeginPlay()override;
 void BuildTrunks();
private:
 UPROPERTY() TArray<TObjectPtr<class UCapsuleComponent>> Trunks;
};
