#pragma once
#include "Production/FoundrylineFactoryTypes.h"

enum class EFoundrylinePlannerItem:uint8 {None,Machine,StorageRack,FixedRack,Station,WorkZone,Forklift,Pallet,Lane};
struct FFoundrylinePlannerItem {
 EFoundrylinePlannerItem Kind=EFoundrylinePlannerItem::None;
 int32 Id=0;FString Name;FVector Center=FVector::ZeroVector;FBox Bounds=FBox(ForceInit);
 TArray<int32> Slots;int32 Quarter=0;bool Stored=false;
};
struct FOUNDRYLINE_API FFoundrylinePlannerItems {
 static TArray<FFoundrylinePlannerItem> List(const FFoundrylineFactoryState& State);
 static TArray<FFoundrylinePlannerItem> FixedRacks(const FFoundrylineFactoryState& State);
 static FBox MachineBounds(int32 Id,FVector Origin);
};
