#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Pathing/FoundrylineVehiclePath.h"
#include "FoundrylineVehiclePathSubsystem.generated.h"
class UFoundrylinePlacementSubsystem;
UCLASS()
class FOUNDRYLINE_API UFoundrylineVehiclePathSubsystem : public UWorldSubsystem
{
 GENERATED_BODY()
public:
 TSharedPtr<const FFoundrylineDrivingMap,ESPMode::ThreadSafe> GetMap();
 static TArray<FFoundrylineDriveRect> Entrances(const UFoundrylinePlacementSubsystem& Placement,const struct FFoundrylinePlacedObject& Object);
 UFUNCTION(BlueprintPure,Category="Driving") int32 GetParkingTargetCount();
 UFUNCTION(BlueprintPure,Category="Driving") int32 GetTruckTargetCount();
 UFUNCTION(BlueprintPure,Category="Driving") FString GetDrivingDigest();
 void DrawOverlay(FVector ViewLocation,const FFoundrylineVehicleRoute* Route=nullptr,bool Semi=false) const;
 bool ReserveTarget(FGuid Target,FGuid Vehicle);void ReleaseTarget(FGuid Target,FGuid Vehicle);
 bool IsTargetAvailable(FGuid Target) const;
private:
 TSharedPtr<const FFoundrylineDrivingMap,ESPMode::ThreadSafe> Cached;
 TMap<FGuid,FGuid> OccupiedTargets;
};
