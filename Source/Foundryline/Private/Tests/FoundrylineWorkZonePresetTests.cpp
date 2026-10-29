#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkZonePresets,"Foundryline.Factory.WorkZonePresetLayouts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorkZonePresets::RunTest(const FString&){
 const EFoundrylinePart Parts[]={EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings};
 for(int Preset=0;Preset<3;++Preset)for(int Q=0;Q<4;++Q)for(int Side:{-1,1}){
  FFoundrylineFactorySimulation M;M.Seed();M.State.Zones.Reset();M.State.Lanes.Reset();M.Solids.Reset();M.State.Walkers.Reset();M.State.Routes[0].Destinations.RemoveAll([](int V){return V>=2000;});M.State.FloorMin={-6000,-6000};M.State.FloorMax={6000,6000};FString E;
  TestTrue(TEXT("Preset places for every size, orientation and side"),M.AddPresetZone({0,0,20},Preset,Q,Side,1,E));if(M.State.Zones.IsEmpty())continue;
  const auto Z=M.State.Zones[0];const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(Z);auto Expected=FFoundrylineWorkZoneLayout::TotalSize(Preset);if(Q%2)Swap(Expected.X,Expected.Y);
  TestTrue(TEXT("Whole footprint is centered at cursor at physical scale"),Bounds.GetCenter().IsNearlyZero(.1)&&Bounds.GetSize().Equals(Expected,.1));
  TestTrue(TEXT("Zone supplies automatically join chosen route"),M.State.Routes[0].Destinations.Contains(Z.Id));
  const auto Equipment=FFoundrylineWorkZoneLayout::WelderBounds(Z);TestTrue(TEXT("Standard welding machine fits every preset, rotation and delivery side"),Bounds.IsInsideOrOn(Equipment.Min)&&Bounds.IsInsideOrOn(Equipment.Max));
  TArray<FBox2D> Bays;for(auto Part:Parts){const auto Center=M.BufferPosition(Z,Part);const auto Size=FFoundrylineWorkZoneLayout::BaySize(Z,Part);const FBox2D Bay(FVector2D(Center)-Size/2,FVector2D(Center)+Size/2);
   TestTrue(TEXT("Actual-size parts bays fit the internal supply strip"),FBox2D(FVector2D(Z.DropMin),FVector2D(Z.DropMax)).IsInsideOrOn(Bay.Min)&&FBox2D(FVector2D(Z.DropMin),FVector2D(Z.DropMax)).IsInsideOrOn(Bay.Max));
   TestTrue(TEXT("Part pallet footprint is inside its work zone"),FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)).IsInsideOrOn(Bay.Min)&&FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)).IsInsideOrOn(Bay.Max));for(const auto& Other:Bays)TestFalse(TEXT("No overlapping part bays"),Other.Intersect(Bay));Bays.Add(Bay);
   const FVector Local=FFoundrylineWorkZoneLayout::Rotation(Z).UnrotateVector(Center-FFoundrylineWorkZoneLayout::WorkCenter(Z));TestTrue(TEXT("Delivery side is relative to rotated product"),Local.Y*Side>FFoundrylineWorkZoneLayout::WorkSize(Preset).Y/2);
  }
  TestTrue(TEXT("Preset state validates"),M.Valid(E));const int Before=M.State.NextId,Count=M.State.Zones.Num();
  TestFalse(TEXT("Overlapping co-op placement rejected"),M.AddPresetZone({0,0,20},Preset,Q,Side,1,E));TestEqual(TEXT("Rejected placement preserves identity"),M.State.NextId,Before);TestEqual(TEXT("Rejected placement preserves zone count"),M.State.Zones.Num(),Count);
  auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TArray<uint8> Bytes;TestTrue(TEXT("Preset saves"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));TestNotNull(TEXT("Preset loads"),Loaded);if(Loaded){FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;TestTrue(TEXT("Restored geometry and ownership valid"),Restored.Valid(E));TestEqual(TEXT("Rotation saved"),Restored.State.Zones[0].QuarterTurns,Q);TestEqual(TEXT("Delivery side saved"),Restored.State.Zones[0].SupplySide,Side);}
  M.State.Zones[0].DropMax.X+=20;TestFalse(TEXT("Tampered preset geometry rejected"),M.Valid(E));
 }
 FFoundrylineFactorySimulation M;M.Seed();FString E;const int Before=M.State.NextId;
 TestFalse(TEXT("Out of bounds footprint rejected"),M.AddPresetZone({99999,0,20},0,0,1,1,E));TestFalse(TEXT("Unknown product size rejected"),M.AddPresetZone({0,0,20},3,0,1,1,E));TestFalse(TEXT("Invalid rotation rejected"),M.AddPresetZone({0,0,20},0,4,1,1,E));TestFalse(TEXT("Invalid side rejected"),M.AddPresetZone({0,0,20},0,0,0,1,E));TestFalse(TEXT("Unknown route rejected"),M.AddPresetZone({0,0,20},0,0,1,999,E));TestEqual(TEXT("Rejected requests do not allocate identities"),M.State.NextId,Before);
 M.State.Zones.Reset();M.State.Walkers.Reset();M.State.FloorMin={-6000,-6000};M.State.FloorMax={6000,6000};M.Solids={FBox2D({-100,-100},{100,100})};TestFalse(TEXT("Equipment obstruction rejects full preset"),M.AddPresetZone({0,0,20},0,0,1,1,E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotatedWorkZoneProduction,"Foundryline.Factory.RotatedPresetProduction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRotatedWorkZoneProduction::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed();auto& Z=M.State.Zones[0];FFoundrylineFactoryZone Layout;FFoundrylineWorkZoneLayout::Create(FVector(-2500,0,20),2,1,-1,Layout);Z.Min=Layout.Min;Z.Max=Layout.Max;Z.DropMin=Layout.DropMin;Z.DropMax=Layout.DropMax;Z.Preset=2;Z.QuarterTurns=1;Z.SupplySide=-1;
 M.State.FloorMin={-5000,-5000};M.State.FloorMax={5000,5000};M.State.Lanes.Reset();
 const auto Reserved=FFoundrylineWorkZoneLayout::Bounds(Z);
 // This production fixture offers free routing around, rather than through, its work zone.
 const FBox2D Bands[]={FBox2D(M.State.FloorMin,{Reserved.Min.X,M.State.FloorMax.Y}),FBox2D({Reserved.Max.X,M.State.FloorMin.Y},M.State.FloorMax),FBox2D({Reserved.Min.X,M.State.FloorMin.Y},{Reserved.Max.X,Reserved.Min.Y}),FBox2D({Reserved.Min.X,Reserved.Max.Y},{Reserved.Max.X,M.State.FloorMax.Y})};
 for(auto Kind:{EFoundrylineFloorLane::Forklift,EFoundrylineFloorLane::Pedestrian})for(const auto& Band:Bands){FFoundrylineFactoryLane L;L.Id=M.State.NextId++;L.Kind=Kind;L.Min=Band.Min;L.Max=Band.Max;M.State.Lanes.Add(L);}
 FString E;TestTrue(TEXT("Rotated preset validates before production"),M.Valid(E));const float Initial=M.SteelMass();M.Order(M.State.Zones[0].Id,E);M.State.Speed=8;
 for(int I=0;I<24000&&M.State.Zones[0].CompletedMask!=63;++I){
  if(M.Total(EFoundrylinePart::Sheet)<5&&M.Available(EFoundrylinePart::Sheet,EFoundrylinePalletPlace::Receiving,10)==0){M.ReceiveSteel(10,E);}
  M.Step(.1f);
 }
 if(M.State.Zones[0].CompletedMask!=63){for(const auto& D:M.State.Drivers)AddInfo(FString::Printf(TEXT("Driver %d job%d wait%d pos%s path%d distance%.1f"),D.Id,D.Job,D.Waiting,*D.Position.ToString(),D.Path.Num(),D.Distance));for(const auto& W:M.State.Walkers)AddInfo(FString::Printf(TEXT("Walker %d zone%d wait%d pos%s path%d distance%.1f"),W.Id,W.Zone,W.Waiting,*W.Position.ToString(),W.Path.Num(),W.Distance));for(const auto& O:M.State.Operators)AddInfo(FString::Printf(TEXT("Operator %d machine%d ready%d pos%s"),O.Id,O.Machine,M.MachineStaffReady(O.Machine),*O.Position.ToString()));for(const auto& A:M.State.Machines)AddInfo(FString::Printf(TEXT("Machine%d remaining%.2f making%d bed%.1f"),A.Id,A.Remaining,int(A.Making),A.BedLb));AddInfo(M.State.Status);}
 TestEqual(TEXT("Rotated product, workers and supply bays finish entire tanker"),M.State.Zones[0].CompletedMask,63);TestTrue(TEXT("Rotated production conserves material"),FMath::Abs(M.SteelMass()-Initial-M.State.ReceivedSteelLb)<2);TestTrue(TEXT("Rotated finished state validates"),M.Valid(E));return true;
}
#endif
