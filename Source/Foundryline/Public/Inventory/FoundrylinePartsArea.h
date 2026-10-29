#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylinePartsArea.generated.h"

// Physical starter parts storage and staging, not generated inventory quantities.
UCLASS()
class FOUNDRYLINE_API AFoundrylinePartsArea:public AActor {
 GENERATED_BODY()
public:
 AFoundrylinePartsArea();
 virtual void BeginPlay()override;
 static constexpr int32 WidthCm=1400;
 static constexpr int32 DepthCm=800;
};
