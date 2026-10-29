#pragma once
#include "CoreMinimal.h"
#include "Construction/FoundrylinePlacementTypes.h"
#include "Companies/FoundrylineBuildRegion.h"
#include "FoundrylinePropertyState.generated.h"

USTRUCT()
struct FFoundrylineDoorState
{
 GENERATED_BODY()
 UPROPERTY() FGuid ObjectId;
 UPROPERTY() FGuid OpeningId;
 UPROPERTY() bool bOpen=true;
};

// One replicated transaction image: records arrive together before local visual reconstruction.
// Simulation remains server-owned; generated meshes, lights and materials stay local to each view.
USTRUCT()
struct FFoundrylinePropertyState
{
 GENERATED_BODY()
 UPROPERTY() int64 Revision=0;
 UPROPERTY() bool bStarterProperty=false;
 UPROPERTY() TArray<FFoundrylineBuildRegion> Regions;
 UPROPERTY() TArray<FFoundrylinePlacedObject> Objects;
 UPROPERTY() TArray<FFoundrylineDoorState> Doors;
};
