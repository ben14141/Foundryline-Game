#pragma once
#include "Production/FoundrylineFactoryTypes.h"

// Product-space coordinates keep physical product, supplies and access aligned when rotated.
struct FOUNDRYLINE_API FFoundrylineWorkZoneLayout {
 static const TCHAR* PresetName(int32 Preset);
 static FVector2D WorkSize(int32 Preset);
 static FVector2D TotalSize(int32 Preset);
 static bool Create(FVector Center,int32 Preset,int32 QuarterTurns,int32 Side,FFoundrylineFactoryZone& Out);
 static FRotator Rotation(const FFoundrylineFactoryZone& Zone){return FRotator(0,Zone.QuarterTurns*90,0);}
 static FVector Point(const FFoundrylineFactoryZone& Zone,FVector Local);
 static FVector WorkCenter(const FFoundrylineFactoryZone& Zone);
 static FVector ProductCenter(const FFoundrylineFactoryZone& Zone);
 static FVector WelderPosition(const FFoundrylineFactoryZone& Zone);
 static FBox2D WelderBounds(const FFoundrylineFactoryZone& Zone);
 static FVector WeldPoint(const FFoundrylineFactoryZone& Zone);
 static FBox2D Bounds(const FFoundrylineFactoryZone& Zone);
 static FVector Buffer(const FFoundrylineFactoryZone& Zone,EFoundrylinePart Part);
 static FVector2D BaySize(const FFoundrylineFactoryZone& Zone,EFoundrylinePart Part);
 static FVector WorkPoint(const FFoundrylineFactoryZone& Zone);
};
