#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Management/FoundrylineOfficeTypes.h"
#include "FoundrylineOfficeDecorationComponent.generated.h"
UCLASS() class FOUNDRYLINE_API UFoundrylineOfficeDecorationComponent:public USceneComponent {
 GENERATED_BODY()
public:
 void Apply(const FFoundrylineOfficeAssignment& Assignment);
 uint8 Floor()const{return Source.Floor;}
 int32 PieceCount()const{return Source.Decorations.Num();}
private:
 UPROPERTY() FFoundrylineOfficeAssignment Source;
 UPROPERTY() TArray<TObjectPtr<USceneComponent>> Pieces;
 bool Built=false;
};
