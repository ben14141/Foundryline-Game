#pragma once
#include "Production/FoundrylineFactoryTypes.h"

// Fixed 19 ft 6 in x 13 ft welding station. Plan coordinates are feet from the footprint's
// lower-left corner (X along the 19 ft 6 in length); every result is world centimetres.
// Side 0 is Welder A (local Y 0-4.5 ft), side 1 is Welder B (local Y 8.5-13 ft). Each side has its own
// welding machine in a 4.5 x 2.5 ft corner bay at the outer end of that welder's strip.
// The core (bench, welder strips, machines, parts bay and IN) keeps a 17 ft plan; OUT is 6 ft 6 in wide instead of 4 ft so a
// 6 ft 6 in rack lies along it, and the core shifts 2 ft 6 in when OUT is on the left. Core* helpers take core plan feet.
struct FOUNDRYLINE_API FFoundrylineWeldingStationLayout {
 static constexpr double Foot=30.48;
 static constexpr double LengthFt=19.5;
 static constexpr double InBayFt=4;
 static constexpr double OutBayFt=6.5;
 static constexpr double WidthFt=13;
 static constexpr double BenchHeightCm=91.44;
 static constexpr double PaintWidthCm=7.62;
 // Shop height needed above the slab to load the fixture and keep weld fume/light clear.
 static constexpr double ClearHeightCm=304.8;
 // Same fork reach as the existing pallet docks: the forklift stops outside the station end.
 static constexpr double DockReachCm=136;
 static constexpr int32 MaxStations=4;
 // Schema 14 lengthened the station from 17 ft at its OUT end: how far a saved station's centre moves so its core stays put.
 static FVector Schema14Shift(const FFoundrylineWeldingStationRecord& S){return Rotation(S).RotateVector(FVector((S.bSwapSides?-1:1)*(OutBayFt-InBayFt)/2*Foot,0,0));}
 static FRotator Rotation(const FFoundrylineWeldingStationRecord& S){return FRotator(0,S.QuarterTurns*90,0);}
 static FVector Point(const FFoundrylineWeldingStationRecord& S,double XFt,double YFt,double ZCm=0);
 static FBox2D Rect(const FFoundrylineWeldingStationRecord& S,double X0,double Y0,double X1,double Y1);
 static FVector2D FootprintCm(int32 QuarterTurns);
 static FBox2D Bounds(const FFoundrylineWeldingStationRecord& S){return Rect(S,0,0,LengthFt,WidthFt);}
 static bool PortOnLeft(const FFoundrylineWeldingStationRecord& S,bool Out){return Out==S.bSwapSides;}
 static double CoreX(const FFoundrylineWeldingStationRecord& S){return PortOnLeft(S,true)?OutBayFt-InBayFt:0;}
 static FVector CorePoint(const FFoundrylineWeldingStationRecord& S,double XFt,double YFt,double ZCm=0){return Point(S,XFt+CoreX(S),YFt,ZCm);}
 static FBox2D CoreRect(const FFoundrylineWeldingStationRecord& S,double X0,double Y0,double X1,double Y1){return Rect(S,X0+CoreX(S),Y0,X1+CoreX(S),Y1);}
 static FBox2D Bench(const FFoundrylineWeldingStationRecord& S){return CoreRect(S,4.5,4.5,12.5,8.5);}
 static FBox2D Worker(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?CoreRect(S,4.5,8.5,12.5,13):CoreRect(S,4.5,0,12.5,4.5);}
 static double PortFt(bool Out){return Out?OutBayFt:InBayFt;}
 static double PortCenterX(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?PortFt(Out)/2:LengthFt-PortFt(Out)/2;}
 static FBox2D Port(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,0,2.5,PortFt(Out),10.5):Rect(S,LengthFt-PortFt(Out),2.5,LengthFt,10.5);}
 static FVector PortCenter(const FFoundrylineWeldingStationRecord& S,bool Out){return Point(S,PortCenterX(S,Out),6.5);}
 // Both ports set pallets down 2 ft in from the station end, so the forklift stops the same distance outside either one.
 static double PortForkX(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?2:LengthFt-2;}
 static constexpr int32 MaxDockOffsetFt=2;
 static int32 DockOffset(const FFoundrylineWeldingStationRecord& S,bool Out){return Out?S.OutDockOffsetFt:S.InDockOffsetFt;}
 static FVector Dock(const FFoundrylineWeldingStationRecord& S,bool Out);
 // Where a delivered pallet is set down: in line with the forklift stop, still inside the 4 x 8 ft port.
 static FVector PortDrop(const FFoundrylineWeldingStationRecord& S,bool Out){return Point(S,PortForkX(S,Out),6.5+DockOffset(S,Out));}
 // Reserved 4 ft access ring outside the station. Forklifts dock from the 4 x 8 ft space at each port end;
 // people walk the whole ring, so one connected walkway reaches both welder sides around the bench.
 static constexpr double AccessFt=4;
 static FBox2D ForkApproach(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,-AccessFt,2.5,0,10.5):Rect(S,LengthFt,2.5,LengthFt+AccessFt,10.5);}
 static FBox2D EndAccess(const FFoundrylineWeldingStationRecord& S,bool Out){return PortOnLeft(S,Out)?Rect(S,-AccessFt,0,0,WidthFt):Rect(S,LengthFt,0,LengthFt+AccessFt,WidthFt);}
 static FBox2D WorkerApproach(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?Rect(S,-AccessFt,WidthFt,LengthFt+AccessFt,WidthFt+AccessFt):Rect(S,-AccessFt,-AccessFt,LengthFt+AccessFt,0);}
 static TArray<FBox2D> AccessRing(const FFoundrylineWeldingStationRecord& S){return {EndAccess(S,false),EndAccess(S,true),WorkerApproach(S,0),WorkerApproach(S,1)};}
 // Welder A's machine sits at the left end of their strip, Welder B's at the right: diagonal, so leads never cross.
 static FBox2D MachineBay(const FFoundrylineWeldingStationRecord& S,int32 Side){return Side?CoreRect(S,12.5,10.5,17,13):CoreRect(S,0,0,4.5,2.5);}
 // Codex's MIG welder rebuild (SM_Welder_Machine): cart, power unit and bottle, bounds as imported (Blender's Y is mirrored
 // by the FBX import, so the holster that the source places at Y -38 cm is at +38 here). It stands lengthwise in its bay,
 // centred on the bounds. Its front panel (display, controls and lead sockets, the model's +X end) faces in along the bench
 // toward that machine's own welder; the gun holster is then on the machine's outer side.
 static FVector MachineSizeCm(){return {112.15,76.05,162.1};}
 static FVector MachineBoundsCentreCm(){return {.875,1.475,0};}
 static FTransform MachineTransform(const FFoundrylineWeldingStationRecord& S,int32 Side){const FBox2D B=MachineBay(S,Side);const FRotator R(0,Rotation(S).Yaw+(Side?180:0),0);return FTransform(R,FVector(B.GetCenter(),20)-R.RotateVector(MachineBoundsCentreCm()));}
 // Machine-local anchors (cm): holstered gun, front lead sockets, and the idle lead from the sockets to the holstered gun.
 static FVector HolsterCm(){return {17,38,87};}
 static FVector LeadSocketCm(){return {54,12,53};}
 // The ground (work) clamp hangs on the cart's inner side when idle.
 static FVector ClampIdleCm(){return {5,-44,8};}
 static TArray<FVector> IdleLead(){return {LeadSocketCm(),{66,24,5},{30,38,5},{17,38,78}};}
 // Torch support-hand anchor (gun-local cm, the imported SOCKET_Support).
 static FVector GunSupportCm(){return {3,3,8};}
 static FVector WorkPoint(const FFoundrylineWeldingStationRecord& S,int32 Side);
 static float WorkYaw(const FFoundrylineWeldingStationRecord& S,int32 Side){return Rotation(S).Yaw+(Side?-90:90);}
 static FTransform BenchTransform(const FFoundrylineWeldingStationRecord& S);
 // Rack fixtured flat: its upright datum turned onto its back on the risers (from Foundryline_RackWeldingSetup).
 static FTransform ProductTransform(const FFoundrylineWeldingStationRecord& S);
 static FTransform ProductOnBench();
 // Rack jig, from the fixture kit's own parts (SM_WeldingFixtureRiser / SM_WeldingFixtureHoldDown, split from Codex's kit).
 // The rack lies on its back with its 2 in frame tubes on four 4.92 in risers: two under the uprights, two under the upper
 // crossmember. Its bed-rail plates and saddles hang 4.5 in below the tubes and clear the worktop by about 3/8 in. Each
 // hold-down clamp stands beside a riser with its bridge over the tube. Poses are bench-local (floor datum, X along the bench).
 static constexpr double JigRiserCm=12.497;
 static constexpr double RackTubeHalfCm=2.54;
 static TArray<FTransform> JigRisers(){return {FTransform(FVector(-91.44,1.5,BenchHeightCm)),FTransform(FVector(91.44,1.5,BenchHeightCm)),FTransform(FVector(-75,37.5,BenchHeightCm)),FTransform(FVector(75,37.5,BenchHeightCm))};}
 static TArray<FTransform> JigHoldDowns(){return {FTransform(FVector(-100,1.5,BenchHeightCm)),FTransform(FVector(100,1.5,BenchHeightCm)),FTransform(FRotator(0,90,0),FVector(-75,44,BenchHeightCm)),FTransform(FRotator(0,90,0),FVector(75,44,BenchHeightCm))};}
 // Rack centre (bed-rail datum) on the jig: tube undersides on the riser tops.
 static double RackDatumZ(){return BenchHeightCm+JigRiserCm+RackTubeHalfCm;}
 // Headache rack fixtured on the jig, relative to the bench floor datum (bounds of SM_HeadacheRack lying on its back).
 static FBox RackOnBenchCm(){const double Z=RackDatumZ()-13.97;return FBox(FVector(-99.06,-40.64,Z),FVector(99.06,40.71,Z+27.94));}
 // Assembly stage times at the 100% (one welder) rate, in plant seconds.
 static float StageSeconds(int32 Stage){return Stage==1?30.f:Stage==2?90.f:Stage==3?20.f:Stage==4?6.f:0.f;}
 static FVector RackSizeCm(){return {198.12,81.28,27.94};}
 static bool ProductFits(const FBox& OnBench,FVector2D CarriedSize,FString& Error);
 static FString Name(int32 Number){return FString::Printf(TEXT("Welding Station %02d"),Number);}
 // Station parts bay: the free 4.5 x 2.5 ft corner beside IN (diagonal to the welding machines). Kit stock sits on the floor
 // there, with no table: the frame-section bundle along the outer half, plates, brackets and light
 // kits side by side along the inner half. The main bench is the station's only bench. IN is only a transfer port.
 static FBox2D PartsBay(const FFoundrylineWeldingStationRecord& S){return PortOnLeft(S,false)?CoreRect(S,0,10.5,4.5,13):CoreRect(S,12.5,0,17,2.5);}
 // Bin capacity per assembly category: two full headache-rack kits, so one kit can be in work while the next waits.
 static int32 BinCapacity(EFoundrylinePart Part);
 static FVector BinPoint(const FFoundrylineWeldingStationRecord& S,EFoundrylinePart Part);
 // Short unload interaction: the delivered pallet on IN is emptied into the bins and the port clears.
 static constexpr float UnloadSeconds=8;
 // OUT: two wood pallets (SM_Pallet_Wood, 3 ft 9.6 in x 3 ft 10.4 in) side by side across the port's 8 ft, each centred in its
 // 4 ft half and on the 2 ft fork line, so a forklift takes either one straight out of the port end from the same stop as IN.
 // Finished racks lie flat on their backs, length along the 6 ft 6 in bay and centred on it: each stack sits on its pallet
 // flush with the pallet's outer edge and runs on 2 ft 7 in past its inner edge, stacked RacksPerOutPallet high.
 // Welders fill the first pallet with room, then the second; the station waits when both are full.
 static constexpr int32 OutPallets=2;
 static constexpr int32 RacksPerOutPallet=5;
 static constexpr double PalletTopCm=17.557;
 static FVector PalletSizeCm(){return {115.706,117.974,PalletTopCm};}
 static FVector OutPalletPoint(const FFoundrylineWeldingStationRecord& S,int32 Slot){return Point(S,PortForkX(S,true),Slot?8.5:4.5);}
 static FBox2D OutPalletRect(const FFoundrylineWeldingStationRecord& S,int32 Slot){const FVector2D H(PalletSizeCm().X/2/Foot,PalletSizeCm().Y/2/Foot);const double X=PortForkX(S,true),Y=Slot?8.5:4.5;return Rect(S,X-H.X,Y-H.Y,X+H.X,Y+H.Y);}
 // Pallet frame: +X points into the station, away from the forklift that serves it.
 static FTransform OutPalletTransform(const FFoundrylineWeldingStationRecord& S,int32 Slot){return FTransform(FRotator(0,Rotation(S).Yaw+(PortOnLeft(S,true)?0:180),0),OutPalletPoint(S,Slot));}
 // Forklift stop for one OUT pallet: on the fork line's normal through that pallet, as for IN.
 static FVector OutPalletDock(const FFoundrylineWeldingStationRecord& S,int32 Slot){return Point(S,PortForkX(S,true)+(PortOnLeft(S,true)?-1:1)*DockReachCm/Foot,Slot?8.5:4.5);}
 // Which OUT pallet a stored position belongs to (-1 if neither).
 static int32 OutSlot(const FFoundrylineWeldingStationRecord& S,const FVector& At){for(int32 I=0;I<OutPallets;++I)if(FVector::Dist2D(At,OutPalletPoint(S,I))<1)return I;return -1;}
 // Rack Level (0 = on the pallet deck) of a stack, relative to its pallet: bed-rail datum turned onto its back as on the jig.
 // Any pallet of finished racks (OUT, forks, storage, pickup): the stack starts 1 1/4 in inside the pallet edge nearest the
 // forklift (pallet -X) and runs on past the far edge, so it never overhangs the forks' side. On OUT that centres it on the bay.
 static FTransform RackOnPallet(int32 Level){return FTransform(FRotator(0,0,90),FVector(RackStackX(),-30.48,PalletTopCm+13.97+Level*RackSizeCm().Z));}
 static double RackStackX(){return (OutBayFt/2-2)*Foot;}
 static FTransform RackOnOut(const FFoundrylineWeldingStationRecord& S,int32 Slot,int32 Level){return RackOnPallet(Level)*OutPalletTransform(S,Slot);}
 // A stack's frame: on the pallet deck datum, centred on the rack stack (offset into the bay from the pallet centre).
 static FTransform OutStackTransform(const FFoundrylineWeldingStationRecord& S,int32 Slot){return FTransform(FVector(RackStackX(),0,0))*OutPalletTransform(S,Slot);}
 // Racks already on one OUT pallet, and where the next finished rack goes (false when both pallets are full).
 static int32 OutCount(const FFoundrylineWeldingStationRecord& S,const TArray<FFoundrylineFactoryPallet>& Pallets,int32 Slot){int32 N=0;for(const auto& P:Pallets)if(P.Owner==S.Id&&P.Place==EFoundrylinePalletPlace::StationOut&&OutSlot(S,P.Position)==Slot)N+=P.Quantity;return N;}
 // A pallet a forklift has been sent for takes no more racks.
 static bool OutSlotClaimed(const FFoundrylineWeldingStationRecord& S,const TArray<FFoundrylineFactoryPallet>& Pallets,int32 Slot){return Pallets.ContainsByPredicate([&](const auto& P){return P.Owner==S.Id&&P.Place==EFoundrylinePalletPlace::StationOut&&P.Reserved&&OutSlot(S,P.Position)==Slot;});}
 static bool NextOutSpot(const FFoundrylineWeldingStationRecord& S,const TArray<FFoundrylineFactoryPallet>& Pallets,int32& Slot,int32& Level){for(Slot=0;Slot<OutPallets;++Slot){Level=OutCount(S,Pallets,Slot);if(Level<RacksPerOutPallet&&!OutSlotClaimed(S,Pallets,Slot))return true;}Slot=-1;Level=0;return false;}

};
