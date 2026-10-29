#include "Construction/FoundrylineEquipmentCollision.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
UBoxComponent* FFoundrylineEquipmentCollision::AddBox(AActor* Owner,USceneComponent* Parent,FName Name,FVector Center,FVector Extent){
 if(!Owner||!Parent||Extent.GetMin()<=0)return nullptr;
 auto* C=NewObject<UBoxComponent>(Owner,MakeUniqueObjectName(Owner,UBoxComponent::StaticClass(),Name));C->SetupAttachment(Parent);C->SetRelativeLocation(Center);C->SetBoxExtent(Extent);C->SetMobility(Parent->Mobility);C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionObjectType(ECC_WorldDynamic);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->ComponentTags.Add(Name);C->RegisterComponent();Owner->AddInstanceComponent(C);return C;
}
UBoxComponent* FFoundrylineEquipmentCollision::AddPawnBody(UStaticMeshComponent* Body){
 if(!Body||!Body->GetStaticMesh()||!Body->GetOwner())return nullptr;
 auto* Owner=Body->GetOwner();TArray<UBoxComponent*> Existing;Owner->GetComponents(Existing);
 for(auto* C:Existing)if(C->ComponentHasTag(TEXT("FoundrylineEquipmentPawnBody"))&&C->GetAttachParent()==Body)return C;
 const FBox Bounds=Body->GetStaticMesh()->GetBoundingBox();if(!Bounds.IsValid||Bounds.GetExtent().GetMin()<=0)return nullptr;
 auto* C=AddBox(Owner,Body,FName(*(Body->GetName()+TEXT("_PhysicalBody"))),Bounds.GetCenter(),Bounds.GetExtent());C->ComponentTags.Add(TEXT("FoundrylineEquipmentPawnBody"));return C;
}
