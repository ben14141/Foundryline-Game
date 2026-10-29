#include "Production/FoundrylineFactoryPlant.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/App.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineMachineCellLayout.h"
#include "Pathing/FoundrylineFloorLaneGeometry.h"
namespace {
 // The decal's local Z/Y axes are world X/Y with a -90 degree pitch.
 // Bake in those same coordinates: camera position and viewport size never enter paint placement.
 const FVector2D PaintMin(-2450,-50), PaintMax(5650,5150);
 constexpr int PaintWidth=4096, PaintHeight=2688;
 FVector2D PaintPixel(FVector2D P){const auto D=PaintMax-PaintMin;return {(P.X-PaintMin.X)*PaintWidth/D.X,(P.Y-PaintMin.Y)*PaintHeight/D.Y};}
 const FLinearColor PaintColors[]={{1,.62,.04},{.035,.48,.12},{.95,.95,.88},{.07,.32,.85},{.95,.25,.035}};
}
void AFoundrylineFactoryPlant::SetupFloorVisuals(){
 if(!FApp::CanEverRender())return;
 FloorPaintTexture=NewObject<UTextureRenderTarget2D>(this);
 FloorPaintTexture->ClearColor=FLinearColor::Transparent;
 FloorPaintTexture->bAutoGenerateMips=true;
 FloorPaintTexture->Filter=TF_Trilinear;
 FloorPaintTexture->InitCustomFormat(PaintWidth,PaintHeight,PF_B8G8R8A8,true);
 FloorPaintTexture->UpdateResourceImmediate(true);
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryFloorDecal.M_FactoryFloorDecal"));
 if(!ensure(Base))return;
 FloorPaintMaterial=UMaterialInstanceDynamic::Create(Base,this);
 FloorPaintMaterial->SetTextureParameterValue(TEXT("PaintAtlas"),FloorPaintTexture);
 FloorPaintDecal=NewObject<UDecalComponent>(this,TEXT("FactorySurfacePaint"));
 FloorPaintDecal->SetupAttachment(RootComponent);
 FloorPaintDecal->SetRelativeLocationAndRotation(FVector((PaintMin+PaintMax)/2,20),FRotator(-90,0,0));
 const auto Size=PaintMax-PaintMin;
 // Only the slab surface receives this paint; equipment and pallet tops are above this volume.
 FloorPaintDecal->DecalSize=FVector(1,Size.Y/2,Size.X/2);
 FloorPaintDecal->SetFadeScreenSize(0);
 FloorPaintDecal->SetSortOrder(10);
 FloorPaintDecal->SetDecalMaterial(FloorPaintMaterial);
 FloorPaintDecal->RegisterComponent();AddInstanceComponent(FloorPaintDecal);
}
void AFoundrylineFactoryPlant::RebuildFloor(){
 FString Key;for(const auto& L:Snapshot.Lanes)Key+=FString::Printf(TEXT("%d:%d:%s:%s:%d;"),L.Id,int(L.Kind),*L.Min.ToString(),*L.Max.ToString(),L.Direction);for(const auto& Z:Snapshot.Zones)Key+=FString::Printf(TEXT("%d:%s:%s:%s:%s:%d;"),Z.Id,*Z.Min.ToString(),*Z.Max.ToString(),*Z.DropMin.ToString(),*Z.DropMax.ToString(),Z.QuarterTurns);if(FloorLayoutKey==Key)return;FloorLayoutKey=Key;
 if(!FloorPaintTexture)return;
 UKismetRenderingLibrary::ClearRenderTarget2D(this,FloorPaintTexture,FLinearColor::Transparent);
 UCanvas* Canvas=nullptr;FVector2D CanvasSize;FDrawToRenderTargetContext Context;
 UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this,FloorPaintTexture,Canvas,CanvasSize,Context);
 if(!Canvas)return;
 Canvas->Canvas->SetWriteDestinationAlpha(true);
 auto Box=[&](int Style,FVector P,FVector Size,FRotator R=FRotator::ZeroRotator){
  FVector2D V[4];const FVector Local[]={{-Size.X/2,-Size.Y/2,0},{Size.X/2,-Size.Y/2,0},{Size.X/2,Size.Y/2,0},{-Size.X/2,Size.Y/2,0}};
  for(int I=0;I<4;++I)V[I]=PaintPixel(FVector2D(P+R.RotateVector(Local[I])));
  for(int I=1;I<3;++I){FCanvasTriangleItem Tri(V[0],V[I],V[I+1],GWhiteTexture);Tri.SetColor(PaintColors[Style]);Tri.BlendMode=SE_BLEND_Opaque;Canvas->DrawItem(Tri);}
 };
 auto Outline=[&](int Style,FVector2D A,FVector2D B,float Width){const FVector2D C=(A+B)/2,D=B-A;Box(Style,{C.X,A.Y,22},{D.X,Width,1});Box(Style,{C.X,B.Y,22},{D.X,Width,1});Box(Style,{A.X,C.Y,22},{Width,D.Y,1});Box(Style,{B.X,C.Y,22},{Width,D.Y,1});};
 auto Hatch=[&](int Style,FVector2D A,FVector2D B,float Spacing,float Width){for(float K=A.Y-B.X;K<=B.Y-A.X;K+=Spacing){TArray<FVector2D> Points;for(FVector2D P:{FVector2D(A.X,A.X+K),FVector2D(B.X,B.X+K),FVector2D(A.Y-K,A.Y),FVector2D(B.Y-K,B.Y)})if(P.X>=A.X-.01&&P.X<=B.X+.01&&P.Y>=A.Y-.01&&P.Y<=B.Y+.01)Points.AddUnique(P);if(Points.Num()<2)continue;const FVector2D Delta=Points.Last()-Points[0],Center=(Points.Last()+Points[0])/2;if(Delta.Size()<5)continue;Box(Style,FVector(Center,22.1),{Delta.Size(),Width,.8},FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X)),0));}};
 auto Text=[&](FString Value,FVector P,float Size,float Yaw=0){
  const auto Pixel=PaintPixel(FVector2D(P));auto* Font=GEngine->GetMediumFont();
  FCanvasTextItem Item(FVector2D::ZeroVector,FText::FromString(Value),Font,FLinearColor(.025,.03,.035,1));
  Item.Scale=FVector2D(FMath::Max(Size,25.f)*PaintWidth/(PaintMax.X-PaintMin.X)/Font->GetMaxCharHeight());
  Item.BlendMode=SE_BLEND_Translucent;
  Canvas->Canvas->PushRelativeTransform(FTransform(FRotator(0,Yaw,0),FVector(Pixel,0)).ToMatrixWithScale());
  Canvas->DrawItem(Item);Canvas->Canvas->PopTransform();
 };
 auto Line=[&](int Style,FVector2D A,FVector2D B,float Width,int Ignore,FVector2D Probe=FVector2D::ZeroVector){for(const auto& Segment:FFoundrylineFloorLaneGeometry::VisibleLine(A,B,Snapshot.Lanes,Ignore,Probe,Style==1?EFoundrylineFloorLane::Pedestrian:EFoundrylineFloorLane::Forklift)){const FVector2D D=Segment.Value-Segment.Key,P=(Segment.Key+Segment.Value)/2;Box(Style,FVector(P,22),FVector(D.Size(),Width,1),FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(D.Y,D.X)),0));}};
 for(const auto& L:Snapshot.Lanes){const FVector2D C=(L.Min+L.Max)/2,D=L.Max-L.Min;const bool X=D.X>D.Y;
  if(L.Kind==EFoundrylineFloorLane::Crossing)continue;
  if(L.Kind==EFoundrylineFloorLane::Forklift){if(X){for(float Y:{float(L.Min.Y),float(L.Max.Y)}){const float Side=Y==L.Min.Y?-1:1;Line(0,{L.Min.X,Y},{L.Max.X,Y},7,L.Id,{0,Side*.2});Line(0,{L.Min.X,Y-Side*12},{L.Max.X,Y-Side*12},3,L.Id,{0,Side*12.2});}}else for(float PX:{float(L.Min.X),float(L.Max.X)}){const float Side=PX==L.Min.X?-1:1;Line(0,{PX,L.Min.Y},{PX,L.Max.Y},7,L.Id,{Side*.2,0});Line(0,{PX-Side*12,L.Min.Y},{PX-Side*12,L.Max.Y},3,L.Id,{Side*12.2,0});}
   const float Angle=X?0:90;for(int Direction:{-1,1}){if(L.Direction&&(L.Direction==1?Direction<0:Direction>0))continue;const FVector P(C.X+(X?Direction*D.X*.22:0),C.Y+(X?0:Direction*D.Y*.22),22);const FRotator R(0,Angle+(Direction<0?180:0),0);Box(0,P,{100,9,1},R);for(int Side:{-1,1})Box(0,P+R.RotateVector(FVector(30,Side*16,0)),{55,9,1},FRotator(0,R.Yaw+Side*40,0));}
   if(FMath::Max(D.X,D.Y)>600)Text(TEXT("FORKLIFTS"),FVector(C.X-90,C.Y-65,23),24,Angle);
  }else {
   if(X){for(double Y:{L.Min.Y,L.Max.Y}){const double Side=Y==L.Min.Y?-1:1;Line(1,{L.Min.X,Y},{L.Max.X,Y},7,L.Id,{0,Side*.2});}}
   else for(double PX:{L.Min.X,L.Max.X}){const double Side=PX==L.Min.X?-1:1;Line(1,{PX,L.Min.Y},{PX,L.Max.Y},7,L.Id,{Side*.2,0});}
   const bool AtCrossing=Snapshot.Lanes.ContainsByPredicate([&](const auto& F){return F.Kind==EFoundrylineFloorLane::Forklift&&FBox2D(F.Min,F.Max).IsInsideOrOn(C);});
   if(!AtCrossing)Text(TEXT("PEOPLE"),FVector(C.X-45,C.Y-10,23),20,X?0:90);
  }
 }
 for(const auto& Crossing:FFoundrylineFloorLaneGeometry::Crossings(Snapshot.Lanes)){const auto A=Crossing.Bounds.Min,B=Crossing.Bounds.Max,C=Crossing.Bounds.GetCenter(),Size=Crossing.Bounds.GetSize();if(Crossing.WalkingX){for(double X=A.X+15;X<B.X-10;X+=65)Box(2,{X,C.Y,22.6},{30,Size.Y,1});}else for(double Y=A.Y+15;Y<B.Y-10;Y+=65)Box(2,{C.X,Y,22.6},{Size.X,30,1});}
 FFoundrylineFactorySimulation View;View.State=Snapshot;const EFoundrylinePart Parts[]={EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings};const TCHAR* Names[]={TEXT("BRACKETS"),TEXT("FRAME RAILS"),TEXT("SHELLS"),TEXT("WHOLE HEADS"),TEXT("RUNNING GEAR"),TEXT("FITTINGS")};
 if(bCutPartsStarter){
  for(const auto& Rack:FFoundrylineWorkshopLayout::CutShopRacks()){const FVector2D P(Rack.Center),Half(75,220);Outline(3,P-Half,P+Half,6);Text(Rack.Name,Rack.Center+FVector(-70,-240,3),18);}
  const FBox2D Receiving({1050,1480},{1350,1900}),Shipping({-1100,1850},{-600,2170});
  Outline(3,Receiving.Min,Receiving.Max,6);Text(TEXT("RAW STEEL ARRIVAL"),FVector(Receiving.Min+FVector2D(-30,-35),23),18);
  Outline(3,Shipping.Min,Shipping.Max,6);Hatch(3,Shipping.Min,Shipping.Max,90,3);Text(TEXT("OUTBOUND / CUT-PART PALLETS"),FVector(Shipping.Min+FVector2D(20,30),23),22);
  const FBox2D Bay({0,220},{200,720});Outline(2,Bay.Min,Bay.Max,4);Text(TEXT("FORK 01"),FVector(Bay.Min+FVector2D(10,20),23),22);
  const FBox2D CutCell({-1600,400},{-1000,1100});Outline(4,CutCell.Min,CutCell.Max,6);Text(TEXT("01 / BURN TABLE"),FVector(-1580,370,23),22);
  const FBox2D Apron({500,1250},{2700,2050});Outline(3,Apron.Min,Apron.Max,6);Text(TEXT("BOX TRAILER / PALLET LOADING"),FVector(600,1300,23),28);
 }else {
 const FVector2D Steel(FFoundrylineWorkshopLayout::SteelArrival());Outline(3,Steel-FVector2D(100,180),Steel+FVector2D(100,180),6);Hatch(3,Steel-FVector2D(100,180),Steel+FVector2D(100,180),70,2);Text(TEXT("STEEL RECEIVING"),FVector(Steel+FVector2D(-90,-200),23),18);
 for(const auto& Slot:Snapshot.Slots)if(Slot.Id==104){const FVector2D P(Slot.Position);Outline(3,P-FVector2D(150),P+FVector2D(150),6);Text(TEXT("WHOLE HEADS"),Slot.Position+FVector(-130,-170,3),18);}
 for(const auto& Rack:FFoundrylineWorkshopLayout::Racks()){const FVector2D P(Rack.Center),Half=Rack.Yaw==0?FVector2D(75,220):FVector2D(220,75);Outline(3,P-Half,P+Half,6);Hatch(3,P-Half,P+Half,50,2);Text(Rack.Name,Rack.Center+FVector(-80,-240,3),20);}
 for(int I=0;I<4;++I){const auto Bay=FFoundrylineWorkshopLayout::DriverParkingBounds(I);Outline(2,Bay.Min,Bay.Max,4);Text(FString::Printf(TEXT("FORK %02d"),I+1),FVector(Bay.Min+FVector2D(10,20),20),25);}
 for(int Id=1;Id<=3;++Id){const auto CellBounds=FFoundrylineMachineCellLayout::Bounds(Id);Outline(4,CellBounds.Min,CellBounds.Max,6);Hatch(4,CellBounds.Min,CellBounds.Max,75,2);}
 const auto Exit=FFoundrylineWorkshopLayout::PickupClearance();for(float Y:{float(Exit.Min.Y),float(Exit.Max.Y)})for(double X=Exit.Min.X;X<Exit.Max.X;X+=90)Box(2,{X+20,Y,22},{40,5,1});Text(TEXT("TANKER BAY / DIRECT TO LOADING DOOR"),FVector(1400,Exit.Min.Y+35,23),20);
 const FBox2D Inbound({2900,2700},{5400,3300});Outline(3,Inbound.Min,Inbound.Max,7);Hatch(3,Inbound.Min,Inbound.Max,120,3);Text(TEXT("FORKLIFT / MATERIAL RECEIVING"),FVector(Inbound.Min+FVector2D(60,35),23),28);
 const auto Pickup=FFoundrylineWorkshopLayout::PickupBay();Outline(3,Pickup.Min,Pickup.Max,7);Hatch(3,Pickup.Min,Pickup.Max,120,3);Text(TEXT("FINISHED TANKER PICKUP"),FVector(Pickup.Min+FVector2D(60,35),23),32);
 const FBox2D SecondPickup({2800,4050},{4000,4650});Outline(3,SecondPickup.Min,SecondPickup.Max,7);Hatch(3,SecondPickup.Min,SecondPickup.Max,120,3);Text(TEXT("TANKER BAY 02 / PICKUP"),FVector(SecondPickup.Min+FVector2D(60,35),23),26);

 for(int I=1;I<=3;++I){const TCHAR* CellNames[]={TEXT("01 / PLASMA CUTTING"),TEXT("02 / PRESS BRAKE"),TEXT("03 / SHEET ROLLER")};Text(CellNames[I-1],FFoundrylineWorkshopLayout::Machine(I)+FVector(-280,-400,3),28);const auto P=FFoundrylineWorkshopLayout::OutputStock(I);Outline(3,FVector2D(P)-FVector2D(100,180),FVector2D(P)+FVector2D(100,180),5);Hatch(3,FVector2D(P)-FVector2D(100,180),FVector2D(P)+FVector2D(100,180),50,2);Text(TEXT("OUTPUT PALLET"),P+FVector(-100,190,3),16);}
 }
 if(bCutPartsStarter&&Snapshot.Machines.ContainsByPredicate([](const auto& M){return M.Id==2;})){Outline(4,{-2100,1600},{-1600,2100},6);Text(TEXT("02 / PRESS BRAKE"),{-2070,1580,23},22);}
 for(int ZI=0;ZI<Snapshot.Zones.Num();++ZI){const auto& Z=Snapshot.Zones[ZI];const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(Z);Outline(4,Bounds.Min,Bounds.Max,12);Hatch(4,FVector2D(Z.Min),FVector2D(Z.Max),150,8);Text(FString::Printf(TEXT("WORK ZONE %02d / TANKER"),ZI+1),Z.Min+FVector(60,65,3),38);
  const auto Equipment=FFoundrylineWorkZoneLayout::WelderBounds(Z).ExpandBy(10);Outline(2,Equipment.Min,Equipment.Max,3);Text(TEXT("MIG WELDER"),FVector(Equipment.Min+FVector2D(0,-20),23),14);
  Text(FString::Printf(TEXT("ZONE %02d / SUPPLIES"),ZI+1),Z.DropMin+FVector(25,15,3),22);for(int I=0;I<6;++I){const auto P=View.BufferPosition(Z,Parts[I]);const auto Size=FFoundrylineWorkZoneLayout::BaySize(Z,Parts[I]);Outline(3,FVector2D(P)-Size/2,FVector2D(P)+Size/2,6);Hatch(3,FVector2D(P)-Size/2,FVector2D(P)+Size/2,45,2);Text(Names[I],P+FVector(-Size.X/2+10,-Size.Y/2+15,3),15);}
 }
 UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this,Context);
 FloorPaintTexture->UpdateResourceImmediate(false);

}
