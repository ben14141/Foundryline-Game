#include "Logistics/FoundrylineFreightRig.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AFoundrylineFreightRig::AFoundrylineFreightRig() {
 PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetNetUpdateFrequency(15);SetNetCullDistanceSquared(FMath::Square(40000.f));
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Rig")));
 TractorRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Tractor"));TractorRoot->SetupAttachment(RootComponent);
 TrailerRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Tanker"));TrailerRoot->SetupAttachment(RootComponent);
 TractorCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("TractorCollision"));TractorCollision->SetupAttachment(TractorRoot);TractorCollision->SetRelativeLocation({210,0,210});TractorCollision->SetBoxExtent({300,130,185});TractorCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 TrailerCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("TrailerCollision"));TrailerCollision->SetupAttachment(TrailerRoot);TrailerCollision->SetRelativeLocation({-410,0,235});TrailerCollision->SetBoxExtent({665,130,205});TrailerCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}
void AFoundrylineFreightRig::BeginPlay() {
 Super::BeginPlay();
 auto Part=[&](const TCHAR* Name,bool Trailer,FVector Location,int Kind,float Radius,bool Steers){
  auto* M=NewObject<UStaticMeshComponent>(this,FName(Name));M->SetupAttachment(Trailer?TrailerRoot:TractorRoot);M->SetRelativeLocation(Location);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  const FString Path=FString::Printf(TEXT("/Game/Foundryline/%s/SM_%s.SM_%s"),Trailer?TEXT("Products/Tankers/StraightBarrel"):TEXT("Vehicles/Semi/Megapoly"),Name,Name);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));M->RegisterComponent();AddInstanceComponent(M);
  if(Kind==1) {Wheels.Add(M);TrailerWheel.Add(Trailer);SteerWheel.Add(Steers);WheelRadius.Add(Radius);}
  if(Kind==2) {Legs.Add(M);LegRest.Add(Location);}
 };
 Part(TEXT("Tanker_Body"),true,{0.0f,0.0f,0.0f},0,1.0f,false);
 Part(TEXT("Tanker_Wheel_0"),true,{-714.75911f,95.22266f,45.53077f},1,47.35245f,false);
 Part(TEXT("Tanker_Wheel_1"),true,{-928.79219f,-95.59224f,45.53091f},1,47.35245f,false);
 Part(TEXT("Tanker_Wheel_2"),true,{-821.77563f,-95.59224f,45.53091f},1,47.35245f,false);
 Part(TEXT("Tanker_Wheel_3"),true,{-928.79229f,95.22266f,45.53077f},1,47.35245f,false);
 Part(TEXT("Tanker_Wheel_4"),true,{-821.77582f,95.22266f,45.53077f},1,47.35245f,false);
 Part(TEXT("Tanker_Wheel_5"),true,{-714.75921f,-95.59224f,45.53091f},1,47.35245f,false);
 Part(TEXT("Tanker_Leg_0"),true,{-263.90362f,85.81524f,118.34186f},2,1.0f,false);
 Part(TEXT("Tanker_Leg_1"),true,{-263.90362f,-85.81499f,118.34214f},2,1.0f,false);
 Part(TEXT("Tractor_Body"),false,{0.0f,0.0f,0.0f},0,1.0f,false);
 Part(TEXT("Tractor_Wheel_0"),false,{0.00926f,95.22259f,45.53076f},1,47.35245f,false);
 Part(TEXT("Tractor_Wheel_1"),false,{0.00921f,-95.59264f,45.53089f},1,47.35245f,false);
 Part(TEXT("Tractor_Wheel_2"),false,{367.83292f,114.05562f,45.51796f},1,47.35242f,true);
 Part(TEXT("Tractor_Wheel_3"),false,{367.83297f,-114.08374f,45.5181f},1,47.35242f,true);
 UpdateVisuals(0);
 if(HasAuthority()) Feedback=bTractorOnly?TEXT("Pickup tractor / waiting for a finished trailer."):TEXT("Bring the semi close to the trailer front to auto-connect.");
}
void AFoundrylineFreightRig::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineFreightRig,State);DOREPLIFETIME(AFoundrylineFreightRig,bTractorOnly);DOREPLIFETIME(AFoundrylineFreightRig,BusinessCompanyId);DOREPLIFETIME(AFoundrylineFreightRig,BusinessPlantId);DOREPLIFETIME(AFoundrylineFreightRig,Driver);DOREPLIFETIME(AFoundrylineFreightRig,Feedback);
}
FVector AFoundrylineFreightRig::Hitch(bool Tractor) const {return GetActorTransform().TransformPosition((Tractor?State.Tractor:State.Trailer)+FRotator(0,Tractor?State.TractorYaw:State.TrailerYaw,0).RotateVector(Tractor?FVector(42,0,128):FVector(0,0,128)));}
FVector AFoundrylineFreightRig::TractorWorldPosition() const {return TractorRoot->GetComponentLocation();}
bool AFoundrylineFreightRig::Claim(APlayerController* Player,bool Take) {
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&Take&&!R->CanOperate(Player,BusinessCompanyId,BusinessPlantId)){return false;}

 if(!HasAuthority() || !IsValid(Player) || Player->GetWorld()!=GetWorld() || !Player->PlayerState) return false;
 if(Take) {if(Driver && Driver!=Player->PlayerState) return false;Driver=Player->PlayerState;}
 else {if(Driver!=Player->PlayerState) return false;Driver=nullptr;ThrottleInput=SteerInput=0;}
 ForceNetUpdate();return true;
}
void AFoundrylineFreightRig::Drive(APlayerController* Player,float Throttle,float Steering) {
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&!R->CanOperate(Player,BusinessCompanyId,BusinessPlantId)){return;}

 if(!HasAuthority() || !Player || !Driver || Driver!=Player->PlayerState || !FMath::IsFinite(Throttle) || !FMath::IsFinite(Steering)) return;
 ThrottleInput=FMath::Clamp(Throttle,-1.f,1.f);SteerInput=FMath::Clamp(Steering,-1.f,1.f);InputAge=0;
}
bool AFoundrylineFreightRig::Command(APlayerController* Player,uint8 Action) {
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&!R->CanOperate(Player,BusinessCompanyId,BusinessPlantId)){return false;}

 if(!HasAuthority() || !Player || !Driver || Driver!=Player->PlayerState) return false;
 const bool OK=Action==1?TryCouple():Action==2?SetGear(GearTarget<.5f):Action==3?Uncouple():false;ForceNetUpdate();return OK;
}
bool AFoundrylineFreightRig::TryCouple() {
 if(!HasAuthority() || State.Coupled || bTractorOnly) return false;
 if(FMath::Abs(State.Speed)>150) {Feedback=TEXT("Slow down to connect.");return false;}
 const FVector Delta=Hitch(true)-Hitch(false);
 if(Delta.Size2D()>220 || FMath::Abs(Delta.Z)>50 || FMath::Abs(FMath::FindDeltaAngleDegrees(State.TractorYaw,State.TrailerYaw))>60) {Feedback=TEXT("Bring the back of the semi close to the trailer front.");return false;}
 auto Snapped=State;Snapped.TrailerYaw=State.TractorYaw;
 Snapped.Trailer=State.Tractor+FRotator(0,State.TractorYaw,0).RotateVector({42,0,0});
 if(!ClearPose(Snapped)) {Feedback=TEXT("Make room around the trailer before connecting.");return false;}
 State=Snapped;State.Speed=0;State.Coupled=true;State.Detaching=false;GearTarget=0;
 Feedback=TEXT("Connected. Landing gear raises automatically.");ForceNetUpdate();return true;
}

bool AFoundrylineFreightRig::SetGear(bool Down) {
 if(!HasAuthority() || FMath::Abs(State.Speed)>1 || (!State.Coupled && !Down)) {Feedback=TEXT("Stop first. A detached tanker must remain supported.");return false;}
 GearTarget=Down?1:0;Feedback=Down?TEXT("Lowering landing gear."):TEXT("Retracting landing gear.");return true;
}
bool AFoundrylineFreightRig::Uncouple() {
 if(!HasAuthority() || !State.Coupled) return false;
 State.Detaching=true;ThrottleInput=0;
 if(FMath::Abs(State.Speed)<1) GearTarget=1;
 Feedback=TEXT("Detaching: stopping and lowering the landing gear.");ForceNetUpdate();return true;
}

bool AFoundrylineFreightRig::ClearPose(const FFoundrylineFreightPose& P) const {
 FCollisionQueryParams Q(SCENE_QUERY_STAT(FreightRigClearance),false,this);
 auto Clear=[&](FVector Origin,float Yaw,FVector Center,FVector Half){
  const FRotator R(0,Yaw,0);const FVector C=GetActorTransform().TransformPosition(Origin+R.RotateVector(Center));
  const FQuat Rot=GetActorQuat()*R.Quaternion();return !GetWorld()->OverlapBlockingTestByChannel(C,Rot,ECC_Pawn,FCollisionShape::MakeBox(Half),Q);
 };
 return Clear(P.Tractor,P.TractorYaw,{210,0,210},{300,130,185}) && (bTractorOnly||Clear(P.Trailer,P.TrailerYaw,{-410,0,235},{665,130,205}));
}
void AFoundrylineFreightRig::Step(float Dt) {
 if(!HasAuthority() || Dt<=0 || Dt>.1f) return;
 State.SampleTime=GetWorld()->GetTimeSeconds();
 InputAge+=Dt;if(InputAge>.35f || !IsValid(Driver)) ThrottleInput=SteerInput=0;
 if(State.Detaching && FMath::Abs(State.Speed)<1) GearTarget=1;
 State.Gear=FMath::FInterpConstantTo(State.Gear,GearTarget,Dt,.5f);
 State.Steering=FMath::FInterpConstantTo(State.Steering,SteerInput*28,Dt,65);
 if(State.Detaching && State.Gear>=.99f && FMath::Abs(State.Speed)<1) {
  State.Gear=1;State.Coupled=false;State.Detaching=false;bAutoCoupleReady=false;
  Feedback=TEXT("Detached. Legs lowered automatically; drive away.");ForceNetUpdate();
 }
 const float HitchDistance=(Hitch(true)-Hitch(false)).Size2D();
 if(!State.Coupled && HitchDistance>350) bAutoCoupleReady=true;
 if(!State.Coupled && Driver && bAutoCoupleReady && HitchDistance<=220 && FMath::Abs(State.Speed)<=150) TryCouple();
 float Target=ThrottleInput*(ThrottleInput<0?120:350);
 if(!bTractorOnly && !State.Coupled && Target<0 && HitchDistance<350) Target=-65;
 if(State.Detaching || (State.Coupled && (State.Gear>.01f || GearTarget>.01f))) Target=0;
 State.Speed=FMath::FInterpConstantTo(State.Speed,Target,Dt,Target==0?350:140);
 if(FMath::Abs(State.Speed)<.01f) return;
 auto Next=State;const float Distance=State.Speed*Dt;
 Next.TractorYaw+=FMath::RadiansToDegrees(Distance/368.f*FMath::Tan(FMath::DegreesToRadians(State.Steering)));
 Next.Tractor+=FRotator(0,(State.TractorYaw+Next.TractorYaw)*.5f,0).Vector()*Distance;
 if(State.Coupled) {
  const FVector OldH=State.Tractor+FRotator(0,State.TractorYaw,0).RotateVector({42,0,0});
  const FVector NewH=Next.Tractor+FRotator(0,Next.TractorYaw,0).RotateVector({42,0,0});
  const FVector Right=FRotationMatrix(FRotator(0,State.TrailerYaw,0)).GetUnitAxis(EAxis::Y);
  Next.TrailerYaw+=FMath::RadiansToDegrees(FVector::DotProduct(NewH-OldH,Right)/821.8f);Next.Trailer=NewH;
  if(FMath::Abs(FMath::FindDeltaAngleDegrees(Next.TractorYaw,Next.TrailerYaw))>65) {State.Speed=0;Feedback=TEXT("Articulation limit. Pull forward to straighten.");return;}
  Next.TrailerTravel+=FVector::DotProduct(Next.Trailer-State.Trailer,FRotator(0,State.TrailerYaw,0).Vector());
 }
 if(!ClearPose(Next)) {State.Speed=0;Feedback=TEXT("Blocked by an object. Stop and reposition.");return;}
 Next.TractorTravel+=Distance;State=Next;
 // Re-arm only after leaving the snap radius, so detaching does not relatch.
 if(!State.Coupled && bAutoCoupleReady && Driver && (Hitch(true)-Hitch(false)).Size2D()<220) TryCouple();
}
namespace {
 FFoundrylineFreightPose BlendPose(const FFoundrylineFreightPose& A,const FFoundrylineFreightPose& B,float T) {
  auto P=B;P.Tractor=FMath::Lerp(A.Tractor,B.Tractor,T);P.Trailer=FMath::Lerp(A.Trailer,B.Trailer,T);
  P.TractorYaw=A.TractorYaw+FMath::FindDeltaAngleDegrees(A.TractorYaw,B.TractorYaw)*T;
  P.TrailerYaw=A.TrailerYaw+FMath::FindDeltaAngleDegrees(A.TrailerYaw,B.TrailerYaw)*T;
  P.TractorTravel=FMath::Lerp(A.TractorTravel,B.TractorTravel,T);P.TrailerTravel=FMath::Lerp(A.TrailerTravel,B.TrailerTravel,T);
  P.Steering=FMath::Lerp(A.Steering,B.Steering,T);P.Gear=FMath::Lerp(A.Gear,B.Gear,T);return P;
 }
}
void AFoundrylineFreightRig::OnRep_State() {
 SnapshotBuffer.Add(State);if(SnapshotBuffer.Num()>12) SnapshotBuffer.RemoveAt(0);
}
void AFoundrylineFreightRig::UpdateVisuals(float Dt) {
 TrailerRoot->SetVisibility(!bTractorOnly,true);TrailerCollision->SetCollisionEnabled(bTractorOnly?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
 if(!bDisplayReady) {PreviousState=DisplayState=State;bDisplayReady=true;}
 if(HasAuthority()) DisplayState=BlendPose(PreviousState,State,FMath::Clamp(Accumulator*30,0.f,1.f));
 else if(SnapshotBuffer.Num()>1) {
  const auto* GS=GetWorld()->GetGameState();const float Now=(GS?GS->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds())-.1f;
  while(SnapshotBuffer.Num()>2 && SnapshotBuffer[1].SampleTime<Now) SnapshotBuffer.RemoveAt(0);
  const auto& A=SnapshotBuffer[0];const auto& B=SnapshotBuffer[1];
  const float T=FMath::Clamp((Now-A.SampleTime)/FMath::Max(.001f,B.SampleTime-A.SampleTime),0.f,1.f);DisplayState=BlendPose(A,B,T);
 } else DisplayState=State;
 const auto& P=DisplayState;
 TractorRoot->SetRelativeLocationAndRotation(P.Tractor,FRotator(0,P.TractorYaw,0));TrailerRoot->SetRelativeLocationAndRotation(P.Trailer,FRotator(0,P.TrailerYaw,0));
 for(int32 I=0;I<Wheels.Num();++I) {const float Travel=TrailerWheel[I]?P.TrailerTravel:P.TractorTravel;Wheels[I]->SetRelativeRotation(FRotator(FMath::Fmod(-FMath::RadiansToDegrees(Travel/WheelRadius[I]),360.f),SteerWheel[I]?P.Steering:0,0));}
 for(int32 I=0;I<Legs.Num();++I) Legs[I]->SetRelativeLocation(LegRest[I]-FVector(0,0,73.5f*P.Gear));
}
void AFoundrylineFreightRig::Tick(float Dt) {
 Super::Tick(Dt);
 if(HasAuthority()) {Accumulator+=FMath::Min(Dt,.1f);for(int32 I=0;I<4 && Accumulator>=1.f/30;++I) {PreviousState=State;Step(1.f/30);Accumulator-=1.f/30;}}
 UpdateVisuals(Dt);
}
