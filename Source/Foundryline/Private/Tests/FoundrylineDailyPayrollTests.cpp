#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Economy/FoundrylinePayroll.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDailyPayroll,"Foundryline.Commerce.DailyPayrollIdleUnassignedAndMidnight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDailyPayroll::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();auto& S=M.State;auto& C=S.Commerce;
 const int64 Initial=FFoundrylinePayroll::PlantCentsPerDay(S);
 TestEqual(TEXT("Two starter employees cost $240/day"),Initial,int64(24000));
 FString Error;TestTrue(TEXT("Hire a waiting employee"),M.HireEmployee(EFoundrylineEmployeeRole::Unassigned,FGuid::NewGuid(),Error));
 TestEqual(TEXT("Unassigned hire is paid too"),FFoundrylinePayroll::PlantCentsPerDay(S),int64(36000));
 // No machine work is needed to earn wages. The clock is advanced before accrual.
 C.Clock=1439;const int64 Cash=C.CashCents;C.Clock+=.5;
 TestEqual(TEXT("No cash bill before midnight"),FFoundrylinePayroll::Accrue(S,.5),int64(0));
 TestTrue(TEXT("Idle salary is owed before payday"),C.PayrollAccruedCents>0);TestEqual(TEXT("Accrual does not spend cash early"),C.CashCents,Cash);
 auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=S;TArray<uint8> Bytes;
 TestTrue(TEXT("Save mid-day accrued salary"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 const auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(!TestNotNull(TEXT("Read payroll save"),Loaded))return false;
 auto Restored=Loaded->Factory;TestEqual(TEXT("Owed salary survives save/load"),Restored.Commerce.PayrollAccruedCents,C.PayrollAccruedCents);
 Restored.Commerce.Clock+=.5;const int64 Bill=FFoundrylinePayroll::Accrue(Restored,.5);
 TestEqual(TEXT("Three employees earn 25 cents over a game minute"),Bill,int64(25));
 TestEqual(TEXT("Bill is counted once"),Restored.Commerce.WagesPaidCents,int64(25));
 TestEqual(TEXT("Settled accrual clears"),Restored.Commerce.PayrollAccruedCents,int64(0));
 Restored.Commerce.Clock+=.1;TestEqual(TEXT("No duplicate midnight bill"),FFoundrylinePayroll::Accrue(Restored,.1),int64(0));
 // Fractional slices must produce the same bill as a whole day.
 FFoundrylineFactoryState Day;Day.UnassignedEmployees.AddDefaulted();Day.Commerce.Clock=0;
 int64 Paid=0;for(int I=0;I<14400;++I){Day.Commerce.Clock=(I+1)*.1;Paid+=FFoundrylinePayroll::Accrue(Day,.1f);}
 TestEqual(TEXT("A full idle day costs exactly $120"),Paid,int64(12000));
 TestEqual(TEXT("No salary is lost to fractional cents"),Day.Commerce.PayrollAccruedCents,int64(0));
 // A fresh partial day does not charge retroactively for time before hiring.
 FFoundrylineFactoryState LateHire;LateHire.UnassignedEmployees.AddDefaulted();LateHire.Commerce.Clock=1440;
 TestEqual(TEXT("Hire in the last hour owes only $5"),FFoundrylinePayroll::Accrue(LateHire,60),int64(500));
 TestEqual(TEXT("Another company's accrual is untouched"),Day.Commerce.WagesPaidCents,int64(12000));
 const auto Pending=C.PayrollAccruedCents;S.Running=false;M.Step(.1f);
 TestEqual(TEXT("Pausing game time freezes salary accrual"),C.PayrollAccruedCents,Pending);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDailyPayrollDebt,"Foundryline.Commerce.DailyPayrollCashAndUnpaidBill",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDailyPayrollDebt::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();M.State.Running=true;auto& C=M.State.Commerce;
 C.Clock=1439.95;C.PayrollAccruedCents=10000;C.CashCents=5000;const auto Cost=C.CostCents;
 M.Step(.1f);
 TestEqual(TEXT("Payday spends available company cash"),C.CashCents,int64(0));
 TestTrue(TEXT("Unpaid salary becomes a bill rather than disappearing"),C.UnpaidCents>=5000);
 TestTrue(TEXT("Daily payroll appears in expenses"),C.CostCents>=Cost+10000);
 TestTrue(TEXT("Daily payroll has a clear journal label"),C.Money.ContainsByPredicate([](const auto& V){return V.Label==TEXT("Daily payroll");}));
 FString Error;TestTrue(TEXT("Payroll debt remains a valid company state: ")+Error,M.Valid(Error));
 return true;
}
#endif
