#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FoundrylineHandymanAssetTools.generated.h"
class USkeletalMesh;
class UMaterialInterface;
UCLASS()
class FOUNDRYLINE_API UFoundrylineHandymanAssetTools:public UBlueprintFunctionLibrary {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Foundryline|Art") static FString InspectMesh(USkeletalMesh* Mesh);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Art") static USkeletalMesh* BuildRuntimeVariant(int32 Index,const TArray<USkeletalMesh*>& Parts);
};
