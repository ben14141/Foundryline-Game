#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UBoxComponent;
class AActor;
class USceneComponent;
// Physical walking bounds follow the actual primary body, independently of its
// larger placement/working-clearance overlay and production navigation graph.
struct FOUNDRYLINE_API FFoundrylineEquipmentCollision {
 static UBoxComponent* AddPawnBody(UStaticMeshComponent* Body);
 static UBoxComponent* AddBox(AActor* Owner,USceneComponent* Parent,FName SemanticName,FVector Center,FVector Extent);
};
