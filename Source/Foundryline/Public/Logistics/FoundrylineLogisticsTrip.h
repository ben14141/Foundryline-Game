#pragma once
#include "CoreMinimal.h"
#include "Pathing/FoundrylineVehiclePath.h"

// A player's trip draft and immutable route-check result, never shared construction state.
struct FFoundrylineLogisticsGate {
 FGuid SurfaceId;FString Label;FVector2D Portal;float InwardYaw=0;FFoundrylineVehiclePose Entry,Exit;
};
struct FFoundrylineLogisticsTripResult {
 bool Success=false;int64 Revision=-1;FString Reason;FFoundrylineVehicleRoute Inbound,Outbound;
};
struct FOUNDRYLINE_API FFoundrylineLogisticsTrip {
 bool Semi=true,SmallTruck=false,HasEntry=false,HasExit=false;FGuid TargetId;FFoundrylineLogisticsGate Entrance,Departure;
 FFoundrylineVehicleProfile Vehicle() const {return !Semi?FFoundrylineVehicleProfile::Car():SmallTruck?FFoundrylineVehicleProfile::BoxTruck():FFoundrylineVehicleProfile::Truck();}
 static TArray<FFoundrylineLogisticsGate> Gates(const FFoundrylineDrivingMap& Map,bool Semi,bool SmallTruck=false);
 static FString TargetLabel(const FFoundrylineDrivingMap& Map,FGuid Target);
 static FVector2D TargetMarker(const FFoundrylineVehicleTarget& Target);
 bool Select(const FFoundrylineDrivingMap& Map,FName Step,FVector2D Point,FString& Message);
 bool Ready(const FFoundrylineDrivingMap& Map) const;
 static FFoundrylineLogisticsTripResult Test(const FFoundrylineDrivingMap& Map,const FFoundrylineLogisticsTrip& Trip);
};
