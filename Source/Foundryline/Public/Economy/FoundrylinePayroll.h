#pragma once
#include "CoreMinimal.h"
#include "Employees/FoundrylineEmployeeTypes.h"

struct FFoundrylineFactoryState;

// Payroll: wages per game hour for everyone holding a job in one plant, paid by that plant's company. Rates come from
// FFoundrylineEconomyTuning. A hire without a job is not paid.
struct FOUNDRYLINE_API FFoundrylinePayroll {
 static int64 RateCentsPerHour(EFoundrylineEmployeeRole Role);
 static int64 PlantCentsPerHour(const FFoundrylineFactoryState& State);
 // Accrues Minutes of wages into the plant's commerce state and returns the whole cents now due (also added to the
 // plant's running wages total). The caller books them as a "Wages" expense.
 static int64 Accrue(FFoundrylineFactoryState& State,float Minutes);
};
