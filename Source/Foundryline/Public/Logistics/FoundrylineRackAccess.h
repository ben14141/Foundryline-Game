#pragma once
#include "Production/FoundrylineFactoryTypes.h"
struct FFoundrylineFactorySimulation;
struct FFoundrylinePlannerItem;
struct FOUNDRYLINE_API FFoundrylineRackAccess {
 static bool FinishedSlot(const FFoundrylineFactoryState&,int32 Slot);
 static bool Ready(const FFoundrylineFactoryState&,const FFoundrylineFactoryPallet&);
 static TArray<FVector> Docks(const FFoundrylineFactoryState&,int32 Slot);
 static bool Route(const FFoundrylineFactorySimulation&,FFoundrylineFactoryJob&,FVector From,TArray<FVector>& Path);
 static FBox2D Service(const FFoundrylinePlannerItem&,int Side);
 static bool Clear(const FFoundrylineFactorySimulation&,const FBox2D&,const FBox2D& Extra=FBox2D(ForceInit));
};
