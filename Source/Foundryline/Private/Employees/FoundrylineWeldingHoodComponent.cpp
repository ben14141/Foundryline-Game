#include "Employees/FoundrylineWeldingHoodComponent.h"
#include "Employees/FoundrylineEmployeeAppearance.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Employees/FoundrylineHandymanAssetTools.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

namespace {
 constexpr float OpenAngle=105.f;
 // Measured Handyman head rest transform: hood neutral root is ear hinge at native Z170cm.
 // Final asset metadata must agree with these constants before native acceptance.
 const FTransform HoodFromHead(FQuat(-.02512077,-.70663461,.02516714,-.70668457).GetNormalized(),FVector(14.55806393,-1.85629664,.00112296));
 const TCHAR* HoodFolder=TEXT("/Game/Foundryline/Employees/Equipment/WeldingHood/");
}
UFoundrylineWeldingHoodComponent::UFoundrylineWeldingHoodComponent(){PrimaryComponentTick.bCanEverTick=false;SetIsReplicatedByDefault(false);}
bool UFoundrylineWeldingHoodComponent::WantsHood(const FFoundrylineEmployeeRecord& R){return R.Role==EFoundrylineEmployeeRole::Welder&&R.Assignment>0&&!R.bSeated;}
bool UFoundrylineWeldingHoodComponent::WantsClosedVisor(const FFoundrylineEmployeeRecord& R){return WantsHood(R)&&R.bWorking&&R.Activity==EFoundrylineEmployeeActivity::Welding&&!R.bMoving;}
USkeletalMesh* UFoundrylineWeldingHoodComponent::NoHatOutfit(UFoundrylineEmployeeAppearance* P,int32 Variant){
 if(!P||!P->Outfits.IsValidIndex(Variant))return nullptr;
 // Shared across all people/companies using this profile. Only three current outfits exist.
 // Runtime meshes are NEVER saved as editor assets (they do not contain ImportedModel).
 if(P->RuntimeWeldingHoodVariants.Num()!=P->Outfits.Num())P->RuntimeWeldingHoodVariants.SetNum(P->Outfits.Num());
 if(P->RuntimeWeldingHoodVariants[Variant])return P->RuntimeWeldingHoodVariants[Variant].Get();
 TArray<USkeletalMesh*> Parts;bool RemovedCap=false;
 for(auto Part:P->Outfits[Variant].Parts)if(Part){if(Part->GetName().StartsWith(TEXT("SK_Hat_")))RemovedCap=true;else Parts.Add(Part.Get());}
 if(!RemovedCap||Parts.Num()<2)return nullptr;
 auto* M=UFoundrylineHandymanAssetTools::BuildRuntimeVariant(1000+Variant,Parts);if(!M)return nullptr;
 P->RuntimeWeldingHoodVariants[Variant]=M;return M;
}
void UFoundrylineWeldingHoodComponent::CreateMeshes(){
 if(bTriedAssets||!GetOwner()||GetOwner()->GetNetMode()==NM_DedicatedServer)return;bTriedAssets=true;
 auto Make=[&](const TCHAR* Name){auto* C=NewObject<UStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),UStaticMeshComponent::StaticClass(),Name));C->SetupAttachment(this);C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(FString(HoodFolder)+Name)));C->RegisterComponent();GetOwner()->AddInstanceComponent(C);return C;};
 Harness=Make(TEXT("SM_WeldingHood_Harness"));Visor=Make(TEXT("SM_WeldingHood_Visor"));SetVisibility(false,true);
}
bool UFoundrylineWeldingHoodComponent::HasModelAssets(){CreateMeshes();return Harness&&Visor&&Harness->GetStaticMesh()&&Visor->GetStaticMesh();}
void UFoundrylineWeldingHoodComponent::Present(UPoseableMeshComponent* Body,const FFoundrylineEmployeeRecord& R,bool bNoHatBody,float Dt){
 if(!GetOwner()||GetOwner()->GetNetMode()==NM_DedicatedServer)return;
 const bool Equip=Body&&Body->GetBoneIndex(TEXT("head"))!=INDEX_NONE&&bNoHatBody&&WantsHood(R)&&HasModelAssets();
 if(!Equip){if(bEquipped)SetVisibility(false,true);bEquipped=false;return;}
 if(!bEquipped){VisorAngleDegrees=OpenAngle;SetVisibility(true,true);}bEquipped=true;
 // Read the actual evaluated poseable head AFTER CommitHandymanPose, including employee yaw.
 SetWorldTransform(HoodFromHead*Body->GetSocketTransform(TEXT("head"),RTS_World));
 const float Target=WantsClosedVisor(R)?0.f:OpenAngle;
 VisorAngleDegrees=FMath::FInterpConstantTo(VisorAngleDegrees,Target,FMath::Clamp(Dt,0.f,.1f),720.f);
 // Both original meshes have their origins on the same ear hinge; visor native axis is +X.
 Visor->SetRelativeRotation(FQuat(FVector::ForwardVector,FMath::DegreesToRadians(VisorAngleDegrees)));
}
