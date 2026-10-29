#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineWeldingStation.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylineWeldingStation:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineWeldingStation();
 virtual void BeginPlay() override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UFoundrylineWeldingEffectComponent> WeldingEffect;
private:
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Gun;
 FTimerHandle DemoTimer;
 void ToggleDemo();
};
