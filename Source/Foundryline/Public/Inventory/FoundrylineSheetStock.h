#pragma once
#include "CoreMinimal.h"
#include "FoundrylineSheetStock.generated.h"

// One starting stock format. Count and mass never depend on the visual instance count.
struct FFoundrylineSteelSheet {
 static constexpr double WidthCm=152.4,LengthCm=304.8,ThicknessCm=.3175;
 static double PoundsEach(){return WidthCm*LengthCm*ThicknessCm*.000001*7850.0/.45359237;}
 static TArray<int32> VisibleIndices(int32 Count) {
  if(Count<=0)return {};if(Count==1)return {0};if(Count==2)return {0,1};return {0,(Count-1)/2,Count-1};
 }
};
UENUM(BlueprintType)
enum class EFoundrylineSheetPlace:uint8 { Floor,Rack,Carried,RollerInput };
USTRUCT(BlueprintType)
struct FFoundrylineSheetStock {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FGuid Id;
 UPROPERTY(BlueprintReadOnly) int32 Count=10;
 UPROPERTY(BlueprintReadOnly) EFoundrylineSheetPlace Place=EFoundrylineSheetPlace::Floor;
 UPROPERTY(BlueprintReadOnly) int32 RackLevel=0;
 UPROPERTY(BlueprintReadOnly) FVector FloorPosition=FVector(-50,0,0);
 UPROPERTY(BlueprintReadOnly) float FloorYaw=0;
 double Pounds() const {return Count*FFoundrylineSteelSheet::PoundsEach();}
 bool Consume(int32 Quantity){if(Quantity<=0 || Quantity>Count)return false;Count-=Quantity;return true;}
};
