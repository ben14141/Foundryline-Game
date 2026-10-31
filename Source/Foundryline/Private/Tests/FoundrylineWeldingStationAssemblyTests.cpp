#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FoundrylineWeldingStationTestHelpers.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
namespace WeldingAssemblyTest {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Role=EFoundrylineEmployeeRole;
// The shared reorganised starter with the station placed.
bool AssemblyStarterStation(FFoundrylineFactorySimulation& M,FString& E){return FoundrylineWeldingTestHelpers::StarterStation(M,E);}
// Two complete kits already unloaded into the station stock (Phase 2 delivers exactly these).
void FillBins(FFoundrylineFactorySimulation& M,const FFoundrylineWeldingStationRecord& S){FoundrylineWeldingTestHelpers::KitStock(M,S,false);}
int KitBin(const FFoundrylineFactorySimulation& M,int Station,Part P){return FoundrylineWeldingTestHelpers::KitStockOf(M,Station,P);}
int OnOut(const FFoundrylineFactorySimulation& M,int Station){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==Place::StationOut)V+=A.Quantity;return V;}
using FoundrylineWeldingTestHelpers::LeaveOut;
const FFoundrylineWeldingStationRecord& Station(const FFoundrylineFactorySimulation& M){return M.State.WeldingStations[0];}
using FoundrylineWeldingTestHelpers::RunUntil;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationAssembly,"Foundryline.Factory.WeldingStation.Assembly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationAssembly::RunTest(const FString&){using namespace WeldingAssemblyTest;
 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station places: ")+E,AssemblyStarterStation(M,E)))return false;LeaveOut(M);
 const int Id=Station(M).Id;TestTrue(TEXT("Headache racks x2: ")+E,M.ConfigureWeldingStation(Id,1,2,E));FillBins(M,Station(M));
 FFoundrylineAssemblyRecipe R;FFoundrylineFactoryCatalog::Recipe(1,R);const float KitLb=2*2+4*5+6*16+2*4.5f;
 M.State.Running=true;M.State.Speed=8;const float Mass=M.SteelMass();
 for(int I=0;I<200;++I)M.Step(.1f);TestEqual(TEXT("No unit starts without a welder"),Station(M).Stage,0);
 TestTrue(TEXT("Status asks for a welder: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).Contains(TEXT("assign a welder")));

 // Hire a welder and give them the station from the office computer's assignment command.
 if(!TestTrue(TEXT("Hire a welder: ")+E,M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)))return false;
 const int Welder=M.State.UnassignedEmployees.Last().Id;
 TestTrue(TEXT("Assign them to the station: ")+E,M.AssignEmployee(Welder,Role::Welder,Id,E));
 {auto T=M;TestFalse(TEXT("A second welder cannot take the same station (one welder in Phase 3)"),T.HireEmployee(Role::Welder,FGuid::NewGuid(),E)&&T.AssignEmployee(T.State.UnassignedEmployees.Last().Id,Role::Welder,Id,E));}
 const auto* W=M.State.Walkers.FindByPredicate([&](const auto& V){return V.Id==Welder;});
 TestTrue(TEXT("Welder is assigned to the station, not a zone"),W&&W->Station==Id&&!W->Zone);
 TestTrue(TEXT("Station with an assigned welder validates: ")+E,M.Valid(E));

 // They walk to Welder A's work point; the kit is reserved only then, exactly once.
 if(!TestTrue(TEXT("Welder reaches the work point and a unit starts"),RunUntil(M,[&]{return Station(M).Stage>0;})))return false;
 TestTrue(TEXT("Welder stands at Welder A's work point"),M.StationWelderReady(Station(M)));
 TestTrue(TEXT("Exactly one kit left the IN stock"),KitBin(M,Id,Part::MountingPlate)==2&&KitBin(M,Id,Part::BentBracket)==4&&KitBin(M,Id,Part::FrameSection)==6&&KitBin(M,Id,Part::LightKit)==2);
 TestTrue(TEXT("The unit holds the recipe's whole kit"),Station(M).Reserved==R.Bill&&FMath::IsNearlyEqual(Station(M).ReservedLb,KitLb,.01f)&&Station(M).UnitRecipe==1);
 TestEqual(TEXT("One unit of the batch is in work"),Station(M).BatchRemaining,1);
 TestTrue(TEXT("Material mass is conserved at kit reservation"),FMath::Abs(M.SteelMass()-Mass)<.01f);
 TestTrue(TEXT("Fit-up status: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).StartsWith(TEXT("Fit-up")));

 // Welding at the one-welder (100%) rate: 90 s of plant time.
 if(!TestTrue(TEXT("Fit-up completes into welding"),RunUntil(M,[&]{return Station(M).Stage==2;})))return false;
 const double Start=M.State.Commerce.Clock;RunUntil(M,[&]{return Station(M).Work>=45;});TestTrue(TEXT("Weld progress advances while the welder is at work"),Station(M).bAdvancing);
 TestTrue(TEXT("Half the weld takes about 45 s of plant time at 100%"),FMath::Abs(M.State.Commerce.Clock-Start-45)<1.5);

 // Pause keeps progress exactly.
 const float Before=Station(M).Work;M.State.Running=false;for(int I=0;I<200;++I)M.Step(.1f);TestEqual(TEXT("Paused plant keeps the weld progress"),Station(M).Work,Before);M.State.Running=true;

 // Save mid-weld; the restored plant finishes the same unit.
 TArray<uint8> Bytes;{auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TestTrue(TEXT("Save mid-assembly"),UGameplayStatics::SaveGameToMemory(Save,Bytes));}

 // The welder leaves: progress stays, nothing advances, the kit stays reserved.
 TestTrue(TEXT("Release the welder mid-weld: ")+E,M.AssignEmployee(Welder,Role::Welder,0,E));
 const float Left=Station(M).Work;for(int I=0;I<300;++I)M.Step(.1f);
 TestTrue(TEXT("No progress without the welder"),Station(M).Work==Left&&Station(M).Stage==2&&!Station(M).bAdvancing);
 TestTrue(TEXT("Status says the unit is paused: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).Contains(TEXT("paused")));
 TestTrue(TEXT("Released welder validates: ")+E,M.Valid(E));
 RunUntil(M,[&]{const auto* P=M.State.UnassignedEmployees.FindByPredicate([&](const auto& V){return V.Id==Welder;});const auto* V=M.State.Walkers.FindByPredicate([&](const auto& X){return X.Id==Welder;});return P&&P->Position.Equals(P->Home,1)||V&&V->Position.Equals(V->Home,1);});
 TestTrue(TEXT("Reassign the same welder: ")+E,M.AssignEmployee(Welder,Role::Welder,Id,E));
 TestTrue(TEXT("Welding resumes from the kept progress"),RunUntil(M,[&]{return Station(M).Work>Left+1||Station(M).Stage>2;}));

 // Stopping the product mid-unit finishes the unit first and starts no other.
 TestTrue(TEXT("Stop the batch mid-unit: ")+E,M.ConfigureWeldingStation(Id,0,0,E));TestEqual(TEXT("The unit stays on the bench"),Station(M).Stage,2);
 TestFalse(TEXT("A station with a unit on the bench cannot be removed"),M.RemoveWeldingStation(Id,E));
 if(!TestTrue(TEXT("Final assembly installs the lights"),RunUntil(M,[&]{return Station(M).Stage==3;})))return false;
 if(!TestTrue(TEXT("Finished rack moves to OUT"),RunUntil(M,[&]{return Station(M).Stage==4;})))return false;
 int MaxOut=0;RunUntil(M,[&]{MaxOut=FMath::Max(MaxOut,OnOut(M,Id));return Station(M).Stage==0;});
 for(int I=0;I<400;++I){M.Step(.1f);MaxOut=FMath::Max(MaxOut,OnOut(M,Id));}
 const auto* Out=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==Id&&A.Place==Place::StationOut;});
 TestTrue(TEXT("Exactly one headache rack on OUT, never two"),Out&&Out->Part==Part::HeadacheRack&&Out->Quantity==1&&MaxOut==1);
 TestTrue(TEXT("The first rack goes on the first OUT pallet"),Out&&Layout::OutSlot(Station(M),Out->Position)==0);
 TestTrue(TEXT("The rack carries the whole kit's mass"),Out&&FMath::IsNearlyEqual(Out->UnitLb,KitLb,.01f));
 TestTrue(TEXT("Bench is clear and the unit counted once"),Station(M).Stage==0&&Station(M).UnitsCompleted==1&&Station(M).ReservedLb==0&&!Station(M).UnitRecipe);
 TestTrue(TEXT("The stopped batch starts no new unit; the second kit stays in the IN stock"),KitBin(M,Id,Part::FrameSection)==6&&KitBin(M,Id,Part::LightKit)==2);
 TestTrue(TEXT("Material mass is conserved through to the finished rack"),FMath::Abs(M.SteelMass()-Mass)<.01f);
 TestTrue(TEXT("Station with a rack on OUT validates: ")+E,M.Valid(E));

 // OUT has room for more: a new rack starts from the second kit with the first one still waiting on OUT.
 TestTrue(TEXT("Status counts the finished rack on OUT: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).Contains(TEXT("1 finished on OUT")));
 TestTrue(TEXT("Start another rack: ")+E,M.ConfigureWeldingStation(Id,1,1,E));
 TestTrue(TEXT("A unit starts while OUT still holds the first rack"),RunUntil(M,[&]{return Station(M).Stage>0;},600));
 TestTrue(TEXT("Stop after it: ")+E,M.ConfigureWeldingStation(Id,0,0,E));TestTrue(TEXT("It lands beside the first, on pallet 1"),RunUntil(M,[&]{return Station(M).Stage==0;})&&Layout::OutCount(Station(M),M.State.Pallets,0)==2);

 // The mid-weld save restores and completes the same unit exactly once.
 if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){
  FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Restored.Solids=M.Solids;TestTrue(TEXT("Mid-assembly save validates: ")+E,Restored.Valid(E));
  TestTrue(TEXT("Restored unit keeps its stage, progress and kit"),Restored.State.WeldingStations[0].Stage==2&&Restored.State.WeldingStations[0].Work==Before&&Restored.State.WeldingStations[0].Reserved==R.Bill);
  FString StopError;TestTrue(TEXT("Stop the restored batch after this unit: ")+StopError,Restored.ConfigureWeldingStation(Id,0,0,StopError));
  RunUntil(Restored,[&]{return OnOut(Restored,Id)>0;});for(int I=0;I<400;++I)Restored.Step(.1f);
  TestEqual(TEXT("Restored plant finishes the rack exactly once"),OnOut(Restored,Id),1);TestEqual(TEXT("Restored plant counts one completed unit"),Restored.State.WeldingStations[0].UnitsCompleted,1);
 }else AddError(TEXT("Mid-assembly save did not load"));

 // Hostile or corrupt records are rejected.
 {auto T=M;T.State.WeldingStations[0].Stage=5;TestFalse(TEXT("Unknown stage rejected"),T.Valid(E));}
 {auto T=M;T.State.WeldingStations[0].Stage=2;T.State.WeldingStations[0].UnitRecipe=1;T.State.WeldingStations[0].Reserved[int(Part::FrameSection)]=1;TestFalse(TEXT("Unit without its whole kit rejected"),T.Valid(E));}
 {auto T=M;T.State.WeldingStations[0].ReservedLb=50;TestFalse(TEXT("Empty bench holding mass rejected"),T.Valid(E));}
 {auto T=M;if(auto* V=T.State.Walkers.FindByPredicate([&](const auto& X){return X.Id==Welder;})){V->Station=999999;TestFalse(TEXT("Welder at an unknown station rejected"),T.Valid(E));}}
 // Schema 11 plants load with an empty bench.
 {auto T=M;T.State.Version=11;T.State.WeldingStations[0].Reserved.Reset();for(auto& S:T.State.WeldingStations)S.Center-=Layout::Schema14Shift(S);T.MigrateWeldingStations();TestEqual(TEXT("Schema 11 migrates to 14"),T.State.Version,15);TestTrue(TEXT("Migrated schema 11 plant validates: ")+E,T.Valid(E));}
 return true;
}
#endif
