#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
namespace {
 FFoundrylineFactorySimulation RoutingFloor(FVector2D Min={-1000,-1000},FVector2D Max={1000,1000}){FFoundrylineFactorySimulation M;M.State.FloorMin={-2000,-2000};M.State.FloorMax={2000,2000};FFoundrylineFactoryLane L;L.Id=1;L.Kind=EFoundrylineFloorLane::Forklift;L.Min=Min;L.Max=Max;M.State.Lanes.Add(L);return M;}
 bool Diagonal(const TArray<FVector>& P){for(int I=1;I<P.Num();++I){const auto D=P[I]-P[I-1];if(FMath::Abs(D.X)>1&&FMath::Abs(D.Y)>1)return true;}return false;}
 bool CoversFootprint(const FFoundrylineFactorySimulation& M,const TArray<FVector>& P){for(int I=1;I<P.Num();++I){const int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist(P[I-1],P[I])/5));for(int S=0;S<=Steps;++S)for(int X=-1;X<=1;++X)for(int Y=-1;Y<=1;++Y){const FVector2D V(FMath::Lerp(P[I-1],P[I],double(S)/Steps)+FVector(X*79,Y*79,0));if(!M.State.Lanes.ContainsByPredicate([&](const auto& L){return L.Kind==EFoundrylineFloorLane::Forklift&&FBox2D(L.Min,L.Max).IsInsideOrOn(V);}))return false;}}return true;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkliftDiagonalRouting,"Foundryline.Factory.ForkliftDiagonalAndRoundedRouting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkliftDiagonalRouting::RunTest(const FString&){
 auto M=RoutingFloor();auto P=M.Path({-700,-700,20},{700,700,20});TestTrue(TEXT("Open painted floor gets a shorter diagonal rather than grid stairs"),P.Num()>1&&Diagonal(P)&&M.Length(P)<2100);TestTrue(TEXT("Exact pickup and destination positions are preserved"),P.Num()>1&&P[0].Equals({-700,-700,20},.001)&&P.Last().Equals({700,700,20},.001));
 M=RoutingFloor({-1000,-200},{200,200});auto V=M.State.Lanes[0];V.Id=2;V.Min={-200,-200};V.Max={200,1000};M.State.Lanes.Add(V);P=M.Path({-800,0,20},{0,800,20});
 TestTrue(TEXT("Elbow uses a safe shorter corner cut"),P.Num()>1&&Diagonal(P)&&M.Length(P)<1560);TestTrue(TEXT("Corner cut keeps the routing footprint in connected forklift pavement"),CoversFootprint(M,P));
 bool Rounded=false;for(const auto& Point:P)if(FMath::Abs(Point.X/50-FMath::RoundToDouble(Point.X/50))>.001||FMath::Abs(Point.Y/50-FMath::RoundToDouble(Point.Y/50))>.001)Rounded=true;TestTrue(TEXT("Clear turns contain a rounded arc rather than only sharp grid vertices"),Rounded);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkliftShortcutSafety,"Foundryline.Factory.ForkliftShortcutClearanceAndDirection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkliftShortcutSafety::RunTest(const FString&){
 auto M=RoutingFloor();M.Solids.Add(FBox2D({-50,-300},{50,300}));auto P=M.Path({-800,0,20},{800,0,20});TestTrue(TEXT("Forklift routes around machinery"),P.Num()>1);const auto B=M.Solids[0].ExpandBy(79);for(int I=1;I<P.Num();++I){const FVector A(P[I-1].X,P[I-1].Y,0),C(P[I].X,P[I].Y,0);TestFalse(TEXT("No shortcut clips the expanded machine corner"),FMath::LineBoxIntersection(FBox(FVector(B.Min,-1),FVector(B.Max,1)),A,C,C-A));}
 M=RoutingFloor({-1000,-200},{-5,200});auto Gap=M.State.Lanes[0];Gap.Id=2;Gap.Min={5,-200};Gap.Max={1000,200};M.State.Lanes.Add(Gap);TestTrue(TEXT("Diagonal routing cannot jump a narrow unpainted gap"),M.Path({-800,0,20},{800,0,20}).IsEmpty());
 M=RoutingFloor({-1000,-300},{1000,300});M.State.Lanes[0].Direction=1;TestTrue(TEXT("One-way route works forward"),M.Path({-700,0,20},{700,0,20}).Num()>1);TestTrue(TEXT("Corner shortcuts cannot reverse a one-way route"),M.Path({700,0,20},{-700,0,20}).IsEmpty());TestTrue(TEXT("Short docking connectors cannot bypass one-way rules"),M.Path({110,0,20},{90,0,20}).IsEmpty());
 M=RoutingFloor({-1000,-500},{1000,500});FFoundrylineFactoryDriver Other;Other.Id=2;Other.Position={0,0,20};Other.Parking=Other.Position;M.State.Drivers.Add(Other);P=M.Path({-800,0,20},{800,0,20});TestTrue(TEXT("Route can pass a stationary forklift where space allows"),P.Num()>1);for(int I=1;I<P.Num();++I)TestTrue(TEXT("Smoothed route retains vehicle separation"),FMath::PointDistToSegment(Other.Position,P[I-1],P[I])>=199.9);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForkliftYieldContinuity,"Foundryline.Factory.ForkliftYieldDoesNotTeleport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FForkliftYieldContinuity::RunTest(const FString&){
 auto M=RoutingFloor({-1000,-500},{1000,500});M.State.Running=true;FFoundrylineFactoryDriver D;D.Id=2;D.EmployeeId=D.Id;D.DisplayName=FString::Printf(TEXT("Test Driver %d"),D.Id);D.Position={-200,0,20};D.Parking={800,0,20};D.Path={D.Position,D.Parking};M.State.Drivers.Add(D);D.Id=3;D.EmployeeId=D.Id;D.DisplayName=FString::Printf(TEXT("Test Driver %d"),D.Id);D.Position=D.Parking={0,0,20};D.Path.Reset();M.State.Drivers.Add(D);
 for(int I=0;I<100;++I){const auto Before=M.State.Drivers[0].Position;M.Step(.1f);TestTrue(TEXT("Yielding and replanning never jump a vehicle sideways"),FVector::Dist2D(Before,M.State.Drivers[0].Position)<=15.1);}
 TestTrue(TEXT("Yield reroute makes progress around a parked truck"),FVector::Dist2D(M.State.Drivers[0].Position,FVector(-200,0,20))>50);return true;
}
#endif
