#pragma once
#include "Production/FoundrylineWorkshopLayout.h"

// A machine cell includes its equipment, assigned racks and output pickup pad.
struct FFoundrylineMachineCellLayout {
 static FBox2D Bounds(int32 Id){
  FBox2D Box=FFoundrylineWorkshopLayout::Solids()[Id-1].ExpandBy(80);
  for(FVector Center:{FFoundrylineWorkshopLayout::InputRack(Id),FFoundrylineWorkshopLayout::OutputRack(Id)}){Box+=FVector2D(Center)-FVector2D(75,220);Box+=FVector2D(Center)+FVector2D(75,220);}
  const FVector2D Output(FFoundrylineWorkshopLayout::OutputStock(Id));Box+=Output-FVector2D(100,180);Box+=Output+FVector2D(100,180);return Box.ExpandBy(20);
 }
};
