#include "Production/FoundrylineFactorySimulation.h"
#include "Algo/Reverse.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "Production/FoundrylineWeldingStationLayout.h"

bool FFoundrylineFactorySimulation::Walkable(FVector2D P,bool Pedestrian,bool Cart)const{
 const float Radius=Cart?75:Pedestrian?35:80;
 if(P.X<State.FloorMin.X+Radius||P.Y<State.FloorMin.Y+Radius||P.X>State.FloorMax.X-Radius||P.Y>State.FloorMax.Y-Radius)return false;
 for(const auto& Box:Solids)if(Box.ExpandBy(Radius).IsInside(P))return false;
 for(const auto& Z:State.Zones){if(FFoundrylineWorkZoneLayout::WelderBounds(Z).ExpandBy(Radius).IsInside(P))return false;const FVector Local=FFoundrylineWorkZoneLayout::Rotation(Z).UnrotateVector(FVector(P,20)-FFoundrylineWorkZoneLayout::ProductCenter(Z));if(FMath::Abs(Local.X)<665+Radius&&FMath::Abs(Local.Y)<130+Radius)return false;}
 for(const auto& S:State.WeldingStations){if(FFoundrylineWeldingStationLayout::Bench(S).ExpandBy(Radius).IsInside(P))return false;if(!Pedestrian&&FFoundrylineWeldingStationLayout::Bounds(S).ExpandBy(Radius-.01).IsInside(P))return false;}
 auto Inside=[&](FVector2D V){if(Cart)return true;if(Pedestrian){for(const auto& Z:State.Zones)if(FFoundrylineWorkZoneLayout::Bounds(Z).IsInsideOrOn(V))return true;for(const auto& S:State.WeldingStations){for(int Side=0;Side<2;++Side)if(FFoundrylineWeldingStationLayout::Worker(S,Side).IsInsideOrOn(V))return true;for(const auto& R:FFoundrylineWeldingStationLayout::AccessRing(S))if(R.IsInsideOrOn(V))return true;}}for(const auto& L:State.Lanes)if((Pedestrian?(L.Kind!=EFoundrylineFloorLane::Forklift):(L.Kind==EFoundrylineFloorLane::Forklift))&&FBox2D(L.Min,L.Max).IsInsideOrOn(V))return true;return false;};
 return Inside(P)&&Inside(P+FVector2D(Radius,0))&&Inside(P-FVector2D(Radius,0))&&Inside(P+FVector2D(0,Radius))&&Inside(P-FVector2D(0,Radius));
}
namespace {
 // Slab clipping also catches thin obstacles and small gaps between painted sections.
 bool ClipRouteBox(FVector2D A,FVector2D B,const FBox2D& Box,double& Lo,double& Hi){
  Lo=0;Hi=1;const auto D=B-A;for(int Axis=0;Axis<2;++Axis){const double P=Axis?A.Y:A.X,V=Axis?D.Y:D.X,Min=Axis?Box.Min.Y:Box.Min.X,Max=Axis?Box.Max.Y:Box.Max.X;
   if(FMath::Abs(V)<.000001){if(P<Min||P>Max)return false;continue;}
   double T0=(Min-P)/V,T1=(Max-P)/V;if(T0>T1)Swap(T0,T1);Lo=FMath::Max(Lo,T0);Hi=FMath::Min(Hi,T1);if(Lo>Hi)return false;
  }return true;
 }
}
TArray<FVector> FFoundrylineFactorySimulation::Path(FVector Start,FVector End,bool Pedestrian,bool Cart)const{
 TArray<FVector> Empty;if(Start.ContainsNaN()||End.ContainsNaN())return Empty;
 auto MobileClear=[&](FVector V){if(Cart)for(const auto& D:State.Drivers){const FVector Body=D.Position+FRotator(0,D.Yaw,0).RotateVector(FVector(-95,0,0));if(FVector::Dist2D(V,Body)<FMath::Min(220.f,float(FVector::Dist2D(Start,Body)))-.1f)return false;}if(Cart){auto ClearPerson=[&](FVector Person){const float Before=FVector::Dist2D(Start,Person);return Before<1||FVector::Dist2D(V,Person)>=FMath::Min(100.f,Before)-.01f;};for(const auto& O:State.Operators)if(!ClearPerson(O.Position))return false;for(const auto& W:State.Walkers)if(!ClearPerson(W.Position))return false;for(const auto& U:State.UnassignedEmployees)if(!ClearPerson(U.Position))return false;for(const auto& C:State.Collectors)if(!C.Position.Equals(Start,1)&&(!ClearPerson(C.Position)||!ClearPerson(C.Position-FRotator(0,C.Yaw,0).RotateVector(FVector(90,0,0)))))return false;}if(!Pedestrian)for(const auto& Other:State.Drivers)if(FVector::Dist2D(Start,Other.Position)>1&&FVector::Dist2D(End,Other.Position)>200&&FVector::Dist2D(V,Other.Position)<200)return false;return true;};
 auto PeopleClear=[&](FVector A,FVector B){if(Pedestrian)return true;auto Clear=[&](FVector Person){const float Before=FVector::Dist2D(A,Person),After=FVector::Dist2D(B,Person),Closest=FMath::PointDistToSegment(Person,A,B);return Closest>=140||(Before<140&&After>Before+.01&&Closest>=Before-.01);};for(const auto& O:State.Operators)if(!Clear(O.Position))return false;for(const auto& C:State.Collectors)if(!Clear(C.Position))return false;for(const auto& W:State.Walkers)if(!Clear(W.Position))return false;for(const auto& D:State.UnassignedEmployees)if(!Clear(D.Position))return false;return true;};
 auto PhysicalClear=[&](FVector A,FVector B,double Radius,bool LaneOnly){
  if(!PeopleClear(A,B))return false;
  if(Cart)for(const auto& Driver:State.Drivers){float Yaw=Driver.Yaw;const FVector Body=Driver.Position+FRotator(0,Yaw,0).RotateVector(FVector(-95,0,0));const FVector Ahead=Driver.Path.Num()>1&&!Driver.Waiting?Along(Driver.Path,FMath::Min(Length(Driver.Path),Driver.Distance+300),&Yaw)+FRotator(0,Yaw,0).RotateVector(FVector(-95,0,0)):Body;
   const int32 Samples=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(A,B)/25));FVector Previous=A;for(int32 Index=1;Index<=Samples;++Index){const FVector Point=FMath::Lerp(A,B,double(Index)/Samples);const float Before=FMath::PointDistToSegment(Previous,Body,Ahead),After=FMath::PointDistToSegment(Point,Body,Ahead);if(After<185&&After<=Before+.01)return false;Previous=Point;}}

  if(A.X<State.FloorMin.X+Radius||A.Y<State.FloorMin.Y+Radius||A.X>State.FloorMax.X-Radius||A.Y>State.FloorMax.Y-Radius||B.X<State.FloorMin.X+Radius||B.Y<State.FloorMin.Y+Radius||B.X>State.FloorMax.X-Radius||B.Y>State.FloorMax.Y-Radius)return false;
  double Lo,Hi;for(const auto& S:Solids)if(ClipRouteBox(FVector2D(A),FVector2D(B),S.ExpandBy(Radius),Lo,Hi))return false;
  for(const auto& Z:State.Zones){if(ClipRouteBox(FVector2D(A),FVector2D(B),FFoundrylineWorkZoneLayout::WelderBounds(Z).ExpandBy(Radius),Lo,Hi))return false;
   const auto R=FFoundrylineWorkZoneLayout::Rotation(Z);const auto C=FFoundrylineWorkZoneLayout::ProductCenter(Z);
   if(ClipRouteBox(FVector2D(R.UnrotateVector(A-C)),FVector2D(R.UnrotateVector(B-C)),FBox2D({-665-Radius,-130-Radius},{665+Radius,130+Radius}),Lo,Hi))return false;
   if(LaneOnly&&ClipRouteBox(FVector2D(A),FVector2D(B),FFoundrylineWorkZoneLayout::Bounds(Z).ExpandBy(Radius-.01),Lo,Hi))return false;
  }
  // Forklift docking spurs stop outside a welding station; nobody routes through its bench.
  for(const auto& S:State.WeldingStations){if(ClipRouteBox(FVector2D(A),FVector2D(B),FFoundrylineWeldingStationLayout::Bench(S).ExpandBy(Radius),Lo,Hi))return false;if(!Pedestrian&&ClipRouteBox(FVector2D(A),FVector2D(B),FFoundrylineWeldingStationLayout::Bounds(S).ExpandBy(Radius-.01),Lo,Hi))return false;}
  if(!Pedestrian)for(const auto& Other:State.Drivers)if(FVector::Dist2D(Start,Other.Position)>1&&FVector::Dist2D(End,Other.Position)>200&&FMath::PointDistToSegment(Other.Position,A,B)<200)return false;
  return true;
 };
 auto DirectionClear=[&](FVector A,FVector B){if(Pedestrian)return true;const FVector2D PA(A),PB(B),Delta=PB-PA;double Lo,Hi;
  for(const auto& L:State.Lanes)if(L.Kind==EFoundrylineFloorLane::Forklift&&L.Direction){const bool X=L.Max.X-L.Min.X>L.Max.Y-L.Min.Y;const double Move=X?Delta.X:Delta.Y;if(((L.Direction==1&&Move<-.001)||(L.Direction==2&&Move>.001))&&ClipRouteBox(PA,PB,FBox2D(L.Min,L.Max),Lo,Hi)&&Hi-Lo>.00001)return false;}return true;
 };
 auto LaneSegment=[&](FVector A,FVector B){
  if(!PhysicalClear(A,B,80,true)||!DirectionClear(A,B))return false;const FVector2D PA(A),PB(B),Delta=PB-PA;
  bool InSingleLane=false;double Lo,Hi;
  for(const auto& L:State.Lanes)if(L.Kind==EFoundrylineFloorLane::Forklift){
   const FBox2D Box(L.Min,L.Max),Core(L.Min+FVector2D(80),L.Max-FVector2D(80));
   if(Core.IsInsideOrOn(PA)&&Core.IsInsideOrOn(PB))InSingleLane=true;
   const bool X=L.Max.X-L.Min.X>L.Max.Y-L.Min.Y;const double Move=X?Delta.X:Delta.Y;
   if(L.Direction&&((L.Direction==1&&Move<-.001)||(L.Direction==2&&Move>.001))&&ClipRouteBox(PA,PB,Box,Lo,Hi)&&Hi-Lo>.00001)return false;
  }
  if(InSingleLane)return true;
  // All nine footprint samples need continuous lane coverage, including diagonals at concave corners.
  for(int X=-1;X<=1;++X)for(int Y=-1;Y<=1;++Y){const FVector2D Offset(X*80,Y*80);TArray<TPair<double,double>> Intervals;
   for(const auto& L:State.Lanes)if(L.Kind==EFoundrylineFloorLane::Forklift&&ClipRouteBox(PA+Offset,PB+Offset,FBox2D(L.Min,L.Max),Lo,Hi))Intervals.Emplace(Lo,Hi);
   Intervals.Sort([](const auto& U,const auto& V){return U.Key<V.Key;});double Covered=0;
   for(const auto& I:Intervals){if(I.Key>Covered+.000001)break;Covered=FMath::Max(Covered,I.Value);}if(Covered<1-.000001)return false;
  }return true;
 };
 auto ClearSpur=[&](FVector A,FVector B){return PhysicalClear(A,B,Cart?75:Pedestrian?30:65,false)&&DirectionClear(A,B);};
 TMap<FIntPoint,bool> NodeCache;
 auto NodeClear=[&](FIntPoint N){if(const auto* Cached=NodeCache.Find(N))return *Cached;const FVector V(N.X*50,N.Y*50,20);const bool Clear=MobileClear(V)&&(Pedestrian?Walkable(FVector2D(V),true,Cart):LaneSegment(V,V));NodeCache.Add(N,Clear);return Clear;};
 auto Nearest=[&](FVector P,FIntPoint& Out){float Best=FMath::Square(360.f);bool Found=false;const FIntPoint C(FMath::RoundToInt(P.X/50),FMath::RoundToInt(P.Y/50));
  for(int X=-8;X<=8;++X)for(int Y=-8;Y<=8;++Y){FIntPoint N=C+FIntPoint(X,Y);const FVector V(N.X*50,N.Y*50,20);const float D=FVector::DistSquared2D(P,V);if(D<Best&&NodeClear(N)&&ClearSpur(P,V)){Best=D;Out=N;Found=true;}}return Found;};
 FIntPoint A,B;if(!Nearest(Start,A)||!Nearest(End,B))return Empty;
 TArray<FIntPoint> Open={A};TMap<FIntPoint,float> Cost;TMap<FIntPoint,FIntPoint> From;TSet<FIntPoint> Closed;Cost.Add(A,0);
 const FIntPoint Dirs[]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
 auto Heuristic=[&](FIntPoint N){const int X=FMath::Abs(N.X-B.X),Y=FMath::Abs(N.Y-B.Y);return Pedestrian?float(X+Y):float(FMath::Max(X,Y)+.41421356237*FMath::Min(X,Y));};
 for(int Iter=0;Open.Num()&&Iter<8192;++Iter){
  int Pick=0;float Score=MAX_flt;for(int I=0;I<Open.Num();++I){const auto N=Open[I];const float F=Cost[N]+Heuristic(N);if(F<Score){Score=F;Pick=I;}}
  const FIntPoint N=Open[Pick];Open.RemoveAtSwap(Pick);if(Closed.Contains(N))continue;Closed.Add(N);
  if(N==B){TArray<FVector> P;FIntPoint At=B;P.Add(FVector(B.X*50,B.Y*50,20));while(At!=A){At=From[At];P.Add(FVector(At.X*50,At.Y*50,20));}Algo::Reverse(P);
   if(!Pedestrian){
    // Pull the path taut only through verified lane space; terminal docking spurs stay separate.
    TArray<FVector> Taut;Taut.Add(P[0]);for(int I=0;I<P.Num()-1;){int J=FMath::Min(I+24,P.Num()-1);for(;J>I+1;--J)if(LaneSegment(P[I],P[J]))break;Taut.Add(P[J]);I=J;}
    TArray<FVector> Smooth;Smooth.Add(Taut[0]);for(int I=1;I<Taut.Num()-1;++I){const auto In=(Taut[I]-Taut[I-1]).GetSafeNormal(),Out=(Taut[I+1]-Taut[I]).GetSafeNormal();bool Rounded=false;
     if(FVector::DotProduct(In,Out)>-.5&&FVector::DotProduct(In,Out)<.9999)for(float Trim:{120.f,90.f,60.f}){
      const float T=FMath::Min(Trim,FMath::Min(FVector::Dist(Taut[I-1],Taut[I]),FVector::Dist(Taut[I],Taut[I+1]))*.35f);if(T<30)continue;
      const FVector Enter=Taut[I]-In*T,Exit=Taut[I]+Out*T;TArray<FVector> Arc;Arc.Add(Enter);bool Clear=LaneSegment(Smooth.Last(),Enter);
      for(int K=1;K<=6&&Clear;++K){const double U=K/6.;const FVector V=FMath::Lerp(FMath::Lerp(Enter,Taut[I],U),FMath::Lerp(Taut[I],Exit,U),U);Clear=LaneSegment(Arc.Last(),V);Arc.Add(V);}
      if(Clear){Smooth.Append(Arc);Rounded=true;break;}
     }
     if(!Rounded)Smooth.Add(Taut[I]);
    }if(Taut.Num()>1)Smooth.Add(Taut.Last());P=MoveTemp(Smooth);
   }
   P.Insert(Start,0);P.Add(End);for(int I=P.Num()-1;I>0;--I)if(P[I].Equals(P[I-1],.01))P.RemoveAt(I);
   for(int I=P.Num()-2;I>0;--I)if((P[I]-P[I-1]).GetSafeNormal().Equals((P[I+1]-P[I]).GetSafeNormal(),.001))P.RemoveAt(I);return P;
  }
  for(int I=0;I<(Pedestrian?4:8);++I){const auto D=Dirs[I],Next=N+D;if(Closed.Contains(Next)||!NodeClear(Next))continue;
   if(Cart&&!PhysicalClear(FVector(N.X*50,N.Y*50,20),FVector(Next.X*50,Next.Y*50,20),75,false))continue;
   if(!Pedestrian&&!LaneSegment(FVector(N.X*50,N.Y*50,20),FVector(Next.X*50,Next.Y*50,20)))continue;
   const float G=Cost[N]+(D.X&&D.Y?1.41421356237f:1.f);if(!Cost.Contains(Next)||G<Cost[Next]){Cost.Add(Next,G);From.Add(Next,N);Open.Add(Next);}
  }
 }return Empty;
}
float FFoundrylineFactorySimulation::Length(const TArray<FVector>& P){float N=0;for(int I=1;I<P.Num();++I)N+=FVector::Dist(P[I-1],P[I]);return N;}
FVector FFoundrylineFactorySimulation::Along(const TArray<FVector>& P,float D,float* Yaw){if(P.IsEmpty())return FVector::ZeroVector;for(int I=1;I<P.Num();++I){float L=FVector::Dist(P[I-1],P[I]);if(D<=L){if(Yaw)*Yaw=(P[I]-P[I-1]).Rotation().Yaw;return FMath::Lerp(P[I-1],P[I],L>0?FMath::Clamp(D/L,0.f,1.f):0.f);}D-=L;}if(Yaw&&P.Num()>1)*Yaw=(P.Last()-P[P.Num()-2]).Rotation().Yaw;return P.Last();}
bool FFoundrylineFactorySimulation::PaintLane(EFoundrylineFloorLane Kind,FVector A,FVector B,float Width,int32 Direction,FString& Error){
 if(State.Lanes.Num()>=64){Error=TEXT("Floor route limit reached (64 sections)");return false;}
 if(A.ContainsNaN()||B.ContainsNaN()||!FMath::IsFinite(Width)||int(Kind)>2||Width<(Kind==EFoundrylineFloorLane::Forklift?300:100)||Width>610||Direction<0||Direction>2){Error=TEXT("Choose a valid lane width and direction");return false;}
 const bool Horizontal=FMath::Abs(A.X-B.X)>=FMath::Abs(A.Y-B.Y);
 FFoundrylineFactoryLane L;L.Id=State.NextId++;L.Kind=Kind;L.Direction=Direction;
 if(Kind==EFoundrylineFloorLane::Crossing){L.Min=FVector2D(A.X-100,A.Y-250);L.Max=FVector2D(A.X+100,A.Y+250);}
 else {if(Horizontal)B.Y=A.Y;else B.X=A.X;if(FVector::Dist2D(A,B)<200){Error=TEXT("Drag at least 6 feet 7 inches");return false;}
  L.Min=FVector2D(FMath::Min(A.X,B.X)-(Horizontal?0:Width/2),FMath::Min(A.Y,B.Y)-(Horizontal?Width/2:0));L.Max=FVector2D(FMath::Max(A.X,B.X)+(Horizontal?0:Width/2),FMath::Max(A.Y,B.Y)+(Horizontal?Width/2:0));}
 if(L.Min.X<State.FloorMin.X||L.Min.Y<State.FloorMin.Y||L.Max.X>State.FloorMax.X||L.Max.Y>State.FloorMax.Y){Error=TEXT("Paint lanes inside the shop floor");return false;}
 for(const auto& S:Solids)if(FFoundrylineFloorPlacement::Overlap(S,FBox2D(L.Min,L.Max))){Error=TEXT("Lane overlaps machinery or racking");return false;}
 for(const auto& Z:State.Zones)if(FFoundrylineFloorPlacement::Overlap(FFoundrylineWorkZoneLayout::Bounds(Z),FBox2D(L.Min,L.Max))){Error=TEXT("Lane overlaps a work zone; connect at its edge");return false;}
 if(OverlapsWeldingStation(FBox2D(L.Min,L.Max),false)){Error=TEXT("Lane overlaps a welding station; paint it along the station edge");return false;}
 State.Lanes.Add(L);for(auto& O:State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}for(auto& W:State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}++State.Revision;return true;
}
