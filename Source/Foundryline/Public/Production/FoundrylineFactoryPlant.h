#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "FoundrylineFactoryPlant.generated.h"

UENUM()
enum class EFoundrylineFactoryAction:uint8 {Run,Speed,Order,Collect,ReceiveSteel,ReceiveKits,Program,MachineEnabled,AddDriver,AddWelder,AssignSlot,AddZone,PaintLane,EraseLane,AddRoute,LinkSource,LinkDestination,Save,Load,AddPresetZone,AssignEmployee,HireEmployee,ConfigureSupplyBin,BuyPlanningEquipment,AddWeldingStation,RemoveWeldingStation,ConfigureWeldingStation};

USTRUCT()
struct FFoundrylineFactoryCommand {
 GENERATED_BODY()
 UPROPERTY() EFoundrylineFactoryAction Action=EFoundrylineFactoryAction::Run;
 UPROPERTY() FGuid TargetPlantId;
 UPROPERTY() FGuid RequestId;
 UPROPERTY() int32 Id=0;
 UPROPERTY() int32 Other=0;
 UPROPERTY() int32 Count=1;
 UPROPERTY() int32 QuarterTurns=0;
 UPROPERTY() int32 SupplySide=1;
 UPROPERTY() float WidthCm=400;
 UPROPERTY() EFoundrylinePart Part=EFoundrylinePart::Bracket;
 UPROPERTY() bool Flag=true;
 UPROPERTY() FVector A=FVector::ZeroVector;
 UPROPERTY() FVector B=FVector::ZeroVector;
};
UCLASS()
class FOUNDRYLINE_API AFoundrylineFactoryPlant:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineFactoryPlant();
 UPROPERTY(EditAnywhere,Category="Foundryline|Employees") TObjectPtr<class UFoundrylineEmployeeAppearance> EmployeeAppearance;
 bool ExecuteCommerce(class APlayerController* Player,const FFoundrylineCommerceCommand& Command,FString& Error);
 FVector MachineWorldPosition(int32 Id)const;
 FVector ScreenWorldPosition(int32 Id)const;
 void PublishCommerce();
 void EnsureCommerceEquipment();
 UPROPERTY(Replicated) FGuid BusinessCompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid BusinessPlantId=FGuid(0xF001,2,1,1);
 UPROPERTY(Replicated) bool bCutPartsStarter=false;
 virtual void BeginPlay()override;
 virtual void Tick(float Dt)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 UPROPERTY(ReplicatedUsing=OnRep_State) FFoundrylineFactoryState Snapshot;
 UFUNCTION() void OnRep_State();
 UPROPERTY(Replicated) double PresentationEpoch=0;
 UFUNCTION(BlueprintPure,Category="Factory") FString GetLedgerDigest()const;
 UFUNCTION(BlueprintPure,Category="Factory") FString GetFactoryReport()const;
 UFUNCTION(BlueprintPure,Category="Factory") int32 GetFinishedProducts()const{return Snapshot.FinishedProducts;}
 UFUNCTION(BlueprintPure,Category="Factory") int32 GetPalletCount()const{return Snapshot.Pallets.Num();}
 UFUNCTION(BlueprintPure,Category="Factory") int32 GetZoneCount()const{return Snapshot.Zones.Num();}
 UFUNCTION(BlueprintPure,Category="Factory") int32 GetCompletedZoneCount()const{int32 Count=0;for(const auto& Zone:Snapshot.Zones)if(Zone.CompletedMask==63)++Count;return Count;}
 bool Execute(class APlayerController* Player,const FFoundrylineFactoryCommand& Command,FString& Error);
 bool SaveFactory(FString& Error,const FString& TestSlot=FString());
 bool LoadFactory(FString& Error,const FString& TestSlot=FString());
 static FVector ScreenPosition(int32 Machine);
 static FVector MachinePosition(int32 Machine);
 bool bLocalOverlay=false;
 FFoundrylineFactorySimulation Model;
 // Presentation read-outs for review tools: guns shown in welders' hands and spark emitters active at a station (snapshot index).
 int32 StationGunsInHand(int32 StationIndex)const;
 int32 StationSparksActive(int32 StationIndex)const;
private:
 UPROPERTY() TObjectPtr<class AFoundrylineOperationsState> OperationsState;
 bool bCommerceRackBuilt=false;
 void UpdateSupplyVisuals(float Dt);
 UPROPERTY() TMap<int32,TObjectPtr<class UStaticMeshComponent>> SupplyBinVisuals;
 UPROPERTY() TMap<int32,TObjectPtr<class UStaticMeshComponent>> PlanningRackVisuals;
 UPROPERTY() TMap<int32,TObjectPtr<class UStaticMeshComponent>> CollectorCartVisuals;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> CollectorCartWheels;
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> CommerceRackComponents;
 UPROPERTY() TArray<TObjectPtr<class AFoundrylineCarrierSceneActor>> CarrierVisuals;
 float SimClock=0,PublishClock=0;
 bool bRebuild=true;
 UPROPERTY() TArray<TObjectPtr<class UInstancedStaticMeshComponent>> PartMeshes;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Pallets;
 UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> FloorPaintTexture;
 UPROPERTY() TObjectPtr<class UDecalComponent> FloorPaintDecal;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> FloorPaintMaterial;
 UPROPERTY() TArray<TObjectPtr<class UInstancedStaticMeshComponent>> CargoMeshes;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> CargoPallets;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> ProductMeshes;
 UPROPERTY() TArray<TObjectPtr<class UBoxComponent>> ProductCollision;
 UPROPERTY() TArray<TObjectPtr<class UBoxComponent>> WelderCollision;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> WeldingMachines;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> WeldingGuns;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> IdleWeldingGuns;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> IdleWeldingLeads;
 UPROPERTY() TArray<TObjectPtr<class USplineMeshComponent>> WeldingLeads;
 UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> ProductMaterials;
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> Forks;
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> Carriages;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> ForkWheels;
 UPROPERTY() TMap<FString,TObjectPtr<class UFoundrylineEmployeeVisualComponent>> EmployeeVisuals;
 TArray<FFoundrylineEmployeeRecord> CachedEmployees;
 TMap<FString,FVector> EmployeeDisplayPositions;
 TMap<FString,float> EmployeeDisplayYaw,EmployeeDisplayDistance;
 UPROPERTY() TArray<TObjectPtr<class UFoundrylineWeldingEffectComponent>> WeldEffects;
 UPROPERTY() TObjectPtr<class AFoundrylinePlasmaTable> Plasma;
 UPROPERTY() TObjectPtr<class AFoundrylineMaterialHandlingCell> Cell;
 UPROPERTY() TObjectPtr<class AFoundrylinePressBrakeDisplay> Brake;
 TMap<int32,FVector> DisplayPositions;
 TMap<int32,float> DisplayYaw,DisplayLift;
 TArray<int32> ProductZones,ProductParts;
 TArray<float> ProductThresholds;
 FString VisualLayoutKey;
 FString FloorLayoutKey;


 void RefreshSnapshot();
 void SetupVisuals();
 void RebuildVisuals();
 void RebuildZoneWeldingEquipment();
 void Animate(float Dt);
 void UpdateEmployees(float Dt);
 void UpdateCollision();
 void UpdatePhysicalObjects(float Dt);
 UPROPERTY() TArray<TObjectPtr<class UBoxComponent>> ForkPhysicalBodies;
 UPROPERTY() TMap<int32,TObjectPtr<class UBoxComponent>> PalletPhysicalBodies;
 void SetupFloorVisuals();
 void RebuildFloor();
 // Bench, fixture and collision per placed welding station; rebuilt only when a station record changes.
 void RebuildWeldingStations();
 FString StationLayoutKey;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationMeshes;
 UPROPERTY() TArray<TObjectPtr<class UBoxComponent>> StationCollision;
 // Lead from each machine to its holstered gun, three segments per machine (station index * 2 + side).
 UPROPERTY() TArray<TObjectPtr<class USplineMeshComponent>> StationIdleLeads;
 // The unit on each station's bench, by snapshot station index: fixture kit stored on the shelf or set out as a jig on
 // the worktop, the rack as hologram, as weld progress and as the finished product, and Welder A's gun, lead and sparks.
 UPROPERTY() TArray<TObjectPtr<class USceneComponent>> StationUnitParts;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationStoredFixtures;
 // Eight jig parts per station (four risers, four hold-downs), in JigRisers/JigHoldDowns order.
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationJigParts;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationRackGhosts;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationRackBuilds;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationRacks;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationHolsteredGuns;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> StationGuns;
 UPROPERTY() TArray<TObjectPtr<class USplineMeshComponent>> StationLeads;
 UPROPERTY() TArray<TObjectPtr<class UFoundrylineWeldingEffectComponent>> StationWeldEffects;
 TArray<int32> StationShownStage;
 void AnimateWeldingStations(float Dt,double Age);
 void AddStationFitUpParts();
 static FString RackBuildSuffix(const class UMaterialInterface* Material);
 static bool IsRackLight(const class UMaterialInterface* Material);
 class UMaterialInstanceDynamic* RackProgressMaterial(class UMaterialInterface* Base,const FTransform& On);
};
