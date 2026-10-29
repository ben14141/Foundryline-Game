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
UFoundrylineWeldingEffectComponent::UFoundrylineWeldingEffectComponent(){bAutoActivate=true;PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=true;PrimaryComponentTick.TickInterval=1.f/30;SetIsReplicatedByDefault(true);}
void UFoundrylineWeldingEffectComponent::BeginPlay(){
 Super::BeginPlay();SetComponentTickEnabled(GetNetMode()!=NM_DedicatedServer);if(GetNetMode()==NM_DedicatedServer)return;
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/VFX/M_WeldingGlow.M_WeldingGlow"));
 Arc=NewObject<UStaticMeshComponent>(GetOwner());Arc->SetupAttachment(this);Arc->SetStaticMesh(Sphere);Arc->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arc->SetCastShadow(false);Arc->SetCanEverAffectNavigation(false);Arc->SetRelativeScale3D(FVector(.04f));Arc->RegisterComponent();
 if(Material){ArcMaterial=UMaterialInstanceDynamic::Create(Material,this);ArcMaterial->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(.18f,.48f,1.f));ArcMaterial->SetScalarParameterValue(TEXT("Strength"),65000);Arc->SetMaterial(0,ArcMaterial);}
 Sparks=NewObject<UInstancedStaticMeshComponent>(GetOwner());Sparks->SetupAttachment(this);Sparks->SetStaticMesh(Sphere);Sparks->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sparks->SetCastShadow(false);Sparks->SetCanEverAffectNavigation(false);Sparks->RegisterComponent();
 if(Material){auto* M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(1,.45f,.035f));M->SetScalarParameterValue(TEXT("Strength"),16000);Sparks->SetMaterial(0,M);}
 Light=NewObject<UPointLightComponent>(GetOwner());Light->SetupAttachment(this);Light->SetLightColor(FLinearColor(.52f,.72f,1));Light->SetIntensityUnits(ELightUnits::Candelas);Light->SetIntensity(40000);Light->SetAttenuationRadius(600);Light->SetCastShadows(false);Light->SetSourceRadius(2.5);Light->SetIndirectLightingIntensity(.15);Light->RegisterComponent();OnRep_Welding();
}
void UFoundrylineWeldingEffectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UFoundrylineWeldingEffectComponent,bWelding);}
void UFoundrylineWeldingEffectComponent::SetWelding(bool Active){if(!GetOwner() || !GetOwner()->HasAuthority() || bWelding==Active)return;bWelding=Active;OnRep_Welding();GetOwner()->ForceNetUpdate();}
void UFoundrylineWeldingEffectComponent::ResetVisuals(){Particles.Reset();SpawnBudget=0;if(Sparks)Sparks->ClearInstances();if(Arc)Arc->SetVisibility(false);if(Light)Light->SetVisibility(false);}
void UFoundrylineWeldingEffectComponent::OnRep_Welding(){if(!IsWelding())ResetVisuals();}
void UFoundrylineWeldingEffectComponent::SetLocalActivity(bool Active){bUseLocalActivity=true;bLocalPlasmaActive=Active;if(!Active)ResetVisuals();}
void UFoundrylineWeldingEffectComponent::SetLocalPlasmaActive(bool Active){if(bPlasmaCutting)SetLocalActivity(Active);}
int32 UFoundrylineWeldingEffectComponent::GetVisibleSparkCount()const{return Sparks?Sparks->GetInstanceCount():0;}
void UFoundrylineWeldingEffectComponent::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F){
 Super::TickComponent(Dt,T,F);if(!Arc || !Sparks || !Light)return;
 auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
 const bool Nearby=!Camera || FVector::DistSquared(Camera->GetCameraLocation(),GetComponentLocation())<FMath::Square(5500.f);
 if(!IsWelding() || !Nearby){ResetVisuals();return;}
 Dt=FMath::Clamp(Dt,0.f,.1f);Clock+=Dt;Arc->SetVisibility(true);Light->SetVisibility(true);
 const float Flicker=.80f+.12f*FMath::Sin(Clock*73)+.08f*FMath::Sin(Clock*117);
 Arc->SetRelativeScale3D((bPlasmaCutting?FVector(.018f,.018f,.045f):FVector(.04f))*Flicker);Light->SetIntensity((bPlasmaCutting?3500:40000)*Flicker);if(ArcMaterial)ArcMaterial->SetScalarParameterValue(TEXT("Strength"),65000*Flicker);
 const float Distance=Camera?FVector::Distance(Camera->GetCameraLocation(),GetComponentLocation()):0;
 SpawnBudget+=Dt*(Distance<1800?450:90);
 while(SpawnBudget>=1 && Particles.Num()<MaxSparks){SpawnBudget-=1;FSpark P;P.Position=GetComponentLocation();P.Velocity=bPlasmaCutting?FVector(FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-180.f,180.f),FMath::FRandRange(-420.f,-120.f)):FVector(FMath::FRandRange(-250.f,250.f),FMath::FRandRange(-250.f,250.f),FMath::FRandRange(140.f,360.f));P.Life=FMath::FRandRange(.22f,.60f);P.Length=FMath::FRandRange(.035f,.075f);P.Width=FMath::FRandRange(.0045f,.008f);Particles.Add(P);}
 SpawnBudget=FMath::Min(SpawnBudget,1.f);Sparks->ClearInstances();
 for(int32 I=Particles.Num()-1;I>=0;--I){auto& P=Particles[I];P.Age+=Dt;if(P.Age>P.Life){Particles.RemoveAtSwap(I);continue;}P.Velocity.Z-=980*Dt;P.Position+=P.Velocity*Dt;
  const float Fade=1-P.Age/P.Life;const FVector Local=GetComponentTransform().InverseTransformPosition(P.Position);const FVector V=GetComponentTransform().InverseTransformVectorNoScale(P.Velocity);
  Sparks->AddInstance(FTransform(V.ToOrientationQuat(),Local,FVector(P.Length*Fade,P.Width*FMath::Sqrt(Fade),P.Width*FMath::Sqrt(Fade))));
 }
}
