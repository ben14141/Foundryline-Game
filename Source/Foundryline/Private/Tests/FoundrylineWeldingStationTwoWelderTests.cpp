#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FoundrylineWeldingStationTestHelpers.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
namespace WeldingTwoWelderTest {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Role=EFoundrylineEmployeeRole;
// The shared reorganised starter with the station placed and two complete kits in its stock.
bool TwoWelderStation(FFoundrylineFactorySimulation& M,FString& E){
 if(!FoundrylineWeldingTestHelpers::StarterStation(M,E))return false;FoundrylineWeldingTestHelpers::KitStock(M,M.State.WeldingStations[0],false);return true;
}
int Hire(FFoundrylineFactorySimulation& M,FString& E){return M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)?M.State.UnassignedEmployees.Last().Id:0;}
using FoundrylineWeldingTestHelpers::LeaveOut;
int OutCount(const FFoundrylineFactorySimulation& M,int Station){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==Place::StationOut)V+=A.Quantity;return V;}
template<class F> bool StepUntil(FFoundrylineFactorySimulation& M,F Done,int Steps=20000){for(int I=0;I<Steps;++I){if(Done())return true;M.Step(.1f);}return Done();}
// Work done per second of plant time over a short stretch of welding.
float MeasuredRate(FFoundrylineFactorySimulation& M){const float W0=M.State.WeldingStations[0].Work;const double C0=M.State.Commerce.Clock;for(int I=0;I<20;++I)M.Step(.1f);return (M.State.WeldingStations[0].Work-W0)/FMath::Max(.001,M.State.Commerce.Clock-C0);}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationTwoWelders,"Foundryline.Factory.WeldingStation.TwoWelders",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationTwoWelders::RunTest(const FString&){using namespace WeldingTwoWelderTest;
 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station with two kits: ")+E,TwoWelderStation(M,E)))return false;LeaveOut(M);
 const int Id=M.State.WeldingStations[0].Id;TestTrue(TEXT("Headache racks x2: ")+E,M.ConfigureWeldingStation(Id,1,2,E));
 FFoundrylineAssemblyRecipe R;FFoundrylineFactoryCatalog::Recipe(1,R);TestEqual(TEXT("The two-welder rate is recipe balance data: 170%"),R.TwoWelderRate,1.7f);
 const int A=Hire(M,E),B=Hire(M,E),Late=Hire(M,E);if(!TestTrue(TEXT("Hire three welders: ")+E,A&&B&&Late))return false;

 // Exclusive slots: one person per slot, one slot per person, first command wins.
 TestTrue(TEXT("Welder A slot: ")+E,M.AssignEmployee(A,Role::Welder,Id,E,0));
 TestFalse(TEXT("The same person cannot also take Welder B"),M.AssignEmployee(A,Role::Welder,Id,E,1));
 TestTrue(TEXT("Welder B slot: ")+E,M.AssignEmployee(B,Role::Welder,Id,E,1));
 TestFalse(TEXT("A second claim on slot B loses the race"),M.AssignEmployee(Late,Role::Welder,Id,E,1));TestTrue(TEXT("...and says the slot is taken: ")+E,E.Contains(TEXT("Welder B is already taken")));
 TestFalse(TEXT("Slot A is taken too"),M.AssignEmployee(Late,Role::Welder,Id,E,0));
 TestFalse(TEXT("Only welding stations have a slot B"),M.AssignEmployee(Late,Role::Operator,1,E,1));
 TestFalse(TEXT("Unknown slot refused"),M.AssignEmployee(Late,Role::Welder,Id,E,2));
 TestTrue(TEXT("Two-welder station validates: ")+E,M.Valid(E));
 {auto T=M;if(auto* W=T.State.Walkers.FindByPredicate([&](const auto& V){return V.Id==B;}))W->StationSide=0;TestFalse(TEXT("Two welders in one slot rejected"),T.Valid(E));}
 {auto T=M;if(auto* W=T.State.Walkers.FindByPredicate([&](const auto& V){return V.Id==B;}))W->StationSide=2;TestFalse(TEXT("Unknown slot in a save rejected"),T.Valid(E));}

 // Both walk to their own work points; the shared unit then runs at 170%.
 M.State.Running=true;M.State.Speed=8;const float Mass=M.SteelMass();
 if(!TestTrue(TEXT("Both welders reach their work points and welding starts"),StepUntil(M,[&]{return M.State.WeldingStations[0].Stage==2&&M.StationWelders(M.State.WeldingStations[0],true)==2;})))return false;
 const auto* WA=M.StationWelder(M.State.WeldingStations[0],0);const auto* WB=M.StationWelder(M.State.WeldingStations[0],1);
 TestTrue(TEXT("Each welder stands at their own slot's work point"),WA&&WB&&WA->Position.Equals(Layout::WorkPoint(M.State.WeldingStations[0],0),1)&&WB->Position.Equals(Layout::WorkPoint(M.State.WeldingStations[0],1),1));
 TestEqual(TEXT("Station rate with both at work"),M.StationRate(M.State.WeldingStations[0]),R.TwoWelderRate);
 TestTrue(TEXT("Weld progress advances at about 170% of plant time"),FMath::IsNearlyEqual(MeasuredRate(M),R.TwoWelderRate,.05f));
 TestTrue(TEXT("Status notes both welders: ")+M.WeldingStationStatus(M.State.WeldingStations[0]),M.WeldingStationStatus(M.State.WeldingStations[0]).Contains(TEXT("2 welders")));
 TArray<uint8> Bytes;{auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TestTrue(TEXT("Save with both welders mid-weld"),UGameplayStatics::SaveGameToMemory(Save,Bytes));}

 // Welder B leaves mid-unit: the progress stays and the rate drops to the remaining welder's 100%.
 const float Before=M.State.WeldingStations[0].Work;TestTrue(TEXT("Release Welder B mid-weld: ")+E,M.AssignEmployee(B,Role::Welder,0,E));
 TestEqual(TEXT("Releasing a welder keeps the progress"),M.State.WeldingStations[0].Work,Before);
 TestTrue(TEXT("Remaining welder carries on at 100%"),FMath::IsNearlyEqual(MeasuredRate(M),1.f,.05f)&&M.State.WeldingStations[0].Work>Before&&M.State.WeldingStations[0].Stage==2);
 TestEqual(TEXT("Rate now 100%"),M.StationRate(M.State.WeldingStations[0]),1.f);TestEqual(TEXT("Rate with both still offered"),M.StationRate(M.State.WeldingStations[0],2),R.TwoWelderRate);
 TestTrue(TEXT("Released welder validates: ")+E,M.Valid(E));

 // The unit finishes once and lands on the first OUT pallet; the batch was stopped, so no second unit starts.
 TestTrue(TEXT("Stop after this unit: ")+E,M.ConfigureWeldingStation(Id,0,0,E));
 int MaxOut=0;StepUntil(M,[&]{MaxOut=FMath::Max(MaxOut,OutCount(M,Id));return OutCount(M,Id)>0&&M.State.WeldingStations[0].Stage==0;});for(int I=0;I<300;++I){M.Step(.1f);MaxOut=FMath::Max(MaxOut,OutCount(M,Id));}
 TestTrue(TEXT("Exactly one rack on OUT, once"),OutCount(M,Id)==1&&MaxOut==1&&M.State.WeldingStations[0].UnitsCompleted==1);
 TestTrue(TEXT("Material mass is conserved with two welders"),FMath::Abs(M.SteelMass()-Mass)<.01f);

 // The two-welder save restores both slots and the same progress, and finishes the rack once.
 if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){
  FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Restored.Solids=M.Solids;TestTrue(TEXT("Two-welder save validates: ")+E,Restored.Valid(E));
  const auto& S=Restored.State.WeldingStations[0];TestTrue(TEXT("Both slots and the weld progress restore"),Restored.StationWelder(S,0)&&Restored.StationWelder(S,0)->Id==A&&Restored.StationWelder(S,1)&&Restored.StationWelder(S,1)->Id==B&&S.Stage==2&&S.Work>0);
  TestEqual(TEXT("Restored station welds at 170%"),Restored.StationRate(S),R.TwoWelderRate);
  FString StopError;TestTrue(TEXT("Stop the restored batch after this unit: ")+StopError,Restored.ConfigureWeldingStation(Id,0,0,StopError));
  StepUntil(Restored,[&]{return OutCount(Restored,Id)>0;});for(int I=0;I<300;++I)Restored.Step(.1f);TestEqual(TEXT("Restored plant finishes the rack exactly once"),OutCount(Restored,Id),1);
 }else AddError(TEXT("Two-welder save did not load"));

 // Schema 12 plants load with every station welder as Welder A.
 {auto T=M;T.State.Version=12;for(auto& S:T.State.WeldingStations)S.Center-=FFoundrylineWeldingStationLayout::Schema14Shift(S);T.MigrateWeldingStations();TestEqual(TEXT("Schema 12 migrates to 14"),T.State.Version,14);TestTrue(TEXT("Migrated schema 12 plant validates: ")+E,T.Valid(E));}
 return true;
}
#endif
