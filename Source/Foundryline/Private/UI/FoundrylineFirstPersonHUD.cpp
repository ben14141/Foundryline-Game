#include "UI/FoundrylineBuildHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AFoundrylineBuildHUD::DrawFirstPersonPrompt(const FString& Title,const FString& Detail)
{
 if(!Canvas)return;
 const float S=FMath::Clamp(FMath::Min(Canvas->ClipY/900.f,Canvas->ClipX/1440.f),.65f,1.4f);
 const float X=Canvas->ClipX*.5f-200*S,Y=Canvas->ClipY-190*S;
 const FLinearColor Ink(FColor::FromHex(TEXT("1F2327"))),Paper(FColor::FromHex(TEXT("F7F5F1"))),Orange(FColor::FromHex(TEXT("F07A2F"))),Grey(FColor::FromHex(TEXT("4B5258")));
 DrawRect(FLinearColor(0,0,0,.2f),X+3*S,Y+3*S,400*S,76*S);DrawRect(Paper,X,Y,400*S,76*S);DrawRect(Orange,X,Y,4*S,76*S);
 DrawRect(Ink,X+18*S,Y+18*S,40*S,40*S);DrawText(TEXT("E"),Paper,X+31*S,Y+29*S,GEngine->GetMediumFont(),1.3f*S);
 float W,H;GetTextSize(Title,W,H,GEngine->GetMediumFont(),1.3f*S);
 const float Fit=FMath::Min(1.3f*S,300*S/FMath::Max(1.f,W)*1.3f*S);
 DrawText(Title,Ink,X+76*S,Y+17*S,GEngine->GetMediumFont(),Fit);
 DrawText(Detail,Grey,X+76*S,Y+46*S,GEngine->GetMediumFont(),.85f*S);
}
