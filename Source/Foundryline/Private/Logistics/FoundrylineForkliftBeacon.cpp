#include "Logistics/FoundrylineForkliftBeacon.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Actor.h"
UPointLightComponent* FFoundrylineForkliftBeacon::Attach(AActor* Owner,USceneComponent* Vehicle){
 if(!Owner||!Vehicle||Owner->GetNetMode()==NM_DedicatedServer)return nullptr;
 auto* Light=NewObject<UPointLightComponent>(Owner);Light->SetupAttachment(Vehicle);Light->SetRelativeLocation(BulbCm());Light->SetMobility(EComponentMobility::Movable);
 Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(45);Light->SetLightColor(FFoundrylineForkliftBeacon::Colour());Light->SetAttenuationRadius(120);Light->SetCastShadows(false);Light->SetSourceRadius(2);
 Light->RegisterComponent();Owner->AddInstanceComponent(Light);return Light;
}
