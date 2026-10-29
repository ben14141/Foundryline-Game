#pragma once
#include "CoreMinimal.h"
#include "Pathing/FoundrylineDrivingLayout.h"

struct FFoundrylineVehicleProfile {
 bool Semi=false,Freight=false;float Front=350,Rear=100,Width=180,Clearance=15,TurnRadius=400;
 float MaxArticulationDegrees=70;
 float Hitch=100,TrailerAxle=1100,TrailerFront=200,TrailerRear=1420,TrailerWidth=255;
 static FFoundrylineVehicleProfile Car(){return {};}
 bool IsFreight() const {return Semi || Freight;}
 static FFoundrylineVehicleProfile BoxTruck(){FFoundrylineVehicleProfile P;P.Freight=true;P.Front=650;P.Rear=150;P.Width=240;P.Clearance=20;P.TurnRadius=700;return P;}
 static FFoundrylineVehicleProfile Truck(){FFoundrylineVehicleProfile P;P.Semi=P.Freight=true;P.Front=450;P.Rear=150;P.Width=255;P.Clearance=20;P.TurnRadius=1000;return P;}
};
struct FFoundrylineVehiclePose {FVector2D Position=FVector2D::ZeroVector;float Yaw=0,TrailerYaw=0;bool Reverse=false;};
struct FFoundrylineDriveSurface {
 FGuid Id;FName Function;FTransform Transform;FVector2D Size;bool Public=false;
 TArray<FFoundrylineDriveRect> Entrances,Aisles,Islands,TruckZones;
 FVector2D Local(FVector2D P) const {const auto L=Transform.InverseTransformPosition(FVector(P,0));return {L.X,L.Y};}
 FVector2D World(FVector2D P) const {const auto W=Transform.TransformPosition(FVector(P,0));return {W.X,W.Y};}
};
struct FFoundrylineVehicleTarget {
 FGuid Id,SurfaceId;bool Semi=false,Loading=false,LayoutClear=true,SmallTruckClear=true;FFoundrylineVehiclePose Pose;FFoundrylineDriveRect Bay;
};
struct FOUNDRYLINE_API FFoundrylineDrivingMap {
 int64 Revision=0;TArray<FFoundrylineDriveSurface> Surfaces;TArray<FFoundrylineVehicleTarget> Targets;
 bool PointAllowed(FVector2D P,bool Semi,bool UseZones,const FGuid& Target) const;
 bool ReverseAllowed(FVector2D P) const;
 bool LaneAllowed(const FFoundrylineVehiclePose& Pose,bool Semi=false) const;
};
struct FFoundrylineVehicleRoute {
 bool Success=false;FString Reason;TArray<FFoundrylineVehiclePose> Poses;int32 Expanded=0;double Milliseconds=0;int64 Revision=0;
};
// Pure bounded hybrid A*: orientation, steering curvature, reversing and articulated trailer sweep.
// No UObject/world access; callers may run queries on a worker using immutable snapshots.
struct FOUNDRYLINE_API FFoundrylineVehiclePath {
 static FFoundrylineVehiclePose TargetPose(const FFoundrylineVehicleTarget& Target,const FFoundrylineVehicleProfile& Vehicle);
 static FFoundrylineVehiclePose Advance(FFoundrylineVehiclePose P,const FFoundrylineVehicleProfile& V,float Distance,float Curvature);
 static bool PoseClear(const FFoundrylineDrivingMap& Map,const FFoundrylineVehicleProfile& V,const FFoundrylineVehiclePose& Pose,bool Zones=true,FGuid Target={});
 static FFoundrylineVehicleRoute Find(const FFoundrylineDrivingMap& Map,const FFoundrylineVehicleProfile& V,FFoundrylineVehiclePose Start,FFoundrylineVehiclePose Goal,bool Zones=true,FGuid Target={},int32 MaxExpanded=40000);
};
