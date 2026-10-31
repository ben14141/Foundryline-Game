#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "FoundrylineCarrierSceneActor.generated.h"
UCLASS(BlueprintType)
class FOUNDRYLINE_API UFoundrylineFreightVehicleLibrary:public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<class UStaticMesh>> Bodies;
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<class UStaticMesh>> Beds;
 UPROPERTY(EditAnywhere) TArray<FVector> BodyOffsets;
 UPROPERTY(EditAnywhere) TArray<float> YawOffsets;
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<class UStaticMesh>> StarterWheels;
 UPROPERTY(EditAnywhere) TArray<FVector> StarterWheelPositions;
};
UCLASS()
class FOUNDRYLINE_API AFoundrylineCarrierSceneActor:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineCarrierSceneActor();
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor",meta=(DevelopmentOnly)) bool ConfigureFlatbedReview(bool Parked=false);
 void SetBooking(const FFoundrylineFreightBooking& Booking,const FFoundrylineCommerceState& State,const FTransform& Plant,const TArray<FFoundrylineFactoryPallet>& Pallets);
 virtual void Tick(float Dt)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
private:
 UPROPERTY(ReplicatedUsing=Rebuild) int32 Kind=1;
 UPROPERTY(ReplicatedUsing=Rebuild) TArray<FFoundrylineFreightItem> Cargo;
 UPROPERTY(Replicated) bool bDocked=false;
 UPROPERTY(Replicated) int32 BookingId=0;
 UFUNCTION() void Rebuild();
 UPROPERTY() TObjectPtr<class USceneComponent> Visual;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Body;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Bed;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Gate;
 UPROPERTY() TArray<TObjectPtr<class UBoxComponent>> PawnCollision;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Proxy;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Pallets;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Steel;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Wheels;
 UPROPERTY() TArray<TObjectPtr<class UInstancedStaticMeshComponent>> CargoParts;
 UPROPERTY() TObjectPtr<class USceneComponent> FlatbedTractor;
 UPROPERTY() TObjectPtr<class USceneComponent> FlatbedTrailer;
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> FlatbedParts;
 bool bFlatbedReady=false;
 FVector PreviousHitch;
 float TrailerYaw=0;
 void RebuildFlatbed();
 void TickFlatbed(float Dt,float Distance);
 float WheelAngle=0;
 float SteeringAngle=0;
 float GateAngle=0;
 FRotator DisplayRotation;
 FVector Display;
 bool bDisplayReady=false;
 int32 LastKind=-1;
 int32 LastBooking=0;
 FString CargoKey;
};
