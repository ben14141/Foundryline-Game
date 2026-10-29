#include "Economy/FoundrylinePayroll.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Production/FoundrylineFactoryTypes.h"

int64 FFoundrylinePayroll::RateCentsPerHour(EFoundrylineEmployeeRole Role){
 using Tuning=FFoundrylineEconomyTuning;
 switch(Role){
  case EFoundrylineEmployeeRole::Welder:return Tuning::WelderWageCentsPerHour;
  case EFoundrylineEmployeeRole::Operator:return Tuning::OperatorWageCentsPerHour;
  case EFoundrylineEmployeeRole::ForkliftDriver:return Tuning::DriverWageCentsPerHour;
  default:return 0;
 }
}
int64 FFoundrylinePayroll::PlantCentsPerHour(const FFoundrylineFactoryState& State){
 int64 Cents=0;
 for(const auto& O:State.Operators)if(O.Machine)Cents+=RateCentsPerHour(EFoundrylineEmployeeRole::Operator);
 for(const auto& W:State.Walkers)if(W.Station||W.Zone)Cents+=RateCentsPerHour(EFoundrylineEmployeeRole::Welder);
 for(const auto& D:State.Drivers)if(D.EmployeeId)Cents+=RateCentsPerHour(EFoundrylineEmployeeRole::ForkliftDriver);
 return Cents;
}
int64 FFoundrylinePayroll::Accrue(FFoundrylineFactoryState& State,float Minutes){
 auto& C=State.Commerce;C.WageFraction+=Minutes*PlantCentsPerHour(State)/60.0;
 const int64 Due=int64(C.WageFraction);C.WageFraction-=Due;C.WagesPaidCents+=Due;return Due;
}
