#include "Economy/FoundrylinePayroll.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Production/FoundrylineFactoryTypes.h"

int64 FFoundrylinePayroll::RateCentsPerDay(EFoundrylineEmployeeRole Role){
 using T=FFoundrylineEconomyTuning;
 return Role==EFoundrylineEmployeeRole::Welder?T::WelderWageCentsPerDay:
  Role==EFoundrylineEmployeeRole::ForkliftDriver||Role==EFoundrylineEmployeeRole::PartsCollector?T::DriverWageCentsPerDay:T::OperatorWageCentsPerDay;
}
int64 FFoundrylinePayroll::PlantCentsPerDay(const FFoundrylineFactoryState& S){
 int64 Due=0;
 for(const auto& O:S.Operators)Due+=RateCentsPerDay(EFoundrylineEmployeeRole::Operator);
 for(const auto& W:S.Walkers)Due+=RateCentsPerDay(EFoundrylineEmployeeRole::Welder);
 for(const auto& D:S.Drivers)if(D.EmployeeId)Due+=RateCentsPerDay(EFoundrylineEmployeeRole::ForkliftDriver);
 for(const auto& C:S.Collectors)Due+=RateCentsPerDay(EFoundrylineEmployeeRole::PartsCollector);
 Due+=(S.UnassignedEmployees.Num()+S.OfficeEmployees.Num())*RateCentsPerDay(EFoundrylineEmployeeRole::Unassigned);
 return Due;
}
int64 FFoundrylinePayroll::Accrue(FFoundrylineFactoryState& S,float Minutes){
 if(!FMath::IsFinite(Minutes)||Minutes<=0)return 0;
 auto& C=S.Commerce;const double Rate=double(PlantCentsPerDay(S))/1440.;
 double At=C.Clock-Minutes,Left=Minutes;int64 Bill=0;
 while(Left>1.e-9){
  const double Boundary=(FMath::FloorToDouble(At/1440.)+1)*1440.;
  const double Part=FMath::Min(Left,Boundary-At);
  C.WageFraction+=Part*Rate;const int64 Earned=FMath::FloorToInt64(C.WageFraction+1.e-8);
  C.PayrollAccruedCents+=Earned;C.WageFraction=FMath::Max(0.,C.WageFraction-Earned);
  At+=Part;Left-=Part;
  if(At>=Boundary-1.e-8){Bill+=C.PayrollAccruedCents;C.PayrollAccruedCents=0;}
 }
 C.WagesPaidCents+=Bill;return Bill;
}
