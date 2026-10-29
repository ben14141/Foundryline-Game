#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineFreightRig.generated.h"

USTRUCT(BlueprintType)
struct FFoundrylineFreightPose {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FVector Tractor=FVector(700,0,0);
 UPROPERTY(BlueprintReadOnly) FVector Trailer=FVector::ZeroVector;
 UPROPERTY(BlueprintReadOnly) float TractorYaw=0;
 UPROPERTY(BlueprintReadOnly) float TrailerYaw=0;
 UPROPERTY(BlueprintReadOnly) float Speed=0;
 UPROPERTY(BlueprintReadOnly) float Steering=0;
 UPROPERTY(BlueprintReadOnly) float Gear=1;
 UPROPERTY(BlueprintReadOnly) float TractorTravel=0;
 UPROPERTY(BlueprintReadOnly) float TrailerTravel=0;
 UPROPERTY(BlueprintReadOnly) bool Coupled=false;
 UPROPERTY(BlueprintReadOnly) bool Detaching=false;
 UPROPERTY() float SampleTime=0;
};

// Kinematic low-speed tractor/trailer test. Continuous vehicle state is separate
// from construction records. Only the server advances motion and hitch state.
UCLASS()
class FOUNDRYLINE_API AFoundrylineFreightRig : public AActor {
 GENERATED_BODY()
public:
 AFoundrylineFreightRig();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 UPROPERTY(ReplicatedUsing=OnRep_State,BlueprintReadOnly) FFoundrylineFreightPose State;
 UPROPERTY(EditAnywhere,Replicated,BlueprintReadOnly) bool bTractorOnly=false;
 UFUNCTION() void OnRep_State();
 UPROPERTY(Replicated) FGuid BusinessCompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid BusinessPlantId=FGuid(0xF001,2,1,1);
 UPROPERTY(Replicated,BlueprintReadOnly) TObjectPtr<class APlayerState> Driver;
 UPROPERTY(Replicated,BlueprintReadOnly) FString Feedback;
 bool Claim(class APlayerController* Player,bool Take);
 void Drive(class APlayerController* Player,float Throttle,float Steering);
 bool Command(class APlayerController* Player,uint8 Action);
 bool TryCouple();
 bool SetGear(bool Down);
 bool Uncouple();
 void Step(float Dt);
 FVector Hitch(bool Tractor) const;
 FVector TractorWorldPosition() const;
 bool ClearPose(const FFoundrylineFreightPose& Pose) const;
private:
 void UpdateVisuals(float Dt);
 FFoundrylineFreightPose PreviousState,DisplayState;
 TArray<FFoundrylineFreightPose> SnapshotBuffer;
 bool bDisplayReady=false,bAutoCoupleReady=true;
 UPROPERTY() TObjectPtr<class USceneComponent> TractorRoot;
 UPROPERTY() TObjectPtr<class USceneComponent> TrailerRoot;
 UPROPERTY() TObjectPtr<class UBoxComponent> TractorCollision;
 UPROPERTY() TObjectPtr<class UBoxComponent> TrailerCollision;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Wheels;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Legs;
 TArray<bool> TrailerWheel;
 TArray<bool> SteerWheel;
 TArray<float> WheelRadius;
 TArray<FVector> LegRest;
 float ThrottleInput=0,SteerInput=0,InputAge=0,Accumulator=0,GearTarget=1;
};
