#include "Production/FoundrylineWeldingStationLayout.h"
#include "Core/FoundrylineUnits.h"
#include "Economy/FoundrylineEconomyTuning.h"

FVector FFoundrylineWeldingStationLayout::Point(const FFoundrylineWeldingStationRecord& S,double X,double Y,double Z){return FVector(S.Center.X,S.Center.Y,20)+Rotation(S).RotateVector(FVector((X-LengthFt/2)*Foot,(Y-WidthFt/2)*Foot,Z));}
FBox2D FFoundrylineWeldingStationLayout::Rect(const FFoundrylineWeldingStationRecord& S,double X0,double Y0,double X1,double Y1){FBox2D B(ForceInit);for(auto C:{FVector2D(X0,Y0),FVector2D(X1,Y0),FVector2D(X1,Y1),FVector2D(X0,Y1)})B+=FVector2D(Point(S,C.X,C.Y));return B;}
FVector2D FFoundrylineWeldingStationLayout::FootprintCm(int32 Q){const FVector2D Size(LengthFt*Foot,WidthFt*Foot);return Q%2?FVector2D(Size.Y,Size.X):Size;}
FVector FFoundrylineWeldingStationLayout::Dock(const FFoundrylineWeldingStationRecord& S,bool Out){const bool Left=PortOnLeft(S,Out);return Point(S,PortForkX(S,Out)+(Left?-1:1)*DockReachCm/Foot,6.5+DockOffset(S,Out));}
// Each welder stands 18 in from the bench edge, offset along the bench so both can work one product.
FVector FFoundrylineWeldingStationLayout::WorkPoint(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?CorePoint(S,9.5,10):CorePoint(S,7.5,3);}
// The bench's torch holster end faces OUT, leaving the full IN port clear for pallets.
FTransform FFoundrylineWeldingStationLayout::BenchTransform(const FFoundrylineWeldingStationRecord& S){return FTransform(FRotator(0,Rotation(S).Yaw+(S.bSwapSides?0:180),0),CorePoint(S,8.5,6.5));}
int32 FFoundrylineWeldingStationLayout::BinCapacity(EFoundrylinePart P){switch(P){case EFoundrylinePart::MountingPlate:return 4;case EFoundrylinePart::Bracket:return 8;case EFoundrylinePart::FrameSection:return 12;case EFoundrylinePart::LightKit:return 4;default:return 0;}}
// Stock on the IN pallets: frame sections along one side of their pallet with the light kits beside them, plates and
// brackets side by side on the other pallet.
FVector FFoundrylineWeldingStationLayout::BinPoint(const FFoundrylineWeldingStationRecord& S,EFoundrylinePart P){
 const double Across=P==EFoundrylinePart::FrameSection?-22:P==EFoundrylinePart::LightKit?35:P==EFoundrylinePart::MountingPlate?-28:28;
 const int32 Slot=InSlot(P);return Point(S,PortForkX(S,false),(Slot?8.5:(S.bSwapSides?8.5:4.5))+Across/Foot,PalletTopCm);
}
FTransform FFoundrylineWeldingStationLayout::ProductOnBench(){
return FTransform(FRotator(0,0,90),FVector(0,-30.48,RackDatumZ()));}
FTransform FFoundrylineWeldingStationLayout::ProductTransform(const FFoundrylineWeldingStationRecord& S){return ProductOnBench()*BenchTransform(S);}
bool FFoundrylineWeldingStationLayout::ProductFits(const FBox& P,FVector2D Carried,FString& E){
 const FVector2D Top(4*Foot,2*Foot);
 if(!P.IsValid||P.Min.X< -Top.X||P.Max.X>Top.X||P.Min.Y< -Top.Y||P.Max.Y>Top.Y){E=TEXT("Product is larger than the 8 ft x 4 ft bench top");return false;}
 if(P.Min.Z<=BenchHeightCm){E=TEXT("Product mounts would touch the worktop; use a fixture riser");return false;}
 if(P.Max.Z>=ClearHeightCm){E=FString::Printf(TEXT("Fixtured product needs more than %s of clear height"),*FFoundrylineUnits::Length(ClearHeightCm));return false;}
 const double Long=FMath::Max(Carried.X,Carried.Y),Short=FMath::Min(Carried.X,Carried.Y);
 if(Long>OutBayFt*Foot+.01||Short>PalletSizeCm().Y){E=TEXT("Finished product does not fit along the 6 ft 6 in OUT bay on one pallet");return false;}
 return true;
}
float FFoundrylineWeldingStationLayout::StageSeconds(int32 Stage){return FFoundrylineEconomyTuning::RackStageMinutes(Stage);}
float FFoundrylineWeldingStationLayout::UnloadSeconds(){return FFoundrylineEconomyTuning::StationUnloadMinutes;}
