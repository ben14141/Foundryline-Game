#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "CollisionQueryParams.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentWalkingCollision,"Foundryline.Production.EquipmentWalkingCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEquipmentWalkingCollision::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Plasma=W->SpawnActor<AFoundrylinePlasmaTable>();Plasma->bFactoryAutomation=true;Plasma->DispatchBeginPlay();auto* Brake=W->SpawnActor<AFoundrylinePressBrakeDisplay>(FVector(3000,0,0),FRotator::ZeroRotator);Brake->DispatchBeginPlay();auto* Roller=W->SpawnActor<AFoundrylineMaterialHandlingCell>(FVector(6000,0,0),FRotator::ZeroRotator);Roller->bFactoryAutomation=true;Roller->DispatchBeginPlay();
 const auto* Capsule=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();const float Radius=Capsule->GetUnscaledCapsuleRadius(),Half=Capsule->GetUnscaledCapsuleHalfHeight();const auto Shape=FCollisionShape::MakeCapsule(Radius,Half);
 for(AActor* A:{static_cast<AActor*>(Plasma),static_cast<AActor*>(Brake),static_cast<AActor*>(Roller)}){
  TArray<UBoxComponent*> Boxes;A->GetComponents(Boxes);UBoxComponent* Body=nullptr;for(auto* C:Boxes)if(C->ComponentHasTag(TEXT("FoundrylineEquipmentPawnBody")))Body=C;
  if(!TestNotNull(TEXT("Machine has primary body walking collision"),Body))continue;auto* Mesh=Cast<UStaticMeshComponent>(Body->GetAttachParent());if(!TestNotNull(TEXT("Walking bounds attach to actual primary mesh"),Mesh)||!Mesh->GetStaticMesh())continue;
  const FBox B=Mesh->GetStaticMesh()->GetBoundingBox();const auto T=Mesh->GetComponentTransform();const FVector End(B.GetCenter().X,B.GetCenter().Y,B.Min.Z+Half),Start(B.Min.X-Radius-100,End.Y,End.Z);FHitResult Hit;FCollisionQueryParams Query;for(auto* C:Boxes)if(C!=Body)Query.AddIgnoredComponent(C);
  const bool Blocked=W->SweepSingleByChannel(Hit,T.TransformPosition(Start),T.TransformPosition(End),FQuat::Identity,ECC_Pawn,Shape,Query);TestTrue(TEXT("Real first-person capsule cannot walk through machinery"),Blocked&&Hit.GetActor()==A);
  const FVector ClearStart(Start.X,B.Max.Y+Radius+100,End.Z),ClearEnd(End.X,ClearStart.Y,End.Z);TestFalse(TEXT("No invisible working-clearance wall beyond the real machine body"),W->SweepSingleByChannel(Hit,T.TransformPosition(ClearStart),T.TransformPosition(ClearEnd),FQuat::Identity,ECC_Pawn,Shape,Query));TestFalse(TEXT("Equipment walking bounds do not block dynamic material-handling helpers"),W->SweepSingleByChannel(Hit,T.TransformPosition(Start),T.TransformPosition(End),FQuat::Identity,ECC_WorldDynamic,Shape,Query));
 }
 W->DestroyWorld(false);return true;
}
#endif
