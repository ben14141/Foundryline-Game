#include "Construction/FoundrylineLoadingBayLayout.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePavementGeometry.h"
#include "Engine/Canvas.h"
#include "Core/FoundrylineUnits.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

namespace {using PaveHUDG=FFoundrylinePavementGeometry;const FLinearColor PaveHUDPaper(FColor::FromHex(TEXT("F7F5F1"))),PaveHUDInk(FColor::FromHex(TEXT("242B31"))),PaveHUDGrey(FColor::FromHex(TEXT("677077"))),PaveHUDOrange(FColor::FromHex(TEXT("ED7B2D"))),PaveHUDMuted(FColor::FromHex(TEXT("E6E7E3")));}
void AFoundrylineBuildHUD::DrawPavementPalette(AFoundrylinePlayerController* PC){
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f);auto R=[&](float X,float Y,float W,float H,FLinearColor C){DrawRect(C,X*S,Y*S,W*S,H*S);};auto T=[&](FString V,float X,float Y,float Size,FLinearColor C){DrawText(V,C,X*S,Y*S,GEngine->GetMediumFont(),Size*S);};
 auto Button=[&](const TCHAR* Key,const TCHAR* Label,const TCHAR* Detail,float Y,bool Active){const FName Id(*(FString(TEXT("Pave"))+Key));float& Hover=HoverAmounts.FindOrAdd(Id);Hover=FMath::FInterpTo(Hover,HoveredBox==Id?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);R(24,Y,352,64,FMath::Lerp(Active?PaveHUDMuted:PaveHUDPaper,FLinearColor(1,.86f,.72f),Hover*.65));if(Active)R(24,Y+8,3,48,PaveHUDOrange);T(Label,40+Hover*2,Y+11,1.45f,PaveHUDInk);T(Detail,40,Y+39,1.04f,PaveHUDGrey);AddHitBox({24*S,Y*S},{352*S,64*S},Id,true);};
 float Y=478;
 const TCHAR* Tabs[]={TEXT("AreaMenu"),TEXT("PaintMenu"),TEXT("EditMenu")};const TCHAR* Titles[]={TEXT("PAVE"),TEXT("PAINT"),TEXT("EDIT")};
 for(int I=0;I<3;++I){const float X=24+I*120;const FName Id(*(FString(TEXT("Pave"))+Tabs[I]));R(X,Y,112,42,PC->PavePalettePage==I?PaveHUDOrange:PaveHUDMuted);T(Titles[I],X+25,Y+13,1.3f,PaveHUDInk);AddHitBox({X*S,Y*S},{112*S,42*S},Id,true);}Y+=58;
 if(PC->PavePalettePage==0){
  Button(TEXT("Rect"),TEXT("RECTANGLE"),TEXT("Drag to pave / touching areas join"),Y,PC->BuildTool==TEXT("PaveRect"));Y+=72;
  Button(TEXT("Spline"),TEXT("ROAD / DRIVEWAY SPLINE"),TEXT("Click points / Enter finishes"),Y,PC->BuildTool==TEXT("PaveSpline"));Y+=72;
  Button(TEXT("Polygon"),TEXT("CUSTOM SHAPE"),TEXT("Click corners / Enter finishes"),Y,PC->BuildTool==TEXT("PavePolygon"));Y+=72;
  Button(TEXT("Walkway"),TEXT("SIDEWALK"),TEXT("Drag a connected concrete path"),Y,PC->BuildTool==TEXT("Walkway"));Y+=90;
  Button(TEXT("BayPlace"),TEXT("LOADING BAY PRESET"),TEXT("Snap complete small-truck bay to a wall"),Y,PC->BuildTool==TEXT("LoadingBay"));
 }else if(PC->PavePalettePage==1){
  auto Sample=[&](int Kind,float X,float Y){
   FFoundrylinePavementMarking M;M.Kind=EFoundrylinePavementMark(Kind);M.Points={{0,0},{Kind==0?822.96:500,0}};M.WidthCm=300;const auto Lines=PaveHUDG::PaintLines(M);FBox2D Bounds(ForceInit);for(const auto& L:Lines){Bounds+=L.Key;Bounds+=L.Value;}const auto Size=Bounds.GetSize();const float Scale=FMath::Min(55.f/FMath::Max(1.,Size.X),30.f/FMath::Max(1.,Size.Y));const auto Color=(Kind==2||Kind==5||Kind==6)?FLinearColor(1,.72f,.15f):PaveHUDPaper;
   R(X-4,Y-5,66,42,PaveHUDInk);for(const auto& L:Lines){auto A=L.Key-Bounds.Min,B=L.Value-Bounds.Min;if(Kind==3){for(float T=0;T<1;T+=.28f){const auto P=FMath::Lerp(A,B,T),Q=FMath::Lerp(A,B,FMath::Min(1.f,T+.16f));DrawLine((X+P.X*Scale)*S,(Y+P.Y*Scale+4)*S,(X+Q.X*Scale)*S,(Y+Q.Y*Scale+4)*S,Color,2*S);}}else DrawLine((X+A.X*Scale)*S,(Y+A.Y*Scale+4)*S,(X+B.X*Scale)*S,(Y+B.Y*Scale+4)*S,Color,(Kind==7?4:2)*S);}
  };
  const TCHAR* Details[]={TEXT("Drag row / 9 x 18 ft spaces"),TEXT("Solid edge or lane marking"),TEXT("Solid yellow divider"),TEXT("Broken white lane divider"),TEXT("Drag in travel direction"),TEXT("Drag opposite corners"),TEXT("Drag a hatched rectangle"),TEXT("Drag across the approach")};
  const int Order[]={0,1,2,3,4,7,6};for(int Index=0;Index<7;++Index){const int I=Order[Index];if(Index==0||Index==1||Index==6){T(Index==0?TEXT("PARKING"):Index==1?TEXT("TRAFFIC MARKINGS"):TEXT("SAFETY"),32,Y,1.04f,PaveHUDGrey);Y+=24;}
   const FName Id(*FString::Printf(TEXT("PaveMark%d"),I));const bool Active=PC->BuildTool==TEXT("PavePaint")&&int(PC->PaveMark)==I;float& Hover=HoverAmounts.FindOrAdd(Id);Hover=FMath::FInterpTo(Hover,HoveredBox==Id?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);R(24,Y,352,50,FMath::Lerp(Active?PaveHUDMuted:PaveHUDPaper,FLinearColor(1,.86f,.72f),Hover*.65));if(Active)R(24,Y+5,3,44,PaveHUDOrange);Sample(I,34,Y+13);T(PaveHUDG::PaintName(EFoundrylinePavementMark(I)).ToUpper(),110,Y+9,1.2f,PaveHUDInk);T(Details[I],110,Y+33,.93f,PaveHUDGrey);AddHitBox({24*S,Y*S},{352*S,50*S},Id,true);Y+=53;
  }
  Y+=4;Button(TEXT("BayPlace"),TEXT("LOADING BAY PRESET"),TEXT("Door, apron, bollards and paint together"),Y,PC->BuildTool==TEXT("LoadingBay"));Y+=68;Button(TEXT("SelectArea"),TEXT("EDIT EXISTING PAINT"),TEXT("Click a painted line to move or remove"),Y,false);
 }else{
  Button(TEXT("SelectArea"),TEXT("SELECT & ADJUST"),TEXT("Click pavement or paint on the map"),Y,PC->BuildTool==TEXT("PaveEdit"));Y+=72;
  Button(TEXT("Erase"),TEXT("ERASE ASPHALT"),TEXT("Drag to trim an edge or make an island"),Y,PC->BuildTool==TEXT("PaveErase"));Y+=94;
  T(TEXT("1  Click an area to show its handles."),40,Y,1.13f,PaveHUDGrey);Y+=32;
  T(TEXT("2  Drag a corner, edge or road handle."),40,Y,1.13f,PaveHUDGrey);Y+=32;
  T(TEXT("3  Click paint; drag it or press Delete."),40,Y,1.13f,PaveHUDGrey);Y+=48;
  T(TEXT("Z undoes changes in solo play."),40,Y,1.08f,PaveHUDGrey);
 }

}
void AFoundrylineBuildHUD::DrawPavementTools(AFoundrylinePlayerController* PC){
 PC->PavePanelBounds.Init();
 if(!PC->bPropertyViewAccess||PC->IsWalking()||PC->IsLogisticsOverlayVisible())return;AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){Site=*It;break;}if(!Site)return;
 const auto* O=Site->GetSelectedObject();const bool Selected=O&&!O->Pavement.Polygons.IsEmpty();const bool Tool=PC->IsPavementTool();const auto* Def=O?GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->FindDefinition(O->DefinitionId):nullptr;const bool Legacy=O&&Def&&FFoundrylineDrivingLayout::IsPaved(Def->FunctionId)&&!Selected;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f);int VW,VH;PC->GetViewportSize(VW,VH);if(VW<=0||VH<=0)return;
 const auto* Bay=Site->GetSelectedOpening();if(PC->BuildTool==TEXT("LoadingBay")||(Bay&&Bay->LoadingTier)){
  const int Tier=Bay&&Bay->LoadingTier?Bay->LoadingTier:1;const auto Spec=FFoundrylineLoadingBayLayout::Spec(PC->BuildTool==TEXT("UpgradeBay")?FMath::Min(3,Tier+1):Tier);const float X=Canvas->ClipX-360*S,Y=285*S,W=336*S;float Row=Y+20*S;
  DrawRect(PaveHUDPaper,X,Y,W,338*S);DrawRect(PaveHUDOrange,X,Y,3*S,338*S);auto Label=[&](FString Value,float Size=1.f){DrawText(Value,PaveHUDInk,X+18*S,Row,GEngine->GetMediumFont(),Size*S);Row+=29*S;};auto Button=[&](const TCHAR* Key,const TCHAR* Value){const FName Id(*(FString(TEXT("Pave"))+Key));float& H=HoverAmounts.FindOrAdd(Id);H=FMath::FInterpTo(H,HoveredBox==Id?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);DrawRect(FMath::Lerp(PaveHUDMuted,PaveHUDOrange,H*.7f),X+16*S,Row,W-32*S,39*S);DrawText(Value,PaveHUDInk,X+26*S,Row+12*S,GEngine->GetMediumFont(),S);AddHitBox({X+16*S,Row},{W-32*S,39*S},Id,true);Row+=47*S;};
  Label(Spec.Name,1.25f);Label(FFoundrylineUnits::Length(Spec.Width)+TEXT(" x ")+FFoundrylineUnits::Length(Spec.Depth)+TEXT(" apron"));Label(TEXT("Door + apron + guards + markings"));Label(TEXT("Receiving and shipping / one bay"));
  if(PC->BuildTool==TEXT("LoadingBay")){Label(TEXT("Point at an exterior shop wall."));Label(TEXT("The whole preset snaps together."));Label(TEXT("Click to place. Right-click cancels."));}
  else if(PC->BuildTool==TEXT("UpgradeBay")){Label(TEXT("Review the larger hologram first."));Button(TEXT("BayConfirm"),TEXT("CONFIRM UPGRADE"));Button(TEXT("BayCancel"),TEXT("CANCEL UPGRADE"));}
  else {Label(TEXT("Move / delete the complete bay."));if(Tier<3)Button(TEXT("BayUpgrade"),Tier==1?TEXT("PREVIEW BOX TRUCK UPGRADE"):TEXT("PREVIEW SEMI UPGRADE"));else Label(TEXT("Largest bay / semi capable"));}
  PC->PavePanelBounds=FBox2D({X,Y},{X+W,Y+338*S});return;
 }
 auto Project=[&](FVector P,FVector2D& Out){if(!PC->ProjectWorldLocationToScreen(P,Out,false))return false;Out.X*=Canvas->ClipX/VW;Out.Y*=Canvas->ClipY/VH;return true;};
 auto Segment=[&](FVector A,FVector B,FLinearColor Color,float Width){FVector2D P,Q;if(Project(A,P)&&Project(B,Q))DrawLine(P.X,P.Y,Q.X,Q.Y,Color,Width*S);};
 auto Outline=[&](const FFoundrylinePlacedObject& Object,bool Hover){const FTransform T(FRotator(0,Object.QuarterTurns*90,0),FVector(Object.Anchor));const auto Color=Hover?FLinearColor(.2f,.7f,1):PaveHUDOrange;for(const auto& P:Object.Pavement.Polygons){auto Loop=[&](const auto& Points){for(int I=0;I<Points.Num();++I){const auto W=T.TransformPosition(FVector(Points[I],23));Segment(W,T.TransformPosition(FVector(Points[(I+1)%Points.Num()],23)),Color,Hover?2:3);}};Loop(P.Outer);for(const auto& H:P.Holes)Loop(H.Points);}
  if(!Hover){for(const auto& Handle:PaveHUDG::EditHandles(Object.Pavement)){FVector2D Screen;if(Project(T.TransformPosition(FVector(Handle.Position,20)),Screen)){
    const float Size=Handle.Corner?7:6;DrawRect(PaveHUDInk,Screen.X-(Size+2)*S,Screen.Y-(Size+2)*S,(Size+2)*2*S,(Size+2)*2*S);DrawRect((Handle.Tool==TEXT("PaveBend")||(Object.Pavement.Spline&&!Handle.Corner))?FLinearColor(.1f,.65f,1):PaveHUDPaper,Screen.X-Size*S,Screen.Y-Size*S,Size*2*S,Size*2*S);
   }}
   if(Site->SelectedPavementMark.IsValid())for(const auto& M:Object.Pavement.Markings)if(M.Id==Site->SelectedPavementMark)for(const auto& L:PaveHUDG::PaintLines(M))Segment(T.TransformPosition(FVector(L.Key,24)),T.TransformPosition(FVector(L.Value,24)),FLinearColor(.1f,.75f,1),5);
  }
 };
 if(Site->bPavementPreviewActive&&!Site->LastPavementPreview.Pavement.RoadControls.IsEmpty()){const auto& Draft=Site->LastPavementPreview;const FTransform T(FRotator(0,Draft.QuarterTurns*90,0),FVector(Draft.Anchor));for(const auto Point:Draft.Pavement.RoadControls){FVector2D Screen;if(Project(T.TransformPosition(FVector(Point,24)),Screen))DrawRect(FLinearColor(.12f,.7f,1),Screen.X-5*S,Screen.Y-5*S,10*S,10*S);}}
 if(Selected&&(!Tool||PC->BuildTool==TEXT("PaveEdit")||PC->bPaveDirectDrag))Outline(*O,false);FIntVector Cursor;if(!PC->IsOverBuildHUD()&&PC->GetCursorAnchor(Cursor)){FIntVector DragA,DragB;if((!Tool||PC->BuildTool==TEXT("PaveEdit"))&&!PC->IsConstructionDragging(DragA,DragB))if(const auto* Hover=Site->ObjectAt(Cursor);Hover&&!Hover->Pavement.Polygons.IsEmpty()&&(!O||O->ObjectId!=Hover->ObjectId))Outline(*Hover,true);
  if(PC->BuildTool==TEXT("PavePolygon")&&!PC->PavePoints.IsEmpty()){for(int I=1;I<PC->PavePoints.Num();++I)Segment(FVector(PC->PavePoints[I-1],25),FVector(PC->PavePoints[I],25),FLinearColor(.1f,.65f,1),3);Segment(FVector(PC->PavePoints.Last(),25),FVector(Cursor.X,Cursor.Y,25),FLinearColor(.1f,.65f,1),3);}
 }
 if(Site->bPavementPreviewActive&&Site->LastPavementPreview.Pavement.Lanes)for(int I=0;I+1<Site->PreviewJoinLines.Num();I+=2){Segment(Site->PreviewJoinLines[I],Site->PreviewJoinLines[I+1],FLinearColor(.15f,.85f,.6f),4);FVector2D P;if(Project((Site->PreviewJoinLines[I]+Site->PreviewJoinLines[I+1])*.5,P)){DrawRect(PaveHUDInk,P.X+10*S,P.Y-25*S,108*S,25*S);DrawText(TEXT("CONNECTED"),PaveHUDPaper,P.X+17*S,P.Y-19*S,GEngine->GetMediumFont(),.85*S);}}
 if(PC->PaveVehicle>=0)for(int I=0;I+1<PC->PaveSweepLines.Num();I+=2)Segment(PC->PaveSweepLines[I],PC->PaveSweepLines[I+1],PC->PaveSweepClear?FLinearColor(.15f,.65f,.8f,.8f):FLinearColor(1,.13f,.035f,.9f),1.5);
 FIntVector HandleAt;FName HandleTool;FString HandleHint;
 if(!PC->IsOverBuildHUD()&&PC->FindPavementHandle(HandleAt,HandleTool,HandleHint)){FVector2D Screen;if(Project(FVector(HandleAt)+FVector(0,0,20),Screen)){const float Width=180*S;DrawRect(PaveHUDInk,Screen.X+16*S,Screen.Y-28*S,Width,30*S);DrawText(HandleHint,PaveHUDPaper,Screen.X+24*S,Screen.Y-20*S,GEngine->GetMediumFont(),S);}}
 // Draft dimensions and rejection reasons stay next to the task, independent of the inspector.
 if(Site->bPavementPreviewActive){const auto& Draft=Site->LastPavementPreview;const auto Bounds=PaveHUDG::Bounds(Draft.Pavement.Polygons);FString Label;
  if(Draft.Pavement.Lanes){double Length=0;const auto Path=PaveHUDG::RoadPath(Draft.Pavement.RoadControls,Draft.Pavement.Spline);for(int I=1;I<Path.Num();++I)Length+=FVector2D::Distance(Path[I-1],Path[I]);Label=FFoundrylineUnits::Length(Length)+TEXT(" long / ")+FFoundrylineUnits::Length(Draft.Pavement.RoadWidthCm)+TEXT(" clear");}
  else Label=FFoundrylineUnits::Length(Bounds.GetSize().X)+TEXT(" x ")+FFoundrylineUnits::Length(Bounds.GetSize().Y)+FString::Printf(TEXT(" / %.0f sq ft"),PaveHUDG::Area(Draft.Pavement.Polygons)/929.0304);
  if(PC->BuildTool==TEXT("PavePaint")&&Draft.Pavement.Markings.Num()==1){const auto& M=Draft.Pavement.Markings[0];const double Length=FVector2D::Distance(M.Points[0],M.Points.Last());Label=PaveHUDG::PaintName(M.Kind)+TEXT(" / ");if(M.Kind==EFoundrylinePavementMark::Parking)Label+=FString::Printf(TEXT("%d spaces / "),FMath::Clamp(FMath::FloorToInt((Length+.01)/274.32),1,64))+FFoundrylineUnits::Length(M.WidthCm)+TEXT(" deep");else if(M.Kind==EFoundrylinePavementMark::Loading||M.Kind==EFoundrylinePavementMark::KeepClear)Label+=FFoundrylineUnits::Length(Length)+TEXT(" x ")+FFoundrylineUnits::Length(M.WidthCm);else Label+=FFoundrylineUnits::Length(Length);}
  const float CX=FMath::Max(430*S,(Canvas->ClipX-440*S)*.5f),CY=Canvas->ClipY-186*S;DrawRect(PaveHUDInk,CX,CY,440*S,76*S);DrawText(Label,PaveHUDPaper,CX+14*S,CY+12*S,GEngine->GetMediumFont(),1.1*S);const FString Reason=Site->IsConstructionFeedbackError()?Site->GetConstructionFeedbackText():PC->bPaveDirectDrag?TEXT("Release to apply. Right-click cancels."):PC->BuildTool==TEXT("PavePaint")?TEXT("Release to paint. Right-click cancels."):PC->PaveAwaitBend?TEXT("Shape the bend, then click to finish."):PC->BuildTool==TEXT("PaveRoad")&&PC->PavePoints.Num()==1?TEXT("Click to set the end. Right-click cancels."):PC->BuildTool==TEXT("PavePolygon")?TEXT("Click corners. Enter finishes. Backspace removes one."):PC->BuildTool==TEXT("PaveSpline")?TEXT("Click next point. Enter accepts this preview."):TEXT("Release to place. Right-click cancels.");DrawText(Reason.Left(55),!Site->IsConstructionFeedbackError()?FLinearColor(.4f,.85f,.95f):FLinearColor(1,.5f,.3f),CX+14*S,CY+43*S,GEngine->GetMediumFont(),.95*S);
 }
 if(!Selected&&!Tool&&!Legacy)return;
 const float X=Canvas->ClipX/S-368,Y=280,W=344;float Row=Y+18;
 struct Item{FString Id,Label;float X,Y,W,H,Size;bool Active,PaveHUDMuted;};TArray<Item> Items;
 auto Text=[&](FString Label,bool bMuted=false,float Size=1.02f){Items.Add({{},Label,X+16,Row,W-32,0,Size,false,bMuted});Row+=25;};
 auto Button=[&](FString Id,FString Label,float Offset=16,float Width=312,bool Active=false){Items.Add({Id,Label,X+Offset,Row,Width,36,1.f,Active,false});};
 auto Full=[&](FString Id,FString Label,bool Active=false){Button(Id,Label,16,W-32,Active);Row+=42;};
 auto WidthRow=[&](bool Existing){Button(Existing?TEXT("SelectedWidthLess"):TEXT("WidthLess"),TEXT("- 1 FT"),16,82);Items.Add({{},FString::Printf(TEXT("%.0f ft clear"),(Existing?O->Pavement.RoadWidthCm:PC->PaveWidthCm)/30.48),X+118,Row+12,130,0,1.1f,false,false});Button(Existing?TEXT("SelectedWidthMore"):TEXT("WidthMore"),TEXT("+ 1 FT"),246,82);Row+=44;};
 auto RadiusRow=[&](bool Existing){Text(TEXT("ENTRANCE RADIUS"),true);Button(Existing?TEXT("SelectedRadiusLess"):TEXT("RadiusLess"),TEXT("- 5 FT"),16,82);Items.Add({{},FString::Printf(TEXT("%.0f ft"),(Existing?O->Pavement.EntranceRadiusCm:Site->DraftEntranceRadiusCm)/30.48),X+132,Row+12,110,0,1.1f,false,false});Button(Existing?TEXT("SelectedRadiusMore"):TEXT("RadiusMore"),TEXT("+ 5 FT"),246,82);Row+=44;};
 const bool Editing=Selected&&(!Tool||PC->BuildTool==TEXT("PaveEdit")||PC->bPaveDirectDrag),RoadTool=PC->BuildTool==TEXT("PaveRoad")||PC->BuildTool==TEXT("PaveSpline"),Paint=PC->BuildTool==TEXT("PavePaint");
 Text(Editing?(Site->SelectedPavementMark.IsValid()?TEXT("EDIT PAINT"):TEXT("EDIT PAVEMENT")):RoadTool?TEXT("DRIVEWAY / ROAD"):Paint?TEXT("PAINT"):Legacy?TEXT("EDIT EXISTING PAVEMENT"):TEXT("PAVEMENT"),false,1.25f);Row+=8;
 if(Legacy&&!Tool){Text(TEXT("Enable shape and paint editing."),true);Full(TEXT("Convert"),TEXT("CUSTOMIZE THIS AREA"));}
 else if(Editing){
  const bool Mark=Site->SelectedPavementMark.IsValid();Text(Mark?TEXT("Drag selected paint to move it."):TEXT("Drag handles directly on the map."),true);
  if(!Mark)Text(FString::Printf(TEXT("%.0f sq ft / %d marking groups"),PaveHUDG::Area(O->Pavement.Polygons)/929.0304,O->Pavement.Markings.Num()),true);
  Button(TEXT("EditMove"),TEXT("MOVE"),16,151);Button(TEXT("DeleteSelection"),Mark?TEXT("DELETE PAINT"):TEXT("DELETE AREA"),177,151);Row+=44;
  if(!Mark){Full(TEXT("SelectedCurbs"),O->Pavement.Curbs?TEXT("CURBS ON"):TEXT("CURBS OFF"),O->Pavement.Curbs);if(O->Pavement.Lanes){if(O->Pavement.Spline){Button(TEXT("InsertPoint"),TEXT("ADD ROAD POINT"),16,151);Button(TEXT("RemovePoint"),TEXT("REMOVE POINT"),177,151);Row+=44;}WidthRow(true);RadiusRow(true);}Full(TEXT("Advanced"),PC->bPaveAdvanced?TEXT("LESS OPTIONS"):TEXT("MORE OPTIONS"));
   if(PC->bPaveAdvanced){if(!O->Pavement.Spline)Full(O->Pavement.Lanes?TEXT("Bend"):TEXT("Round"),O->Pavement.Lanes?TEXT("BEND ROAD"):TEXT("ROUND A CORNER"));Full(TEXT("EditResize"),TEXT("RESHAPE"));if(O->Pavement.Lanes)Full(TEXT("StripeRoad"),TEXT("REAPPLY ROAD PAINT"));Full(TEXT("MarkSelect"),TEXT("SELECT PAINT"));}
   int Invalid=0;for(const auto& M:O->Pavement.Markings)if(!PaveHUDG::MarkingFits(O->Pavement,M))++Invalid;if(Invalid)Text(FString::Printf(TEXT("%d markings need repositioning"),Invalid));
  }else {const auto* M=O->Pavement.Markings.FindByPredicate([&](const auto& V){return V.Id==Site->SelectedPavementMark;});if(M){Text(PaveHUDG::PaintName(M->Kind));Full(TEXT("RotatePaint"),TEXT("ROTATE 90 DEGREES"));if(M->Kind==EFoundrylinePavementMark::Parking||M->Kind==EFoundrylinePavementMark::Arrow)Full(TEXT("FlipPaint"),M->Kind==EFoundrylinePavementMark::Parking?TEXT("FLIP PARKING SIDE"):TEXT("REVERSE ARROW"));}Text(TEXT("Delete removes paint only."),true);}
 }else if(RoadTool){
  Button(TEXT("StandardPreset"),TEXT("STANDARD 24 FT"),16,151,!PC->PaveTruck&&PC->PaveLanes==2&&FMath::Abs(PC->PaveWidthCm-731.52)<1);Button(TEXT("TruckPreset"),TEXT("TRUCK 30 FT"),177,151,PC->PaveTruck&&PC->PaveLanes==2&&FMath::Abs(PC->PaveWidthCm-914.4)<1);Row+=44;
  if(PC->BuildTool==TEXT("PaveSpline")){Full(TEXT("FinishSpline"),TEXT("FINISH ROAD  [ENTER]"),PC->PavePoints.Num()>=2);if(!PC->PavePoints.IsEmpty())Full(TEXT("UndoSplinePoint"),TEXT("UNDO POINT  [BACKSPACE]"));}
  else Full(TEXT("Spline"),TEXT("SWITCH TO SPLINE DRAWING"));
  Text(FString::Printf(TEXT("%.0f ft clear / %s"),PC->PaveWidthCm/30.48,PC->PaveLanes==1?TEXT("one way"):PC->PaveLanes==4?TEXT("four lanes"):TEXT("two way")),true);RadiusRow(false);
  if(PC->BuildTool==TEXT("PaveSpline"))Text(FString::Printf(TEXT("%d points / click next / Enter finishes"),PC->PavePoints.Num()),true);else Text(PC->PaveAwaitBend?TEXT("3  Shape the bend. Click to finish."):PC->PavePoints.Num()==1?TEXT("2  Click the road end."):TEXT("1  Click start, or hold and drag."),true);
  if(PC->PaveCurved&&!PC->PaveAwaitBend)Text(TEXT("Then move the bend and click."),true);
  Button(TEXT("TurningCheck"),PC->PaveVehicle<0?TEXT("TURN CHECK OFF"):TEXT("TURN CHECK ON"),16,151,PC->PaveVehicle>=0);Button(TEXT("DoneRoad"),TEXT("DONE / SELECT"),177,151);Row+=44;Full(TEXT("Advanced"),PC->bPaveAdvanced?TEXT("LESS OPTIONS"):TEXT("MORE OPTIONS"));
  if(PC->bPaveAdvanced){Button(TEXT("OneWay"),TEXT("ONE WAY"),16,98,PC->PaveLanes==1);Button(TEXT("TwoWay"),TEXT("TWO WAY"),123,98,PC->PaveLanes==2);Button(TEXT("FourLane"),TEXT("4 LANES"),230,98,PC->PaveLanes==4);Row+=44;WidthRow(false);Full(TEXT("Curbs"),PC->PaveCurbs?TEXT("CURBS ON"):TEXT("CURBS OFF"),PC->PaveCurbs);}

 }else if(Paint){
  Text(PaveHUDG::PaintName(PC->PaveMark).ToUpper(),false,1.3f);Row+=6;
  if(PC->PaveMark==EFoundrylinePavementMark::Loading||PC->PaveMark==EFoundrylinePavementMark::KeepClear){Text(TEXT("Drag from one corner to the opposite."),true);Text(TEXT("The rectangle follows your cursor."),true);Text(TEXT("No size buttons needed."),true);}
  else {Text(PC->PaveMark==EFoundrylinePavementMark::Parking?TEXT("Drag along the front of the row."):TEXT("Hold and drag to draw the marking."),true);if(PC->PaveMark==EFoundrylinePavementMark::Parking){Text(TEXT("9 ft wide x 18 ft deep per space."),true);Text(TEXT("Preview shows the exact space count."),true);}Full(TEXT("PaintSnap"),PC->PavePaintAngleSnap?TEXT("ANGLE SNAP: 45 DEGREES"):TEXT("ANGLE SNAP: FREE"),PC->PavePaintAngleSnap);if(PC->PaveMark==EFoundrylinePavementMark::Parking||PC->PaveMark==EFoundrylinePavementMark::Arrow)Full(TEXT("Flip"),PC->PaveMark==EFoundrylinePavementMark::Parking?TEXT("FLIP PARKING SIDE  [R]"):TEXT("REVERSE ARROW  [R]"),PC->PaveFlipMark);}
  Row+=8;Text(TEXT("Release to paint. Right-click cancels."),true);Text(TEXT("The entire marking must fit on asphalt."),true);Full(TEXT("PaintDone"),TEXT("SELECT / EDIT PAINT"));
 }else{
  Text(PC->BuildTool==TEXT("PaveEdit")?TEXT("Click pavement or paint to edit it."):PC->BuildTool==TEXT("PavePolygon")?TEXT("Click corners. Enter finishes."):PC->BuildTool==TEXT("PaveErase")?TEXT("Drag across asphalt to remove it."):PC->BuildTool==TEXT("PaveRound")?TEXT("Drag a corner inward to round it."):PC->BuildTool==TEXT("PaveBend")?TEXT("Drag the blue bend handle."):PC->BuildTool==TEXT("PaveInsert")?TEXT("Click the road to add a point."):PC->BuildTool==TEXT("PaveRemovePoint")?TEXT("Click near a road point to remove it."):TEXT("Drag a rectangle. Release to pave."),true);
  if(PC->BuildTool!=TEXT("PaveEdit")&&PC->BuildTool!=TEXT("PaveErase"))Full(TEXT("Curbs"),PC->PaveCurbs?TEXT("CURBS ON"):TEXT("CURBS OFF"),PC->PaveCurbs);
  Text(TEXT("Right-click cancels."),true);
 }
 if((RoadTool||(Editing&&O->Pavement.Lanes))&&PC->bPaveAdvanced){Text(TEXT("VEHICLE TURNING CHECK"),true);const TCHAR* Names[]={TEXT("OFF"),TEXT("CAR"),TEXT("BOX"),TEXT("SEMI")};for(int I=0;I<4;++I)Button(FString::Printf(TEXT("Vehicle%d"),I),Names[I],16+I*80,72,PC->PaveVehicle==I-1);Row+=44;if(!PC->PaveSweepStatus.IsEmpty()){Text(PC->PaveSweepStatus.Left(42),true);if(PC->PaveSweepStatus.Len()>42)Text(PC->PaveSweepStatus.Mid(42),true);}}
 const float H=Row-Y+12;DrawRect(FLinearColor(0,0,0,.15f),(X+4)*S,(Y+4)*S,W*S,H*S);DrawRect(PaveHUDPaper,X*S,Y*S,W*S,H*S);DrawRect(PaveHUDOrange,X*S,Y*S,3*S,H*S);PC->PavePanelBounds=FBox2D(FVector2D(X*S,Y*S),FVector2D((X+W)*S,(Y+H)*S));
 for(const auto& I:Items){if(I.H){const FName Key(*(TEXT("Pave")+I.Id));float& Hover=HoverAmounts.FindOrAdd(Key);Hover=FMath::FInterpTo(Hover,HoveredBox==Key?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);DrawRect(I.Active?PaveHUDOrange:FMath::Lerp(PaveHUDMuted,FLinearColor(1,.86f,.72f),Hover),I.X*S,I.Y*S,I.W*S,I.H*S);DrawText(I.Label,PaveHUDInk,(I.X+10)*S,(I.Y+12)*S,GEngine->GetMediumFont(),I.Size*S);AddHitBox({I.X*S,I.Y*S},{I.W*S,I.H*S},Key,true);}else DrawText(I.Label,I.PaveHUDMuted?PaveHUDGrey:PaveHUDInk,I.X*S,I.Y*S,GEngine->GetMediumFont(),I.Size*S);}
}
