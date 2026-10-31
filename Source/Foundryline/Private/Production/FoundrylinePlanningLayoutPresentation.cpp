#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

void AFoundrylineFactoryPlant::RebuildPlanningLayout(){
 auto Match=[&](const auto* A){return A&&A->BusinessCompanyId==BusinessCompanyId&&A->BusinessPlantId==BusinessPlantId;};
 if(!IsValid(Plasma))for(TActorIterator<AFoundrylinePlasmaTable> It(GetWorld());It;++It)if(Match(*It)){Plasma=*It;break;}
 if(!IsValid(Brake))for(TActorIterator<AFoundrylinePressBrakeDisplay> It(GetWorld());It;++It)if(Match(*It)){Brake=*It;break;}
 if(!IsValid(Cell))for(TActorIterator<AFoundrylineMaterialHandlingCell> It(GetWorld());It;++It)if(Match(*It)){Cell=*It;break;}
 for(const auto& M:Snapshot.Machines){AActor* Actor=M.Id==1?static_cast<AActor*>(Plasma.Get()):M.Id==2?static_cast<AActor*>(Brake.Get()):static_cast<AActor*>(Cell.Get());if(!IsValid(Actor))continue;
  const FVector Local=M.Origin-(M.Id==3?FVector(300,550,0):FVector::ZeroVector);const FVector At=GetActorTransform().TransformPosition(Local);
  if(!Actor->GetActorLocation().Equals(At,.01))Actor->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
  Actor->SetActorHiddenInGame(M.bStored);Actor->SetActorEnableCollision(!M.bStored);
 }
 const auto Racks=FFoundrylinePlannerItems::FixedRacks(Snapshot);FString Key;for(const auto& R:Racks)Key+=FString::Printf(TEXT("%d:%s:%d;"),R.Id,*R.Center.ToString(),R.Quarter);
 if(Key==PlanningLayoutKey)return;PlanningLayoutKey=Key;
 TInlineComponentArray<USceneComponent*> Existing;GetComponents(Existing);for(auto* C:Existing)if(C->ComponentHasTag(TEXT("FoundrylinePlanningFixedRack")))C->DestroyComponent();
 for(const auto& R:Racks){const FRotator Rotation(0,R.Quarter*90,0);
  auto Register=[&](USceneComponent* C){C->SetupAttachment(RootComponent);C->ComponentTags.Add(TEXT("FoundrylinePlanningFixedRack"));C->RegisterComponent();AddInstanceComponent(C);};
  auto* Body=NewObject<UBoxComponent>(this);Body->SetBoxExtent({60,208,210});Body->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));Body->SetCanEverAffectNavigation(false);Register(Body);Body->SetRelativeLocationAndRotation(R.Center+FVector(0,0,210),Rotation);
  if(GetNetMode()==NM_DedicatedServer)continue;
  auto Part=[&](const TCHAR* Name,FVector Offset){auto* C=NewObject<UStaticMeshComponent>(this);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_%s.SM_%s"),Name,Name)));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);Register(C);C->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(Offset),Rotation);};
  for(int Side:{-1,1})Part(TEXT("Rack_Upright"),{0,Side*201.735,0});
  for(int I=0;I<3;++I)Part(TEXT("Rack_Shelf400"),{0,0,FFoundrylineWorkshopLayout::ShelfOrigin(I)});
 }
}
