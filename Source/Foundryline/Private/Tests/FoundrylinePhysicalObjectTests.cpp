#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Production/FoundrylineFactoryPlant.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicalObjectWalking,"Foundryline.Production.PhysicalObjectWalking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPhysicalObjectWalking::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AFoundrylineFactoryPlant>();A->bCutPartsStarter=true;A->DispatchBeginPlay();A->Model.State.Running=false;A->Tick(.01f);
 auto Find=[&](const TCHAR* Tag){TArray<UBoxComponent*> Boxes;A->GetComponents(Boxes);for(auto* C:Boxes)if(C->ComponentHasTag(Tag))return C;return static_cast<UBoxComponent*>(nullptr);};
 auto* Body=Find(TEXT("Fork0_Chassis"));if(TestNotNull(TEXT("Allocated forklift has tagged chassis"),Body)){
 const auto* C=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();auto Shape=FCollisionShape::MakeCapsule(C->GetUnscaledCapsuleRadius(),C->GetUnscaledCapsuleHalfHeight());auto T=Body->GetComponentTransform();FHitResult H;FCollisionQueryParams Q;TArray<UBoxComponent*> Boxes;A->GetComponents(Boxes);for(auto* B:Boxes)if(B!=Body)Q.AddIgnoredComponent(B);
 TestTrue(TEXT("Real player capsule blocked by parked forklift chassis"),W->SweepSingleByChannel(H,T.TransformPosition({0,-250,0}),T.TransformPosition({0,0,0}),FQuat::Identity,ECC_Pawn,Shape,Q));
 TestFalse(TEXT("Walking clear beside forklift has no giant invisible box"),W->SweepSingleByChannel(H,T.TransformPosition({-400,-250,0}),T.TransformPosition({-400,250,0}),FQuat::Identity,ECC_Pawn,Shape,Q));
 }
 A->Snapshot.Drivers.Reset();A->Tick(.01f);if(Body)TestTrue(TEXT("Unused forklift pool cannot leave invisible chassis"),Body->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
 A->Snapshot.Pallets.Reset();FFoundrylineFactoryPallet P;P.Id=9001;P.Part=EFoundrylinePart::Sheet;P.Quantity=10;P.Place=EFoundrylinePalletPlace::Receiving;P.Position={9000,9000,20};A->Snapshot.Pallets.Add(P);A->Tick(.01f);auto* Pal=Find(TEXT("Pallet9001_Physical"));TestNotNull(TEXT("Physical displayed stock gets pooled pallet bounds"),Pal);if(Pal){TestTrue(TEXT("Sheet/support union matches long stock instead of a tiny default pallet"),Pal->GetUnscaledBoxExtent().Y>100);TestTrue(TEXT("Thin steel pile remains honestly step-height, not fake tall wall"),Pal->GetUnscaledBoxExtent().Z<25);}
 A->Snapshot.Pallets[0].Place=EFoundrylinePalletPlace::InTransit;A->Tick(.01f);TestNull(TEXT("Offsite pallet destroys old world blocker"),Find(TEXT("Pallet9001_Physical")));
 W->DestroyWorld(false);return true;
}
#endif
