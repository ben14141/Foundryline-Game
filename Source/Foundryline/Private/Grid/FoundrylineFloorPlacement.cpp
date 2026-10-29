#include "Grid/FoundrylineFloorPlacement.h"
bool FFoundrylineFloorPlacement::FloorPoint(FVector O,FVector D,FVector& Out){if(O.ContainsNaN()||D.ContainsNaN()||FMath::Abs(D.Z)<.000001)return false;const double T=(20-O.Z)/D.Z;if(T<0||T>500000)return false;Out=O+D*T;Out.Z=20;return true;}
bool FFoundrylineFloorPlacement::Overlap(const FBox2D& A,const FBox2D& B){return A.Min.X<B.Max.X-.01&&A.Max.X>B.Min.X+.01&&A.Min.Y<B.Max.Y-.01&&A.Max.Y>B.Min.Y+.01;}
FVector2D FFoundrylineFloorPlacement::Snap(FVector2D Raw,FVector2D Half,double Step,const FBox2D& Floor,const TArray<FBox2D>& Targets,double Gap,bool Magnet,bool& Snapped){
 Step=FMath::Max(Step,2.54);FVector2D P(FMath::GridSnap(Raw.X,Step),FMath::GridSnap(Raw.Y,Step));Snapped=false;if(!Magnet)return P;
 const double Tolerance=FMath::Clamp(Step*.8,12.,35.);double DX=Tolerance+.001,DY=DX;FVector2D Best=P;
 auto X=[&](double V){const double D=FMath::Abs(V-Raw.X);if(D<=Tolerance&&D<DX){DX=D;Best.X=V;}};
 auto Y=[&](double V){const double D=FMath::Abs(V-Raw.Y);if(D<=Tolerance&&D<DY){DY=D;Best.Y=V;}};
 X(Floor.Min.X+Half.X);X(Floor.Max.X-Half.X);Y(Floor.Min.Y+Half.Y);Y(Floor.Max.Y-Half.Y);
 for(const auto& B:Targets){
  if(Raw.Y+Half.Y>=B.Min.Y-Tolerance&&Raw.Y-Half.Y<=B.Max.Y+Tolerance){X(B.Min.X-Half.X-Gap);X(B.Max.X+Half.X+Gap);X(B.Min.X+Half.X);X(B.Max.X-Half.X);X(B.GetCenter().X);}
  if(Raw.X+Half.X>=B.Min.X-Tolerance&&Raw.X-Half.X<=B.Max.X+Tolerance){Y(B.Min.Y-Half.Y-Gap);Y(B.Max.Y+Half.Y+Gap);Y(B.Min.Y+Half.Y);Y(B.Max.Y-Half.Y);Y(B.GetCenter().Y);}
 }
 Snapped=DX<=Tolerance||DY<=Tolerance;return Best;
}
