#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FoundrylineNetworkTestTools.generated.h"

// Editor-only session operation. No gameplay command or network privilege is exposed.
UCLASS()
class UFoundrylineNetworkTestTools : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Collision",meta=(DevelopmentOnly)) static bool PlacePlayerCollisionReview(APlayerController* Player,FVector Position,float Yaw);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Collision") static FString AuthoritativePlayerCollisionPosition(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Collision",meta=(DevelopmentOnly)) static FString ProbePlayerCollision(APlayerController* Player,FVector Start,FVector End);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Employees",meta=(DevelopmentOnly)) static bool SetupEmployeeJobOffice(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Employees",meta=(DevelopmentOnly)) static bool QueueFactoryOrderZone(APlayerController* Player,int32 ZoneId);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Employees",meta=(DevelopmentOnly)) static bool QueueEmployeeHire(APlayerController* Player,int32 Role,FGuid RequestId,FGuid TargetPlant);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Employees") static FString EmployeeReport(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Employees",meta=(DevelopmentOnly)) static bool QueueEmployeeAssignment(APlayerController* Player,int32 Id,int32 Role,int32 Target,FGuid TargetPlant);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Commerce",meta=(DevelopmentOnly)) static bool QueueCommerceAction(APlayerController* Player,const FString& Action);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Commerce",meta=(DevelopmentOnly)) static bool QueueCommerceForeignTarget(APlayerController* Player,FGuid PlantId);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Commerce") static FString CommerceReport(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Commerce",meta=(DevelopmentOnly)) static bool AdvanceCommerceTest(APlayerController* Host,int32 Steps);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Commerce",meta=(DevelopmentOnly)) static bool SetupOperationsReview(APlayerController* Player,int32 Scene);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Commerce",meta=(DevelopmentOnly)) static AActor* CreateReviewCamera(APlayerController* Player);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Commerce") static FString OperationsUIReport(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Companies",meta=(DevelopmentOnly)) static FString ExerciseBusinessFoundation(APlayerController* Host,int32 Phase=0);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Companies") static FString ScopedPlantDigest(APlayerController* Player);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Companies") static FString OwnBusinessReport(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool PrepareEmptyFactoryZones(APlayerController* Host);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool QueueFactoryPreset(APlayerController* Player,FVector Center,int32 Preset,int32 Quarter,int32 Side);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static FString InspectFactoryZones(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool QueueFactoryConsole(APlayerController* Player,int32 Machine=1);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool QueueFactoryAction(APlayerController* Player,FName Action);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool AdvanceFactoryTest(APlayerController* Host,int32 Steps);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Camera",meta=(DevelopmentOnly)) static bool SetupWorkshopReview(APlayerController* Player,bool Interior=false);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Camera",meta=(DevelopmentOnly)) static bool SetupPlasmaReview(APlayerController* Player,bool CloseUp=false);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Camera",meta=(DevelopmentOnly)) static bool SetupWeldingReview(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueMaterialControl(APlayerController* Player,int32 Action,float Throttle,float Steering,float Lift);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool SetupMaterialFixture(APlayerController* Host,int32 Scenario);

 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueFreightControl(APlayerController* Player,int32 Action,float Throttle,float Steering);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Camera",meta=(DevelopmentOnly)) static bool HoldCameraKey(APlayerController* Player,FName Key,float Seconds);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool ClientTrafficBlocked(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueLogisticsAction(APlayerController* Player,FName Action,FVector Point,bool Click);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool TestDrivingSave(APlayerController* Player,FString Slot,bool Load);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Network") static bool TestSaveBusy(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueTruckZone(APlayerController* Player,FIntVector Start,FIntVector End,bool Erase);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Network") static FString DrivingInventory(APlayerController* Player);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Network") static bool TruckPointAllowed(APlayerController* Player,FVector Point);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueConstructionTool(APlayerController* Player,FName Tool,FIntVector Start,FIntVector End,int32 Channel,FLinearColor Color);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static FString DescribeConstruction(APlayerController* Player,FIntVector Cursor);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool PreviewConstruction(APlayerController* Player,FIntVector Start,FIntVector End);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool ClientDirectExecutionBlocked(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool DrivePlayer(APlayerController* Player,FVector Direction,float Seconds);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static FString InspectPlayer(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool RequestLateJoin();
 // Session lifecycle checks: kick a PIE client on the listen server, enter Property View / truck drive as a player would, and report server-side cleanup state.
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool KickClientForTest(APlayerController* Client);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueuePropertyView(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool QueueFreightDriveToggle(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Network",meta=(DevelopmentOnly)) static bool SpawnFreightRigForTest(APlayerController* Host,FVector Location);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Network") static FString SessionReport(APlayerController* Player);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static FString PreviewFactoryBrush(APlayerController* Player,FName Tool,FVector Start,FVector End,bool Dragging,bool Commit=false);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool AimFactoryBrush(APlayerController* Player,FVector Point);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool StartEquipmentExercise(APlayerController* Host);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static FString EquipmentMotionReport(APlayerController* Player);
 // Welding stations through the real controller-to-server command path; Foreign targets another plant ID.
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool QueueWeldingStation(APlayerController* Player,FVector Center,int32 Quarter,bool bSwap,bool bForeignPlant=false,bool bRemove=false,int32 StationId=0);
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static bool QueueFactoryLane(APlayerController* Player,int32 Kind,FVector A,FVector B,float Width);
 UFUNCTION(BlueprintPure,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static FString WeldingStationReport(APlayerController* Player);
 // Places the player's placement hologram at a model-space point with a chosen rotation/swap and returns the check result.
 UFUNCTION(BlueprintCallable,Category="Foundryline|Editor|Factory",meta=(DevelopmentOnly)) static FString PreviewWeldingStation(APlayerController* Player,FVector Center,int32 Quarter,bool bSwap,bool bCommit=false);
};
