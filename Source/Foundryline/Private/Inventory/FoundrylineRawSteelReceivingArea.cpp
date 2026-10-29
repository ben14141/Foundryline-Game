#include "Inventory/FoundrylineRawSteelReceivingArea.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
AFoundrylineRawSteelReceivingArea::AFoundrylineRawSteelReceivingArea(){bReplicates=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ReceivingArea")));}
void AFoundrylineRawSteelReceivingArea::BeginPlay(){
 Super::BeginPlay();Tags.Add(TEXT("FoundrylineRawSteelReceiving"));
 auto* Lines=NewObject<UInstancedStaticMeshComponent>(this);Lines->SetupAttachment(RootComponent);Lines->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Lines->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_SafetyYellow.M_MH_SafetyYellow")));Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lines->SetCanEverAffectNavigation(false);Lines->SetCastShadow(false);Lines->RegisterComponent();AddInstanceComponent(Lines);
 auto Mark=[&](FVector P,FVector Size){Lines->AddInstance(FTransform(FQuat::Identity,P,Size/100));};
 // Typed receiving bays are painted and labelled by the factory plant.
 // Two straight loading guides on the paved apron, aligned with the 5 m doors.
 for(float CenterY:{1650.f,3000.f,4350.f}){
  for(float Side:{-300.f,300.f})Mark(FVector(4200,CenterY+Side,1.5),FVector(2400,5,3));
  Mark(FVector(5400,CenterY,1.5),FVector(5,600,3));
 }
}
