#pragma once
#include "CoreMinimal.h"
class AActor;
class USceneComponent;
class UPointLightComponent;
// Roof beacon light for the forklift (Codex's original beacon, SourceAssets/Vehicles/Forklift/AmberBeacon/manifest.json).
// The housing and lens are part of SM_Forklift_Body at roof-local (-115,0,253); the light sits at the bulb, 10.1 cm above it.
// Deep blue, like a warehouse pedestrian-warning light (Ben, 29 Oct), with the manifest's 45 lm, 120 cm radius and no
// shadows. Created locally on each machine from the vehicle it belongs to.
struct FOUNDRYLINE_API FFoundrylineForkliftBeacon {
 static FVector BulbCm(){return {-115,0,263.1};}
 static FLinearColor Colour(){return {.01f,.08f,1.f};}
 static UPointLightComponent* Attach(AActor* Owner,USceneComponent* Vehicle);
};
