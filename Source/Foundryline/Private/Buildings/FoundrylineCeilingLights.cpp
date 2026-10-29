#include "Construction/FoundrylineBuildSite.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void AFoundrylineBuildSite::AddCeilingLights(AActor* Actor,const FFoundrylinePlacedObject& Object,FVector Size)
{
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,Object.bOfficeAnnex?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Game/Factory/Meshes/Props/Light/SM_Lamp_03.SM_Lamp_03"));if(!Mesh) return;
 const bool HighBay=Size.Z>=1220;
 const float Spacing=Object.bOfficeAnnex?400.f:HighBay?1200.f:800.f;
 TArray<FBox2D> Zones;
 if(Object.bOfficeAnnex)
 {
  TArray<FBox2D> Open={FBox2D(FVector2D::ZeroVector,FVector2D(Size.X,Size.Y))};
  for(const auto& Room:Object.OfficeRooms)
  {
   FBox2D R(FVector2D(Room.Min),FVector2D(Room.Max));Zones.Add(R);TArray<FBox2D> Remaining;
   for(const auto& Area:Open)
   {
    const auto Low=Area.Min.ComponentMax(R.Min),High=Area.Max.ComponentMin(R.Max);
    if(Low.X>=High.X || Low.Y>=High.Y) {Remaining.Add(Area);continue;}
    auto Add=[&](FVector2D A,FVector2D B){if(B.X>A.X && B.Y>A.Y) Remaining.Add(FBox2D(A,B));};
    Add(Area.Min,{Low.X,Area.Max.Y});Add({High.X,Area.Min.Y},Area.Max);
    Add({Low.X,Area.Min.Y},{High.X,Low.Y});Add({Low.X,High.Y},{High.X,Area.Max.Y});
   }
   Open=MoveTemp(Remaining);
  }
  Zones.Append(Open);
 }
 else Zones.Add(FBox2D(FVector2D::ZeroVector,FVector2D(Size.X,Size.Y)));
 const auto Bounds=Mesh->GetBoundingBox();
 auto* Fixtures=NewObject<UInstancedStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Fixtures);Fixtures->SetupAttachment(Actor->GetRootComponent());Fixtures->SetStaticMesh(Mesh);if(Object.bOfficeAnnex) Fixtures->SetMaterial(0,Materials.FindRef(TEXT("Metal")));
 Fixtures->ComponentTags={TEXT("FoundrylineRoof"),TEXT("FoundrylineCeilingFixture")};Fixtures->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fixtures->SetCanEverAffectNavigation(false);Fixtures->SetCastShadow(false);Fixtures->SetVisibility(bRoofsVisible);Fixtures->RegisterComponent();
 auto* Glow=NewObject<UInstancedStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Glow);Glow->SetupAttachment(Actor->GetRootComponent());Glow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Glow->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Buildings/Lighting/M_CeilingGlow.M_CeilingGlow")));Glow->ComponentTags={TEXT("FoundrylineRoof"),TEXT("FoundrylineCeilingGlow")};Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);Glow->SetCanEverAffectNavigation(false);Glow->SetCastShadow(false);Glow->SetAffectDistanceFieldLighting(false);Glow->SetVisibleInRayTracing(false);Glow->SetVisibility(bRoofsVisible);Glow->RegisterComponent();
 UInstancedStaticMeshComponent* Supports=nullptr;
 if(Object.bOfficeAnnex) {Supports=NewObject<UInstancedStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Supports);Supports->SetupAttachment(Actor->GetRootComponent());Supports->SetStaticMesh(Mesh);Supports->SetMaterial(0,Materials.FindRef(TEXT("Metal")));Supports->ComponentTags={TEXT("FoundrylineRoof")};Supports->SetCollisionEnabled(ECollisionEnabled::NoCollision);Supports->SetCanEverAffectNavigation(false);Supports->SetCastShadow(false);Supports->SetVisibility(bRoofsVisible);Supports->RegisterComponent();}
 for(const auto& Zone:Zones)
 {
 const int32 NX=FMath::Max(1,FMath::RoundToInt(Zone.GetSize().X/Spacing)),NY=FMath::Max(1,FMath::RoundToInt(Zone.GetSize().Y/Spacing));
 for(int32 X=0;X<NX;++X) for(int32 Y=0;Y<NY;++Y)
 {
  const FVector Position(Zone.Min.X+(X+.5f)*Zone.GetSize().X/NX,Zone.Min.Y+(Y+.5f)*Zone.GetSize().Y/NY,Object.bOfficeAnnex?Size.Z-34:Size.Z-20-Bounds.Max.Z);
  Glow->AddInstance(FTransform(FQuat::Identity,Position+FVector(0,0,Object.bOfficeAnnex?-5:Bounds.Min.Z-2),FVector(Object.bOfficeAnnex?1.1f:2.2f,Object.bOfficeAnnex?.22f:.15f,.015f)));
  Fixtures->AddInstance(FTransform(FQuat::Identity,Position,Object.bOfficeAnnex?FVector(1.2f,.3f,.08f):FVector::OneVector)); // Native fixture dimensions, suspension top below ceiling.
  if(Supports) for(float Side:{-45.f,45.f}) Supports->AddInstance(FTransform(FQuat::Identity,FVector(Position.X+Side,Position.Y,Size.Z-25),FVector(.015,.015,.1)));
  auto* Light=NewObject<URectLightComponent>(Actor);Actor->AddInstanceComponent(Light);Light->SetupAttachment(Actor->GetRootComponent());Light->SetMobility(EComponentMobility::Movable);
  Light->ComponentTags={TEXT("FoundrylineRoof"),TEXT("FoundrylineCeilingLight")};
  Light->SetRelativeLocationAndRotation(Position+FVector(0,0,Object.bOfficeAnnex?-7:Bounds.Min.Z-5),FRotator(-90,0,0));
  Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(Object.bOfficeAnnex?5000.f:HighBay?100000.f:30000.f);
  Light->SetUseTemperature(true);Light->SetTemperature(Object.bOfficeAnnex?4300.f:4500.f);
  Light->SetSourceWidth(Object.bOfficeAnnex?110:220);Light->SetSourceHeight(Object.bOfficeAnnex?22:20);
  Light->SetAttenuationRadius(Object.bOfficeAnnex?550.f:HighBay?1700.f:1000.f);
  Light->SetCastShadows(true);Light->SetVisibility(bRoofsVisible);Light->RegisterComponent();
 }
 }
}
