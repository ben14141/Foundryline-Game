#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "World/FoundrylineWalkCharacter.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeDoorWalkingClearance,"Foundryline.Collision.OfficeDoorWalkingClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeDoorWalkingClearance::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->bCutPartsStarter=true;Site->DispatchBeginPlay();const auto* Capsule=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetUnscaledCapsuleRadius(),Capsule->GetUnscaledCapsuleHalfHeight());FHitResult Hit;FCollisionQueryParams Query;
 TestFalse(TEXT("Open internal office doors leave the real corridor centre clear"),World->SweepSingleByChannel(Hit,{-2500,500,112},{-2500,1300,112},FQuat::Identity,ECC_Pawn,Shape,Query));
 if(Hit.bBlockingHit)AddInfo(FString::Printf(TEXT("Corridor hit %s at %s"),*GetPathNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString()));
 UFoundrylinePersonnelDoorComponent* Door=nullptr;for(TActorIterator<AActor> It(World);It;++It){TArray<UFoundrylinePersonnelDoorComponent*> Doors;It->GetComponents(Doors);for(auto* D:Doors)if(FMath::Abs(D->GetComponentLocation().X+2600)<1&&D->GetComponentLocation().Y<800)Door=D;}
 // The new behavior is the authored leaf's direction and free corridor. The
 // imported door's animated physics is exercised after asset readiness in PIE.
 if(TestNotNull(TEXT("Actual first-office door exists"),Door)){TestTrue(TEXT("Room door starts open"),Door->IsOpen());TestTrue(TEXT("Authored open leaf stays inside the office rather than the corridor"),Door->Bounds.GetBox().Max.X<-2590);}
 World->DestroyWorld(false);return true;
}
#endif
