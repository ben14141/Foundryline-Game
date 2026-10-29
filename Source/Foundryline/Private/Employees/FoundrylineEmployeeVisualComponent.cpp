#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Employees/FoundrylineEmployeeAppearance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace { constexpr const TCHAR* Orange=TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_FoundryOrange"); constexpr const TCHAR* Dark=TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Graphite"); }
USceneComponent* UFoundrylineEmployeeVisualComponent::Joint(FName Name,USceneComponent* Parent,FVector Position){
 auto* C=NewObject<USceneComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),USceneComponent::StaticClass(),Name));C->SetupAttachment(Parent);C->SetRelativeLocation(Position);C->ComponentTags.Add(Name);C->RegisterComponent();GetOwner()->AddInstanceComponent(C);return C;
}
UStaticMeshComponent* UFoundrylineEmployeeVisualComponent::Part(FName Name,USceneComponent* Parent,FVector Centre,FVector Size,const TCHAR* Material){
 auto* C=NewObject<UStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),UStaticMeshComponent::StaticClass(),Name));C->SetupAttachment(Parent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(Centre);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));C->ComponentTags.Add(Name);C->RegisterComponent();GetOwner()->AddInstanceComponent(C);return C;
}
void UFoundrylineEmployeeVisualComponent::InitializeVisual(){
 if(bBuilt)return;bBuilt=true;PrimaryComponentTick.bCanEverTick=false;
 WalkingBody=NewObject<UCapsuleComponent>(GetOwner());WalkingBody->SetupAttachment(this);WalkingBody->SetRelativeLocation({0,0,90});WalkingBody->InitCapsuleSize(24,90);WalkingBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);WalkingBody->SetCollisionObjectType(ECC_WorldDynamic);WalkingBody->SetCollisionResponseToAllChannels(ECR_Ignore);WalkingBody->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);WalkingBody->SetCanEverAffectNavigation(false);WalkingBody->ComponentTags.Add(TEXT("EmployeeWalkingBody"));WalkingBody->RegisterComponent();GetOwner()->AddInstanceComponent(WalkingBody);
 if(GetOwner()->GetNetMode()==NM_DedicatedServer)return;
 if(!Appearance)Appearance=LoadObject<UFoundrylineEmployeeAppearance>(nullptr,TEXT("/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees"));
 if(Appearance&&Appearance->bHandymanPose&&InitializeHandyman())return;
 if(Appearance&&Appearance->Mesh){SkeletalBody=NewObject<USkeletalMeshComponent>(GetOwner());SkeletalBody->SetupAttachment(this);SkeletalBody->SetSkeletalMeshAsset(Appearance->Mesh);SkeletalBody->SetRelativeLocationAndRotation(Appearance->FootRootOffset,Appearance->FacingOffset);SkeletalBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);SkeletalBody->SetCanEverAffectNavigation(false);SkeletalBody->SetAnimationMode(EAnimationMode::AnimationSingleNode);SkeletalBody->RegisterComponent();GetOwner()->AddInstanceComponent(SkeletalBody);return;}
 Pelvis=Joint(TEXT("EmployeePelvis"),this,{0,0,92});Part(TEXT("EmployeeHips"),Pelvis,{0,0,4},{24,30,20},Dark);
 Part(TEXT("EmployeeTorso"),Pelvis,{0,0,32},{24,39,38},Orange);
 Part(TEXT("EmployeeReflectorFront"),Pelvis,{12.2,0,33},{.6,40,3},Dark);
 Part(TEXT("EmployeeReflectorBack"),Pelvis,{-12.2,0,33},{.6,40,3},Dark);
 auto* Head=Part(TEXT("EmployeeHead"),Pelvis,{0,0,69},{19,19,24},Orange);
 auto* Skin=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),this);Skin->SetVectorParameterValue(TEXT("Color"),FLinearColor(.48,.28,.16));Head->SetMaterial(0,Skin);
 Part(TEXT("EmployeeNeck"),Pelvis,{0,0,54},{10,10,8},Orange)->SetMaterial(0,Skin);
 Part(TEXT("EmployeeHardhat"),Pelvis,{0,0,85},{24,24,6},Orange);
 Part(TEXT("EmployeeHatBrim"),Pelvis,{3,0,81},{31,28,2},Orange);
 Part(TEXT("EmployeeFace"),Pelvis,{10,0,70},{2,12,4},Dark);
 for(int32 I=0;I<2;++I){const float S=I?1:-1;
  auto* T=Joint(I?TEXT("EmployeeRightThigh"):TEXT("EmployeeLeftThigh"),Pelvis,{0,S*10,0});Thighs.Add(T);Part(TEXT("EmployeeTrouserThigh"),T,{0,0,-21.5},{17,17,46},Dark);
  auto* K=Joint(I?TEXT("EmployeeRightKnee"):TEXT("EmployeeLeftKnee"),T,{0,0,-43});Shins.Add(K);Part(TEXT("EmployeeTrouserShin"),K,{0,0,-21.5},{15,15,45},Dark);
  auto* F=Joint(I?TEXT("EmployeeRightFoot"):TEXT("EmployeeLeftFoot"),K,{0,0,-43});Feet.Add(F);Part(TEXT("EmployeeBoot"),F,{5,0,-1.5},{29,15,11},Dark);
  auto* A=Joint(I?TEXT("EmployeeRightShoulder"):TEXT("EmployeeLeftShoulder"),Pelvis,{0,S*24,51});Shoulders.Add(A);Part(TEXT("EmployeeSleeve"),A,{0,0,-15.5},{13,13,34},Orange);
  auto* E=Joint(I?TEXT("EmployeeRightElbow"):TEXT("EmployeeLeftElbow"),A,{0,0,-31});Elbows.Add(E);Part(TEXT("EmployeeForearm"),E,{0,0,-14},{11,11,30},Orange);
  auto* H=Joint(I?TEXT("EmployeeRightHand"):TEXT("EmployeeLeftHand"),E,{0,0,-28});Hands.Add(H);Part(TEXT("EmployeeGlove"),H,{0,0,-5},{10,10,12},Dark);
 }
}
FVector UFoundrylineEmployeeVisualComponent::LegAngles(float Hip,float X,float Z){
 const double H=Hip-Z,D=FMath::Clamp(FMath::Sqrt(X*X+H*H),.1,85.99);const double A=FMath::Atan2(X,H),B=FMath::Acos(FMath::Clamp(D/86.,-1.,1.));const double Knee=-2*B;return {FMath::RadiansToDegrees(A+B),FMath::RadiansToDegrees(Knee),FMath::RadiansToDegrees(-A-B-Knee)};
}
void UFoundrylineEmployeeVisualComponent::Arm(int32 I,FVector Wrist){
 const FVector Shoulder(0,I?24:-24,HipHeight+51);const FVector D=Wrist-Shoulder;const double X=FMath::Sqrt(D.X*D.X+D.Y*D.Y),H=-D.Z,L=FMath::Clamp(FMath::Sqrt(X*X+H*H),3.01,58.99);const double A=FMath::Atan2(X,H),B=FMath::Acos(FMath::Clamp((31.*31+L*L-28.*28)/(62*L),-1.,1.));const double K=PI-FMath::Acos(FMath::Clamp((31.*31+28.*28-L*L)/(2*31.*28),-1.,1.));const float P=FMath::RadiansToDegrees(A-B),E=FMath::RadiansToDegrees(K);Shoulders[I]->SetRelativeRotation(FRotator(P,FMath::RadiansToDegrees(FMath::Atan2(D.Y,D.X)),0));Elbows[I]->SetRelativeRotation(FRotator(E,0,0));Hands[I]->SetRelativeRotation(FRotator(90-P-E,0,0));
}
void UFoundrylineEmployeeVisualComponent::ArmPalm(int32 I,FVector Palm){
 // Fingers extend along the shoulder's horizontal aim, not necessarily body +X.
 const FVector Direction=FVector(Palm.X,Palm.Y-(I?24:-24),0).GetSafeNormal();Arm(I,Palm-Direction*5);
}
void UFoundrylineEmployeeVisualComponent::Present(const FFoundrylineEmployeeRecord& R,float Dt){
 InitializeVisual();Identity=R.Key;ComponentTags.AddUnique(FName(*R.Key));ActivityName=StaticEnum<EFoundrylineEmployeeActivity>()->GetNameStringByValue(int64(R.Activity));bMoving=R.bMoving;bSeated=R.bSeated;WeldingGripError=-1;
 WalkingBody->SetCollisionEnabled(bSeated?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
 // Only real travelled distance advances feet. Time is used solely for an active work gesture.
 DisplayDistance=R.Distance;Phase=FMath::Fmod(DisplayDistance/110.f,1.f);WorkPhase=R.WorkPhase;
 if(HandymanBody){PresentHandyman(R,Dt);return;}
 if(SkeletalBody&&Appearance){UAnimSequence* Clip=Appearance->Idle;
  if(bSeated)Clip=Appearance->Seated;else if(bMoving)Clip=Appearance->Walk;else if(R.Activity==EFoundrylineEmployeeActivity::Welding)Clip=Appearance->Weld;else if(R.Activity==EFoundrylineEmployeeActivity::Handling)Clip=Appearance->Handling;else if(R.bWorking)Clip=Appearance->Operate;
  if(!Clip)Clip=Appearance->Idle;if(Clip){auto* Instance=SkeletalBody->GetSingleNodeInstance();if(Instance&&Instance->GetCurrentAsset()!=Clip)SkeletalBody->SetAnimation(Clip);SkeletalBody->SetPlayRate(0);const float T=bMoving?FMath::Fmod(DisplayDistance/FMath::Max(20.f,Appearance->WalkStrideCm),1.f)*Clip->GetPlayLength():FMath::Fmod(FMath::Max(0.f,WorkPhase),FMath::Max(.01f,Clip->GetPlayLength()));SkeletalBody->SetPosition(T,false);}return;
 }
 if(!Pelvis)return;
 HipHeight=bSeated?148:(bMoving?88:92);Pelvis->SetRelativeLocation({0,0,HipHeight});
 for(int32 I=0;I<2;++I){float X=0,Z=7;const float P=FMath::Fmod(Phase+I*.5f,1.f);if(bMoving){if(P<.5f)X=27.5f-110*P;else {const float T=(P-.5f)*2;X=FMath::Lerp(-27.5f,27.5f,T);Z+=8*FMath::Sin(PI*T);}}
  const FVector Ang=LegAngles(HipHeight,bSeated?50:X,bSeated?90:Z);Thighs[I]->SetRelativeRotation(FRotator(Ang.X,0,0));Shins[I]->SetRelativeRotation(FRotator(Ang.Y,0,0));Feet[I]->SetRelativeRotation(FRotator(Ang.Z,0,0));
  const float Side=I?1:-1;
  if(bSeated)Arm(I,{43,Side*18,174});else if(R.bWorking&&R.Activity==EFoundrylineEmployeeActivity::Welding)ArmPalm(I,I?FVector(42.55,0,125.26):FVector(49.1,-3,130.7));else if(R.bWorking){const float Tap=1.5f*FMath::Sin(WorkPhase*5+I*PI);Arm(I,{36+Tap,Side*15,113+Tap});}
  else {const float Swing=bMoving?25*FMath::Sin(2*PI*Phase+I*PI):0;Shoulders[I]->SetRelativeRotation(FRotator(-Swing,0,Side*3));Elbows[I]->SetRelativeRotation(FRotator(8,0,0));Hands[I]->SetRelativeRotation(FRotator::ZeroRotator);}
 }
 LastDistance=R.Distance;
}
void UFoundrylineEmployeeVisualComponent::PresentWeldingGrip(FVector WorldGrip,FVector WorldSupport){
 if(HandymanBody&&ActivityName==TEXT("Welding")&&!WorldGrip.ContainsNaN()&&!WorldSupport.ContainsNaN()){const auto T=GetComponentTransform();HandymanPalm(1,T.InverseTransformPosition(WorldGrip),true);HandymanPalm(0,T.InverseTransformPosition(WorldSupport),true);CommitHandymanPose();const int B=HandymanBones.FindRef(TEXT("hand_r"));WeldingGripError=FVector::Dist(HandymanBody->GetComponentTransform().TransformPosition(HandymanPose[B].TransformPosition(HandymanPalmOffsets[1])),WorldGrip);return;}
 if(!Pelvis||SkeletalBody||ActivityName!=TEXT("Welding")||WorldGrip.ContainsNaN()||WorldSupport.ContainsNaN())return;
 const auto T=GetComponentTransform();ArmPalm(1,T.InverseTransformPosition(WorldGrip));ArmPalm(0,T.InverseTransformPosition(WorldSupport));
 WeldingGripError=FVector::Dist(Hands[1]->GetComponentTransform().TransformPosition({0,0,-5}),WorldGrip);
}
FString UFoundrylineEmployeeVisualComponent::GetPresentationReport()const{
 auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("key"),Identity);J->SetStringField(TEXT("activity"),ActivityName);J->SetNumberField(TEXT("distanceCm"),DisplayDistance);J->SetNumberField(TEXT("gaitPhase"),Phase);J->SetBoolField(TEXT("moving"),bMoving);J->SetBoolField(TEXT("seated"),bSeated);J->SetNumberField(TEXT("footRootWorldZ"),GetComponentLocation().Z);J->SetNumberField(TEXT("heightCm"),180);
 J->SetBoolField(TEXT("walkingCollision"),WalkingBody&&WalkingBody->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);J->SetNumberField(TEXT("weldingGripErrorCm"),WeldingGripError);
 if(HandymanBody){J->SetBoolField(TEXT("skeletal"),true);if(SkeletalBody){const auto& Sample=SkeletalBody->GetComponentSpaceTransforms();for(FName N:{TEXT("pelvis"),TEXT("foot_l"),TEXT("hand_l")}){const int B=SkeletalBody->GetBoneIndex(N);if(Sample.IsValidIndex(B)){J->SetNumberField(N.ToString()+TEXT("SamplerZ"),Sample[B].GetLocation().Z);J->SetNumberField(N.ToString()+TEXT("SamplerX"),Sample[B].GetLocation().X);}}}J->SetStringField(TEXT("profile"),Appearance?Appearance->GetPathName():TEXT(""));TArray<TSharedPtr<FJsonValue>> Outfit;if(Appearance&&Appearance->Outfits.IsValidIndex(HandymanVariant))for(auto P:Appearance->Outfits[HandymanVariant].Parts)if(P&&(!bWeldingHoodBody||!P->GetName().StartsWith(TEXT("SK_Hat_"))))Outfit.Add(MakeShared<FJsonValueString>(P->GetPathName()));J->SetArrayField(TEXT("outfit"),Outfit);J->SetStringField(TEXT("mesh"),HandymanBody->GetSkinnedAsset()?HandymanBody->GetSkinnedAsset()->GetPathName():TEXT(""));J->SetNumberField(TEXT("variant"),HandymanVariant);J->SetStringField(TEXT("animation"),HandymanAnimation);if(HandymanBody->GetSkinnedAsset())J->SetNumberField(TEXT("heightCm"),HandymanBody->GetSkinnedAsset()->GetBounds().BoxExtent.Z*2);const auto World=HandymanBody->GetComponentTransform();const auto Relative=HandymanBody->GetRelativeTransform();for(int I=0;I<2;++I){const int F=HandymanBones.FindRef(I?TEXT("foot_r"):TEXT("foot_l")),K=HandymanBones.FindRef(I?TEXT("calf_r"):TEXT("calf_l")),S=HandymanBones.FindRef(I?TEXT("upperarm_r"):TEXT("upperarm_l")),H=HandymanBones.FindRef(I?TEXT("hand_r"):TEXT("hand_l"));if(!HandymanPose.IsValidIndex(H))continue;J->SetNumberField(FString::Printf(TEXT("foot%dWorldZ"),I),World.TransformPosition(HandymanPose[F].GetLocation()).Z-8.23821);J->SetNumberField(FString::Printf(TEXT("knee%dPitch"),I),(HandymanPose[K].GetRotation()*HandymanRest[K].GetRotation().Inverse()).Rotator().Pitch);J->SetNumberField(FString::Printf(TEXT("shoulder%dPitch"),I),(HandymanPose[S].GetRotation()*HandymanRest[S].GetRotation().Inverse()).Rotator().Pitch);const FVector Hand=Relative.TransformPosition(HandymanPose[H].GetLocation()),Palm=Relative.TransformPosition(HandymanPose[H].TransformPosition(HandymanPalmOffsets[I]));J->SetNumberField(FString::Printf(TEXT("hand%dLocalX"),I),Hand.X);J->SetNumberField(FString::Printf(TEXT("hand%dLocalZ"),I),Hand.Z);J->SetNumberField(FString::Printf(TEXT("palm%dLocalX"),I),Palm.X);J->SetNumberField(FString::Printf(TEXT("palm%dLocalZ"),I),Palm.Z);TArray<TSharedPtr<FJsonValue>> Point;const FVector P=World.TransformPosition(HandymanPose[H].TransformPosition(HandymanPalmOffsets[I]));Point.Add(MakeShared<FJsonValueNumber>(P.X));Point.Add(MakeShared<FJsonValueNumber>(P.Y));Point.Add(MakeShared<FJsonValueNumber>(P.Z));J->SetArrayField(FString::Printf(TEXT("palm%dWorld"),I),Point);}}
 else J->SetBoolField(TEXT("skeletal"),false);
 for(int32 I=0;I<Feet.Num();++I){J->SetNumberField(FString::Printf(TEXT("foot%dWorldZ"),I),Feet[I]->GetComponentLocation().Z-7);J->SetNumberField(FString::Printf(TEXT("knee%dPitch"),I),Shins[I]->GetRelativeRotation().Pitch);J->SetNumberField(FString::Printf(TEXT("shoulder%dPitch"),I),Shoulders[I]->GetRelativeRotation().Pitch);const FVector Hand=GetComponentTransform().InverseTransformPosition(Hands[I]->GetComponentLocation());J->SetNumberField(FString::Printf(TEXT("hand%dLocalX"),I),Hand.X);J->SetNumberField(FString::Printf(TEXT("hand%dLocalZ"),I),Hand.Z);}
 FString Out;const auto W=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,W);return Out;
}
