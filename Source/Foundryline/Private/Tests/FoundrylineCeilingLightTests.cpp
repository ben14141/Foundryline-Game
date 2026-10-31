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
 Site->SetRoofsVisible(true);int32 Low=0,High=0,Fixtures=0,OfficeLights=0;
 for(TActorIterator<AActor> It(World);It;++It) if(!It->IsActorBeingDestroyed())
 {
  TInlineComponentArray<URectLightComponent*> Lights;It->GetComponents(Lights);
  for(auto* Light:Lights) if(Light->ComponentHasTag(TEXT("FoundrylineCeilingLight")))
  {
   TestTrue(TEXT("Ceiling lights show with roofs"),Light->IsVisible());
   TestTrue(TEXT("Walls block artificial light"),Light->CastShadows);
   if(Light->ComponentHasTag(TEXT("FoundrylineRecessedOfficeLight"))){++OfficeLights;TestTrue(TEXT("Office source fits a standard 2 by 4 foot panel"),FMath::IsNearlyEqual(Light->SourceWidth,54.96f,.01f)||FMath::IsNearlyEqual(Light->SourceWidth,115.92f,.01f));TestEqual(TEXT("Office neutral white light"),Light->Temperature,4000.f);TestEqual(TEXT("Office panel output"),Light->Intensity,5000.f);}
   else TestEqual(TEXT("Shop fixture retains native linear lamp width"),Light->SourceWidth,220.f);
   if(Light->GetRelativeLocation().Z>900) {++High;TestEqual(TEXT("High bay output"),Light->Intensity,100000.f);}
   else ++Low;
  }
  TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
  for(auto* C:Meshes) if(C->ComponentHasTag(TEXT("FoundrylineCeilingFixture")))
   for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T);if(C->ComponentHasTag(TEXT("FoundrylineRecessedOfficeLight"))){if(FMath::IsNearlyEqual(T.GetScale3D().Z,.03,.001)){const auto S=T.GetScale3D()*100;TestTrue(TEXT("Office housing has standard fixed footprint"),FMath::IsNearlyEqual(S.GetMin(),3.,.01)&&FMath::IsNearlyEqual(FMath::Max(S.X,S.Y),121.92,.01));++Fixtures;}}else{TestTrue(TEXT("Hanging shop lamps retain native 1:1 dimensions"),T.GetScale3D().Equals(FVector::OneVector));++Fixtures;}}
  for(auto* Lens:Meshes)if(Lens->ComponentHasTag(TEXT("FoundrylineCeilingGlow"))&&Lens->ComponentHasTag(TEXT("FoundrylineRecessedOfficeLight")))for(int I=0;I<Lens->GetInstanceCount();++I){FTransform L;Lens->GetInstanceTransform(I,L);const auto Bottom=L.GetLocation()-FVector(0,0,L.GetScale3D().Z*50);for(auto* Frame:Meshes)if(Frame->ComponentHasTag(TEXT("FoundrylineCeilingFixture")))for(int J=0;J<Frame->GetInstanceCount();++J){FTransform F;Frame->GetInstanceTransform(J,F);const FVector Half=F.GetScale3D()*50;TestFalse(TEXT("Diffuser center is visible through the open housing"),FBox(F.GetLocation()-Half,F.GetLocation()+Half).IsInside(Bottom));}}
 }
 TestTrue(TEXT("Offices receive recessed lights"),OfficeLights>0);
 TestTrue(TEXT("Both tiers receive evenly distributed lighting"),Low>0 && High>0);TestEqual(TEXT("One light per fixture"),Fixtures,Low+High);
 Site->SetRoofsVisible(false);
 for(TActorIterator<AActor> It(World);It;++It) if(!It->IsActorBeingDestroyed()) {TInlineComponentArray<URectLightComponent*> Lights;It->GetComponents(Lights);for(auto* Light:Lights) if(Light->ComponentHasTag(TEXT("FoundrylineCeilingLight"))) TestFalse(TEXT("Cutaway hides ceiling lights with roofs"),Light->IsVisible());}
 World->DestroyWorld(false);return true;
}
