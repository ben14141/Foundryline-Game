#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FoundrylineManufacturingAudio.generated.h"

USTRUCT()
struct FFoundrylineManufacturingVoice {
 GENERATED_BODY()
 UPROPERTY() TObjectPtr<class UAudioComponent> Component;
 FName Role;
 float Volume=0,StopAt=0;
};

// Cosmetic audio is rebuilt independently on each listening client from the shared simulation.
UCLASS()
class FOUNDRYLINE_API UFoundrylineManufacturingAudio : public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual bool DoesSupportWorldType(EWorldType::Type Type)const override;
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 virtual void Tick(float Dt) override;
 virtual TStatId GetStatId()const override;
 virtual bool IsTickable()const override;
 UFUNCTION(BlueprintPure,Category="Foundryline|Audio") FString GetAudioReport()const;
 UFUNCTION(BlueprintPure,Category="Foundryline|Audio",meta=(WorldContext="Context")) static UFoundrylineManufacturingAudio* ForWorld(const UObject* Context);
 static float CrowdGain(int32 Sources){return 1.f/FMath::Sqrt(float(FMath::Max(1,Sources)));}
 static constexpr int32 MaxVoices=8;
private:
 UPROPERTY() TMap<FString,FFoundrylineManufacturingVoice> Voices;
 UPROPERTY() TMap<FName,TObjectPtr<class USoundBase>> Sounds;
 UPROPERTY() TObjectPtr<class USoundConcurrency> Concurrency;
 UPROPERTY() TObjectPtr<class USoundClass> MachineryClass;
 float Accumulator=0;
 int32 CandidateCount=0,MissingAssets=0;
 TMap<FString,float> PreviousDistance,MovementUntil,PreviousPhase,ContactUntil;
};
