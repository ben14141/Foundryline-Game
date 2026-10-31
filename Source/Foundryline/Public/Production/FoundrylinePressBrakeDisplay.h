#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "FoundrylinePressBrakeDisplay.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylinePressBrakeDisplay:public AActor {
 GENERATED_BODY()
public:
 AFoundrylinePressBrakeDisplay();
 UPROPERTY(Replicated) FGuid BusinessCompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid BusinessPlantId=FGuid(0xF001,2,1,1);
 UPROPERTY(Replicated) int32 MachineId=2;
 virtual void BeginPlay()override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 void SetFactoryPresentation(float Phase,bool Active,bool HasWork=true,EFoundrylinePart Part=EFoundrylinePart::BentBracket);
private:
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Machine;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Moving;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Gauge;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> FlatPart;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> BentPart;
 EFoundrylinePart ShownPart=EFoundrylinePart::Count;
};
