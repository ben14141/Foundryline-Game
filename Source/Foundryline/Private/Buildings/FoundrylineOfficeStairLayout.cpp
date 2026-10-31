#include "Buildings/FoundrylineOfficeStairLayout.h"

FVector FFoundrylineOfficeStairLayout::Point(FIntPoint Origin,int32 Quarter,FVector P){return FVector(Origin.X,Origin.Y,0)+FRotator(0,Quarter*90,0).RotateVector(P);}
namespace {
FBox2D Rectangle(FIntPoint Origin,int32 Q,FVector2D Low,FVector2D High){FBox2D B(ForceInit);for(int X:{0,1})for(int Y:{0,1})B+=FVector2D(FFoundrylineOfficeStairLayout::Point(Origin,Q,{X?High.X:Low.X,Y?High.Y:Low.Y,0}));return B;}
}
FBox2D FFoundrylineOfficeStairLayout::FullBounds(FIntPoint O,int32 Q,uint8 Style){return Rectangle(O,Q,{0,0},Style?FVector2D(136,764):FVector2D(Width,Depth));}
FBox2D FFoundrylineOfficeStairLayout::HoleBounds(FIntPoint O,int32 Q,uint8 Style){return Rectangle(O,Q,{0,LandingDepth},Style?FVector2D(136,634):FVector2D(Width,Depth));}
TArray<FBox2D> FFoundrylineOfficeStairLayout::FloorPanels(double W,double D,FIntPoint O,int32 Q,uint8 Style){
 TArray<FBox2D> Out;if(W<=0||D<=0)return Out;const FBox2D Full({0,0},{W,D}),Cut=HoleBounds(O,Q,Style);const FVector2D Low=Full.Min.ComponentMax(Cut.Min),High=Full.Max.ComponentMin(Cut.Max);
 if(Low.X>=High.X||Low.Y>=High.Y){Out.Add(Full);return Out;}
 auto Add=[&](FVector2D A,FVector2D B){if(B.X-A.X>.01&&B.Y-A.Y>.01)Out.Add({A,B});};
 Add(Full.Min,{Low.X,D});Add({High.X,0},Full.Max);Add({Low.X,0},{High.X,Low.Y});Add({Low.X,High.Y},{High.X,D});return Out;
}
TArray<FVector> FFoundrylineOfficeStairLayout::WalkRoute(FIntPoint O,int32 Q,uint8 Style){
 if(Style){TArray<FVector> Straight;for(const auto P:TArray<FVector>{{68,65,20},{68,130,20},{68,382,180},{68,634,340},{68,699,340}})Straight.Add(Point(O,Q,P));return Straight;}
 TArray<FVector> Out;for(const FVector P:TArray<FVector>{{68,65,20},{68,130,20},{68,447,180},{232,447,180},{232,382,180},{232,65,340}})Out.Add(Point(O,Q,P));return Out;
}
