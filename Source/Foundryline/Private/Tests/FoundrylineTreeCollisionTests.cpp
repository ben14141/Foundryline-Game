#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "World/FoundrylineTreeCollision.h"
#include "World/FoundrylineWalkCharacter.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreeStemWalking,"Foundryline.World.TreeStemWalking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTreeStemWalking::RunTest(const FString&){auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Holder=W->SpawnActor<AActor>();auto* Trees=NewObject<UInstancedStaticMeshComponent>(Holder);Trees->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/Trees/SM_Tree_Birch_1")));if(!TestNotNull(TEXT("Authored starter birch exists"),Trees->GetStaticMesh().Get())){W->DestroyWorld(false);return false;}Trees->SetCollisionEnabled(ECollisionEnabled::NoCollision);Trees->RegisterComponent();Holder->AddInstanceComponent(Trees);Trees->AddInstance(FTransform(FVector(2000,2000,0)));auto* A=W->SpawnActor<AFoundrylineTreeCollision>();A->BuildTrunks();TArray<UCapsuleComponent*> Caps;A->GetComponents(Caps);TestEqual(TEXT("One trunk per actual instance, no canopy colliders"),Caps.Num(),1);
 if(Caps.Num()){const auto* Player=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();auto Shape=FCollisionShape::MakeCapsule(Player->GetUnscaledCapsuleRadius(),Player->GetUnscaledCapsuleHalfHeight());FHitResult H;FVector P=Caps[0]->GetComponentLocation();P.Z=Player->GetUnscaledCapsuleHalfHeight();TestTrue(TEXT("Player capsule cannot cross tree stem"),W->SweepSingleByChannel(H,P-FVector(100,0,0),P,FQuat::Identity,ECC_Pawn,Shape));TestFalse(TEXT("Player may pass beside trunk under nonblocking canopy"),W->SweepSingleByChannel(H,P+FVector(-100,100,0),P+FVector(100,100,0),FQuat::Identity,ECC_Pawn,Shape));A->BuildTrunks();TArray<UCapsuleComponent*> Again;A->GetComponents(Again);TestEqual(TEXT("Rebuild does not duplicate trunks"),Again.Num(),1);}
 W->DestroyWorld(false);return true;}
#endif
