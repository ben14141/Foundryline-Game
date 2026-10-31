#include "UI/FoundrylinePlayerController.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/PlayerCameraManager.h"

namespace {
const FName FlowTag(TEXT("FoundrylinePlacementFlow"));
// Components belong to the existing local hologram actor. Its normal hide/rebuild
// and destruction lifecycle therefore owns these markers too, with no replicated actors.
USceneComponent* FindPart(AActor* Owner,FName Tag){TInlineComponentArray<USceneComponent*> Parts;Owner->GetComponents(Parts);for(auto* Part:Parts)if(IsValid(Part)&&Part->IsRegistered()&&Part->ComponentHasTag(Tag))return Part;return nullptr;}
UStaticMeshComponent* ArrowBar(AActor* Owner,FName Tag){
 if(auto* Existing=Cast<UStaticMeshComponent>(FindPart(Owner,Tag)))return Existing;
 auto* Part=NewObject<UStaticMeshComponent>(Owner);Part->SetupAttachment(Owner->GetRootComponent());Part->ComponentTags.Add(FlowTag);Part->ComponentTags.Add(Tag);
 Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);Part->SetCastShadow(false);
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));
 if(Base)Part->SetMaterial(0,UMaterialInstanceDynamic::Create(Base,Owner));
 Part->RegisterComponent();Owner->AddInstanceComponent(Part);return Part;
}
UTextRenderComponent* FlowLabel(AActor* Owner,FName Tag,bool Out){
 if(auto* Existing=Cast<UTextRenderComponent>(FindPart(Owner,Tag)))return Existing;
 auto* Label=NewObject<UTextRenderComponent>(Owner);Label->SetupAttachment(Owner->GetRootComponent());Label->ComponentTags.Add(FlowTag);Label->ComponentTags.Add(Tag);
 Label->SetText(FText::FromString(Out?TEXT("OUT"):TEXT("IN")));Label->SetHorizontalAlignment(EHTA_Center);Label->SetVerticalAlignment(EVRTA_TextCenter);Label->SetWorldSize(55);
 Label->SetTextMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineMaterials/DefaultTextMaterialTranslucent.DefaultTextMaterialTranslucent")));
 Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);Label->SetCanEverAffectNavigation(false);Label->SetCastShadow(false);Label->RegisterComponent();Owner->AddInstanceComponent(Label);return Label;
}
void HideFlow(AActor* Owner){if(!Owner)return;TInlineComponentArray<USceneComponent*> Parts;Owner->GetComponents(Parts);for(auto* Part:Parts)if(Part->ComponentHasTag(FlowTag))Part->SetVisibility(false);}
void DrawFlow(AActor* Owner,const FTransform& Plant,FVector At,FVector Direction,FVector Camera,bool Out,bool Valid){
 Direction.Z=0;Direction.Normalize();const FRotator Yaw=Direction.Rotation();At.Z=23.5;
 const FLinearColor Colour=!Valid?FLinearColor(2.4,.1,.04):Out?FLinearColor(.02,1.2,.15):FLinearColor(.025,.48,1);
 // Three shallow solid bars form a floor arrow; no debug drawing or line-thickness ambiguity.
 const FVector Positions[]={{-15,0,0},{30,-19,0},{30,19,0}};const FVector Sizes[]={{100,15,3},{55,15,3},{55,15,3}};const double Turns[]={0,45,-45};
 for(int I=0;I<3;++I){auto* Bar=ArrowBar(Owner,FName(*FString::Printf(TEXT("Flow%sBar%d"),Out?TEXT("Out"):TEXT("In"),I)));
  const FTransform Local(FRotator(0,Yaw.Yaw+Turns[I],0),At+Yaw.RotateVector(Positions[I]),Sizes[I]/100);
  Bar->SetWorldTransform(Local*Plant);Bar->SetVisibility(true);
  if(auto* Material=Cast<UMaterialInstanceDynamic>(Bar->GetMaterial(0))){Material->SetVectorParameterValue(TEXT("GlowColor"),Colour);Material->SetScalarParameterValue(TEXT("Strength"),2500);}
 }
 auto* Label=FlowLabel(Owner,FName(Out?TEXT("FlowOutLabel"):TEXT("FlowInLabel")),Out);
 const FVector WorldLabel=Plant.TransformPosition(At+FVector(0,0,85));Label->SetWorldLocationAndRotation(WorldLabel,(Camera-WorldLabel).Rotation());Label->SetWorldScale3D(FVector::OneVector);
 Label->SetTextRenderColor((!Valid?FLinearColor(1,.12,.04):Out?FLinearColor(.08,1,.25):FLinearColor(.08,.55,1)).ToFColor(false));Label->SetVisibility(true);
}
}
void AFoundrylinePlayerController::UpdateFactoryFlowPreview(bool ShowEquipment,bool ShowStation){
 if(!IsLocalController())return;
 HideFlow(FactoryEquipmentPreview);HideFlow(FactorySupplyPreview);
 // Replaces the old station text at the port centres with the same readable dock labels.
 for(UTextRenderComponent* Label:FactoryStationLabels)if(Label)Label->SetVisibility(false);
 auto* Plant=FactoryPlant();if(!Plant||!PlayerCameraManager)return;
 const auto Transform=Plant->GetActorTransform();const FVector Camera=PlayerCameraManager->GetCameraLocation();
 if(ShowEquipment&&FactoryEquipmentPreview&&FactoryEquipmentChoice>=4&&FactoryEquipmentChoice<=6){
  const int Kind=FactoryEquipmentChoice-3;const FRotator Turn(0,FactoryStationQuarter*90,0);
  const FVector In=Kind==1?FVector(-840,0,0):FVector(740,-250,0),Out(Kind==1?840:740,250,0);
  DrawFlow(FactoryEquipmentPreview,Transform,FactoryPreviewA+Turn.RotateVector(In),Turn.RotateVector(Kind==1?FVector::ForwardVector:-FVector::ForwardVector),Camera,false,bFactoryPresetValid);
  DrawFlow(FactoryEquipmentPreview,Transform,FactoryPreviewA+Turn.RotateVector(Out),Turn.RotateVector(FVector::ForwardVector),Camera,true,bFactoryPresetValid);
 }else if(ShowStation&&FactorySupplyPreview){
  using Layout=FFoundrylineWeldingStationLayout;FFoundrylineWeldingStationRecord Station;Station.Center=FactoryPreviewA;Station.QuarterTurns=FactoryStationQuarter;Station.bSwapSides=bFactoryStationSwap;
  for(bool Out:{false,true}){
   const bool Left=Layout::PortOnLeft(Station,Out);const FVector Outward=Layout::Rotation(Station).RotateVector(FVector(Left?-1:1,0,0));
   DrawFlow(FactorySupplyPreview,Transform,Layout::Dock(Station,Out)+Outward*65,Out?Outward:-Outward,Camera,Out,bFactoryPresetValid);
  }
 }
}
