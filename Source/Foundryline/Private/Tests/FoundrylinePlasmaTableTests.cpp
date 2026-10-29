#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePlasmaTest,"Foundryline.Production.PlasmaCycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylinePlasmaTest::RunTest(const FString&){
 for(int32 Sample=0;Sample<290;++Sample){
  const double T=Sample/10.;
  auto P=AFoundrylinePlasmaTable::EvaluateCycle(T);
  TestTrue(TEXT("Tool remains inside standard stock"),FMath::Abs(P.Tip.X)<76.2&&FMath::Abs(P.Tip.Y)<152.4);
  TestEqual(TEXT("Only stationary piercing and the contour emit"),P.bCutting,T>=4.8&&T<24.75);
  if(P.bCutting)TestTrue(TEXT("Cutting tip is at sheet height"),FMath::IsNearlyEqual(P.Tip.Z,100.95));
  auto Next=AFoundrylinePlasmaTable::EvaluateCycle(T+.01);
  TestTrue(TEXT("No gantry teleport at phase boundaries"),FVector::Distance(P.Tip,Next.Tip)<.5);
 }
 auto A=AFoundrylinePlasmaTable::EvaluateCycle(8),B=AFoundrylinePlasmaTable::EvaluateCycle(37);
 TestTrue(TEXT("Cycle repeats deterministically for late join"),A.Tip.Equals(B.Tip,.001));
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Table=W->SpawnActor<AFoundrylinePlasmaTable>();Table->DispatchBeginPlay();
 TestTrue(TEXT("Authority starts demonstration"),Table->IsOperating());
 TArray<UStaticMeshComponent*> Meshes;Table->GetComponents(Meshes);
 for(auto* Mesh:Meshes)if(Mesh->GetFName()==TEXT("Torch")){
  TestTrue(TEXT("Imported torch loaded"),Mesh->GetStaticMesh()!=nullptr);
  if(Mesh->GetStaticMesh()){
   const auto Bounds=Mesh->GetStaticMesh()->GetBounds();const FVector Tip=AFoundrylinePlasmaTable::AuthoredTorchTip();
   TestTrue(TEXT("Arc X aligns with actual imported nozzle"),FMath::Abs(Bounds.Origin.X-Tip.X)<.01);
   TestTrue(TEXT("Arc Y accounts for FBX handedness"),FMath::Abs(Bounds.Origin.Y-Tip.Y)<.01);
   TestTrue(TEXT("Arc sits just below imported nozzle face"),FMath::Abs(Bounds.Origin.Z-Bounds.BoxExtent.Z-Tip.Z-.05)<.01);
  }
 }
 Table->PlasmaEffect->SetLocalPlasmaActive(true);
 for(int I=0;I<15;++I)Table->PlasmaEffect->TickComponent(1.f/30,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Plasma emits sparks"),Table->PlasmaEffect->GetVisibleSparkCount()>0);
 TestTrue(TEXT("Plasma spark count is bounded"),Table->PlasmaEffect->GetVisibleSparkCount()<=256);
 Table->SetOperating(false);TestFalse(TEXT("Authority stops cycle"),Table->IsOperating());
 TestEqual(TEXT("Stop removes sparks"),Table->PlasmaEffect->GetVisibleSparkCount(),0);
 W->DestroyWorld(false);return true;
}
#endif
