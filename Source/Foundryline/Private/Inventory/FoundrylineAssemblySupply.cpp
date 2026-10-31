#include "Inventory/FoundrylineAssemblySupply.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"

namespace { using Part=EFoundrylinePart; using Place=EFoundrylinePalletPlace; }

TArray<FFoundrylineSupplyItem> FFoundrylineAssemblySupplyCatalog::Items(){
 // Handling weight includes bought-in items even when the legacy steel-only ledger does not.
 TArray<FFoundrylineSupplyItem> Out;
 auto Add=[&](Part P,float Lb,int32 Max){FFoundrylineSupplyItem I;I.Part=int32(P);I.HandlingLb=Lb;I.MaxUnitsPerCompartment=Max;Out.Add(I);};
 Add(Part::Bracket,5,50);Add(Part::BentBracket,5,50);Add(Part::MountingPlate,2,100);
 Add(Part::LightKit,4.5f,30);
 return Out;
}
const FFoundrylineSupplyItem* FFoundrylineAssemblySupplyCatalog::Find(int32 P,const TArray<FFoundrylineSupplyItem>& Items){return Items.FindByPredicate([&](const auto& I){return I.Part==P;});}
FVector FFoundrylineAssemblySupplyCatalog::BinSize(int32 N){return FVector((N/2)*46.2+13,66.96,120);}
int32 FFoundrylineAssemblySupplyCatalog::Fit(float Lb,float Each,int32 Space){if(!FMath::IsFinite(Lb)||!FMath::IsFinite(Each)||Lb<0||Each<=0||Space<=0)return 0;return FMath::Min(Space,FMath::FloorToInt(FMath::Min(100000.f,Lb/Each+.00001f)));}

void FFoundrylineFactorySimulation::MigrateAssemblySupply(){if(State.Version==14){FString Error;for(const auto& Station:State.WeldingStations)if(!State.SupplyBins.ContainsByPredicate([&](const auto& B){return B.Area==Station.Id;}))ConfigureSupplyBin(Station.Id,4,Error);for(auto& Pallet:State.Pallets)if(Pallet.Part==Part::HeadacheRack&&Pallet.Quantity==5)Pallet.bLegacyFiveStack=true;State.Version=15;}}
FTransform FFoundrylineAssemblySupplyCatalog::Transform(const FFoundrylineWeldingStationRecord& S){
 using L=FFoundrylineWeldingStationLayout;const bool Left=L::PortOnLeft(S,false);
 return FTransform(FRotator(0,L::Rotation(S).Yaw+(Left?-90:90),0),L::Point(S,Left?2.9:L::LengthFt-2.9,(S.bSwapSides?3.3:9.7)));
}
int32 FFoundrylineFactorySimulation::CollectorIncoming(int32 Area,int32 P)const{int32 N=0;for(const auto& C:State.Collectors)if(C.Area==Area)for(const auto& L:C.Load)if(L.Part==P)N+=L.Quantity;return N;}
int32 FFoundrylineFactorySimulation::CollectorReserved(int32 Pallet)const{int32 N=0;for(const auto& C:State.Collectors)for(const auto& L:C.Load)if(!L.Picked&&L.SourcePallet==Pallet)N+=L.Quantity;return N;}
bool FFoundrylineFactorySimulation::CollectorSupplies(int32 Area,int32 P)const{return State.SupplyBins.ContainsByPredicate([&](const auto& B){return B.Area==Area&&B.Slots.ContainsByPredicate([&](const auto& S){return S.Part==P;});});}

bool FFoundrylineFactorySimulation::ConfigureSupplyBin(int32 Area,int32 Count,FString& E){
 const auto* Station=State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==Area;});
 if(!Station||(Count!=4&&Count!=6&&Count!=8)){E=TEXT("Choose an assembly station and 4, 6 or 8 compartments");return false;}
 if(State.Version>=15&&(State.Running||State.Collectors.ContainsByPredicate([&](const auto& C){return C.Area==Area&&!C.Load.IsEmpty();})||State.Jobs.ContainsByPredicate([&](const auto& J){return J.Destination==Area;}))){E=TEXT("Pause and finish incoming deliveries before changing the supply bin");return false;}
 auto* Existing=State.SupplyBins.FindByPredicate([&](const auto& B){return B.Area==Area;});
 if(Existing&&Existing->Compartments==Count)return true;
 if(Existing&&Count<Existing->Compartments){E=TEXT("Supply bins upgrade from 4 to 6 to 8 compartments");return false;}
 FFoundrylineAssemblySupplyBin B;B.Area=Area;B.Compartments=Count;B.PayloadLb=Count*100.f;
 const FTransform On=FFoundrylineAssemblySupplyCatalog::Transform(*Station);B.Position=On.GetLocation();B.Yaw=On.Rotator().Yaw;B.Access=On.TransformPosition(FVector(0,-210,0));
 const FVector Size=FFoundrylineAssemblySupplyCatalog::BinSize(Count);
 FBox2D Bounds(ForceInit);for(float X:{-.5f,.5f})for(float Y:{-.5f,.5f})Bounds+=FVector2D(On.TransformPosition(FVector(X*Size.X,Y*Size.Y,0)));
 const auto StationBounds=FFoundrylineWeldingStationLayout::Port(*Station,false);
 if(!StationBounds.IsInsideOrOn(Bounds.Min)||!StationBounds.IsInsideOrOn(Bounds.Max)){E=TEXT("The larger supply bin does not fit this station's IN space");return false;}
 for(const auto& Solid:Solids)if(Solid.Intersect(Bounds)){E=TEXT("Assembly Supply Bin overlaps equipment");return false;}
 if(B.Access.X<State.FloorMin.X||B.Access.Y<State.FloorMin.Y||B.Access.X>State.FloorMax.X||B.Access.Y>State.FloorMax.Y){E=TEXT("Supply-bin cart access is outside the shop");return false;}
 const auto Items=FFoundrylineAssemblySupplyCatalog::Items();FFoundrylineAssemblyRecipe Recipe;FFoundrylineFactoryCatalog::Recipe(Station->Recipe?Station->Recipe:1,Recipe);
 for(const auto& I:Items)if(Recipe.Bill.IsValidIndex(I.Part)&&Recipe.Bill[I.Part]>0){FFoundrylineSupplyCompartment S;S.Part=I.Part;S.RefillTarget=FMath::Min(Recipe.Bill[I.Part]*2,FFoundrylineAssemblySupplyCatalog::Fit(100,I.HandlingLb,I.MaxUnitsPerCompartment));B.Slots.Add(S);}
 if(B.Slots.Num()>Count){E=TEXT("Recipe requires more supply compartments");return false;}
 for(const auto& S:B.Slots){const auto* I=FFoundrylineAssemblySupplyCatalog::Find(S.Part,Items);if(Available(Part(S.Part),Place::Buffer,Area)*I->HandlingLb>S.CapacityLb+.01){E=TEXT("Existing parts exceed the new compartment's capacity");return false;}}
 if(Existing)*Existing=B;else State.SupplyBins.Add(B);++State.Revision;return true;
}

bool FFoundrylineFactorySimulation::AssignCollector(int32 Id,int32 Area,FString& E){
 if(auto* C=State.Collectors.FindByPredicate([&](const auto& V){return V.Id==Id;})){
  if(Area){E=TEXT("Release the collector's current area before changing assignments");return false;}
  C->ReleaseRequested=true;C->Status=TEXT("Finishing delivery, then returning to waiting area");++State.Revision;return true;
 }
 const auto Roster=FFoundrylineEmployeeRoster::Build(State);const auto* Person=Roster.FindByPredicate([&](const auto& P){return P.Id==Id;});
 if(!Person||Person->Role!=EFoundrylineEmployeeRole::Unassigned||!Area||!Person->Position.Equals(Person->Home,1)){E=TEXT("Choose an unassigned employee at the waiting area");return false;}
 if(!State.SupplyBins.ContainsByPredicate([&](const auto& B){return B.Area==Area;}))if(!ConfigureSupplyBin(Area,4,E))return false;
 FFoundrylinePartsCollector C;C.Id=Person->Id;C.DisplayName=Person->Label;C.Home=Person->Home;C.Yaw=Person->Yaw;C.Position=Person->Position+FRotator(0,C.Yaw,0).RotateVector(FVector(90,0,0));C.Area=Area;
 State.Operators.RemoveAll([&](const auto& O){return O.Id==Id;});State.Walkers.RemoveAll([&](const auto& W){return W.Id==Id;});

 State.Collectors.Add(C);State.UnassignedEmployees.RemoveAll([&](const auto& P){return P.Id==Id;});++State.Revision;return true;
}

void FFoundrylineFactorySimulation::UpdateCollectors(float Dt){
 const auto Items=FFoundrylineAssemblySupplyCatalog::Items();
 for(auto& C:State.Collectors){
  C.Speed=0;C.Retry=FMath::Max(0.f,C.Retry-Dt);
  const auto* Bin=State.SupplyBins.FindByPredicate([&](const auto& B){return B.Area==C.Area;});
  const auto* Z=State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==C.Area;});
  FFoundrylineAssemblyRecipe Recipe;const bool Ordered=Z&&Z->BatchRemaining>0&&FFoundrylineFactoryCatalog::Recipe(Z->Recipe,Recipe);
  if(!Bin||!Z){C.Status=TEXT("Assembly area unavailable");continue;}
  // Plan one bounded mixed trip. All lines are reserved before another collector plans.
  if(!C.ReleaseRequested&&C.Load.IsEmpty()&&Ordered&&C.Retry<=0){
   float CartLb=0,BinLb=0;int32 Units=0;
   for(const auto& S:Bin->Slots){const auto* I=FFoundrylineAssemblySupplyCatalog::Find(S.Part,Items);if(I)BinLb+=(Available(Part(S.Part),Place::Buffer,C.Area)+CollectorIncoming(C.Area,S.Part))*I->HandlingLb;}
   for(const auto& S:Bin->Slots){
    const auto* I=FFoundrylineAssemblySupplyCatalog::Find(S.Part,Items);if(!I)continue;
    const int Have=Available(Part(S.Part),Place::Buffer,C.Area),Coming=CollectorIncoming(C.Area,S.Part)+Incoming(Part(S.Part),C.Area);
    const int Remaining=FMath::Max(0,Recipe.Bill[S.Part]*Z->BatchRemaining);
    int Need=FMath::Min(S.RefillTarget,Remaining)-Have-Coming;
    Need=FMath::Min(Need,FFoundrylineAssemblySupplyCatalog::Fit(S.CapacityLb,I->HandlingLb,I->MaxUnitsPerCompartment)-Have-Coming);
    TArray<int32> Sources;for(int32 P=0;P<State.Pallets.Num();++P){const auto& A=State.Pallets[P];if(int32(A.Part)==S.Part&&A.Place==Place::Rack&&!A.OrderId&&!A.BookingId&&A.Quantity>A.Reserved)Sources.Add(P);}
    Sources.Sort([&](int32 A,int32 B){return FVector::DistSquared(C.Position,State.Pallets[A].Position)<FVector::DistSquared(C.Position,State.Pallets[B].Position);});
    for(int32 Index:Sources){auto& P=State.Pallets[Index];const auto* Slot=State.Slots.FindByPredicate([&](const auto& V){return V.Id==P.Owner;});if(!Slot||State.StorageRacks.ContainsByPredicate([&](const auto& R){return R.FinishedGoods&&R.Slots.Contains(Slot->Id);}))continue;
     int32 Q=FMath::Min3(Need,P.Quantity-P.Reserved,StationRackSupply(P.Part));Q=FMath::Min(Q,FFoundrylineAssemblySupplyCatalog::Fit(C.PayloadLb-CartLb,I->HandlingLb,C.MaxUnits-Units));Q=FMath::Min(Q,FFoundrylineAssemblySupplyCatalog::Fit(Bin->PayloadLb-BinLb,I->HandlingLb,Q));
     if(Q<=0)break;FFoundrylineCollectorLoad L;L.SourcePallet=P.Id;L.Part=S.Part;L.Quantity=Q;L.UnitLb=P.UnitLb;L.Pickup=Slot->Dock;
     C.Load.Add(L);P.Reserved+=Q;CartLb+=Q*I->HandlingLb;BinLb+=Q*I->HandlingLb;Units+=Q;Need-=Q;if(Need<=0)break;
    }
   }
   C.Retry=0;
  }
  auto* Next=C.Load.FindByPredicate([](const auto& L){return !L.Picked;});
  // Do not queue a cart on the forklift's docking point. Pick from another accessible rack end.
  if(Next&&State.Drivers.ContainsByPredicate([&](const auto& D){return FVector::Dist2D(D.Position,Next->Pickup)<250;})){
   const auto* Pallet=State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Next->SourcePallet;});
   const auto* Slot=Pallet?State.Slots.FindByPredicate([&](const auto& S){return S.Id==Pallet->Owner;}):nullptr;
   if(Slot){const FVector Outward=(Slot->Dock-Slot->Position).GetSafeNormal2D(),Tangent=FVector::CrossProduct(FVector::UpVector,Outward);
    for(float Side:{1.f,-1.f}){const FVector Candidate=Slot->Dock+Tangent*Side*280;if(State.Drivers.ContainsByPredicate([&](const auto& D){return FVector::Dist2D(D.Position,Candidate)<250;}))continue;auto Route=Path(C.Position,Candidate,true,true);if(Route.IsEmpty())continue;Next->Pickup=Candidate;C.Path=MoveTemp(Route);C.Distance=0;C.Retry=0;break;}
   }
  }
  const bool Deliver=!Next&&!C.Load.IsEmpty();const FVector Goal=Next?Next->Pickup:Deliver?Bin->Access+(Bin->Position-Bin->Access).GetSafeNormal2D()*90:C.Home+FVector(90,0,0);
  if(FVector::Dist2D(C.Position,Goal)>1){
   if(C.Path.IsEmpty()||!C.Path.Last().Equals(Goal,1)){if(C.Retry>0&&C.Path.IsEmpty())continue;C.Path=Path(C.Position,Goal,true,true);C.Distance=0;C.Retry=1;}
   if(C.Path.IsEmpty()){C.Status=TEXT("No cart route - check access");continue;}
   const float At=FMath::Min(Length(C.Path),C.Distance+90*Dt);float Yaw=C.Yaw;const FVector P=Along(C.Path,At,&Yaw);
   if(!CanEmployeeAdvance(C.Position,P,C.Id)){if(C.Retry<=0){C.Path.Reset();C.Distance=0;C.Retry=1;}C.Status=TEXT("Yielding to traffic");continue;}
   C.Speed=Dt>0?FVector::Dist2D(C.Position,P)/Dt:0;C.Position=P;C.Distance=At;C.Yaw=FMath::FixedTurn(C.Yaw,Yaw,180*Dt);
   C.Status=Next?TEXT("Collecting parts"):Deliver?TEXT("Delivering parts"):TEXT("Returning / waiting for supplies");continue;
  }
  C.Path.Reset();C.Distance=0;
  float Facing=0;if(Deliver)Facing=(Bin->Position-C.Position).Rotation().Yaw;else if(Next)if(const auto* Source=State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Next->SourcePallet;}))Facing=(Source->Position-C.Position).Rotation().Yaw;
  if(FMath::Abs(FMath::FindDeltaAngleDegrees(C.Yaw,Facing))>1){C.Yaw=FMath::FixedTurn(C.Yaw,Facing,120*Dt);C.Speed=90;C.Status=TEXT("Aligning cart");continue;}C.Yaw=Facing;
  if(Next){
   C.Handling+=Dt;C.Status=TEXT("Loading cart");if(C.Handling<1)continue;C.Handling=0;
   auto* P=State.Pallets.FindByPredicate([&](const auto& A){return A.Id==Next->SourcePallet;});
   if(P&&P->Reserved>=Next->Quantity&&P->Quantity>=Next->Quantity){P->Reserved-=Next->Quantity;P->Quantity-=Next->Quantity;Next->Picked=true;State.Pallets.RemoveAll([](const auto& A){return A.Quantity==0&&A.Reserved==0;});}
   else C.Status=TEXT("Reserved stock unavailable");
  }else if(Deliver){
   C.Handling+=Dt;C.Status=TEXT("Unloading supply bin");if(C.Handling<1)continue;C.Handling=0;
   // Preflight the whole unload so a full pallet-record budget cannot lose half a cart.
   int32 NewRecords=0;for(const auto& L:C.Load)if(!State.Pallets.ContainsByPredicate([&](const auto& P){return P.Place==Place::Buffer&&P.Owner==C.Area&&int32(P.Part)==L.Part&&FMath::IsNearlyEqual(P.UnitLb,L.UnitLb,.01f);}))++NewRecords;
   if(State.Pallets.Num()+NewRecords>64){C.Status=TEXT("Supply storage record capacity reached");continue;}
   for(const auto& L:C.Load){auto* P=State.Pallets.FindByPredicate([&](const auto& A){return A.Place==Place::Buffer&&A.Owner==C.Area&&int32(A.Part)==L.Part&&FMath::IsNearlyEqual(A.UnitLb,L.UnitLb,.01f);});
    if(P)P->Quantity+=L.Quantity;else AddPallet(Part(L.Part),L.Quantity,Place::Buffer,C.Area,Bin->Position,L.UnitLb);
   }C.Load.Reset();
  }else C.Status=Ordered?TEXT("Waiting for required parts / bin target met"):TEXT("No production order");
 }
 for(int32 Index=State.Collectors.Num()-1;Index>=0;--Index){const auto& C=State.Collectors[Index];if(C.ReleaseRequested&&C.Load.IsEmpty()&&C.Position.Equals(C.Home+FVector(90,0,0),1)){FFoundrylineStandbyDriver P;P.Id=C.Id;P.DisplayName=C.DisplayName;P.Position=C.Home;P.Home=C.Home;P.Yaw=C.Yaw;State.UnassignedEmployees.Add(P);State.Collectors.RemoveAt(Index);}}
}

FString FFoundrylineFactorySimulation::SupplyReport(int32 Area)const{
 const auto* B=State.SupplyBins.FindByPredicate([&](const auto& V){return V.Area==Area;});if(!B)return TEXT("No Assembly Supply Bin");
 FString Out=FString::Printf(TEXT("Assembly Supply Bin / %d compartments"),B->Compartments);
 const auto Items=FFoundrylineAssemblySupplyCatalog::Items();
 for(const auto& S:B->Slots){const auto* I=FFoundrylineAssemblySupplyCatalog::Find(S.Part,Items);const int Have=Available(Part(S.Part),Place::Buffer,Area);Out+=FString::Printf(TEXT("\n%s: %d / %d; incoming %d; %.0f / %.0f lb"),FFoundrylineFactoryCatalog::Part(Part(S.Part)).Name,Have,S.RefillTarget,CollectorIncoming(Area,S.Part),I?Have*I->HandlingLb:0,S.CapacityLb);}
 return Out;
}

bool FFoundrylineFactorySimulation::SupplyValid(FString& E)const{
 if(!FFoundrylineOffices::Valid(State,E))return false;
 auto Fail=[&](){E=TEXT("Invalid assembly supply, cart capacity or stock reservation");return false;};
 if(State.SupplyBins.Num()>8||State.Collectors.Num()>12)return Fail();
 if(State.OfficeDesks.Num()>32)return Fail();TSet<int32> DeskIds;for(const auto& D:State.OfficeDesks){if(D.Id<=0||D.Id>=State.NextId||DeskIds.Contains(D.Id)||D.Center.ContainsNaN()||(FMath::Abs(D.Center.Z-20)>1&&FMath::Abs(D.Center.Z-340)>1)||D.QuarterTurns<0||D.QuarterTurns>3)return Fail();DeskIds.Add(D.Id);}
 if(State.StorageRacks.Num()>8)return Fail();TSet<int32> RackIds,RackSlots;
 for(const auto& Rack:State.StorageRacks){if(Rack.Id<=0||Rack.Id>=State.NextId||RackIds.Contains(Rack.Id)||Rack.Center.ContainsNaN()||Rack.QuarterTurns<0||Rack.QuarterTurns>3||Rack.Levels<1||Rack.Levels>3||Rack.Slots.Num()!=Rack.Levels*2)return Fail();RackIds.Add(Rack.Id);for(int32 Slot:Rack.Slots){if(RackSlots.Contains(Slot)||!State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==Slot;}))return Fail();RackSlots.Add(Slot);}}
 const auto Items=FFoundrylineAssemblySupplyCatalog::Items();TSet<int32> Areas,People;
 for(const auto& B:State.SupplyBins){if(!State.WeldingStations.ContainsByPredicate([&](const auto& Z){return Z.Id==B.Area;})||Areas.Contains(B.Area)||(B.Compartments!=4&&B.Compartments!=6&&B.Compartments!=8)||B.Slots.Num()>B.Compartments||!FMath::IsFinite(B.PayloadLb)||B.PayloadLb<=0||B.PayloadLb>B.Compartments*100||B.Position.ContainsNaN()||B.Access.ContainsNaN()||!FMath::IsFinite(B.Yaw))return Fail();Areas.Add(B.Area);TSet<int32> Parts;float Weight=0;
  for(const auto& S:B.Slots){const auto* I=FFoundrylineAssemblySupplyCatalog::Find(S.Part,Items);if(!I||Parts.Contains(S.Part)||!FMath::IsFinite(S.CapacityLb)||S.CapacityLb<=0||S.CapacityLb>100||S.RefillTarget<1||S.RefillTarget>FFoundrylineAssemblySupplyCatalog::Fit(S.CapacityLb,I->HandlingLb,I->MaxUnitsPerCompartment))return Fail();Parts.Add(S.Part);const int Count=Available(Part(S.Part),Place::Buffer,B.Area)+CollectorIncoming(B.Area,S.Part);if(Count>FFoundrylineAssemblySupplyCatalog::Fit(S.CapacityLb,I->HandlingLb,I->MaxUnitsPerCompartment))return Fail();Weight+=Count*I->HandlingLb;}if(Weight>B.PayloadLb+.01)return Fail();
 }
 for(const auto& C:State.Collectors){if(!Areas.Contains(C.Area)||People.Contains(C.Id)||C.Id<=0||C.Id>=State.NextId||C.DisplayName.IsEmpty()||C.DisplayName.Len()>64||C.Position.ContainsNaN()||C.Home.ContainsNaN()||!FMath::IsFinite(C.Yaw)||!FMath::IsFinite(C.PayloadLb)||C.PayloadLb<=0||C.PayloadLb>300||C.MaxUnits<1||C.MaxUnits>100||C.Load.Num()>16||C.Path.Num()>4096||!FMath::IsFinite(C.Distance)||C.Distance<0||!FMath::IsFinite(C.Speed)||C.Speed<0||C.Speed>91||!FMath::IsFinite(C.Retry)||C.Retry<0||!FMath::IsFinite(C.Handling)||C.Handling<0||C.Handling>1.1)return Fail();People.Add(C.Id);for(const auto& P:C.Path)if(P.ContainsNaN())return Fail();float Weight=0;int32 Units=0;
  for(const auto& L:C.Load){const auto* I=FFoundrylineAssemblySupplyCatalog::Find(L.Part,Items);if(!I||L.Quantity<1||L.Pickup.ContainsNaN()||!FMath::IsFinite(L.UnitLb)||L.UnitLb<0||!CollectorSupplies(C.Area,L.Part))return Fail();Weight+=L.Quantity*I->HandlingLb;Units+=L.Quantity;if(!L.Picked&&!State.Pallets.ContainsByPredicate([&](const auto& P){return P.Id==L.SourcePallet&&P.Place==Place::Rack&&!P.OrderId&&!P.BookingId&&int32(P.Part)==L.Part&&P.Reserved>=L.Quantity&&FMath::IsNearlyEqual(P.UnitLb,L.UnitLb,.01f);}))return Fail();}
  if(Weight>C.PayloadLb+.01||Units>C.MaxUnits)return Fail();
 }
 return true;
}
