#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
namespace WeldingAssemblyTest {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Role=EFoundrylineEmployeeRole;
// Same reorganised cut-parts starter as the Phase 1/2 tests, with the station north of the burn table.
bool AssemblyStarterStation(FFoundrylineFactorySimulation& M,FString& E){
 M.SeedCutPartsShop(true);M.State.Commerce.CashCents=1000000;
 return M.PaintLane(EFoundrylineFloorLane::Forklift,{-2217.6,1275,20},{-2217.6,2170,20},304.8f,0,E)&&M.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1455,1214.04,20},{-810,1214.04,20},121.92f,0,E)
  &&M.PaintLane(EFoundrylineFloorLane::Crossing,{-1370,1450,20},{-1370,1450,20},200,0,E)&&M.AddWeldingStation({-1768.02,1823.12,20},0,false,E);
}
// Two complete kits already unloaded into the station bins (Phase 2 delivers exactly these).
void FillBins(FFoundrylineFactorySimulation& M,const FFoundrylineWeldingStationRecord& S){
 for(auto [P,Q,Lb]:{TTuple<Part,int,float>{Part::MountingPlate,4,2},{Part::Bracket,8,5},{Part::FrameSection,12,16},{Part::LightKit,4,4.5f}}){
  FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=Q;A.Place=Place::Buffer;A.Owner=S.Id;A.Position=Layout::BinPoint(S,P);A.UnitLb=Lb;M.State.Pallets.Add(A);}
}
int KitBin(const FFoundrylineFactorySimulation& M,int Station,Part P){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==Place::Buffer&&A.Part==P)V+=A.Quantity;return V;}
int OnOut(const FFoundrylineFactorySimulation& M,int Station){int V=0;for(const auto& A:M.State.Pallets)if(A.Owner==Station&&A.Place==Place::StationOut)V+=A.Quantity;return V;}
const FFoundrylineWeldingStationRecord& Station(const FFoundrylineFactorySimulation& M){return M.State.WeldingStations[0];}
// Steps until the condition holds; false if it never does.
template<class F> bool RunUntil(FFoundrylineFactorySimulation& M,F Done,int Steps=20000){for(int I=0;I<Steps;++I){if(Done())return true;M.Step(.1f);}return Done();}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationAssembly,"Foundryline.Factory.WeldingStation.Assembly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationAssembly::RunTest(const FString&){using namespace WeldingAssemblyTest;
 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station places: ")+E,AssemblyStarterStation(M,E)))return false;
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
 TestTrue(TEXT("Exactly one kit left the bins"),KitBin(M,Id,Part::MountingPlate)==2&&KitBin(M,Id,Part::Bracket)==4&&KitBin(M,Id,Part::FrameSection)==6&&KitBin(M,Id,Part::LightKit)==2);
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
 TestTrue(TEXT("The stopped batch starts no new unit; the second kit stays in the bins"),KitBin(M,Id,Part::FrameSection)==6&&KitBin(M,Id,Part::LightKit)==2);
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
 {auto T=M;T.State.Version=11;T.State.WeldingStations[0].Reserved.Reset();for(auto& S:T.State.WeldingStations)S.Center-=Layout::Schema14Shift(S);T.MigrateWeldingStations();TestEqual(TEXT("Schema 11 migrates to 14"),T.State.Version,14);TestTrue(TEXT("Migrated schema 11 plant validates: ")+E,T.Valid(E));}
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationOutPallets,"Foundryline.Factory.WeldingStation.OutPallets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationOutPallets::RunTest(const FString&){using namespace WeldingAssemblyTest;
 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station places: ")+E,AssemblyStarterStation(M,E)))return false;
 const int Id=Station(M).Id;const float KitLb=2*2+4*5+6*16+2*4.5f;M.State.Running=true;M.State.Speed=8;
 auto Stack=[&](int Slot){return M.State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==Id&&A.Place==Place::StationOut&&Layout::OutSlot(Station(M),A.Position)==Slot;});};
 auto Count=[&](int Slot){return Layout::OutCount(Station(M),M.State.Pallets,Slot);};
 // Bins topped up to exactly two kits, and a batch no larger than the bins hold, so no supply run changes the count.
 auto Refill=[&]{M.State.Pallets.RemoveAll([&](const auto& A){return A.Owner==Id&&A.Place==Place::Buffer;});FillBins(M,Station(M));};
 TestTrue(TEXT("Headache racks x2: ")+E,M.ConfigureWeldingStation(Id,1,2,E));FillBins(M,Station(M));
 if(!TestTrue(TEXT("Hire a welder: ")+E,M.HireEmployee(Role::Welder,FGuid::NewGuid(),E)))return false;
 TestTrue(TEXT("Assign them to the station: ")+E,M.AssignEmployee(M.State.UnassignedEmployees.Last().Id,Role::Welder,Id,E));

 // The first rack starts the first pallet; later racks stack on it until it holds RacksPerOutPallet.
 if(!TestTrue(TEXT("First rack lands on OUT"),RunUntil(M,[&]{return Count(0)+Count(1)>0;})))return false;
 TestTrue(TEXT("First rack is on pallet 1, at its own position"),Count(0)==1&&!Count(1)&&Stack(0)&&Stack(0)->Position.Equals(Layout::OutPalletPoint(Station(M),0),.01));
 Stack(0)->Quantity=Layout::RacksPerOutPallet-1;
 if(!TestTrue(TEXT("Second rack finishes"),RunUntil(M,[&]{return Station(M).Stage==0&&Station(M).BatchRemaining==0&&Count(0)==Layout::RacksPerOutPallet;})))return false;
 TestTrue(TEXT("It stacks on pallet 1, which is now full, with nothing on pallet 2"),Count(0)==Layout::RacksPerOutPallet&&!Count(1)&&M.State.Pallets.FilterByPredicate([&](const auto& A){return A.Owner==Id&&A.Place==Place::StationOut;}).Num()==1);
 TestTrue(TEXT("Stack with five racks validates: ")+E,M.Valid(E));

 // A full first pallet sends the next rack to the second.
 Refill();TestTrue(TEXT("One more rack: ")+E,M.ConfigureWeldingStation(Id,1,1,E));
 if(!TestTrue(TEXT("Third rack lands"),RunUntil(M,[&]{return Station(M).Stage==0&&Count(1)==1;})))return false;
 TestTrue(TEXT("It starts pallet 2 at its own position; pallet 1 keeps five"),Count(0)==Layout::RacksPerOutPallet&&Stack(1)&&Stack(1)->Position.Equals(Layout::OutPalletPoint(Station(M),1),.01));

 // Both pallets full: ten racks at OUT and the station waits, with the kit left in the bins.
 Stack(1)->Quantity=Layout::RacksPerOutPallet;TestTrue(TEXT("Ten racks at OUT validate: ")+E,M.Valid(E));
 TestTrue(TEXT("One more rack: ")+E,M.ConfigureWeldingStation(Id,1,1,E));for(int I=0;I<300;++I)M.Step(.1f);
 TestEqual(TEXT("No unit starts while both OUT pallets are full"),Station(M).Stage,0);TestEqual(TEXT("The kit stays in the bins"),KitBin(M,Id,Part::FrameSection),6);
 TestTrue(TEXT("Status reports OUT full: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).Contains(TEXT("OUT full (10 on 2 pallets)")));

 // A collected pallet frees its position (an empty pallet takes its place), and the first position with room fills first.
 const int Collected=Stack(0)->Id;M.State.Pallets.RemoveAll([&](const auto& A){return A.Id==Collected;});
 if(!TestTrue(TEXT("Station resumes once a pallet is collected"),RunUntil(M,[&]{return Station(M).Stage==0&&Count(0)==1;})))return false;
 TestTrue(TEXT("The new rack starts the freed pallet 1; pallet 2 keeps five"),Count(0)==1&&Count(1)==Layout::RacksPerOutPallet);
 TestEqual(TEXT("Four units counted"),Station(M).UnitsCompleted,4);
 TestTrue(TEXT("Every rack on OUT carries a whole kit's mass"),Stack(0)&&Stack(1)&&FMath::IsNearlyEqual(Stack(0)->UnitLb,KitLb,.01f)&&FMath::IsNearlyEqual(Stack(1)->UnitLb,KitLb,.01f));
 TestTrue(TEXT("Station validates: ")+E,M.Valid(E));

 // Both stacks survive a save.
 {TArray<uint8> Bytes;auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TestTrue(TEXT("Save the stacks"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
  if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Restored.Solids=M.Solids;
   TestTrue(TEXT("Restored stacks validate: ")+E,Restored.Valid(E));TestTrue(TEXT("Restored stacks keep their pallets and heights"),Layout::OutCount(Restored.State.WeldingStations[0],Restored.State.Pallets,0)==1&&Layout::OutCount(Restored.State.WeldingStations[0],Restored.State.Pallets,1)==Layout::RacksPerOutPallet);}
  else AddError(TEXT("Stacks save did not load"));}

 // Hostile or corrupt OUT records are rejected.
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut&&A.Quantity==5)A.Quantity=6;TestFalse(TEXT("Six racks on one pallet rejected"),T.Valid(E));}
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut)A.Position=Layout::OutPalletPoint(T.State.WeldingStations[0],1);TestFalse(TEXT("Two stacks on one pallet rejected"),T.Valid(E));}
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut)A.Position=Layout::PortCenter(T.State.WeldingStations[0],true);TestFalse(TEXT("A stack off the pallet positions rejected"),T.Valid(E));}
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut)A.Reserved=A.Quantity+1;TestFalse(TEXT("More reserved than stacked rejected"),T.Valid(E));}
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut)A.Part=Part::Bracket;TestFalse(TEXT("Other stock on an OUT pallet rejected"),T.Valid(E));}

 // Schema 13 saves held one rack on a 4 ft OUT cradle in a 17 ft station: the station grows at its OUT end and the rack moves
 // onto the first OUT pallet, everything else staying where it was.
 {auto T=M;T.State.Pallets.RemoveAll([&](const auto& A){return A.Owner==Id&&A.Place==Place::StationOut&&Layout::OutSlot(T.State.WeldingStations[0],A.Position)==1;});
  const FVector Bench=Layout::BenchTransform(T.State.WeldingStations[0]).GetLocation(),InDock=Layout::Dock(T.State.WeldingStations[0],false);
  T.State.Version=13;auto& S=T.State.WeldingStations[0];S.Center-=Layout::Schema14Shift(S);for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut)A.Position=S.Center+FVector(200,0,0);
  T.MigrateWeldingStations();const auto& After=T.State.WeldingStations[0];
  TestEqual(TEXT("Schema 13 migrates to 14"),T.State.Version,14);TestTrue(TEXT("Migrated schema 13 plant validates: ")+E,T.Valid(E));
  TestTrue(TEXT("Migrated station keeps its bench and IN dock where they were"),Layout::BenchTransform(After).GetLocation().Equals(Bench,.01)&&Layout::Dock(After,false).Equals(InDock,.01));
  TestEqual(TEXT("The cradle rack is now on pallet 1"),Layout::OutCount(After,T.State.Pallets,0),1);}
 return true;
}
#endif
