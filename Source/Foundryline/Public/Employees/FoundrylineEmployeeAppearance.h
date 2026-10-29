#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FoundrylineEmployeeAppearance.generated.h"
class USkeletalMesh;
class UAnimSequence;
USTRUCT(BlueprintType)
struct FFoundrylineEmployeeOutfit {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<USkeletalMesh>> Parts;
};
// Final art plugs into the existing roster without changing employee IDs or simulation.
// Mesh must be authored at 1:1 centimetres; animation is in-place and forward +X.
UCLASS(BlueprintType)
class FOUNDRYLINE_API UFoundrylineEmployeeAppearance : public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere) TObjectPtr<USkeletalMesh> Mesh;
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<USkeletalMesh>> Variants;
 UPROPERTY(EditAnywhere) bool bHandymanPose=false;
 UPROPERTY(EditAnywhere) TArray<FFoundrylineEmployeeOutfit> Outfits;
 UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMesh>> RuntimeVariants;
 UPROPERTY(EditAnywhere) FVector FootRootOffset=FVector::ZeroVector;
 UPROPERTY(EditAnywhere) FRotator FacingOffset=FRotator::ZeroRotator;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Idle;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Walk;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Operate;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Weld;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Seated;
 UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Handling;
 UPROPERTY(EditAnywhere,meta=(ClampMin="20",ClampMax="250")) float WalkStrideCm=110;
};
