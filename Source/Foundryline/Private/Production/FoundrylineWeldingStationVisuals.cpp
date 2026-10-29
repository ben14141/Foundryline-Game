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
 // Parts bay rack beside IN (frame cradle below, three bins on the shelf) and the steel OUT cradle, built from painted blocks.
 auto* Block=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")):nullptr;
 auto* Orange=Render?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_FoundryOrange.M_MH_FoundryOrange")):nullptr;
 auto* Steel=Render?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_Metal.M_Metal")):nullptr;
 for(const auto& S:Snapshot.WeldingStations){const FRotator Yaw=Layout::Rotation(S);
  auto Collide=[&](FVector Center,FVector Extent,FRotator Rot){auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(Extent);Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);Box->SetRelativeLocationAndRotation(Center,Rot);StationCollision.Add(Box);};
  const FBox2D Bay=Layout::PartsBay(S);const FVector BayCenter(Bay.GetCenter(),20);

  Collide(BayCenter+FVector(0,0,50),FVector(Bay.GetSize()/2,50),FRotator::ZeroRotator);
  const FTransform Cradle=Layout::CradleTransform(S);const FVector C=Layout::CradleSizeCm();Collide(Cradle.GetLocation()+FVector(0,0,C.Z/2),C/2,Cradle.Rotator());
  if(!Block)continue;
  // Box in a local frame: centre and size in cm, turned with that frame.
  auto Part=[&](const FTransform& Frame,FVector Center,FVector Size,UMaterialInterface* M){auto* Mc=NewObject<UStaticMeshComponent>(this);Mc->SetupAttachment(RootComponent);Mc->SetStaticMesh(Block);Mc->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mc->SetCanEverAffectNavigation(false);Mc->RegisterComponent();AddInstanceComponent(Mc);Mc->SetRelativeTransform(FTransform(Frame.Rotator(),Frame.TransformPosition(Center),Size/100));if(M)Mc->SetMaterial(0,M);StationMeshes.Add(Mc);};
  const FTransform BayFrame(Yaw,BayCenter);const double HX=Layout::Foot*4.5/2-4,HY=Layout::Foot*2.5/2-4,Shelf=Layout::BinShelfCm;
  for(int Corner=0;Corner<4;++Corner)Part(BayFrame,FVector(Corner&1?HX:-HX,Corner&2?HY:-HY,52),FVector(5,5,104),Orange);
  Part(BayFrame,FVector(0,0,Shelf-1.5),FVector(2*HX+5,2*HY+5,3),Steel);
  for(double X:{-21.,21.})Part(BayFrame,FVector(X,0,Shelf+6),FVector(1,2*HY,12),Steel);
  for(double X:{-45.,45.})Part(BayFrame,FVector(X,0,6),FVector(8,2*HY,12),Orange);
  for(double Y:{-40.,40.})Part(Cradle,FVector(0,Y,C.Z/2),FVector(C.X,10,C.Z),Orange);
  for(double X:{-100.,0.,100.})Part(Cradle,FVector(X,0,5),FVector(10,C.Y,10),Steel);
 }
}

