#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
namespace {using Activity=EFoundrylineEmployeeActivity;using Wait=EFoundrylineEmployeeWait;using Role=EFoundrylineEmployeeRole;}
FVector FFoundrylineFactorySimulation::OperatorWorkPoint(int32 Id)const{
 const auto* M=State.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!M)return {};
 if(State.bCutPartsStarter&&Id==2)return M->Origin+FRotator(0,M->QuarterTurns*90,0).RotateVector(FVector(180,280,0));
 return M->Origin+FRotator(0,M->QuarterTurns*90,0).RotateVector(M->Kind()==1?FVector(326,201,0):M->Kind()==2?FVector(180,300,0):FVector(115,350,0));
}
void FFoundrylineFactorySimulation::AddOperatorAccessLanes(int32 Machine){
 auto Add=[&](FVector2D Min,FVector2D Max){if(State.Lanes.Num()>=64||State.Lanes.ContainsByPredicate([&](const auto& L){return L.Kind==EFoundrylineFloorLane::Pedestrian&&L.Min.Equals(Min,.01)&&L.Max.Equals(Max,.01);}))return;FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=EFoundrylineFloorLane::Pedestrian;L.Min=Min;L.Max=Max;State.Lanes.Add(L);};
 if(State.bCutPartsStarter&&Machine==2){Add({-2310,140},{-2190,2170});Add({-2250,2050},{-1640,2170});}
 else {const float End=State.bCutPartsStarter?1200:4230;Add({-930,140},{-810,End});}
}
void FFoundrylineFactorySimulation::AddMachineOperator(int32 Machine,bool AtWork){
 if(State.Operators.Num()>=16||!State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Machine;})||State.Operators.ContainsByPredicate([&](const auto& O){return O.Machine==Machine;}))return;
 AddOperatorAccessLanes(Machine);if(auto* Existing=State.Operators.FindByPredicate([](const auto& O){return O.Machine==0;})){Existing->Machine=Machine;Existing->WorkPoint=OperatorWorkPoint(Machine);Existing->Path.Reset();Existing->Distance=Existing->Retry=Existing->Speed=0;Existing->Target=Existing->Home;return;}if(!AtWork&&EmployeeHireCapacity(Role::Operator)<=0)return;FFoundrylineFactoryOperator O;O.Id=State.NextId++;O.DisplayName=NewEmployeeName(O.Id,!AtWork);O.Machine=Machine;O.Home=State.bCutPartsStarter&&Machine==2?FVector(-2250,350,20):FVector(-1100-(Machine-1)*180,350,20);O.WorkPoint=OperatorWorkPoint(Machine);if(AtWork)O.Home=O.WorkPoint;O.Position=AtWork?O.WorkPoint:O.Home;O.Target=O.Position;O.Path={O.Position};State.Operators.Add(O);
}
void FFoundrylineFactorySimulation::MigrateEmployees(){
 if(State.Version!=6)return;const auto SavedLanes=State.Lanes;for(const auto& M:State.Machines)AddMachineOperator(M.Id,true);State.Lanes=SavedLanes;for(auto& W:State.Walkers){W.Home={-2300,300,20};W.Speed=0;W.Retry=0;}State.Version=7;
}
bool FFoundrylineFactorySimulation::MachineStaffReady(int32 Machine)const{
 const auto* Target=State.Machines.FindByPredicate([&](const auto& V){return V.Id==Machine;});if(!Target||Target->bStored)return false;const FVector Station=OperatorWorkPoint(Machine);return State.Operators.ContainsByPredicate([&](const auto& O){return O.Machine==Machine&&FVector::Dist2D(O.Position,Station)<=1;});
}
bool FFoundrylineFactorySimulation::AssignEmployeeInternal(int32 Id,Role Type,int32 Target,FString& E,int32 Slot){
 if(Target<0||int(Type)>2){E=TEXT("Choose a valid employee assignment");return false;}
 if(Type==Role::Operator){auto* O=State.Operators.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!O||Target&&!State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Target&&!M.bStored;})||Target&&State.Operators.ContainsByPredicate([&](const auto& V){return V.Id!=Id&&V.Machine==Target;})){E=TEXT("Choose an installed machine with an available operator position");return false;}if(!Target&&O->Home.Equals(O->WorkPoint,1)){FVector Waiting;if(!FindEmployeeWaitingPosition(Waiting)){E=TEXT("No safe waiting space is available");return false;}O->Home=Waiting;}O->Machine=Target;O->WorkPoint=Target?OperatorWorkPoint(Target):O->Home;O->Path.Reset();O->Distance=0;O->Retry=0;O->Speed=0;O->Wait=Target?Wait::None:Wait::Unassigned;O->Activity=Activity::Idle;O->Target=FVector(MAX_flt,MAX_flt,20);}
 else if(Type==Role::Welder){auto* W=State.Walkers.FindByPredicate([&](const auto& V){return V.Id==Id;});const bool IsStation=Target&&State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==Target;});if(!W||Target&&!IsStation&&!State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==Target;})){E=TEXT("Choose an existing work zone or welding station");return false;}
  // Leaving a station never touches its unit: the work done stays on the bench until a welder returns.
  W->Zone=IsStation?0:Target;W->Station=IsStation?Target:0;W->StationSide=IsStation?FMath::Clamp(Slot,0,1):0;W->Path.Reset();W->Distance=0;W->Retry=0;W->Speed=0;for(auto& Z:State.Zones)Z.Welders=State.Walkers.FilterByPredicate([&](const auto& V){return V.Zone==Z.Id;}).Num();}
 else if(!AssignDriverEmployee(Id,Target,E))return false;
 ++State.Revision;return true;
}
void FFoundrylineFactorySimulation::UpdateOperators(float Dt){
 for(auto& O:State.Operators){O.Speed=0;O.Retry=FMath::Max(0.f,O.Retry-Dt);const auto* M=State.Machines.FindByPredicate([&](const auto& V){return V.Id==O.Machine;});const bool AutoDemand=M&&M->Auto&&State.Zones.ContainsByPredicate([](const auto& Z){return Z.Ordered;});const bool Needed=M&&(M->Remaining>0||M->Enabled&&(M->BatchRemaining>0&&!M->Auto||M->HasQueuedProgram||!M->Queue.IsEmpty()||AutoDemand));O.WorkPoint=M?OperatorWorkPoint(M->Id):O.Home;
  // Let a departing colleague clear the shared access lane before a replacement leaves home.
  if(Needed&&O.Position.Equals(O.Home,1)&&!O.Home.Equals(O.WorkPoint,1)&&State.Operators.ContainsByPredicate([&](const auto& Other){return Other.Id!=O.Id&&!Other.Machine&&!Other.Position.Equals(Other.Home,1);})){O.Target=O.Home;O.Path={O.Home};O.Distance=0;O.Activity=Activity::Waiting;O.Wait=Wait::Traffic;continue;}
  const FVector Goal=Needed?O.WorkPoint:O.Home;
  if(!O.Target.Equals(Goal,1)){O.Target=Goal;O.Path.Reset();O.Distance=0;O.Retry=0;}
  if(FVector::Dist2D(O.Position,Goal)<=1){O.Position=Goal;O.Activity=Activity::Idle;O.Wait=M?Wait::None:Wait::Unassigned;if(Needed){const FVector Screen=M->Origin+FRotator(0,M->QuarterTurns*90,0).RotateVector(M->Kind()==1?FVector(269,197,0):M->Kind()==2?FVector(180,215,0):FVector(115,277.6,0));O.Yaw=(Screen-O.Position).Rotation().Yaw;if(M->Remaining>0){O.Activity=Activity::Operating;O.Wait=Wait::None;}else{O.Activity=Activity::Waiting;O.Wait=Wait::Materials;for(const auto& P:State.Pallets)if(P.Place==EFoundrylinePalletPlace::MachineOutput&&P.Owner==M->Id&&(P.Part!=M->Making||P.Quantity>=FFoundrylineFactoryCatalog::Part(P.Part).Capacity))O.Wait=Wait::OutputFull;}}continue;}
  if(O.Path.IsEmpty()&&O.Retry<=0){const bool Moved=M&&(M->QuarterTurns!=0||M->MachineType!=0||!M->Origin.Equals(State.bCutPartsStarter&&M->Id==2?FVector(-1900,1850,20):FFoundrylineWorkshopLayout::Machine(M->Id),.1));O.Path=Path(O.Position,Goal,true,false,Moved);O.Distance=0;O.Retry=1;}
  if(O.Path.IsEmpty()){O.Activity=Activity::Waiting;O.Wait=Wait::NoRoute;continue;}
  const float Next=FMath::Min(Length(O.Path),O.Distance+110*Dt);float Yaw=O.Yaw;const FVector P=Along(O.Path,Next,&Yaw);if(!CanEmployeeAdvance(O.Position,P,O.Id)){TryEmployeePassing(O.Position,P,O.Id,O.Path,O.Distance);O.Activity=Activity::Waiting;O.Wait=Wait::Traffic;continue;}O.Distance=Next;O.Speed=Dt>0?FVector::Dist2D(O.Position,P)/Dt:0;O.Position=P;O.Yaw=FMath::FixedTurn(O.Yaw,Yaw,240*Dt);O.Activity=Activity::Walking;O.Wait=Wait::None;
 }
}
bool FFoundrylineFactorySimulation::EmployeesValid(FString& E)const{
 auto Fail=[&](){E=TEXT("Invalid employee identity, assignment or walking state");return false;};if(State.Operators.Num()>16)return Fail();TSet<int32> Ids,Assigned;for(const auto& O:State.OfficeAssignments)Ids.Add(O.Id);for(const auto& O:State.OfficeDesks)Ids.Add(O.Id);for(const auto& P:State.Pallets)Ids.Add(P.Id);for(const auto& D:State.Drivers)Ids.Add(D.Id);for(const auto& W:State.Walkers)Ids.Add(W.Id);for(const auto& J:State.Jobs)Ids.Add(J.Id);for(const auto& Z:State.Zones)Ids.Add(Z.Id);for(const auto& L:State.Lanes)Ids.Add(L.Id);for(const auto& O:State.Commerce.Orders)Ids.Add(O.Id);for(const auto& B:State.Commerce.Bookings)Ids.Add(B.Id);
 for(const auto& O:State.Operators){if(O.Id<1||O.Id>=State.NextId||Ids.Contains(O.Id)&&!State.Drivers.ContainsByPredicate([&](const auto& D){return D.Id==O.Id;})||O.Machine<0||O.Machine&&!State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==O.Machine;})||O.Machine&&Assigned.Contains(O.Machine)||O.Position.ContainsNaN()||O.Home.ContainsNaN()||O.WorkPoint.ContainsNaN()||O.Target.ContainsNaN()||O.Path.Num()>4096||!FMath::IsFinite(O.Distance)||O.Distance<0||!FMath::IsFinite(O.Yaw)||!FMath::IsFinite(O.Speed)||O.Speed<0||O.Speed>111||!FMath::IsFinite(O.Retry)||O.Retry<0||int(O.Activity)>7||int(O.Wait)>6)return Fail();for(auto P:O.Path)if(P.ContainsNaN())return Fail();Ids.Add(O.Id);if(O.Machine)Assigned.Add(O.Machine);}
 for(const auto& W:State.Walkers)if(W.Home.ContainsNaN()||!FMath::IsFinite(W.Yaw)||!FMath::IsFinite(W.Speed)||W.Speed<0||W.Speed>111||!FMath::IsFinite(W.Retry)||W.Retry<0)return Fail();
 if(!State.StandbyDrivers.IsEmpty()||State.UnassignedEmployees.Num()>64||State.HireRequests.Num()>32)return Fail();TSet<int32> EmployeeIds,VehicleIds;TSet<FString> Names;for(const auto& D:State.Drivers)VehicleIds.Add(D.Id);
 auto Person=[&](int Id,const FString& Name,bool VehicleAlias){if(Id<1||Id>=State.NextId||EmployeeIds.Contains(Id)||!VehicleAlias&&Ids.Contains(Id)||Name.TrimStartAndEnd().IsEmpty()||Name.Len()>64||Names.Contains(Name))return false;for(TCHAR C:Name)if(C<32||C==127)return false;EmployeeIds.Add(Id);Names.Add(Name);return true;};
 // Operators and welders have already entered the object-ID set above.
 for(const auto& O:State.Operators)if(!Person(O.Id,O.DisplayName,true))return Fail();for(const auto& W:State.Walkers)if(!Person(W.Id,W.DisplayName,true))return Fail();
 for(const auto& D:State.Drivers){if(!D.EmployeeId){if(D.Job||!D.DisplayName.IsEmpty())return Fail();continue;}if(!Person(D.EmployeeId,D.DisplayName,VehicleIds.Contains(D.EmployeeId)))return Fail();}
 for(const auto& C:State.Collectors)if(!Person(C.Id,C.DisplayName,VehicleIds.Contains(C.Id)))return Fail();
 for(const auto& D:State.UnassignedEmployees){if(!Person(D.Id,D.DisplayName,VehicleIds.Contains(D.Id))||D.Position.ContainsNaN()||D.Home.ContainsNaN()||!FMath::IsFinite(D.Yaw))return Fail();}
 for(const auto& O:State.OfficeEmployees)if(!Person(O.Id,O.DisplayName,false)||O.Home.ContainsNaN())return Fail();
 TSet<FGuid> Requests;for(const auto& Request:State.HireRequests){if(!Request.IsValid()||Requests.Contains(Request))return Fail();Requests.Add(Request);}return true;
}
