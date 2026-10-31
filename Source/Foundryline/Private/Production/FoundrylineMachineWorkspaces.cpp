#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Economy/FoundrylineEconomyTuning.h"

namespace MachineWorkspace {
int64 Price(int Kind){return Kind==1?850000:Kind==2?FFoundrylineEconomyTuning::PressBrakeCents:600000;}
FBox2D Bounds(int Kind,FVector At,int Quarter){
 const FRotator R(0,Quarter*90,0);FBox2D B(ForceInit);
 const auto Body=FFoundrylinePlannerItems::MachineBounds(Kind,FVector::ZeroVector);
 for(int X:{0,1})for(int Y:{0,1})B+=FVector2D(At+R.RotateVector({X?Body.Max.X:Body.Min.X,Y?Body.Max.Y:Body.Min.Y,0}));
 const double Side=Kind==1?600:500;
 for(int Input:{0,1}){const FVector C(Input&&Kind==1?-600:Side,Input?(Kind==1?0:-250):250,0),Half(75,155,0);for(int X:{-1,1})for(int Y:{-1,1})B+=FVector2D(At+R.RotateVector(C+FVector(X*Half.X,Y*Half.Y,0)));}
 return B.ExpandBy(50);
}
}
FBox2D FFoundrylineFactorySimulation::MachineWorkspaceBounds(int32 Kind,FVector At,int32 Quarter){return MachineWorkspace::Bounds(Kind,At,Quarter);}
bool FFoundrylineFactorySimulation::CanBuyMachine(int Kind,FVector At,int Quarter,FString& E)const{
 if(Kind<1||Kind>3||Quarter<0||Quarter>3||At.ContainsNaN()||FMath::Abs(At.Z-20)>1){E=TEXT("Choose a machine, position and quarter-turn rotation");return false;}
 if(State.Running){E=TEXT("Pause production before placing equipment");return false;}
 if(!State.Commerce.Enabled||State.Commerce.UnpaidCents||State.Commerce.CashCents<MachineWorkspace::Price(Kind)){E=TEXT("Not enough available business cash for this machine");return false;}
 if(State.Machines.Num()>=12||State.Slots.Num()+(Kind==1?6:5)>64){E=TEXT("Machine or pallet-position capacity reached");return false;}
 const auto B=MachineWorkspace::Bounds(Kind,At,Quarter);const FBox2D Floor(State.FloorMin,State.FloorMax);
 if(!Floor.IsInsideOrOn(B.Min)||!Floor.IsInsideOrOn(B.Max)){E=TEXT("Keep the complete machine, racks and access inside the usable floor");return false;}
 for(const auto& Item:FFoundrylinePlannerItems::List(State))if(!Item.Stored&&Item.Kind!=EFoundrylinePlannerItem::Lane&&Item.Kind!=EFoundrylinePlannerItem::Pallet&&B.Intersect(FBox2D(FVector2D(Item.Bounds.Min),FVector2D(Item.Bounds.Max)))){E=TEXT("Machine workspace overlaps existing equipment or storage");return false;}
 for(const auto& L:State.Lanes)if(B.Intersect(FBox2D(L.Min,L.Max))){E=TEXT("Keep the machine workspace clear of existing lanes");return false;}
 return true;
}
void FFoundrylineFactorySimulation::InitialiseMachineRacks(int Id,bool Starter){
 auto* M=State.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!M)return;
 M->bRackOutput=true;const FRotator R(0,M->QuarterTurns*90,0);const int Kind=M->Kind();
 const FVector In=Kind==1?M->Origin+R.RotateVector({-600,0,0}):M->Origin+R.RotateVector({500,-250,0});
 const FVector Out=Starter?FFoundrylineWorkshopLayout::OutputRack(1):M->Origin+R.RotateVector({Kind==1?600.:500.,250,0});
 if(Starter)if(auto* Raw=State.Slots.FindByPredicate([](const auto& S){return S.Id==100;})){
  Raw->MachineId=0;Raw->bInputSupply=true;Raw->Position=FFoundrylineWorkshopLayout::InputRack(1)+FVector(0,-200,152.4);Raw->Dock=FFoundrylineWorkshopLayout::InputRack(1)+FVector(160,-200,0);
 }
 if(Kind==1||!Starter)for(int I=0;I<(Kind==1?2:1);++I){
  FFoundrylineFactorySlot S;S.Id=State.NextId++;S.MachineId=Id;S.bInputSupply=true;S.Part=Kind==2?EFoundrylinePart::Bracket:EFoundrylinePart::Sheet;
  S.Position=In+FVector(0,0,Kind==1?I*152.4:40);S.Dock=In+R.RotateVector({Kind==1?-160.:160.,0,0});State.Slots.Add(S);
  if(I==0){M->InputStock=S.Position;M->InputDock=S.Dock;}
 }
 const int OldIds[]={107,108,109,113};
 for(int I=0;I<4;++I){FFoundrylineFactorySlot* Slot=Starter?State.Slots.FindByPredicate([&](const auto& V){return V.Id==OldIds[I];}):nullptr;
  if(!Slot){FFoundrylineFactorySlot S;S.Id=Starter?OldIds[I]:State.NextId++;State.Slots.Add(S);Slot=&State.Slots.Last();}
  Slot->MachineId=Id;Slot->bInputSupply=false;Slot->bMachineOutput=true;Slot->Part=Kind==2?EFoundrylinePart::BentBracket:I%2?EFoundrylinePart::MountingPlate:EFoundrylinePart::Bracket;
  Slot->Position=Out+R.RotateVector({0,I%2?70.:-70.,I/2*152.4});Slot->Dock=Out+R.RotateVector({160,I%2?70.:-70.,0});
 }
 M->OutputStock=Out;M->OutputDock=Out+R.RotateVector({160,0,0});
 for(auto& Route:State.Routes){Route.Sources.AddUnique(Id);Route.Destinations.AddUnique(Id);for(const auto& S:State.Slots)if(S.MachineId==Id){Route.Sources.AddUnique(S.Id);Route.Destinations.AddUnique(S.Id);}}
}
bool FFoundrylineFactorySimulation::BuyMachine(int Kind,FVector At,int Quarter,FString& E){
 if(!CanBuyMachine(Kind,At,Quarter,E))return false;auto Candidate=*this;
 FFoundrylineFactoryMachine M;M.Id=Candidate.State.NextId++;M.MachineType=Kind;M.QuarterTurns=Quarter;M.Origin=At;M.Auto=false;M.Making=Kind==2?EFoundrylinePart::BentBracket:Kind==3?EFoundrylinePart::ShellSection:EFoundrylinePart::Bracket;
 Candidate.State.Machines.Add(M);Candidate.State.Programs.Append(FFoundrylineFactoryCatalog::MachinePrograms(M.Id,Kind,true));Candidate.InitialiseMachineRacks(M.Id);
 if(Kind==2)Candidate.State.Commerce.BrakeOwned=true;
 Candidate.CommerceExpense(MachineWorkspace::Price(Kind),Kind==1?TEXT("Plasma workspace purchase"):Kind==2?TEXT("Press brake workspace purchase"):TEXT("Sheet roller workspace purchase"));
 Candidate.Solids=FFoundrylineWorkshopLayout::PlantSolids(Candidate.State);
 if(!Candidate.Valid(E))return false;++Candidate.State.Revision;*this=MoveTemp(Candidate);return true;
}
int32 FFoundrylineFactorySimulation::SelectMachineOutput(int Id,EFoundrylinePart Part){
 for(auto& S:State.Slots)if(S.MachineId==Id&&S.bMachineOutput){
  const auto* Stock=State.Pallets.FindByPredicate([&](const auto& P){return P.Place==EFoundrylinePalletPlace::Rack&&P.Owner==S.Id;});
  if(Stock&&Stock->Part==Part&&!Stock->Reserved&&Stock->Quantity<FFoundrylineFactoryCatalog::Part(Part).Capacity)return S.Id;
 }
 if(State.Pallets.Num()>=63)return 0;
 for(auto& S:State.Slots)if(S.MachineId==Id&&S.bMachineOutput&&!State.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==EFoundrylinePalletPlace::Rack&&P.Owner==S.Id;})&&!State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==S.Id;})){S.Part=Part;return S.Id;}
 return 0;
}
bool FFoundrylineFactorySimulation::QueueMachineJob(int Id,int Order,int Quantity,FString& E,EFoundrylinePart Component){
 auto* M=State.Machines.FindByPredicate([&](const auto& V){return V.Id==Id&&!V.bStored;});const auto* O=State.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Id==Order&&V.Status==EFoundrylineOrderState::Accepted;});
 if(!M||!O||M->Queue.Num()>=16){E=TEXT("Choose an active order and an available machine");return false;}
 EFoundrylinePart Part=EFoundrylinePart(O->Part);if(M->Kind()==1&&Part==EFoundrylinePart::BentBracket)Part=EFoundrylinePart::Bracket;
 int Multiplier=1;if(O->Part==uint8(EFoundrylinePart::HeadacheRack)){Part=Component==EFoundrylinePart::Sheet?(M->Kind()==2?EFoundrylinePart::BentBracket:EFoundrylinePart::Bracket):Component;FFoundrylineAssemblyRecipe Recipe;FFoundrylineFactoryCatalog::Recipe(1,Recipe);const auto& Bill=Recipe.Bill;const auto Required=Part==EFoundrylinePart::Bracket?EFoundrylinePart::BentBracket:Part;Multiplier=Bill.IsValidIndex(int(Required))?Bill[int(Required)]:0;if(Multiplier<=0){E=TEXT("Choose a component required by this assembly");return false;}}
 if(!State.Programs.ContainsByPredicate([&](const auto& P){return P.Machine==Id&&P.Part==Part;})){E=TEXT("Choose a recipe this machine can make for the selected job");return false;}
 int Allocated=0;for(const auto& V:State.Machines){if(V.ActiveOrderId==Order&&V.Making==Part)Allocated+=V.BatchRemaining+(V.Remaining>0?1:0);for(const auto& B:V.Queue)if(B.OrderId==Order&&B.Part==Part)Allocated+=B.Quantity;}
 int Stock=0;for(const auto& P:State.Pallets)if(P.Part==Part&&!P.BookingId&&(!P.OrderId||P.OrderId==Order))Stock+=P.Quantity;
 // Free stock satisfies older accepted contracts first. Queued batches remain tied to their job.
 for(const auto& Earlier:State.Commerce.Orders)if(Earlier.Status==EFoundrylineOrderState::Accepted&&Earlier.Id!=Order&&(Earlier.AcceptedAt<O->AcceptedAt||Earlier.AcceptedAt==O->AcceptedAt&&Earlier.Id<Order)){
  const bool Same=Earlier.Part==O->Part;int Reserved=0;for(const auto& V:State.Machines){if(V.ActiveOrderId==Earlier.Id&&V.Making==Part)Reserved+=V.BatchRemaining+(V.Remaining>0?1:0);for(const auto& B:V.Queue)if(B.OrderId==Earlier.Id&&B.Part==Part)Reserved+=B.Quantity;}
  if(Same)Stock=FMath::Max(0,Stock-FMath::Max(0,(Earlier.Quantity-Earlier.Delivered)*Multiplier-Reserved));
 }
 int Booked=0;for(const auto& Booking:State.Commerce.Bookings)if(!Booking.Inbound&&Booking.OrderId==Order&&Booking.Status!=EFoundrylineBookingState::Delivered)for(const auto& Item:Booking.Manifest)Booked+=Item.Quantity;
 const int Need=FMath::Max(0,(O->Quantity-O->Delivered-Booked)*Multiplier-Allocated-Stock);
 if(Need<=0){E=TEXT("Existing stock and allocated production already cover this job");return false;}
 const int Count=Quantity>0?FMath::Min(Quantity,Need):FMath::Min(200,Need);if(Count>200){E=TEXT("Queue up to 200 parts in one batch");return false;}
 FFoundrylineMachineBatch B;B.Part=Part;B.Quantity=Count;B.OrderId=Order;M->Queue.Add(B);M->Auto=false;M->Enabled=true;++State.Revision;return true;
}
