#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Orders/FoundrylineCommerceTypes.h"
#include "FoundrylineOperationsState.generated.h"
// Commerce details are visible only to members; physical factory records remain public.
UCLASS()
class FOUNDRYLINE_API AFoundrylineOperationsState : public AActor {
 GENERATED_BODY()
public:
 AFoundrylineOperationsState();
 UPROPERTY(Replicated) FGuid CompanyId;
 UPROPERTY(Replicated) FGuid PlantId;
 UPROPERTY(Replicated) FFoundrylineCommerceState Data;
 virtual void Tick(float Dt)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 virtual bool IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& Source)const override;
};
