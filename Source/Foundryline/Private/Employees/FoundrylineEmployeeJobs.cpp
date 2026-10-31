#include "Production/FoundrylineFactorySimulation.h"
using Role=EFoundrylineEmployeeRole;
bool FFoundrylineFactorySimulation::AssignEmployee(int32 Id,Role Type,int32 Target,FString& E,int32 Slot){
 if(Type==Role::PartsCollector||State.Collectors.ContainsByPredicate([&](const auto& C){return C.Id==Id;})){if(Slot){E=TEXT("Collector assignments do not take welder slots");return false;}return AssignCollector(Id,Target,E);}
 const auto Roster=FFoundrylineEmployeeRoster::Build(State);const auto* Person=Roster.FindByPredicate([&](const auto& P){return P.Id==Id;});if(!Person||Target<0||int(Type)>3){E=TEXT("Choose an employee and valid job in this business");return false;}
 if(!Target){if(!Person->Assignment){E=TEXT("This employee has no assigned job");return false;}if(Person->Role==Role::Operator){const auto* M=State.Machines.FindByPredicate([&](const auto& V){return V.Id==Person->Assignment;});if(M&&M->Enabled&&M->Remaining>0){E=TEXT("Pause the machine before releasing its operator");return false;}}if(Person->Role==Role::Welder){const auto* Z=State.Zones.FindByPredicate([&](const auto& V){return V.Id==Person->Assignment;});if(State.Running&&Z&&Z->ActiveStep>=0){E=TEXT("Pause production before releasing the welding job");return false;}}return AssignEmployeeInternal(Id,Person->Role,0,E);}
 if(Type==Role::Unassigned){E=TEXT("Choose a specific job and equipment");return false;}if(Person->Assignment){E=TEXT("Release the current job before choosing another assignment");return false;}if(!Person->Position.Equals(Person->Home,1)){E=TEXT("Let the employee reach the waiting area before assigning a new job");return false;}
 if(Type==Role::Operator){if(!State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Target&&!M.bStored;})||State.Operators.ContainsByPredicate([&](const auto& O){return O.Id!=Id&&O.Machine==Target;})){E=TEXT("Choose an installed machine with an available operator station");return false;}if(!State.Operators.ContainsByPredicate([&](const auto& O){return O.Id==Id;})&&State.Operators.Num()>=16){E=TEXT("Operator simulation capacity reached");return false;}}
 // A welder works at a product zone or in one of a welding station's two slots (Welder A or B). Each slot takes one person;
 // the first valid command wins and a later one for the same slot is refused.
 if(Type==Role::Welder){const bool IsStation=State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==Target;});
  if(Slot<0||Slot>1||Slot&&!IsStation){E=TEXT("Choose Welder A or Welder B at a welding station");return false;}
  if(!State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==Target;})&&!IsStation||State.Walkers.ContainsByPredicate([&](const auto& W){return W.Id!=Id&&(W.Zone==Target||W.Station==Target&&W.StationSide==Slot);})){E=IsStation?FString::Printf(TEXT("Welder %s is already taken at that station"),Slot?TEXT("B"):TEXT("A")):FString(TEXT("Choose an available welding station or product work zone"));return false;}}
 else if(Slot){E=TEXT("Only welders take a station slot");return false;}
 if(Type==Role::Welder&&!State.Walkers.ContainsByPredicate([&](const auto& W){return W.Id==Id;})&&State.Walkers.Num()>=16){E=TEXT("Welding simulation capacity reached");return false;}
 if(Type==Role::ForkliftDriver){const auto* D=State.Drivers.FindByPredicate([&](const auto& V){return V.Id==Target;});if(!D||D->EmployeeId||D->Job||FVector::Dist2D(D->Position,D->Parking)>=20){E=TEXT("Choose an owned, parked, unoccupied forklift");return false;}}
 const auto Backup=State;const auto Copy=*Person;
 const bool ExistingOperator=State.Operators.ContainsByPredicate([&](const auto& O){return O.Id==Id;}),ExistingWelder=State.Walkers.ContainsByPredicate([&](const auto& W){return W.Id==Id;});
 if(Type==Role::Operator&&ExistingOperator||Type==Role::Welder&&ExistingWelder)return AssignEmployeeInternal(Id,Type,Target,E,Slot);
 State.Operators.RemoveAll([&](const auto& O){return O.Id==Id;});State.Walkers.RemoveAll([&](const auto& W){return W.Id==Id;});State.UnassignedEmployees.RemoveAll([&](const auto& P){return P.Id==Id;});
 for(auto& Z:State.Zones)Z.Welders=State.Walkers.FilterByPredicate([&](const auto& W){return W.Zone==Z.Id;}).Num();
 if(Type==Role::Operator){FFoundrylineFactoryOperator O;O.Id=Id;O.DisplayName=Copy.Label;O.Position=Copy.Position;O.Home=Copy.Home;O.WorkPoint=O.Target=Copy.Home;O.Yaw=Copy.Yaw;O.Path={O.Position};State.Operators.Add(O);}
 else if(Type==Role::Welder){FFoundrylineFactoryWalker W;W.Id=Id;W.DisplayName=Copy.Label;W.Position=Copy.Position;W.Home=Copy.Home;W.Yaw=Copy.Yaw;W.Path={W.Position};State.Walkers.Add(W);}
 else {FFoundrylineStandbyDriver P;P.Id=Id;P.DisplayName=Copy.Label;P.Position=Copy.Position;P.Home=Copy.Home;P.Yaw=Copy.Yaw;State.UnassignedEmployees.Add(P);}
 if(AssignEmployeeInternal(Id,Type,Target,E,Slot))return true;State=Backup;return false;
}
