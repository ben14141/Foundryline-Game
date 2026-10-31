#include "Construction/FoundrylinePavementGeometry.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"

void AFoundrylineBuildHUD::DrawLogisticsMap(AFoundrylinePlayerController* PC)
{
 const auto M=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();const auto& Trip=PC->LogisticsTrip;const auto& Test=PC->TripResult;
 const float Scale=FMath::Min(1.5f,Canvas->ClipY/1320.f);auto* Font=GEngine->GetMediumFont();
 const FLinearColor Green(.12f,.85f,.48f,1),Blue(.1f,.6f,1,1),Amber(1,.52f,.1f,1),Red(1,.15f,.1f,1),Paper(.97f,.96f,.93f,1),Ink(.025f,.035f,.045f,1);
 int32 ViewW=0,ViewH=0;PC->GetViewportSize(ViewW,ViewH);
 auto Project=[&](FVector2D W,FVector2D& S){if(ViewW<=0 || ViewH<=0 || !PC->ProjectWorldLocationToScreen(FVector(W,55),S,false)) return false;S.X*=Canvas->ClipX/ViewW;S.Y*=Canvas->ClipY/ViewH;return true;};
 auto Polygon=[&](const TArray<FVector2D>& World,FLinearColor Color) {
  TArray<FVector2D> Points;for(const auto& W:World) {FVector2D P;if(!Project(W,P)) return;Points.Add(P);}if(Points.Num()<3) return;
  for(int32 I=1;I<Points.Num()-1;++I) {FCanvasTriangleItem T(Points[0],Points[I],Points[I+1],GWhiteTexture);T.SetColor(Color);T.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(T);}
 };
 auto Stroke=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width){FVector2D P,Q;if(Project(A,P) && Project(B,Q)) DrawLine(P.X,P.Y,Q.X,Q.Y,Color,Width*Scale);};
 auto Shape=[&](const FFoundrylineDriveSurface& S,const TArray<FFoundrylinePavementPolygon>& Polys,FLinearColor Color){for(const auto& P:Polys){TArray<FVector2D> V;TArray<int32> T;if(!FFoundrylinePavementGeometry::Triangulate(P,V,T))continue;for(int I=0;I<T.Num();I+=3)Polygon({S.World(V[T[I]]),S.World(V[T[I+1]]),S.World(V[T[I+2]])},Color);}};
 auto Rect=[&](const FFoundrylineDriveSurface& S,const FFoundrylineDriveRect& R,FLinearColor Color){if(!S.Pavement.Polygons.IsEmpty()){const auto Box=FFoundrylinePavementGeometry::Rectangle(R.Min,R.Max);TArray<FFoundrylinePavementPolygon> Outside,Clipped;if(FFoundrylinePavementGeometry::Boolean(Box,S.Pavement.Polygons,true,Outside)&&FFoundrylinePavementGeometry::Boolean(Box,Outside,true,Clipped))Shape(S,Clipped,Color);}else Polygon({S.World(R.Min),S.World({R.Max.X,R.Min.Y}),S.World(R.Max),S.World({R.Min.X,R.Max.Y})},Color);};
 auto Arrow=[&](FVector2D P,float Yaw,float Length,float Width,FLinearColor C){const FVector2D F(FMath::Cos(Yaw),FMath::Sin(Yaw)),L(-F.Y,F.X);Polygon({P-F*Length*.5-L*Width*.22,P+F*Length*.1-L*Width*.22,P+F*Length*.1-L*Width*.5,P+F*Length*.5,P+F*Length*.1+L*Width*.5,P+F*Length*.1+L*Width*.22,P-F*Length*.5+L*Width*.22},C);};
 auto Label=[&](FVector2D W,const FString& Value,FLinearColor Color,float Size=1.3f){FVector2D P;if(!Project(W,P) || P.X<420*Scale || P.X>Canvas->ClipX-20 || P.Y<15 || P.Y>Canvas->ClipY-20) return;float TW=0,TH=0;GetTextSize(Value,TW,TH,Font,Size*Scale);DrawRect(Ink,P.X-TW/2-8*Scale,P.Y-TH/2-6*Scale,TW+16*Scale,TH+12*Scale);DrawRect(Color,P.X-TW/2-8*Scale,P.Y+TH/2+4*Scale,TW+16*Scale,2*Scale);DrawText(Value,Color,P.X-TW/2,P.Y-TH/2,Font,Size*Scale);};
 for(const auto& S:M->Surfaces) {
  if(S.Public) {Rect(S,{{0,0},S.Size},FLinearColor(.15f,.2f,.25f,.16f));continue;}
  if(S.Function==TEXT("Pavement")&&PC->bShowTruckLayer&&S.Pavement.Lanes)Shape(S,S.Pavement.Polygons,FLinearColor(.12f,.55f,.72f,.12f));
  if(S.Function==TEXT("Parking")) {
   if(!PC->bShowParkingLayer) continue;Rect(S,{{0,0},S.Size},FLinearColor(.1f,.5f,.9f,.15f));
   for(const auto& A:S.Aisles) Rect(S,A,FLinearColor(.12f,.5f,.9f,.12f));
   for(const auto& E:S.Entrances) {Rect(S,E,FLinearColor(.2f,.65f,1,.5f));Arrow(S.World((E.Min+E.Max)/2),FMath::DegreesToRadians(S.Transform.Rotator().Yaw+90),450,220,Blue);}
   Label(S.World({S.Size.X/2,S.Size.Y+180}),TEXT("CAR PARKING"),Blue,1.5f);
  }
  if(PC->bShowTruckLayer) {
   for(const auto& Z:S.TruckZones) Rect(S,Z,FLinearColor(.1f,.8f,.42f,S.Function==TEXT("Parking")?.13f:.32f));
   if(S.Function==TEXT("FreightYard")) Label(S.World({S.Size.X*.68,S.Size.Y/2}),TEXT("TRUCK TURNING AREA"),Green,1.5f);
  }
  for(const auto& Island:S.Islands) Rect(S,Island,FLinearColor(.8f,.15f,.12f,.6f));
 }
 FGuid HoverTarget,HoverGate;FIntVector Cursor=FIntVector::ZeroValue;const bool Picking=PC->IsEditingLogistics() && !PC->IsOverBuildHUD() && PC->GetCursorAnchor(Cursor);
 const auto Step=PC->BuildTool;
 if(Picking && Step==TEXT("TripDestination")) {
  double Best=FMath::Square(900.);const FVector2D Point(Cursor.X,Cursor.Y);
  for(const auto& T:M->Targets) if(T.Semi==Trip.Semi) {const double D=FMath::Min(FVector2D::DistSquared(Point,FFoundrylineLogisticsTrip::TargetMarker(T)),FVector2D::DistSquared(Point,T.Pose.Position));if(D<Best) {Best=D;HoverTarget=T.Id;}}
 }
 if(Picking && (Step==TEXT("TripEntrance") || Step==TEXT("TripExit"))) {
  double Best=FMath::Square(1400.);for(const auto& G:Trip.Semi?PC->TruckGates:PC->CarGates) {const double D=FVector2D::DistSquared(FVector2D(Cursor.X,Cursor.Y),G.Portal);if(D<Best) {Best=D;HoverGate=G.SurfaceId;}}
 }
 auto Halo=[&](FVector2D Center,FLinearColor C){const float Radius=180+20*FMath::Sin(GetWorld()->GetTimeSeconds()*4);for(int32 I=0;I<24;++I) {const float A=I*2*PI/24,B=(I+1)*2*PI/24;Stroke(Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius,Center+FVector2D(FMath::Cos(B),FMath::Sin(B))*Radius,C,3);}};
 if(PC->LogisticsPanel==1) for(const auto& T:M->Targets) {
  if(T.Semi?!PC->bShowTruckLayer:!PC->bShowParkingLayer) continue;
  const bool Selected=T.Id==Trip.TargetId,Hovered=T.Id==HoverTarget;const auto C=(T.Semi && Trip.SmallTruck?T.SmallTruckClear:T.LayoutClear)?(T.Semi?Amber:Blue):Red;
  if(!T.Loading) if(const auto* S=M->Surfaces.FindByPredicate([&](const auto& S){return S.Id==T.SurfaceId;})) {auto Fill=C;Fill.A=Hovered?.75f:Selected?.65f:.23f;if(T.Footprint.IsEmpty())Rect(*S,T.Bay,Fill);else {TArray<FVector2D> Vertices;for(const auto Point:T.Footprint)Vertices.Add(S->World(Point));Polygon(Vertices,Fill);}}
  const auto Name=(T.Loading || Selected || Hovered || T.Semi)?FFoundrylineLogisticsTrip::TargetLabel(*M,T.Id):FString();const auto Where=FFoundrylineLogisticsTrip::TargetMarker(T);
  if(Selected || Hovered) Halo(Where,Hovered?Paper:C);
  if(T.Loading || Selected || Hovered) Label(Where,(Hovered?TEXT("CLICK / "):Selected?TEXT("3 / "):TEXT(""))+Name,C,Hovered?1.9f:Selected?1.65f:1.45f);
  else if(T.Semi) {const int32 Number=FCString::Atoi(*Name.Right(2));Label(Where,FString::Printf(TEXT("%s %02d"),T.Footprint.IsEmpty()?TEXT("STAGING"):TEXT("LOADING"),Number),C,1.25f);}
 }
 if(PC->LogisticsPanel==1) for(bool Semi:{true,false}) {
  if(Semi?!PC->bShowTruckLayer:!PC->bShowParkingLayer) continue;
  for(const auto& G:Semi?PC->TruckGates:PC->CarGates) {
   const FVector2D In(FMath::Cos(G.InwardYaw),FMath::Sin(G.InwardYaw)),Side(-In.Y,In.X);
   const bool Entry=Trip.HasEntry && Trip.Semi==Semi && Trip.Entrance.SurfaceId==G.SurfaceId&&FVector2D::Distance(Trip.Entrance.Portal,G.Portal)<100,Exit=Trip.HasExit && Trip.Semi==Semi && Trip.Departure.SurfaceId==G.SurfaceId&&FVector2D::Distance(Trip.Departure.Portal,G.Portal)<100;
   if(G.SurfaceId==HoverGate) {Halo(G.Portal,Paper);Label(G.Portal-In*220,Step==TEXT("TripExit")?TEXT("CLICK TO SET EXIT"):TEXT("CLICK TO SET ENTRANCE"),Paper,1.75f);}
   if(G.CanEnter)Arrow(G.Portal+In*350-Side*(Semi?350:110),G.InwardYaw,600,Semi?260:150,Semi?Green:Blue);
   if(G.CanExit)Arrow(G.Portal+In*350+Side*(Semi?350:110),G.InwardYaw+PI,600,Semi?260:150,Paper);
   Label(G.Portal+In*950,(Entry && Exit?TEXT("ENTRANCE + EXIT / "):Entry?TEXT("1 / "):Exit?TEXT("4 / "):TEXT(""))+G.Label,Semi?Green:Blue,1.65f);
  }
 }
 auto Leg=[&](const FFoundrylineVehicleRoute& Route,FLinearColor Color) {
  if(!Route.Success || Route.Poses.IsEmpty()) return;TArray<FVector2D> Points{Route.Poses[0].Position};
  for(int32 I=1;I<Route.Poses.Num();++I) if(I==Route.Poses.Num()-1 || FVector2D::Distance(Points.Last(),Route.Poses[I].Position)>=60) Points.Add(Route.Poses[I].Position);
  // Complete the outline pass before the colour pass; segment caps must not
  // repeatedly overwrite the preceding segment and make the route look striped.
  for(int32 I=1;I<Points.Num();++I) Stroke(Points[I-1],Points[I],Ink,9);
  for(int32 I=1;I<Points.Num();++I) Stroke(Points[I-1],Points[I],Color,5);
  float NextArrow=800,Travelled=0;for(int32 I=1;I<Points.Num();++I) {const auto A=Points[I-1],B=Points[I];Travelled+=FVector2D::Distance(A,B);if(Travelled>=NextArrow){Arrow(B,FMath::Atan2(B.Y-A.Y,B.X-A.X),280,160,Color);NextArrow+=800;}}
 };
 if(PC->LogisticsPanel==1 && Test.Revision==M->Revision) {
  if(PC->LogisticsRouteView!=2) Leg(Test.Inbound,Blue);if(PC->LogisticsRouteView!=1) Leg(Test.Outbound,Amber);
  if(!Test.Success) {const auto* T=M->Targets.FindByPredicate([&](const auto& X){return X.Id==Trip.TargetId;});if(T) Label(Test.Inbound.Success?Trip.Departure.Portal:FFoundrylineLogisticsTrip::TargetMarker(*T),Test.Inbound.Success?TEXT("EXIT TURN NEEDS ROOM"):TEXT("NO CLEAR ARRIVAL"),Red,1.8f);}
 }
 if(PC->LogisticsPanel==1 && Trip.HasEntry) Label(Trip.Entrance.Entry.Position,TEXT("ARRIVAL FROM TOWN"),Blue,1.4f);
 if(PC->LogisticsPanel==1 && Trip.HasExit) Label(Trip.Departure.Exit.Position,TEXT("DEPARTURE TO TOWN"),Amber,1.4f);
 FIntVector BrushA,BrushB;if(PC->LogisticsBrush(BrushA,BrushB)) {FFoundrylineDriveSurface Identity;Identity.Transform=FTransform::Identity;Rect(Identity,{{double(FMath::Min(BrushA.X,BrushB.X)),double(FMath::Min(BrushA.Y,BrushB.Y))},{double(FMath::Max(BrushA.X,BrushB.X)),double(FMath::Max(BrushA.Y,BrushB.Y))}},PC->BuildTool==TEXT("RouteErase")?FLinearColor(1,.15f,.1f,.35f):FLinearColor(.12f,1,.55f,.4f));}
 FFoundrylineVehiclePose P;FString Phase;
 bool HavePose=PC->LogisticsPanel==1 && PC->LogisticsPreviewPose(P,Phase);
 if(PC->LogisticsPanel!=1) {
  const auto& G=PC->RouteNetwork;
  for(int32 I=0;I<G.Nodes.Num();++I) {
   const auto& N=G.Nodes[I];const bool Selected=N.Data.Id==PC->NetworkSelected;
   for(const auto& E:N.Data.Links) if(const auto* B=G.Find(E.To)) {
    const auto D=(B->Position-N.Position).GetSafeNormal(),Side=FVector2D(-D.Y,D.X);const auto Offset=Side*35;
    Stroke(N.Position+Offset,B->Position+Offset,Ink,7);Stroke(N.Position+Offset,B->Position+Offset,E.Reverse?Amber:Blue,3);
    Arrow(FMath::Lerp(N.Position,B->Position,.6)+Offset,FMath::Atan2(D.Y,D.X),220,140,E.Reverse?Amber:Blue);
   }
   for(int32 K=0;K<20;++K) {const float A=K*2*PI/20,B=(K+1)*2*PI/20;Stroke(N.Position+FVector2D(FMath::Cos(A),FMath::Sin(A))*100,N.Position+FVector2D(FMath::Cos(B),FMath::Sin(B))*100,Selected?Paper:N.Data.Staging?Green:Blue,Selected?5:3);}
   if(Selected) Halo(N.Position,Paper);
   const FString Tag=N.Data.Id==PC->NetworkStart?TEXT("START / "):N.Data.Id==PC->NetworkFinish?TEXT("FINISH / "):N.Data.Staging?TEXT("WAIT / "):TEXT("");
   Label(N.Position+FVector2D(0,190),Tag+FString::Printf(TEXT("%02d"),I+1),Selected?Paper:N.Data.Staging?Green:Blue,Selected?1.7f:1.25f);
  }
  if(Picking && PC->BuildTool==TEXT("NetDraw")) if(const auto* N=G.Find(PC->NetworkSelected)) {const FVector2D End(Cursor.X,Cursor.Y);for(int32 I=0;I<16;I+=2) Stroke(FMath::Lerp(N->Position,End,I/16.),FMath::Lerp(N->Position,End,(I+1)/16.),Paper,3);}
  const auto& C=PC->NetworkCheck;
  if(C.Revision==M->Revision && !C.Route.Poses.IsEmpty()) {
   const auto Color=C.Success?(C.Comfortable?Green:Amber):Red;
   const auto V=Trip.Vehicle();
   auto Body=[&](FVector2D Origin,float Yaw,float Front,float Rear,float Width){const FVector2D F(FMath::Cos(Yaw),FMath::Sin(Yaw)),L(-F.Y,F.X);auto Fill=Color;Fill.A=.035f;Polygon({Origin+F*Front+L*Width/2,Origin-F*Rear+L*Width/2,Origin-F*Rear-L*Width/2,Origin+F*Front-L*Width/2},Fill);};
   for(int32 I=0;I<C.Route.Poses.Num();I+=8) if(PC->bNetworkSweep) {const auto& Q=C.Route.Poses[I];Body(Q.Position,Q.Yaw,V.Front,V.Rear,V.Width+2*V.Clearance);if(V.Semi) {const FVector2D F(FMath::Cos(Q.Yaw),FMath::Sin(Q.Yaw));Body(Q.Position-F*V.Hitch,Q.TrailerYaw,V.TrailerFront,V.TrailerRear,V.TrailerWidth+2*V.Clearance);}}
   for(int32 I=1;I<C.Route.Poses.Num();I+=3) {const auto A=C.Route.Poses[I-1].Position,B=C.Route.Poses[FMath::Min(I+2,C.Route.Poses.Num()-1)].Position;Stroke(A,B,Ink,9);Stroke(A,B,Color,5);}
   if(C.Success && PC->LogisticsPanel==0) {const float Clock=FMath::Fmod(PC->NetworkClock,FMath::Max(1.f,C.PoseTimes.Last()+2));int32 Low=0,High=C.PoseTimes.Num()-1;while(Low<High) {const int32 Mid=(Low+High+1)/2;if(C.PoseTimes[Mid]<=Clock) Low=Mid;else High=Mid-1;}const int32 I=Low;P=C.Route.Poses[I];if(I+1<C.Route.Poses.Num()) {const auto& B=C.Route.Poses[I+1];const float A=FMath::Clamp((Clock-C.PoseTimes[I])/FMath::Max(.001f,C.PoseTimes[I+1]-C.PoseTimes[I]),0.f,1.f);P.Position=FMath::Lerp(P.Position,B.Position,A);P.Yaw+=FMath::FindDeltaAngleRadians(P.Yaw,B.Yaw)*A;P.TrailerYaw+=FMath::FindDeltaAngleRadians(P.TrailerYaw,B.TrailerYaw)*A;}Phase=TEXT("CHECKED NODE ROUTE");HavePose=true;}
  }
 }
 if(HavePose) {
  const auto V=Trip.Vehicle();const FVector2D F(FMath::Cos(P.Yaw),FMath::Sin(P.Yaw));
  auto VehicleBox=[&](FVector2D Center,float Yaw,float Length,float Width,FLinearColor Color){const FVector2D D(FMath::Cos(Yaw),FMath::Sin(Yaw)),R(-D.Y,D.X);TArray<FVector2D> Q={Center-D*Length/2-R*Width/2,Center+D*Length/2-R*Width/2,Center+D*Length/2+R*Width/2,Center-D*Length/2+R*Width/2};Polygon(Q,Color);for(int32 I=0;I<4;++I) Stroke(Q[I],Q[(I+1)%4],Ink,3);};
  if(V.Semi) {const FVector2D T(FMath::Cos(P.TrailerYaw),FMath::Sin(P.TrailerYaw));VehicleBox(P.Position-F*V.Hitch+T*((V.TrailerFront-V.TrailerRear)/2),P.TrailerYaw,V.TrailerFront+V.TrailerRear,V.TrailerWidth,Paper);VehicleBox(P.Position-F*V.Hitch-T*450,P.TrailerYaw,60,V.TrailerWidth,Amber);}
  VehicleBox(P.Position+F*((V.Front-V.Rear)/2),P.Yaw,V.Front+V.Rear,V.Width,Trip.Semi?Amber:Blue);
  VehicleBox(P.Position+F*(V.Front-100),P.Yaw,110,V.Width*.75,Ink);
  Label(P.Position+F*(V.Front+150),TEXT("PREVIEW / ")+Phase,Paper,1.35f);
 }
 // A persistent on-map legend makes route direction and layer meanings explicit.
 const float X=430*Scale,Y=24*Scale;DrawRect(Ink,X,Y,470*Scale,38*Scale);
 DrawText(PC->LogisticsPanel!=1?TEXT("Blue: forward   /   Amber: backing   /   Green: waiting"):TEXT("Green: truck access   /   Blue: parking   /   Amber: loading"),Paper,X+14*Scale,Y+12*Scale,Font,1.15f*Scale);
}
