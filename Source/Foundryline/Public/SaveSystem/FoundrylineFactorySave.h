#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "FoundrylineFactorySave.generated.h"
UCLASS()
class FOUNDRYLINE_API UFoundrylineFactorySave:public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) FFoundrylineFactoryState Factory;
 UPROPERTY(SaveGame) TArray<uint8> Property;
};
