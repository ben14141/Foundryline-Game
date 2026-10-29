#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
void AFoundrylineBuildHUD::DrawFreightControls(AFoundrylinePlayerController* PC) {
 if(PC->IsWalking()) return;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f);
 const float X=440*S,Y=Canvas->ClipY-190*S,W=700*S;
 if(!PC->LocalFreightRig) {
  DrawRect(FLinearColor(.1,.12,.14,.95),X,Canvas->ClipY-60*S,230*S,42*S);
  DrawText(TEXT("TRUCK TEST  [V]"),FLinearColor::White,X+14*S,Canvas->ClipY-48*S,GEngine->GetMediumFont(),1.4*S);
  AddHitBox({X,Canvas->ClipY-60*S},{230*S,42*S},TEXT("FreightDrive"),true);return;
 }
 const auto* R=PC->LocalFreightRig.Get();
 DrawRect(FLinearColor(.06,.07,.09,.96),X,Y,W,172*S);
 DrawRect(FLinearColor(1,.4,.1,1),X,Y,4*S,172*S);
 auto Text=[&](FString V,float Row,FLinearColor C=FLinearColor::White){DrawText(V,C,X+18*S,Y+Row*S,GEngine->GetMediumFont(),1.25*S);};
 Text(TEXT("TRACTOR + TANKER TEST"),14);
 Text(TEXT("W / S  forward / reverse    A / D  steer    V  return to map"),46);
 Text(TEXT("Get close to auto-connect    U  auto-detach / lower legs"),72);
 const FString State=R->Driver && R->Driver!=PC->PlayerState?TEXT("Another player is driving"):FString::Printf(TEXT("%s   |   Legs %.0f%% down   |   %.1f km/h"),R->State.Detaching?TEXT("DETACHING"):R->State.Coupled?TEXT("COUPLED"):TEXT("DETACHED"),R->State.Gear*100,FMath::Abs(R->State.Speed)*.036f);
 Text(State,104,FLinearColor(.4,.8,1,1));Text(R->Feedback,138,FLinearColor(1,.7,.4,1));
}
