#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePavementGeometry.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Misc/Crc.h"
#include "Algo/Reverse.h"

namespace {using G=FFoundrylinePavementGeometry;
AFoundrylineBuildSite* Site(UWorld* W){for(TActorIterator<AFoundrylineBuildSite> It(W);It;++It)return *It;return nullptr;}
}
void AFoundrylinePlayerController::PavementAction(FName Action){
 if(IsWalking()||!bPropertyViewAccess||bConstructionPending)return;auto* S=Site(GetWorld());if(!S)return;const FString Key=Action.ToString();
 if(Action==TEXT("BayPlace")){ChooseTool(TEXT("LoadingBay"));return;}
 if(Action==TEXT("BayUpgrade")){ChooseTool(TEXT("UpgradeBay"));return;}
 if(Action==TEXT("BayConfirm")){if(S->CommitEdit(TEXT("UpgradeBay"),FIntVector::ZeroValue,FIntVector::ZeroValue))ChooseTool(TEXT("Select"));return;}
 if(Action==TEXT("BayCancel")){ChooseTool(TEXT("Select"));return;}
 if(Action==TEXT("PaintSnap")){PavePaintAngleSnap=!PavePaintAngleSnap;return;}
 if(Action==TEXT("RotatePaint")||Action==TEXT("FlipPaint")){S->PavementOption(Action);return;}
 if(Action==TEXT("InsertPoint")||Action==TEXT("RemovePoint")){ChooseTool(Action==TEXT("InsertPoint")?TEXT("PaveInsert"):TEXT("PaveRemovePoint"));return;}
 if(Action==TEXT("UndoSplinePoint")){if(BuildTool==TEXT("PaveSpline")&&!PavePoints.IsEmpty()){PavePoints.Pop();S->HidePreview();}return;}
 if(Action==TEXT("FinishSpline")){if(BuildTool!=TEXT("PaveSpline"))return;auto Points=PavePoints;FIntVector Cursor;if(!IsOverBuildHUD()&&!Points.IsEmpty()&&Points.Num()<32&&GetCursorAnchor(Cursor)&&FVector2D::Distance(Points.Last(),FVector2D(Cursor.X,Cursor.Y))>=100)Points.Add({double(Cursor.X),double(Cursor.Y)});if(Points.Num()<2){S->Status=TEXT("Click a start point and an end point first");return;}if(S->PavementShape(TEXT("SplineRoad"),Points,PaveWidthCm,PaveLanes,PaveTruck,PaveCurbs))PavePoints.Reset();return;}
 if(Action==TEXT("StandardPreset")||Action==TEXT("TruckPreset")){PaveTruck=Action==TEXT("TruckPreset");PaveLanes=2;PaveWidthCm=G::MinimumWidth(2,PaveTruck);PaveCurbs=true;S->DraftEntranceRadiusCm=PaveTruck?914.4f:304.8f;PaveVehicle=-1;PaveSweepKey.Reset();return;}
 if(Action==TEXT("TurningCheck")){PaveVehicle=PaveVehicle<0?(PaveTruck?2:0):-1;PaveSweepKey.Reset();if(PaveVehicle<0){PaveSweepLines.Reset();PaveSweepStatus.Reset();}return;}
 if(Action==TEXT("RadiusMore")||Action==TEXT("RadiusLess")){S->DraftEntranceRadiusCm=FMath::Clamp(S->DraftEntranceRadiusCm+(Action==TEXT("RadiusMore")?152.4f:-152.4f),0.f,1524.f);return;}
 if(Action==TEXT("SelectedRadiusMore")||Action==TEXT("SelectedRadiusLess")){S->PavementOption(Action==TEXT("SelectedRadiusMore")?TEXT("RadiusMore"):TEXT("RadiusLess"));return;}
 if(Action==TEXT("Advanced")){bPaveAdvanced=!bPaveAdvanced;return;}
 if(Action==TEXT("EditMenu")||Action==TEXT("SelectArea")||Action==TEXT("DoneRoad")||Action==TEXT("PaintDone")){bPaveAdvanced=false;PlannerKind=PlannerId=0;PavePalettePage=2;ChooseTool(TEXT("PaveEdit"));return;}
 if(Action==TEXT("DeleteSelection")){S->CommitEdit(TEXT("Demolish"),FIntVector::ZeroValue,FIntVector::ZeroValue);return;}
 if(Action==TEXT("Walkway")){ChooseTool(TEXT("Walkway"));return;}
 if(Action==TEXT("Convert")){S->PavementOption(Action);return;}
 if(Action==TEXT("EditMove")||Action==TEXT("EditResize")){ChooseTool(Action==TEXT("EditMove")?TEXT("Move"):TEXT("Resize"));return;}
 if(Action==TEXT("PaintMenu")){bPaveAdvanced=false;PlannerKind=PlannerId=0;PavePalettePage=1;ChooseTool(NAME_None);return;}if(Action==TEXT("AreaMenu")){bPaveAdvanced=false;PlannerKind=PlannerId=0;PavePalettePage=0;ChooseTool(TEXT("PaveRect"));return;}
 if(Action==TEXT("Rect")||Action==TEXT("Polygon")||Action==TEXT("Road")||Action==TEXT("Spline")||Action==TEXT("Erase")||Action==TEXT("MarkSelect")){PavePoints.Reset();PaveAwaitBend=false;PlannerKind=PlannerId=0;ChooseTool(FName(*(TEXT("Pave")+Key)));return;}
 if(Action==TEXT("Round")||Action==TEXT("Bend")){ChooseTool(Action==TEXT("Round")?TEXT("PaveRound"):TEXT("PaveBend"));return;}
 if(Action==TEXT("Curbs")){PaveCurbs=!PaveCurbs;return;}
 if(Action==TEXT("SelectedCurbs")){S->PavementOption(TEXT("Curbs"));return;}
 if(Action==TEXT("SelectedWidthMore")||Action==TEXT("SelectedWidthLess")){S->PavementOption(Action==TEXT("SelectedWidthMore")?TEXT("WidthMore"):TEXT("WidthLess"));return;}
 if(Action==TEXT("StripeRoad")){S->PavementOption(Action);return;}
 if(Action==TEXT("ClearMark")){S->PavementOption(Action);return;}
 if(Action==TEXT("Curve")){PaveCurved=!PaveCurved;PaveAwaitBend=false;PavePoints.Reset();bDraggingBuild=false;S->HidePreview();return;}
 if(Action==TEXT("OneWay")||Action==TEXT("TwoWay")||Action==TEXT("FourLane")){PaveLanes=Action==TEXT("OneWay")?1:Action==TEXT("TwoWay")?2:4;PaveWidthCm=G::MinimumWidth(PaveLanes,PaveTruck);return;}
 if(Action==TEXT("Truck")){PaveTruck=!PaveTruck;PaveWidthCm=G::MinimumWidth(PaveLanes,PaveTruck);return;}
 if(Action==TEXT("WidthMore")||Action==TEXT("WidthLess")){PaveWidthCm=FMath::Clamp(PaveWidthCm+(Action==TEXT("WidthMore")?30.48f:-30.48f),float(G::MinimumWidth(PaveLanes,PaveTruck)),2400.f);return;}
 if(Action==TEXT("DepthMore")||Action==TEXT("DepthLess")){PaveMarkDepthCm=FMath::Clamp(PaveMarkDepthCm+(Action==TEXT("DepthMore")?30.48f:-30.48f),243.84f,2400.f);return;}
 if(Action==TEXT("Flip")){PaveFlipMark=!PaveFlipMark;return;}
 if(Key.StartsWith(TEXT("Vehicle"))){PaveVehicle=FCString::Atoi(*Key.RightChop(7))-1;PaveSweepKey.Reset();return;}
 if(Key.StartsWith(TEXT("Mark"))){const int Type=FCString::Atoi(*Key.RightChop(4));if(Type<0||Type>7||Type==5)return;PaveMark=EFoundrylinePavementMark(Type);PavePalettePage=1;PaveFlipMark=false;PaveMarkDepthCm=Type==0?548.64f:Type==5?365.76f:Type==6?304.8f:10.16f;PavePoints.Reset();PaveAwaitBend=false;PlannerKind=PlannerId=0;ChooseTool(TEXT("PavePaint"));return;}
}
bool AFoundrylinePlayerController::HandlePavementInput(const FInputKeyEventArgs& P){
 if(!IsLocalController()||IsWalking()||!bPropertyViewAccess||OperationsScreen||bFactoryMenu||bLogisticsOverlay)return false;auto* S=Site(GetWorld());if(!S)return false;
 if(bPaveDirectDrag){
  if(P.Event==IE_Pressed&&(P.Key==EKeys::Escape||P.Key==EKeys::RightMouseButton)){bPaveDirectDrag=false;bDraggingBuild=false;S->HidePreview();return true;}
  if(P.Key==EKeys::LeftMouseButton&&P.Event==IE_Released){
   // Capture the endpoint while the direct-drag pavement plane and snap are still active.
   FIntVector At=DragEnd,Cursor;if(GetCursorAnchor(Cursor)&&Cursor!=DragStart)At=Cursor;bPaveDirectDrag=false;bDraggingBuild=false;
   if(!bConstructionPending&&!S->IsPropertyBusy()&&S->SelectedObject==PaveDirectObject&&!IsOverBuildHUD())S->CommitEdit(PaveDirectTool,DragStart,At);S->HidePreview();return true;
  }return P.Key==EKeys::LeftMouseButton;
 }
 const bool Direct=BuildTool.IsNone()||BuildTool==TEXT("Select")||BuildTool==TEXT("PaveEdit");
 if(Direct&&P.Key==EKeys::LeftMouseButton&&P.Event==IE_Pressed&&!IsOverBuildHUD()&&!bConstructionPending&&!S->IsPropertyBusy()){
  FIntVector HandleAt;FName HandleTool;FString Hint;
  if(FindPavementHandle(HandleAt,HandleTool,Hint)){
   S->SelectedPavementMark.Invalidate();PaveDirectTool=HandleTool;PaveDirectObject=S->SelectedObject;DragStart=DragEnd=HandleAt;bPaveDirectDrag=true;bDraggingBuild=true;ActiveSnap=EFoundrylineSnap::Fine25cm;return true;
  }
  FIntVector At;if(BuildTool==TEXT("PaveEdit")&&GetCursorAnchor(At))if(const auto* Hit=S->ObjectAt(At);Hit&&!Hit->Pavement.Polygons.IsEmpty()){
   const auto Previous=S->SelectedPavementMark;const auto PreviousObject=S->SelectedObject;S->BeginEdit(TEXT("Select"),At);const bool Paint=S->SelectPavementMark(At);
   if(Paint&&Previous==S->SelectedPavementMark&&PreviousObject==S->SelectedObject){PaveDirectTool=TEXT("Move");PaveDirectObject=S->SelectedObject;DragStart=DragEnd=At;bPaveDirectDrag=true;bDraggingBuild=true;ActiveSnap=EFoundrylineSnap::Fine25cm;}
   else S->Status=Paint?TEXT("Paint selected. Drag it to move / Delete removes paint only."):TEXT("Pavement selected. Drag a handle / click paint to edit it.");
   return true;
  }
 }
 if(!IsPavementTool()||bConstructionPending)return false;
 if(P.Event==IE_Pressed&&(P.Key==EKeys::Escape||P.Key==EKeys::RightMouseButton)){const bool Draft=bDraggingBuild||PaveAwaitBend||!PavePoints.IsEmpty();bDraggingBuild=false;PaveAwaitBend=false;PavePoints.Reset();PaveSweepLines.Reset();S->HidePreview();if(!Draft)ChooseTool(NAME_None);return true;}
 if(BuildTool==TEXT("PaveSpline")&&P.Event==IE_Pressed&&(P.Key==EKeys::Enter||P.Key==EKeys::BackSpace)){if(P.Key==EKeys::Enter)PavementAction(TEXT("FinishSpline"));else if(!PavePoints.IsEmpty()){PavePoints.Pop();S->HidePreview();}return true;}
 if(BuildTool==TEXT("PaveEdit"))return P.Key==EKeys::LeftMouseButton&&!IsOverBuildHUD();
 if(P.Event==IE_Pressed&&P.Key==EKeys::R){if(BuildTool==TEXT("PaveRoad"))PavementAction(TEXT("Curve"));else if(BuildTool==TEXT("PavePaint"))PavementAction(TEXT("Flip"));return true;}
 if(BuildTool==TEXT("PavePolygon")&&P.Event==IE_Pressed&&P.Key==EKeys::BackSpace){if(!PavePoints.IsEmpty())PavePoints.Pop();S->HidePreview();return true;}
 if(BuildTool==TEXT("PavePolygon")&&P.Event==IE_Pressed&&P.Key==EKeys::Enter){if(PavePoints.Num()>=3&&S->PavementShape(TEXT("Polygon"),PavePoints,0,0,false,PaveCurbs))PavePoints.Reset();return true;}
 if(AFoundrylineBuildSite::IsEditTool(BuildTool))return false;
 if(P.Key!=EKeys::LeftMouseButton)return false;if(IsOverBuildHUD())return false;if(bConstructionPending||S->IsPropertyBusy())return true;FIntVector Cursor;if(!GetCursorAnchor(Cursor))return true;const FVector2D At(Cursor.X,Cursor.Y);
 if(BuildTool==TEXT("PaveInsert")||BuildTool==TEXT("PaveRemovePoint")){if(P.Event==IE_Pressed)S->CommitEdit(BuildTool,Cursor,Cursor);return true;}
 if(BuildTool==TEXT("PaveSpline")){if(P.Event==IE_Pressed){if(PavePoints.IsEmpty()||FVector2D::Distance(PavePoints.Last(),At)>=100){if(PavePoints.Num()<32)PavePoints.Add(At);S->Status=TEXT("Click the next road point. Enter finishes; Backspace removes a point.");}else S->Status=TEXT("Place the next point farther away, or press Enter to finish");}return true;}
 if(BuildTool==TEXT("PaveMarkSelect")){if(P.Event==IE_Pressed)S->SelectPavementMark(Cursor);return true;}
 if(BuildTool==TEXT("PavePolygon")){if(P.Event==IE_Pressed){if(PavePoints.Num()>=3&&FVector2D::Distance(At,PavePoints[0])<=100){if(S->PavementShape(TEXT("Polygon"),PavePoints,0,0,false,PaveCurbs))PavePoints.Reset();}else if(PavePoints.Num()<64)PavePoints.Add(At);}return true;}
 if(BuildTool==TEXT("PaveRoad")&&PaveAwaitBend){if(P.Event==IE_Pressed){const TArray<FVector2D> C={FVector2D(DragStart.X,DragStart.Y),At,FVector2D(DragEnd.X,DragEnd.Y)};if(S->PavementShape(TEXT("Road"),C,PaveWidthCm,PaveLanes,PaveTruck,PaveCurbs)){PaveAwaitBend=false;PavePoints.Reset();}}return true;}
 if(P.Event==IE_Pressed){if(BuildTool==TEXT("PaveRoad")&&PavePoints.Num()==1){DragStart=FIntVector(FVector(PavePoints[0],0));DragEnd=Cursor;PavePoints.Reset();}else DragStart=DragEnd=Cursor;bDraggingBuild=true;return true;}
 if(P.Event!=IE_Released||!bDraggingBuild)return true;bDraggingBuild=false;if(Cursor!=DragStart)DragEnd=Cursor;TArray<FVector2D> Points={{double(DragStart.X),double(DragStart.Y)},{double(DragEnd.X),double(DragEnd.Y)}};
 if(BuildTool==TEXT("PaveRoad")&&FVector2D::Distance(Points[0],Points[1])<100){PavePoints={Points[0]};S->HidePreview();S->Status=TEXT("Start set. Click the road end, or right-click to cancel.");return true;}
 if(BuildTool==TEXT("PaveRoad")&&PaveCurved){PaveAwaitBend=true;S->Status=TEXT("Move the curve handle. Click to finish the road.");return true;}
 if(BuildTool==TEXT("PavePaint")){const auto M=G::PaintDraft(PaveMark,Points[0],Points[1],PaveMarkDepthCm,PaveFlipMark,PavePaintAngleSnap);if(FVector2D::Distance(Points[0],Points[1])>=100)S->AddPavementMark(M.Kind,M.Points,M.WidthCm);else {S->HidePreview();S->Status=TEXT("Hold and drag to size the marking. Right-click cancels.");}}
 else {const bool Placed=S->PavementShape(BuildTool==TEXT("PaveRoad")?TEXT("Road"):BuildTool==TEXT("PaveErase")?TEXT("Erase"):TEXT("Rectangle"),Points,PaveWidthCm,BuildTool==TEXT("PaveRoad")?PaveLanes:0,PaveTruck,PaveCurbs);if(!Placed&&BuildTool==TEXT("PaveRoad"))PavePoints={Points[0]};}
 return true;
}
void AFoundrylinePlayerController::TickPavement(){
 auto* S=Site(GetWorld());if(!S)return;S->SelectionOutlines.Reset();
 auto Sweep=[&](){if(PaveVehicle<0)return;const auto* O=S->bPavementPreviewActive?&S->LastPavementPreview:S->GetSelectedObject();if(!O||!O->Pavement.Lanes){PaveSweepLines.Reset();return;}TArray<FVector2D> C;const FTransform T(FRotator(0,O->QuarterTurns*90,0),FVector(O->Anchor));for(const auto P:O->Pavement.RoadControls)C.Add(FVector2D(T.TransformPosition(FVector(P,0))));UpdatePavementSweep(C,O->Pavement.RoadWidthCm,O->Pavement.Curbs,O->Pavement.Lanes);};

 if(bPaveDirectDrag){FIntVector At;if(S->SelectedObject!=PaveDirectObject||S->IsPropertyBusy()){bPaveDirectDrag=false;bDraggingBuild=false;S->HidePreview();return;}if(!IsOverBuildHUD()&&GetCursorAnchor(At)){DragEnd=At;S->PreviewEdit(PaveDirectTool,DragStart,DragEnd);Sweep();}return;}
 if(BuildTool==TEXT("PaveEdit")){S->HidePreview();Sweep();return;}
 if(BuildTool==TEXT("PaveInsert")||BuildTool==TEXT("PaveRemovePoint")){FIntVector At;if(!IsOverBuildHUD()&&GetCursorAnchor(At))S->PreviewEdit(BuildTool,At,At);else S->HidePreview();return;}
 if(AFoundrylineBuildSite::IsEditTool(BuildTool))return;FIntVector Cursor;if(IsOverBuildHUD()||!GetCursorAnchor(Cursor)){S->HidePreview();return;}TArray<FVector2D> Points;
 if(BuildTool==TEXT("PaveSpline")){Points=PavePoints;if(Points.IsEmpty()){S->HidePreview();return;}const FVector2D At(Cursor.X,Cursor.Y);if(FVector2D::Distance(Points.Last(),At)>=100&&Points.Num()<32)Points.Add(At);if(Points.Num()>=2)S->PavementShape(TEXT("SplineRoad"),Points,PaveWidthCm,PaveLanes,PaveTruck,PaveCurbs,true);else S->HidePreview();Sweep();return;}
 if(BuildTool==TEXT("PavePolygon")){Points=PavePoints;Points.Add({double(Cursor.X),double(Cursor.Y)});if(Points.Num()>=3)S->PavementShape(TEXT("Polygon"),Points,0,0,false,PaveCurbs,true);return;}
 if(BuildTool==TEXT("PaveRoad")&&PaveAwaitBend)Points={FVector2D(DragStart.X,DragStart.Y),FVector2D(Cursor.X,Cursor.Y),FVector2D(DragEnd.X,DragEnd.Y)};
 else if(bDraggingBuild){DragEnd=Cursor;Points={FVector2D(DragStart.X,DragStart.Y),FVector2D(Cursor.X,Cursor.Y)};}else if(BuildTool==TEXT("PaveRoad")&&PavePoints.Num()==1)Points={PavePoints[0],FVector2D(Cursor.X,Cursor.Y)};else return;
 if(BuildTool==TEXT("PavePaint")){const auto M=G::PaintDraft(PaveMark,Points[0],Points[1],PaveMarkDepthCm,PaveFlipMark,PavePaintAngleSnap);S->AddPavementMark(M.Kind,M.Points,M.WidthCm,true);return;}
 if(BuildTool==TEXT("PaveRoad")&&Points.Num()==2&&FVector2D::Distance(Points[0],Points[1])<100){S->HidePreview();S->Status=TEXT("Move farther from the road start, then click to set the end.");return;}
 S->PavementShape(BuildTool==TEXT("PaveRoad")?TEXT("Road"):BuildTool==TEXT("PaveErase")?TEXT("Erase"):TEXT("Rectangle"),Points,PaveWidthCm,BuildTool==TEXT("PaveRoad")?PaveLanes:0,PaveTruck,PaveCurbs,true);
 if(BuildTool==TEXT("PaveRoad")&&S->bPavementPreviewActive){const auto& Draft=S->LastPavementPreview;TArray<FVector2D> Actual;const FTransform T(FRotator(0,Draft.QuarterTurns*90,0),FVector(Draft.Anchor));for(const auto V:Draft.Pavement.RoadControls)Actual.Add(FVector2D(T.TransformPosition(FVector(V,0))));UpdatePavementSweep(Actual,PaveWidthCm,PaveCurbs,PaveLanes);}
}
void AFoundrylinePlayerController::UpdatePavementSweep(const TArray<FVector2D>& Controls,float Width,bool Curbs,int Lanes){
 if(!Lanes)Lanes=PaveLanes;
 if(PaveVehicle<0){PaveSweepLines.Reset();PaveSweepStatus.Reset();return;}const auto* MapSource=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>();auto* Source=const_cast<UFoundrylineVehiclePathSubsystem*>(MapSource);const auto Existing=Source->GetMap();
 const FString Key=FString::Printf(TEXT("%u_%d_%.1f_%d_%d_%lld"),FCrc::MemCrc32(Controls.GetData(),Controls.Num()*sizeof(FVector2D)),PaveVehicle,Width,Curbs,Lanes,Existing->Revision);const auto* PreviewSite=Site(GetWorld());const auto* Shape=PreviewSite?(PreviewSite->bPavementPreviewActive?&PreviewSite->LastPavementPreview:PreviewSite->GetSelectedObject()):nullptr;
 const FString ShapeKey=Key+(Shape?FString::Printf(TEXT("_R%.2f_%d"),Shape->Pavement.EntranceRadiusCm,Shape->Pavement.EntranceEnds)+FString::FromInt(Shape->Pavement.Spline):TEXT(""));if(ShapeKey==PaveSweepKey)return;PaveSweepKey=ShapeKey;PaveSweepLines.Reset();
 FFoundrylineDrivingMap Map=*Existing;if(const auto* SiteActor=Site(GetWorld()))if(const auto* Selected=SiteActor->GetSelectedObject();Selected&&Selected->Pavement.Lanes)Map.Surfaces.RemoveAll([&](const auto& V){return V.Id==Selected->ObjectId;});FFoundrylineDriveSurface Surface;Surface.Id=FGuid(0xF001,0xC0DE,1,1);Surface.Function=TEXT("Pavement");Surface.Size={1000000,1000000};Surface.Pavement.Polygons=G::Road(Controls,Width+(Curbs?36:0));if(const auto* S=Site(GetWorld())){const auto* O=S->bPavementPreviewActive?&S->LastPavementPreview:S->GetSelectedObject();if(O&&O->Pavement.Lanes){const FTransform T(FRotator(0,O->QuarterTurns*90,0),FVector(O->Anchor));Surface.Pavement.Polygons=G::Transform(O->Pavement.Polygons,T);}}Surface.Pavement.Curbs=Curbs;const auto Bounds=G::Bounds(Surface.Pavement.Polygons);if(!Bounds.bIsValid){PaveSweepClear=false;PaveSweepStatus=TEXT("Adjust the road shape before checking the vehicle");return;}G::Translate(Surface.Pavement,-Bounds.Min);Surface.Transform=FTransform(FVector(Bounds.Min,0));Surface.Size=Bounds.GetSize();Map.Surfaces.Add(Surface);
 auto Path=G::RoadPath(Controls,Shape&&Shape->Pavement.Spline);if(Path.Num()==2){const int N=FMath::Clamp(FMath::CeilToInt(FVector2D::Distance(Path[0],Path[1])/75),2,128);const auto A=Path[0],B=Path[1];Path.Reset();for(int I=0;I<=N;++I)Path.Add(FMath::Lerp(A,B,double(I)/N));}if(Path.Num()<2)return;
 const auto V=PaveVehicle==0?FFoundrylineVehicleProfile::Car():PaveVehicle==1?FFoundrylineVehicleProfile::BoxTruck():FFoundrylineVehicleProfile::Truck();
 // Right-hand travel for two-way roads. A driver follows a smooth lane, with the trailer simulated behind it.
 const auto OriginalPath=Path;for(int I=0;I<Path.Num();++I){const auto F=(OriginalPath[FMath::Min(I+1,Path.Num()-1)]-OriginalPath[FMath::Max(0,I-1)]).GetSafeNormal();if(Lanes>1)Path[I]+=FVector2D(F.Y,-F.X)*(Width/4);}
 double Length=0;for(int I=1;I<Path.Num();++I)Length+=FVector2D::Distance(Path[I-1],Path[I]);const auto D=(Path[1]-Path[0]).GetSafeNormal();FFoundrylineVehiclePose Pose;Pose.Position=Path[0];Pose.Yaw=Pose.TrailerYaw=FMath::Atan2(D.Y,D.X);double Travel=0;bool Checked=false;PaveSweepClear=true;
 auto Cage=[&](FVector2D At,double Yaw,double Front,double Rear,double HalfWidth){const FVector2D F(FMath::Cos(Yaw),FMath::Sin(Yaw)),R(-F.Y,F.X);const FVector2D Corners[]={At+F*Front+R*HalfWidth,At+F*Front-R*HalfWidth,At-F*Rear-R*HalfWidth,At-F*Rear+R*HalfWidth};for(int I=0;I<4;++I){PaveSweepLines.Add(FVector(Corners[I],28));PaveSweepLines.Add(FVector(Corners[(I+1)%4],28));}};
 for(int I=1;I<Path.Num();++I){const auto F=(Path[I]-Path[I-1]).GetSafeNormal();const double Distance=FVector2D::Distance(Path[I-1],Path[I]),Yaw=FMath::Atan2(F.Y,F.X),Change=FMath::FindDeltaAngleRadians(Pose.Yaw,Yaw);Pose=FFoundrylineVehiclePath::Advance(Pose,V,Distance,Distance>.001?Change/Distance:0);Pose.Position=Path[I];Pose.Yaw=Yaw;Travel+=Distance;
  if(Travel<(V.Semi?V.TrailerRear+V.Hitch:V.Rear)+30||Length-Travel<V.Front+30)continue;Checked=true;if(!FFoundrylineVehiclePath::PoseClear(Map,V,Pose,false))PaveSweepClear=false;
  if(I%6==0){Cage(Pose.Position,Pose.Yaw,V.Front,V.Rear,V.Width/2+V.Clearance);if(V.Semi){const FVector2D Heading(FMath::Cos(Pose.Yaw),FMath::Sin(Pose.Yaw));Cage(Pose.Position-Heading*V.Hitch,Pose.TrailerYaw,V.TrailerFront,V.TrailerRear,V.TrailerWidth/2+V.Clearance);}}
 }
 const TCHAR* Name=PaveVehicle==0?TEXT("Car"):PaveVehicle==1?TEXT("Box truck"):TEXT("Semi");PaveSweepStatus=!Checked?TEXT("Connect more pavement to check the full vehicle"):FString(Name)+(PaveSweepClear?TEXT(" sweep fits this preview"):TEXT(" needs more room / widen the inside of the turn"));if(!Checked)PaveSweepClear=false;
}

bool AFoundrylinePlayerController::FindPavementHandle(FIntVector& At,FName& Tool,FString& Hint)const{
 const auto* S=Site(GetWorld());const auto* O=S?S->GetSelectedObject():nullptr;if(!O||O->Pavement.Polygons.IsEmpty())return false;
 float X,Y;if(!GetMousePosition(X,Y))return false;const FTransform T(FRotator(0,O->QuarterTurns*90,0),FVector(O->Anchor));int VW,VH;GetViewportSize(VW,VH);const double Scale=FMath::Min(1.5,double(VH)/1320);double Best=FMath::Square(16*Scale);bool Found=false;
 for(const auto& H:G::EditHandles(O->Pavement)){const auto W=T.TransformPosition(FVector(H.Position,20));FVector2D Screen;if(!ProjectWorldLocationToScreen(W,Screen,false))continue;const double D=FVector2D::DistSquared(Screen,{X,Y});if(D<Best){Best=D;At=FIntVector(FVector(W.X,W.Y,0));Tool=H.Tool;Hint=H.Hint;Found=true;}}
 return Found;
}

EFoundrylineSnap AFoundrylinePlayerController::GetPlacementSnap()const{
 return IsPavementTool()||bPaveDirectDrag?EFoundrylineSnap::Fine25cm:BuildTool.IsNone()?ActiveSnap:EFoundrylineSnap::Structural100cm;
}
