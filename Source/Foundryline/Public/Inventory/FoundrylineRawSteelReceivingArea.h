#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineRawSteelReceivingArea.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylineRawSteelReceivingArea:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineRawSteelReceivingArea();
 virtual void BeginPlay()override;
};
