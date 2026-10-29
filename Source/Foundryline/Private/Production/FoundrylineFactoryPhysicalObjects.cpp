#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylineEquipmentCollision.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
void AFoundrylineFactoryPlant::UpdatePhysicalObjects(float Dt){
 if(ForkPhysicalBodies.IsEmpty())for(int I=0;I<Forks.Num();++I){
  auto Add=[&](const TCHAR* N,FVector P,FVector E,USceneComponent* Parent){ForkPhysicalBodies.Add(FFoundrylineEquipmentCollision::AddBox(this,Parent,FName(*FString::Printf(TEXT("Fork%d_%s"),I,N)),P,E));};
  Add(TEXT("Chassis"),{-130,0,72},{125,72,56},Forks[I]);
  for(int Side:{-1,1})Add(TEXT("MastPost"),{28,Side*52.,142},{7,7,126},Forks[I]);
  for(int X:{-175,-15})for(int Side:{-1,1})Add(TEXT("CabPost"),{double(X),Side*62.,185},{3,3,65},Forks[I]);
  Add(TEXT("Roof"),{-95,0,252},{100,70,8},Forks[I]);
 }
 for(int I=0;I<Forks.Num();++I){bool Active=Snapshot.Drivers.IsValidIndex(I);for(int J=I*8;J<(I+1)*8&&J<ForkPhysicalBodies.Num();++J)ForkPhysicalBodies[J]->SetCollisionEnabled(Active?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  if(Active&&GetNetMode()==NM_DedicatedServer){const auto& D=Snapshot.Drivers[I];Forks[I]->SetRelativeLocationAndRotation(D.Position,FRotator(0,D.Yaw,0));float H=12;const auto* J=Snapshot.Jobs.FindByPredicate([&](const auto& V){return V.Id==D.Job;});if(J&&(J->Phase==2||J->Phase==4)){if(J->Phase==4)H=J->StoredAt.Z-20;else if(const auto* P=Snapshot.Pallets.FindByPredicate([&](const auto& V){return V.Id==J->SourcePallet;}))H=P->Position.Z-20;}Carriages[I]->SetRelativeLocation({28,0,16+H});}
 }
 static TArray<FBox> Bounds;static FBox Wood(ForceInit);
 if(Bounds.IsEmpty()){auto Load=[](const FString& Path){auto* M=LoadObject<UStaticMesh>(nullptr,*Path);return M?M->GetBoundingBox():FBox(FVector(-1),FVector(1));};Wood=Load(TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood"));for(int I=0;I<FFoundrylineFactoryCatalog::PartCount;++I)Bounds.Add(Load(FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart(I))));}
 TSet<int32> Live;
 for(const auto& P:Snapshot.Pallets){if(P.Quantity<=0||P.Place==EFoundrylinePalletPlace::Inbound||P.Place==EFoundrylinePalletPlace::Outbound||P.Place==EFoundrylinePalletPlace::InTransit)continue;int Part=int(P.Part);if(!Bounds.IsValidIndex(Part))continue;FVector Pos=P.Position;FRotator Rot=FRotator::ZeroRotator;bool Carried=P.Place==EFoundrylinePalletPlace::Forks;
  // Station bins and the OUT pallets have their own fixed collision; a pallet waiting on IN and each OUT stack need a body here.
  const auto* Station=P.Place==EFoundrylinePalletPlace::Buffer||P.Place==EFoundrylinePalletPlace::StationIn||P.Place==EFoundrylinePalletPlace::StationOut?Snapshot.WeldingStations.FindByPredicate([&](const auto& S){return S.Id==P.Owner;}):nullptr;
  if(Station&&P.Place==EFoundrylinePalletPlace::StationOut){using Layout=FFoundrylineWeldingStationLayout;const int Slot=Layout::OutSlot(*Station,P.Position);if(Slot<0)continue;
   const FVector R=Layout::RackSizeCm();const FBox Stack(FVector(-R.X/2,-R.Y/2,Layout::PalletTopCm),FVector(R.X/2,R.Y/2,Layout::PalletTopCm+P.Quantity*R.Z));const FTransform T=Layout::OutStackTransform(*Station,Slot);
   Live.Add(P.Id);auto& C=PalletPhysicalBodies.FindOrAdd(P.Id);if(!C)C=FFoundrylineEquipmentCollision::AddBox(this,RootComponent,FName(*FString::Printf(TEXT("Pallet%d_Physical"),P.Id)),FVector::ZeroVector,Stack.GetExtent());
   C->SetRelativeLocationAndRotation(T.TransformPosition(Stack.GetCenter()),T.Rotator());C->SetBoxExtent(Stack.GetExtent());C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);continue;}
  if(Station&&P.Place!=EFoundrylinePalletPlace::StationIn)continue;if(Station)Rot=FFoundrylineWeldingStationLayout::Rotation(*Station);

  if(Carried){int DI=Snapshot.Drivers.IndexOfByPredicate([&](const auto& D){return D.Id==P.Owner;});if(!Forks.IsValidIndex(DI))continue;auto* R=Forks[DI].Get();Rot=R->GetRelativeRotation();Pos=R->GetRelativeLocation()+Rot.RotateVector({136,0,Carriages[DI]->GetRelativeLocation().Z-16});}
  else {if(P.Place==EFoundrylinePalletPlace::Rack&&(P.Owner==105||P.Owner==106))Rot.Yaw=90;if(P.Place==EFoundrylinePalletPlace::Buffer)if(const auto* Z=Snapshot.Zones.FindByPredicate([&](const auto& V){return V.Id==P.Owner;}))Rot=FFoundrylineWorkZoneLayout::Rotation(*Z);}
  FBox B(ForceInit);auto Union=[&](FBox Box,FVector Offset){B+=FBox(Box.Min+Offset,Box.Max+Offset);};if(!Carried&&(Part==0||Part==2||Part==3||Part==4||Part==5||Part==6)){Union(Wood,{0,-65,0});Union(Wood,{0,65,0});}else Union(Wood,FVector::ZeroVector);
  int Count=FMath::Min(P.Quantity,Carried?3:Part==0?3:Part==1||Part==9||Part==10||Part==11?10:Part==6?2:3);int Cap=FFoundrylineFactoryCatalog::Part(P.Part).Capacity;if(!Carried&&Cap>10)Count=FMath::Max(1,FMath::CeilToInt(10.f*P.Quantity/Cap));
  for(int I=0;I<Count;++I){FVector O(0,0,18);if(Carried)O.Z+=I*(Part==6?48:Part==0?.65:12);else if(Part==1||Part==9||Part==10||Part==11)O+=FVector((I%5-2)*27,(I/5-.5)*40,(I/10)*14);else O.Z+=I*(Part==0?.65:Part==5?7:Part==6?48:Part==3?12:Part==7?48:Part==8?35:1.2);Union(Bounds[Part],O);}
  Live.Add(P.Id);auto& C=PalletPhysicalBodies.FindOrAdd(P.Id);if(!C)C=FFoundrylineEquipmentCollision::AddBox(this,RootComponent,FName(*FString::Printf(TEXT("Pallet%d_Physical"),P.Id)),FVector::ZeroVector,B.GetExtent());C->SetRelativeLocationAndRotation(Pos+Rot.RotateVector(B.GetCenter()),Rot);C->SetBoxExtent(B.GetExtent());C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 }
 for(auto It=PalletPhysicalBodies.CreateIterator();It;++It)if(!Live.Contains(It.Key())){if(It.Value())It.Value()->DestroyComponent();It.RemoveCurrent();}
}
