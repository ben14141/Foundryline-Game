#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "FoundrylineUIAudio.generated.h"

/** Listener-local interface feedback. Never replicated and never used for world machinery. */
UCLASS()
class FOUNDRYLINE_API UFoundrylineUIAudio : public ULocalPlayerSubsystem
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Foundryline|Audio") static void Play(class APlayerController* Player, FName Cue);
 UFUNCTION(BlueprintPure,Category="Foundryline|Audio") static FString PlaybackReport(class APlayerController* Player);
private:
 UPROPERTY() TMap<FName,TObjectPtr<class USoundBase>> Sounds;
 UPROPERTY() TObjectPtr<class USoundConcurrency> VoiceLimit;
 TMap<FName,int32> Played;
 TMap<FName,double> LastPlayed;
 double LastAction=-100;
};
