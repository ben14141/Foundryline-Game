#pragma once
#include "Production/FoundrylineFactoryTypes.h"

// Fixed 17 x 13 ft welding station. Plan coordinates are feet from the footprint's
// lower-left corner (X along the 17 ft length); every result is world centimetres.
// Side 0 is Welder A (local Y 0-4.5 ft), side 1 is Welder B (local Y 8.5-13 ft). Each side has its own
// welding machine in a 4.5 x 2.5 ft corner bay at the outer end of that welder's strip.
struct FOUNDRYLINE_API FFoundrylineWeldingStationLayout {
 static constexpr double Foot=30.48;
 static constexpr double LengthFt=17;
 static constexpr double WidthFt=13;
 static constexpr double BenchHeightCm=91.44;
 static constexpr double PaintWidthCm=7.62;
 // Shop height needed above the slab to load the fixture and keep weld fume/light clear.
 static constexpr double ClearHeightCm=304.8;
 // Same fork reach as the existing pallet docks: the forklift stops outside the station end.
 static constexpr double DockReachCm=136;
 static constexpr int32 MaxStations=4;
 static FRotator Rotation(const FFoundrylineWeldingStationRecord& S){return FRotator(0,S.QuarterTurns*90,0);}
 static FVector Point(const FFoundrylineWeldingStationRecord& S,double XFt,double YFt,double ZCm=0);
 static FBox2D Rect(const FFoundrylineWeldingStationRecord& S,double X0,double Y0,double X1,double Y1);
 static FVector2D FootprintCm(int32 QuarterTurns);
 static FBox2D Bounds(const FFoundrylineWeldingStationRecord& S){return Rect(S,0,0,LengthFt,WidthFt);}
 static FBox2D Bench(const FFoundrylineWeldingStationRecord& S){return Rect(S,4.5,4.5,12.5,8.5);}
 static FBox2D Worker(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?Rect(S,4.5,8.5,12.5,13):Rect(S,4.5,0,12.5,4.5);}
 static bool PortOnLeft(const FFoundrylineWeldingStationRecord& S,bool Out){return Out==S.bSwapSides;}
 static FBox2D Port(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,0,2.5,4,10.5):Rect(S,13,2.5,17,10.5);}
 static FVector PortCenter(const FFoundrylineWeldingStationRecord& S,bool Out){return Point(S,PortOnLeft(S,Out)?2:15,6.5);}
 static constexpr int32 MaxDockOffsetFt=2;
 static int32 DockOffset(const FFoundrylineWeldingStationRecord& S,bool Out){return Out?S.OutDockOffsetFt:S.InDockOffsetFt;}
 static FVector Dock(const FFoundrylineWeldingStationRecord& S,bool Out);
 // Where a delivered pallet is set down: in line with the forklift stop, still inside the 4 x 8 ft port.
 static FVector PortDrop(const FFoundrylineWeldingStationRecord& S,bool Out){return Point(S,PortOnLeft(S,Out)?2:15,6.5+DockOffset(S,Out));}
 // Reserved 4 ft access ring outside the station. Forklifts dock from the 4 x 8 ft space at each port end;
 // people walk the whole ring, so one connected walkway reaches both welder sides around the bench.
 static constexpr double AccessFt=4;
 static FBox2D ForkApproach(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,-AccessFt,2.5,0,10.5):Rect(S,LengthFt,2.5,LengthFt+AccessFt,10.5);}
 static FBox2D EndAccess(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,-AccessFt,0,0,WidthFt):Rect(S,LengthFt,0,LengthFt+AccessFt,WidthFt);}
 static FBox2D WorkerApproach(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?Rect(S,-AccessFt,WidthFt,LengthFt+AccessFt,WidthFt+AccessFt):Rect(S,-AccessFt,-AccessFt,LengthFt+AccessFt,0);}
 static TArray<FBox2D> AccessRing(const FFoundrylineWeldingStationRecord& S){return {EndAccess(S,false),EndAccess(S,true),WorkerApproach(S,0),WorkerApproach(S,1)};}
 // Welder A's machine sits at the left end of their strip, Welder B's at the right: diagonal, so leads never cross.
 static FBox2D MachineBay(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?Rect(S,12.5,10.5,17,13):Rect(S,0,0,4.5,2.5);}
 // Existing Welder_Machine footprint (manifest), long side along the station length. Its front panel (display, controls and
 // lead terminals) is the model's +X end, which faces in along the bench toward that machine's own welder; the gun holster,
 // modelled on the -Y long side, is then on the machine's outer side within the bay.
 static FVector MachineSizeCm(){return {122.5561,70.7,160.8};}
 static FTransform MachineTransform(const FFoundrylineWeldingStationRecord& S,int32 Side){const FBox2D B=MachineBay(S,Side);return FTransform(FRotator(0,Rotation(S).Yaw+(Side?180:0),0),FVector(B.GetCenter(),20));}
 // Idle lead from the front terminals, along the floor and up the machine's side to the holstered gun (machine-local cm).
 static TArray<FVector> IdleLead(){return {{54,-12,53},{66,-24,5},{30,-38,5},{17,-38,78}};}
 static FVector WorkPoint(const FFoundrylineWeldingStationRecord& S,int32 Side);
 static float WorkYaw(const FFoundrylineWeldingStationRecord& S,int32 Side){return Rotation(S).Yaw+(Side?-90:90);}
 static FTransform BenchTransform(const FFoundrylineWeldingStationRecord& S);
 // Rack fixtured flat: its upright datum turned onto its back on the risers (from Foundryline_RackWeldingSetup).
 static FTransform ProductTransform(const FFoundrylineWeldingStationRecord& S);
 static FTransform ProductOnBench();
 // Headache rack fixtured flat on the bench, relative to the bench floor datum (from the source asset QA).
 static FBox RackOnBenchCm(){return FBox(FVector(-99.06,-40.64,92.71),FVector(99.06,40.64,120.65));}
 static FVector RackSizeCm(){return {198.12,81.28,27.94};}
 static bool ProductFits(const FBox& OnBench,FVector2D CarriedSize,FString& Error);
 static FString Name(int32 Number){return FString::Printf(TEXT("Welding Station %02d"),Number);}
};
