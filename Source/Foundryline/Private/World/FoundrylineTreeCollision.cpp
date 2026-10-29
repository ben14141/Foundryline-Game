#include "World/FoundrylineTreeCollision.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "TimerManager.h"
AFoundrylineTreeCollision::AFoundrylineTreeCollision(){bReplicates=true;bAlwaysRelevant=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TrunkCollision")));}
void AFoundrylineTreeCollision::BeginPlay(){Super::BeginPlay();FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AFoundrylineTreeCollision::BuildTrunks,.5f,false);}
void AFoundrylineTreeCollision::BuildTrunks(){
 for(auto C:Trunks)if(C)C->DestroyComponent();Trunks.Reset();
 // Conservative stem-only profiles for the four authored starter forest species.
 // Canopies deliberately do not participate in walking collision.
 for(TActorIterator<AActor> It(GetWorld());It;++It){TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);for(auto* M:Parts){if(!M->GetStaticMesh())continue;const FString N=M->GetStaticMesh()->GetName();if(!N.StartsWith(TEXT("SM_Tree_Birch_"))&&!N.StartsWith(TEXT("SM_Tree_Broadleaf_")))continue;
 auto Add=[&](const FTransform& T){if(Trunks.Num()>=4096||T.GetLocation().Size2D()>20000)return;const FVector S=T.GetScale3D().GetAbs();const float Radius=(N.Contains(TEXT("Birch"))?12.f:20.f)*FMath::Min(S.X,S.Y);const double Bottom=M->GetStaticMesh()->GetBoundingBox().Min.Z;const FVector Base=T.TransformPosition({0,0,Bottom});auto* C=NewObject<UCapsuleComponent>(this,FName(*FString::Printf(TEXT("TreeTrunk_%d"),Trunks.Num())));C->SetupAttachment(RootComponent);C->SetCapsuleSize(Radius,100*S.Z);C->SetWorldLocation(Base+FVector(0,0,100*S.Z));C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionObjectType(ECC_WorldStatic);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->ComponentTags.Add(TEXT("FoundrylineTreeTrunk"));C->RegisterComponent();AddInstanceComponent(C);Trunks.Add(C);};
 if(auto* ISM=Cast<UInstancedStaticMeshComponent>(M)){for(int I=0;I<ISM->GetInstanceCount();++I){FTransform T;if(ISM->GetInstanceTransform(I,T,true))Add(T);}}else Add(M->GetComponentTransform());
 }}
}
