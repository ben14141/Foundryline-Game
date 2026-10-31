#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Components/PoseableMeshComponent.h"

void UFoundrylineEmployeeVisualComponent::PresentOfficeComputer(float Dt,int32 Seed){
 if(!FMath::IsFinite(Dt)||Dt<0)return;
 FFoundrylineEmployeeRecord R;R.Id=FMath::Max(1,Seed);R.Key=FString::Printf(TEXT("office-animation/%d"),R.Id);R.bSeated=true;R.bWorking=true;R.Activity=EFoundrylineEmployeeActivity::Operating;
 Present(R,Dt);ActivityName=TEXT("Office computer");HandymanAnimation=TEXT("Office / seated typing and mouse");
 const float Time=HandymanClock,Loop=FMath::Fmod(Time,12.f);
 const float Mouse=FMath::SmoothStep(8.f,8.7f,Loop)*(1-FMath::SmoothStep(11.3f,12.f,Loop));
 const float Tap=.28f*FMath::Sin(Time*30.f),Breath=.22f*FMath::Sin(Time*1.8f);
 const FVector Left(68,-10,82+Tap),Right=FMath::Lerp(FVector(68,10,82-Tap),FVector(68+FMath::Sin(Time*2.f)*1.3,39+FMath::Sin(Time*1.3f)*1.1,82),Mouse);
 if(HandymanBody&&!HandymanPose.IsEmpty()){
  const auto T=HandymanBody->GetRelativeTransform();const int Pel=HandymanBones.FindRef(TEXT("pelvis"));
  const FVector Shift=T.InverseTransformPosition(FVector(12,0,53+Breath))-HandymanPose[Pel].GetLocation();
  for(int I=Pel;I<HandymanPose.Num();++I){int P=I;while(P>Pel)P=HandymanParents[P];if(P==Pel)HandymanPose[I].AddToTranslation(Shift);}
  HandymanRotate(HandymanBones.FindRef(TEXT("spine_02")),FQuat(T.InverseTransformVectorNoScale(FVector::RightVector),FMath::DegreesToRadians(4.f)));
  for(int I=0;I<2;++I){const FName F=I?TEXT("foot_r"):TEXT("foot_l");const FQuat Rotation=HandymanRest[HandymanBones.FindRef(F)].GetRotation();HandymanIK(I?TEXT("thigh_r"):TEXT("thigh_l"),I?TEXT("calf_r"):TEXT("calf_l"),F,T.InverseTransformPosition({50,I?14.:-14.,8.23821}),T.InverseTransformPosition({80,I?16.:-16.,38}),&Rotation);}
  auto Palm=[&](int I,FVector At){const FName Hand=I?TEXT("hand_r"):TEXT("hand_l");const int H=HandymanBones.FindRef(Hand);const FVector Dir=T.InverseTransformVectorNoScale(FVector(1,0,-.18)).GetSafeNormal();const FVector RestDir=HandymanRest[H].GetRotation().RotateVector(HandymanPalmOffsets[I]).GetSafeNormal();const FQuat Rot=FQuat::FindBetweenVectors(RestDir,Dir)*HandymanRest[H].GetRotation();HandymanIK(I?TEXT("upperarm_r"):TEXT("upperarm_l"),I?TEXT("lowerarm_r"):TEXT("lowerarm_l"),Hand,T.InverseTransformPosition(At)-Rot.RotateVector(HandymanPalmOffsets[I]),T.InverseTransformPosition({22,I?48.:-48.,77}),&Rot);};
  Palm(0,Left);Palm(1,Right);
  for(int Side=0;Side<2;++Side)for(int Finger=0;Finger<3;++Finger){const TCHAR* Names[]={TEXT("index"),TEXT("middle"),TEXT("ring")};const FName Bone(*FString::Printf(TEXT("%s_01_%s"),Names[Finger],Side?TEXT("r"):TEXT("l")));if(const auto* B=HandymanBones.Find(Bone)){const float Amount=6+5*FMath::Sin(Time*(24+Finger*3)+Side+Finger);HandymanRotate(*B,FQuat(HandymanPose[*B].GetRotation().RotateVector(FVector::RightVector),FMath::DegreesToRadians(Amount*(Side&&Mouse>.5f?.3f:1.f))));}}
  CommitHandymanPose();return;
 }
 if(Pelvis){HipHeight=53;Pelvis->SetRelativeLocation({12,0,53+Breath});for(int I=0;I<2;++I){const auto A=LegAngles(53,38,7);Thighs[I]->SetRelativeRotation(FRotator(A.X,0,0));Shins[I]->SetRelativeRotation(FRotator(A.Y,0,0));Feet[I]->SetRelativeRotation(FRotator(A.Z,0,0));ArmPalm(I,I?Right:Left);}}
}
