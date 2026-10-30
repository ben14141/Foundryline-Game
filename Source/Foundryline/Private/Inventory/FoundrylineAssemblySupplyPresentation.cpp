#include "Production/FoundrylineFactoryPlant.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"

void AFoundrylineFactoryPlant::UpdateSupplyVisuals(float Dt){
 if(GetNetMode()==NM_DedicatedServer)return;
 const bool RefreshContents=bRebuild;
 auto Contents=[&](USceneComponent* Parent){UInstancedStaticMeshComponent* Mesh=nullptr;for(USceneComponent* Child:Parent->GetAttachChildren())if(auto* Existing=Cast<UInstancedStaticMeshComponent>(Child))Mesh=Existing;if(!Mesh){Mesh=NewObject<UInstancedStaticMeshComponent>(this);Mesh->SetupAttachment(Parent);Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/Materials/M_Supply_GalvanizedSteel")));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);Mesh->RegisterComponent();AddInstanceComponent(Mesh);}Mesh->ClearInstances();return Mesh;};
 auto Pieces=[&](UInstancedStaticMeshComponent* Mesh,FVector At,int32 Part,int32 Count){for(int32 I=0;I<FMath::Min(Count,6);++I){const FVector Base=At+FVector(0,0,I*1.4);Mesh->AddInstance(FTransform(FRotator::ZeroRotator,Base,FVector(20,15,1.2)/100));if(Part==int32(EFoundrylinePart::Bracket)||Part==int32(EFoundrylinePart::BentBracket))Mesh->AddInstance(FTransform(FRotator::ZeroRotator,Base+FVector(-9.4,0,6),FVector(1.2,15,12)/100));}};

 auto RemoveMissing=[&](auto& Map,auto Exists){for(auto It=Map.CreateIterator();It;++It)if(!Exists(It.Key())){if(It.Value()){TArray<USceneComponent*> Children;It.Value()->GetChildrenComponents(true,Children);for(int32 I=Children.Num()-1;I>=0;--I)Children[I]->DestroyComponent();It.Value()->DestroyComponent();}It.RemoveCurrent();}};
 CollectorCartWheels.RemoveAll([](const auto& Wheel){return !IsValid(Wheel);});
 RemoveMissing(SupplyBinVisuals,[&](int32 Id){return Snapshot.SupplyBins.ContainsByPredicate([&](const auto& B){return B.Area==Id;});});
 RemoveMissing(CollectorCartVisuals,[&](int32 Id){return Snapshot.Collectors.ContainsByPredicate([&](const auto& C){return C.Id==Id;});});
 RemoveMissing(PlanningRackVisuals,[&](int32 Id){return Snapshot.StorageRacks.ContainsByPredicate([&](const auto& R){return R.Id==Id;});});
 for(const auto& Rack:Snapshot.StorageRacks){auto& Visual=PlanningRackVisuals.FindOrAdd(Rack.Id);if(!Visual){Visual=NewObject<UStaticMeshComponent>(this);Visual->SetupAttachment(RootComponent);Visual->SetMobility(EComponentMobility::Movable);Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FFoundrylinePlanningEquipment::Mesh(Rack.FinishedGoods?2:1,Rack.Levels)));Visual->SetCollisionProfileName(TEXT("BlockAll"));Visual->SetCanEverAffectNavigation(false);Visual->RegisterComponent();AddInstanceComponent(Visual);
   auto* Collision=NewObject<UBoxComponent>(this);Collision->SetupAttachment(Visual);const auto Size=FFoundrylinePlanningEquipment::Size(Rack.FinishedGoods?2:1,Rack.Levels);Collision->SetBoxExtent(Size*.5);Collision->SetRelativeLocation(FVector(0,0,Size.Z*.5));Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));Collision->SetCanEverAffectNavigation(false);Collision->RegisterComponent();AddInstanceComponent(Collision);
   auto* Plate=NewObject<UStaticMeshComponent>(this);Plate->SetupAttachment(Visual);Plate->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Plate->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/Materials/M_Supply_Graphite")));Plate->SetRelativeLocation(FVector(0,-75,140));Plate->SetRelativeScale3D(FVector(1.3,.01,.24));Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plate->RegisterComponent();AddInstanceComponent(Plate);
   auto* Sign=NewObject<UTextRenderComponent>(this);Sign->SetupAttachment(Visual);Sign->SetRelativeLocationAndRotation(FVector(0,-75.6,140),FRotator(0,-90,0));Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetVerticalAlignment(EVRTA_TextCenter);Sign->SetWorldSize(15);Sign->SetTextRenderColor(Rack.FinishedGoods?FColor(30,170,65):FColor(40,115,235));Sign->SetText(FText::FromString(Rack.FinishedGoods?TEXT("FINISHED GOODS"):TEXT("PARTS")));Sign->SetCastShadow(false);Sign->RegisterComponent();AddInstanceComponent(Sign);
  }Visual->SetRelativeLocationAndRotation(Rack.Center,FRotator(0,Rack.QuarterTurns*90,0));
 }
 for(const auto& B:Snapshot.SupplyBins){auto& V=SupplyBinVisuals.FindOrAdd(B.Area);if(!V){V=NewObject<UStaticMeshComponent>(this);V->SetupAttachment(RootComponent);V->SetMobility(EComponentMobility::Movable);V->RegisterComponent();AddInstanceComponent(V);V->SetCollisionProfileName(TEXT("BlockAll"));V->SetCanEverAffectNavigation(false);}
  const FString Path=FString::Printf(TEXT("/Game/Foundryline/Production/AssemblySupply/SM_AssemblySupplyBin_%d.SM_AssemblySupplyBin_%d"),B.Compartments,B.Compartments);
  V->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));V->SetRelativeLocationAndRotation(B.Position,FRotator(0,B.Yaw,0));
  if(RefreshContents){auto* Cargo=Contents(V);for(int32 Index=0;Index<B.Slots.Num();++Index){const auto& Slot=B.Slots[Index];int32 Quantity=0;for(const auto& Pallet:Snapshot.Pallets)if(Pallet.Owner==B.Area&&Pallet.Place==EFoundrylinePalletPlace::Buffer&&int32(Pallet.Part)==Slot.Part)Quantity+=Pallet.Quantity;const int32 Columns=B.Compartments/2;Pieces(Cargo,FVector((Index%Columns-(Columns-1)*.5)*46.2,0,37+(Index/Columns)*39),Slot.Part,Quantity);}}

 }
 for(const auto& C:Snapshot.Collectors){auto& V=CollectorCartVisuals.FindOrAdd(C.Id);
  const auto* Employee=CachedEmployees.FindByPredicate([&](const auto& E){return E.Id==C.Id;});const FString Key=Employee?Employee->Key:FString();
  const FVector Person=EmployeeDisplayPositions.Contains(Key)?EmployeeDisplayPositions[Key]:C.Position;
  const float Yaw=EmployeeDisplayYaw.Contains(Key)?EmployeeDisplayYaw[Key]:C.Yaw;
  const FVector Target=Person+FRotator(0,Yaw,0).RotateVector(FVector(90,0,0));
  if(!V){V=NewObject<UStaticMeshComponent>(this);V->SetupAttachment(RootComponent);V->SetMobility(EComponentMobility::Movable);V->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/SM_CollectorCart_Body.SM_CollectorCart_Body")));V->SetCollisionProfileName(TEXT("BlockAll"));V->SetCanEverAffectNavigation(false);V->RegisterComponent();AddInstanceComponent(V);V->SetRelativeLocation(Target);
   for(float X:{-52.46f,52.46f})for(float Y:{-28.56f,28.56f}){
    auto* Caster=NewObject<UStaticMeshComponent>(this);Caster->SetupAttachment(V);Caster->SetMobility(EComponentMobility::Movable);Caster->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/SM_CollectorCart_Caster.SM_CollectorCart_Caster")));Caster->SetCollisionEnabled(ECollisionEnabled::NoCollision);Caster->SetRelativeLocation(FVector(X,Y,19.4));Caster->RegisterComponent();AddInstanceComponent(Caster);
    auto* Wheel=NewObject<UStaticMeshComponent>(this);Wheel->SetupAttachment(Caster);Wheel->SetMobility(EComponentMobility::Movable);Wheel->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/SM_CollectorCart_Wheel.SM_CollectorCart_Wheel")));Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);Wheel->SetRelativeLocation(FVector(0,0,-11.78));Wheel->RegisterComponent();AddInstanceComponent(Wheel);CollectorCartWheels.Add(Wheel);
   }
  }
  if(RefreshContents){auto* Cargo=Contents(V);int32 Index=0;for(const auto& Line:C.Load)if(Line.Picked){Pieces(Cargo,FVector((Index%3-1)*39,(Index/3%2-.5)*22,73),Line.Part,Line.Quantity);++Index;}}
  const FVector Before=V->GetRelativeLocation();const float PreviousYaw=V->GetRelativeRotation().Yaw;
  V->SetRelativeLocationAndRotation(Target,FRotator(0,Yaw,0));
  const bool Moving=Snapshot.Running&&C.Speed>.1;const float Travel=Moving?FVector::Dist2D(Before,Target):0;
  const float Steering=Moving?FMath::Clamp(-FMath::FindDeltaAngleDegrees(PreviousYaw,Yaw)*4,-35.f,35.f):0;
  for(UStaticMeshComponent* Wheel:CollectorCartWheels)if(IsValid(Wheel)&&Wheel->GetAttachParent()&&Wheel->GetAttachParent()->GetAttachParent()==V){Wheel->AddLocalRotation(FRotator(-Travel/7.62f*180.f/PI,0,0));auto* Caster=Wheel->GetAttachParent();Caster->SetRelativeRotation(FMath::RInterpTo(Caster->GetRelativeRotation(),FRotator(0,Steering,0),Dt,8));}
  if(auto* Visual=EmployeeVisuals.Find(Key);Visual&&*Visual){
   FVector Left(-65,-22,99),Right(-65,22,99);
   // Keep one hand on the handle while the other reaches into a cart bin during handling.
   if(Snapshot.Running&&C.Handling>0){const float Reach=FMath::Sin(FMath::Clamp(C.Handling,0.f,1.f)*PI);Right=FMath::Lerp(Right,FVector(-45,10,84),Reach);}
   (*Visual)->PresentCollectorGrip(V->GetComponentTransform().TransformPosition(Left),V->GetComponentTransform().TransformPosition(Right));
  }
 }
}
