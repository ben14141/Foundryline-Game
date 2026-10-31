#pragma once
#include "Production/FoundrylineFactoryTypes.h"

class UWorld;

enum class EFoundrylinePlannerItem:uint8 {None,Machine,StorageRack,FixedRack,Station,WorkZone,Forklift,Pallet,Lane,OfficeDesk};
struct FFoundrylinePlannerItem {
 EFoundrylinePlannerItem Kind=EFoundrylinePlannerItem::None;
 int32 Id=0;FString Name;FVector Center=FVector::ZeroVector;FBox Bounds=FBox(ForceInit);
 TArray<int32> Slots;int32 Quarter=0;bool Stored=false;
 bool CompactRack=false;int32 MachineId=0;
};
struct FOUNDRYLINE_API FFoundrylinePlannerItems {
 static TArray<FFoundrylinePlannerItem> List(const FFoundrylineFactoryState& State);
 static TArray<FFoundrylinePlannerItem> FixedRacks(const FFoundrylineFactoryState& State);
 static FBox MachineBounds(int32 Id,FVector Origin);
 static FBox2D StationWorkspaceBounds(FVector Center,int32 Quarter,bool Swap=false);
 // Expand the simulation envelope to owned building additions, then require the
 // complete workspace on the union of their real floors (not the envelope's gaps).
 // Call on a preview/candidate state before CanBuyPlanningEquipment/BuyMachine.
 static bool PrepareOwnedBuildingFloor(FFoundrylineFactoryState& State,UWorld* World,const FTransform& PlantTransform,const FBox2D& RequiredLocal,FString& Error,int32 OfficeDeskId=-1,double FloorZ=20);
};
