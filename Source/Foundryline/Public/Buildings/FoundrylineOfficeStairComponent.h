#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FoundrylineOfficeStairComponent.generated.h"

// Game-owned stair visuals and pawn collision, rebuilt from the saved host record.
UCLASS()
class FOUNDRYLINE_API UFoundrylineOfficeStairComponent:public USceneComponent {
 GENERATED_BODY()
public:
 UFoundrylineOfficeStairComponent();
 void Build(FIntPoint Origin,int32 Quarter,class UMaterialInterface* Step,class UMaterialInterface* Rail,bool Preview,class UMaterialInterface* Ghost=nullptr,uint8 Style=0);
private:
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> GeneratedParts;
};
