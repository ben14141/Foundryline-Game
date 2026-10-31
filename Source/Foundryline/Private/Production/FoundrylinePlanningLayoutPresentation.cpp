#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

void AFoundrylineFactoryPlant::RebuildPlanningLayout(){
 EnsureMachineActors();
 const auto Racks=FFoundrylinePlannerItems::FixedRacks(Snapshot);FString Key;for(const auto& R:Racks)Key+=FString::Printf(TEXT("%d:%s:%d:%d;"),R.Id,*R.Center.ToString(),R.Quarter,R.CompactRack);
 if(Key==PlanningLayoutKey)return;PlanningLayoutKey=Key;
 TInlineComponentArray<USceneComponent*> Existing;GetComponents(Existing);for(auto* C:Existing)if(C->ComponentHasTag(TEXT("FoundrylinePlanningFixedRack")))C->DestroyComponent();
 for(const auto& R:Racks){const FRotator Rotation(0,R.Quarter*90,0);
  auto Register=[&](USceneComponent* C){C->SetupAttachment(RootComponent);C->ComponentTags.Add(TEXT("FoundrylinePlanningFixedRack"));C->RegisterComponent();AddInstanceComponent(C);};
  auto Block=[&](FVector At,FVector Extent){auto* C=NewObject<UBoxComponent>(this);C->SetBoxExtent(Extent);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->SetCollisionProfileName(TEXT("BlockAllDynamic"));C->SetCanEverAffectNavigation(false);Register(C);C->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(At),Rotation);};
  if(R.CompactRack){for(int Side:{-1,1})Block({Side*145.,0,152.4},{8,75,152.4});Block({0,0,146.4},{155,75,6});}
  else {for(int Side:{-1,1})Block({0,Side*201.735,210},{60,7.5,210});for(int Level=0;Level<3;++Level)Block({0,0,FFoundrylineWorkshopLayout::ShelfOrigin(Level)+7},{60,208,6});}

  if(GetNetMode()==NM_DedicatedServer)continue;
  for(int Side:{-1,1}){
   const FVector Local=R.CompactRack?FVector(0,Side*76,140):FVector(Side*61,0,284);
   const FRotator Face(0,R.Quarter*90+(R.CompactRack?Side*90:Side<0?180:0),0);
   auto* Plate=NewObject<UStaticMeshComponent>(this);Plate->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Plate->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/Materials/M_Supply_Graphite")));Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);Register(Plate);Plate->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(Local),Face);Plate->SetRelativeScale3D({.02,2.4,.28});
   auto* Sign=NewObject<UTextRenderComponent>(this);Sign->SetText(FText::FromString(R.Name));Sign->SetWorldSize(12);Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetVerticalAlignment(EVRTA_TextCenter);Sign->SetTextRenderColor(FColor(235,235,230));Sign->SetCastShadow(false);Register(Sign);Sign->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(Local)+Face.Vector()*1.2,Face);
  }
  auto Part=[&](const TCHAR* Name,FVector Offset){auto* C=NewObject<UStaticMeshComponent>(this);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_%s.SM_%s"),Name,Name)));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);Register(C);C->SetRelativeLocationAndRotation(R.Center+Rotation.RotateVector(Offset),Rotation);};
  if(R.CompactRack){auto* Rack=NewObject<UStaticMeshComponent>(this);Rack->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels.SM_PalletRack_2Levels")));Rack->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rack->SetCanEverAffectNavigation(false);Register(Rack);Rack->SetRelativeLocationAndRotation(R.Center,Rotation);continue;}
  for(int Side:{-1,1})Part(TEXT("Rack_Upright"),{0,Side*201.735,0});
  for(int I=0;I<3;++I)Part(TEXT("Rack_Shelf400"),{0,0,FFoundrylineWorkshopLayout::ShelfOrigin(I)});
 }
}
