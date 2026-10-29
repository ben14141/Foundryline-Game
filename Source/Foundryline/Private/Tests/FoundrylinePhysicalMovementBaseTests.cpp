#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Interfaces/MovementBaseInterface.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicalMovementBase,"Foundryline.Collision.LocalPhysicalMovementBase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPhysicalMovementBase::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Walker=World->SpawnActor<AFoundrylineWalkCharacter>();auto* Holder=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Holder);Holder->SetRootComponent(Box);Box->SetMobility(EComponentMobility::Movable);Box->SetBoxExtent({60,60,10});Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->RegisterComponent();Holder->AddInstanceComponent(Box);
 FMovementBaseInterfaceData Base;Base.Set(Box);Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Walker->SetBase(&Base);TestTrue(TEXT("Local pooled pallet cannot become an unsupported relative movement base"),Walker->GetMovementBaseObject()==nullptr);TestTrue(TEXT("Local pallet still blocks the walking capsule"),Box->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block);
 Box->ComponentTags.Remove(TEXT("FoundrylinePhysicalCollision"));Walker->SetBase(&Base);TestTrue(TEXT("A real moving platform retains ordinary based movement"),Walker->GetMovementBaseObject()==Box);World->DestroyWorld(false);return true;
}
#endif
