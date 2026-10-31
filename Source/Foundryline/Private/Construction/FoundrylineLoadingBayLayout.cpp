#include "Construction/FoundrylineLoadingBayLayout.h"
FFoundrylineLoadingBayLayout::FSpec FFoundrylineLoadingBayLayout::Spec(uint8 Tier){
 switch(Tier){case 1:return {TEXT("SMALL FLATBED BAY"),762,1372,276,1053,253};case 2:return {TEXT("BOX TRUCK BAY"),914,1524,276,1200,200};case 3:return {TEXT("SEMI BAY"),1067,2438,295,2170,1570};default:return {TEXT("LEGACY LOADING DOOR"),500,330,255,0,1570};}
}
FVector FFoundrylineLoadingBayLayout::Point(const UFoundrylinePlaceableDefinition& D,const FFoundrylineOpening& O,float Across,float Distance,float Z){
 const float Along=O.OffsetCm+O.WidthCm()*.5f;const FVector Out=O.Edge==0?FVector(0,-1,0):O.Edge==1?FVector(1,0,0):O.Edge==2?FVector(0,1,0):FVector(-1,0,0),Side(-Out.Y,Out.X,0);const FVector Wall=O.Edge%2==0?FVector(Along,O.Edge==0?0:D.Footprint.Max.Y,0):FVector(O.Edge==3?0:D.Footprint.Max.X,Along,0);return Wall+Side*Across+Out*Distance+FVector(0,0,Z);
}
FFoundrylineGridBounds FFoundrylineLoadingBayLayout::Bounds(const UFoundrylinePlaceableDefinition& D,const FFoundrylinePlacedObject& Object,const FFoundrylineOpening& O){
 const auto S=Spec(O.LoadingTier);FBox B(ForceInit);for(float X:{-S.Width*.5f,S.Width*.5f})for(float Y:{0.f,S.Depth})B+=Point(D,O,X,Y);FFoundrylineGridBounds Local;Local.Min={FMath::FloorToInt32(B.Min.X),FMath::FloorToInt32(B.Min.Y),0};Local.Max={FMath::CeilToInt32(B.Max.X),FMath::CeilToInt32(B.Max.Y),600};return FFoundrylineGrid::RotateBounds(Local,Object.Anchor,Object.QuarterTurns);
}
