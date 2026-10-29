#pragma once
#include "CoreMinimal.h"
#include "Companies/FoundrylineBuildRegion.h"

// Authored property geometry; ownership and setbacks persist through build-region saves.
struct FFoundrylinePropertyLot
{
 FName Name;
 FFoundrylineBuildRegion Region;
 static FFoundrylinePropertyLot Starter(FGuid Company,FGuid Plant)
 {
  FFoundrylinePropertyLot Lot;Lot.Name=TEXT("Foundry Lane / Lot 01");
  auto& R=Lot.Region;R.RegionId=FGuid(0xF001,3,1,1);R.CompanyId=Company;R.PlantId=Plant;R.SurfaceId=TEXT("StarterLand");
  R.Bounds.Min={-4000,-2400,0};R.Bounds.Max={6000,5600,5};
  R.bHasBuildingSetback=true;R.BuildingBounds.Min={-3700,-2100,0};R.BuildingBounds.Max={5700,5300,5};
  return Lot;
 }
};
