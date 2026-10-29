#include "Construction/FoundrylineEquipmentCollision.h"
#include "Production/FoundrylineWeldingStation.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
AFoundrylineWeldingStation::AFoundrylineWeldingStation(){
 bReplicates=true;SetNetUpdateFrequency(2);SetNetCullDistanceSquared(FMath::Square(10000.f));SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Station")));
 Gun=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MIGGun"));Gun->SetupAttachment(RootComponent);Gun->SetRelativeLocationAndRotation(FVector(100,0,90),FRotator(-45,0,0));Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 WeldingEffect=CreateDefaultSubobject<UFoundrylineWeldingEffectComponent>(TEXT("WeldingEffect"));WeldingEffect->SetupAttachment(Gun,TEXT("WeldTip"));
}
void AFoundrylineWeldingStation::BeginPlay(){
 Super::BeginPlay();Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_MIGGun.SM_Welder_MIGGun")));
 // Mesh is loaded after component registration; refresh the attachment against its sockets.
 WeldingEffect->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 WeldingEffect->AttachToComponent(Gun,FAttachmentTransformRules::SnapToTargetNotIncludingScale,TEXT("WeldTip"));
 WeldingEffect->UpdateComponentToWorld();
 auto Mesh=[&](FName Name,const TCHAR* Path,FVector Location,FVector Scale){auto* M=NewObject<UStaticMeshComponent>(this,Name);M->SetupAttachment(RootComponent);M->SetRelativeLocation(Location);M->SetRelativeScale3D(Scale);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->RegisterComponent();AddInstanceComponent(M);if(Name!=TEXT("GunFixture")&&Name!=TEXT("WeldSample"))FFoundrylineEquipmentCollision::AddPawnBody(M);return M;};
 Mesh(TEXT("Machine"),TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Machine.SM_Welder_Machine"),FVector::ZeroVector,FVector::OneVector);
 auto* Top=Mesh(TEXT("Bench"),TEXT("/Engine/BasicShapes/Cube.Cube"),FVector(126,0,78.5),FVector(1,.8,.05));
 auto* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_DarkSteel.M_MH_DarkSteel"));if(Steel)Top->SetMaterial(0,Steel);
 auto* Plate=Mesh(TEXT("WeldSample"),TEXT("/Engine/BasicShapes/Cube.Cube"),FVector(126,0,81.65),FVector(.65,.45,.02));if(Steel)Plate->SetMaterial(0,Steel);
 for(int X:{90,162})for(int Y:{-28,28}){auto* L=Mesh(NAME_None,TEXT("/Engine/BasicShapes/Cube.Cube"),FVector(X,Y,38),FVector(.05,.05,.76));if(Steel)L->SetMaterial(0,Steel);}
 // Fixture supports the demonstration gun until worker-hand animation is connected.
 auto* Stand=Mesh(TEXT("GunFixture"),TEXT("/Engine/BasicShapes/Cube.Cube"),FVector(94,0,85),FVector(.035,.12,.12));if(Steel)Stand->SetMaterial(0,Steel);
 const FVector LeadPoints[]={{54,-12,53},{75,-45,30},{85,-65,4},{60,-70,4},{76,-55,75},{88,-40,88},{90.38f,0,82.65f}};
 auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Rubber.M_MH_Rubber"));
 for(int32 I=0;I<6;++I){auto* Lead=NewObject<USplineMeshComponent>(this);Lead->SetupAttachment(RootComponent);Lead->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Lead->SetForwardAxis(ESplineMeshAxis::Z,false);Lead->SetStartScale(FVector2D(.028f));Lead->SetEndScale(FVector2D(.028f));
  const FVector StartTangent=I==0?LeadPoints[1]-LeadPoints[0]:(LeadPoints[I+1]-LeadPoints[I-1])*.5;
  const FVector EndTangent=I==5?LeadPoints[6]-LeadPoints[5]:(LeadPoints[I+2]-LeadPoints[I])*.5;
  Lead->SetStartAndEnd(LeadPoints[I],StartTangent,LeadPoints[I+1],EndTangent);Lead->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lead->SetCanEverAffectNavigation(false);if(Rubber)Lead->SetMaterial(0,Rubber);Lead->RegisterComponent();AddInstanceComponent(Lead);
 }
 if(HasAuthority()){WeldingEffect->SetWelding(true);GetWorldTimerManager().SetTimer(DemoTimer,this,&AFoundrylineWeldingStation::ToggleDemo,2.2f,false);}
}
void AFoundrylineWeldingStation::ToggleDemo(){WeldingEffect->SetWelding(!WeldingEffect->IsWelding());GetWorldTimerManager().SetTimer(DemoTimer,this,&AFoundrylineWeldingStation::ToggleDemo,WeldingEffect->IsWelding()?2.2f:1.3f,false);}
