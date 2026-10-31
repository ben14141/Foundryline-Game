#pragma once
#include "CoreMinimal.h"
#include "Employees/FoundrylineEmployeeTypes.h"
struct FFoundrylineFactoryState;
// All hired employees accrue daily salary, including idle and unassigned workers.
// Game time pauses accrual. At midnight, the server settles the accrued bill once.
struct FOUNDRYLINE_API FFoundrylinePayroll {
 static int64 RateCentsPerDay(EFoundrylineEmployeeRole Role);
 static int64 PlantCentsPerDay(const FFoundrylineFactoryState& State);
 // Commerce.Clock has already advanced by Minutes. Returns only the midnight bill;
 // unpaid wages earned so far remain saved in PayrollAccruedCents and WageFraction.
 static int64 Accrue(FFoundrylineFactoryState& State,float Minutes);
};
