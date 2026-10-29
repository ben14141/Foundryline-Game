#include "Production/FoundrylineFactorySimulation.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
namespace {using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;constexpr int SimulationPartCount=FFoundrylineFactoryCatalog::PartCount;
bool CutPart(Part P){return P==Part::Bracket||P==Part::RailBlank||P==Part::ShellBlank;}
Part InputPart(Part Out){return Out==Part::BentBracket?Part::Bracket:Out==Part::FrameRail?Part::RailBlank:Part::ShellBlank;}
}
void FFoundrylineFactorySimulation::Seed(bool StockedWorkshop){
 State=FFoundrylineFactoryState();State.FloorMin=FFoundrylineWorkshopLayout::FloorMin();State.FloorMax=FFoundrylineWorkshopLayout::FloorMax();Solids=FFoundrylineWorkshopLayout::Solids();
 for(int I=1;I<=3;++I){FFoundrylineFactoryMachine M;M.Id=I;M.Origin=FFoundrylineWorkshopLayout::Machine(I);M.InputDock=FFoundrylineWorkshopLayout::InputDock(I);M.OutputDock=FFoundrylineWorkshopLayout::OutputDock(I);M.InputStock=FFoundrylineWorkshopLayout::InputStock(I);M.OutputStock=FFoundrylineWorkshopLayout::OutputStock(I);State.Machines.Add(M);}
 State.Programs=FFoundrylineFactoryCatalog::DefaultPrograms();State.Slots=FFoundrylineWorkshopLayout::Slots();
 AddPallet(Part::Sheet,10,Place::Receiving,10,FFoundrylineWorkshopLayout::SteelArrival(),FFoundrylineSteelSheet::PoundsEach());AddPallet(Part::Sheet,10,Place::Rack,100,State.Slots[0].Position,FFoundrylineSteelSheet::PoundsEach());
 AddPallet(Part::RunningGear,2,Place::Rack,105,State.Slots[5].Position);AddPallet(Part::Fittings,2,Place::Rack,106,State.Slots[6].Position);AddPallet(Part::EndCap,2,Place::Rack,104,State.Slots[4].Position,270);
 FString E;FFoundrylineFactoryRoute Route;Route.Id=1;Route.Name=TEXT("Workshop supply");Route.Sources={10,11,1,2,3};Route.Destinations={1,2,3};for(const auto& Slot:State.Slots){Route.Sources.Add(Slot.Id);Route.Destinations.Add(Slot.Id);}State.Routes.Add(Route);
 AddPresetZone(FFoundrylineWorkshopLayout::ZoneCenter(),2,0,1,1,E);
 auto Lane=[&](EFoundrylineFloorLane Kind,FVector2D A,FVector2D B,int W){FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=Kind;bool X=FMath::Abs(A.X-B.X)>FMath::Abs(A.Y-B.Y);L.Min=FVector2D(FMath::Min(A.X,B.X)-(X?0:W/2),FMath::Min(A.Y,B.Y)-(X?W/2:0));L.Max=FVector2D(FMath::Max(A.X,B.X)+(X?0:W/2),FMath::Max(A.Y,B.Y)+(X?W/2:0));State.Lanes.Add(L);};
 Lane(EFoundrylineFloorLane::Forklift,{-300,100},{-300,4700},500);Lane(EFoundrylineFloorLane::Forklift,{-1900,100},{-1900,4700},400);
 for(float Y:{1500.f})Lane(EFoundrylineFloorLane::Forklift,{-1900,Y},{-300,Y},400);
 for(float Y:{3200.f,4500.f})Lane(EFoundrylineFloorLane::Forklift,{-1900,Y},{-300,Y},400);
 for(float Y:{2550.f,3450.f})Lane(EFoundrylineFloorLane::Forklift,{-300,Y},{2600,Y},400);
 Lane(EFoundrylineFloorLane::Forklift,{2400,2500},{2400,3500},300);
 Lane(EFoundrylineFloorLane::Pedestrian,{-2300,200},{-2300,4800},150);Lane(EFoundrylineFloorLane::Pedestrian,{-2300,200},{2700,200},150);Lane(EFoundrylineFloorLane::Pedestrian,{400,200},{400,4700},150);for(float Y:{1450.f,4550.f})Lane(EFoundrylineFloorLane::Pedestrian,{400,Y},{941.2,Y},100);Lane(EFoundrylineFloorLane::Pedestrian,{400,950},{2700,950},100);
 AddDriver(1,E);AddWelder(State.Zones[0].Id,E);
 if(StockedWorkshop){
  // Starting practice inventory is real stock, consumed and transported normally.
  auto Weight=[](Part P){switch(P){case Part::Bracket:case Part::BentBracket:return 5.f;case Part::RailBlank:case Part::FrameRail:return 52.f;case Part::ShellBlank:case Part::ShellSection:return 96.f;default:return 0.f;}};
  for(const auto& S:State.Slots)if(!Available(S.Part,Place::Rack,S.Id)){
   const int Q=S.Part==Part::Bracket?20:S.Part==Part::BentBracket?8:S.Part==Part::RailBlank||S.Part==Part::FrameRail?4:3;
   AddPallet(S.Part,Q,Place::Rack,S.Id,S.Position,Weight(S.Part));
  }
  const auto& Z=State.Zones[0];
  for(Part P:{Part::FrameRail,Part::BentBracket,Part::ShellSection})AddPallet(P,P==Part::BentBracket?4:2,Place::Buffer,Z.Id,BufferPosition(Z,P),Weight(P));
  // Stage one head and each bought kit in its linked bay, leaving the rest in storage.
  for(Part P:{Part::EndCap,Part::RunningGear,Part::Fittings}){
   auto* Stock=State.Pallets.FindByPredicate([&](const auto& A){return A.Part==P&&A.Place==Place::Rack;});
   if(Stock&&Stock->Quantity>1){const float Lb=Stock->UnitLb;--Stock->Quantity;AddPallet(P,1,Place::Buffer,Z.Id,BufferPosition(Z,P),Lb);}
  }
  for(const auto& M:State.Machines){const Part P=M.Id==1?Part::Bracket:M.Id==2?Part::BentBracket:Part::ShellSection;AddPallet(P,M.Id==1?5:M.Id==2?3:1,Place::MachineOutput,M.Id,M.OutputStock,Weight(P));}
  if(AddPresetZone(FFoundrylineWorkshopLayout::SecondZoneCenter(),2,0,-1,1,E)){
   const auto Second=State.Zones.Last();AddWelder(Second.Id,E);
   for(Part P:{Part::FrameRail,Part::BentBracket,Part::ShellSection,Part::EndCap,Part::RunningGear,Part::Fittings})AddPallet(P,P==Part::BentBracket?4:P==Part::FrameRail||P==Part::ShellSection?2:1,Place::Buffer,Second.Id,BufferPosition(Second,P),P==Part::EndCap?270:Weight(P));
  }
 }
 for(int MachineId=1;MachineId<=3;++MachineId)AddMachineOperator(MachineId,true);
 State.Status=TEXT("Choose a zone and order a tanker. Computer programs control fabrication.");
}
int32 FFoundrylineFactorySimulation::AddPallet(Part P,int32 Q,Place Where,int32 Owner,FVector Position,float UnitLb){FFoundrylineFactoryPallet A;A.Id=State.NextId++;A.Part=P;A.Quantity=Q;A.Place=Where;A.Owner=Owner;A.Position=Position;A.UnitLb=UnitLb;State.Pallets.Add(A);return A.Id;}
int32 FFoundrylineFactorySimulation::Available(Part P,Place Where,int32 Owner)const{int V=0;for(const auto& A:State.Pallets)if(A.Part==P&&A.Place==Where&&(!Owner||A.Owner==Owner))V+=A.Quantity-A.Reserved;return V;}
int32 FFoundrylineFactorySimulation::Incoming(Part P,int32 Owner)const{int V=0;for(const auto& J:State.Jobs)if(J.Part==P&&J.Destination==Owner)V+=J.Quantity;return V;}
int32 FFoundrylineFactorySimulation::Total(Part P)const{int V=0;for(const auto& A:State.Pallets)if(A.Part==P)V+=A.Quantity;return V;}
 float FFoundrylineFactorySimulation::SteelMass()const{float V=State.ScrapLb+State.FinishedSteelLb;for(const auto& A:State.Pallets)V+=A.UnitLb*A.Quantity;for(const auto& M:State.Machines)V+=M.BedLb+M.ReservedLb;for(const auto& Z:State.Zones)V+=Z.ReservedLb+Z.InstalledLb;return V;}
bool FFoundrylineFactorySimulation::Consume(Part P,int Q,Place Where,int Owner,float* Mass){if(Available(P,Where,Owner)<Q)return false;float Lb=0;for(auto& A:State.Pallets)if(A.Part==P&&A.Place==Where&&A.Owner==Owner){int Take=FMath::Min(Q,A.Quantity-A.Reserved);A.Quantity-=Take;Q-=Take;Lb+=Take*A.UnitLb;if(!Q)break;}State.Pallets.RemoveAll([](const auto& A){return !A.Quantity&&!A.Reserved;});if(Mass)*Mass=Lb;return Q==0;}
FVector FFoundrylineFactorySimulation::BufferPosition(const FFoundrylineFactoryZone& Z,Part P)const{return FFoundrylineWorkZoneLayout::Buffer(Z,P);}
bool FFoundrylineFactorySimulation::CanPlaceZone(const FFoundrylineFactoryZone& Z,FString& E)const{
 const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(Z);
 if(State.Zones.Num()>=8||Bounds.Min.ContainsNaN()||Bounds.Max.ContainsNaN()){E=TEXT("Invalid zone or zone limit reached");return false;}
 if(Bounds.Min.X<State.FloorMin.X||Bounds.Min.Y<State.FloorMin.Y||Bounds.Max.X>State.FloorMax.X||Bounds.Max.Y>State.FloorMax.Y){E=TEXT("Entire zone, including supplies, must fit on the shop floor");return false;}
 for(const auto& S:Solids)if(FFoundrylineFloorPlacement::Overlap(S,Bounds)){E=TEXT("Zone overlaps equipment or racks");return false;}
 for(const auto& L:State.Lanes)if(FFoundrylineFloorPlacement::Overlap(FBox2D(L.Min,L.Max),Bounds)){E=TEXT("Work zone overlaps a floor lane or crossing");return false;}
 for(const auto& Other:State.Zones)if(FFoundrylineFloorPlacement::Overlap(FFoundrylineWorkZoneLayout::Bounds(Other),Bounds)){E=TEXT("Another work zone already reserves that space");return false;}
 return true;
}
bool FFoundrylineFactorySimulation::AddPresetZone(FVector Center,int32 Preset,int32 Quarter,int32 Side,int32 Route,FString& E){
 FFoundrylineFactoryZone Z;if(!FFoundrylineWorkZoneLayout::Create(Center,Preset,Quarter,Side,Z)){E=TEXT("Choose a valid tanker preset, rotation and delivery side");return false;}
 auto* R=State.Routes.FindByPredicate([&](const auto& X){return X.Id==Route;});if(!R){E=TEXT("Select a forklift supply route first");return false;}
 if(!CanPlaceZone(Z,E))return false;
 Z.Id=State.NextId++;Z.Welders=0;Z.ReservedParts.Init(0,SimulationPartCount);Z.Installed.Init(0,SimulationPartCount);
 State.Zones.Add(Z);R->Destinations.AddUnique(Z.Id);++State.Revision;return true;
}
bool FFoundrylineFactorySimulation::AddZone(FVector A,FVector B,FString& E){
 if(State.Zones.Num()>=8||A.ContainsNaN()||B.ContainsNaN()){E=TEXT("Invalid work zone or zone limit reached");return false;}
 FFoundrylineFactoryZone Z;Z.Id=State.NextId++;Z.Min=FVector(FMath::Min(A.X,B.X),FMath::Min(A.Y,B.Y),20);Z.Max=FVector(FMath::Max(A.X,B.X),FMath::Max(A.Y,B.Y),20);
 const auto Minimum=FFoundrylineFactoryCatalog::TankerZoneCm();if(Z.Max.X-Z.Min.X<Minimum.X||Z.Max.Y-Z.Min.Y<Minimum.Y){E=TEXT("Tanker requires 20 x 10 m plus a 15 x 4 m drop area");return false;}
 Z.DropMin=FVector(Z.Min.X,Z.Max.Y+200,20);const auto Drop=FFoundrylineFactoryCatalog::TankerDropCm();Z.DropMax=Z.DropMin+FVector(Drop,0);Z.Max.Y=Z.DropMax.Y;
 if(!CanPlaceZone(Z,E))return false;
 Z.ReservedParts.Init(0,SimulationPartCount);Z.Installed.Init(0,SimulationPartCount);Z.Welders=0;State.Zones.Add(Z);++State.Revision;return true;
}
bool FFoundrylineFactorySimulation::AssignSlot(int Id,Part P,FString& E){auto* S=State.Slots.FindByPredicate([&](const auto& X){return X.Id==Id;});if(!S||int(P)>=SimulationPartCount){E=TEXT("Invalid rack position or part");return false;}if(S->MachineId){E=TEXT("Machine rack positions have fixed part assignments");return false;}for(const auto& A:State.Pallets)if(A.Owner==Id&&A.Quantity){E=TEXT("Empty this rack position before changing its part");return false;}for(const auto& J:State.Jobs)if(J.Destination==Id){E=TEXT("A delivery already reserves this rack position");return false;}S->Part=P;++State.Revision;return true;}
bool FFoundrylineFactorySimulation::Order(int Id,FString& E){auto* Z=State.Zones.FindByPredicate([&](const auto& X){return X.Id==Id;});if(!Z||Z->Ordered||Z->CompletedMask){E=TEXT("Choose an idle zone; send finished products to pickup first");return false;}Z->Ordered=true;Z->CompletedMask=0;Z->ActiveStep=-1;Z->Work=0;Z->ReservedParts.Init(0,SimulationPartCount);Z->Installed.Init(0,SimulationPartCount);Z->ReservedLb=Z->InstalledLb=0;State.Running=true;++State.Revision;return true;}
bool FFoundrylineFactorySimulation::Collect(int Id,FString& E){auto* Z=State.Zones.FindByPredicate([&](const auto& X){return X.Id==Id;});if(!Z||Z->CompletedMask!=63){E=TEXT("Complete inspection before sending to pickup");return false;}State.FinishedSteelLb+=Z->InstalledLb;Z->InstalledLb=0;Z->Installed.Init(0,SimulationPartCount);Z->CompletedMask=0;++State.FinishedProducts;++State.Revision;return true;}
bool FFoundrylineFactorySimulation::ReceiveSteel(int Q,FString& E){if(State.Commerce.Enabled){E=TEXT("Purchase steel and schedule delivery on the office computer");return false;}if(Q<1||Q>10||State.Pallets.Num()>=64){E=TEXT("Receive one to ten sheets per pallet");return false;}for(const auto& P:State.Pallets)if(P.Place==Place::Receiving&&P.Owner==10&&P.Quantity){E=TEXT("Clear the arrival pallet before recording another delivery");return false;}AddPallet(Part::Sheet,Q,Place::Receiving,10,State.bCutPartsStarter?FVector(-300,1800,20):FFoundrylineWorkshopLayout::SteelArrival(),FFoundrylineSteelSheet::PoundsEach());State.ReceivedSteelLb+=Q*FFoundrylineSteelSheet::PoundsEach();++State.Revision;return true;}
bool FFoundrylineFactorySimulation::AddDriver(int Route,FString& E,bool Staff){if(State.Drivers.Num()>=(State.Commerce.Enabled?2:4)||!State.Routes.ContainsByPredicate([&](const auto& R){return R.Id==Route;})){E=TEXT("Choose a route; prototype supports four forklifts");return false;}FFoundrylineFactoryDriver D;D.Id=State.NextId++;if(Staff&&EmployeeHireCapacity(EFoundrylineEmployeeRole::Unassigned)>0){D.EmployeeId=D.Id;D.DisplayName=NewEmployeeName(D.EmployeeId,true);}D.Route=Route;D.Position=State.Commerce.Enabled?FVector(100,State.Drivers.IsEmpty()?500:1100,20):FFoundrylineWorkshopLayout::DriverHome(State.Drivers.Num());D.Yaw=90;D.Parking=D.Position;State.Drivers.Add(D);++State.Revision;return true;}
bool FFoundrylineFactorySimulation::AddWelder(int Zone,FString& E){auto* Z=State.Zones.FindByPredicate([&](const auto& A){return A.Id==Zone;});if(!Z){E=TEXT("Select a product zone first");return false;}if(auto* Existing=State.Walkers.FindByPredicate([](const auto& W){return W.Zone==0;})){Existing->Zone=Zone;Existing->Path.Reset();Existing->Distance=0;Existing->Retry=0;Z->Welders=State.Walkers.FilterByPredicate([&](const auto& W){return W.Zone==Zone;}).Num();++State.Revision;return true;}if(EmployeeHireCapacity(EFoundrylineEmployeeRole::Welder)<=0){E=TEXT("Testing limit: four welders per business");return false;}FVector Home;if(!FindEmployeeWaitingPosition(Home)){E=TEXT("No safe waiting space is available");return false;}FFoundrylineFactoryWalker W;W.Id=State.NextId++;W.DisplayName=NewEmployeeName(W.Id,true);W.Zone=Zone;W.Position=W.Home=Home;State.Walkers.Add(W);Z->Welders=State.Walkers.FilterByPredicate([&](const auto& A){return A.Zone==Zone;}).Num();++State.Revision;return true;}
bool FFoundrylineFactorySimulation::Program(int Id,Part P,int Batch,bool Auto,FString& E){auto* M=State.Machines.FindByPredicate([&](const auto& A){return A.Id==Id;});const auto* R=State.Programs.FindByPredicate([&](const auto& A){return A.Machine==Id&&A.Part==P;});if(!M||!R||Batch<1||Batch>200){E=TEXT("Choose a supported part and a batch of 1 to 200");return false;}if(M->Remaining>0){M->NextPart=P;M->NextBatch=Batch;M->NextAuto=Auto;M->HasQueuedProgram=true;++State.Revision;return true;}M->Making=P;M->Auto=Auto;M->BatchRemaining=Batch;M->Enabled=true;++State.Revision;return true;}
int32 FFoundrylineFactorySimulation::Wanted(Part P)const{int Need=0;auto Bill=FFoundrylineFactoryCatalog::TankerBill();for(const auto& Z:State.Zones)if(Z.Ordered){Need+=Bill[int(P)]-Z.Installed[int(P)]-Z.ReservedParts[int(P)];}Need-=Total(P);for(const auto& Stock:State.Pallets)if(Stock.Part==P&&Stock.Place==Place::Buffer&&!State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==Stock.Owner&&Z.Ordered;}))Need+=Stock.Quantity;for(const auto& M:State.Machines)if(M.Remaining>0&&M.Making==P)--Need;return FMath::Max(0,Need);}
bool FFoundrylineFactorySimulation::ReadyForWork(const FFoundrylineFactoryZone& Z)const{for(const auto& W:State.Walkers)if(W.Zone==Z.Id&&FVector::Dist2D(W.Position,FFoundrylineWorkZoneLayout::WorkPoint(Z))<=1)return true;return false;}
FString FFoundrylineFactorySimulation::ZoneStatus(const FFoundrylineFactoryZone& Z)const{if(Z.CompletedMask==63)return TEXT("Finished / ready for pickup");if(!Z.Ordered)return TEXT("Choose a product to start");if(!Z.Welders)return TEXT("Assign a welder");if(!ReadyForWork(Z))return TEXT("Welder walking / check pedestrian lanes");if(Z.ActiveStep>=0)return FFoundrylineFactoryCatalog::TankerSteps()[Z.ActiveStep].Name;return TEXT("Waiting for required parts");}
void FFoundrylineFactorySimulation::Step(float Dt){if(!State.Running)return;float Left=FMath::Clamp(Dt,0.f,.25f)*FMath::Clamp(State.Speed,1.f,8.f);while(Left>.0001f){float Slice=FMath::Min(.1f,Left);TickCommerce(Slice);UpdateOperators(Slice);Plan();Dispatch();Travel(Slice);Machines(Slice);Assembly(Slice);Left-=Slice;}++State.Revision;}
bool FFoundrylineFactorySimulation::Valid(FString& E)const{
 if(!State.CompanyId.IsValid()||!State.PlantId.IsValid()){E=TEXT("Missing company or plant ownership");return false;}
 if(State.Version!=9||State.Pallets.Num()>64||State.Jobs.Num()>32||State.Zones.Num()>8||State.Lanes.Num()>64||State.Routes.Num()>8||State.Drivers.Num()>4||State.Walkers.Num()>16||!FMath::IsFinite(State.Speed)||State.Speed<1||State.Speed>8){E=TEXT("Unsupported or oversized factory save");return false;}
 TSet<int> Ids;auto Id=[&](int V){if(V<=0||Ids.Contains(V))return false;Ids.Add(V);return true;};
 for(const auto& A:State.Pallets)if(!Id(A.Id)||int(A.Part)>=SimulationPartCount||A.Quantity<1||A.Quantity>FFoundrylineFactoryCatalog::Part(A.Part).Capacity||A.Reserved<0||A.Reserved>A.Quantity||A.Position.ContainsNaN()||!FMath::IsFinite(A.UnitLb)||A.UnitLb<0){E=TEXT("Invalid pallet or stock quantity");return false;}
 for(const auto& Z:State.Zones)if(!Id(Z.Id)||Z.Min.ContainsNaN()||Z.Max.ContainsNaN()||Z.DropMin.ContainsNaN()||Z.DropMax.ContainsNaN()||Z.ReservedParts.Num()!=SimulationPartCount||Z.Installed.Num()!=SimulationPartCount||Z.CompletedMask<0||Z.CompletedMask>63||Z.ActiveStep< -1||Z.ActiveStep>5||!FMath::IsFinite(Z.Work)||Z.Work<0||!FMath::IsFinite(Z.ReservedLb)||!FMath::IsFinite(Z.InstalledLb)){E=TEXT("Invalid work zone or assembly progress");return false;}
 for(const auto& D:State.Drivers)if(!Id(D.Id)||D.Position.ContainsNaN()||D.Path.Num()>4096||!FMath::IsFinite(D.Distance)||D.Distance<0){E=TEXT("Invalid driver route");return false;}
 for(const auto& J:State.Jobs){if(!Id(J.Id)||J.Quantity<1||int(J.Part)>=SimulationPartCount||J.Phase<0||J.Phase>4||J.Pickup.ContainsNaN()||J.Drop.ContainsNaN()){E=TEXT("Invalid delivery");return false;}if(J.Phase<3){const auto* P=State.Pallets.FindByPredicate([&](const auto& A){return A.Id==J.SourcePallet;});if(!P||P->Reserved<J.Quantity){E=TEXT("Delivery has no stock reservation");return false;}}else if(!State.Pallets.ContainsByPredicate([&](const auto& A){return A.Id==J.CarryPallet&&A.Place==Place::Forks&&A.Quantity==J.Quantity;})){E=TEXT("Delivery cargo is missing");return false;}}
 for(const auto& L:State.Lanes)if(L.Min.ContainsNaN()||L.Max.ContainsNaN()||L.Min.X>=L.Max.X||L.Min.Y>=L.Max.Y||int(L.Kind)>2){E=TEXT("Invalid painted floor routes");return false;}
 for(int I=0;I<State.Zones.Num();++I){const auto A=FFoundrylineWorkZoneLayout::Bounds(State.Zones[I]);
  for(int J=I+1;J<State.Zones.Num();++J)if(FFoundrylineFloorPlacement::Overlap(A,FFoundrylineWorkZoneLayout::Bounds(State.Zones[J]))){E=TEXT("Saved work zones overlap");return false;}
  for(const auto& L:State.Lanes)if(FFoundrylineFloorPlacement::Overlap(A,FBox2D(L.Min,L.Max))){E=TEXT("Saved work zone overlaps a lane; layout was not loaded");return false;}
 }
 const auto FixedRecipes=FFoundrylineFactoryCatalog::DefaultPrograms();TSet<int> RecipeKeys;
 for(const auto& P:State.Programs){const int Key=P.Machine*SimulationPartCount+int(P.Part);const auto* Fixed=FixedRecipes.FindByPredicate([&](const auto& R){return R.Machine==P.Machine&&R.Part==P.Part;});if(!Fixed||RecipeKeys.Contains(Key)||!FMath::IsNearlyEqual(P.Seconds,Fixed->Seconds)||!FMath::IsNearlyEqual(P.SteelLb,Fixed->SteelLb)){E=TEXT("Part weights and processing times must match the fixed recipe catalog");return false;}RecipeKeys.Add(Key);}
 for(const auto& M:State.Machines){if(M.HasQueuedProgram&&(M.NextBatch<1||M.NextBatch>200||!FixedRecipes.ContainsByPredicate([&](const auto& R){return R.Machine==M.Id&&R.Part==M.NextPart;}))){E=TEXT("Invalid queued part program");return false;}if(M.Remaining>0){const auto* R=FixedRecipes.FindByPredicate([&](const auto& P){return P.Machine==M.Id&&P.Part==M.Making;});if(!R||!FMath::IsNearlyEqual(R->Seconds,M.Duration)){E=TEXT("Active processing time differs from the fixed recipe");return false;}}}
 if(State.Machines.Num()!=(State.bCutPartsStarter?(State.Commerce.BrakeOwned?2:1):3)||State.Slots.Num()>64||State.Programs.Num()!=(State.bCutPartsStarter?(State.Commerce.Enabled?(State.Commerce.BrakeOwned?6:4):6):FFoundrylineFactoryCatalog::DefaultPrograms().Num())||State.NextId<2000||State.Revision<0||State.FloorMin.ContainsNaN()||State.FloorMax.ContainsNaN()||State.FloorMin.X>=State.FloorMax.X||State.FloorMin.Y>=State.FloorMax.Y||State.ScrapLb<0||!FMath::IsFinite(State.ScrapLb)||!FMath::IsFinite(State.ReceivedSteelLb)||State.ReceivedSteelLb<0||!FMath::IsFinite(State.FinishedSteelLb)||State.FinishedSteelLb<0){E=TEXT("Invalid plant layout or ledger");return false;}
 TSet<int> SlotIds,MachineIds,RouteIds;
 for(const auto& S:State.Slots)if(S.Id<100||SlotIds.Contains(S.Id)||int(S.Part)>=SimulationPartCount||S.Position.ContainsNaN()||S.Dock.ContainsNaN()||S.MachineId<0||S.MachineId>3){E=TEXT("Invalid rack assignment");return false;}else SlotIds.Add(S.Id);
 for(const auto& M:State.Machines)if(M.Id<1||M.Id>3||MachineIds.Contains(M.Id)||M.InputDock.ContainsNaN()||M.OutputDock.ContainsNaN()||M.InputStock.ContainsNaN()||M.OutputStock.ContainsNaN()||!FMath::IsFinite(M.Remaining)||!FMath::IsFinite(M.Duration)||!FMath::IsFinite(M.BedLb)||!FMath::IsFinite(M.ReservedLb)||M.Remaining<0||M.Remaining>M.Duration||M.Duration<0||M.BedLb<0||M.ReservedLb<0||int(M.Making)>=SimulationPartCount){E=TEXT("Invalid machine cycle");return false;}else MachineIds.Add(M.Id);
 auto Endpoint=[&](int V){return V==10||V==11||State.Commerce.Enabled&&(V==12||V>=1000&&V<1004||State.Commerce.Bookings.ContainsByPredicate([&](const auto& B){return B.Id==V;}))||SlotIds.Contains(V)||MachineIds.Contains(V)||State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==V;});};
 for(const auto& R:State.Routes){if(R.Id<1||RouteIds.Contains(R.Id)||R.Name.Len()>80||R.Sources.Num()>80||R.Destinations.Num()>80){E=TEXT("Invalid supply route");return false;}RouteIds.Add(R.Id);for(int V:R.Sources)if(!Endpoint(V)){E=TEXT("Unknown pickup endpoint");return false;}for(int V:R.Destinations)if(!Endpoint(V)){E=TEXT("Unknown delivery endpoint");return false;}}
 const auto Bill=FFoundrylineFactoryCatalog::TankerBill();
 for(const auto& Z:State.Zones){
  if(Z.Preset< -1||Z.Preset>2||Z.QuarterTurns<0||Z.QuarterTurns>3||(Z.SupplySide!=1&&Z.SupplySide!=-1)){E=TEXT("Invalid work zone preset");return false;}
  if(Z.Preset>=0){const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(Z);FFoundrylineFactoryZone Expected;
   if(!FFoundrylineWorkZoneLayout::Create(FVector(Bounds.GetCenter(),20),Z.Preset,Z.QuarterTurns,Z.SupplySide,Expected)||!Z.Min.Equals(Expected.Min,.1)||!Z.Max.Equals(Expected.Max,.1)||!Z.DropMin.Equals(Expected.DropMin,.1)||!Z.DropMax.Equals(Expected.DropMax,.1)){E=TEXT("Saved preset geometry differs from its product layout");return false;}
  }
  if(Z.Min.X>=Z.Max.X||Z.Min.Y>=Z.Max.Y||Z.DropMin.X>=Z.DropMax.X||Z.DropMin.Y>=Z.DropMax.Y||Z.Welders<0||Z.Welders>16||Z.ReservedLb<0||Z.InstalledLb<0){E=TEXT("Invalid assembly zone");return false;}for(int I=0;I<SimulationPartCount;++I)if(Z.Installed[I]<0||Z.ReservedParts[I]<0||Z.Installed[I]+Z.ReservedParts[I]>Bill[I]){E=TEXT("Invalid installed or reserved parts");return false;}}
 for(const auto& W:State.Walkers){if(!Id(W.Id)&&!State.Drivers.ContainsByPredicate([&](const auto& D){return D.Id==W.Id;})||W.Position.ContainsNaN()||W.Path.Num()>4096||!FMath::IsFinite(W.Distance)||W.Distance<0||W.Zone&&!State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==W.Zone;})){E=TEXT("Invalid employee assignment");return false;}for(auto P:W.Path)if(P.ContainsNaN()){E=TEXT("Invalid employee path");return false;}}
 for(const auto& D:State.Drivers){if(D.Route&&!RouteIds.Contains(D.Route)||(D.Job&&!State.Jobs.ContainsByPredicate([&](const auto& J){return J.Id==D.Job&&J.Driver==D.Id;}))){E=TEXT("Invalid driver assignment");return false;}for(auto P:D.Path)if(P.ContainsNaN()){E=TEXT("Invalid forklift path");return false;}}
 for(const auto& L:State.Lanes)if(!Id(L.Id)||L.Direction<0||L.Direction>2){E=TEXT("Invalid lane identity");return false;}
 for(const auto& P:State.Pallets){int Reserved=0;for(const auto& J:State.Jobs)if(J.Phase<3&&J.SourcePallet==P.Id)Reserved+=J.Quantity;if(Reserved!=P.Reserved){E=TEXT("Pallet reservations do not match delivery jobs");return false;}if(P.Place==Place::Forks&&!State.Jobs.ContainsByPredicate([&](const auto& J){return J.CarryPallet==P.Id&&J.Driver==P.Owner&&J.Phase>=3;})){E=TEXT("Orphaned forklift cargo");return false;}}
 for(const auto& J:State.Jobs)if(!Endpoint(J.Destination)||J.StoredAt.ContainsNaN()||!FMath::IsFinite(J.Handling)||J.Driver&&!State.Drivers.ContainsByPredicate([&](const auto& D){return D.Id==J.Driver&&D.Job==J.Id;})){E=TEXT("Invalid transport destination or driver");return false;}
 for(int V:Ids)if(V>=State.NextId){E=TEXT("Invalid next object identity");return false;}
 return EmployeesValid(E)&&CommerceValid(E);
}
