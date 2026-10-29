#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkZoneLayout.h"
namespace {using Role=EFoundrylineEmployeeRole;}
FString FFoundrylineFactorySimulation::NewEmployeeName(int32 Id,bool Random)const{
 static const TCHAR* First[]={TEXT("Avery"),TEXT("Jordan"),TEXT("Morgan"),TEXT("Riley"),TEXT("Casey"),TEXT("Taylor"),TEXT("Alex"),TEXT("Sam"),TEXT("Cameron"),TEXT("Drew"),TEXT("Jamie"),TEXT("Reese"),TEXT("Quinn"),TEXT("Parker"),TEXT("Logan"),TEXT("Rowan"),TEXT("Blake"),TEXT("Skyler"),TEXT("Hayden"),TEXT("Emerson"),TEXT("Dakota"),TEXT("Finley"),TEXT("Charlie"),TEXT("Sage")};
 static const TCHAR* Last[]={TEXT("Bennett"),TEXT("Carter"),TEXT("Hayes"),TEXT("Walker"),TEXT("Brooks"),TEXT("Reed"),TEXT("Morgan"),TEXT("Hayden"),TEXT("Foster"),TEXT("Ellis"),TEXT("Parker"),TEXT("Sullivan"),TEXT("Cooper"),TEXT("Bailey"),TEXT("Reynolds"),TEXT("Spencer"),TEXT("Mason"),TEXT("Walsh"),TEXT("Harper"),TEXT("Dawson"),TEXT("West"),TEXT("Coleman"),TEXT("Porter"),TEXT("Mitchell")};
 TSet<FString> Used;for(const auto& O:State.Operators)Used.Add(O.DisplayName);for(const auto& W:State.Walkers)Used.Add(W.DisplayName);for(const auto& D:State.Drivers)if(D.EmployeeId)Used.Add(D.DisplayName);for(const auto& D:State.StandbyDrivers)Used.Add(D.DisplayName);for(const auto& D:State.UnassignedEmployees)Used.Add(D.DisplayName);
 constexpr int Count=UE_ARRAY_COUNT(First)*UE_ARRAY_COUNT(Last);const uint32 Start=Random?FMath::RandHelper(Count):HashCombine(GetTypeHash(State.CompanyId),GetTypeHash(Id))%Count;
 for(int I=0;I<Count;++I){const int Pick=(Start+I)%Count;FString Name=FString(First[Pick/UE_ARRAY_COUNT(Last)])+TEXT(" ")+Last[Pick%UE_ARRAY_COUNT(Last)];if(!Used.Contains(Name))return Name;}return FString::Printf(TEXT("%s %s %d"),First[0],Last[0],Id);
}
int32 FFoundrylineFactorySimulation::EmployeeHireCapacity(Role)const{return FMath::Max(0,HireLimitTotal-FFoundrylineEmployeeRoster::Build(State).Num());}
bool FFoundrylineFactorySimulation::FindEmployeeWaitingPosition(FVector& Out)const{
 const auto People=FFoundrylineEmployeeRoster::Build(State);
 for(float Y:{275.f,425.f})for(int Col=0;Col<16;++Col){const FVector P(-2250+110*Col,Y,20);const FVector2D XY(P);if(P.X<State.FloorMin.X+35||P.Y<State.FloorMin.Y+35||P.X>State.FloorMax.X-35||P.Y>State.FloorMax.Y-35)continue;
  bool Blocked=false;for(const auto& S:Solids)if(S.ExpandBy(35).IsInsideOrOn(XY))Blocked=true;for(const auto& L:State.Lanes)if(FBox2D(L.Min,L.Max).ExpandBy(35).IsInsideOrOn(XY))Blocked=true;for(const auto& Z:State.Zones)if(FFoundrylineWorkZoneLayout::Bounds(Z).ExpandBy(35).IsInsideOrOn(XY))Blocked=true;
  for(const auto& E:People){const float Clearance=E.bSeated?300:70;if(FVector::Dist2D(P,E.Position)<Clearance||FVector::Dist2D(P,E.Home)<Clearance)Blocked=true;}for(const auto& D:State.Drivers)if(FVector::Dist2D(P,D.Position)<300)Blocked=true;
  if(!Blocked){Out=P;return true;}
 }return false;
}
bool FFoundrylineFactorySimulation::HireEmployee(Role Type,FGuid Request,FString& E){
 if(int(Type)>3||!Request.IsValid()){E=TEXT("Choose a valid hiring request");return false;}if(State.HireRequests.Contains(Request)){E=TEXT("This employee hiring request was already processed");return false;}if(EmployeeHireCapacity(Type)<=0){E=TEXT("Testing limit: twelve employees in each business");return false;}if(State.NextId<2000||State.NextId>=MAX_int32){E=TEXT("Employee identity capacity reached");return false;}FVector Home;if(!FindEmployeeWaitingPosition(Home)){E=TEXT("No safe employee waiting space is available in this shop");return false;}
 const int Id=State.NextId++;const FString Name=NewEmployeeName(Id,true);
 FFoundrylineStandbyDriver Person;Person.Id=Id;Person.DisplayName=Name;Person.Position=Person.Home=Home;State.UnassignedEmployees.Add(Person);
 State.HireRequests.Add(Request);if(State.HireRequests.Num()>32)State.HireRequests.RemoveAt(0,State.HireRequests.Num()-32);++State.Revision;return true;
}
void FFoundrylineFactorySimulation::MigrateEmployeeNames(){
 if(State.Version!=7)return;for(auto& O:State.Operators)if(O.DisplayName.IsEmpty())O.DisplayName=NewEmployeeName(O.Id,false);for(auto& W:State.Walkers)if(W.DisplayName.IsEmpty())W.DisplayName=NewEmployeeName(W.Id,false);for(auto& D:State.Drivers){D.EmployeeId=D.Id;if(D.DisplayName.IsEmpty())D.DisplayName=NewEmployeeName(D.EmployeeId,false);}State.Version=8;
}

bool FFoundrylineFactorySimulation::AssignDriverEmployee(int32 Employee,int32 Vehicle,FString& E){
 auto* Active=State.Drivers.FindByPredicate([&](const auto& D){return D.EmployeeId==Employee&&Employee>0;});auto* Standby=State.UnassignedEmployees.FindByPredicate([&](const auto& D){return D.Id==Employee;});
 if(!Active&&!Standby){E=TEXT("Choose an employee in this business");return false;}
 if(Active){if(Vehicle){E=TEXT("Set the driver off duty before choosing another forklift");return false;}if(Active->Job||FVector::Dist2D(Active->Position,Active->Parking)>=20){E=TEXT("Let the driver finish the pallet trip and park before going off duty");return false;}FVector Home;if(!FindEmployeeWaitingPosition(Home)){E=TEXT("No safe employee waiting space is available");return false;}FFoundrylineStandbyDriver D;D.Id=Active->EmployeeId;D.DisplayName=Active->DisplayName;D.Position=D.Home=Home;State.UnassignedEmployees.Add(D);Active->EmployeeId=0;Active->DisplayName.Reset();Active->Path.Reset();Active->Distance=0;Active->Waiting=false;return true;}
 if(!Vehicle){E=TEXT("This employee is already off duty");return false;}auto* Fork=State.Drivers.FindByPredicate([&](const auto& D){return D.Id==Vehicle;});if(!Fork||Fork->EmployeeId||Fork->Job||FVector::Dist2D(Fork->Position,Fork->Parking)>=20){E=TEXT("Choose an owned, parked forklift without an assigned driver");return false;}
 if(!Fork->Route)if(const auto* Route=State.Routes.FindByPredicate([](const auto& R){return R.Enabled;}))Fork->Route=Route->Id;Fork->EmployeeId=Standby->Id;Fork->DisplayName=Standby->DisplayName;Fork->Path.Reset();Fork->Distance=0;Fork->Waiting=false;State.UnassignedEmployees.RemoveAll([&](const auto& D){return D.Id==Employee;});return true;
}

void FFoundrylineFactorySimulation::MigrateGenericEmployees(){if(State.Version!=8)return;State.UnassignedEmployees.Append(State.StandbyDrivers);State.StandbyDrivers.Reset();for(auto& W:State.Walkers){bool Unsafe=false;for(const auto& L:State.Lanes)if(FBox2D(L.Min,L.Max).ExpandBy(35).IsInsideOrOn(FVector2D(W.Home)))Unsafe=true;if(Unsafe){FVector Home;if(FindEmployeeWaitingPosition(Home))W.Home=Home;}}State.Version=9;}
