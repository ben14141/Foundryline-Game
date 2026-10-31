#include "Management/FoundrylineOfficeDecorationComponent.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
void UFoundrylineOfficeDecorationComponent::Apply(const FFoundrylineOfficeAssignment& A){
 if(Built&&FFoundrylineOfficeAssignment::StaticStruct()->CompareScriptStruct(&Source,&A,0))return;
 for(auto P:Pieces)if(IsValid(P))P->DestroyComponent();Pieces.Reset();Source=A;Built=true;
 auto Register=[&](USceneComponent* C){C->SetupAttachment(this);C->ComponentTags.Add(TEXT("FoundrylineOfficeDecoration"));C->UpdateComponentToWorld();C->RegisterComponent();GetOwner()->AddInstanceComponent(C);Pieces.Add(C);};
 for(const auto& P:A.Decorations){if(P.HiddenByGlazing)continue;const FRotator Rotation(0,P.Quarter*90,0);const FString Name=FString(TEXT("SM_"))+FFoundrylineOffices::Mesh(P.Kind);const FString Path=TEXT("/Game/Foundryline/Office/Decor/")+Name+TEXT(".")+Name;
  auto* C=NewObject<UStaticMeshComponent>(GetOwner());C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));C->SetRelativeLocationAndRotation(P.Position,Rotation);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);Register(C);
  if(FFoundrylineOffices::Collides(P.Kind)){const auto Size=FFoundrylineOffices::Size(P.Kind);auto* B=NewObject<UBoxComponent>(GetOwner());B->SetRelativeLocationAndRotation(P.Position+FVector(0,0,Size.Z/2),Rotation);B->SetBoxExtent(Size/2);B->SetCollisionProfileName(TEXT("BlockAll"));B->SetCanEverAffectNavigation(false);B->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Register(B);}
 }
}
void AFoundrylineFactoryPlant::UpdateOfficeDecorations(){if(HasAuthority())if(auto* P=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>())if(OfficeDecorPropertyRevision!=P->GetRevision()||Model.State.OfficeAssignments.ContainsByPredicate([](const auto& A){return A.FurnishingVersion<2;})){OfficeDecorPropertyRevision=P->GetRevision();FFoundrylineOffices::ReconcileWallDecor(GetWorld(),Model,GetActorTransform());Snapshot=Model.State;}const auto& State=HasAuthority()?Model.State:Snapshot;UpdateOfficeDoors();
 for(auto It=OfficeDecorationVisuals.CreateIterator();It;++It)if(!State.OfficeAssignments.ContainsByPredicate([&](const auto& A){return A.Id==It.Key();})){if(auto* C=It.Value().Get()){TArray<USceneComponent*> Descendants;C->GetChildrenComponents(true,Descendants);for(auto* P:Descendants)P->DestroyComponent();C->DestroyComponent();}It.RemoveCurrent();}
 for(const auto& A:State.OfficeAssignments){auto& C=OfficeDecorationVisuals.FindOrAdd(A.Id);if(!IsValid(C)){C=NewObject<UFoundrylineOfficeDecorationComponent>(this);C->SetupAttachment(RootComponent);C->RegisterComponent();AddInstanceComponent(C);}C->Apply(A);}
}
void AFoundrylineFactoryPlant::SetOfficeDecorationFloor(int32 Floor,bool Walking){for(const auto& P:OfficeDecorationVisuals)if(IsValid(P.Value))P.Value->SetVisibility(Walking||P.Value->Floor()==Floor,true);}
