#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Logistics/FoundrylineRackAccess.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"

namespace {
using Kind=EFoundrylinePlannerItem;using Place=EFoundrylinePalletPlace;
bool Overlap(const FBox& A,const FBox& B){return A.Min.Z<B.Max.Z-.1&&A.Max.Z>B.Min.Z+.1&&A.Min.X<B.Max.X-.1&&A.Max.X>B.Min.X+.1&&A.Min.Y<B.Max.Y-.1&&A.Max.Y>B.Min.Y+.1;}
FBox RackAccess(const FFoundrylinePlannerItem& I,int Side=-1){
 if(I.Kind==Kind::StorageRack||I.Kind==Kind::FixedRack){const auto B=FFoundrylineRackAccess::Service(I,Side);return FBox(FVector(B.Min,20),FVector(B.Max,300));}
 return FBox(ForceInit);
}
// Validate the candidate lane graph before changing any live reservation or load.
// Travel phases: 1 drive to pickup, 2 handle pickup, 3 drive loaded, 4 unload.
bool ReplanPausedLaneJobs(FFoundrylineFactorySimulation& M,FString& E){
 auto& S=M.State;
 auto Reach=[&](FVector A,FVector B){return !M.Path(A,B).IsEmpty();};
 for(auto& J:S.Jobs){
  if(!J.Driver){for(const auto& D:S.Drivers){auto Candidate=J;TArray<FVector> P;if(FFoundrylineRackAccess::Route(M,Candidate,D.Position,P)&&Reach(Candidate.Drop,D.Parking)){J.Pickup=Candidate.Pickup;J.Drop=Candidate.Drop;break;}}}
  if(J.Phase<3&&!Reach(J.Pickup,J.Drop)){E=TEXT("This lane change disconnects a reserved pallet from its destination; keep a connected forklift route");return false;}
  if(!J.Driver){
   bool Access=false;
   for(const auto& D:S.Drivers){const auto* Route=S.Routes.FindByPredicate([&](const auto& R){return R.Id==D.Route&&R.Enabled;});const auto* Source=S.Pallets.FindByPredicate([&](const auto& P){return P.Id==J.SourcePallet;});
    if(!Route||!Source||!Route->Sources.Contains(Source->Owner)||!(Route->Destinations.Contains(J.Destination)||J.DestinationPlace==Place::Outbound&&Route->Destinations.Contains(12)))continue;
    const auto* Active=S.Jobs.FindByPredicate([&](const auto& V){return V.Id==D.Job;});const FVector From=Active?Active->Drop:D.Position;
    if(Reach(From,J.Pickup)&&Reach(J.Drop,D.Parking)){Access=true;break;}
   }
   if(!Access){E=TEXT("This lane change leaves a waiting delivery without forklift access or a return route");return false;}
  }else{
   const auto* D=S.Drivers.FindByPredicate([&](const auto& V){return V.Id==J.Driver&&V.Job==J.Id;});
   if(!D||!Reach(J.Drop,D->Parking)){E=TEXT("This lane change would strand a forklift after its delivery; keep its parking route connected");return false;}
   const FVector Target=J.Phase<3?J.Pickup:J.Drop;
   if(!Reach(D->Position,Target)||(J.Phase==2&&!Reach(D->Position,J.Drop))){E=TEXT("This lane change blocks a forklift's current load or pickup; keep a connected route");return false;}
  }
 }
 for(auto& D:S.Drivers){
  const auto* J=S.Jobs.FindByPredicate([&](const auto& V){return V.Id==D.Job;});
  if(J&&(J->Phase==1||J->Phase==3))D.Path=M.Path(D.Position,J->Phase==1?J->Pickup:J->Drop);
  else if(!J&&FVector::Dist2D(D.Position,D.Parking)>=20){D.Path=M.Path(D.Position,D.Parking);if(D.Path.IsEmpty()){E=TEXT("This lane change would strand a returning forklift");return false;}}
  else D.Path.Reset();
  D.Distance=0;D.WaitFor=0;D.Waiting=false;
 }
 return true;
}
bool Apply(FFoundrylineFactorySimulation& M,Kind K,int Id,FVector At,bool Remove,FString& E,int Quarter,FVector Size){
 auto& S=M.State;const auto Items=FFoundrylinePlannerItems::List(S);const auto* Item=Items.FindByPredicate([&](const auto& V){return V.Kind==K&&V.Id==Id;});
 if(!Item){E=TEXT("That item no longer exists in this business");return false;}
 if(Item->Kind==Kind::FixedRack&&Item->MachineId){E=TEXT("Move or store the complete machine workspace; its input and output racks stay attached");return false;}
 if(S.Running){E=TEXT("Pause production before changing the floor layout");return false;}
 if(K!=Kind::Lane&&!S.Jobs.IsEmpty()){E=TEXT("Let the current forklift deliveries finish before moving or removing equipment");return false;}
 if(At.ContainsNaN()||At.GetAbsMax()>1000000||(FMath::Abs(At.Z-20)>.1&&(K!=Kind::OfficeDesk||FMath::Abs(At.Z-340)>.1))){E=TEXT("Choose a valid floor position");return false;}
 auto BusySlots=[&](const TArray<int32>& Slots){return S.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Rack&&Slots.Contains(P.Owner);});};
 auto ClearRoutes=[&](const TArray<int32>& Endpoints){for(auto& R:S.Routes){R.Sources.RemoveAll([&](int N){return Endpoints.Contains(N);});R.Destinations.RemoveAll([&](int N){return Endpoints.Contains(N);});}};
 auto ClearPeople=[&](int Target){for(auto& O:S.Operators)if(O.Machine==Target){O.Machine=0;O.WorkPoint=O.Target=O.Home;O.Path.Reset();O.Distance=O.Retry=O.Speed=0;}for(auto& W:S.Walkers)if(W.Zone==Target||W.Station==Target){W.Zone=W.Station=W.StationSide=0;W.Path.Reset();W.Distance=W.Retry=W.Speed=0;}};
 if(Quarter< -1||Quarter>3||Size.ContainsNaN()||Size.GetAbsMax()>1000000){E=TEXT("Invalid orientation or size");return false;}
 const int NewQuarter=Quarter<0?Item->Quarter:Quarter,Turn=(NewQuarter-Item->Quarter+4)%4;const FRotator Rotation(0,Turn*90,0);
 auto Point=[&](FVector V){return At+Rotation.RotateVector(V-Item->Center);};
 auto RotateBox=[&](FVector2D Low,FVector2D High){FBox2D B(ForceInit);for(int X:{0,1})for(int Y:{0,1})B+=FVector2D(Point(FVector(X?High.X:Low.X,Y?High.Y:Low.Y,20)));return B;};
 const FVector Delta=At-Item->Center;TSet<int32> MovedSlots;
 if(K==Kind::OfficeDesk){
  auto* D=S.OfficeDesks.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!D)return false;
  if(Remove)S.OfficeDesks.RemoveAll([&](const auto& V){return V.Id==Id;});else{D->Center=At;D->QuarterTurns=NewQuarter;}
 }
 else if(K==Kind::Pallet){
  auto* P=S.Pallets.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!P||P->Reserved||P->BookingId||P->Place==Place::Forks){E=TEXT("This pallet is reserved or already booked for shipping");return false;}
  if(Remove){S.ScrapLb+=P->Quantity*P->UnitLb;S.Pallets.RemoveAll([&](const auto& V){return V.Id==Id;});return true;}
  const FFoundrylineFactorySlot* Slot=nullptr;double Best=250;
  for(const auto& V:S.Slots){const double D=FVector::Dist2D(V.Position,At);if(D<Best&&V.Part==P->Part&&!S.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==Place::Rack&&A.Owner==V.Id;})){Best=D;Slot=&V;}}
  if(!Slot){E=TEXT("Choose an empty rack slot assigned to this part; a forklift will move the pallet");return false;}
  if(S.Drivers.IsEmpty()){E=TEXT("Assign a forklift driver before requesting a pallet move");return false;}
  if(!M.RequestPlanningPalletMove(P->Id,Slot->Id,E)){E=TEXT("No valid forklift move could be scheduled");return false;}
  return true;
 }
 else if(K==Kind::StorageRack||K==Kind::FixedRack){
  if(BusySlots(Item->Slots)){E=TEXT("Empty this rack before moving or removing it");return false;}
  if(Remove){ClearRoutes(Item->Slots);S.Slots.RemoveAll([&](const auto& V){return Item->Slots.Contains(V.Id);});S.StorageRacks.RemoveAll([&](const auto& V){return K==Kind::StorageRack&&V.Id==Id;});}
  else {for(auto& Slot:S.Slots)if(Item->Slots.Contains(Slot.Id)){Slot.Position=Point(Slot.Position);Slot.Dock=Point(Slot.Dock);MovedSlots.Add(Slot.Id);}if(auto* R=S.StorageRacks.FindByPredicate([&](const auto& V){return K==Kind::StorageRack&&V.Id==Id;})){R->Center=At;R->QuarterTurns=NewQuarter;}}
 }
 else if(K==Kind::Machine){
  auto* Machine=S.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!Machine)return false;
  if(Machine->Remaining>0||Machine->ReservedLb>0||Machine->SheetLoad>0){E=TEXT("Finish the current machine cycle before relocating or storing it");return false;}
  TArray<int32> Slots;for(const auto& Slot:S.Slots)if(Slot.MachineId==Id)Slots.Add(Slot.Id);
  if(BusySlots(Slots)||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::MachineInput||P.Place==Place::MachineOutput);})) {E=TEXT("Empty the machine input, output and attached racks first");return false;}
  if(Remove){if(Machine->bStored){E=TEXT("This machine is already in equipment storage");return false;}Machine->StoredSlots.Reset();for(const auto& Slot:S.Slots)if(Slots.Contains(Slot.Id))Machine->StoredSlots.Add(Slot);S.Slots.RemoveAll([&](const auto& V){return Slots.Contains(V.Id);});Machine->bStored=true;Machine->Enabled=false;ClearPeople(Id);}
  else {
   if(Machine->bStored){for(auto Slot:Machine->StoredSlots){Slot.Position=Point(Slot.Position);Slot.Dock=Point(Slot.Dock);S.Slots.Add(Slot);MovedSlots.Add(Slot.Id);}Machine->StoredSlots.Reset();Machine->bStored=false;Machine->Enabled=true;}
   else for(auto& Slot:S.Slots)if(Slots.Contains(Slot.Id)){Slot.Position=Point(Slot.Position);Slot.Dock=Point(Slot.Dock);MovedSlots.Add(Slot.Id);}
   Machine->Origin=At;Machine->QuarterTurns=NewQuarter;Machine->InputDock=Point(Machine->InputDock);Machine->InputStock=Point(Machine->InputStock);Machine->OutputDock=Point(Machine->OutputDock);Machine->OutputStock=Point(Machine->OutputStock);for(auto& O:S.Operators)if(O.Machine==Id){O.WorkPoint=Point(O.WorkPoint);O.Target=O.WorkPoint;O.Path.Reset();}
  }
 }
 else if(K==Kind::Station){
  const int Index=S.WeldingStations.IndexOfByPredicate([&](const auto& V){return V.Id==Id;});if(Index<0)return false;
  if(Remove)return M.RemoveWeldingStation(Id,E);
  if(S.WeldingStations[Index].Stage||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&(P.Place==Place::Buffer||P.Place==Place::StationIn||P.Place==Place::StationOut);})||S.Collectors.ContainsByPredicate([&](const auto& C){return C.Area==Id;})){E=TEXT("Empty the station and release its collector before relocating it");return false;}
  auto Station=S.WeldingStations[Index];S.WeldingStations.RemoveAt(Index);Station.Center=At;Station.QuarterTurns=NewQuarter;
  if(!M.CanPlaceWeldingStation(Station,E))return false;S.WeldingStations.Insert(Station,Index);
  for(auto& Bin:S.SupplyBins)if(Bin.Area==Id){Bin.Position=Point(Bin.Position);Bin.Access=Point(Bin.Access);}
 }
 else if(K==Kind::WorkZone){
  const int Index=S.Zones.IndexOfByPredicate([&](const auto& V){return V.Id==Id;});if(Index<0)return false;auto Z=S.Zones[Index];
  if(Z.Ordered||Z.ActiveStep>=0||Z.InstalledLb>0||Z.ReservedLb>0||S.Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==Id&&P.Place==Place::Buffer;})||S.Collectors.ContainsByPredicate([&](const auto& C){return C.Area==Id;})){E=TEXT("Complete the product and empty/release the work zone before editing it");return false;}
  S.Zones.RemoveAt(Index);
  if(Remove){ClearPeople(Id);ClearRoutes({Id});S.SupplyBins.RemoveAll([&](const auto& B){return B.Area==Id;});}
  else {const auto Main=RotateBox(FVector2D(Z.Min),FVector2D(Z.Max)),Drop=RotateBox(FVector2D(Z.DropMin),FVector2D(Z.DropMax));Z.Min=FVector(Main.Min,20);Z.Max=FVector(Main.Max,20);Z.DropMin=FVector(Drop.Min,20);Z.DropMax=FVector(Drop.Max,20);Z.QuarterTurns=NewQuarter;if(!M.CanPlaceZone(Z,E))return false;S.Zones.Insert(Z,Index);for(auto& B:S.SupplyBins)if(B.Area==Id){B.Position=Point(B.Position);B.Access=Point(B.Access);}}
 }
 else if(K==Kind::Forklift){
  auto* D=S.Drivers.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!D)return false;
  if(D->Job||FVector::Dist2D(D->Position,D->Parking)>25){E=TEXT("Park this forklift and finish its load before editing it");return false;}
  if(Remove){if(D->EmployeeId){E=TEXT("Release the forklift driver's assignment before removing the vehicle");return false;}S.Drivers.RemoveAll([&](const auto& V){return V.Id==Id;});}
  else {D->Position=D->Parking=At;D->Yaw=D->ParkingYaw=NewQuarter*90;D->Path.Reset();D->Distance=0;}
 }
 else if(K==Kind::Lane){
  auto* L=S.Lanes.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!L)return false;
  if(Remove)S.Lanes.RemoveAll([&](const auto& V){return V.Id==Id;});
  else {
   // Direction is encoded against the dominant world axis, not an object yaw.
   // Rotate the actual flow vector before converting it back to that encoding.
   const bool WasX=L->Max.X-L->Min.X>L->Max.Y-L->Min.Y;
   const FVector Flow=Rotation.RotateVector((WasX?FVector::ForwardVector:FVector::RightVector)*(L->Direction==2?-1.:1.));
   const auto Bounds=RotateBox(L->Min,L->Max);L->Min=Bounds.Min;L->Max=Bounds.Max;
   if(L->Direction&&Turn){const bool IsX=L->Max.X-L->Min.X>L->Max.Y-L->Min.Y;L->Direction=(IsX?Flow.X:Flow.Y)<0?2:1;}
   if(!Size.IsNearlyZero()){
    if(Size.X<360||Size.Y<360){E=TEXT("Forklift areas need at least 11 ft 10 in on each side so two forklifts can pass");return false;}
    L->Min=FVector2D(At)-FVector2D(Size)/2;L->Max=FVector2D(At)+FVector2D(Size)/2;
   }
  }
 }
 else {E=TEXT("This item is not editable");return false;}
 if(!Remove){
  const auto After=FFoundrylinePlannerItems::List(S);auto AccessCheck=M;AccessCheck.Solids=FFoundrylineWorkshopLayout::PlantSolids(S);auto ChosenAccess=[&](const auto& I){auto A=RackAccess(I);if(A.IsValid&&!FFoundrylineRackAccess::Clear(AccessCheck,FBox2D(FVector2D(A.Min),FVector2D(A.Max))))A=RackAccess(I,1);return A;};
  for(const auto& A:After){const bool IsSelf=A.Kind==K&&A.Id==Id;const bool Attached=A.Kind==Kind::FixedRack&&A.Slots.ContainsByPredicate([&](int Slot){return MovedSlots.Contains(Slot);});if((!IsSelf&&!Attached)||A.Stored)continue;
   const FBox2D Floor(S.FloorMin,S.FloorMax);const FBox Access=ChosenAccess(A);if(Access.IsValid&&!FFoundrylineRackAccess::Clear(AccessCheck,FBox2D(FVector2D(Access.Min),FVector2D(Access.Max)))){E=TEXT("Keep at least one rack face accessible to forklifts");return false;}if(!Floor.IsInsideOrOn(FVector2D(A.Bounds.Min))||!Floor.IsInsideOrOn(FVector2D(A.Bounds.Max))||(Access.IsValid&&(!Floor.IsInsideOrOn(FVector2D(Access.Min))||!Floor.IsInsideOrOn(FVector2D(Access.Max))))){E=TEXT("Keep the complete item and its loading aisle inside the usable shop floor");return false;}
   for(const auto& B:After){if(B.Stored||(B.Kind==K&&B.Id==Id)||(B.Kind==Kind::FixedRack&&B.Slots.ContainsByPredicate([&](int Slot){return MovedSlots.Contains(Slot);})))continue;
    if(A.Kind==Kind::Lane&&(B.Kind==Kind::Lane||B.Kind==Kind::Forklift))continue;
    if(A.Kind!=Kind::Lane&&B.Kind!=Kind::Lane&&B.Kind!=Kind::Pallet&&((Access.IsValid&&Overlap(Access,B.Bounds))||(ChosenAccess(B).IsValid&&Overlap(A.Bounds,ChosenAccess(B))))){E=TEXT("Keep the rack loading aisle clear; shared aisles may overlap");return false;}
    if((A.Kind==Kind::Machine&&B.Kind==Kind::Lane)||(A.Kind==Kind::FixedRack&&B.Kind==Kind::Lane))continue;
    if(Overlap(A.Bounds,B.Bounds)){E=TEXT("That position overlaps another item or a reserved work area");return false;}
   }
  }
 }
 M.Solids=FFoundrylineWorkshopLayout::PlantSolids(S);
 if(K==Kind::Lane){if(!ReplanPausedLaneJobs(M,E))return false;}else for(auto& D:S.Drivers){D.Path.Reset();D.Distance=0;}
 for(auto& O:S.Operators){O.Path.Reset();O.Distance=O.Retry=0;}
 for(auto& W:S.Walkers){W.Path.Reset();W.Distance=W.Retry=0;}
 return true;
}
}
bool FFoundrylineFactorySimulation::EditPlanningItem(int32 KindId,int32 Id,FVector At,bool Remove,FString& E,int32 Quarter,FVector Size){
 if(KindId<1||KindId>9){E=TEXT("Choose an existing planner item");return false;}
 auto Candidate=*this;if(!Apply(Candidate,EFoundrylinePlannerItem(KindId),Id,At,Remove,E,Quarter,Size)||!Candidate.Valid(E))return false;
 ++Candidate.State.Revision;*this=MoveTemp(Candidate);return true;
}

bool FFoundrylineFactorySimulation::RequestPlanningPalletMove(int32 Pallet,int32 Slot,FString& E){
 const auto* P=State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Pallet;});const auto* S=State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});
 if(!P||!S||P->Reserved||P->BookingId||P->Part!=S->Part||Available(P->Part,EFoundrylinePalletPlace::Rack,Slot)>0){E=TEXT("Choose unreserved stock and an empty matching rack slot");return false;}
 return MakeJob(Pallet,Slot,EFoundrylinePalletPlace::Rack,P->Quantity,S->Dock,S->Position,100);
}
