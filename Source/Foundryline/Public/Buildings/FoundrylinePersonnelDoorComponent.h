#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "FoundrylinePersonnelDoorComponent.generated.h"
class APawn;
// Runtime door state is separate from construction geometry and opening ownership.
UCLASS()
class FOUNDRYLINE_API UFoundrylinePersonnelDoorComponent : public UStaticMeshComponent
{
 GENERATED_BODY()
public:
 UFoundrylinePersonnelDoorComponent();
 virtual void TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* Function) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 void InitializeSwing(float Closed,float Swing,FGuid Id,bool Open=true);
 bool TryToggle(APawn* User);
 UFUNCTION(BlueprintPure,Category="Door") bool IsOpen() const {return bOpen;}
 FGuid OpeningId;
 FGuid BuildingObjectId;
 void ApplyNetworkOpen(bool Open);
private:
 UPROPERTY(ReplicatedUsing=StateChanged) bool bOpen=true;
 UPROPERTY(Replicated) float ClosedYaw=0;
 UPROPERTY(Replicated) float SwingYaw=90;
 UFUNCTION() void StateChanged();
 float Progress=1;
};
