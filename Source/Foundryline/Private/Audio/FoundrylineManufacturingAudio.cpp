#include "Audio/FoundrylineManufacturingAudio.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineMachineMotion.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Production/FoundrylineWeldingStation.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundAttenuation.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

bool UFoundrylineManufacturingAudio::DoesSupportWorldType(EWorldType::Type T)const{return T==EWorldType::Game||T==EWorldType::PIE;}
bool UFoundrylineManufacturingAudio::IsTickable()const{return Super::IsTickable()&&GetWorld()&&GetWorld()->GetNetMode()!=NM_DedicatedServer;}
TStatId UFoundrylineManufacturingAudio::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(FoundrylineManufacturingAudio,STATGROUP_Tickables);}
void UFoundrylineManufacturingAudio::Initialize(FSubsystemCollectionBase& C){
 Super::Initialize(C);if(GetWorld()->GetNetMode()==NM_DedicatedServer)return;
 for(const TCHAR* Name:{TEXT("WeldingArc"),TEXT("PlasmaCut"),TEXT("PlasmaGantry"),TEXT("BrakeHydraulics"),TEXT("SheetRoller"),TEXT("ForkliftDrive"),TEXT("ForkliftHydraulics"),TEXT("CollectorCart"),TEXT("BrakeContact"),TEXT("RoomAmbience")}){
  const FString Path=FString(TEXT("/Game/Foundryline/Audio/Manufacturing/Loops/SW_"))+Name;
  auto* Sound=LoadObject<USoundBase>(nullptr,*Path);if(Sound)Sounds.Add(FName(Name),Sound);else ++MissingAssets;
 }
 MachineryClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/Foundryline/Audio/Manufacturing/SC_Manufacturing"));
 Concurrency=NewObject<USoundConcurrency>(this);Concurrency->Concurrency.MaxCount=MaxVoices;Concurrency->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopQuietest;Concurrency->Concurrency.VoiceStealReleaseTime=.15f;
}
void UFoundrylineManufacturingAudio::Deinitialize(){for(auto& Pair:Voices)if(auto* A=Pair.Value.Component.Get()){A->Stop();A->DestroyComponent();}Voices.Reset();Sounds.Reset();Super::Deinitialize();}
void UFoundrylineManufacturingAudio::Tick(float Dt){
 Accumulator+=Dt;if(Accumulator<.1f)return;Dt=FMath::Min(Accumulator,.25f);Accumulator=0;
 auto* Camera=UGameplayStatics::GetPlayerCameraManager(GetWorld(),0);if(!Camera)return;const FVector Listener=Camera->GetCameraLocation();const float Now=GetWorld()->GetTimeSeconds();
 struct FCandidate {AActor* Owner;FString Key;FName Role;FVector At;float Gain,Pitch,Distance;};TArray<FCandidate> Wanted;TSet<FString> LiveDistanceKeys;
 auto Add=[&](AActor* Owner,const FString& Key,FName Role,FVector At,float Gain,float Pitch=1.f){const float Distance=FVector::Distance(At,Listener);if(Distance<2000&&Sounds.Contains(Role))Wanted.Add({Owner,Key,Role,At,Gain,Pitch,Distance});};
 auto Effect=[&](AActor* Actor){TArray<UFoundrylineWeldingEffectComponent*> Components;Actor->GetComponents(Components);for(auto* E:Components)if(E->IsWelding())Add(Actor,E->GetPathName(),E->bPlasmaCutting?TEXT("PlasmaCut"):TEXT("WeldingArc"),E->GetComponentLocation(),E->bPlasmaCutting?.45f:.55f);};
 for(TActorIterator<AFoundrylineFactoryPlant> It(GetWorld());It;++It){auto* P=*It;const auto& S=P->Snapshot;Effect(P);const FString Prefix=P->GetPathName();int32 ActiveMachines=0;
  for(const auto& M:S.Machines)if(FFoundrylineMachineMotion::Active(S,M)){
   ++ActiveMachines;const float Phase=FFoundrylineMachineMotion::Phase(M.Remaining,M.Duration,true,S.Speed,0);
   if(M.Id==1)Add(P,Prefix+TEXT(":gantry"),TEXT("PlasmaGantry"),P->MachineWorldPosition(1)+FVector(0,0,90),.18f);
   if(M.Id==2){const FString Key=Prefix+TEXT(":brake");const float* Before=PreviousPhase.Find(Key);
    if(Before&&*Before<.45f&&Phase>=.45f&&Phase-*Before<.3f)ContactUntil.Add(Key,Now+.65f);PreviousPhase.Add(Key,Phase);
    if(Phase>.08f&&Phase<.87f)Add(P,Key,TEXT("BrakeHydraulics"),P->MachineWorldPosition(2)+FVector(0,0,110),.45f,Phase<.58f?.94f:1.03f);
    if(ContactUntil.FindRef(Key)>Now)Add(P,Key+TEXT(":contact"),TEXT("BrakeContact"),P->MachineWorldPosition(2)+FVector(0,0,100),.24f);
   }
   if(M.Id==3)Add(P,Prefix+TEXT(":roller"),TEXT("SheetRoller"),P->MachineWorldPosition(3)+FVector(0,0,110),.40f);
  }
  if(ActiveMachines>1){const FVector Center=P->GetActorTransform().TransformPosition(FVector((S.FloorMin+S.FloorMax)*.5,100));Add(P,Prefix+TEXT(":room"),TEXT("RoomAmbience"),Center,.035f);}
  for(const auto& D:S.Drivers){const FString Key=Prefix+FString::Printf(TEXT(":fork%d"),D.Id);LiveDistanceKeys.Add(Key);const float* Before=PreviousDistance.Find(Key);if(Before&&D.Distance>*Before+.1f)MovementUntil.Add(Key,Now+.4f);const bool Moving=MovementUntil.FindRef(Key)>Now;PreviousDistance.Add(Key,D.Distance);
   if(S.Running&&D.EmployeeId&&Moving)Add(P,Key,TEXT("ForkliftDrive"),P->GetActorTransform().TransformPosition(D.Position+FVector(0,0,60)),.28f);
   else if(S.Running&&D.EmployeeId&&D.Job){const auto* Job=S.Jobs.FindByPredicate([&](const auto& J){return J.Id==D.Job;});if(Job&&(Job->Phase==2||Job->Phase==4))Add(P,Key,TEXT("ForkliftHydraulics"),P->GetActorTransform().TransformPosition(D.Position+FVector(0,0,90)),.24f);}
  }
  for(const auto& C:S.Collectors)if(S.Running&&C.Speed>15)Add(P,Prefix+FString::Printf(TEXT(":cart%d"),C.Id),TEXT("CollectorCart"),P->GetActorTransform().TransformPosition(C.Position+FVector(0,0,15)),.09f);
 }
 for(TActorIterator<AFoundrylineWeldingStation> It(GetWorld());It;++It)Effect(*It);
 for(TActorIterator<AFoundrylinePlasmaTable> It(GetWorld());It;++It)Effect(*It);
 for(auto It=PreviousDistance.CreateIterator();It;++It)if(!LiveDistanceKeys.Contains(It.Key())){MovementUntil.Remove(It.Key());It.RemoveCurrent();}
 Wanted.Sort([](const auto& A,const auto& B){return A.Distance<B.Distance;});CandidateCount=Wanted.Num();if(Wanted.Num()>MaxVoices)Wanted.SetNum(MaxVoices);TSet<FString> Used;
 const float Mix=CrowdGain(Wanted.Num());
 for(const auto& C:Wanted){if(!Voices.Contains(C.Key)&&Voices.Num()>=MaxVoices)continue;Used.Add(C.Key);auto& V=Voices.FindOrAdd(C.Key);if(!V.Component){auto* A=NewObject<UAudioComponent>(C.Owner);A->bAutoActivate=false;A->bAutoDestroy=false;A->bAllowSpatialization=true;A->SetSound(Sounds.FindChecked(C.Role));A->SoundClassOverride=MachineryClass;A->ConcurrencySet.Add(Concurrency);A->SetMobility(EComponentMobility::Movable);
   FSoundAttenuationSettings Settings;Settings.bAttenuate=true;Settings.bSpatialize=true;Settings.AttenuationShape=EAttenuationShape::Sphere;Settings.AttenuationShapeExtents=FVector(150);Settings.FalloffDistance=1650;Settings.DistanceAlgorithm=EAttenuationDistanceModel::NaturalSound;Settings.dBAttenuationAtMax=-42;
   Settings.bAttenuateWithLPF=true;Settings.LPFRadiusMin=300;Settings.LPFRadiusMax=1500;Settings.LPFFrequencyAtMin=16000;Settings.LPFFrequencyAtMax=1800;
   Settings.bEnableOcclusion=true;Settings.OcclusionTraceChannel=ECC_Visibility;Settings.OcclusionLowPassFilterFrequency=1200;Settings.OcclusionVolumeAttenuation=.4f;Settings.OcclusionInterpolationTime=.35f;
   A->AdjustAttenuation(Settings);A->RegisterComponentWithWorld(GetWorld());C.Owner->AddInstanceComponent(A);V.Component=A;V.Role=C.Role;A->SetWorldLocation(C.At);A->FadeIn(.08f,1.f,C.Role==TEXT("BrakeContact")?0.f:FMath::FRandRange(0.f,FMath::Max(.1f,A->Sound->GetDuration()-.2f)));
  }
  if(V.Role!=C.Role){V.Component->Stop();V.Component->SetSound(Sounds.FindChecked(C.Role));V.Role=C.Role;V.Component->FadeIn(.15f,1.f);}
  V.Component->SetWorldLocation(FMath::VInterpTo(V.Component->GetComponentLocation(),C.At,Dt,12.f));V.Component->SetPitchMultiplier(C.Pitch);V.StopAt=0;V.Volume=C.Gain*Mix;V.Component->AdjustVolume(.12f,V.Volume);
 }
 for(auto It=Voices.CreateIterator();It;++It)if(!Used.Contains(It.Key())){auto& V=It.Value();if(V.StopAt<=0){V.StopAt=Now+.24f;if(V.Component)V.Component->FadeOut(.18f,0.f);}if(Now>=V.StopAt){if(V.Component){V.Component->Stop();V.Component->DestroyComponent();}It.RemoveCurrent();}}
}
FString UFoundrylineManufacturingAudio::GetAudioReport()const{
 auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("voices"),Voices.Num());J->SetNumberField(TEXT("candidates"),CandidateCount);J->SetNumberField(TEXT("missing_assets"),MissingAssets);J->SetNumberField(TEXT("budget"),MaxVoices);J->SetBoolField(TEXT("dedicated_server"),GetWorld()->GetNetMode()==NM_DedicatedServer);TArray<TSharedPtr<FJsonValue>> Items;
 for(const auto& Pair:Voices){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("role"),Pair.Value.Role.ToString());R->SetNumberField(TEXT("gain"),Pair.Value.Volume);R->SetBoolField(TEXT("playing"),Pair.Value.Component&&Pair.Value.Component->IsPlaying());Items.Add(MakeShared<FJsonValueObject>(R));}J->SetArrayField(TEXT("sources"),Items);FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);return Text;
}

UFoundrylineManufacturingAudio* UFoundrylineManufacturingAudio::ForWorld(const UObject* Context){return Context&&Context->GetWorld()?Context->GetWorld()->GetSubsystem<UFoundrylineManufacturingAudio>():nullptr;}
