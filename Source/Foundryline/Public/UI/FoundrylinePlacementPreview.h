#pragma once

#include "CoreMinimal.h"
#include "Construction/FoundrylinePlacementSubsystem.h"

// Rendering contract; no actor, material, collision or authoritative mutation is created here.
struct FOUNDRYLINE_API FFoundrylinePlacementPreview
{
	bool bHasGeometry = false;
	FFoundrylinePlacementResult Validation;
	FFoundrylinePlacementRequest Request;
	FTransform MeshTransform = FTransform::Identity;
	TSoftObjectPtr<UStaticMesh> Mesh;
	FFoundrylineGridBounds Footprint;
	FFoundrylineGridBounds WorkingClearance;
	int64 PriceCents = 0;
	static FFoundrylinePlacementPreview Build(const UFoundrylinePlacementSubsystem& Placement,
		const FFoundrylinePlacementContext& Context, FName DefinitionId, FVector GroundCm, uint8 QuarterTurns);
	static FText ErrorText(EFoundrylinePlacementError Error);
};
