#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FoundrylineEquipmentFootprintComponent.generated.h"

// Local presentation of authored placement bounds, independent of replicated production state.
UCLASS(ClassGroup=(Foundryline),meta=(BlueprintSpawnableComponent))
class FOUNDRYLINE_API UFoundrylineEquipmentFootprintComponent:public USceneComponent {
 GENERATED_BODY()
public:
 UFoundrylineEquipmentFootprintComponent();
 virtual void BeginPlay() override;
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Placement") FName EquipmentId=TEXT("PressBrake");
 UFUNCTION(BlueprintCallable,Category="Placement") void ShowPlacementArea(bool Show);
 UFUNCTION(BlueprintPure,Category="Placement") const class UFoundrylinePlaceableDefinition* GetPlacementDefinition()const{return Definition;}
private:
 UPROPERTY() TObjectPtr<class UFoundrylinePlaceableDefinition> Definition;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Grid;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Boundary;
 UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
 bool bRequested=false;
};
