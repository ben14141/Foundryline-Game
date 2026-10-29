#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Components/RectLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineCeilingLightTest,"Foundryline.Buildings.AutomaticCeilingLights",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineCeilingLightTest::RunTest(const FString&)
{
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->bLargeShowcase=true;Site->DispatchBeginPlay();
 Site->SetRoofsVisible(true);int32 Low=0,High=0,Fixtures=0;
 for(TActorIterator<AActor> It(World);It;++It) if(!It->IsActorBeingDestroyed())
 {
  TInlineComponentArray<URectLightComponent*> Lights;It->GetComponents(Lights);
  for(auto* Light:Lights) if(Light->ComponentHasTag(TEXT("FoundrylineCeilingLight")))
  {
   TestTrue(TEXT("Ceiling lights show with roofs"),Light->IsVisible());
   TestTrue(TEXT("Walls block artificial light"),Light->CastShadows);
   TestEqual(TEXT("Fixture width follows native linear lamp"),Light->SourceWidth,Light->GetRelativeLocation().Z<300?110.f:220.f);
   if(Light->GetRelativeLocation().Z>900) {++High;TestEqual(TEXT("High bay output"),Light->Intensity,100000.f);}
   else ++Low;
  }
  TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
  for(auto* C:Meshes) if(C->ComponentHasTag(TEXT("FoundrylineCeilingFixture")))
   for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T);TestTrue(TEXT("Hanging lamps retain native 1:1 dimensions"),T.GetScale3D().Equals(C->GetStaticMesh()->GetName()==TEXT("Cube")?FVector(1.2,.3,.08):FVector::OneVector));++Fixtures;}
 }
 TestTrue(TEXT("Both tiers receive evenly distributed lighting"),Low>0 && High>0);TestEqual(TEXT("One light per fixture"),Fixtures,Low+High);
 Site->SetRoofsVisible(false);
 for(TActorIterator<AActor> It(World);It;++It) if(!It->IsActorBeingDestroyed()) {TInlineComponentArray<URectLightComponent*> Lights;It->GetComponents(Lights);for(auto* Light:Lights) if(Light->ComponentHasTag(TEXT("FoundrylineCeilingLight"))) TestFalse(TEXT("Cutaway hides ceiling lights with roofs"),Light->IsVisible());}
 World->DestroyWorld(false);return true;
}
