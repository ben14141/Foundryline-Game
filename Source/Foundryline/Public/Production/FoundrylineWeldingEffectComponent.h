#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FoundrylineWeldingEffectComponent.generated.h"

// Attach to the MIG gun's WeldTip socket. Replicate activity, never individual sparks.
UCLASS(ClassGroup=(Foundryline),meta=(BlueprintSpawnableComponent))
class FOUNDRYLINE_API UFoundrylineWeldingEffectComponent : public USceneComponent {
 GENERATED_BODY()
public:
 static constexpr int32 MaxSparks=640;
 UFoundrylineWeldingEffectComponent();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 UFUNCTION(BlueprintCallable,Category="Welding") void SetWelding(bool Active);
 UFUNCTION(BlueprintPure,Category="Welding") bool IsWelding() const {return bUseLocalActivity?bLocalPlasmaActive:bWelding;}
 // Plasma activity follows the replicated machine clock locally, not particle RPCs.
 UPROPERTY(EditAnywhere,Category="Effect") bool bPlasmaCutting=false;
 void SetLocalPlasmaActive(bool Active);
 // Factory tools derive activity from the replicated simulation, retaining MIG visuals.
 void SetLocalActivity(bool Active);
 UFUNCTION(BlueprintPure,Category="Welding") int32 GetVisibleSparkCount() const;
 UFUNCTION(BlueprintPure,Category="Welding") int32 GetGroundImpactCount() const {return GroundImpacts;}
private:
 UPROPERTY(ReplicatedUsing=OnRep_Welding) bool bWelding=false;
 UFUNCTION() void OnRep_Welding();
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Arc;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Sparks;
 UPROPERTY() TObjectPtr<class UPointLightComponent> Light;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> ArcMaterial;
 struct FSpark {FVector Position,Velocity;float Age=0,Life=1.5f,Width=.002f,Heat=1;uint8 Bounces=0;};
 TArray<FSpark> Particles;
 float Clock=0,SpawnBudget=0,FloorQueryClock=0;
 FVector FloorPoint=FVector::ZeroVector,FloorNormal=FVector::UpVector;
 FBox FloorBounds=FBox(ForceInit);
 bool bHasFloor=false;
 int32 GroundImpacts=0;
 void UpdateFloor(float Dt);
 bool bLocalPlasmaActive=false;
 bool bUseLocalActivity=false;
 void ResetVisuals();
};
