#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
// Bench and fixture kit are the original models from main (Docs/Architecture/ORIGINAL_MODEL_ASSETS.md), both on floor
// datums; a checkout without Content falls back to a plain bench block.
void AFoundrylineFactoryPlant::RebuildWeldingStations(){
 using Layout=FFoundrylineWeldingStationLayout;
 FString Key;for(const auto& S:Snapshot.WeldingStations)Key+=FString::Printf(TEXT("%d:%s:%d:%d;"),S.Id,*S.Center.ToString(),S.QuarterTurns,S.bSwapSides?1:0);
 if(Key==StationLayoutKey)return;StationLayoutKey=Key;
 for(UStaticMeshComponent* C:StationMeshes)if(C)C->DestroyComponent();StationMeshes.Reset();for(UBoxComponent* C:StationCollision)if(C)C->DestroyComponent();StationCollision.Reset();
 if(Snapshot.WeldingStations.IsEmpty())return;
 const bool Render=GetNetMode()!=NM_DedicatedServer;
 auto* Bench=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_StandardWeldingBench.SM_StandardWeldingBench")):nullptr;
 auto* Fixture=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureKit.SM_WeldingFixtureKit")):nullptr;
 auto* Cube=Render&&!Bench?LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")):nullptr;
 auto* Paint=Cube?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_FoundryOrange.M_MH_FoundryOrange")):nullptr;
 for(const auto& S:Snapshot.WeldingStations){const FTransform T=Layout::BenchTransform(S);
  // Model-aligned body over the 8 x 4 ft top; worker clearances and both ports stay open.
  auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(FVector(4*Layout::Foot,2*Layout::Foot,Layout::BenchHeightCm/2));Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);
  Box->SetRelativeLocationAndRotation(T.GetLocation()+FVector(0,0,Layout::BenchHeightCm/2),T.Rotator());StationCollision.Add(Box);
  if(!Render)continue;
  auto Mesh=[&](UStaticMesh* M,const FTransform& At){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetRelativeTransform(At);StationMeshes.Add(C);return C;};
  if(Bench)Mesh(Bench,T);else if(Cube){auto* C=Mesh(Cube,FTransform(T.Rotator(),T.GetLocation()+FVector(0,0,Layout::BenchHeightCm/2),FVector(8*Layout::Foot,4*Layout::Foot,Layout::BenchHeightCm)/100));if(Paint)C->SetMaterial(0,Paint);}
  // The fixture kit is authored in its stored pose on the bench's lower shelf, so it shares the bench transform.
  if(Fixture)Mesh(Fixture,T);
 }
 // One welding machine per welder side, each with its own holstered gun and coiled leads (same offsets as zone welders).
 auto* Machine=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Machine.SM_Welder_Machine")):nullptr;
 auto* Gun=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_MIGGun.SM_Welder_MIGGun")):nullptr;
 auto* Leads=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Leads.SM_Welder_Leads")):nullptr;
 for(const auto& S:Snapshot.WeldingStations)for(int Side=0;Side<2;++Side){const FTransform T=Layout::MachineTransform(S,Side);const FVector Size=Layout::MachineSizeCm();
  auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(Size/2);Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);
  Box->SetRelativeLocationAndRotation(T.GetLocation()+FVector(0,0,Size.Z/2),T.Rotator());StationCollision.Add(Box);
  if(!Render)continue;
  auto Mesh=[&](UStaticMesh* M,const FTransform& At){if(!M)return;auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetRelativeTransform(At);StationMeshes.Add(C);};
  Mesh(Machine,T);Mesh(Leads,T);Mesh(Gun,FTransform(T.Rotator(),T.TransformPosition(FVector(17,-38,87))));
 }
}
