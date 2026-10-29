#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Construction/FoundrylinePlacementTypes.h"
#include "Companies/FoundrylineBuildRegion.h"
#include "FoundrylinePlacementSave.generated.h"

UCLASS()
class FOUNDRYLINE_API UFoundrylinePlacementSave : public USaveGame
{
	GENERATED_BODY()
public:
	static constexpr int32 CurrentSchemaVersion = 2;
	UPROPERTY(SaveGame)
	int32 SchemaVersion = CurrentSchemaVersion;
	UPROPERTY(SaveGame)
	int64 Revision = 0;
	UPROPERTY(SaveGame)
	TArray<FFoundrylineBuildRegion> Regions;
	UPROPERTY(SaveGame)
	TArray<FFoundrylinePlacedObject> Objects;
};
