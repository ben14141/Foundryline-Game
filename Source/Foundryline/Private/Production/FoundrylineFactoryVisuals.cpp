#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineMachineMotion.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "GameFramework/GameStateBase.h"
namespace {
 UStaticMesh* Mesh(const FString& Path){return LoadObject<UStaticMesh>(nullptr,*Path);}
 FString Asset(const FString& Folder,const FString& Name){return TEXT("/Game/Foundryline/")+Folder+TEXT("/SM_")+Name;}
}
void AFoundrylineFactoryPlant::SetupVisuals(){
 for(TActorIterator<AFoundrylinePlasmaTable> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Plasma=*It;
 for(TActorIterator<AFoundrylineMaterialHandlingCell> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Cell=*It;
 for(TActorIterator<AFoundrylinePressBrakeDisplay> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Brake=*It;
 // Equipment footprints feed path validation as well as the placement overlay.
 Model.Solids=bCutPartsStarter?FFoundrylineWorkshopLayout::CutShopSolids():FFoundrylineWorkshopLayout::Solids();
 // Rack collision is authoritative even when a dedicated server has no renderer.
 for(const auto& Rack:(bCutPartsStarter?FFoundrylineWorkshopLayout::CutShopRacks():FFoundrylineWorkshopLayout::Racks())){const FRotator Rot(0,Rack.Yaw,0);auto* C=NewObject<UBoxComponent>(this);C->SetupAttachment(RootComponent);C->SetRelativeLocationAndRotation(Rack.Center+FVector(0,0,210),Rot);C->SetBoxExtent({60,208,210});C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->SetCollisionProfileName(TEXT("BlockAllDynamic"));C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);}
 for(int I=0;I<4;++I){auto* R=NewObject<USceneComponent>(this);R->SetupAttachment(RootComponent);R->RegisterComponent();AddInstanceComponent(R);Forks.Add(R);auto* Car=NewObject<USceneComponent>(this);Car->SetupAttachment(R);Car->SetRelativeLocation({28,0,16});Car->RegisterComponent();AddInstanceComponent(Car);Carriages.Add(Car);}
 if(GetNetMode()==NM_DedicatedServer)return;
 auto Instances=[&](const FString& Path){auto* C=NewObject<UInstancedStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh(Path));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);return C;};
 Pallets=Instances(Asset(TEXT("Inventory/Storage/Pallets"),TEXT("Pallet_Wood")));
 CargoPallets=Instances(Asset(TEXT("Inventory/Storage/Pallets"),TEXT("Pallet_Wood")));
 WeldingMachines=Instances(Asset(TEXT("Production/Equipment/Welder"),TEXT("Welder_Machine")));
 SetupFloorVisuals();
 for(int I=0;I<FFoundrylineFactoryCatalog::PartCount;++I){const auto P=FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart(I));PartMeshes.Add(Instances(P));CargoMeshes.Add(Instances(P));}
 auto Add=[&](const FString& Path,USceneComponent* Parent,FVector P){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Parent);C->SetStaticMesh(Mesh(Path));C->SetRelativeLocation(P);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);return C;};
 for(const auto& Rack:(bCutPartsStarter?FFoundrylineWorkshopLayout::CutShopRacks():FFoundrylineWorkshopLayout::Racks())){
  const FRotator Rot(0,Rack.Yaw,0);
  auto RackMesh=[&](const TCHAR* Name,FVector Offset){auto* C=Add(Asset(TEXT("Inventory/Storage/SheetRacking"),Name),RootComponent,Rack.Center+Rot.RotateVector(Offset));C->SetRelativeRotation(Rot);};
  for(int Side:{-1,1})RackMesh(TEXT("Rack_Upright"),FVector(0,Side*201.735,0));
  for(int Level=0;Level<3;++Level)RackMesh(TEXT("Rack_Shelf400"),FVector(0,0,20+Level*130));
 }
 for(int I=0;I<4;++I){auto* R=Forks[I].Get();
  Add(Asset(TEXT("Vehicles/Forklift/FL01"),TEXT("Forklift_Body")),R,FVector::ZeroVector);
  Add(Asset(TEXT("Vehicles/Forklift/FL01"),TEXT("Forklift_MastOuter")),R,FVector(28,0,16));
  auto* Car=Carriages[I].Get();
  Add(Asset(TEXT("Vehicles/Forklift/FL01"),TEXT("Forklift_Carriage")),Car,FVector::ZeroVector);Add(Asset(TEXT("Vehicles/Forklift/FL01"),TEXT("Forklift_MastInner")),Car,FVector::ZeroVector);
  const TCHAR* W[]={TEXT("FL"),TEXT("RL"),TEXT("FR"),TEXT("RR")};const FVector P[]={{0,59.59,41.84},{-196.67,56.81,35.66},{0,-59.59,41.79},{-197.33,-58.44,34.75}};
  for(int J=0;J<4;++J)ForkWheels.Add(Add(Asset(TEXT("Vehicles/Forklift/FL01"),FString(TEXT("Forklift_Wheel_"))+W[J]),R,P[J]));
 }
}
void AFoundrylineFactoryPlant::RebuildVisuals(){
 if(!Pallets)return;Pallets->ClearInstances();for(auto C:PartMeshes)C->ClearInstances();RebuildFloor();
 using StationLayout=FFoundrylineWeldingStationLayout;auto StationOf=[&](const FFoundrylineFactoryPallet& P){return P.Place==EFoundrylinePalletPlace::Buffer||P.Place==EFoundrylinePalletPlace::StationIn||P.Place==EFoundrylinePalletPlace::StationOut?Snapshot.WeldingStations.FindByPredicate([&](const auto& S){return S.Id==P.Owner;}):nullptr;};
 for(const auto& P:Snapshot.Pallets){if(P.Place==EFoundrylinePalletPlace::Forks||P.Place==EFoundrylinePalletPlace::InTransit||P.Place==EFoundrylinePalletPlace::Inbound||P.Place==EFoundrylinePalletPlace::Outbound)continue;
  // Station bins and OUT stacks are drawn below (the OUT pallets belong to the station); a delivery on IN is a normal pallet.
  const auto* Station=StationOf(P);if(Station&&P.Place!=EFoundrylinePalletPlace::StationIn)continue;
  FVector Pos=P.Position;const int Part=int(P.Part),Cap=FFoundrylineFactoryCatalog::Part(P.Part).Capacity;FRotator Rot=Station?StationLayout::Rotation(*Station):FRotator::ZeroRotator;
  // Long stock is authored along Y, matching the machine rack shelves.
  if(P.Place==EFoundrylinePalletPlace::Rack&&(P.Owner==105||P.Owner==106))Rot.Yaw=90;
  if(P.Place==EFoundrylinePalletPlace::Buffer)if(const auto* Z=Snapshot.Zones.FindByPredicate([&](const auto& V){return V.Id==P.Owner;}))Rot=FFoundrylineWorkZoneLayout::Rotation(*Z);
  // Finished headache racks: one pallet with a flat stack, its far end running past the pallet away from the side the
  // forklift serves it from (rack slots face their dock; pickup pallets face the dock 136 cm in -X).
  if(P.Part==EFoundrylinePart::HeadacheRack){if(P.Place==EFoundrylinePalletPlace::Rack)if(const auto* Slot=Snapshot.Slots.FindByPredicate([&](const auto& V){return V.Id==P.Owner;}))Rot=FRotator(0,(Pos-Slot->Dock).Rotation().Yaw,0);
   Pallets->AddInstance(FTransform(Rot,Pos));for(int Level=0;Level<P.Quantity;++Level)PartMeshes[Part]->AddInstance(StationLayout::RackOnPallet(Level)*FTransform(Rot,Pos));continue;}
  if(Part==0||Part==2||Part==3||Part==4||Part==5||Part==6){for(int Side:{-1,1})Pallets->AddInstance(FTransform(Rot,Pos+Rot.RotateVector(FVector(0,Side*65,0))));}else Pallets->AddInstance(FTransform(Rot,Pos));
  int Count=FMath::Min(P.Quantity,Part==0?3:Part==1||Part==9||Part==10||Part==11?10:Part==6?2:3);if(Cap>10)Count=FMath::Max(1,FMath::CeilToInt(10.f*P.Quantity/Cap));
  for(int I=0;I<Count;++I){FVector Offset(0,0,18);if(Part==1||Part==9||Part==10||Part==11)Offset+=FVector((I%5-2)*27,(I/5-.5)*40,(I/10)*14);else if(Part==int(EFoundrylinePart::FrameSection))Offset+=FVector(0,(I%5-2)*14,(I/5)*10);else Offset.Z+=I*(Part==0?.65:Part==5?7:Part==6?48:Part==3?12:Part==7?48:Part==8||Part==int(EFoundrylinePart::LightKit)?35:1.2);
   PartMeshes[Part]->AddInstance(FTransform(Rot,Pos+Rot.RotateVector(Offset)));}
 }
 // Representative station stock: each item scaled down (never up) to fit its bin cell, long stock along the station length.
 for(const auto& P:Snapshot.Pallets){const auto* S=StationOf(P);if(!S||P.Place==EFoundrylinePalletPlace::StationIn||!PartMeshes.IsValidIndex(int(P.Part)))continue;auto* Ism=PartMeshes[int(P.Part)].Get();if(!Ism||!Ism->GetStaticMesh())continue;
  const FBox B=Ism->GetStaticMesh()->GetBoundingBox();const FRotator Yaw=StationLayout::Rotation(*S);
  if(P.Place==EFoundrylinePalletPlace::StationOut){const int Slot=StationLayout::OutSlot(*S,P.Position);if(Slot>=0)for(int Level=0;Level<P.Quantity;++Level)Ism->AddInstance(StationLayout::RackOnOut(*S,Slot,Level));continue;}
  const bool Frames=P.Part==EFoundrylinePart::FrameSection,Plates=P.Part==EFoundrylinePart::MountingPlate;const FVector BinCell=Frames?FVector(120,10,10):Plates?FVector(36,30,4):FVector(16,14,13);
  const bool Turn=B.GetSize().Y>B.GetSize().X;const FVector Size=Turn?FVector(B.GetSize().Y,B.GetSize().X,B.GetSize().Z):B.GetSize();
  const float Scale=FMath::Min(1.f,float(FMath::Min3(BinCell.X/FMath::Max(1.,Size.X),BinCell.Y/FMath::Max(1.,Size.Y),BinCell.Z/FMath::Max(1.,Size.Z))));const FRotator ItemYaw(0,Yaw.Yaw+(Turn?90:0),0);
  for(int I=0;I<P.Quantity;++I){FVector Offset=Frames?FVector(0,(I%4-1.5)*11,(I/4)*Size.Z*Scale):Plates?FVector(0,0,I*BinCell.Z):FVector((I%2-.5)*18,(I/2%2-.5)*15,(I/4)*BinCell.Z);
   const FVector At=P.Position+Yaw.RotateVector(Offset)-FVector(0,0,B.Min.Z*Scale);Ism->AddInstance(FTransform(ItemYaw,At,FVector(Scale)));}
 }

 AddStationFitUpParts();
 // Recreate product components only if the number of zones changes, not each snapshot.
 FString Layout;for(const auto& Z:Snapshot.Zones)Layout+=FString::Printf(TEXT("%d:%s:%s:%d:%d;"),Z.Id,*Z.Min.ToString(),*Z.Max.ToString(),Z.QuarterTurns,Z.SupplySide);
 if(ProductZones.Num()!=Snapshot.Zones.Num()*26||VisualLayoutKey!=Layout){VisualLayoutKey=Layout;for(auto C:ProductMeshes)C->DestroyComponent();ProductMeshes.Reset();ProductMaterials.Reset();ProductZones.Reset();ProductParts.Reset();ProductThresholds.Reset();for(auto E:WeldEffects)E->DestroyComponent();WeldEffects.Reset();
  for(int ZI=0;ZI<Snapshot.Zones.Num();++ZI){const auto& Z=Snapshot.Zones[ZI];const FRotator ZoneRotation=FFoundrylineWorkZoneLayout::Rotation(Z);const FVector Origin=FFoundrylineWorkZoneLayout::ProductCenter(Z)+FFoundrylineWorkZoneLayout::Rotation(Z).RotateVector(FVector(404,0,0));
   auto AddProduct=[&](const FString& Path,FVector P,FRotator Rot,int Part,float Threshold){auto* SM=Mesh(Path);if(!SM)return;for(int Ghost=0;Ghost<2;++Ghost){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(SM);C->SetRelativeLocationAndRotation(Origin+ZoneRotation.RotateVector(P),(ZoneRotation.Quaternion()*Rot.Quaternion()).Rotator());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);ProductMeshes.Add(C);ProductZones.Add(ZI);ProductParts.Add(Part);ProductThresholds.Add(Threshold);
     for(int Slot=0;Slot<SM->GetStaticMaterials().Num();++Slot){FString Suffix=TEXT("Palette");if(auto* Mat=SM->GetMaterial(Slot)){const FString N=Mat->GetName();if(N.Contains(TEXT("Rubber")))Suffix=TEXT("Rubber");else if(N.Contains(TEXT("Red")))Suffix=TEXT("Red");else if(N.Contains(TEXT("White")))Suffix=TEXT("White");else if(N.Contains(TEXT("Metal"))||N.Contains(TEXT("Steel")))Suffix=TEXT("Metal");}
      auto* Base=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Foundryline/Production/Materials/")+FString(Ghost?TEXT("M_FactoryHologram"):TEXT("M_FactoryBuild"))+ (Ghost?TEXT(""):*Suffix)));if(!Base)continue;auto* MID=UMaterialInstanceDynamic::Create(Base,this);C->SetMaterial(Slot,MID);MID->SetVectorParameterValue(TEXT("BuildOrigin"),FLinearColor(Origin));MID->SetVectorParameterValue(TEXT("BuildAxis"),FLinearColor(0,0,1));C->UpdateBounds();const auto Bounds=C->Bounds;MID->SetScalarParameterValue(TEXT("BuildLow"),Bounds.Origin.Z-Bounds.BoxExtent.Z-Origin.Z);MID->SetScalarParameterValue(TEXT("BuildHigh"),Bounds.Origin.Z+Bounds.BoxExtent.Z-Origin.Z);ProductMaterials.Add(MID);
     }
    }};
   AddProduct(Asset(TEXT("Products/Tankers/StraightBarrel/ProductionStages"),TEXT("ProductFrame")),{}, {},3,0);
   AddProduct(Asset(TEXT("Products/Tankers/StraightBarrel/ProductionStages"),TEXT("ProductShell")),{}, {},5,0);
   AddProduct(Asset(TEXT("Products/Tankers/StraightBarrel/ProductionStages"),TEXT("ProductFittings")),{}, {},8,0);
   AddProduct(Asset(TEXT("Products/Components/DishedHeads"),TEXT("TankHead_ASME_FD")),{86.38,0,244.9},FRotator(90,0,0),6,1);
   AddProduct(Asset(TEXT("Products/Components/DishedHeads"),TEXT("TankHead_ASME_FD")),{-1015.25,0,244.9},FRotator(-90,0,0),6,2);
   const FVector WheelPos[]={{-714.759,95.223,45.531},{-928.792,-95.592,45.531},{-821.776,-95.592,45.531},{-928.792,95.223,45.531},{-821.776,95.223,45.531},{-714.759,-95.592,45.531}};
   for(int I=0;I<6;++I)AddProduct(Asset(TEXT("Products/Tankers/StraightBarrel"),FString::Printf(TEXT("Tanker_Wheel_%d"),I)),WheelPos[I],{},7,1);
   for(int I=0;I<2;++I)AddProduct(Asset(TEXT("Products/Tankers/StraightBarrel"),FString::Printf(TEXT("Tanker_Leg_%d"),I)),{-263.9,I? -85.815:85.815,63.34},{},3,12);
  }
  RebuildZoneWeldingEquipment();
 }
 const auto Bill=FFoundrylineFactoryCatalog::TankerBill();
 for(int I=0;I<ProductMeshes.Num();++I){const auto& Z=Snapshot.Zones[ProductZones[I]];const int P=ProductParts[I];float F=Z.Installed.IsValidIndex(P)?float(Z.Installed[P])/FMath::Max(1,Bill[P]):0;if(ProductThresholds[I]>0)F=Z.Installed[P]>=ProductThresholds[I]?1:0;
  auto C=ProductMeshes[I];C->SetVisibility(true);for(int J=0;J<C->GetNumMaterials();++J)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(J)))M->SetScalarParameterValue(TEXT("BuildFraction"),F<=0?-1:F>=1?2:F);
 }
}
void AFoundrylineFactoryPlant::Animate(float Dt){
 // Replicated equipment can arrive after the plant on clients and late joins.
 if(!IsValid(Plasma))for(TActorIterator<AFoundrylinePlasmaTable> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Plasma=*It;
 if(!IsValid(Brake))for(TActorIterator<AFoundrylinePressBrakeDisplay> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Brake=*It;
 if(!IsValid(Cell))for(TActorIterator<AFoundrylineMaterialHandlingCell> It(GetWorld());It;++It)if(It->BusinessCompanyId==BusinessCompanyId&&It->BusinessPlantId==BusinessPlantId)Cell=*It;
 for(int I=0;I<Forks.Num();++I){auto R=Forks[I];R->SetVisibility(Snapshot.Drivers.IsValidIndex(I),true);if(!Snapshot.Drivers.IsValidIndex(I))continue;const auto& D=Snapshot.Drivers[I];auto& P=DisplayPositions.FindOrAdd(D.Id,D.Position);P=FMath::VInterpTo(P,D.Position,Dt,10);auto& Y=DisplayYaw.FindOrAdd(D.Id,D.Yaw);Y+=FMath::FindDeltaAngleDegrees(Y,D.Yaw)*FMath::Min(1.f,Dt*10);R->SetRelativeLocationAndRotation(P,FRotator(0,Y,0));float Lift=12;
  const auto* Job=Snapshot.Jobs.FindByPredicate([&](const auto& J){return J.Id==D.Job;});if(Job&&(Job->Phase==2||Job->Phase==4)){if(Job->Phase==4)Lift=Job->StoredAt.Z-20;else if(const auto* Source=Snapshot.Pallets.FindByPredicate([&](const auto& A){return A.Id==Job->SourcePallet;}))Lift=Source->Position.Z-20;}
  auto& H=DisplayLift.FindOrAdd(D.Id,12);H=FMath::FInterpTo(H,Lift,Dt,3);Carriages[I]->SetRelativeLocation({28,0,16+H});
  float AheadYaw=D.Yaw;FFoundrylineFactorySimulation::Along(D.Path,D.Distance+80,&AheadYaw);const float RearSteer=-FMath::Clamp(FMath::FindDeltaAngleDegrees(D.Yaw,AheadYaw)*1.25f,-45.f,45.f);
  for(int J=0;J<4;++J)ForkWheels[I*4+J]->SetRelativeRotation(FRotator(FMath::Fmod(D.Distance/(J%2?34.74:41.06)*-57.2958,360.f),J%2?RearSteer:0,0));
  if(bLocalOverlay){DrawDebugDirectionalArrow(GetWorld(),P+FVector(0,0,230),P+FRotator(0,Y,0).Vector()*250+FVector(0,0,230),50,D.Waiting?FColor::Red:FColor::Cyan,false,0,0,5);for(int J=1;J<D.Path.Num();++J)DrawDebugLine(GetWorld(),D.Path[J-1]+FVector(0,0,10),D.Path[J]+FVector(0,0,10),FColor::Cyan,false,0,0,3);}
 }
 CargoPallets->ClearInstances();for(auto C:CargoMeshes)C->ClearInstances();
 for(const auto& C:Snapshot.Pallets)if(C.Place==EFoundrylinePalletPlace::Forks){const auto* D=Snapshot.Drivers.FindByPredicate([&](const auto& X){return X.Id==C.Owner;});if(!D)continue;const FVector P=DisplayPositions.FindRef(D->Id);const float Y=DisplayYaw.FindRef(D->Id),H=DisplayLift.FindRef(D->Id);const FTransform T(FRotator(0,Y,0),P+FRotator(0,Y,0).RotateVector(FVector(136,0,H)));CargoPallets->AddInstance(T);
  if(C.Part==EFoundrylinePart::HeadacheRack){for(int Level=0;Level<C.Quantity;++Level)CargoMeshes[int(C.Part)]->AddInstance(FFoundrylineWeldingStationLayout::RackOnPallet(Level)*T);continue;}
  for(int I=0;I<FMath::Min(3,C.Quantity);++I){FTransform Item(FVector(0,0,18+I*(C.Part==EFoundrylinePart::EndCap?48:C.Part==EFoundrylinePart::Sheet?.65:12)));CargoMeshes[int(C.Part)]->AddInstance(Item*T);}}
 auto* GS=GetWorld()->GetGameState();const double Age=(GS?GS->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds())-PresentationEpoch;
 for(int I=0;I<WeldEffects.Num();++I){const auto& Z=Snapshot.Zones[I];int AtWork=0;for(const auto& W:Snapshot.Walkers)if(W.Zone==Z.Id&&Z.Ordered&&FVector::Dist2D(W.Position,FFoundrylineWorkZoneLayout::WorkPoint(Z))<=1)++AtWork;const bool Working=AtWork&&Snapshot.Running&&Z.ActiveStep>=0&&Z.ActiveStep!=5;
  WeldingGuns[I]->SetVisibility(Working);IdleWeldingGuns[I]->SetVisibility(!Working);IdleWeldingLeads[I]->SetVisibility(!Working);for(int J=I*5;J<I*5+5;++J)WeldingLeads[J]->SetVisibility(Working);WeldEffects[I]->SetLocalActivity(Working);
  const auto R=FFoundrylineWorkZoneLayout::Rotation(Z);const double Time=Z.Work+(Working?FMath::Clamp(Age,0.,.5)*Snapshot.Speed*AtWork:0);const FVector Offset=Z.ActiveStep>=0&&Z.ActiveStep!=5?R.RotateVector(FVector(12*FMath::Sin(Time*.65),0,1.5*FMath::Sin(Time*2.6))):FVector::ZeroVector;
  const auto GunR=WeldingGuns[I]->GetRelativeRotation();const FVector Socket=WeldingGuns[I]->GetSocketTransform(TEXT("WeldTip"),RTS_Component).GetLocation();const FVector Target=FFoundrylineWorkZoneLayout::WeldPoint(Z)+Offset-GunR.RotateVector(Socket);
  const FVector Position=FMath::VInterpTo(WeldingGuns[I]->GetRelativeLocation(),Target,Dt,18);if(!Position.Equals(WeldingGuns[I]->GetRelativeLocation(),.001)){WeldingGuns[I]->SetRelativeLocation(Position);if(WeldingLeads.IsValidIndex(I*5+4))WeldingLeads[I*5+4]->SetEndPosition(Position);}
  for(const auto& Employee:CachedEmployees)if(Employee.Role==EFoundrylineEmployeeRole::Welder&&Employee.Assignment==Z.Id)if(auto* Visual=EmployeeVisuals.FindRef(Employee.Key).Get())Visual->PresentWeldingGrip(WeldingGuns[I]->GetComponentLocation(),WeldingGuns[I]->GetComponentTransform().TransformPosition(FFoundrylineWeldingStationLayout::GunSupportCm()));
 }
 AnimateWeldingStations(Dt,Age);
 for(const auto& M:Snapshot.Machines){const bool Work=M.Remaining>0,Active=FFoundrylineMachineMotion::Active(Snapshot,M);const float Phase=FFoundrylineMachineMotion::Phase(M.Remaining,M.Duration,Active,Snapshot.Speed,Age);
  if(M.Id==1&&Plasma)Plasma->SetFactoryPresentation(Phase,Active,Work,Work||M.BedLb>0,M.Making);
  if(M.Id==2&&Brake)Brake->SetFactoryPresentation(Phase,Active,Work,M.Making);
  if(M.Id==3&&Cell)Cell->SetFactoryRoller(Phase,Active,Work);
 }


}
