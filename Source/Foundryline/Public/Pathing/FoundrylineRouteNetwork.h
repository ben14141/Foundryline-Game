#pragma once
#include "CoreMinimal.h"
#include "Pathing/FoundrylineVehiclePath.h"
#include "FoundrylineRouteNetwork.generated.h"

USTRUCT()
struct FFoundrylineRouteLink {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid To;
 UPROPERTY(SaveGame) bool Reverse=false;
};
// Saved on owned paving. Position is local centimetres, never a rescaled mesh.
USTRUCT()
struct FFoundrylineRouteNode {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid Id;
 UPROPERTY(SaveGame) FIntPoint Position=FIntPoint::ZeroValue;
 UPROPERTY(SaveGame) TArray<FFoundrylineRouteLink> Links;
 UPROPERTY(SaveGame) int32 RadiusCm=1200;
 UPROPERTY(SaveGame) bool Staging=false;
};
struct FFoundrylineWorldRouteNode {
 FFoundrylineRouteNode Data;FGuid Surface,Company,Plant;FVector2D Position;
};
struct FFoundrylineNetworkCheck {
 bool Success=false,Comfortable=false;FString Reason;
 int64 Revision=-1;TArray<FGuid> Nodes;TArray<float> PoseTimes;FFoundrylineVehicleRoute Route;
};
struct FOUNDRYLINE_API FFoundrylineRouteNetwork {
 int64 Revision=0;int32 BrokenLinks=0;
 TArray<FFoundrylineWorldRouteNode> Nodes;
 static FFoundrylineRouteNetwork Read(const class UFoundrylinePlacementSubsystem& Placement);
 const FFoundrylineWorldRouteNode* Find(FGuid Id) const;
 // Dijkstra uses directed links and optional immutable, server-provided wait estimates.
 TArray<FGuid> Path(FGuid From,FGuid To,const TMap<FGuid,float>& WaitSeconds={}) const;
 // Follows the polyline with circular fillets. Never falls back to a route that ignores nodes.
 FFoundrylineNetworkCheck Check(const FFoundrylineDrivingMap& Map,const FFoundrylineVehicleProfile& Vehicle,FGuid From,FGuid To,const TMap<FGuid,float>& WaitSeconds={}) const;
};
