#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineFloorGuide.generated.h"
// Local-only projected guides. They never reserve space or replicate a player's preview.
UCLASS()
class FOUNDRYLINE_API AFoundrylineFloorGuide:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineFloorGuide();
 virtual void BeginPlay()override;
 void Show(const FBox2D& Floor,const FBox2D& Footprint,double Step,bool Grid,bool Valid,bool Visible);
private:
 UPROPERTY() TObjectPtr<class UDecalComponent> GridDecal;
 UPROPERTY() TObjectPtr<class UDecalComponent> FootprintDecal;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> GridMaterial;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> FootprintMaterial;
 UPROPERTY() TObjectPtr<class UTexture2D> GridTexture;
 UPROPERTY() TObjectPtr<class UTexture2D> FootprintTexture;
};
