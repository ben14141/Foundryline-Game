#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FoundrylineEmployeeVisualComponent.generated.h"

struct FFoundrylineEmployeeRecord;
class UStaticMeshComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UPoseableMeshComponent;
class UFoundrylineEmployeeAppearance;
class UFoundrylineWeldingHoodComponent;

// Presentation only: identity, movement and work remain authoritative in the plant roster.
// Foot-root, centimetres, forward +X. Replace this component with a skeletal adapter
// consuming the same record when the final employee models are selected.
UCLASS(ClassGroup=(Foundryline), meta=(BlueprintSpawnableComponent))
class FOUNDRYLINE_API UFoundrylineEmployeeVisualComponent : public USceneComponent {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UFoundrylineEmployeeAppearance> Appearance;
 void InitializeVisual();
 void Present(const FFoundrylineEmployeeRecord& Record,float DeltaSeconds);
 // Call after the zone gun has moved, using its real grip and barrel-support points.
 void PresentWeldingGrip(FVector WorldGrip,FVector WorldSupport);
 void PresentCollectorGrip(FVector WorldLeft,FVector WorldRight);
 // Presentation clip for an office chair: foot root at floor, +X faces keyboard.
 UFUNCTION(BlueprintCallable,Category="Foundryline|Employees") void PresentOfficeComputer(float DeltaSeconds,int32 EmployeeSeed=1);
 UFUNCTION(BlueprintPure) FString GetPresentationReport() const;
 static FVector LegAngles(float HipHeight,float FootX,float FootHeight);
 // Measured seat cushion top141cm, cabin floor83cm in authored forklift.
 static FVector ForkliftSeatOffset(){return {-106,0,0};}
private:
 USceneComponent* Joint(FName Name,USceneComponent* Parent,FVector Position);
 UStaticMeshComponent* Part(FName Name,USceneComponent* Parent,FVector Centre,FVector Size,const TCHAR* Material);
 void Arm(int32 Side,FVector Wrist);
 void ArmPalm(int32 Side,FVector Palm);
 UPROPERTY() TObjectPtr<USceneComponent> Pelvis;
 UPROPERTY() TObjectPtr<UCapsuleComponent> WalkingBody;
 UPROPERTY() TObjectPtr<USkeletalMeshComponent> SkeletalBody;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Thighs;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Shins;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Feet;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Shoulders;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Elbows;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Hands;
 bool InitializeHandyman();
 void PresentHandyman(const FFoundrylineEmployeeRecord& Record,float DeltaSeconds);
 void HandymanPalm(int32 Side,FVector EmployeePalm,bool bGrip=false);
 void CommitHandymanPose();
 void HandymanIK(FName Upper,FName Lower,FName End,FVector Target,FVector Pole,const FQuat* EndRotation=nullptr);
 void HandymanRotate(int32 Bone,FQuat Delta);
 UPROPERTY() TObjectPtr<UPoseableMeshComponent> HandymanBody;
 UPROPERTY() TObjectPtr<UFoundrylineWeldingHoodComponent> WeldingHood;
 bool bWeldingHoodBody=false;
 TArray<FTransform> HandymanPose,HandymanRest;
 TArray<int32> HandymanParents,HandymanSamplerBones;
 TMap<FName,int32> HandymanBones;
 int32 HandymanVariant=-1;
 float HandymanClock=0;
 FString HandymanAnimation;
 FVector HandymanPalmOffsets[2];
 FString Identity;
 FString ActivityName;
 float DisplayDistance=0,LastDistance=0,Phase=0,WorkPhase=0;
 float HipHeight=92;
 float WeldingGripError=-1;
 bool bBuilt=false,bMoving=false,bSeated=false;
};
