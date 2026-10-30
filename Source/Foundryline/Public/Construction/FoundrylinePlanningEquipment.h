#pragma once
#include "CoreMinimal.h"
#include "FoundrylinePlanningEquipment.generated.h"

USTRUCT()
struct FFoundrylinePlacedStorageRack {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FVector Center=FVector::ZeroVector;
 UPROPERTY(SaveGame) int32 QuarterTurns=0;
 UPROPERTY(SaveGame) int32 Levels=1;
 UPROPERTY(SaveGame) bool FinishedGoods=false;
 UPROPERTY(SaveGame) TArray<int32> Slots;
};

struct FOUNDRYLINE_API FFoundrylinePlanningEquipment {
 // 0 fabrication station, 1 parts rack, 2 finished-goods rack, 3 forklift.
 static int64 Price(int32 Item,int32 Levels=1);
 static FVector Size(int32 Item,int32 Levels=1);
 static FString Mesh(int32 Item,int32 Levels=1);
 static FBox2D Bounds(FVector Center,int32 Quarter,FVector Size);
 static FBox2D Service(FVector Center,int32 Quarter,float Width=310);
};
