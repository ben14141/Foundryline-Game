#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "World/FoundrylineWalkCharacter.h"
#include "World/FoundrylineCameraPawn.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Interfaces/MovementBaseInterface.h"
#include "Materials/MaterialInterface.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineWalkTest,"Foundryline.World.WalkAndPersonnelDoor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineWalkTest::RunTest(const FString& Parameters)
{
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* PC=W->SpawnActor<AFoundrylinePlayerController>();auto* Camera=W->SpawnActor<AFoundrylineCameraPawn>();PC->Possess(Camera);
 auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bLargeShowcase=true;Site->DispatchBeginPlay();
 auto* RoofWindow=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Buildings/Roofs/SM_Skylight_400.SM_Skylight_400"));
 TestTrue(TEXT("Skylight glazing separated from native opaque frame"),RoofWindow && RoofWindow->GetStaticMaterials().Num()==4 && RoofWindow->GetMaterial(3)->GetBlendMode()==BLEND_Translucent && RoofWindow->GetMaterial(2)->GetBlendMode()==BLEND_Opaque);
 auto* WindowGlass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_WindowGlass.M_WindowGlass"));TestTrue(TEXT("Wall windows transmit light through translucent glazing"),WindowGlass && WindowGlass->GetBlendMode()==BLEND_Translucent);
 // Static floor bases must never require replication of client-local components.
 for(TActorIterator<AActor> It(W);It;++It) if(It->ActorHasTag(TEXT("FoundrylinePropertyVisual")))
 {
  TInlineComponentArray<UInstancedStaticMeshComponent*> Components;It->GetComponents(Components);
  for(auto* C:Components) if(C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision)
   TestTrue(TEXT("Committed collision geometry is a stationary world-space movement base"),C->Mobility==EComponentMobility::Static);
 }
 auto Roofs=[&](bool Visible)
 {
  int32 Count=0,Windows=0;
  for(TActorIterator<AActor> It(W);It;++It) if(!It->IsActorBeingDestroyed()) {TInlineComponentArray<UInstancedStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs) if(C->ComponentHasTag(TEXT("FoundrylineRoof")))
  {
   ++Count;TestEqual(TEXT("Roof visibility matches view"),C->IsVisible(),Visible);TestTrue(TEXT("Roofs do not obstruct construction picking"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
   if(C->GetStaticMesh()->GetName()==TEXT("SM_Skylight_400")) {Windows+=C->GetInstanceCount();for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T);TestTrue(TEXT("Skylights retain 1:1 scale"),T.GetScale3D().Equals(FVector::OneVector));}}
  }}
  TestTrue(TEXT("Factory roofs and skylights installed"),Count>=6 && Windows>0);
 };
 Roofs(false);PC->ToggleRoofs();Roofs(true);PC->ToggleRoofs();Roofs(false);
 TestTrue(TEXT("Camera initially possessed"),PC->GetPawn()==Camera);PC->ToggleWalk();auto* Walker=Cast<AFoundrylineWalkCharacter>(PC->GetPawn());
 if(!TestNotNull(TEXT("Walk mode possesses real character"),Walker)) {W->DestroyWorld(false);return false;}
 for(TActorIterator<AActor> It(W);It;++It) if(It->ActorHasTag(TEXT("FoundrylinePropertyVisual")))
 {
  TInlineComponentArray<UInstancedStaticMeshComponent*> Components;It->GetComponents(Components);
  for(auto* C:Components) if(C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision) {
   FMovementBaseInterfaceData Base(C);Walker->SetBase(&Base);
   TestNull(TEXT("Client-local floor is never serialized as character base"),Walker->GetMovementBaseObject());
  }
 }
 Roofs(true);PC->ToggleRoofs();Roofs(true);
 TestEqual(TEXT("Human capsule is 1.8 m tall"),Walker->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),90.f);
 TestTrue(TEXT("Spawns outside office entrance"),Walker->GetActorLocation().X>8000);
 PC->ToggleWalk();TestTrue(TEXT("Free top-down toggle is blocked in first person"),PC->GetPawn()==Walker);
 auto* GS=W->SpawnActor<AGameStateBase>();W->SetGameState(GS);PC->PlayerState=W->SpawnActor<APlayerState>();GS->PlayerArray.Add(PC->PlayerState);auto* Registry=AFoundrylineBusinessRegistry::Find(W);if(!Registry)Registry=W->SpawnActor<AFoundrylineBusinessRegistry>();TestTrue(TEXT("Office user is a registered company member"),Registry&&Registry->RegisterPlayer(PC));auto* Plant=W->SpawnActor<AFoundrylineFactoryPlant>();auto* Terminal=W->SpawnActor<AFoundrylineOperationsTerminal>();Terminal->SetActorLocation(Walker->GetActorLocation()-FVector(0,170,90));const FVector Return=Walker->GetActorLocation();
 TestNotNull(TEXT("Own plant found for office user"),PC->FactoryPlant());TestTrue(TEXT("Office computer is within interaction range"),PC->IsNearOfficeComputer());PC->ServerEnterPropertyView_Implementation();TestTrue(TEXT("Office computer grants property camera"),PC->GetPawn()==Camera&&PC->bPropertyViewAccess);
 Roofs(false);PC->ToggleRoofs();Roofs(true);PC->ToggleWalk();Roofs(true);TestTrue(TEXT("Return restores office position"),PC->GetPawn()->GetActorLocation().Equals(Return,1));PC->ToggleWalk();Roofs(true);TestTrue(TEXT("Free toggle remains blocked after returning"),PC->IsWalking());PC->ServerEnterPropertyView_Implementation();Roofs(false);
 // Isolated door: collision follows the animated leaf rather than a permanent blocker.
 auto* DoorActor=W->SpawnActor<AActor>();auto* Door=NewObject<UFoundrylinePersonnelDoorComponent>(DoorActor);DoorActor->SetRootComponent(Door);DoorActor->AddInstanceComponent(Door);Door->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Door->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Door->SetCollisionResponseToAllChannels(ECR_Block);Door->RegisterComponent();Door->SetWorldLocation(FVector(-10000,0,100));Door->SetWorldScale3D(FVector(1,.05,2));Door->InitializeSwing(0,90,FGuid::NewGuid(),false);
 auto* User=W->SpawnActor<AFoundrylineWalkCharacter>(FVector(-10000,-200,90),FRotator(0,90,0));PC->Possess(User);PC->SetControlRotation(FRotator(0,90,0));
 TestTrue(TEXT("Nearby player opens a door in sight"),Door->TryToggle(User));Door->TickComponent(.25,ELevelTick::LEVELTICK_All,nullptr);
 TestTrue(TEXT("Door has a visible intermediate swing"),Door->GetRelativeRotation().Yaw>0 && Door->GetRelativeRotation().Yaw<90);
 Door->TickComponent(.25,ELevelTick::LEVELTICK_All,nullptr);TestTrue(TEXT("Door reaches open position"),FMath::IsNearlyEqual(Door->GetRelativeRotation().Yaw,90.f));
 TestTrue(TEXT("E can toggle the same door closed"),Door->TryToggle(User));Door->TickComponent(.5,ELevelTick::LEVELTICK_All,nullptr);TestTrue(TEXT("Door reaches closed position"),FMath::IsNearlyZero(Door->GetRelativeRotation().Yaw));
 User->SetActorLocation(FVector(-10000,-1000,90));TestFalse(TEXT("Distant player cannot operate door"),Door->TryToggle(User));
 TestFalse(TEXT("Idle closed doors do not tick"),Door->IsComponentTickEnabled());
 W->DestroyWorld(false);return true;
}
#endif
