#pragma once

#include "CoreMinimal.h"
#include "Grid/FoundrylineGrid.h"
#include "FoundrylineBuildRegion.generated.h"

// Initial rectangular surface/ownership regions. Surface painting is a later layer.
USTRUCT(BlueprintType)
struct FOUNDRYLINE_API FFoundrylineBuildRegion
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGuid RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGuid CompanyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGuid PlantId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName SurfaceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FFoundrylineGridBounds Bounds;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bHasBuildingSetback=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FFoundrylineGridBounds BuildingBounds;
};
