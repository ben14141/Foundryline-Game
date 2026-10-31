#include "Construction/FoundrylinePavementGeometry.h"
#include "Curve/PolygonIntersectionUtils.h"
#include "Curve/PolygonOffsetUtils.h"
#include "CompGeom/Delaunay2.h"
#include "Algo/Reverse.h"

namespace {
using namespace UE::Geometry;using G=FFoundrylinePavementGeometry;using Poly=G::FPolygon;
FGeneralPolygon2d Native(const Poly& P){FPolygon2d Outer(P.Outer);if(Outer.IsClockwise())Outer.Reverse();FGeneralPolygon2d V(Outer);for(const auto& H:P.Holes){FPolygon2d Hole(H.Points);if(!Hole.IsClockwise())Hole.Reverse();V.AddHole(Hole,false,false);}return V;}
TArray<FGeneralPolygon2d> Native(const TArray<Poly>& P){TArray<FGeneralPolygon2d> Out;for(const auto& V:P)Out.Add(Native(V));return Out;}
TArray<Poly> Records(const TArray<FGeneralPolygon2d>& In){TArray<Poly> Out;for(const auto& P:In){if(P.SignedArea()<25)continue;Poly V;auto Outer=P.GetOuter();if(Outer.IsClockwise())Outer.Reverse();Outer.Simplify(.05,.05);V.Outer=Outer.GetVertices();for(const auto& H:P.GetHoles()){FFoundrylinePavementLoop L;auto Hole=H;if(!Hole.IsClockwise())Hole.Reverse();Hole.Simplify(.05,.05);L.Points=Hole.GetVertices();V.Holes.Add(L);}Out.Add(V);}return Out;}
double Cross(FVector2D A,FVector2D B){return A.X*B.Y-A.Y*B.X;}
bool LoopValid(const TArray<FVector2D>& P){
 if(P.Num()<3||P.Num()>1536)return false;
 for(int I=0;I<P.Num();++I){if(P[I].ContainsNaN()||P[I].GetAbsMax()>1000000||FVector2D::Distance(P[I],P[(I+1)%P.Num()])<.0001)return false;
  const auto A=P[I],B=P[(I+1)%P.Num()];for(int J=I+2;J<P.Num();++J){if(I==0&&J==P.Num()-1)continue;const auto C=P[J],D=P[(J+1)%P.Num()];const auto U=B-A,V=D-C;const double Det=Cross(U,V);if(FMath::Abs(Det)<1.e-8)continue;const double T=Cross(C-A,V)/Det,S=Cross(C-A,U)/Det;if(T>1.e-6&&T<1-1.e-6&&S>1.e-6&&S<1-1.e-6)return false;}}
 return FMath::Abs(FPolygon2d(P).SignedArea())>=25;
}
}
TArray<G::FPolygon> G::Rectangle(FVector2D A,FVector2D B){FPolygon P;const auto Min=A.ComponentMin(B),Max=A.ComponentMax(B);P.Outer={Min,{Max.X,Min.Y},Max,{Min.X,Max.Y}};return {P};}
TArray<FVector2D> G::RoadPath(const TArray<FVector2D>& C,bool Spline){
 if(Spline){
  if(C.Num()<2||C.Num()>32)return {};if(C.Num()==2)return C;TArray<FVector2D> Out;Out.Add(C[0]);const int Budget=FMath::Clamp(512/(C.Num()-1),4,64);
  for(int Segment=0;Segment<C.Num()-1;++Segment){const auto P1=C[Segment],P2=C[Segment+1],P0=Segment?C[Segment-1]:P1*2-P2,P3=Segment+2<C.Num()?C[Segment+2]:P2*2-P1;
   auto Span=[](FVector2D A,FVector2D B){return FMath::Sqrt(FMath::Max(1.,FVector2D::Distance(A,B)));};const double T0=0,T1=Span(P0,P1),T2=T1+Span(P1,P2),T3=T2+Span(P2,P3);const int Steps=FMath::Clamp(FMath::CeilToInt(FVector2D::Distance(P1,P2)/75),4,Budget);
   auto Mix=[](FVector2D A,FVector2D B,double ATime,double BTime,double Time){return A*((BTime-Time)/(BTime-ATime))+B*((Time-ATime)/(BTime-ATime));};
   for(int I=1;I<=Steps;++I){const double T=FMath::Lerp(T1,T2,double(I)/Steps);const auto A1=Mix(P0,P1,T0,T1,T),A2=Mix(P1,P2,T1,T2,T),A3=Mix(P2,P3,T2,T3,T),B1=Mix(A1,A2,T0,T2,T),B2=Mix(A2,A3,T1,T3,T);Out.Add(I==Steps?P2:Mix(B1,B2,T1,T2,T));}
  }
  Out[1]=C[0]+(C[1]-C[0]).GetSafeNormal()*FVector2D::Distance(Out[1],C[0]);Out[Out.Num()-2]=C.Last()-(C.Last()-C[C.Num()-2]).GetSafeNormal()*FVector2D::Distance(Out[Out.Num()-2],C.Last());return Out;
 }
 if(C.Num()<2||C.Num()>3)return {};if(C.Num()==2)return C;
 const double Length=FVector2D::Distance(C[0],C[1])+FVector2D::Distance(C[1],C[2]);const int N=FMath::Clamp(FMath::CeilToInt(Length/75),12,128);TArray<FVector2D> Out;
 for(int I=0;I<=N;++I){const double T=double(I)/N;Out.Add((1-T)*(1-T)*C[0]+2*(1-T)*T*C[1]+T*T*C[2]);}
 // Preserve exact endpoint tangents so butt caps meet street boundaries without overshoot.
 const auto Start=(C[1]-C[0]).GetSafeNormal(),End=(C[2]-C[1]).GetSafeNormal();
 if(!Start.IsNearlyZero())Out[1]=C[0]+Start*FVector2D::Distance(C[0],Out[1]);
 if(!End.IsNearlyZero())Out[N-1]=C[2]-End*FVector2D::Distance(C[2],Out[N-1]);return Out;
}
TArray<G::FPolygon> G::Road(const TArray<FVector2D>& C,double Width,bool Spline){
 auto P=RoadPath(C,Spline);if(P.Num()<2||Width<100)return {};TArray<TArrayView<FVector2d>> Paths;Paths.Add(MakeArrayView(P));FOffsetPolygon2d Op(Paths);// The bundled Clipper open-path offset takes the full stroke width.
 Op.Offset=Width;Op.JoinType=EPolygonOffsetJoinType::Miter;Op.EndType=EPolygonOffsetEndType::Butt;Op.MaxStepsPerRadian=12;return Op.ComputeResult()?Records(Op.Result):TArray<FPolygon>();
}
TArray<G::FPolygon> G::RoadSurface(const FFoundrylinePavementData& D){
 const double W=D.RoadWidthCm+(D.Curbs?36:0),R=D.EntranceRadiusCm;const auto Base=Road(D.RoadControls,W,D.Spline);auto Out=Base;if(R<=0||!D.EntranceEnds||D.RoadControls.Num()<2)return Out;
 const auto Path=RoadPath(D.RoadControls,D.Spline);
 for(int End=0;End<2;++End)if(D.EntranceEnds&(1<<End)){
  const auto Origin=End?Path.Last():Path[0],F=End?(Path[Path.Num()-2]-Origin).GetSafeNormal():(Path[1]-Origin).GetSafeNormal(),N=FVector2D(-F.Y,F.X);
  for(int Side:{-1,1}){
   const TArray<FVector2D>* EdgeLoop=nullptr;int First=-1;double Closest=1.;const auto Expected=Origin+N*(Side*W/2);
   for(const auto& P:Base)for(int I=0;I<P.Outer.Num();++I){const double Distance=FVector2D::Distance(P.Outer[I],Expected);if(Distance<Closest){Closest=Distance;First=I;EdgeLoop=&P.Outer;}}
   if(!EdgeLoop)return {};const auto& Loop=*EdgeLoop;const int Direction=FVector2D::DotProduct(Loop[(First+1)%Loop.Num()]-Origin,F)>FVector2D::DotProduct(Loop[(First+Loop.Num()-1)%Loop.Num()]-Origin,F)?1:-1;
   auto Local=[&](FVector2D P){const auto V=P-Origin;return FVector2D(FVector2D::DotProduct(V,F),Side*FVector2D::DotProduct(V,N));};auto World=[&](FVector2D P){return Origin+F*P.X+N*(Side*P.Y);};
   const auto Overlap=N*(Side*.2);FPolygon Patch;Patch.Outer.Add(Loop[First]-Overlap);bool Found=false;
   for(int Step=0;Step<Loop.Num()-1;++Step){const int AIndex=(First+Direction*Step+Loop.Num()*2)%Loop.Num(),BIndex=(AIndex+Direction+Loop.Num())%Loop.Num();const auto A=Local(Loop[AIndex]),B=Local(Loop[BIndex]),Delta=B-A,E=Delta.GetSafeNormal();
    if(Delta.X>.001){const FVector2D Normal(-E.Y,E.X);const double Qs=R-Normal.X*R,T=(Qs-A.X)/Delta.X;
     if(T>=-.00001&&T<=1.00001){const auto Q=FMath::Lerp(A,B,FMath::Clamp(T,0.,1.)),Center=Q+Normal*R;Patch.Outer.Add(World(Q)-Overlap);Patch.Outer.Add(World(Q));const double From=FMath::Atan2(Q.Y-Center.Y,Q.X-Center.X),To=PI,Angle=FMath::FindDeltaAngleRadians(From,To);const int Steps=FMath::Clamp(FMath::CeilToInt(FMath::Abs(Angle)*R/35),8,32);
      for(int I=1;I<=Steps;++I){const double Theta=From+Angle*I/Steps;Patch.Outer.Add(World(Center+FVector2D(FMath::Cos(Theta),FMath::Sin(Theta))*R));}Found=true;break;
     }
    }
    Patch.Outer.Add(Loop[BIndex]-Overlap);if(B.X>R*3||B.X<-.1)break;
   }
   if(!Found)return {};TArray<FPolygon> Combined;if(!Boolean(Out,{Patch},false,Combined))return {};Out=MoveTemp(Combined);
  }
 }return Out;
}
bool G::InEntrance(const FFoundrylinePavementData& D,FVector2D P){
 if(!D.EntranceEnds||D.EntranceRadiusCm<=0||D.RoadControls.Num()<2)return false;const auto& C=D.RoadControls;
 for(int End=0;End<2;++End)if(D.EntranceEnds&(1<<End)){const auto Origin=End?C.Last():C[0],F=End?(C[C.Num()-2]-Origin).GetSafeNormal():(C[1]-Origin).GetSafeNormal();const double Along=FVector2D::DotProduct(P-Origin,F);if(Along>=-1&&Along<=D.EntranceRadiusCm+100)return true;}return false;
}
bool G::Boolean(const TArray<FPolygon>& A,const TArray<FPolygon>& B,bool Subtract,TArray<FPolygon>& Out){auto NA=Native(A),NB=Native(B);TArray<FGeneralPolygon2d> Result;bool Ok;if(Subtract)Ok=PolygonsDifference(NA,NB,Result);else{NA.Append(NB);Ok=PolygonsUnion(NA,Result,false);}if(Ok)Out=Records(Result);return Ok;}
FBox2D G::Bounds(const TArray<FPolygon>& P){FBox2D B(ForceInit);for(const auto& V:P)for(const auto X:V.Outer)B+=X;return B;}
double G::Area(const TArray<FPolygon>& P){double A=0;for(const auto& V:P)A+=Native(V).SignedArea();return A;}
bool G::Contains(const TArray<FPolygon>& Polygons,FVector2D P,double Tol){
 auto Inside=[&](const TArray<FVector2D>& Loop){bool In=false;for(int I=0,J=Loop.Num()-1;I<Loop.Num();J=I++){const auto A=Loop[I],B=Loop[J];if((A.Y>P.Y)!=(B.Y>P.Y)&&P.X<(B.X-A.X)*(P.Y-A.Y)/(B.Y-A.Y)+A.X)In=!In;}return In;};
 auto Close=[&](const TArray<FVector2D>& Loop){if(Tol<=0)return false;for(int I=0;I<Loop.Num();++I){const auto A=Loop[I],B=Loop[(I+1)%Loop.Num()],D=B-A;const double T=FMath::Clamp(FVector2D::DotProduct(P-A,D)/FMath::Max(D.SizeSquared(),.0001),0.,1.);if(FVector2D::DistSquared(P,A+D*T)<=Tol*Tol)return true;}return false;};
 for(const auto& V:Polygons){if(Close(V.Outer))return true;if(!Inside(V.Outer))continue;bool Hole=false;for(const auto& H:V.Holes){if(Close(H.Points))return true;if(Inside(H.Points)){Hole=true;break;}}if(!Hole)return true;}return false;
}
bool G::Intersects(const TArray<FPolygon>& A,const TArray<FPolygon>& B){TArray<FGeneralPolygon2d> Out;return PolygonsIntersection(Native(A),Native(B),Out)&&!Out.IsEmpty()&&Records(Out).Num()>0;}
bool G::Triangulate(const FPolygon& P,TArray<FVector2D>& V,TArray<int32>& T){FDelaunay2 D;D.bAutomaticallyFixEdgesToDuplicateVertices=true;TArray<FIndex3i> Tri;if(!D.Triangulate(Native(P),&Tri,&V))return false;for(auto A:Tri){if(Cross(V[A.B]-V[A.A],V[A.C]-V[A.A])<0)Swap(A.B,A.C);T.Append({A.A,A.B,A.C});}return !Tri.IsEmpty();}
TArray<G::FPolygon> G::Transform(const TArray<FPolygon>& In,const FTransform& T){auto Out=In;for(auto& P:Out){auto Apply=[&](auto& Points){for(auto& V:Points)V=FVector2D(T.TransformPosition(FVector(V,0)));};Apply(P.Outer);for(auto& H:P.Holes)Apply(H.Points);}return Out;}
void G::Translate(FFoundrylinePavementData& D,FVector2D Delta){D.Polygons=Transform(D.Polygons,FTransform(FVector(Delta,0)));for(auto& P:D.RoadControls)P+=Delta;for(auto& M:D.Markings)for(auto& P:M.Points)P+=Delta;}
double G::MinimumWidth(int L,bool Truck){return (L==4?48:L==2?(Truck?30:24):(Truck?16:12))*30.48;}
bool G::RoundCorner(TArray<FPolygon>& Ps,FVector2D Near,double Radius){
 int PolygonIndex=-1,VI=-1;double Best=400;for(int P=0;P<Ps.Num();++P)for(int I=0;I<Ps[P].Outer.Num();++I){const double D=FVector2D::Distance(Near,Ps[P].Outer[I]);if(D<Best){Best=D;PolygonIndex=P;VI=I;}}if(PolygonIndex<0)return false;
 auto& Loop=Ps[PolygonIndex].Outer;const auto B=Loop[VI],A=Loop[(VI+Loop.Num()-1)%Loop.Num()],C=Loop[(VI+1)%Loop.Num()];const auto U=(A-B).GetSafeNormal(),V=(C-B).GetSafeNormal();const double Angle=FMath::Acos(FMath::Clamp(FVector2D::DotProduct(U,V),-.999999,.999999));
 if(Angle<.1||Angle>PI-.1)return false;double T=Radius/FMath::Tan(Angle/2);T=FMath::Clamp(T,10.,FMath::Min(FVector2D::Distance(A,B),FVector2D::Distance(C,B))*.45);Radius=T*FMath::Tan(Angle/2);
 const auto Center=B+(U+V).GetSafeNormal()*(Radius/FMath::Sin(Angle/2)),Start=B+U*T,End=B+V*T;const double From=FMath::Atan2(Start.Y-Center.Y,Start.X-Center.X),To=FMath::Atan2(End.Y-Center.Y,End.X-Center.X),Delta=FMath::FindDeltaAngleRadians(From,To);
 TArray<FVector2D> Arc;const int Steps=FMath::Clamp(FMath::CeilToInt(FMath::Abs(Delta)*Radius/40),4,32);for(int I=0;I<=Steps;++I){const double Ang=From+Delta*I/Steps;Arc.Add(Center+FVector2D(FMath::Cos(Ang),FMath::Sin(Ang))*Radius);}Loop.RemoveAt(VI);Loop.Insert(Arc,VI);return LoopValid(Loop);
}
bool G::Valid(const FFoundrylinePavementData& D,FVector2D Size,FString& E){
 if(D.Polygons.IsEmpty()||D.Polygons.Num()>32||D.Markings.Num()>128){E=TEXT("Pavement is empty or too complex");return false;}int Count=0;
 for(const auto& P:D.Polygons){if(P.Outer.Num()>(D.Spline?1536:512)||!LoopValid(P.Outer)||FPolygon2d(P.Outer).IsClockwise()||P.Holes.Num()>16){E=FString::Printf(TEXT("Invalid pavement outline (%d corners)"),P.Outer.Num());return false;}Count+=P.Outer.Num();const auto N=Native(P);for(const auto& H:P.Holes){if(H.Points.Num()>512||!LoopValid(H.Points)||!FPolygon2d(H.Points).IsClockwise()||!FPolygon2d(P.Outer).Contains(FPolygon2d(H.Points))){E=TEXT("Invalid pavement cutout");return false;}Count+=H.Points.Num();}for(const auto V:P.Outer)if(V.X<-.1||V.Y<-.1||V.X>Size.X+.1||V.Y>Size.Y+.1){E=TEXT("Pavement extends outside its bounds");return false;}TArray<FVector2D> V;TArray<int32> T;if(!Triangulate(P,V,T)){E=TEXT("Cannot triangulate pavement");return false;}}
 if(Count>4096||Area(D.Polygons)<10000){E=TEXT("Pavement must cover at least 11 square feet");return false;}
 if(!FMath::IsFinite(D.EntranceRadiusCm)||D.EntranceRadiusCm<0||D.EntranceRadiusCm>1524||D.EntranceEnds>3){E=TEXT("Entrance radius must be between 0 and 50 ft");return false;}
 if(D.Lanes){if(D.Lanes!=1&&D.Lanes!=2&&D.Lanes!=4||!FMath::IsFinite(D.RoadWidthCm)||D.RoadWidthCm+ .01<MinimumWidth(D.Lanes,D.TruckAccess)||D.RoadWidthCm>2400||D.RoadControls.Num()<2||D.RoadControls.Num()>(D.Spline?32:3)){E=TEXT("Road width is below the selected lane minimum");return false;}if(D.Spline)for(int I=1;I<D.RoadControls.Num();++I)if(FVector2D::Distance(D.RoadControls[I-1],D.RoadControls[I])<100){E=TEXT("Space spline points at least 3 ft 4 in apart");return false;}for(const auto P:D.RoadControls)if(P.ContainsNaN()||P.GetAbsMax()>30000){E=TEXT("Invalid road control point");return false;}double Length=0;const auto Path=RoadPath(D.RoadControls,D.Spline);for(int I=1;I<Path.Num();++I)Length+=FVector2D::Distance(Path[I-1],Path[I]);if(D.EntranceEnds&&D.EntranceRadiusCm>Length*.45+.1){E=TEXT("Extend the road or reduce its entrance radius");return false;}const auto Expected=RoadSurface(D);TArray<FPolygon> Diff;if(!Boolean(Expected,D.Polygons,true,Diff)||Area(Diff)>20||!Boolean(D.Polygons,Expected,true,Diff)||Area(Diff)>20){E=TEXT("Road surface must match its width and curve");return false;}}
 else if(!D.RoadControls.IsEmpty()||D.RoadWidthCm!=0||D.EntranceRadiusCm!=0||D.EntranceEnds!=0||D.Spline){E=TEXT("Area pavement has unexpected road controls");return false;}
 TSet<FGuid> Ids;for(const auto& M:D.Markings){if(!M.Id.IsValid()||Ids.Contains(M.Id)||int(M.Kind)>int(EFoundrylinePavementMark::Stop)||M.Points.Num()<2||M.Points.Num()>(D.Spline&&M.RoadGenerated?513:129)||!FMath::IsFinite(M.WidthCm)||M.WidthCm<10||M.WidthCm>3000){E=TEXT("Invalid pavement marking");return false;}Ids.Add(M.Id);for(const auto V:M.Points)if(V.ContainsNaN()||V.GetAbsMax()>30000){E=TEXT("Invalid paint coordinates");return false;}}
 return true;
}
FString G::PaintName(EFoundrylinePavementMark Kind){
 const TCHAR* Names[]={TEXT("Parking row"),TEXT("White line"),TEXT("Yellow line"),TEXT("Dashed line"),TEXT("Direction arrow"),TEXT("Loading bay"),TEXT("Keep clear"),TEXT("Stop line")};return int(Kind)<UE_ARRAY_COUNT(Names)?Names[int(Kind)]:TEXT("Paint");
}
FFoundrylinePavementMarking G::PaintDraft(EFoundrylinePavementMark Kind,FVector2D A,FVector2D B,float Depth,bool Flip,bool AngleSnap){
 FFoundrylinePavementMarking M;M.Kind=Kind;M.WidthCm=10.16f;
 if(Kind==EFoundrylinePavementMark::Loading||Kind==EFoundrylinePavementMark::KeepClear){const auto Min=A.ComponentMin(B),Max=A.ComponentMax(B);M.Points={Min,{Max.X,Min.Y}};M.WidthCm=Max.Y-Min.Y;return M;}
 auto Delta=B-A;const double Length=Delta.Size();if(AngleSnap&&Length>.01){const double Angle=FMath::RoundToDouble(FMath::Atan2(Delta.Y,Delta.X)/(PI/4))*(PI/4);Delta=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Length;}
 if(Kind==EFoundrylinePavementMark::Parking){const int Count=FMath::Clamp(FMath::FloorToInt((Length+.01)/274.32),1,64);Delta=Delta.GetSafeNormal()*(Count*274.32);M.WidthCm=Depth;}
 M.Points={A,A+Delta};if(Flip)Algo::Reverse(M.Points);return M;
}
TArray<TPair<FVector2D,FVector2D>> G::PaintLines(const FFoundrylinePavementMarking& M){
 TArray<TPair<FVector2D,FVector2D>> Out;if(M.Points.Num()<2)return Out;const auto A=M.Points[0],B=M.Points.Last(),Forward=(B-A).GetSafeNormal(),Right=FVector2D(-Forward.Y,Forward.X);const double Length=FVector2D::Distance(A,B);
 auto Line=[&](FVector2D P,FVector2D Q){if(FVector2D::Distance(P,Q)>.1)Out.Emplace(P,Q);};
 if(M.Kind==EFoundrylinePavementMark::Parking){const int N=FMath::Clamp(FMath::FloorToInt(Length/274.32),1,64);for(int I=0;I<=N;++I){const auto P=A+Forward*(I*274.32);Line(P,P+Right*M.WidthCm);}Line(A,A+Forward*(N*274.32));}
 else if(M.Kind==EFoundrylinePavementMark::Loading||M.Kind==EFoundrylinePavementMark::KeepClear){Line(A,B);Line(B,B+Right*M.WidthCm);Line(B+Right*M.WidthCm,A+Right*M.WidthCm);Line(A+Right*M.WidthCm,A);if(M.Kind==EFoundrylinePavementMark::KeepClear){for(double X=-M.WidthCm;X<Length;X+=90){const double X0=FMath::Max(0.,X),X1=FMath::Min(Length,X+M.WidthCm);Line(A+Forward*X0+Right*(X0-X),A+Forward*X1+Right*(X1-X));}}}
 else if(M.Kind==EFoundrylinePavementMark::Arrow){const auto F=(B-A).GetSafeNormal();const double L=FMath::Min(Length,400.);const auto Tip=A+F*L;Line(A,Tip);Line(Tip,Tip-F*90+Right*65);Line(Tip,Tip-F*90-Right*65);}
 else for(int I=1;I<M.Points.Num();++I)Line(M.Points[I-1],M.Points[I]);return Out;
}
bool G::MarkingFits(const FFoundrylinePavementData& D,const FFoundrylinePavementMarking& M){for(const auto& L:PaintLines(M)){const int N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(L.Key,L.Value)/40));for(int I=0;I<=N;++I)if(!Contains(D.Polygons,FMath::Lerp(L.Key,L.Value,double(I)/N),1))return false;}return true;}

void G::RoadPaint(FFoundrylinePavementData& D,bool Force){
 TArray<FGuid> Existing;for(const auto& M:D.Markings)if(M.RoadGenerated)Existing.Add(M.Id);if(!Force&&Existing.IsEmpty())return;D.Markings.RemoveAll([](const auto& M){return M.RoadGenerated;});if(!D.Lanes)return;
 const auto Path=RoadPath(D.RoadControls,D.Spline);int Index=0;auto Add=[&](double Offset,EFoundrylinePavementMark Kind){FFoundrylinePavementMarking M;M.Id=Existing.IsValidIndex(Index)?Existing[Index]:FGuid::NewGuid();++Index;M.Kind=Kind;M.WidthCm=10.16f;M.RoadGenerated=true;for(int I=0;I<Path.Num();++I){const auto F=(Path[FMath::Min(I+1,Path.Num()-1)]-Path[FMath::Max(0,I-1)]).GetSafeNormal();M.Points.Add(Path[I]+FVector2D(-F.Y,F.X)*Offset);}D.Markings.Add(M);};
 Add(-D.RoadWidthCm/2+12,EFoundrylinePavementMark::WhiteLine);Add(D.RoadWidthCm/2-12,EFoundrylinePavementMark::WhiteLine);
 if(D.Lanes>1){Add(-8,EFoundrylinePavementMark::YellowLine);Add(8,EFoundrylinePavementMark::YellowLine);}if(D.Lanes==4){Add(-D.RoadWidthCm/4,EFoundrylinePavementMark::DashedLine);Add(D.RoadWidthCm/4,EFoundrylinePavementMark::DashedLine);}
}

TArray<G::FEditHandle> G::EditHandles(const FFoundrylinePavementData& D){
 TArray<FEditHandle> Out;
 if(D.Spline&&D.Lanes){for(int I=0;I<D.RoadControls.Num();++I)Out.Add({D.RoadControls[I],TEXT("Resize"),I==0?TEXT("Drag spline start"):I==D.RoadControls.Num()-1?TEXT("Drag spline end"):TEXT("Drag spline point"),I==0||I==D.RoadControls.Num()-1});return Out;}
 if(D.Lanes&&D.RoadControls.Num()>=2){
  Out.Add({D.RoadControls[0],TEXT("Resize"),TEXT("Drag road start"),true});
  Out.Add({D.RoadControls.Last(),TEXT("Resize"),TEXT("Drag road end"),true});
  Out.Add({D.RoadControls.Num()==3?D.RoadControls[1]:(D.RoadControls[0]+D.RoadControls.Last())/2,TEXT("PaveBend"),TEXT("Drag to bend road"),false});return Out;
 }
 for(const auto& P:D.Polygons)for(int I=0;I<P.Outer.Num();++I){
  const auto A=P.Outer[I],B=P.Outer[(I+1)%P.Outer.Num()],Before=P.Outer[(I+P.Outer.Num()-1)%P.Outer.Num()];
  // Keep rounded outlines legible: tiny arc samples are not separate handles.
  if(P.Outer.Num()<=12||FVector2D::DotProduct((A-Before).GetSafeNormal(),(B-A).GetSafeNormal())<.92)Out.Add({A,TEXT("Resize"),TEXT("Drag corner"),true});
  if(FVector2D::Distance(A,B)>250)Out.Add({(A+B)/2,TEXT("Resize"),TEXT("Drag edge"),false});
  if(Out.Num()>=64)return Out;
 }return Out;
}
