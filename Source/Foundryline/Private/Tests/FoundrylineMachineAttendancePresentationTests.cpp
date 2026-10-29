#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineMachineMotion.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMachineAttendancePresentation,"Foundryline.Production.MachineAttendancePresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMachineAttendancePresentation::RunTest(const FString&){
 FFoundrylineFactorySimulation Model;Model.Seed(true);auto& S=Model.State;auto& M=S.Machines[0];S.Running=true;M.Enabled=true;M.Duration=8;M.Remaining=4;
 auto* O=S.Operators.FindByPredicate([&](const auto& P){return P.Machine==M.Id;});if(!TestNotNull(TEXT("Actual machine has seeded staff"),O))return false;
 O->Position=O->WorkPoint;TestTrue(TEXT("Physically attended real active part animates"),FFoundrylineMachineMotion::Active(S,M));
 const auto Before=M.Remaining;const double Mass=Model.SteelMass();O->Position+=FVector(200,0,0);O->Target=O->Position;O->Path.Reset();O->Distance=0;
 TestFalse(TEXT("In-progress machine cannot pretend to operate without attendance"),FFoundrylineMachineMotion::Active(S,M));
 TestTrue(TEXT("Missing attendance holds exact saved phase despite snapshot age"),FMath::IsNearlyEqual(FFoundrylineMachineMotion::Phase(M.Remaining,M.Duration,FFoundrylineMachineMotion::Active(S,M),8,.5),.5f,.0001f));
 Model.Step(.1f);TestEqual(TEXT("Unattended authoritative part also remains unchanged"),M.Remaining,Before);TestTrue(TEXT("Presentation and missing attendance consume no extra mass"),FMath::IsNearlyEqual(Model.SteelMass(),Mass,.001));
 O->Position=O->WorkPoint;S.Running=false;TestFalse(TEXT("Global pause freezes an attended machine"),FFoundrylineMachineMotion::Active(S,M));S.Running=true;M.Enabled=false;TestFalse(TEXT("Machine pause freezes an attended machine"),FFoundrylineMachineMotion::Active(S,M));
 M.Enabled=true;TestTrue(TEXT("Restored physical attendance resumes same existing cycle"),FFoundrylineMachineMotion::Active(S,M));TestTrue(TEXT("Same cycle smoothly advances from saved remaining time"),FMath::IsNearlyEqual(FFoundrylineMachineMotion::Phase(M.Remaining,M.Duration,true,1,.1),.5125f,.0001f));
 M.Remaining=0;TestFalse(TEXT("Idle machine has no pretend cycle"),FFoundrylineMachineMotion::Active(S,M));return true;
}
#endif
