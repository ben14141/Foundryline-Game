#include "Inventory/FoundrylinePartsArea.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"

AFoundrylinePartsArea::AFoundrylinePartsArea(){bReplicates=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PartsArea")));}
void AFoundrylinePartsArea::BeginPlay(){
 Super::BeginPlay();Tags.Add(TEXT("FoundrylinePartsArea"));
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Material=[](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_%s.M_MH_%s"),Name,Name));};
 auto Mesh=[&](UStaticMesh* Asset,FVector P,FRotator R=FRotator::ZeroRotator,FVector S=FVector::OneVector,UMaterialInterface* M=nullptr){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Asset);C->SetRelativeLocationAndRotation(P,R);C->SetRelativeScale3D(S);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);if(M)C->SetMaterial(0,M);C->RegisterComponent();AddInstanceComponent(C);return C;};
 auto Box=[&](FVector P,FVector Size,UMaterialInterface* M){return Mesh(Cube,P,FRotator::ZeroRotator,Size/100,M);};
 auto Block=[&](FVector P,FVector Half){auto* C=NewObject<UBoxComponent>(this);C->SetupAttachment(RootComponent);C->SetRelativeLocation(P);C->SetBoxExtent(Half);C->SetCollisionProfileName(TEXT("BlockAllDynamic"));C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);};
 auto* Yellow=Material(TEXT("SafetyYellow"));auto* DarkMaterial=Material(TEXT("Graphite"));auto* Steel=Material(TEXT("MachinedSteel"));
 // A 14 x 8 m parts zone. Keep a four-metre cross-aisle between staging and racks.
 // Functional factory floor markings are owned by the plant's typed layout.
 auto* Upright=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Upright.SM_Rack_Upright"));
 auto* Shelf=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Shelf400.SM_Rack_Shelf400"));
 auto* Crate=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Props/SM_Box_01.SM_Box_01"));
 const FVector CrateSize=Crate?Crate->GetBounds().BoxExtent*2:FVector(999);
 const bool Fits=Crate&&CrateSize.X<=100&&CrateSize.Y<=100&&CrateSize.Z<=100;
 for(float X:{350.f,1000.f}){
  const FVector Center(X,650,0);const FRotator Rotation(0,90,0);
  for(int End:{-1,1})Mesh(Upright,Center+FVector(End*201.735,0,0),Rotation);
  for(int Level=0;Level<3;++Level){
   const float Z=20+Level*130;Mesh(Shelf,Center+FVector(0,0,Z),Rotation);
   Block(Center+FVector(0,0,Z+10),FVector(201.7,55.8,10));
   // Stock meshes now come from the exact factory pallet records.
  }
  for(int End:{-1,1})Block(Center+FVector(End*201.735,0,210),FVector(6.3,57.5,210));
 }
 Box(FVector(100,100,87),FVector(180,80,6),Steel);
 for(float X:{20.f,180.f})for(float Y:{70.f,130.f})Box(FVector(X,Y,42),FVector(6,6,84),DarkMaterial);
 Block(FVector(100,100,45),FVector(90,40,45));
}
