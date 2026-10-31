#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"

namespace {FBox Box(const FBox2D& B,double Z,double H){return FBox(FVector(B.Min,Z),FVector(B.Max,Z+H));}}
FBox FFoundrylinePlannerItems::MachineBounds(int32 Id,FVector P){
 const FVector Low[]={FVector(-280,-260,0),FVector(-125,-175,0),FVector(-110,-270,0)},High[]={FVector(285,260,190),FVector(225,175,280),FVector(200,270,220)};
 const int I=FMath::Clamp(Id-1,0,2);return FBox(P+Low[I],P+High[I]);
}
TArray<FFoundrylinePlannerItem> FFoundrylinePlannerItems::FixedRacks(const FFoundrylineFactoryState& S){
 TArray<FFoundrylinePlannerItem> Out;
 const TArray<TArray<int32>> Groups={{100},{107,108,109},{110,111},{101,102},{112},{103},{105,106}};
 const TCHAR* Labels[]={TEXT("RAW STEEL RACK"),TEXT("PLASMA OUTPUT RACK"),TEXT("PRESS BRAKE INPUT RACK"),TEXT("PRESS BRAKE OUTPUT RACK"),TEXT("ROLLER INPUT RACK"),TEXT("ROLLER OUTPUT RACK"),TEXT("PURCHASED PARTS RACK")};
 for(int I=0;I<Groups.Num();++I){const auto* Slot=S.Slots.FindByPredicate([&](const auto& V){return Groups[I].Contains(V.Id)&&!V.bMachineOutput;});if(!Slot)continue;
  FFoundrylinePlannerItem Item;Item.Kind=EFoundrylinePlannerItem::FixedRack;Item.Id=Groups[I][0];Item.MachineId=Slot->MachineId;Item.Name=Labels[I];Item.Center=FVector(Slot->Position.X,Slot->Position.Y,20);const FVector Delta=Slot->Dock-Slot->Position;
  Item.Quarter=(FMath::RoundToInt(Delta.Rotation().Yaw/90)+4)%4;
  if(S.bCutPartsStarter&&S.Commerce.Enabled&&I==0&&Slot->MachineId==0&&FVector::Dist2D(Slot->Dock,Slot->Position)>=155){Item.CompactRack=true;Item.Quarter=(Item.Quarter+1)%4;}
  for(int Id:Groups[I])if(S.Slots.ContainsByPredicate([&](const auto& V){return V.Id==Id;}))Item.Slots.Add(Id);
  Item.Bounds=Box(FFoundrylinePlanningEquipment::Bounds(Item.Center,Item.Quarter,Item.CompactRack?FVector(310,150,0):FVector(120,416,0)),20,Item.CompactRack?305:420);Out.Add(Item);
 }
 for(const auto& M:S.Machines)if(!M.bStored){
  for(bool Input:{true,false}){TArray<const FFoundrylineFactorySlot*> Slots;for(const auto& Slot:S.Slots)if(Slot.MachineId==M.Id&&(Input?Slot.bInputSupply:Slot.bMachineOutput)&&!Out.ContainsByPredicate([&](const auto& R){return R.Slots.Contains(Slot.Id);}))Slots.Add(&Slot);if(Slots.IsEmpty())continue;
   FFoundrylinePlannerItem Item;Item.Kind=EFoundrylinePlannerItem::FixedRack;Item.Id=Slots[0]->Id;Item.MachineId=M.Id;Item.CompactRack=true;Item.Quarter=(M.QuarterTurns+(Item.CompactRack?1:0))%4;
   Item.Name=Input?(M.Kind()==1?TEXT("PLASMA INPUT / STEEL SHEETS"):TEXT("MACHINE INPUT RACK")):TEXT("MACHINE OUTPUT RACK / 4 PALLETS");
   FVector Center=FVector::ZeroVector;int Floor=0;double Low=MAX_dbl;for(auto* Slot:Slots)Low=FMath::Min(Low,Slot->Position.Z);
   for(auto* Slot:Slots){Item.Slots.Add(Slot->Id);if(FMath::IsNearlyEqual(Slot->Position.Z,Low,.1)){Center+=Slot->Position;++Floor;}}
   Item.Center=Center/FMath::Max(1,Floor);Item.Center.Z=20;
   Item.Bounds=Box(FFoundrylinePlanningEquipment::Bounds(Item.Center,Item.Quarter,Item.CompactRack?FVector(310,150,0):FVector(120,416,0)),20,Item.CompactRack?305:420);Out.Add(Item);
  }
 }
 return Out;
}
TArray<FFoundrylinePlannerItem> FFoundrylinePlannerItems::List(const FFoundrylineFactoryState& S){
 auto Out=FixedRacks(S);
 for(const auto& D:S.OfficeDesks){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::OfficeDesk;V.Id=D.Id;V.Name=TEXT("OFFICE WORKSTATION");V.Center=D.Center;V.Quarter=D.QuarterTurns;V.Bounds=Box(FFoundrylinePlanningEquipment::Bounds(D.Center+FRotator(0,D.QuarterTurns*90,0).RotateVector(FVector(0,40,0)),D.QuarterTurns,FFoundrylinePlanningEquipment::Size(7)),D.Center.Z,150);Out.Add(V);}
 for(const auto& M:S.Machines){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::Machine;V.Id=M.Id;V.Quarter=M.QuarterTurns;V.Name=M.Kind()==1?TEXT("PLASMA TABLE"):M.Kind()==2?TEXT("PRESS BRAKE"):TEXT("SHEET ROLLER");V.Center=M.Origin;V.Bounds=MachineBounds(M.Kind(),FVector::ZeroVector).TransformBy(FTransform(FRotator(0,M.QuarterTurns*90,0),M.Origin));V.Stored=M.bStored;Out.Add(V);}
 for(const auto& R:S.StorageRacks){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::StorageRack;V.Id=R.Id;V.Name=R.FinishedGoods?TEXT("FINISHED GOODS RACK"):TEXT("PARTS RACK");V.Center=R.Center;V.Quarter=R.QuarterTurns;V.Slots=R.Slots;V.Bounds=Box(FFoundrylinePlanningEquipment::Bounds(R.Center,R.QuarterTurns,FFoundrylinePlanningEquipment::Size(R.FinishedGoods?2:1,R.Levels)),20,R.Levels*152.4);Out.Add(V);}
 for(const auto& S2:S.WeldingStations){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::Station;V.Id=S2.Id;V.Name=TEXT("FABRICATION STATION");V.Center=S2.Center;V.Quarter=S2.QuarterTurns;V.Bounds=Box(FFoundrylineWeldingStationLayout::Bounds(S2),20,180);Out.Add(V);}
 for(const auto& Z:S.Zones){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::WorkZone;V.Id=Z.Id;V.Name=TEXT("PRODUCT WORK ZONE");V.Center=(Z.Min+Z.Max)/2;V.Center.Z=20;V.Quarter=Z.QuarterTurns;V.Bounds=Box(FFoundrylineWorkZoneLayout::Bounds(Z),20,120);Out.Add(V);}
 for(const auto& D:S.Drivers){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::Forklift;V.Id=D.Id;V.Name=TEXT("FORKLIFT");V.Center=D.Position;V.Quarter=(FMath::RoundToInt(D.Yaw/90)+4)%4;V.Bounds=Box(FFoundrylinePlanningEquipment::Bounds(D.Position+FRotator(0,D.Yaw,0).RotateVector({-95,0,0}),V.Quarter,{350,180,0}),20,270);Out.Add(V);}
 for(const auto& P:S.Pallets)if(P.Place!=EFoundrylinePalletPlace::InTransit&&P.Place!=EFoundrylinePalletPlace::Forks&&P.Place!=EFoundrylinePalletPlace::Inbound&&P.Place!=EFoundrylinePalletPlace::Outbound){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::Pallet;V.Id=P.Id;V.Name=FString::Printf(TEXT("PALLET / %s"),FFoundrylineFactoryCatalog::Part(P.Part).Name);V.Center=P.Position;const FVector Half=P.Part==EFoundrylinePart::Sheet?FVector(152.4,76.2,25):P.Part==EFoundrylinePart::HeadacheRack?FVector(110,110,55):FVector(58,59,45);V.Bounds=FBox(P.Position-FVector(Half.X,Half.Y,0),P.Position+Half*FVector(1,1,2));Out.Add(V);}
 for(const auto& L:S.Lanes)if(L.Kind==EFoundrylineFloorLane::Forklift){FFoundrylinePlannerItem V;V.Kind=EFoundrylinePlannerItem::Lane;V.Id=L.Id;V.Name=TEXT("FORKLIFT AREA");V.Center=FVector((L.Min+L.Max)/2,20);V.Bounds=Box(FBox2D(L.Min,L.Max),20,4);Out.Add(V);}
 return Out;
}
