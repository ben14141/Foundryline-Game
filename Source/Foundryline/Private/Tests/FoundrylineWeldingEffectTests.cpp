#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Production/FoundrylineWeldingStation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineWeldingEffectTest,"Foundryline.Production.WeldingEffectLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineWeldingEffectTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AActor>();auto* E=NewObject<UFoundrylineWeldingEffectComponent>(A);A->AddInstanceComponent(E);E->RegisterComponent();A->DispatchBeginPlay();
 TestFalse(TEXT("Starts inactive"),E->IsWelding());E->SetWelding(true);TestTrue(TEXT("Authority starts welding"),E->IsWelding());
 for(int32 I=0;I<30;++I)E->TickComponent(1.f/30,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Sparks remain bounded"),E->GetVisibleSparkCount()<=UFoundrylineWeldingEffectComponent::MaxSparks);TestTrue(TEXT("Active weld emits sparks"),E->GetVisibleSparkCount()>80);
 E->SetWelding(false);TestEqual(TEXT("Stopping clears sparks immediately"),E->GetVisibleSparkCount(),0);
 E->TickComponent(1,LEVELTICK_All,nullptr);TestEqual(TEXT("Inactive effect remains empty"),E->GetVisibleSparkCount(),0);
 auto* Station=W->SpawnActor<AFoundrylineWeldingStation>();Station->DispatchBeginPlay();
 TArray<UStaticMeshComponent*> Meshes;Station->GetComponents(Meshes);
 for(auto* Gun:Meshes)if(Gun->GetFName()==TEXT("MIGGun")){
  TestTrue(TEXT("Imported gun retains the saved WeldTip socket"),Gun->DoesSocketExist(TEXT("WeldTip")));
  TestTrue(TEXT("Effect is at the nozzle socket"),Station->WeldingEffect->GetComponentLocation().Equals(Gun->GetSocketLocation(TEXT("WeldTip")),.01));
  TestTrue(TEXT("Nozzle effect is offset from the hand grip"),FVector::Dist(Station->WeldingEffect->GetComponentLocation(),Gun->GetComponentLocation())>20);
 }
 W->DestroyWorld(false);return true;
}
#endif
