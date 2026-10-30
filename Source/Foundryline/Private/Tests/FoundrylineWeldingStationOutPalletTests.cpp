#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FoundrylineWeldingStationTestHelpers.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
// The station's two OUT pallets: which pallet a finished rack lands on, the 10-rack limit, collection freeing a pallet,
// saves, hostile records and the schema 13 migration from the 17 ft cradle station.
namespace WeldingOutPalletTest {
using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;using Role=EFoundrylineEmployeeRole;
using namespace FoundrylineWeldingTestHelpers;
const FFoundrylineWeldingStationRecord& Station(const FFoundrylineFactorySimulation& M){return M.State.WeldingStations[0];}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationOutPallets,"Foundryline.Factory.WeldingStation.OutPallets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationOutPallets::RunTest(const FString&){using namespace WeldingOutPalletTest;
 FFoundrylineFactorySimulation M;FString E;if(!TestTrue(TEXT("Starter station places: ")+E,StarterStation(M,E)))return false;LeaveOut(M);
 const int Id=Station(M).Id;M.State.Running=true;M.State.Speed=8;
 auto Stack=[&](int Slot){return M.State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==Id&&A.Place==Place::StationOut&&Layout::OutSlot(Station(M),A.Position)==Slot;});};
 auto Count=[&](int Slot){return Layout::OutCount(Station(M),M.State.Pallets,Slot);};
 // IN stock topped up to exactly two kits, and a batch no larger than it holds, so no supply run changes the count.
 auto Refill=[&]{M.State.Pallets.RemoveAll([&](const auto& A){return A.Owner==Id&&A.Place==Place::Buffer;});KitStock(M,Station(M),false);};
 TestTrue(TEXT("Headache racks x2: ")+E,M.ConfigureWeldingStation(Id,1,2,E));KitStock(M,Station(M),false);
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

 // Both pallets full: ten racks at OUT and the station waits, with the kit left in the IN stock.
 Stack(1)->Quantity=Layout::RacksPerOutPallet;TestTrue(TEXT("Ten racks at OUT validate: ")+E,M.Valid(E));
 TestTrue(TEXT("One more rack: ")+E,M.ConfigureWeldingStation(Id,1,1,E));for(int I=0;I<300;++I)M.Step(.1f);
 TestEqual(TEXT("No unit starts while both OUT pallets are full"),Station(M).Stage,0);TestEqual(TEXT("The kit stays in the IN stock"),KitStockOf(M,Id,Part::FrameSection),6);
 TestTrue(TEXT("Status reports OUT full: ")+M.WeldingStationStatus(Station(M)),M.WeldingStationStatus(Station(M)).Contains(TEXT("OUT full (8 on 2 pallets)")));

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
 {auto T=M;for(auto& A:T.State.Pallets)if(A.Owner==Id&&A.Place==Place::StationOut&&A.Quantity==Layout::RacksPerOutPallet)A.Quantity=Layout::RacksPerOutPallet+1;TestFalse(TEXT("More than four racks on one new pallet rejected"),T.Valid(E));}
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
  TestEqual(TEXT("Schema 13 migrates to 14"),T.State.Version,15);TestTrue(TEXT("Migrated schema 13 plant validates: ")+E,T.Valid(E));
  TestTrue(TEXT("Migrated station keeps its bench and IN dock where they were"),Layout::BenchTransform(After).GetLocation().Equals(Bench,.01)&&Layout::Dock(After,false).Equals(InDock,.01));
  TestEqual(TEXT("The cradle rack is now on pallet 1"),Layout::OutCount(After,T.State.Pallets,0),1);}
 return true;
}
#endif
