#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmployeePassing,"Foundryline.Employees.HeadOnPaintedCorridorPassing",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEmployeePassing::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.State.FloorMin={0,0};M.State.FloorMax={2000,400};M.State.Running=true;
 FFoundrylineFactoryLane L;L.Id=2001;L.Kind=EFoundrylineFloorLane::Pedestrian;L.Min={200,125};L.Max={1800,275};M.State.Lanes={L};
 for(int I=0;I<2;++I){FFoundrylineFactoryMachine Machine;Machine.Id=I+1;Machine.Origin=I?FVector(620,-100,20):FVector(874,-1,20);Machine.Enabled=true;Machine.Auto=false;Machine.Remaining=Machine.Duration=1000;M.State.Machines.Add(Machine);FFoundrylineFactoryOperator O;O.Id=2101+I;O.Machine=I+1;O.Position=O.Home=I?FVector(1200,200,20):FVector(800,200,20);M.State.Operators.Add(O);}
 bool OffsetSeen=false;double Minimum=10000;
 for(int Tick=0;Tick<400;++Tick){const FVector A=M.State.Operators[0].Position,B=M.State.Operators[1].Position;M.UpdateOperators(.05);const auto& X=M.State.Operators[0];const auto& Y=M.State.Operators[1];TestTrue(TEXT("Passing moves normally, never teleports"),FVector::Dist2D(A,X.Position)<=5.51&&FVector::Dist2D(B,Y.Position)<=5.51);Minimum=FMath::Min(Minimum,FVector::Dist2D(X.Position,Y.Position));OffsetSeen|=FMath::Abs(X.Position.Y-200)>10||FMath::Abs(Y.Position.Y-200)>10;TestTrue(TEXT("People remain inside painted footprint including 35cm clearance"),X.Position.Y>=160&&X.Position.Y<=240&&Y.Position.Y>=160&&Y.Position.Y<=240);}
 TestTrue(TEXT("Both opposite jobs reach their actual equipment work points"),M.MachineStaffReady(1)&&M.MachineStaffReady(2));TestTrue(TEXT("Actual simulation used lateral passing tracks"),OffsetSeen);TestTrue(TEXT("Actual moving bodies maintain separation"),Minimum>=51.99);
 auto Route=TArray<FVector>{FVector(970,200,20),FVector(1400,200,20)};float Distance=0;M.State.Operators[0].Position={970,200,20};M.State.Operators[0].Path=Route;M.State.Operators[0].Distance=0;M.State.Operators[1].Position={1029,200,20};M.State.Operators[1].Path={FVector(1029,200,20),FVector(600,200,20)};M.State.Operators[1].Distance=0;
 M.State.Lanes[0].Min.Y=140;M.State.Lanes[0].Max.Y=260;TestFalse(TEXT("Narrow 120cm spurs do not pretend two people fit"),M.TryEmployeePassing({970,200,20},{981,200,20},2101,Route,Distance));
 M.State.Lanes[0]=L;FFoundrylineFactoryDriver Fork;Fork.Id=2200;Fork.Position={1100,300,20};M.State.Drivers.Add(Fork);TestFalse(TEXT("Passing cannot bypass a nearby forklift"),M.TryEmployeePassing({970,200,20},{981,200,20},2101,Route,Distance));M.State.Drivers.Reset();
 M.Solids.Add(FBox2D({1000,225},{1100,250}));TestFalse(TEXT("Passing rejects a physical obstacle in the side track"),M.TryEmployeePassing({970,200,20},{981,200,20},2101,Route,Distance));return true;
}
#endif
