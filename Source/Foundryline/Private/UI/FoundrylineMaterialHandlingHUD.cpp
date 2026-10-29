#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
void AFoundrylineBuildHUD::DrawMaterialHandling(AFoundrylinePlayerController* PC){
 if(PC->IsWalking())return;const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f);const float X=440*S;
 if(!PC->LocalMaterialCell){
  // The forklift test only has a button when a cell actually exists in the level.
  if(PC->LocalFreightRig||!TActorIterator<AFoundrylineMaterialHandlingCell>(GetWorld()))return;
  const float B=Canvas->ClipY-62*S;DrawRect(FLinearColor(FColor::FromHex(TEXT("F7F5F1"))),700*S,B,250*S,42*S);DrawRect(FLinearColor(FColor::FromHex(TEXT("F07A2F"))),700*S,B,4*S,42*S);
  DrawText(TEXT("MATERIALS  [K]"),FLinearColor(FColor::FromHex(TEXT("1F2327"))),716*S,B+13*S,GEngine->GetMediumFont(),1.25*S);AddHitBox({700*S,B},{250*S,42*S},TEXT("MaterialHandling"),true);return;}
 PC->BuildHUDCanvasSize={Canvas->ClipX,Canvas->ClipY};const auto* C=PC->LocalMaterialCell.Get();const float Y=Canvas->ClipY-326*S,W=820*S;
 DrawRect(FLinearColor(.055f,.065f,.08f,.97f),X,Y,W,308*S);DrawRect(FLinearColor(1,.4f,.1f,1),X,Y,4*S,308*S);
 auto Text=[&](FString V,float R,FLinearColor Color=FLinearColor::White){DrawText(V,Color,X+18*S,Y+R*S,GEngine->GetMediumFont(),1.25*S);};
 Text(TEXT("MATERIAL HANDLING / 60 x 120 in STEEL"),14);Text(TEXT("W/S drive   A/D steer   Q/E lower/raise   Space pick/drop   K return"),44);
 auto Button=[&](int32 A,FString Label,float Left,float Width){const float Top=Y+78*S;const FName Key(*FString::Printf(TEXT("MaterialAction%d"),A));DrawRect(FLinearColor(.17f,.2f,.23f,1),Left,Top,Width*S,36*S);DrawText(Label,FLinearColor::White,Left+10*S,Top+10*S,GEngine->GetMediumFont(),1.1*S);AddHitBox({Left,Top},{Width*S,36*S},Key,true);};
 Button(2,TEXT("1 LOW RACK"),X+18*S,136);Button(3,TEXT("2 MID RACK"),X+164*S,136);Button(4,TEXT("3 HIGH RACK"),X+310*S,136);Button(5,TEXT("4 ROLLER INPUT"),X+456*S,164);Button(6,TEXT("R ROLL 1 SHEET"),X+630*S,170);
 for(int32 I=0;I<C->Stock.Num();++I){const auto& B=C->Stock[I];const FString Place=B.Place==EFoundrylineSheetPlace::Carried?TEXT("ON FORKLIFT"):B.Place==EFoundrylineSheetPlace::Rack?FString::Printf(TEXT("RACK %d"),B.RackLevel+1):B.Place==EFoundrylineSheetPlace::RollerInput?TEXT("ROLLER INPUT"):TEXT("FLOOR");Text(FString::Printf(TEXT("Load %c: %d sheets / %.0f lb steel / %s"),TCHAR('A'+I),B.Count,B.Pounds(),*Place),130+I*27,FLinearColor(.55f,.82f,1,1));}
 Text(FString::Printf(TEXT("Lift %.2f m   |   Raw sheets %d   |   Rolled blanks %d%s"),C->Pose.Lift/100.f,C->TotalSheets(),C->RolledBlanks,C->RollerRemaining>0?TEXT("   |   ROLLING..."):TEXT("")),190);
 Text(C->Driver && C->Driver!=PC->PlayerState?TEXT("Another player is driving this forklift."):C->Feedback,224,FLinearColor(1,.7f,.4f,1));
 Text(TEXT("Up to 3 sheets shown per load. Inventory and weight use the full count."),270,FLinearColor(.65f,.69f,.73f,1));
}
