#include "Production/FoundrylineFactorySimulation.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Inventory/FoundrylineSheetStock.h"
namespace {using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;constexpr int FactoryPartCount=FFoundrylineFactoryCatalog::PartCount;
Part Input(Part P){return P==Part::SupportChannel?Part::ReinforcementPlate:P==Part::BentBracket?Part::Bracket:P==Part::FrameRail?Part::RailBlank:Part::ShellBlank;}
int Units(const FFoundrylineAssemblyStep& S){int G=0;for(int Q:S.Parts)if(Q){if(!G)G=Q;else{int B=Q;while(B){int R=G%B;G=B;B=R;}}}return FMath::Max(1,G);}
}
void FFoundrylineFactorySimulation::Machines(float Dt){
 for(auto& M:State.Machines){
  if(!M.Enabled||!MachineStaffReady(M.Id))continue;
  // A newly bought press brake is still being installed.
  if(M.Id==2&&State.Commerce.BrakeInstallMinutes>0)continue;
  if(M.Remaining>0){M.Remaining=FMath::Max(0.f,M.Remaining-Dt);if(M.Remaining>0)continue;
   auto* P=State.Pallets.FindByPredicate([&](const auto& A){return A.Place==Place::MachineOutput&&A.Owner==M.Id&&A.Part==M.Making&&FMath::IsNearlyEqual(A.UnitLb,M.ReservedLb,.01f);});
   if(P)++P->Quantity;else AddPallet(M.Making,1,Place::MachineOutput,M.Id,M.OutputStock,M.ReservedLb);M.ReservedLb=0;
   if(M.HasQueuedProgram){M.Making=M.NextPart;M.BatchRemaining=M.NextBatch;M.Auto=M.NextAuto;M.Enabled=true;M.HasQueuedProgram=false;}continue;
  }
  if(!M.Enabled||!MachineStaffReady(M.Id))continue;
  Part P=M.Making;bool Want=!M.Auto&&M.BatchRemaining>0;
  if(M.Auto){
   if(M.Id==1){int Bent=Wanted(Part::BentBracket)-Total(Part::Bracket),Rail=Wanted(Part::FrameRail)-Total(Part::RailBlank),Shell=Wanted(Part::ShellSection)-Total(Part::ShellBlank);
    for(const auto& Other:State.Machines)if(Other.Remaining>0){if(Other.Making==Part::BentBracket)--Bent;if(Other.Making==Part::FrameRail)--Rail;if(Other.Making==Part::ShellSection)--Shell;}
    if(Bent>0)P=Part::Bracket;else if(Rail>0)P=Part::RailBlank;else if(Shell>0)P=Part::ShellBlank;else continue;
   }else if(M.Id==2){if(Wanted(Part::BentBracket)>0)P=Part::BentBracket;else if(Wanted(Part::FrameRail)>0)P=Part::FrameRail;else continue;}
   else {P=Part::ShellSection;if(!Wanted(P))continue;}Want=true;
  }
  if(!Want)continue;
  bool Blocked=false;for(const auto& A:State.Pallets)if(A.Place==Place::MachineOutput&&A.Owner==M.Id&&(A.Part!=P||A.Quantity+A.Reserved>=FFoundrylineFactoryCatalog::Part(P).Capacity))Blocked=true;
  if(Blocked)continue;
  const auto* Program=State.Programs.FindByPredicate([&](const auto& A){return A.Machine==M.Id&&A.Part==P;});if(!Program)continue;
  float Lb=0;
  if(M.Id==1){
   // Loading a new sheet onto the bed takes its process time before the first cut from it.
   if(M.BedLb+.0001f<Program->SteelLb){if(!Available(Part::Sheet,Place::MachineInput,1))continue;M.SheetLoad+=Dt;if(M.SheetLoad<FFoundrylineEconomyTuning::SheetLoadMinutes)continue;M.SheetLoad=0;float SheetLb=0;Consume(Part::Sheet,1,Place::MachineInput,1,&SheetLb);M.BedLb+=SheetLb;++State.RawConsumed;}
   if(M.BedLb+.0001f<Program->SteelLb)continue;M.BedLb-=Program->SteelLb;Lb=Program->SteelLb;
  }else if(!Consume(Input(P),1,Place::MachineInput,M.Id,&Lb))continue;
  M.Making=P;M.ReservedLb=Lb;M.Duration=Program->Seconds;M.Remaining=M.Duration;if(!M.Auto)--M.BatchRemaining;
 }
}
void FFoundrylineFactorySimulation::Assembly(float Dt){
 const auto Steps=FFoundrylineFactoryCatalog::TankerSteps();
 for(auto& Z:State.Zones){if(!Z.Ordered||!ReadyForWork(Z))continue;
  if(Z.ActiveStep<0){
   for(int I=0;I<Steps.Num();++I){if((Z.CompletedMask&(1<<I))||(Z.CompletedMask&Steps[I].Prerequisites)!=Steps[I].Prerequisites)continue;
    const int U=Units(Steps[I]);bool Enough=true;for(int P=0;P<FactoryPartCount;++P)if(Available(Part(P),Place::Buffer,Z.Id)<Steps[I].Parts[P]/U)Enough=false;if(!Enough)continue;
    Z.ActiveStep=I;Z.Work=0;Z.ReservedParts.Init(0,FactoryPartCount);Z.ReservedLb=0;
    for(int P=0;P<FactoryPartCount;++P){const int Q=Steps[I].Parts[P]/U;Z.ReservedParts[P]=Q;if(Q){float Lb=0;Consume(Part(P),Q,Place::Buffer,Z.Id,&Lb);Z.ReservedLb+=Lb;}}break;
   }
  }
  if(Z.ActiveStep<0)continue;const auto& S=Steps[Z.ActiveStep];int AtWork=0;for(const auto& W:State.Walkers)if(W.Zone==Z.Id&&FVector::Dist2D(W.Position,FFoundrylineWorkZoneLayout::WorkPoint(Z))<=1)++AtWork;Z.Work+=Dt*AtWork;
  if(Z.Work<S.Seconds/Units(S))continue;
  Z.InstalledLb+=Z.ReservedLb;Z.ReservedLb=0;for(int I=0;I<FactoryPartCount;++I){Z.Installed[I]+=Z.ReservedParts[I];Z.ReservedParts[I]=0;}
  bool Complete=true;for(int I=0;I<FactoryPartCount;++I)if(Z.Installed[I]<S.Parts[I])Complete=false;
  if(Complete)Z.CompletedMask|=1<<Z.ActiveStep;Z.ActiveStep=-1;Z.Work=0;
  if(Z.CompletedMask==63){Z.Ordered=false;++Z.ProductsCompleted;}
 }
}
bool FFoundrylineFactorySimulation::MakeJob(int Source,int Destination,Place Where,int Count,FVector Dock,FVector Store,int Priority){
 if(Count<=0||State.Jobs.Num()>=32||State.Pallets.Num()>=63)return false;
 auto* A=State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Source;});if(!A||A->Quantity-A->Reserved<Count||A->BookingId&&Where!=Place::Outbound&&A->Place!=Place::Inbound)return false;
 int Physical=0;for(const auto& P:State.Pallets)if(P.Place==Where&&P.Owner==Destination&&(Where!=Place::Buffer||P.Part==A->Part)){if(P.Part!=A->Part||!FMath::IsNearlyEqual(P.UnitLb,A->UnitLb,.01f))return false;Physical+=P.Quantity;}
 if(Where!=Place::Outbound&&Physical+Incoming(A->Part,Destination)+Count>(A->bLegacyFiveStack&&Count==5?5:FFoundrylineFactoryCatalog::Part(A->Part).Capacity))return false;
 if(State.Jobs.ContainsByPredicate([&](const auto& J){return J.SourcePallet==Source||(J.Destination==Destination&&J.Part==A->Part);}))return false;
 bool Allowed=false;for(const auto& R:State.Routes)if(R.Enabled&&R.Sources.Contains(A->Owner)&&(R.Destinations.Contains(Destination)||Where==Place::Outbound&&R.Destinations.Contains(12)))Allowed=true;if(!Allowed)return false;
 FFoundrylineFactoryJob J;J.Id=State.NextId++;J.SourcePallet=Source;J.Part=A->Part;J.Quantity=Count;J.Destination=Destination;J.DestinationPlace=Where;J.Drop=Dock;J.StoredAt=Store;J.Priority=Priority;J.Task=Where==Place::MachineInput?EFoundrylineForkTask::Machine:EFoundrylineForkTask::Storage;
 if(A->Place==Place::Rack){const auto* S=State.Slots.FindByPredicate([&](const auto& X){return X.Id==A->Owner;});if(!S)return false;J.Pickup=S->Dock;}
 else if(A->Place==Place::MachineOutput){const auto* M=State.Machines.FindByPredicate([&](const auto& X){return X.Id==A->Owner;});if(!M)return false;J.Pickup=M->OutputDock;}
 // A whole pallet of finished products is lifted off a welding station's OUT from the forklift stop in line with that pallet.
 else if(A->Place==Place::StationOut){const auto* S=State.WeldingStations.FindByPredicate([&](const auto& X){return X.Id==A->Owner;});const int Slot=S?FFoundrylineWeldingStationLayout::OutSlot(*S,A->Position):-1;if(Slot<0||Count!=A->Quantity)return false;J.Pickup=FFoundrylineWeldingStationLayout::OutPalletDock(*S,Slot);}
 else J.Pickup=A->Place==Place::Inbound?CarrierDock(0):A->Position+FVector(-136,0,0);
 A->Reserved+=Count;State.Jobs.Add(J);return true;
}
void FFoundrylineFactorySimulation::Plan(){
 PlanCommerce();PlanWeldingStations();
 // Demand is bounded to the next two assembly stages, and every incoming item is counted.
 const auto Steps=FFoundrylineFactoryCatalog::TankerSteps();
 for(const auto& Z:State.Zones)if(Z.Ordered){
  TArray<int> Need;Need.Init(0,FactoryPartCount);int Stages=0;
  for(int S=0;S<Steps.Num()&&Stages<2;++S)if(!(Z.CompletedMask&(1<<S))){++Stages;for(int P=0;P<FactoryPartCount;++P)Need[P]+=Steps[S].Parts[P];}
  for(int P=0;P<FactoryPartCount;++P){int Q=Need[P]-Z.Installed[P]-Z.ReservedParts[P]-Available(Part(P),Place::Buffer,Z.Id)-Incoming(Part(P),Z.Id);Q=FMath::Min(Q,FFoundrylineFactoryCatalog::Part(Part(P)).Capacity-Available(Part(P),Place::Buffer,Z.Id)-Incoming(Part(P),Z.Id));if(Q<=0)continue;
   for(const auto& A:State.Pallets)if(A.Part==Part(P)&&A.Place==Place::Rack&&A.Quantity>A.Reserved){FVector Pos=BufferPosition(Z,Part(P));if(MakeJob(A.Id,Z.Id,Place::Buffer,FMath::Min(Q,A.Quantity-A.Reserved),Pos+FFoundrylineWorkZoneLayout::Rotation(Z).RotateVector(FVector(0,Z.SupplySide*136,0)),Pos,Z.ActiveStep<0?100:80))break;}
  }
 }
 // Replenish each machine without mixing two input part types on the same pallet spot.
 for(const auto& M:State.Machines){if(!M.Enabled||!MachineStaffReady(M.Id))continue;Part In=Part::Sheet;int Need=0;
  if(M.Id==1){bool Demand=false;for(auto P:{Part::BentBracket,Part::FrameRail,Part::ShellSection})Demand|=Wanted(P)>0;if((M.Auto&&Demand)||(!M.Auto&&M.BatchRemaining>0))Need=10;}
  else {Part Out=M.Id==3?Part::ShellSection:(!M.Auto?M.Making:Wanted(Part::BentBracket)>0?Part::BentBracket:Part::FrameRail);In=Input(Out);Need=M.Auto?Wanted(Out):M.BatchRemaining;Need=FMath::Min(Need,FFoundrylineFactoryCatalog::Part(In).Capacity);}
  bool Other=false;for(const auto& A:State.Pallets)if(A.Owner==M.Id&&A.Place==Place::MachineInput&&A.Part!=In&&A.Quantity)Other=true;if(Other)continue;
  Need-=Available(In,Place::MachineInput,M.Id)+Incoming(In,M.Id);if(Need<=0)continue;
  const auto* InputSlot=State.Slots.FindByPredicate([&](const auto& S){return S.MachineId==M.Id&&S.bInputSupply&&S.Part==In;});if(!InputSlot)continue;
  for(const auto& A:State.Pallets)if(A.Part==In&&A.Place==Place::Rack&&A.Owner==InputSlot->Id&&A.Quantity>A.Reserved){if(MakeJob(A.Id,M.Id,Place::MachineInput,FMath::Min(Need,A.Quantity-A.Reserved),M.InputDock,M.InputStock,70))break;}
  // Cut blanks first enter the downstream machine's dedicated supply rack.
  const int Stock=Available(In,Place::Rack,InputSlot->Id)+Incoming(In,InputSlot->Id);const int Room=FMath::Min(Need-Stock,FFoundrylineFactoryCatalog::Part(In).Capacity-Stock);
  if(Room>0)for(const auto& A:State.Pallets)if(A.Part==In&&A.Place==Place::Rack&&A.Owner!=InputSlot->Id&&A.Quantity>A.Reserved){if(MakeJob(A.Id,InputSlot->Id,Place::Rack,FMath::Min(Room,A.Quantity-A.Reserved),InputSlot->Dock,InputSlot->Position,75))break;}
 }
 // Every completed pallet enters the producing machine output rack, including cut blanks.
 for(const auto& A:State.Pallets)if(A.Place==Place::MachineOutput&&A.Quantity>A.Reserved){
  // Fill the part's pallet before shipping; flush the last partial batch when
  // its remaining demand is already covered. Avoid long trips for every cut.
  Part Final=A.Part;if(Final==Part::Bracket)Final=Part::BentBracket;else if(Final==Part::RailBlank)Final=Part::FrameRail;else if(Final==Part::ShellBlank)Final=Part::ShellSection;
  const int Remaining=Final==A.Part?Wanted(Final):Wanted(Final)-Total(A.Part);
  const auto* Producer=State.Machines.FindByPredicate([&](const auto& M){return M.Id==A.Owner;});
  const bool ManualDone=Producer&&!Producer->Auto&&Producer->BatchRemaining==0&&Producer->Remaining<=0;
  if(A.Quantity<FFoundrylineFactoryCatalog::Part(A.Part).Capacity&&Remaining>0&&!ManualDone)continue;
  if(State.Commerce.Enabled){if(Producer&&A.Quantity<FFoundrylineFactoryCatalog::Part(A.Part).Capacity&&Producer->Enabled&&Producer->Making==A.Part&&(Producer->BatchRemaining>0||Producer->Remaining>0)&&!Producer->HasQueuedProgram)continue;
   bool HasSlot=State.Slots.ContainsByPredicate([&](const auto& S){return S.Part==A.Part&&!S.bInputSupply&&Available(A.Part,Place::Rack,S.Id)+Incoming(A.Part,S.Id)<FFoundrylineFactoryCatalog::Part(A.Part).Capacity;});if(!HasSlot)for(auto& S:State.Slots)if(!S.bInputSupply&&!State.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Rack&&P.Owner==S.Id;})&&!State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==S.Id;})){S.Part=A.Part;break;}}
  for(const auto& S:State.Slots)if(S.Part==A.Part&&!S.bInputSupply){int Present=Available(A.Part,Place::Rack,S.Id)+Incoming(A.Part,S.Id);int Room=FFoundrylineFactoryCatalog::Part(A.Part).Capacity-Present;if(Room>0&&MakeJob(A.Id,S.Id,Place::Rack,FMath::Min(Room,A.Quantity-A.Reserved),S.Dock,S.Position,60))break;}
 }
 // Receiving can replenish a dedicated raw-steel rack, keeping arrival space clear.
 for(const auto& A:State.Pallets)if(A.Place==Place::Receiving&&A.Quantity>A.Reserved)for(const auto& S:State.Slots)if(S.Part==A.Part){int Room=FFoundrylineFactoryCatalog::Part(A.Part).Capacity-Available(A.Part,Place::Rack,S.Id)-Incoming(A.Part,S.Id);if(Room>0&&MakeJob(A.Id,S.Id,Place::Rack,FMath::Min(Room,A.Quantity-A.Reserved),S.Dock,S.Position,20))break;}
}
void FFoundrylineFactorySimulation::Dispatch(){
 // The compact starter has one shared narrow freight corridor. Lease the whole
 // loaded trip and return-to-parking leg, so opposing loads never enter together.
 // The next waiting driver receives the lease, rather than racing for a pallet.
 bool CorridorBusy=false;for(const auto& D:State.Drivers)if(D.Job||FVector::Dist2D(D.Position,D.Parking)>20)CorridorBusy=true;
 TArray<int> DriverOrder;for(int I=0;I<State.Drivers.Num();++I)DriverOrder.Add(I);DriverOrder.Sort([&](int A,int B){const int IA=State.Drivers[A].Id>State.LastDriver?State.Drivers[A].Id:State.Drivers[A].Id+1000000;const int IB=State.Drivers[B].Id>State.LastDriver?State.Drivers[B].Id:State.Drivers[B].Id+1000000;return IA<IB;});
 for(int DriverIndex:DriverOrder){auto& D=State.Drivers[DriverIndex];if(CorridorBusy&&!State.Commerce.Enabled)break;if(D.Job||!D.EmployeeId||State.Commerce.ForkliftInstallDriver==D.Id&&State.Commerce.ForkliftInstallMinutes>0)continue;const auto* R=State.Routes.FindByPredicate([&](const auto& A){return A.Id==D.Route&&A.Enabled;});if(!R)continue;
  FFoundrylineFactoryJob* Best=nullptr;TArray<FVector> BestPath;
  for(auto& J:State.Jobs)if(!J.Driver){const auto* Source=State.Pallets.FindByPredicate([&](const auto& A){return A.Id==J.SourcePallet;});if(!Source||!R->Sources.Contains(Source->Owner)||!(R->Destinations.Contains(J.Destination)||J.DestinationPlace==Place::Outbound&&R->Destinations.Contains(12)))continue;
   auto Score=[&](const auto& V){return (State.Commerce.Enabled&&D.Priorities.Num()==3?(3-D.Priorities[int(V.Task)])*1000:0)+V.Priority;};if(Best&&Score(*Best)>=Score(J))continue;auto P=Path(D.Position,J.Pickup);auto Delivery=Path(J.Pickup,J.Drop);if(P.IsEmpty()||Delivery.IsEmpty()){State.Status=TEXT("A transport request needs connected forklift lanes");continue;}Best=&J;BestPath=MoveTemp(P);
  }
  if(Best){Best->Driver=D.Id;Best->Phase=1;D.Job=Best->Id;D.Path=MoveTemp(BestPath);D.Distance=0;D.Waiting=false;State.LastDriver=D.Id;CorridorBusy=true;}
 }
 for(auto& W:State.Walkers){const auto* Z=State.Zones.FindByPredicate([&](const auto& A){return A.Id==W.Zone;});
  // A station welder stays at Welder A's work point while the station has a unit or a batch to make.
  const auto* St=W.Station?State.WeldingStations.FindByPredicate([&](const auto& A){return A.Id==W.Station;}):nullptr;const bool AtStation=St&&StationNeedsWelder(*St);
  const FVector Goal=Z&&Z->Ordered?FFoundrylineWorkZoneLayout::WorkPoint(*Z):AtStation?FFoundrylineWeldingStationLayout::WorkPoint(*St,W.StationSide):W.Home;
  if((Z||St)&&!W.Path.IsEmpty()&&!W.Path.Last().Equals(Goal,1)){W.Path.Reset();W.Distance=0;W.Retry=0;}
  if(!W.Path.IsEmpty()||W.Retry>0)continue;if((Z&&Z->Ordered||AtStation)&&W.Position.Equals(W.Home,1)&&State.Walkers.ContainsByPredicate([&](const auto& Other){return Other.Id!=W.Id&&!Other.Zone&&!Other.Station&&!Other.Position.Equals(Other.Home,1);}))continue;
  W.Path=W.Position.Equals(Goal,1)?TArray<FVector>{Goal}:Path(W.Position,Goal,true);W.Distance=0;W.Retry=1;
 }
}
void FFoundrylineFactorySimulation::ReplanYield(FFoundrylineFactoryDriver& D,FVector Target){
 float Clearance=MAX_flt;for(const auto& Other:State.Drivers)if(Other.Id!=D.Id)Clearance=FMath::Min(Clearance,float(FVector::Dist2D(D.Position,Other.Position)));
 FVector Escape=D.Position;float Best=Clearance;const int Side=D.Id%2?1:-1;
 for(FVector Offset:{FVector(Side*50,0,0),FVector(-Side*50,0,0),FVector(0,50,0),FVector(0,-50,0)}){const FVector Candidate=D.Position+Offset;if(!Walkable(FVector2D(Candidate),false))continue;float Space=MAX_flt;for(const auto& Other:State.Drivers)if(Other.Id!=D.Id)Space=FMath::Min(Space,float(FVector::Dist2D(Candidate,Other.Position)));if(Space>Best+1||!Walkable(FVector2D(D.Position),false)&&Space>200){Best=Space;Escape=Candidate;}}
 auto Retry=Path(D.Position,Target);
 if(Retry.IsEmpty()&&!Escape.Equals(D.Position,.01)){auto Out=Path(D.Position,Escape),Rest=Path(Escape,Target);if(!Out.IsEmpty()&&!Rest.IsEmpty()){Out.Append(Rest);Retry=MoveTemp(Out);}}
 D.Path=MoveTemp(Retry);D.Distance=0;D.WaitFor=0;
}
const TArray<FFoundrylineFloorCrossing>& FFoundrylineFactorySimulation::FloorCrossings()const{
 uint32 Key=GetTypeHash(State.Lanes.Num());for(const auto& L:State.Lanes){Key=HashCombine(Key,GetTypeHash(L.Id));Key=HashCombine(Key,GetTypeHash(int(L.Kind)));Key=HashCombine(Key,GetTypeHash(L.Min));Key=HashCombine(Key,GetTypeHash(L.Max));}
 if(Key!=CrossingHash){CrossingHash=Key;CachedCrossings=FFoundrylineFloorLaneGeometry::Crossings(State.Lanes);}return CachedCrossings;
}
void FFoundrylineFactorySimulation::Travel(float Dt){
 auto DrivingSpeed=[&](const FFoundrylineFactoryDriver& D){float Ahead=D.Yaw;Along(D.Path,FMath::Min(Length(D.Path),D.Distance+100),&Ahead);const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(D.Yaw,Ahead));return FMath::Lerp(150.f,65.f,FMath::Clamp(Turn/90.f,0.f,1.f));};
 const auto& Crossings=FloorCrossings();
 auto ForkApproaching=[&](const FFoundrylineFloorCrossing& C){for(const auto& D:State.Drivers){if(C.Bounds.ExpandBy(200).IsInside(FVector2D(D.Position)))return true;if(D.Path.Num()>1){const float End=FMath::Min(Length(D.Path),D.Distance+900);for(float At=D.Distance;At<=End;At+=50)if(C.Bounds.ExpandBy(120).IsInside(FVector2D(Along(D.Path,At))))return true;}}return false;};
 auto PeopleBlock=[&](FVector From,FVector To){auto Blocks=[&](FVector Person){const float Before=FVector::Dist2D(From,Person),After=FVector::Dist2D(To,Person),Closest=FMath::PointDistToSegment(Person,From,To);return Closest<120&&!(Before<120&&After>Before+.01&&Closest>=Before-.01);};for(const auto& C:State.Collectors)if(Blocks(C.Position)||Blocks(C.Position-FRotator(0,C.Yaw,0).RotateVector(FVector(90,0,0))))return true;for(const auto& W:State.Walkers)if(Blocks(W.Position))return true;for(const auto& O:State.Operators)if(Blocks(O.Position))return true;for(const auto& D:State.UnassignedEmployees)if(Blocks(D.Position))return true;return false;};
 TArray<int> Done;
 for(auto& D:State.Drivers){D.Waiting=false;if(!D.EmployeeId)continue;if(!D.Job){
  if(FVector::Dist2D(D.Position,D.Parking)<20){D.Path.Reset();D.Yaw=FMath::FixedTurn(D.Yaw,D.ParkingYaw,120*Dt);continue;}
  if(D.Path.IsEmpty()){D.WaitFor+=Dt;if(D.WaitFor>3)ReplanYield(D,D.Parking);continue;}
  const float Next=FMath::Min(Length(D.Path),D.Distance+DrivingSpeed(D)*Dt);float Yaw=D.Yaw;const FVector P=Along(D.Path,Next,&Yaw);
  if(PeopleBlock(D.Position,P))D.Waiting=true;
  for(const auto& Other:State.Drivers)if(Other.Id!=D.Id&&FMath::PointDistToSegment(Other.Position,D.Position,P)<200)D.Waiting=true;
  if(D.Waiting){D.WaitFor+=Dt;if(D.WaitFor>3)ReplanYield(D,D.Parking);continue;}
  D.WaitFor=0;D.Distance=Next;D.Position=P;D.Yaw=FMath::FixedTurn(D.Yaw,Yaw,120*Dt);if(Next>=Length(D.Path)-.1){D.Path.Reset();D.Distance=0;}continue;
 }auto* J=State.Jobs.FindByPredicate([&](const auto& A){return A.Id==D.Job;});if(!J){D.Job=0;continue;}
  if(J->Phase==1||J->Phase==3){if(D.Path.IsEmpty()){D.WaitFor+=Dt;D.Waiting=true;if(D.WaitFor>3)ReplanYield(D,J->Phase==1?J->Pickup:J->Drop);continue;}
   const float Next=FMath::Min(Length(D.Path),D.Distance+DrivingSpeed(D)*Dt);float Yaw=D.Yaw;FVector P=Along(D.Path,Next,&Yaw);
   // Forklifts have crossing priority. Only immediate physical separation can stop one.
   if(PeopleBlock(D.Position,P))D.Waiting=true;
   for(const auto& Other:State.Drivers)if(Other.Id!=D.Id&&FMath::PointDistToSegment(Other.Position,D.Position,P)<200&&((Other.Position-D.Position).Dot((P-D.Position).GetSafeNormal())>0||Other.Id<D.Id))D.Waiting=true;
   if(D.Waiting){D.WaitFor+=Dt;if(D.WaitFor>3)ReplanYield(D,J->Phase==1?J->Pickup:J->Drop);continue;}D.WaitFor=0;D.Distance=Next;D.Position=P;D.Yaw=FMath::FixedTurn(D.Yaw,Yaw,120*Dt);
   if(D.Distance>=Length(D.Path)-.1){D.Path.Reset();D.Distance=0;J->Phase=J->Phase==1?2:4;J->Handling=FFoundrylineEconomyTuning::ForkHandlingMinutes;}
  }else {
   FVector Target=J->StoredAt;if(J->Phase==2)if(const auto* Source=State.Pallets.FindByPredicate([&](const auto& A){return A.Id==J->SourcePallet;}))Target=Source->Position;D.Yaw=FMath::FixedTurn(D.Yaw,(Target-D.Position).Rotation().Yaw,120*Dt);
   J->Handling-=Dt;if(J->Handling>0)continue;
   if(J->Phase==2){auto* A=State.Pallets.FindByPredicate([&](const auto& X){return X.Id==J->SourcePallet;});if(!A||A->Quantity<J->Quantity||A->Reserved<J->Quantity){State.Status=TEXT("Invalid pickup reservation");D.Waiting=true;continue;}float Lb=A->UnitLb;const int Booking=A->Place==Place::Inbound?0:A->BookingId;A->Reserved-=J->Quantity;if(A->Quantity==J->Quantity){J->CarryPallet=A->Id;A->BookingId=Booking;A->Place=Place::Forks;A->Owner=D.Id;A->Position=D.Position;if(J->OrderId)A->OrderId=J->OrderId;}else{A->Quantity-=J->Quantity;J->CarryPallet=AddPallet(J->Part,J->Quantity,Place::Forks,D.Id,D.Position,Lb);auto* New=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==J->CarryPallet;});New->BookingId=Booking;New->OrderId=J->OrderId;}State.Pallets.RemoveAll([](const auto& X){return !X.Quantity&&!X.Reserved;});J->Phase=3;D.Path=Path(D.Position,J->Drop);D.Distance=0;
   }else {auto* Cargo=State.Pallets.FindByPredicate([&](const auto& A){return A.Id==J->CarryPallet;});if(Cargo){auto* Existing=State.Pallets.FindByPredicate([&](const auto& A){return J->DestinationPlace!=Place::Outbound&&J->DestinationPlace!=Place::Pickup&&!A.BookingId&&A.Id!=Cargo->Id&&A.Part==Cargo->Part&&A.Place==J->DestinationPlace&&A.Owner==J->Destination&&FMath::IsNearlyEqual(A.UnitLb,Cargo->UnitLb,.01f);});if(Existing){Existing->Quantity+=Cargo->Quantity;int Id=Cargo->Id;State.Pallets.RemoveAll([&](const auto& A){return A.Id==Id;});}else {Cargo->Place=J->DestinationPlace;Cargo->Owner=J->Destination;Cargo->Position=J->StoredAt;if(J->DestinationPlace==Place::Rack||J->DestinationPlace==Place::MachineInput||J->DestinationPlace==Place::StationIn){Cargo->BookingId=0;Cargo->OrderId=0;}
else{Cargo->OrderId=J->OrderId;Cargo->BookingId=J->BookingId;}}}Done.Add(J->Id);D.Job=0;}
  }
 }
 State.Jobs.RemoveAll([&](const auto& J){return Done.Contains(J.Id);});
 for(auto& W:State.Walkers){W.Speed=0;W.Retry=FMath::Max(0.f,W.Retry-Dt);W.Waiting=false;if(W.Path.IsEmpty()||W.Distance>=Length(W.Path))continue;FVector P=Along(W.Path,W.Distance+110*Dt);
  // Wait before the stripes for approaching forklifts. Anyone already in the crossing clears it.
  for(const auto& C:Crossings)if(!C.Bounds.IsInside(FVector2D(W.Position))&&C.Bounds.ExpandBy(60).IsInside(FVector2D(P))&&ForkApproaching(C))W.Waiting=true;
  if(!CanEmployeeAdvance(W.Position,P,W.Id)){if(TryEmployeePassing(W.Position,P,W.Id,W.Path,W.Distance)){W.Waiting=true;continue;}W.Waiting=true;}
  if(!W.Waiting){const FVector Before=W.Position;W.Distance=FMath::Min(Length(W.Path),W.Distance+110*Dt);W.Position=Along(W.Path,W.Distance);W.Speed=FVector::Dist2D(Before,W.Position)/Dt;W.Yaw=FMath::FixedTurn(W.Yaw,(W.Position-Before).Rotation().Yaw,240*Dt);}
 }
}
