#include "Production/FoundrylineWorkZoneLayout.h"

namespace {double SupplyGap(int32 Preset){return Preset==2?40.:200.;}}
const TCHAR* FFoundrylineWorkZoneLayout::PresetName(int32 P){return P==0?TEXT("Compact"):P==1?TEXT("Standard"):P==2?TEXT("Tanker bay"):TEXT("Existing layout");}
FVector2D FFoundrylineWorkZoneLayout::WorkSize(int32 P){return (P==0?FVector2D(66,34):P==1?FVector2D(72,40):FVector2D(60,16))*30.48;}
FVector2D FFoundrylineWorkZoneLayout::TotalSize(int32 P){auto S=WorkSize(P);S.Y+=(P==2?14:20)*30.48;return S;}
FVector FFoundrylineWorkZoneLayout::WorkCenter(const FFoundrylineFactoryZone& Z){if(Z.Preset<0)return {(Z.Min.X+Z.Max.X)/2,(Z.Min.Y+Z.DropMin.Y-200)/2,20};const auto Work=WorkSize(Z.Preset),Total=TotalSize(Z.Preset);return (Z.Min+Z.Max)/2+Rotation(Z).RotateVector({0,-Z.SupplySide*(Total.Y-Work.Y)/2,0});}
FVector FFoundrylineWorkZoneLayout::Point(const FFoundrylineFactoryZone& Z,FVector Local){return WorkCenter(Z)+Rotation(Z).RotateVector(Local);}
FVector FFoundrylineWorkZoneLayout::ProductCenter(const FFoundrylineFactoryZone& Z){return Point(Z,{Z.Preset==2?WorkSize(2).X/2-715:0,0,0});}
FVector FFoundrylineWorkZoneLayout::WelderPosition(const FFoundrylineFactoryZone& Z){return ProductCenter(Z)+Rotation(Z).RotateVector({-850,-Z.SupplySide*40.,0});}
FBox2D FFoundrylineWorkZoneLayout::WelderBounds(const FFoundrylineFactoryZone& Z){const FVector2D P(WelderPosition(Z)),Half=Z.QuarterTurns%2?FVector2D(40,65):FVector2D(65,40);return {P-Half,P+Half};}
FVector FFoundrylineWorkZoneLayout::WeldPoint(const FFoundrylineFactoryZone& Z){return WorkPoint(Z)+Rotation(Z).RotateVector({0,Z.SupplySide*70.,125});}
FBox2D FFoundrylineWorkZoneLayout::Bounds(const FFoundrylineFactoryZone& Z){return FBox2D(FVector2D(Z.Min.ComponentMin(Z.DropMin)),FVector2D(Z.Max.ComponentMax(Z.DropMax)));}
bool FFoundrylineWorkZoneLayout::Create(FVector Center,int32 P,int32 Q,int32 Side,FFoundrylineFactoryZone& Z){
 if(Center.ContainsNaN()||P<0||P>2||Q<0||Q>3||(Side!=1&&Side!=-1))return false;
 Z=FFoundrylineFactoryZone();Z.Preset=P;Z.QuarterTurns=Q;Z.SupplySide=Side;
 const auto Work=WorkSize(P),Total=TotalSize(P);const double Strip=Total.Y-Work.Y;const auto R=Rotation(Z);
 Center.Z=20;const FVector WorkCenter=Center+R.RotateVector({0,-Side*Strip/2,0});
 auto Box=[&](FVector2D A,FVector2D B,FVector& Min,FVector& Max){FBox Bounds(ForceInit);for(auto V:{A,FVector2D(B.X,A.Y),B,FVector2D(A.X,B.Y)})Bounds+=WorkCenter+R.RotateVector(FVector(V,0));Min=Bounds.Min;Max=Bounds.Max;};
 // Min/Max enclose the complete zone, including its internal parts strip.
 FBox Whole(ForceInit);for(auto V:{-Total/2,FVector2D(Total.X/2,-Total.Y/2),Total/2,FVector2D(-Total.X/2,Total.Y/2)})Whole+=Center+R.RotateVector(FVector(V,0));Z.Min=Whole.Min;Z.Max=Whole.Max;
 const double Gap=SupplyGap(P);const double Y0=Side>0?Work.Y/2+Gap:-Work.Y/2-Strip,Y1=Side>0?Work.Y/2+Strip:-Work.Y/2-Gap;
 Box({-Work.X/2,Y0},{Work.X/2,Y1},Z.DropMin,Z.DropMax);return true;
}
FVector FFoundrylineWorkZoneLayout::Buffer(const FFoundrylineFactoryZone& Z,EFoundrylinePart P){
 const EFoundrylinePart Parts[]={EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings};
 const bool Legacy=Z.Preset<0;const auto Work=WorkSize(Z.Preset);double X=Legacy?Z.DropMin.X+20:-Work.X/2+20;
 for(auto Item:Parts){const auto Size=FFoundrylineFactoryCatalog::DropSizeCm(Item);if(Item==P){if(Legacy)return {X+Size.X/2,Z.DropMin.Y+20+Size.Y/2,20};return Point(Z,{X+Size.X/2,Z.SupplySide*(Work.Y/2+SupplyGap(Z.Preset)+20+Size.Y/2),0});}X+=Size.X+25;}
 return Z.DropMin;
}
FVector2D FFoundrylineWorkZoneLayout::BaySize(const FFoundrylineFactoryZone& Z,EFoundrylinePart P){auto S=FFoundrylineFactoryCatalog::DropSizeCm(P);return Z.QuarterTurns%2?FVector2D(S.Y,S.X):S;}
FVector FFoundrylineWorkZoneLayout::WorkPoint(const FFoundrylineFactoryZone& Z){if(Z.Preset<0)return Z.Min+FVector(650,300,0);const auto S=WorkSize(Z.Preset);return ProductCenter(Z)+Rotation(Z).RotateVector({-S.X/2+650,Z.Preset==2?-Z.SupplySide*195.:-S.Y/2+300,0});}
