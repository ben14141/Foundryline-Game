#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/FoundrylineGrid.h"
#include "FoundrylinePlaceableDefinition.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EFoundrylineCollisionClass : uint8
{
	FunctionalBlocker,
	MinorObstruction,
	Cosmetic
};

// Authored bounds are in real Unreal centimetres. Mesh scale stays at 1:1; snap never resizes it.
UCLASS(BlueprintType)
class FOUNDRYLINE_API UFoundrylinePlaceableDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FName DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<UStaticMesh> Mesh;

	// Mesh pivot relative to placement anchor, in real Unreal centimetres.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FVector MeshOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	EFoundrylineSnap Snap = EFoundrylineSnap::Fine25cm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	FFoundrylineGridBounds Footprint;

	// Must enclose Footprint. Clearance volumes can overlap each other, but not solids.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	FFoundrylineGridBounds WorkingClearance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	EFoundrylineCollisionClass CollisionClass = EFoundrylineCollisionClass::FunctionalBlocker;

	// Explicit allowed surface IDs; empty means no valid surface, not unrestricted.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	TArray<FName> AllowedSurfaces;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Economy", meta=(ClampMin="0"))
	int64 PriceCents = 0;

	// Reserved for real inventory/production functionality; cosmetics never create tasks.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Simulation")
	FName FunctionId;

	bool IsDefinitionValid() const;
};
