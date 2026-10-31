#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
UFoundrylineWeldingEffectComponent::UFoundrylineWeldingEffectComponent(){bAutoActivate=true;PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=true;PrimaryComponentTick.TickInterval=1.f/30;SetIsReplicatedByDefault(true);}
void UFoundrylineWeldingEffectComponent::BeginPlay(){
 Super::BeginPlay();SetComponentTickEnabled(GetNetMode()!=NM_DedicatedServer);if(GetNetMode()==NM_DedicatedServer)return;
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/VFX/M_WeldingGlow.M_WeldingGlow"));
 Arc=NewObject<UStaticMeshComponent>(GetOwner());Arc->SetupAttachment(this);Arc->SetStaticMesh(Sphere);Arc->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arc->SetCastShadow(false);Arc->SetCanEverAffectNavigation(false);Arc->SetRelativeScale3D(FVector(.04f));Arc->RegisterComponent();
 if(Material){ArcMaterial=UMaterialInstanceDynamic::Create(Material,this);ArcMaterial->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(.65f,.82f,1.f));ArcMaterial->SetScalarParameterValue(TEXT("Strength"),180000);Arc->SetMaterial(0,ArcMaterial);}
 Sparks=NewObject<UInstancedStaticMeshComponent>(GetOwner());Sparks->SetupAttachment(this);Sparks->SetStaticMesh(Sphere);Sparks->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sparks->SetCastShadow(false);Sparks->SetCanEverAffectNavigation(false);Sparks->NumCustomDataFloats=4;Sparks->RegisterComponent();
 auto* SparkMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/VFX/M_WeldingSpark.M_WeldingSpark"));
 if(SparkMaterial)Sparks->SetMaterial(0,SparkMaterial);
 else if(Material){auto* M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(1,.35f,.015f));M->SetScalarParameterValue(TEXT("Strength"),10000);Sparks->SetMaterial(0,M);}
 Light=NewObject<UPointLightComponent>(GetOwner());Light->SetupAttachment(this);Light->SetLightColor(FLinearColor(.52f,.72f,1));Light->SetIntensityUnits(ELightUnits::Candelas);Light->SetIntensity(125000);Light->SetAttenuationRadius(900);Light->SetCastShadows(false);Light->SetSourceRadius(2.5);Light->SetIndirectLightingIntensity(.25);Light->RegisterComponent();OnRep_Welding();
}
void UFoundrylineWeldingEffectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UFoundrylineWeldingEffectComponent,bWelding);}
void UFoundrylineWeldingEffectComponent::SetWelding(bool Active){if(!GetOwner() || !GetOwner()->HasAuthority() || bWelding==Active)return;bWelding=Active;OnRep_Welding();GetOwner()->ForceNetUpdate();}
void UFoundrylineWeldingEffectComponent::ResetVisuals(){Particles.Reset();SpawnBudget=0;if(Sparks)Sparks->ClearInstances();if(Arc)Arc->SetVisibility(false);if(Light)Light->SetVisibility(false);}
void UFoundrylineWeldingEffectComponent::OnRep_Welding(){if(!IsWelding())ResetVisuals();}
void UFoundrylineWeldingEffectComponent::SetLocalActivity(bool Active){bUseLocalActivity=true;bLocalPlasmaActive=Active;if(!Active)ResetVisuals();}
void UFoundrylineWeldingEffectComponent::SetLocalPlasmaActive(bool Active){if(bPlasmaCutting)SetLocalActivity(Active);}
int32 UFoundrylineWeldingEffectComponent::GetVisibleSparkCount()const{return Sparks?Sparks->GetInstanceCount():0;}
void UFoundrylineWeldingEffectComponent::EndPlay(const EEndPlayReason::Type Reason){
 ResetVisuals();if(Arc)Arc->DestroyComponent();if(Sparks)Sparks->DestroyComponent();if(Light)Light->DestroyComponent();Super::EndPlay(Reason);
}
void UFoundrylineWeldingEffectComponent::UpdateFloor(float Dt){
 FloorQueryClock-=Dt;if(FloorQueryClock>0)return;FloorQueryClock=.35f;bHasFloor=false;
 // One shared surface query per emitter, not a physics body or network message per spark.
 // The owner holds the bench/gun geometry; ignore it to find the actual slab below the work.
 FCollisionQueryParams Query(SCENE_QUERY_STAT(WeldingFloor),false,GetOwner());FHitResult Hit;
 const FVector Origin=GetComponentLocation();
 if(GetWorld()->LineTraceSingleByChannel(Hit,Origin+FVector(0,0,30),Origin-FVector(0,0,1500),ECC_Visibility,Query)&&Hit.ImpactNormal.Z>.7f){
  FloorPoint=Hit.ImpactPoint;FloorNormal=Hit.ImpactNormal;FloorBounds=Hit.GetComponent()?Hit.GetComponent()->Bounds.GetBox():FBox(ForceInit);bHasFloor=FloorBounds.IsValid!=0;
 }
}
void UFoundrylineWeldingEffectComponent::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F){
 Super::TickComponent(Dt,T,F);if(!Arc || !Sparks || !Light)return;
 auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
 const float Distance=Camera?FVector::Distance(Camera->GetCameraLocation(),GetComponentLocation()):0;
 if(!IsWelding() || Distance>5500){ResetVisuals();return;}
 Dt=FMath::Clamp(Dt,0.f,.1f);Clock+=Dt;UpdateFloor(Dt);Arc->SetVisibility(true);Light->SetVisibility(true);
 const float Flicker=.86f+.09f*FMath::Sin(Clock*73)+.05f*FMath::Sin(Clock*117);
 Arc->SetRelativeScale3D((bPlasmaCutting?FVector(.018f,.018f,.045f):FVector(.025f))*Flicker);
 Light->SetWorldLocation(GetComponentLocation()+FVector(0,0,bPlasmaCutting?0.f:3.5f));
 Light->SetIntensity((bPlasmaCutting?3500.f:125000.f)*Flicker);Light->SetCastShadows(!bPlasmaCutting&&Distance<1800);
 if(ArcMaterial)ArcMaterial->SetScalarParameterValue(TEXT("Strength"),(bPlasmaCutting?65000.f:180000.f)*Flicker);
 const int32 Budget=Distance<1800?MaxSparks:Distance<3500?256:96;
 SpawnBudget+=Dt*(Distance<1800?700.f:Distance<3500?210.f:65.f);
 while(SpawnBudget>=1 && Particles.Num()<Budget){
  SpawnBudget-=1;FSpark P;P.Position=GetComponentLocation();const float Angle=FMath::FRandRange(0.f,2*PI),Speed=FMath::FRandRange(100.f,FMath::FRand()<.08f?460.f:300.f);
  P.Velocity=bPlasmaCutting?FVector(FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-420.f,-120.f)):FVector(FMath::Cos(Angle)*Speed,FMath::Sin(Angle)*Speed,FMath::FRandRange(-100.f,230.f));
  P.Life=FMath::FRandRange(1.1f,2.1f);P.Width=FMath::FRandRange(.0015f,.0035f);Particles.Add(P);
 }
 SpawnBudget=FMath::Min(SpawnBudget,1.f);TArray<FTransform> Transforms;TArray<FLinearColor> Colours;Transforms.Reserve(Particles.Num());Colours.Reserve(Particles.Num());
 for(int32 I=Particles.Num()-1;I>=0;--I){
  auto& P=Particles[I];P.Age+=Dt;if(P.Age>P.Life){Particles.RemoveAtSwap(I);continue;}
  // Integrate in world space so sparks stay behind as the torch travels along a seam.
  const FVector Previous=P.Position;P.Velocity*=FMath::Exp(-.32f*Dt);P.Velocity.Z-=980*Dt;P.Position+=P.Velocity*Dt;
  if(bHasFloor&&P.Position.X>=FloorBounds.Min.X&&P.Position.X<=FloorBounds.Max.X&&P.Position.Y>=FloorBounds.Min.Y&&P.Position.Y<=FloorBounds.Max.Y){
   const float Height=FVector::DotProduct(P.Position-FloorPoint,FloorNormal);
   if(Height<.2f){
    P.Position+=FloorNormal*(.2f-Height);const float Into=FVector::DotProduct(P.Velocity,FloorNormal);
    if(Into<0){if(!P.Bounces){++GroundImpacts;P.Heat*=.65f;P.Life=FMath::Min(P.Life,P.Age+FMath::FRandRange(.35f,.75f));}
     ++P.Bounces;const FVector Tangent=P.Velocity-FloorNormal*Into;P.Velocity=Tangent*.58f-FloorNormal*Into*(P.Bounces<3?.25f:0.f);
     if(P.Bounces>=3)P.Velocity=FVector::ZeroVector;
    }
   }
  }
  const float Fade=FMath::Clamp(1-P.Age/P.Life,0.f,1.f),Heat=Fade*P.Heat;
  const FVector Local=GetComponentTransform().InverseTransformPosition(P.Position),V=GetComponentTransform().InverseTransformVectorNoScale(P.Velocity);
  const float Length=FMath::Clamp(float((P.Position-Previous).Size())*.40f,.18f,12.f)/100.f;
  const float Width=P.Width*(.4f+.6f*FMath::Sqrt(Fade));
  Transforms.Add(FTransform(V.IsNearlyZero()?FQuat::Identity:V.ToOrientationQuat(),Local,FVector(Length,Width,Width)));
  Colours.Add(FLinearColor(1.f,FMath::Lerp(.025f,.60f,Heat*Heat),FMath::Lerp(.001f,.10f,Heat*Heat*Heat),FMath::Lerp(30.f,18000.f,FMath::Pow(Heat,1.6f))));
 }
 Sparks->ClearInstances();Sparks->AddInstances(Transforms,false,false,false);
 for(int32 I=0;I<Colours.Num();++I){const auto& C=Colours[I];Sparks->SetCustomDataValue(I,0,C.R,false);Sparks->SetCustomDataValue(I,1,C.G,false);Sparks->SetCustomDataValue(I,2,C.B,false);Sparks->SetCustomDataValue(I,3,C.A,false);}Sparks->MarkRenderStateDirty();
}
