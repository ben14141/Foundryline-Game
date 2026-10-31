#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylinePlanningEquipment.h"

bool FFoundrylineFactorySimulation::BuyStoredMachine(int32 Kind,FString& Error){
 if(Kind<1||Kind>4){Error=TEXT("Choose a plasma table, press brake, sheet roller or forklift");return false;}
 const int64 Price=FFoundrylinePlanningEquipment::Price(Kind==4?3:Kind+3,1);
 if(!State.Commerce.Enabled||State.Commerce.UnpaidCents||State.Commerce.CashCents<Price){Error=TEXT("Not enough available business cash for this machine");return false;}
 if(Kind==4){
  if(State.UnplacedForklifts<0||State.Drivers.Num()+State.UnplacedForklifts>=4){Error=TEXT("Forklift ownership limit reached (4)");return false;}
  auto Candidate=*this;++Candidate.State.UnplacedForklifts;Candidate.CommerceExpense(Price,TEXT("Forklift purchase / awaiting placement"));
  if(!Candidate.Valid(Error))return false;++Candidate.State.Revision;*this=MoveTemp(Candidate);return true;
 }
 int32 ReservedSlots=State.Slots.Num();for(const auto& Machine:State.Machines)ReservedSlots+=Machine.StoredSlots.Num();
 if(State.Machines.Num()>=12||ReservedSlots+(Kind==1?6:5)>64){Error=TEXT("Machine or pallet-position capacity reached");return false;}
 // Buying owns equipment; it does not reserve a floor location, pause production,
 // spawn active racks or start consuming material. Placement is a separate edit.
 auto Candidate=*this;FFoundrylineFactoryMachine Machine;Machine.Id=Candidate.State.NextId++;Machine.MachineType=Kind;
 Machine.Origin={0,0,20};Machine.QuarterTurns=0;Machine.Auto=false;Machine.Enabled=false;Machine.bStored=true;
 Machine.Making=Kind==2?EFoundrylinePart::BentBracket:Kind==3?EFoundrylinePart::ShellSection:EFoundrylinePart::Bracket;
 Candidate.State.Machines.Add(Machine);Candidate.State.Programs.Append(FFoundrylineFactoryCatalog::MachinePrograms(Machine.Id,Kind,true));Candidate.InitialiseMachineRacks(Machine.Id);
 auto* Stored=Candidate.State.Machines.FindByPredicate([&](const auto& M){return M.Id==Machine.Id;});
 for(const auto& Slot:Candidate.State.Slots)if(Slot.MachineId==Machine.Id)Stored->StoredSlots.Add(Slot);
 Candidate.State.Slots.RemoveAll([&](const auto& Slot){return Slot.MachineId==Machine.Id;});
 if(Kind==2)Candidate.State.Commerce.BrakeOwned=true;
 Candidate.CommerceExpense(Price,Kind==1?TEXT("Plasma table purchase / unplaced equipment"):Kind==2?TEXT("Press brake purchase / unplaced equipment"):TEXT("Sheet roller purchase / unplaced equipment"));
 if(!Candidate.Valid(Error))return false;
 ++Candidate.State.Revision;*this=MoveTemp(Candidate);return true;
}
