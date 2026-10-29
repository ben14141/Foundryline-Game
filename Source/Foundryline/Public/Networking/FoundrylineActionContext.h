#pragma once

#include "CoreMinimal.h"

// Supplied by trusted local/server code. Never accept identity/authority from an RPC payload.
struct FOUNDRYLINE_API FFoundrylinePlacementContext
{
	FGuid CompanyId;
	FGuid PlantId;
	bool bCanConstruct = false;
};
