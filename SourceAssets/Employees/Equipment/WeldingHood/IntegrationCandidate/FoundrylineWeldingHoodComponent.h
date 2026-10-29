#pragma once
// INTEGRATION CANDIDATE ONLY: not in live Source, not compiled or native-tested.
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FoundrylineWeldingHoodComponent.generated.h"
class UFoundrylineEmployeeAppearance;
class UPoseableMeshComponent;
class USkeletalMesh;
class UStaticMeshComponent;
struct FFoundrylineEmployeeRecord;

UCLASS(ClassGroup=(Foundryline),meta=(BlueprintSpawnableComponent))
class FOUNDRYLINE_API UFoundrylineWeldingHoodComponent : public USceneComponent {
 GENERATED_BODY()
public:
 UFoundrylineWeldingHoodComponent();
 // Cache only original outfit parts with SK_Hat_* removed. Never remove the head/beard.
 // Returns null if no safe no-hat mesh can be assembled; caller keeps normal body/hidden hood.
 static USkeletalMesh* NoHatOutfit(UFoundrylineEmployeeAppearance* Profile,int32 Variant);
 static bool WantsHood(const FFoundrylineEmployeeRecord& Record);
 static bool WantsClosedVisor(const FFoundrylineEmployeeRecord& Record);
 bool HasModelAssets();
 void Present(UPoseableMeshComponent* Body,const FFoundrylineEmployeeRecord& Record,bool bNoHatBody,float DeltaSeconds);
 UFUNCTION(BlueprintPure) bool IsEquipped()const{return bEquipped;}
 UFUNCTION(BlueprintPure) bool IsVisorClosed()const{return bEquipped&&VisorAngleDegrees<2;}
 UFUNCTION(BlueprintPure) float GetVisorAngleDegrees()const{return VisorAngleDegrees;}
private:
 void CreateMeshes();
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Harness;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Visor;
 bool bTriedAssets=false,bEquipped=false;
 float VisorAngleDegrees=105;
};
