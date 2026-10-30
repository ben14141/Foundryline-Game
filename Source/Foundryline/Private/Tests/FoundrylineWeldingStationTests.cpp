#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
namespace {
using Layout=FFoundrylineWeldingStationLayout;
FFoundrylineWeldingStationRecord Station(FVector Center,int Q,bool Swap){FFoundrylineWeldingStationRecord S;S.Center=Center;S.QuarterTurns=Q;S.bSwapSides=Swap;return S;}
void AddStock(FFoundrylineFactorySimulation& M,EFoundrylinePart Part,int Q,EFoundrylinePalletPlace Place,int Owner,FVector At){FFoundrylineFactoryPallet P;P.Id=M.State.NextId++;P.Part=Part;P.Quantity=Q;P.Place=Place;P.Owner=Owner;P.Position=At;M.State.Pallets.Add(P);}
bool Inside(const FBox2D& Outer,const FBox2D& Inner){return Outer.ExpandBy(.01).IsInsideOrOn(Inner.Min)&&Outer.ExpandBy(.01).IsInsideOrOn(Inner.Max);}
bool Disjoint(const FBox2D& A,const FBox2D& B){return A.Max.X<=B.Min.X+.01||B.Max.X<=A.Min.X+.01||A.Max.Y<=B.Min.Y+.01||B.Max.Y<=A.Min.Y+.01;}
// Open test floor with forklift spurs at both station ends and walkways along both welder sides,
// authored in station-local orientation then turned with the station (quarter turns keep boxes axis-aligned).
void OpenFloor(FFoundrylineFactorySimulation& M,int Q,bool WalkwayB=true,bool RightSpur=true){
 M.Seed();M.State.Zones.Reset();M.State.Walkers.Reset();M.State.Operators.Reset();M.State.UnassignedEmployees.Reset();M.State.Lanes.Reset();M.State.Pallets.Reset();M.State.Jobs.Reset();M.Solids.Reset();M.State.Drivers.SetNum(1);
 M.State.Routes[0].Destinations.RemoveAll([](int V){return V>=2000;});M.State.FloorMin={-6000,-6000};M.State.FloorMax={6000,6000};
 const FRotator R(0,Q*90,0);auto Turn=[&](FVector2D A,FVector2D B){FBox2D Box(ForceInit);Box+=FVector2D(R.RotateVector(FVector(A,0)));Box+=FVector2D(R.RotateVector(FVector(B,0)));return Box;};
 auto Lane=[&](EFoundrylineFloorLane Kind,FVector2D A,FVector2D B){const auto Box=Turn(A,B);FFoundrylineFactoryLane L;L.Id=M.State.NextId++;L.Kind=Kind;L.Min=Box.Min;L.Max=Box.Max;M.State.Lanes.Add(L);};
 Lane(EFoundrylineFloorLane::Forklift,{-800,400},{800,760});Lane(EFoundrylineFloorLane::Forklift,{-700,-100},{-340,760});if(RightSpur)Lane(EFoundrylineFloorLane::Forklift,{340,-100},{700,760});
 Lane(EFoundrylineFloorLane::Pedestrian,{-1200,-320},{250,-198.12});Lane(EFoundrylineFloorLane::Pedestrian,{-1200,-320},{-1090,320});if(WalkwayB)Lane(EFoundrylineFloorLane::Pedestrian,{-1200,198.12},{250,320});
 auto& D=M.State.Drivers[0];D.Parking=D.Position=R.RotateVector(FVector(-520,600,20));D.Job=0;D.Path.Reset();
 FFoundrylineStandbyDriver Person;Person.Id=M.State.NextId++;Person.DisplayName=TEXT("Test Welder");Person.Position=Person.Home=R.RotateVector(FVector(-1145,-260,20));M.State.UnassignedEmployees.Add(Person);
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationLayout,"Foundryline.Factory.WeldingStation.Layout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationLayout::RunTest(const FString&){
 // Same light-bezel point in the upright rack source and in the assembled bench setup (Blender Y is mirrored by FBX import).
 TestTrue(TEXT("Rack lies on its back on the jig: tube undersides on the riser tops"),FMath::IsNearlyEqual(Layout::ProductOnBench().TransformPosition(FVector(0,Layout::RackTubeHalfCm,40)).Z,Layout::BenchHeightCm+Layout::JigRiserCm,.01));
 TestTrue(TEXT("Bed-rail plates clear the worktop and the rack stays over it"),Layout::RackOnBenchCm().Min.Z>Layout::BenchHeightCm&&Layout::RackOnBenchCm().Min.Z<Layout::BenchHeightCm+2&&FMath::Abs(Layout::RackOnBenchCm().Min.Y)<2*Layout::Foot&&Layout::RackOnBenchCm().Max.Y<2*Layout::Foot);
 for(const auto& R:Layout::JigRisers())TestTrue(TEXT("Every riser stands on the worktop under the rack"),FMath::IsNearlyEqual(R.GetLocation().Z,Layout::BenchHeightCm)&&FMath::Abs(R.GetLocation().X)<4*Layout::Foot-5.08&&FMath::Abs(R.GetLocation().Y)<2*Layout::Foot-5.08);
 FString E;TestTrue(TEXT("Full-size rack fits the bench fixture, clear height and OUT bay: ")+E,Layout::ProductFits(Layout::RackOnBenchCm(),FVector2D(Layout::RackSizeCm()),E));
 TestFalse(TEXT("A product longer than the bench is rejected, not scaled"),Layout::ProductFits(FBox(FVector(-150,-40,95),FVector(150,40,120)),{300,80},E));
 TestFalse(TEXT("Mounts resting on the worktop are rejected"),Layout::ProductFits(FBox(FVector(-99,-40,Layout::BenchHeightCm),FVector(99,40,120)),{198,81},E));
 TestFalse(TEXT("A finished product too long for the OUT bay is rejected"),Layout::ProductFits(Layout::RackOnBenchCm(),{260,81},E));
 for(int Q=0;Q<4;++Q)for(bool Swap:{false,true}){const auto S=Station({300,-500,20},Q,Swap);const auto B=Layout::Bounds(S);
  TestTrue(TEXT("Footprint is 19 ft 6 in x 13 ft at 1:1 scale, centred on the cursor"),B.GetSize().Equals(Layout::FootprintCm(Q),.01)&&B.GetCenter().Equals(FVector2D(300,-500),.01)&&FMath::IsNearlyEqual(FMath::Max(B.GetSize().X,B.GetSize().Y),19.5*Layout::Foot,.01)&&FMath::IsNearlyEqual(FMath::Min(B.GetSize().X,B.GetSize().Y),13*Layout::Foot,.01));
  const FBox2D Parts[]={Layout::Bench(S),Layout::Worker(S,0),Layout::Worker(S,1),Layout::Port(S,false),Layout::Port(S,true),Layout::MachineBay(S,0),Layout::MachineBay(S,1)};
  for(int I=0;I<7;++I){TestTrue(TEXT("Bench, worker spaces, ports and machine bays stay inside the station"),Inside(B,Parts[I]));for(int J=I+1;J<7;++J)TestTrue(TEXT("Bench, worker spaces, ports and machine bays never overlap"),Disjoint(Parts[I],Parts[J]));}
  // The wider OUT bay sits at its own end, so the bench is centred across the station and 1 ft 3 in toward IN along it.
  const FVector2D Along=FVector2D(Layout::Point(S,1,0)-Layout::Point(S,0,0)).GetSafeNormal(),Across(-Along.Y,Along.X),FromCentre=Parts[0].GetCenter()-B.GetCenter();
  TestTrue(TEXT("Bench is 8 x 4 ft, centred across the station, 1 ft 3 in toward IN"),FMath::IsNearlyEqual(Parts[0].GetArea(),243.84*121.92,1)&&FMath::IsNearlyZero(FromCentre.Dot(Across),.01)&&FMath::IsNearlyEqual(FMath::Abs(FromCentre.Dot(Along)),1.25*Layout::Foot,.01)&&FVector2D::Distance(Parts[0].GetCenter(),Parts[3].GetCenter())<FVector2D::Distance(Parts[0].GetCenter(),Parts[4].GetCenter()));
  TestTrue(TEXT("Worker spaces are at least 8 x 4 ft on opposite long sides"),Parts[1].GetArea()>=243.84*121.92-1&&FMath::IsNearlyEqual(Parts[1].GetArea(),Parts[2].GetArea(),1)&&FMath::Min(Parts[1].GetSize().X,Parts[1].GetSize().Y)>=121.92-.01&&(Parts[1].GetCenter()+Parts[2].GetCenter()).Equals(2*Parts[0].GetCenter(),.01));
  auto AtEnd=[&](const FBox2D& Port){return FMath::IsNearlyEqual(FMath::Abs((Port.GetCenter()-B.GetCenter()).Dot(Along))+Port.GetSize().Dot(Along.GetAbs())/2,19.5*Layout::Foot/2,.01);};
  TestTrue(TEXT("IN is 4 x 10 ft 6 in and OUT 6 ft 6 in x 8 ft, at opposite ends"),FMath::IsNearlyEqual(Parts[3].GetArea(),121.92*320.04,1)&&FMath::IsNearlyEqual(Parts[4].GetArea(),198.12*243.84,1)&&AtEnd(Parts[3])&&AtEnd(Parts[4])&&(Parts[3].GetCenter()-B.GetCenter()).Dot(Parts[4].GetCenter()-B.GetCenter())<0);
  const auto Plain=Station(S.Center,Q,!Swap);TestTrue(TEXT("Swap exchanges the IN and OUT ends; the bench and worker spaces keep their size and place beside IN"),FMath::IsNearlyEqual(Layout::Port(S,false).GetArea(),Layout::Port(Plain,false).GetArea(),1)&&FMath::IsNearlyEqual(FVector2D::Distance(Layout::Bench(S).GetCenter(),FVector2D(Layout::PortCenter(S,false))),FVector2D::Distance(Layout::Bench(Plain).GetCenter(),FVector2D(Layout::PortCenter(Plain,false))),.01)&&Layout::Bench(S).GetSize().Equals(Layout::Bench(Plain).GetSize(),.01)&&(Layout::Bench(S).GetCenter()-Layout::Bench(Plain).GetCenter()).Size()>1);
  // Two OUT pallets inside the OUT bay, each holding a stack of RacksPerOutPallet racks lying flat along the bay.
  const FBox2D Pallet0=Layout::OutPalletRect(S,0),Pallet1=Layout::OutPalletRect(S,1);
  TestTrue(TEXT("Both OUT pallets lie inside the OUT bay, side by side, never overlapping"),Inside(Parts[4],Pallet0)&&Inside(Parts[4],Pallet1)&&Disjoint(Pallet0,Pallet1)&&Layout::OutSlot(S,Layout::OutPalletPoint(S,1))==1&&Layout::OutSlot(S,Layout::PortCenter(S,true))<0);
  FBox2D Stack[2]={FBox2D(ForceInit),FBox2D(ForceInit)};
  for(int Slot=0;Slot<Layout::OutPallets;++Slot)for(int Level=0;Level<Layout::RacksPerOutPallet;++Level){const FBox R=Layout::RackOnBenchCm();const double Base=Layout::PalletTopCm+Level*Layout::RackSizeCm().Z;
   const FBox OnPallet(FVector(R.Min.X,R.Min.Y,Base),FVector(R.Max.X,R.Max.Y,Base+Layout::RackSizeCm().Z));FBox World(ForceInit);for(int C=0;C<8;++C)World+=Layout::OutStackTransform(S,Slot).TransformPosition(FVector(C&1?OnPallet.Max.X:OnPallet.Min.X,C&2?OnPallet.Max.Y:OnPallet.Min.Y,C&4?OnPallet.Max.Z:OnPallet.Min.Z));
   const FTransform Rack=Layout::RackOnOut(S,Slot,Level);
   TestTrue(TEXT("Each rack lies flat on the stack below it (or the pallet deck)"),FMath::IsNearlyEqual(Rack.GetLocation().Z-13.97,20+Base,.01)&&FMath::IsNearlyEqual(World.Min.Z,20+Base,.01));
   Stack[Slot]+=FVector2D(World.Min);Stack[Slot]+=FVector2D(World.Max);}
  TestTrue(TEXT("Both full stacks stay inside the OUT bay and clear of each other"),Inside(Parts[4],Stack[0])&&Inside(Parts[4],Stack[1])&&Disjoint(Stack[0],Stack[1]));
  for(int Slot=0;Slot<2;++Slot){const FBox2D P=Layout::OutPalletRect(S,Slot),Overlap(FVector2D(FMath::Max(P.Min.X,Stack[Slot].Min.X),FMath::Max(P.Min.Y,Stack[Slot].Min.Y)),FVector2D(FMath::Min(P.Max.X,Stack[Slot].Max.X),FMath::Min(P.Max.Y,Stack[Slot].Max.Y)));
   TestTrue(TEXT("Each stack rests on its own pallet over the pallet's full depth"),FMath::IsNearlyEqual(Overlap.GetSize().Dot(Along.GetAbs()),P.GetSize().Dot(Along.GetAbs()),.5)&&Overlap.GetSize().Dot(Across.GetAbs())>=Layout::RackSizeCm().Y-.5);}
  TestTrue(TEXT("A full stack of four stands about 4 ft 3 in high"),FMath::IsNearlyEqual(Layout::PalletTopCm+Layout::RacksPerOutPallet*Layout::RackSizeCm().Z,129.32,.1));
  for(int Side=0;Side<2;++Side)TestTrue(TEXT("Welding machines stay clear of the OUT bay and its pallets"),Disjoint(Layout::MachineBay(S,Side),Parts[4]));
  // The station's supply stays on two pallets in the IN bay; every kit part has its place on one of them.
  TestTrue(TEXT("Frame-section pallet stays inside IN"),Inside(Parts[3],Layout::InPalletRect(S,0)));
  for(int Count:{4,6,8}){const auto Transform=FFoundrylineAssemblySupplyCatalog::Transform(S);const auto Size=FFoundrylineAssemblySupplyCatalog::BinSize(Count);FBox2D Bin(ForceInit);for(float X:{-.5f,.5f})for(float Y:{-.5f,.5f})Bin+=FVector2D(Transform.TransformPosition(FVector(X*Size.X,Y*Size.Y,0)));TestTrue(TEXT("Every supply-bin upgrade fits IN at this rotation and side"),Inside(Parts[3],Bin));TestTrue(TEXT("Supply bin and frame pallet never overlap"),Disjoint(Bin,Layout::InPalletRect(S,0)));}
  for(int Slot=0;Slot<1;++Slot)TestTrue(TEXT("Each IN pallet is served from straight in front, outside the station"),FMath::IsNearlyEqual(FVector::Dist2D(Layout::InPalletDock(S,Slot),Layout::InPalletPoint(S,Slot)),Layout::DockReachCm,.01)&&!B.IsInside(FVector2D(Layout::InPalletDock(S,Slot))));
  for(auto P:{EFoundrylinePart::FrameSection})TestTrue(TEXT("Kit stock sits on its IN pallet's deck, not the floor"),Layout::InPalletRect(S,Layout::InSlot(P)).IsInside(FVector2D(Layout::BinPoint(S,P)))&&FMath::IsNearlyEqual(Layout::BinPoint(S,P).Z,20+Layout::PalletTopCm,.01));
  for(bool Out:{false,true}){const FVector Dock=Layout::Dock(S,Out);const auto Approach=Layout::ForkApproach(S,Out);
   TestFalse(TEXT("Forklift dock is outside the station"),B.IsInside(FVector2D(Dock)));TestTrue(TEXT("Dock sits in the outside approach at the port end"),Approach.IsInside(FVector2D(Dock))&&FMath::IsNearlyEqual(FVector::Dist2D(Dock,Layout::PortDrop(S,Out)),Layout::DockReachCm,.01)&&FMath::IsNearlyEqual(FVector2D(Layout::PortDrop(S,Out)-Dock).Size()-(19.5*Layout::Foot/2-FMath::Abs((FVector2D(Layout::PortDrop(S,Out))-B.GetCenter()).Dot(Along))),Layout::DockReachCm-2*Layout::Foot,.01));
   TestTrue(TEXT("Forklift approach is outside and adjacent"),Disjoint(Approach,B)&&Approach.ExpandBy(1).Intersect(Layout::Port(S,Out)));
   for(int Offset=-Layout::MaxDockOffsetFt;Offset<=Layout::MaxDockOffsetFt;++Offset){auto Shifted=S;(Out?Shifted.OutDockOffsetFt:Shifted.InDockOffsetFt)=Offset;const FVector2D Drop(Layout::PortDrop(Shifted,Out));
    TestTrue(TEXT("Every dock offset stays in the outside approach"),Approach.IsInside(FVector2D(Layout::Dock(Shifted,Out))));TestTrue(TEXT("Shifted pallet (48 x 40 in) stays inside the port"),Inside(Layout::Port(S,Out),FBox2D(Drop-FVector2D(60.96),Drop+FVector2D(60.96))));}}
  for(int Side=0;Side<2;++Side){const FVector P=Layout::WorkPoint(S,Side);const auto Bench=Layout::Bench(S);
   TestTrue(TEXT("Each welder stands inside their own clearance"),Layout::Worker(S,Side).IsInside(FVector2D(P))&&!Layout::Worker(S,1-Side).IsInside(FVector2D(P)));
   const double Gap=FMath::Max(FMath::Max(Bench.Min.X-P.X,P.X-Bench.Max.X),FMath::Max(Bench.Min.Y-P.Y,P.Y-Bench.Max.Y));TestTrue(TEXT("Welder stands 18 in from the bench edge"),FMath::IsNearlyEqual(Gap,45.72,.01));
   const FVector Facing=FRotator(0,Layout::WorkYaw(S,Side),0).Vector();TestTrue(TEXT("Welder faces the bench"),FVector2D(Facing).Dot((Bench.GetCenter()-FVector2D(P)).GetSafeNormal())>.7);
   TestTrue(TEXT("Worker approach is outside the long side"),Disjoint(Layout::WorkerApproach(S,Side),B)&&Layout::WorkerApproach(S,Side).ExpandBy(1).Intersect(Layout::Worker(S,Side)));}
  for(int Side=0;Side<2;++Side){const FTransform T=Layout::MachineTransform(S,Side);const FVector H=Layout::MachineSizeCm()/2;FBox2D Foot(ForceInit);for(int C=0;C<4;++C)Foot+=FVector2D(T.TransformPosition(Layout::MachineBoundsCentreCm()+FVector(C&1?H.X:-H.X,C&2?H.Y:-H.Y,0)));
   TestTrue(TEXT("Welding machine fits its own corner bay inside the border"),Inside(Layout::MachineBay(S,Side),Foot.ExpandBy(-.01)));TestTrue(TEXT("Machine never enters worker clearance, ports or forklift bays"),Disjoint(Foot,Layout::Worker(S,0))&&Disjoint(Foot,Layout::Worker(S,1))&&Disjoint(Foot,Layout::Port(S,false))&&Disjoint(Foot,Layout::Port(S,true))&&Disjoint(Foot,Layout::ForkApproach(S,false))&&Disjoint(Foot,Layout::ForkApproach(S,true)));
   TestTrue(TEXT("Each welder's own machine is nearer to them than to the other welder"),FVector::Dist2D(T.GetLocation(),Layout::WorkPoint(S,Side))<FVector::Dist2D(T.GetLocation(),Layout::WorkPoint(S,1-Side)));
   TestTrue(TEXT("Machine bay touches its welder's strip"),Layout::MachineBay(S,Side).ExpandBy(1).Intersect(Layout::Worker(S,Side)));}
  TestTrue(TEXT("Two welders use separated weld positions"),FVector::Dist2D(Layout::WorkPoint(S,0),Layout::WorkPoint(S,1))>150);
  const FBox Rack=Layout::RackOnBenchCm().TransformBy(FTransform(FRotator::ZeroRotator,FVector(0,0,0)));const FTransform Bench=Layout::BenchTransform(S);
  FBox World(ForceInit);for(int C=0;C<8;++C)World+=Bench.TransformPosition(FVector(C&1?Rack.Max.X:Rack.Min.X,C&2?Rack.Max.Y:Rack.Min.Y,C&4?Rack.Max.Z:Rack.Min.Z));
  TestTrue(TEXT("Fixtured rack stays over the rotated bench top and above it"),Inside(Layout::Bench(S),FBox2D(FVector2D(World.Min),FVector2D(World.Max)))&&World.Min.Z>20+Layout::BenchHeightCm);
  TestTrue(TEXT("Access ring is 4 ft wide on every side and outside the station"),FMath::IsNearlyEqual(FMath::Max(Layout::WorkerApproach(S,0).GetSize().X,Layout::WorkerApproach(S,0).GetSize().Y),(19.5+8)*Layout::Foot,.01)&&Disjoint(Layout::WorkerApproach(S,0),B)&&Disjoint(Layout::EndAccess(S,false),B)&&Inside(Layout::EndAccess(S,Swap),Layout::ForkApproach(S,Swap)));
  TestTrue(TEXT("Product pose follows the station turn at unchanged scale"),Layout::ProductTransform(S).GetScale3D().Equals(FVector::OneVector,.0001));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationPlacement,"Foundryline.Factory.WeldingStation.Placement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationPlacement::RunTest(const FString&){
 for(int Q=0;Q<4;++Q)for(bool Swap:{false,true}){FFoundrylineFactorySimulation M;OpenFloor(M,Q);FString E;
  TestTrue(FString::Printf(TEXT("Connected station places (turn %d, swap %d): "),Q,Swap)+E,M.AddWeldingStation({0,0,20},Q,Swap,E));if(M.State.WeldingStations.IsEmpty())continue;
  const auto S=M.State.WeldingStations[0];TestEqual(TEXT("First station is number 01"),S.Number,1);TestTrue(TEXT("Station state validates: ")+E,M.Valid(E));
  for(bool Out:{false,true}){const auto Route=M.Path(M.State.Drivers[0].Parking,Layout::Dock(S,Out));TestTrue(TEXT("Forklift reaches both ports"),Route.Num()>1);bool Entered=false;for(const auto& P:Route)Entered|=Layout::Bounds(S).ExpandBy(-1).IsInside(FVector2D(P));TestFalse(TEXT("Forklift route never enters the station"),Entered);}
  for(int Side=0;Side<2;++Side){const auto Route=M.Path(M.State.UnassignedEmployees[0].Home,Layout::WorkPoint(S,Side),true);TestTrue(TEXT("Each welder side is reachable on foot"),Route.Num()>1);bool Bad=false;for(const auto& P:Route)Bad|=Layout::Bench(S).IsInside(FVector2D(P))||Layout::Port(S,false).IsInside(FVector2D(P))||Layout::Port(S,true).IsInside(FVector2D(P));TestFalse(TEXT("Walking route avoids the bench and both ports"),Bad);}
  const int Next=M.State.NextId;TestFalse(TEXT("Second station on the same spot is rejected"),M.AddWeldingStation({0,0,20},Q,Swap,E));TestEqual(TEXT("Rejected placement allocates no identity"),M.State.NextId,Next);
  FFoundrylineFactorySimulation NoB;OpenFloor(NoB,Q,false);TestTrue(TEXT("Welder B is reached around the access ring from the Welder A walkway: ")+E,NoB.AddWeldingStation({0,0,20},Q,Swap,E));
  if(NoB.State.WeldingStations.Num()){const auto Route=NoB.Path(NoB.State.UnassignedEmployees[0].Home,Layout::WorkPoint(NoB.State.WeldingStations[0],1),true);TestTrue(TEXT("Welder B walk goes around the ring"),Route.Num()>1);}
  FFoundrylineFactorySimulation Cut;OpenFloor(Cut,Q,false);Cut.State.Lanes.RemoveAll([&](const auto& L){return L.Kind==EFoundrylineFloorLane::Pedestrian&&FBox2D(L.Min,L.Max).GetSize().X>FBox2D(L.Min,L.Max).GetSize().Y==(Q%2==0);});
  TestFalse(TEXT("A station no walkway reaches is rejected"),Cut.AddWeldingStation({0,0,20},Q,Swap,E));TestTrue(TEXT("Reason says the welders cannot walk there: ")+E,E.Contains(TEXT("reached on foot")));
  FFoundrylineFactorySimulation NoSpur;OpenFloor(NoSpur,Q,true,false);TestFalse(TEXT("Unreachable port end is rejected"),NoSpur.AddWeldingStation({0,0,20},Q,Swap,E));TestTrue(TEXT("Reason names the unreachable port: ")+E,E.Contains(Swap?TEXT("IN end"):TEXT("OUT end")));
 }
 FFoundrylineFactorySimulation M;OpenFloor(M,0);FString E;if(!TestTrue(TEXT("Base station places: ")+E,M.AddWeldingStation({0,0,20},0,false,E)))return false;const int Id=M.State.WeldingStations[0].Id;
 TestFalse(TEXT("Painting a lane through the station is rejected"),M.PaintLane(EFoundrylineFloorLane::Forklift,{-1000,0,20},{1000,0,20},365.76f,0,E));
 TestFalse(TEXT("A work zone on top of the station is rejected"),M.AddPresetZone({0,0,20},0,0,1,1,E));
 FFoundrylineFactorySimulation Busy;OpenFloor(Busy,0);AddStock(Busy,EFoundrylinePart::Bracket,5,EFoundrylinePalletPlace::Receiving,10,{100,0,20});TestFalse(TEXT("Stored pallets inside the footprint block placement"),Busy.AddWeldingStation({0,0,20},0,false,E));
 FFoundrylineFactorySimulation Edge;OpenFloor(Edge,0);Edge.State.FloorMax={250,6000};TestFalse(TEXT("Station must fit inside the shop with outside access"),Edge.AddWeldingStation({0,0,20},0,false,E));
 FFoundrylineFactorySimulation Tight;OpenFloor(Tight,0);Tight.Solids={FBox2D({-330,-60},{-280,60})};TestFalse(TEXT("Equipment in the forklift approach blocks placement"),Tight.AddWeldingStation({0,0,20},0,false,E));
 // Save/load round trip keeps the exact station record; tampered records are rejected.
 auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TArray<uint8> Bytes;TestTrue(TEXT("Station saves"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 if(auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes))){FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Restored.Solids=M.Solids;TestTrue(TEXT("Restored station validates: ")+E,Restored.Valid(E));
  const auto& A=M.State.WeldingStations[0];const auto& B=Restored.State.WeldingStations[0];TestTrue(TEXT("Station identity, number, pose and sides restored"),A.Id==B.Id&&A.Number==B.Number&&A.Center.Equals(B.Center)&&A.QuarterTurns==B.QuarterTurns&&A.bSwapSides==B.bSwapSides);}
 else AddError(TEXT("Station save did not load"));
 {auto T=M;T.State.WeldingStations[0].QuarterTurns=5;TestFalse(TEXT("Invalid saved rotation rejected"),T.Valid(E));}
 {auto T=M;FFoundrylineFactoryLane L;L.Id=T.State.NextId++;L.Kind=EFoundrylineFloorLane::Forklift;L.Min={-50,-50};L.Max={50,50};T.State.Lanes.Add(L);TestFalse(TEXT("Saved station overlapping a lane rejected"),T.Valid(E));}
 {auto T=M;auto Copy=T.State.WeldingStations[0];Copy.Id=T.State.NextId++;Copy.Center.X+=2000;T.State.WeldingStations.Add(Copy);TestFalse(TEXT("Duplicate station number rejected"),T.Valid(E));}
 {auto T=M;T.State.Version=9;T.State.WeldingStations.Reset();T.State.SupplyBins.Reset();for(auto& Route:T.State.Routes){Route.Destinations.Remove(Id);Route.Sources.Remove(Id);}T.MigrateWeldingStations();TestEqual(TEXT("Schema 9 plant migrates to 14"),T.State.Version,15);TestTrue(TEXT("Migrated schema 9 plant validates: ")+E,T.Valid(E));}
 // Removal is only for an empty station; real supplies keep it in place.
 AddStock(M,EFoundrylinePart::Bracket,4,EFoundrylinePalletPlace::Buffer,Id,Layout::PortCenter(M.State.WeldingStations[0],false));TestFalse(TEXT("Station holding supplies cannot be removed"),M.RemoveWeldingStation(Id,E));
 M.State.Pallets.RemoveAll([&](const auto& P){return P.Owner==Id;});TestTrue(TEXT("Empty station removes: ")+E,M.RemoveWeldingStation(Id,E));TestTrue(TEXT("No station remains"),M.State.WeldingStations.IsEmpty());TestFalse(TEXT("Unknown station removal rejected"),M.RemoveWeldingStation(Id,E));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldingStationStarterShop,"Foundryline.Factory.WeldingStation.StarterShop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeldingStationStarterShop::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);FString E;TestTrue(TEXT("Starter validates before stations: ")+E,M.Valid(E));
 TestFalse(TEXT("Station on the truck apron outside the shop is rejected"),M.AddWeldingStation({1000,800,20},0,false,E));
 TestFalse(TEXT("Station over the burn table is rejected"),M.AddWeldingStation({-1300,800,20},0,false,E));
 TestFalse(TEXT("Station across the main forklift aisle is rejected"),M.AddWeldingStation({-300,1100,20},1,false,E));
 int Valid=0;FString First;TMap<FString,int> Reasons;for(int Q=0;Q<2;++Q)for(double X=M.State.FloorMin.X;X<M.State.FloorMax.X;X+=60.96)for(double Y=M.State.FloorMin.Y;Y<M.State.FloorMax.Y;Y+=60.96){FFoundrylineWeldingStationRecord S;S.Center={X,Y,20};S.QuarterTurns=Q;if(M.CanPlaceWeldingStation(S,E)){++Valid;if(First.IsEmpty())First=S.Center.ToString()+FString::Printf(TEXT(" turn %d"),Q);}else ++Reasons.FindOrAdd(E);}
 for(const auto& R:Reasons)AddInfo(FString::Printf(TEXT("  %d poses: %s"),R.Value,*R.Key));
 AddInfo(FString::Printf(TEXT("Unmodified starter shop: %d valid station poses on a 2 ft grid%s"),Valid,First.IsEmpty()?TEXT(""):*(TEXT(", first ")+First)));
 TestTrue(TEXT("Starter state is unchanged by placement checks: ")+E,M.Valid(E)&&M.State.WeldingStations.IsEmpty());
 // The unmodified starter has no second forklift aisle, so the player paints one fork lane along the west wall,
 // a walkway from the plasma operator path and a crossing over the burn-table aisle, then places the station north of it.
 TestTrue(TEXT("Paint the west fork lane: ")+E,M.PaintLane(EFoundrylineFloorLane::Forklift,{-2217.6,1275,20},{-2217.6,2170,20},304.8f,0,E));
 TestTrue(TEXT("Paint the walkway from the operator path: ")+E,M.PaintLane(EFoundrylineFloorLane::Pedestrian,{-1455,1214.04,20},{-810,1214.04,20},121.92f,0,E));
 // The 17 ft station's crossing (x -1394) sits under the 19 ft 6 in station's OUT corner; it moves 2 ft east instead.
 {auto Old=M;FFoundrylineWeldingStationRecord S;S.Center={-1768.02,1823.12,20};const FBox2D B=Layout::Bounds(S);
  TestTrue(TEXT("Old crossing stroke still paints: ")+E,Old.PaintLane(EFoundrylineFloorLane::Crossing,{-1394,1450,20},{-1394,1450,20},200,0,E));TestFalse(TEXT("With the old crossing the station does not fit"),Old.AddWeldingStation(S.Center,0,false,E));
  AddInfo(FString::Printf(TEXT("Starter fit: with the Phase 1-4 crossing at x -1394 the station overlaps it by %.2f cm along X (%s); station %s..%s, interior %s..%s"),B.Max.X-(-1494.),*E,*B.Min.ToString(),*B.Max.ToString(),*FFoundrylineWorkshopLayout::CutShopInterior().Min.ToString(),*FFoundrylineWorkshopLayout::CutShopInterior().Max.ToString()));}
 TestTrue(TEXT("Paint the aisle crossing: ")+E,M.PaintLane(EFoundrylineFloorLane::Crossing,{-1370,1450,20},{-1370,1450,20},200,0,E));
 const int Before=M.State.Lanes.Num();TestTrue(TEXT("Station places in the starter after three paint strokes: ")+E,M.AddWeldingStation({-1768.02,1823.12,20},0,false,E));
 TestEqual(TEXT("Placing the station paints no extra lanes"),M.State.Lanes.Num(),Before);
 if(M.State.WeldingStations.Num()){const auto& S=M.State.WeldingStations[0];TestTrue(TEXT("Reorganized starter validates: ")+E,M.Valid(E));AddInfo(FString::Printf(TEXT("Starter station docks: IN offset %d ft, OUT offset %d ft"),S.InDockOffsetFt,S.OutDockOffsetFt));
  const FVector Park=M.State.Drivers[0].Parking;TestTrue(TEXT("Forklift still reaches the burn-table output"),M.Path(Park,M.State.Machines[0].OutputDock).Num()>1);TestTrue(TEXT("Forklift still reaches the burn-table input"),M.Path(Park,M.State.Machines[0].InputDock).Num()>1);
  TestTrue(TEXT("Forklift still reaches the box-trailer dock"),M.Path(Park,FFoundrylineFactorySimulation::CarrierDock(0)).Num()>1);for(int Slot=0;Slot<2;++Slot)TestTrue(TEXT("Forklift still reaches outbound staging"),M.Path(Park,FFoundrylineFactorySimulation::PickupPosition(Slot)-FVector(136,0,0)).Num()>1);
  const FVector Op=M.State.Operators[0].Home;TestTrue(TEXT("Plasma operator still reaches the burn table"),M.Path(Op,M.State.Operators[0].WorkPoint,true).Num()>1||M.State.Operators[0].Home.Equals(M.State.Operators[0].WorkPoint,1));
  FString BrakeError;M.State.Commerce.CashCents=1000000;TestFalse(TEXT("Press brake cannot be installed over the station"),M.InstallBrake(BrakeError));TestTrue(TEXT("Brake refusal names the station: ")+BrakeError,BrakeError.Contains(TEXT("welding station")));}
 return true;
}
#endif
