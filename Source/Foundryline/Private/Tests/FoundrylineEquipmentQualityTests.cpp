#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineMachineMotion.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentQualityCycles,"Foundryline.Production.QualityMechanicalCycles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEquipmentQualityCycles::RunTest(const FString&){
 TestTrue(TEXT("Press separates rapid approach from slow pressing"),FMath::IsNearlyEqual(FFoundrylineMachineMotion::BrakeStroke(.30f),-26.f,.001)&&FMath::IsNearlyEqual(FFoundrylineMachineMotion::BrakeStroke(.45f),-31.5f,.001));TestEqual(TEXT("Pressure dwell physically holds tooling"),FFoundrylineMachineMotion::BrakeStroke(.5f),FFoundrylineMachineMotion::BrakeStroke(.55f));
 for(float P:{.12f,.30f,.45f,.58f,.85f})TestTrue(TEXT("Hydraulic stroke has no discontinuous position jumps"),FMath::Abs(FFoundrylineMachineMotion::BrakeStroke(P+.0001f)-FFoundrylineMachineMotion::BrakeStroke(P-.0001f))<.02);
 const auto Pierce=AFoundrylinePlasmaTable::EvaluateCycle(5);TestTrue(TEXT("Plasma pierces stationary before cutting contour"),Pierce.bCutting&&Pierce.Tip.Equals(AFoundrylinePlasmaTable::EvaluateCycle(5.1).Tip,.0001));TestTrue(TEXT("Torch retracts after arc stops"),!AFoundrylinePlasmaTable::EvaluateCycle(25.4).bCutting&&AFoundrylinePlasmaTable::EvaluateCycle(25.4).Tip.Z>Pierce.Tip.Z);
 for(double T:{4.,4.8,5.25,10.125,15.,19.875,24.75,25.75})TestTrue(TEXT("Rapid, pierce, contour corners and retract join continuously"),FVector::Dist(AFoundrylinePlasmaTable::EvaluateCycle(T-.0001).Tip,AFoundrylinePlasmaTable::EvaluateCycle(T+.0001).Tip)<.02);
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AFoundrylinePlasmaTable>();A->bFactoryAutomation=true;A->DispatchBeginPlay();A->SetFactoryPresentation(.3,true,true,true);A->Tick(.033);TArray<UInstancedStaticMeshComponent*> C;A->GetComponents(C);UInstancedStaticMeshComponent* Chain=nullptr;for(auto* X:C)if(X->GetFName()==TEXT("ArticulatedEnergyChain"))Chain=X;
 if(TestNotNull(TEXT("Plasma has articulated physical return chain"),Chain)){TestEqual(TEXT("Link count is bounded"),Chain->GetInstanceCount(),64);FTransform Before,After;Chain->GetInstanceTransform(60,Before);A->SetFactoryPresentation(.6,true,true,true);A->Tick(.033);Chain->GetInstanceTransform(60,After);TestTrue(TEXT("Chain actually folds as carriage traverses"),!Before.Equals(After,.001));A->SetFactoryPresentation(.6,false,true,true);A->Tick(1);FTransform Frozen;Chain->GetInstanceTransform(60,Frozen);TestTrue(TEXT("Pause retains real chain links"),Frozen.Equals(After,.001));TestEqual(TEXT("Cosmetic chain does not obstruct the player"),Chain->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}
 W->DestroyWorld(false);return true;
}
#endif
