#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FoundrylineSkylightAssetTools.generated.h"
UCLASS()
class FOUNDRYLINE_API UFoundrylineSkylightAssetTools : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor") static bool PrepareSkylight();
};
