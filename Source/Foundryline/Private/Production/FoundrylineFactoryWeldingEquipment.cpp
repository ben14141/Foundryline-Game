#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void AFoundrylineFactoryPlant::RebuildZoneWeldingEquipment(){
 if(!WeldingMachines)return;WeldingMachines->ClearInstances();
 for(auto C:WeldingGuns)C->DestroyComponent();WeldingGuns.Reset();for(auto C:IdleWeldingGuns)C->DestroyComponent();IdleWeldingGuns.Reset();for(auto C:IdleWeldingLeads)C->DestroyComponent();IdleWeldingLeads.Reset();for(auto C:WeldingLeads)C->DestroyComponent();WeldingLeads.Reset();
 auto* GunMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_MIGGun.SM_Welder_MIGGun"));auto* IdleLeadMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Leads.SM_Welder_Leads"));auto* CableMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Rubber.M_MH_Rubber"));
 auto Part=[&](UStaticMesh* Mesh,FVector Position,FRotator Rotation){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetRelativeLocationAndRotation(Position,Rotation);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);return C;};
 for(const auto& Z:Snapshot.Zones){
  const FRotator R=FFoundrylineWorkZoneLayout::Rotation(Z);const FVector Base=FFoundrylineWorkZoneLayout::WelderPosition(Z),Tip=FFoundrylineWorkZoneLayout::WeldPoint(Z);WeldingMachines->AddInstance(FTransform(R,Base));
  IdleWeldingGuns.Add(Part(GunMesh,Base+R.RotateVector(FFoundrylineWeldingStationLayout::HolsterCm()),R));IdleWeldingLeads.Add(Part(IdleLeadMesh,Base,R));
  const FRotator GunRotation=(R.Quaternion()*FRotator(-30,Z.SupplySide*90,0).Quaternion()).Rotator();auto* Gun=Part(GunMesh,FVector::ZeroVector,GunRotation);const FVector Socket=Gun->GetSocketTransform(TEXT("WeldTip"),RTS_Component).GetLocation();Gun->SetRelativeLocation(Tip-GunRotation.RotateVector(Socket));WeldingGuns.Add(Gun);
  auto* Effect=NewObject<UFoundrylineWeldingEffectComponent>(this);Effect->SetIsReplicated(false);Effect->SetLocalActivity(false);Effect->SetupAttachment(Gun,TEXT("WeldTip"));Effect->RegisterComponent();AddInstanceComponent(Effect);WeldEffects.Add(Effect);
  const FVector Work=R.UnrotateVector(FFoundrylineWorkZoneLayout::WorkPoint(Z)-Base);const FVector End=R.UnrotateVector(Gun->GetRelativeLocation()-Base);const FVector Points[]={{54,12,53},{90,-Z.SupplySide*70.,30},{100,Work.Y,4},{Work.X,Work.Y,4},{Work.X,Work.Y,90},End};
  for(int I=0;I<5;++I){auto* Lead=NewObject<USplineMeshComponent>(this);Lead->SetMobility(EComponentMobility::Movable);Lead->SetupAttachment(RootComponent);Lead->SetStaticMesh(CableMesh);Lead->SetForwardAxis(ESplineMeshAxis::Z,false);Lead->SetStartScale(FVector2D(.024f));Lead->SetEndScale(FVector2D(.024f));const FVector A=Base+R.RotateVector(Points[I]),B=Base+R.RotateVector(Points[I+1]),Tangent=R.RotateVector(Points[I+1]-Points[I]);Lead->SetStartAndEnd(A,Tangent,B,Tangent);Lead->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lead->SetCanEverAffectNavigation(false);if(Rubber)Lead->SetMaterial(0,Rubber);Lead->RegisterComponent();AddInstanceComponent(Lead);WeldingLeads.Add(Lead);}
 }
}
