#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Audio/FoundrylineUIAudio.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "InputKeyEventArgs.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"

namespace {
struct Entry{const TCHAR* Action;const TCHAR* Label;};
TArray<Entry> Entries(int Page){
 if(Page==1)return {{TEXT("WheelRect"),TEXT("RECTANGLE")},{TEXT("WheelCustom"),TEXT("CUSTOM SHAPE")},{TEXT("WheelPaint"),TEXT("PAINT")},{TEXT("WheelEdit"),TEXT("EDIT")},{TEXT("WheelErase"),TEXT("ERASE")},{TEXT("WheelSidewalk"),TEXT("SIDEWALK")}};
 if(Page==2)return {{TEXT("WheelStraight"),TEXT("STRAIGHT")},{TEXT("WheelCurve"),TEXT("ROAD SPLINE")},{TEXT("WheelTruck"),TEXT("TRUCK SPLINE")},{TEXT("WheelRadius"),TEXT("ENTRANCE")},{TEXT("WheelEdit"),TEXT("EDIT ROAD")},{TEXT("WheelPaint"),TEXT("MARKINGS")}};
 if(Page==3)return {{TEXT("WheelOverlay"),TEXT("OVERLAY")},{TEXT("WheelDrawRoute"),TEXT("DRAW ROUTE")},{TEXT("WheelEditRoute"),TEXT("EDIT ROUTE")},{TEXT("WheelZones"),TEXT("DRIVE ZONES")},{TEXT("WheelParking"),TEXT("PARKING")},{TEXT("WheelTrip"),TEXT("TRIP CHECK")}};
 if(Page==4||Page==5)return {{TEXT("WheelMark0"),TEXT("PARKING")},{TEXT("WheelMark1"),TEXT("WHITE LINE")},{TEXT("WheelMark2"),TEXT("YELLOW LINE")},{TEXT("WheelMark3"),TEXT("DASHED LINE")},{TEXT("WheelMark4"),TEXT("ARROW")},{TEXT("WheelLoadingBay"),TEXT("LOADING BAY")},{TEXT("WheelMark6"),TEXT("KEEP CLEAR")},{TEXT("WheelMark7"),TEXT("STOP LINE")}};
 return {{TEXT("WheelPavement"),TEXT("PAVEMENT")},{TEXT("WheelRoads"),TEXT("ROADS")},{TEXT("WheelLogistics"),TEXT("LOGISTICS")}};
}
FLinearColor Hex(const TCHAR* Value){return FLinearColor(FColor::FromHex(Value));}
}
bool AFoundrylineBuildHUD::PlanningWheelAvailable()const{
 const auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());return PC&&PC->bPropertyViewAccess&&!PC->IsWalking()&&!PC->OperationsScreen&&!PC->bFactoryMenu&&!PC->LocalFreightRig&&!PC->LocalMaterialCell;
}
FString AFoundrylineBuildHUD::PlanningWheelTool()const{
 const auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(!PC)return TEXT("SELECT");const auto Tool=PC->BuildTool;
 if(Tool==TEXT("LoadingBay"))return TEXT("SMALL FLATBED BAY");if(Tool==TEXT("UpgradeBay"))return TEXT("UPGRADE LOADING BAY");
 if(Tool==TEXT("PaveSpline"))return PC->PaveTruck?TEXT("TRUCK SPLINE"):TEXT("ROAD SPLINE");
 if(Tool==TEXT("PaveRoad"))return PC->PaveTruck?(PC->PaveCurved?TEXT("CURVED TRUCK ROAD"):TEXT("TRUCK ROAD")):PC->PaveCurved?TEXT("CURVED ROAD"):TEXT("STRAIGHT ROAD");
 if(Tool==TEXT("PavePaint")){const TCHAR* Names[]={TEXT("PARKING"),TEXT("WHITE LINE"),TEXT("YELLOW LINE"),TEXT("DASHED LINE"),TEXT("ARROWS"),TEXT("LOADING BAY"),TEXT("KEEP CLEAR"),TEXT("STOP LINE")};return Names[FMath::Clamp(int(PC->PaveMark),0,7)];}
 const TMap<FName,FString> Names={{TEXT("PaveRect"),TEXT("PAVE RECTANGLE")},{TEXT("PavePolygon"),TEXT("CUSTOM SHAPE")},{TEXT("PaveEdit"),TEXT("EDIT PAVEMENT")},{TEXT("PaveErase"),TEXT("ERASE PAVEMENT")},{TEXT("Walkway"),TEXT("SIDEWALK")},{TEXT("NetDraw"),TEXT("DRAW ROUTE")},{TEXT("NetSelect"),TEXT("EDIT ROUTE")},{TEXT("RoutePaint"),TEXT("DRIVING ZONES")},{TEXT("RouteErase"),TEXT("ERASE ZONES")},{TEXT("Building"),TEXT("TIER 1 SHOP")},{TEXT("Shop2"),TEXT("TIER 2 SHOP")},{TEXT("OfficeArea"),TEXT("OFFICE")}};
 if(const auto* Name=Names.Find(Tool))return *Name;if(Tool.ToString().StartsWith(TEXT("Trip")))return TEXT("TRIP CHECK");return Tool.IsNone()||Tool==TEXT("Select")?TEXT("SELECT / INSPECT"):Tool.ToString().ToUpper();
}
bool AFoundrylineBuildHUD::IsOverPlanningWheel()const{
 if(!PlanningWheelAvailable()||!WheelButton.bIsValid)return false;const auto* PC=GetOwningPlayerController();float X,Y;if(!PC->GetMousePosition(X,Y))return false;int W,H;PC->GetViewportSize(W,H);if(W<=0||H<=0)return false;
 X=(X-WheelScreenOrigin.X)/WheelScreenScale.X;Y=(Y-WheelScreenOrigin.Y)/WheelScreenScale.Y;const FVector2D P(X,Y);return WheelButton.IsInside(P)||(bPlanningWheelOpen&&FVector2D::Distance(P,FVector2D(WheelGeometry.X,WheelGeometry.Y))<=WheelGeometry.Z+8);
}
bool AFoundrylineBuildHUD::HandlePlanningWheelInput(const FInputKeyEventArgs& P){
 if(!PlanningWheelAvailable()){bPlanningWheelOpen=false;bWheelPointerCapture=false;return false;}
 if(P.Key==EKeys::Tab&&P.Event==IE_Pressed){PlanningWheelAction(TEXT("WheelToggle"));return true;}
 if(bPlanningWheelOpen&&P.Event==IE_Pressed&&(P.Key==EKeys::RightMouseButton||P.Key==EKeys::Escape)){bPlanningWheelOpen=false;WheelPage=0;return true;}
 if(P.Key!=EKeys::LeftMouseButton)return false;if(P.Event==IE_Released&&bWheelPointerCapture){bWheelPointerCapture=false;return true;}if(P.Event!=IE_Pressed)return false;
 const bool Over=IsOverPlanningWheel();if(!Over&&!bPlanningWheelOpen)return false;bWheelPointerCapture=true;if(!Over){bPlanningWheelOpen=false;return true;}
 auto* PC=GetOwningPlayerController();float X,Y;PC->GetMousePosition(X,Y);int W,H;PC->GetViewportSize(W,H);X=(X-WheelScreenOrigin.X)/WheelScreenScale.X;Y=(Y-WheelScreenOrigin.Y)/WheelScreenScale.Y;const FVector2D At(X,Y),Center(WheelGeometry.X,WheelGeometry.Y);
 if(WheelButton.IsInside(At)){NotifyHitBoxClick(TEXT("WheelToggle"));return true;}
 const auto Delta=At-Center;const double Radius=Delta.Size();if(Radius<WheelGeometry.Z*.42){NotifyHitBoxClick(WheelPage?TEXT("WheelBack"):TEXT("WheelToggle"));return true;}
 const auto Items=Entries(WheelPage);const double Step=360./Items.Num(),Start=-90-Step/2,Angle=FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X))-Start+720,360.);const int Index=FMath::Clamp(FMath::FloorToInt(Angle/Step),0,Items.Num()-1);
 if(FMath::Fmod(Angle,Step)>2&&FMath::Fmod(Angle,Step)<Step-2)NotifyHitBoxClick(FName(Items[Index].Action));return true;
}
void AFoundrylineBuildHUD::PlanningWheelAction(FName Action){
 if(!PlanningWheelAvailable())return;auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(PC->IsConstructionPending())return;
 if(Action==TEXT("WheelToggle")){FIntVector A,B;if(!bPlanningWheelOpen&&(PC->IsConstructionDragging(A,B)||PC->PaveAwaitBend||!PC->PavePoints.IsEmpty())){for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)It->Status=TEXT("Finish the draft or right-click to cancel before changing tools");return;}bPlanningWheelOpen=!bPlanningWheelOpen;WheelPage=0;return;}
 if(Action==TEXT("WheelBack")){WheelPage=WheelPage==5?2:WheelPage==4?1:0;return;}
 if(Action==TEXT("WheelPavement")||Action==TEXT("WheelRoads")||Action==TEXT("WheelLogistics")||Action==TEXT("WheelPaint")){bPlanningWheelOpen=true;WheelPage=Action==TEXT("WheelPavement")?1:Action==TEXT("WheelRoads")?2:Action==TEXT("WheelLogistics")?3:WheelPage==2?5:4;return;}
 const bool Logistics=Action==TEXT("WheelOverlay")||Action==TEXT("WheelDrawRoute")||Action==TEXT("WheelEditRoute")||Action==TEXT("WheelZones")||Action==TEXT("WheelParking")||Action==TEXT("WheelTrip");
 if(Logistics){
  if(Action==TEXT("WheelDrawRoute")){PC->LogisticsPanel=0;PC->LogisticsAction(TEXT("NetNew"));}
  else if(Action==TEXT("WheelEditRoute"))PC->LogisticsAction(TEXT("NetPanel"));
  else if(Action==TEXT("WheelZones")){PC->LogisticsAction(TEXT("TripPanel"));PC->LogisticsAction(TEXT("RoutePaint"));}
  else if(Action==TEXT("WheelTrip"))PC->LogisticsAction(TEXT("TripPanel"));
  else {if(!PC->IsLogisticsOverlayVisible())PC->ToggleLogisticsOverlay();if(Action==TEXT("WheelParking"))PC->bShowParkingLayer=true;}
 }else{
  if(PC->IsLogisticsOverlayVisible())PC->ToggleLogisticsOverlay();bEditTab=bOfficeTab=bPaintTab=false;BuildCategory=TEXT("Roads");PC->PavePalettePage=0;PC->bPaveAdvanced=false;
  if(Action==TEXT("WheelRect"))PC->PavementAction(TEXT("Rect"));else if(Action==TEXT("WheelCustom"))PC->PavementAction(TEXT("Polygon"));else if(Action==TEXT("WheelErase")){PC->PavePalettePage=2;PC->PavementAction(TEXT("Erase"));}else if(Action==TEXT("WheelSidewalk"))PC->PavementAction(TEXT("Walkway"));
  else if(Action==TEXT("WheelEdit")||Action==TEXT("WheelRadius")){PC->PavementAction(TEXT("EditMenu"));if(Action==TEXT("WheelRadius"))for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)It->Status=TEXT("Select a road, then adjust its entrance radius on the right");}
  else if(Action==TEXT("WheelLoadingBay")){PC->PavementAction(TEXT("BayPlace"));}
  else if(Action.ToString().StartsWith(TEXT("WheelMark"))){PC->PavePalettePage=1;PC->PavementAction(FName(*Action.ToString().RightChop(5)));}
  else {PC->PavePalettePage=0;PC->PavementAction(Action==TEXT("WheelStraight")?TEXT("Road"):TEXT("Spline"));PC->PavementAction(Action==TEXT("WheelTruck")?TEXT("TruckPreset"):TEXT("StandardPreset"));if(PC->PaveCurved)PC->PavementAction(TEXT("Curve"));}
 }
 bPlanningWheelOpen=false;WheelPage=0;
}
void AFoundrylineBuildHUD::DrawPlanningWheel(AFoundrylinePlayerController* PC){
 if(!PlanningWheelAvailable()){WheelButton.Init();return;}
 WheelScreenOrigin=FVector2D::ZeroVector;WheelScreenScale={1,1};
 if(auto* LP=PC->GetLocalPlayer();LP&&LP->ViewportClient&&LP->ViewportClient->Viewport){FSceneViewProjectionData Data;if(LP->GetProjectionData(LP->ViewportClient->Viewport,Data)){const auto Rect=Data.GetConstrainedViewRect();WheelScreenOrigin={double(Rect.Min.X),double(Rect.Min.Y)};WheelScreenScale={Rect.Width()/double(Canvas->ClipX),Rect.Height()/double(Canvas->ClipY)};}}
 const float S=FMath::Clamp(FMath::Min(Canvas->ClipX/1600,Canvas->ClipY/1080),.65f,1.25f),CX=Canvas->ClipX*.5f+24*S,CY=Canvas->ClipY-230*S,R=158*S;WheelGeometry={CX,CY,R};WheelButton=FBox2D({CX-152*S,Canvas->ClipY-64*S},{CX+152*S,Canvas->ClipY-14*S});
 const auto Paper=Hex(TEXT("F7F5F1")),Ink=Hex(TEXT("242B31")),Grey=Hex(TEXT("677077")),Orange=Hex(TEXT("ED7B2D")),Muted=Hex(TEXT("E6E7E3"));
 auto CenterText=[&](const FString& Value,FVector2D At,float Size,FLinearColor C){float W,H;GetTextSize(Value,W,H,GEngine->GetMediumFont(),Size*S);DrawText(Value,C,At.X-W/2,At.Y-H/2,GEngine->GetMediumFont(),Size*S);};
 DrawRect(Ink,WheelButton.Min.X+3*S,WheelButton.Min.Y+3*S,WheelButton.GetSize().X,WheelButton.GetSize().Y);DrawRect(Paper,WheelButton.Min.X,WheelButton.Min.Y,WheelButton.GetSize().X,WheelButton.GetSize().Y);DrawRect(Orange,WheelButton.Min.X,WheelButton.Min.Y,4*S,WheelButton.GetSize().Y);CenterText(PlanningWheelTool(),{CX-18*S,WheelButton.GetCenter().Y-6*S},1.15f,Ink);CenterText(bPlanningWheelOpen?TEXT("CLOSE  [TAB]"):TEXT("TOOL WHEEL  [TAB]"),{CX-18*S,WheelButton.GetCenter().Y+13*S},.78f,Grey);CenterText(bPlanningWheelOpen?TEXT("-"):TEXT("+"),{WheelButton.Max.X-25*S,WheelButton.GetCenter().Y},1.6f,Orange);
 if(!bPlanningWheelOpen)return;const auto Items=Entries(WheelPage);float MouseX=0,MouseY=0;PC->GetMousePosition(MouseX,MouseY);int VW,VH;PC->GetViewportSize(VW,VH);MouseX=(MouseX-WheelScreenOrigin.X)/WheelScreenScale.X;MouseY=(MouseY-WheelScreenOrigin.Y)/WheelScreenScale.Y;
 const FVector2D Delta=FVector2D(MouseX,MouseY)-FVector2D(CX,CY);const double Step=360./Items.Num(),Start=-90-Step/2,Angle=FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X))-Start+720,360.);const int Hover=Delta.Size()>R*.42&&Delta.Size()<R+8?int(Angle/Step):-1;
 auto Point=[&](double Degrees,double Radius){const double A=FMath::DegreesToRadians(Degrees);return FVector2D(CX+FMath::Cos(A)*Radius,CY+FMath::Sin(A)*Radius);};
 for(int I=0;I<Items.Num();++I){const FName Key(Items[I].Action);float& Amount=HoverAmounts.FindOrAdd(Key);Amount=FMath::FInterpTo(Amount,Hover==I?1.f:0.f,GetWorld()->GetDeltaSeconds(),16);const double Outer=R+Amount*4*S,Inner=R*.43,A=Start+Step*I+2,B=Start+Step*(I+1)-2;const auto Color=FMath::Lerp(Muted,Orange,Amount*.85f);
  for(int J=0;J<16;++J){const double From=FMath::Lerp(A,B,double(J)/16),To=FMath::Lerp(A,B,double(J+1)/16);FCanvasTriangleItem T(Point(From,Inner),Point(From,Outer),Point(To,Outer),GWhiteTexture);T.SetColor(Color);Canvas->DrawItem(T);FCanvasTriangleItem Q(Point(From,Inner),Point(To,Outer),Point(To,Inner),GWhiteTexture);Q.SetColor(Color);Canvas->DrawItem(Q);}
  const auto At=Point(Start+(I+.5)*Step,R*.75);const bool Preset=WheelPage==2&&I<3;CenterText(Items[I].Label,At-FVector2D(0,Preset?8*S:0),Items.Num()==3?1.2f:1.f,Ink);if(Preset)CenterText(I==2?TEXT("30 FT / 2 WAY"):TEXT("24 FT / 2 WAY"),At+FVector2D(0,11*S),.67f,Grey);
 }
 for(int I=0;I<64;++I){FCanvasTriangleItem T(FVector2D(CX,CY),Point(I*360./64,R*.405),Point((I+1)*360./64,R*.405),GWhiteTexture);T.SetColor(Paper);Canvas->DrawItem(T);}
 CenterText(TEXT("USING"),{CX,CY-25*S},.8f,Grey);const auto Label=PlanningWheelTool();int Split=INDEX_NONE;Label.FindLastChar(' ',Split);if(Label.Len()>12&&Split>0){CenterText(Label.Left(Split),{CX,CY-6*S},1.05f,Ink);CenterText(Label.Mid(Split+1),{CX,CY+10*S},1.05f,Ink);}else CenterText(Label,{CX,CY},1.05f,Ink);CenterText(WheelPage?TEXT("< BACK"):TEXT("CLOSE"),{CX,CY+33*S},.78f,Orange);
 DrawRect(Ink,CX-145*S,CY-R-34*S,290*S,28*S);const TCHAR* Titles[]={TEXT("PLANNING TOOLS"),TEXT("PAVEMENT"),TEXT("ROADS & ENTRANCES"),TEXT("LOGISTICS"),TEXT("PAVEMENT MARKINGS")};CenterText(Titles[FMath::Clamp(WheelPage,0,4)],{CX,CY-R-20*S},1.05f,Paper);
}

FVector AFoundrylineBuildHUD::PlanningWheelGeometry()const{return {WheelGeometry.X*WheelScreenScale.X+WheelScreenOrigin.X,WheelGeometry.Y*WheelScreenScale.Y+WheelScreenOrigin.Y,WheelGeometry.Z*WheelScreenScale.Y};}
FVector2D AFoundrylineBuildHUD::PlanningWheelButtonCenter()const{return WheelButton.GetCenter()*WheelScreenScale+WheelScreenOrigin;}
