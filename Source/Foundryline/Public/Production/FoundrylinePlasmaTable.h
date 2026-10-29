#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "FoundrylinePlasmaTable.generated.h"

struct FFoundrylinePlasmaPose {
 FVector Tip=FVector::ZeroVector;
 bool bCutting=false;
};

USTRUCT()
struct FFoundrylinePlasmaCycle {
 GENERATED_BODY()
 UPROPERTY() double Epoch=0;
 UPROPERTY() bool bOperating=false;
};

// Server owns operating state and epoch. Clients derive motion and bounded VFX.
UCLASS()
class FOUNDRYLINE_API AFoundrylinePlasmaTable : public AActor {
 GENERATED_BODY()
public:
 AFoundrylinePlasmaTable();
 UPROPERTY(Replicated) FGuid BusinessCompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid BusinessPlantId=FGuid(0xF001,2,1,1);
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 UPROPERTY(Replicated) bool bFactoryAutomation=false;
 void SetFactoryPresentation(float Phase,bool Active,bool HasWork=true,bool HasStock=true,EFoundrylinePart Part=EFoundrylinePart::Bracket){FactoryPhase=Phase;FactoryActive=Active;FactoryHasWork=HasWork;FactoryHasStock=HasStock;FactoryPart=Part;}
 UFUNCTION(BlueprintCallable,Category="Production") void SetOperating(bool Active);
 UFUNCTION(BlueprintPure,Category="Production") bool IsOperating() const {return Cycle.bOperating;}
 UFUNCTION(BlueprintPure,Category="Production") bool IsCutting() const {return bCutting;}
 UFUNCTION(BlueprintPure,Category="Production") FVector GetTorchTip() const;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UFoundrylineWeldingEffectComponent> PlasmaEffect;
 static FFoundrylinePlasmaPose EvaluateCycle(double Elapsed);
 static FFoundrylinePlasmaPose EvaluateFactoryCycle(float Phase,EFoundrylinePart Part);
 // FBX converts Blender +Y to Unreal -Y; match the imported mesh, not source coordinates.
 static FVector AuthoredTorchTip(){return FVector(-57.70909,-38.65331,100.95);}
private:
 UPROPERTY(Replicated) FFoundrylinePlasmaCycle Cycle;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Gantry;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Carriage;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Torch;
 UPROPERTY() TArray<TObjectPtr<class USplineMeshComponent>> Leads;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> EnergyChain;
 float LastChainX=100000;
 bool bCutting=false;
 float FactoryPhase=0;
 bool FactoryActive=false,FactoryHasWork=false,FactoryHasStock=false;
 EFoundrylinePart FactoryPart=EFoundrylinePart::Bracket;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StockSheet;
 double ServerTime() const;
};
