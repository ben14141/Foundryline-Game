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
 EnsureMachineActors();
 const auto Racks=FFoundrylinePlannerItems::FixedRacks(Snapshot);FString Key;for(const auto& R:Racks)Key+=FString::Printf(TEXT("%d:%s:%d:%d;"),R.Id,*R.Center.ToString(),R.Quarter,R.CompactRack);
 if(Key==PlanningLayoutKey)return;PlanningLayoutKey=Key;
 TInlineComponentArray<USceneComponent*> Existing;GetComponents(Existing);for(auto* C:Existing)if(C->ComponentHasTag(TEXT("FoundrylinePlanningFixedRack")))C->DestroyComponent();
 for(const auto& R:Racks){const FRotator Rotation(0,R.Quarter*90,0);
  auto Register=[&](USceneComponent* C){C->SetupAttachment(RootComponent);C->ComponentTags.Add(TEXT("FoundrylinePlanningFixedRack"));C->RegisterComponent();AddInstanceComponent(C);};
  auto* Body=NewObject<UBoxComponent>(this);Body->SetBoxExtent(R.CompactRack?FVector(155,75,152.4):FVector(60,208,210));Body->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));Body->SetCanEverAffectNavigation(false);Register(Body);Body->SetRelativeLocationAndRotation(R.Center+FVector(0,0,R.CompactRack?152.4:210),Rotation);
  if(GetNetMode()==NM_DedicatedServer)continue;
  auto Part=[&](const TCHAR* Name,FVector Offset){auto* C=NewObject<UStaticMeshComponent>(this);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_%s.SM_%s"),Name,Name)));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);Register(C);C->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(Offset),Rotation);};
  if(R.CompactRack){auto* Rack=NewObject<UStaticMeshComponent>(this);Rack->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels.SM_PalletRack_2Levels")));Rack->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rack->SetCanEverAffectNavigation(false);Register(Rack);Rack->SetRelativeLocationAndRotation(R.Center,Rotation);continue;}
  for(int Side:{-1,1})Part(TEXT("Rack_Upright"),{0,Side*201.735,0});
  for(int I=0;I<3;++I)Part(TEXT("Rack_Shelf400"),{0,0,FFoundrylineWorkshopLayout::ShelfOrigin(I)});
 }
}
