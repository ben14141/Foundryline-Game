#include "Audio/FoundrylineUIAudio.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"

void UFoundrylineUIAudio::Play(APlayerController* Player,FName Cue)
{
 if(!Player||!Player->IsLocalController()||Player->GetNetMode()==NM_DedicatedServer)return;
 auto* Local=Player->GetLocalPlayer();auto* Self=Local?Local->GetSubsystem<UFoundrylineUIAudio>():nullptr;if(!Self)return;
 struct FEntry{const TCHAR* Key;const TCHAR* Folder;const TCHAR* File;float Volume;};
 static const FEntry Entries[]={
  {TEXT("Hover"),TEXT("01_Buttons"),TEXT("button_hover"),.12f},
  {TEXT("Click"),TEXT("01_Buttons"),TEXT("button_click"),.28f},
  {TEXT("Tab"),TEXT("02_Controls"),TEXT("tab_switch"),.23f},
  {TEXT("Open"),TEXT("03_Navigation"),TEXT("popup_open"),.28f},
  {TEXT("Close"),TEXT("04_Feedback"),TEXT("cancel"),.22f},
  {TEXT("Confirm"),TEXT("04_Feedback"),TEXT("confirm"),.28f},
  {TEXT("Error"),TEXT("04_Feedback"),TEXT("error"),.25f}};
 const FEntry* Entry=nullptr;for(const auto& E:Entries)if(Cue==E.Key){Entry=&E;break;}if(!Entry)return;
 const double Now=Player->GetWorld()->GetRealTimeSeconds();
 if(const double* Last=Self->LastPlayed.Find(Cue);Last&&Now-*Last<(Cue==TEXT("Hover")?.09:.10))return;
 if(Cue==TEXT("Hover")&&Now-Self->LastAction<.14)return;
 Self->LastPlayed.Add(Cue,Now);if(Cue!=TEXT("Hover"))Self->LastAction=Now;
 auto& Sound=Self->Sounds.FindOrAdd(Cue);
 if(!Sound){const FString Name=FString::Printf(TEXT("cleanpaper_%s_01"),Entry->File);Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/UI/Audio/Softline/Clean_Paper/Sounds/%s/%s.%s"),Entry->Folder,*Name,*Name));}
 if(Sound){if(!Self->VoiceLimit){Self->VoiceLimit=NewObject<USoundConcurrency>(Self);Self->VoiceLimit->Concurrency.MaxCount=4;Self->VoiceLimit->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopOldest;}Self->Played.FindOrAdd(Cue)++;UGameplayStatics::PlaySound2D(Player,Sound,Entry->Volume,1,0,Self->VoiceLimit,Player,true);}
}

FString UFoundrylineUIAudio::PlaybackReport(APlayerController* Player)
{
 auto J=MakeShared<FJsonObject>();auto* Local=Player?Player->GetLocalPlayer():nullptr;auto* Self=Local?Local->GetSubsystem<UFoundrylineUIAudio>():nullptr;
 J->SetBoolField(TEXT("local"),Self!=nullptr);J->SetNumberField(TEXT("voice_limit"),4);
 if(Self){for(const auto& P:Self->Played)J->SetNumberField(P.Key.ToString(),P.Value);for(const auto& P:Self->Sounds)J->SetStringField(P.Key.ToString()+TEXT("_asset"),P.Value?P.Value->GetPathName():TEXT("MISSING"));}
 FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));return Text;
}
