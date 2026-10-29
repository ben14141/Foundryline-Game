#pragma once
#include "CoreMinimal.h"
// Shared display conversion. Geometry, save records and Unreal units stay in cm.
struct FOUNDRYLINE_API FFoundrylineUnits {
 static FString Length(double Centimetres);
 static FString Display(const FString& Text);
};
