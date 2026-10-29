#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineSizing.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
void AFoundrylineBuildHUD::DrawSurfaceSizing(AFoundrylinePlayerController* PC) {
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),X=Canvas->ClipX/S-390,Y=276;
 const FLinearColor Ink(FColor::FromHex(TEXT("1F2327"))),Paper(FColor::FromHex(TEXT("F7F5F1"))),Orange(FColor::FromHex(TEXT("F07A2F"))),Grey(FColor::FromHex(TEXT("9AA0A6")));
 auto Rect=[&](float A,float B,float W,float H,FLinearColor C){DrawRect(C,A*S,B*S,W*S,H*S);};
 auto Text=[&](FString V,float A,float B,float Size,FLinearColor C){DrawText(V,C,A*S,B*S,GEngine->GetMediumFont(),Size*S);};
 auto Button=[&](FName Key,FString Label,float A,float B,float W,bool Active=false){auto& H=HoverAmounts.FindOrAdd(Key);H=FMath::FInterpTo(H,HoveredBox==Key?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);Rect(A,B,W,40,Active?FLinearColor(FColor::FromHex(TEXT("E8E7E2"))):FMath::Lerp(Paper,FLinearColor(.7f,.72f,.73f),H*.25f));if(Active) Rect(A,B+7,3,26,Orange);Text(Label,A+12+H*2,B+12,1.3f,Ink);AddHitBox({A*S,B*S},{W*S,40*S},Key,true);};
 // Surface construction always starts with a map drag; no size presets switch
 // the pointer into single-click stamping.
 Rect(X,Y,366,PC->BuildTool==TEXT("TruckAccess")?280:220,Paper);
 Text(TEXT("DRAW PAVEMENT"),X+18,Y+18,1.8f,Ink);
 Text(TEXT("Hold left mouse at the first corner."),X+18,Y+60,1.25f,Ink);
 Text(TEXT("Drag to size. Release to build."),X+18,Y+88,1.25f,Ink);
 Text(TEXT("Right-click cancels. Grid snapping is on."),X+18,Y+122,1.15f,Ink);
 const auto Size=PC->GetConstructionSizeCm();
 if(PC->BuildTool==TEXT("TruckAccess")) {
  Text(TEXT("ACCESS ROAD WIDTH"),X+18,Y+159,1.2f,Ink);
  Button(TEXT("SizeWidthLess"),TEXT("-"),X+18,Y+185,52);
  Text(FString::Printf(TEXT("%.0f m"),Size.X/100.f),X+110,Y+196,2.f,Ink);
  Button(TEXT("SizeWidthMore"),TEXT("+"),X+294,Y+185,52);
  Text(TEXT("Drag sets direction and length."),X+18,Y+245,1.15f,Ink);
 } else {
  Text(FFoundrylineSizing::Road(PC->BuildTool)?FString::Printf(TEXT("Road width: %.0f m / drag the run"),Size.X/100.f):TEXT("Width and depth follow your drag."),X+18,Y+162,1.25f,Ink);
  Text(TEXT("Live dimensions appear beside the hologram."),X+18,Y+193,1.1f,Ink);
 }

}
void AFoundrylineBuildHUD::DrawPlacementGuide(AFoundrylinePlayerController* PC) {
 if(PC->IsLogisticsOverlayVisible() || !PC->bShowPlanningGrid || PC->IsOverBuildHUD()) return;
 FIntVector Anchor;if(!PC->GetCursorAnchor(Anchor)) return;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f);int32 VW=0,VH=0;PC->GetViewportSize(VW,VH);if(VW<=0 || VH<=0) return;
 auto Project=[&](FVector P,FVector2D& Out){if(!PC->ProjectWorldLocationToScreen(P,Out,true)) return false;Out.X*=Canvas->ClipX/VW;Out.Y*=Canvas->ClipY/VH;return true;};
 auto Line=[&](FVector A,FVector B,FLinearColor Color,float Thick){FVector2D P,Q;if(Project(A,P) && Project(B,Q)) DrawLine(P.X,P.Y,Q.X,Q.Y,Color,Thick*S);};
 // All guides use the same ground plane and anchor as picking. The old cell
 // box was displaced half a metre, and debug-world lines could draw over HUD.
 const FVector Center(Anchor);const FLinearColor Blue(.12f,.45f,.8f,.6f),Orange(1,.45f,.12f,1);
 const int32 Step=FFoundrylineGrid::SnapStep(PC->GetPlacementSnap());
 if(Step<=0) return;
 for(int32 I=-10;I<=10;++I) {Line(Center+FVector(I*Step,-10*Step,0),Center+FVector(I*Step,10*Step,0),Blue,1);Line(Center+FVector(-10*Step,I*Step,0),Center+FVector(10*Step,I*Step,0),Blue,1);}
 FVector2D P;if(!Project(Center,P)) return; {DrawLine(P.X-7*S,P.Y,P.X+7*S,P.Y,Orange,2*S);DrawLine(P.X,P.Y-7*S,P.X,P.Y+7*S,Orange,2*S);}
 FIntVector Start,End;const bool Drag=PC->IsConstructionDragging(Start,End);
 if(!Drag && PC->bExactSurface && FFoundrylineSizing::Surface(PC->BuildTool)) {Start=Anchor;End=PC->ExactSurfaceEnd(Anchor);}
 else if(!Drag) return;
 const bool Horizontal=FMath::Abs(End.X-Start.X)>FMath::Abs(End.Y-Start.Y);
 // Invalid small drags still show the intended footprint instead of vanishing.
 const FVector RawA(Start.X,Start.Y,0),RawB(End.X,Start.Y,0),RawC(End.X,End.Y,0),RawD(Start.X,End.Y,0);
 const FLinearColor Pending(1,.35f,.1f,.9f);
 Line(RawA,RawB,Pending,2);Line(RawB,RawC,Pending,2);Line(RawC,RawD,Pending,2);Line(RawD,RawA,Pending,2);

 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) if(It->bHasPreviewBounds) {
  const auto& B=It->PreviewBounds;const FVector A(B.Min.X,B.Min.Y,0),C(B.Max.X,B.Min.Y,0),D(B.Max.X,B.Max.Y,0),E(B.Min.X,B.Max.Y,0);
  const auto Color=It->IsConstructionFeedbackError()?FLinearColor(1,.1f,.08f,1):FLinearColor(.12f,.7f,1,1);Line(A,C,Color,2);Line(C,D,Color,2);Line(D,E,Color,2);Line(E,A,Color,2);
  for(int32 I=0;I+1<It->PreviewJoinLines.Num();I+=2) {
   const auto J=It->PreviewJoinLines[I],K=It->PreviewJoinLines[I+1];
   Line(J,K,FLinearColor(.15f,1,.48f,1),5);
   FVector2D Mid;if(Project((J+K)/2,Mid)) {
    const FString Join=FString::Printf(TEXT("JOIN %.0f m"),FVector::Distance(J,K)/100);
    DrawRect(FLinearColor(.03f,.09f,.06f,.94f),Mid.X-60*S,Mid.Y-15*S,120*S,30*S);
    DrawText(Join,FLinearColor(.3f,1,.6f),Mid.X-50*S,Mid.Y-7*S,GEngine->GetMediumFont(),1.25f*S);
   }
  }
  // Road connection snapping may trim the run. Report the hologram, not the raw drag.
  Start=B.Min;End=B.Max;FVector2D Corner;if(Project(FVector(B.Max.X,B.Max.Y,0),Corner)) P=Corner;break;
 }
 const auto Delta=End-Start;
 FString Label=FString::Printf(TEXT("%.0f x %.0f m"),FMath::Abs(Delta.X)/100.f,FMath::Abs(Delta.Y)/100.f);
 if(FFoundrylineSizing::Road(PC->BuildTool)) Label=FString::Printf(TEXT("%.0f m wide / %.0f m long"),PC->GetConstructionSizeCm().X/100.f,(Horizontal?FMath::Abs(Delta.X):FMath::Abs(Delta.Y))/100.f);
 const float X=FMath::Clamp(float(P.X)+18*S,420*S,Canvas->ClipX-430*S),Y=FMath::Clamp(float(P.Y)+18*S,20*S,Canvas->ClipY-40*S);
 DrawRect(FLinearColor(.03f,.04f,.05f,.94f),X,Y,280*S,32*S);DrawText(Label,FLinearColor::White,X+10*S,Y+8*S,GEngine->GetMediumFont(),1.3f*S);
}
