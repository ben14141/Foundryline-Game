#include "Buildings/FoundrylineExteriorKit.h"

void FFoundrylineExteriorKit::ClipJoinedFaces(TArray<FVector2D> (&Spans)[4],float W,float D,FVector2D Min,FVector2D Max){
 for(int E=0;E<4;++E){const bool Touch=E==0?FMath::Abs(Max.Y)<.1:E==1?FMath::Abs(Min.X-W)<.1:E==2?FMath::Abs(Min.Y-D)<.1:FMath::Abs(Max.X)<.1;if(!Touch)continue;const double A=E%2==0?Min.X:Min.Y,B=E%2==0?Max.X:Max.Y;TArray<FVector2D> Next;for(const auto& S:Spans[E]){if(B<=S.X||A>=S.Y){Next.Add(S);continue;}if(A>S.X)Next.Add({S.X,FMath::Min(A,S.Y)});if(B<S.Y)Next.Add({FMath::Max(B,S.X),S.Y});}Spans[E]=MoveTemp(Next);}
}

TArray<FFoundrylineExteriorKit::FPart> FFoundrylineExteriorKit::Generate(const FFoundrylinePlacedObject& O,float W,float D,float RoofZ,const TArray<FVector2D> (&Exposed)[4]){
 TArray<FPart> Out;const float Base=O.bOfficeAnnex?20:60;
 auto Add=[&](uint8 E,float U,float Distance,float Z,float Length,float Depth,float Height,FName Finish,FName Role){if(Length<=.01f||Depth<=.01f||Height<=.01f)return;const FVector Normal=E==0?FVector(0,-1,0):E==1?FVector(1,0,0):E==2?FVector(0,1,0):FVector(-1,0,0);const FVector Wall=E%2==0?FVector(U,E==0?0:D,Z):FVector(E==3?0:W,U,Z);Out.Add({Wall+Normal*Distance,E%2==0?FVector(Length,Depth,Height):FVector(Depth,Length,Height),Finish,Role,E,U});};
 auto Clear=[&](uint8 E,float U,float Half,float Bottom,float Top,float Margin=12.f){return !O.Openings.ContainsByPredicate([&](const auto& V){return V.Edge==E&&U+Half>V.OffsetCm-Margin&&U-Half<V.OffsetCm+V.WidthCm()+Margin&&Top>20+V.BottomCm()-Margin&&Bottom<20+V.BottomCm()+V.HeightCm()+Margin;});};
 auto Fits=[&](uint8 E,float A,float B){return Exposed[E].ContainsByPredicate([&](const auto& S){return A>=S.X-.1&&B<=S.Y+.1;});};
 auto Corner=[&](uint8 E,bool End){const float L=E%2==0?W:D;const uint8 Other=E%2==0?(End?1:3):(End?2:0);const float At=E%2==0?(E==0?0:D):(E==3?0:W);return Fits(E,End?L-12:0,End?L:12)&&Fits(Other,At==0?0:At-12,At==0?12:At);};
 const bool Horizontal=W>=D;bool Utility=false;
 for(uint8 E=0;E<4;++E){
  const float Length=E%2==0?W:D;
  for(const auto& Span:Exposed[E]){
   // Folded plinth cap and drip edge stop at every door rather than bridging its threshold.
   TArray<FVector2D> Bases={Span};for(const auto& Door:O.Openings)if(Door.Edge==E&&Door.Kind!=EFoundrylineOpeningKind::ShopWindow){TArray<FVector2D> Next;for(const auto& B:Bases){if(Door.OffsetCm>=B.Y||Door.OffsetCm+Door.WidthCm()<=B.X){Next.Add(B);continue;}if(Door.OffsetCm>B.X)Next.Add({B.X,double(Door.OffsetCm)});if(Door.OffsetCm+Door.WidthCm()<B.Y)Next.Add({double(Door.OffsetCm+Door.WidthCm()),B.Y});}Bases=MoveTemp(Next);}
   for(const auto& B:Bases){const float U=(B.X+B.Y)*.5f,L=B.Y-B.X;Add(E,U,-15,20+Base+1.5f,L, O.bOfficeAnnex?39:48,3,TEXT("Hardware"),TEXT("PlinthFlashing"));Add(E,U,O.bOfficeAnnex?5:10,20+Base-2,L,1,7,TEXT("Metal"),TEXT("PlinthDrip"));}
   // Thin cladding seams are split vertically around glazing and doors.
   for(float U=FMath::CeilToFloat((Span.X+30)/120)*120;U<Span.Y-30;U+=120){TArray<FVector2D> Runs={{20+Base+7,RoofZ-25}};for(const auto& V:O.Openings)if(V.Edge==E&&U>V.OffsetCm-10&&U<V.OffsetCm+V.WidthCm()+10){TArray<FVector2D> Next;for(const auto& B:Runs){const float A=20+V.BottomCm()-8,Z=A+V.HeightCm()+16;if(Z<=B.X||A>=B.Y){Next.Add(B);continue;}if(A>B.X)Next.Add({B.X,A});if(Z<B.Y)Next.Add({Z,B.Y});}Runs=MoveTemp(Next);}for(const auto& B:Runs)Add(E,U,1.5f,(B.X+B.Y)*.5f,1.2f,2,B.Y-B.X,TEXT("Metal"),TEXT("PanelSeam"));}
   // Roof flashings follow the exposed perimeter; no trim is generated on hidden shared walls.
   Add(E,(Span.X+Span.Y)*.5f,0,RoofZ+2,Span.Y-Span.X,38,2,TEXT("Metal"),TEXT("RoofFlashing"));Add(E,(Span.X+Span.Y)*.5f,20,RoofZ-3,Span.Y-Span.X,1,10,TEXT("Metal"),TEXT("RoofDrip"));
   if((E%2==0)==Horizontal&&Span.Y-Span.X>=250){
    const bool StartClosed=Span.X>0||Corner(E,false),EndClosed=Span.Y<Length||Corner(E,true);const float A=Span.X+(StartClosed?3:0),B=Span.Y-(EndClosed?3:0),U=(A+B)*.5f;
    // Open U-channel: thin bottom, wall-side fold, outer lip and end closures.
    Add(E,U,17,RoofZ-12,B-A,24,1,TEXT("Metal"),TEXT("Gutter"));Add(E,U,5,RoofZ-8,B-A,1,9,TEXT("Metal"),TEXT("Gutter"));Add(E,U,29,RoofZ-8,B-A,1,9,TEXT("Metal"),TEXT("Gutter"));if(StartClosed)Add(E,A,17,RoofZ-8,1,24,9,TEXT("Metal"),TEXT("GutterEnd"));if(EndClosed)Add(E,B,17,RoofZ-8,1,24,9,TEXT("Metal"),TEXT("GutterEnd"));
    const int Count=FMath::Clamp(FMath::CeilToInt((B-A)/1200.f),1,12);for(int I=0;I<Count;++I){if((I==0&&!StartClosed)||(I==Count-1&&!EndClosed))continue;const float Desired=I?FMath::Lerp(A+65,B-65,float(I)/FMath::Max(1,Count-1)):A+65;float Mount=-1;for(float Shift:{0.f,60.f,-60.f,120.f,-120.f,200.f}){const float Candidate=Desired+Shift;if(Candidate<A+30||Candidate>B-30||!Clear(E,Candidate,15,20,RoofZ-20,20))continue;Mount=Candidate;break;}if(Mount<0)continue;
     Add(E,Mount,17,RoofZ-27,14,12,28,TEXT("Metal"),TEXT("Collector"));Add(E,Mount,17,(RoofZ+18)*.5f,10,10,RoofZ-68,TEXT("Metal"),TEXT("Downpipe"));for(float Z=110;Z<RoofZ-40;Z+=150){Add(E,Mount,17,Z,16,12,3,TEXT("Hardware"),TEXT("PipeStrap"));Add(E,Mount,8,Z,22,2,4,TEXT("Metal"),TEXT("PipeBracket"));}Add(E,Mount,25,38,10,28,10,TEXT("Metal"),TEXT("PipeShoe"));Add(E,Mount,37,23,24,48,6,TEXT("Concrete"),TEXT("SplashBlock"));
    }
   }
   if(!Utility&&Span.Y-Span.X>=360){for(float U=Span.X+150;U<Span.Y-100;U+=120){if(!Clear(E,U,65,30,250,25))continue;Add(E,U,11,165,O.bOfficeAnnex?38:58,22,O.bOfficeAnnex?54:78,TEXT("Metal"),TEXT("ElectricalCabinet"));Add(E,U,23,165,O.bOfficeAnnex?32:52,2,O.bOfficeAnnex?48:72,TEXT("Hardware"),TEXT("CabinetDoor"));Add(E,U+12,25,160,3,3,16,TEXT("Metal"),TEXT("CabinetLatch"));Add(E,U-17,13,73,3,3,100,TEXT("Hardware"),TEXT("Conduit"));Add(E,U+40,6,160,3,3,230,TEXT("Hardware"),TEXT("Conduit"));Add(E,U+40,10,280,22,16,24,TEXT("Metal"),TEXT("JunctionBox"));Utility=true;break;}}
   // A compact high vent, sized independently of the wall and checked against actual openings.
   const float VentU=(Span.X+Span.Y)*.5f,VentZ=RoofZ-80;if(Span.Y-Span.X>700&&Clear(E,VentU,44,VentZ-25,VentZ+25)){Add(E,VentU,6,VentZ,88,12,52,TEXT("Metal"),TEXT("VentFrame"));Add(E,VentU,13,VentZ,76,1,40,TEXT("Rubber"),TEXT("VentBack"));for(float Z=VentZ-15;Z<=VentZ+15;Z+=7.5f)Add(E,VentU,15,Z,76,6,2,TEXT("Hardware"),TEXT("VentLouver"));}
  }
  // L-shaped corner caps only at true exposed building corners, not straight expansion seams.
  if(Corner(E,false))Add(E,6,1,RoofZ*.5f+10,12,3,RoofZ-20,TEXT("Metal"),TEXT("CornerCap"));
  if(Corner(E,true))Add(E,Length-6,1,RoofZ*.5f+10,12,3,RoofZ-20,TEXT("Metal"),TEXT("CornerCap"));
 }
 int Number=0;for(const auto& Door:O.Openings){if(Door.IsGlazing()||!Fits(Door.Edge,Door.OffsetCm,Door.OffsetCm+Door.WidthCm()))continue;++Number;const uint8 E=Door.Edge;const float A=Door.OffsetCm,B=A+Door.WidthCm(),U=(A+B)*.5f,Top=20+Door.HeightCm();const bool Garage=Door.Kind==EFoundrylineOpeningKind::Garage||Door.Kind==EFoundrylineOpeningKind::TruckGarage||Door.Kind==EFoundrylineOpeningKind::LoadingDock;
  // Door stop/seal, front-facing jamb covers and a fixed-gauge sill.
  for(float Side:{A-10,B+10})Add(E,Side,18,Top*.5f+10,8,12,Top-20,TEXT("Metal"),TEXT("DoorJamb"));Add(E,U,18,Top+8,B-A+28,12,10,TEXT("Metal"),TEXT("DoorHeader"));Add(E,U,1,21.5f,B-A,36,3,TEXT("Hardware"),TEXT("Threshold"));
  if(Garage){for(float Side:{A-23,B+23}){Add(E,Side,18,105,18,12,90,TEXT("Rubber"),TEXT("DockBumper"));for(float Z:{70.f,140.f})Add(E,Side,25,Z,5,2,5,TEXT("Hardware"),TEXT("BumperBolt"));}Add(E,U,28,Top+39,B-A+35,3,40,TEXT("Metal"),TEXT("RollerHousing"));for(float Side:{A-22,B+22})Add(E,Side,12,Top+39,7,38,48,TEXT("Metal"),TEXT("RollerEnd"));}
  const float CanopyZ=Garage?Top+80:FMath::Min(Top+60,RoofZ-20);if(CanopyZ<RoofZ-12&&CanopyZ>Top+24){const float Width=B-A+(Garage?80:65),Depth=Garage?110:75;if(Fits(E,U-Width*.5f,U+Width*.5f)){Add(E,U,Depth*.5f,CanopyZ,Width,Depth,4,TEXT("Metal"),TEXT("Canopy"));Add(E,U,Depth-1,CanopyZ-3,Width,2,10,TEXT("Metal"),TEXT("CanopyFold"));Add(E,U,2,CanopyZ+5,Width,3,18,TEXT("Metal"),TEXT("CanopyFlashing"));for(float Side:{U-Width*.35f,U+Width*.35f}){Add(E,Side,Depth*.35f,CanopyZ-5,5,Depth*.65f,5,TEXT("Hardware"),TEXT("CanopyBracket"));Add(E,Side,3,CanopyZ-20,8,5,35,TEXT("Metal"),TEXT("CanopyBracket"));}}}
  if(!Garage){for(float SignU:{B+35,A-35}){if(!Fits(E,SignU-15,SignU+15)||!Clear(E,SignU,15,135,165,4))continue;Add(E,SignU,3,150,26,3,24,TEXT("Metal"),TEXT("DoorNumber"));const int Masks[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};const float Direction=E>=2?-1:1;for(int Digit=0;Digit<2;++Digit){const int Value=Digit?Number%10:(Number/10)%10;const FVector2D Pos[]={{0,6},{3,3},{3,-3},{0,-6},{-3,-3},{-3,3},{0,0}};for(int I=0;I<7;++I)if(Masks[Value]&(1<<I)){const bool Bar=I==0||I==3||I==6;Add(E,SignU+Direction*((Digit?5:-5)+Pos[I].X),5,150+Pos[I].Y,Bar?6:1,1,Bar?1:6,TEXT("White"),TEXT("DoorNumber"));}}break;}}
  const float LampU=B+35,LampZ=Garage?FMath::Min(Top+10,RoofZ-45):FMath::Min(Top+20,RoofZ-40);if(Fits(E,LampU-12,LampU+12)&&Clear(E,LampU,10,LampZ-15,LampZ+15,4)){Add(E,LampU,10,LampZ,20,18,28,TEXT("Metal"),TEXT("DoorLamp"));Add(E,LampU,20,LampZ-2,14,2,18,TEXT("White"),TEXT("LampFace"));Add(E,LampU,13,LampZ+14,25,26,3,TEXT("Metal"),TEXT("LampHood"));}
 }
 return Out;
}
