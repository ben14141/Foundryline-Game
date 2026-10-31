#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FoundrylineOfficeStoreyLink.generated.h"
UCLASS() class FOUNDRYLINE_API UFoundrylineOfficeStoreyLink:public UActorComponent {
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<AActor> UpperActor;
 virtual void EndPlay(const EEndPlayReason::Type Reason)override;
 void Show(bool Visible,bool Roof);
 static TArray<class UFoundrylinePersonnelDoorComponent*> Doors(AActor* Actor);
 virtual void OnComponentDestroyed(bool DestroyingHierarchy)override;
};
