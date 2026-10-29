#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "FoundrylineMaterialHandlingCell.generated.h"
USTRUCT(BlueprintType)
struct FFoundrylineForkliftPose {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FVector Position=FVector(650,0,0);
 UPROPERTY(BlueprintReadOnly) float Yaw=180;
 UPROPERTY(BlueprintReadOnly) float Speed=0;
 UPROPERTY(BlueprintReadOnly) float Steering=0;
 UPROPERTY(BlueprintReadOnly) float Lift=0;
 UPROPERTY(BlueprintReadOnly) float Tilt=0;
 UPROPERTY(BlueprintReadOnly) float Travel=0;
};
// A starter material-handling cell: authored assets, independent stock records and
// server-owned forklift/transfer actions. This is not an employee production scheduler.
UCLASS()
class FOUNDRYLINE_API AFoundrylineMaterialHandlingCell:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineMaterialHandlingCell();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 UPROPERTY(Replicated) bool bFactoryAutomation=false;
 void SetFactoryRoller(float Phase,bool Active,bool HasWork=true);
 UPROPERTY(Replicated,BlueprintReadOnly) FFoundrylineForkliftPose Pose;
 UPROPERTY(ReplicatedUsing=OnRep_Stock,BlueprintReadOnly) TArray<FFoundrylineSheetStock> Stock;
 UPROPERTY(Replicated) FGuid BusinessCompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid BusinessPlantId=FGuid(0xF001,2,1,1);
 UPROPERTY(Replicated,BlueprintReadOnly) TObjectPtr<class APlayerState> Driver;
 UPROPERTY(Replicated,BlueprintReadOnly) FString Feedback;
 UPROPERTY(Replicated,BlueprintReadOnly) int32 Carried=INDEX_NONE;
 UPROPERTY(Replicated,BlueprintReadOnly) int32 TransferStock=INDEX_NONE;
 UPROPERTY(Replicated,BlueprintReadOnly) uint8 TransferAction=0;
 UPROPERTY(Replicated,BlueprintReadOnly) float RollerRemaining=0;
 UPROPERTY(Replicated,BlueprintReadOnly) int32 RolledBlanks=0;
 UFUNCTION() void OnRep_Stock();
 bool Claim(class APlayerController* Player,bool Take);
 void Drive(class APlayerController* Player,float Throttle,float Steering,float Lift);
 bool Command(class APlayerController* Player,uint8 Action);
 bool PickNearest();
 bool PlaceCarried(EFoundrylineSheetPlace Place,int32 Level=0);
 bool StartRoller();
 void Step(float Dt);
 FVector ForkliftWorldPosition() const;
 FVector LoadPosition(int32 Index) const;
 FVector LoadMount(const FFoundrylineForkliftPose& P) const;
 static FVector RackPosition(int32 Level);
 UPROPERTY(Replicated,BlueprintReadOnly) FVector RackOffsetCm=FVector::ZeroVector;
 UPROPERTY(EditAnywhere) FVector ArrivalPositionCm=FVector(-50,0,0);
 FVector GetRackCenter() const;
 FVector GetRackPosition(int32 Level) const;
 UFUNCTION(BlueprintPure,Category="Materials") FVector GetRackWorldPosition(int32 Level)const;
 int32 TotalSheets() const;
 int32 VisibleSheetCount(int32 Index) const;
 bool ClearMove(const FFoundrylineForkliftPose& P) const;
private:
 UPROPERTY() TObjectPtr<class USceneComponent> ForkRoot;
 UPROPERTY() TObjectPtr<class USceneComponent> MastRoot;
 UPROPERTY() TObjectPtr<class USceneComponent> CarriageRoot;
 UPROPERTY() TObjectPtr<class USceneComponent> InnerMastRoot;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Wheels;
 UPROPERTY() TArray<TObjectPtr<class UInstancedStaticMeshComponent>> Sheets;
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> StockRoots;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Rollers;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> FormingSheet;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> FeedSheet;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> CurvedOutput;
 TArray<float> Radius;
 TArray<bool> Steers;
 TArray<int32> DrawnCounts;
 FFoundrylineForkliftPose PreviousPose,DisplayPose;
 float ThrottleInput=0,SteerInput=0,LiftInput=0,InputAge=0,Accumulator=0,TargetLift=0,DisplayPhase=0;
 float LastFactoryPhase=-1;
 bool LastFactoryWork=false;
 int32 TargetLevel=0;
 EFoundrylineSheetPlace TargetPlace=EFoundrylineSheetPlace::Floor;
 void UpdateVisuals(float Dt);
 void RebuildStock();
};
