#pragma once
#include "Production/FoundrylineFactoryTypes.h"

// One authored layout shared by scene geometry, stock positions and navigation.
struct FFoundrylineWorkshopRack {
 FVector Center;
 float Yaw=0;
 FString Name;
};
struct FOUNDRYLINE_API FFoundrylineWorkshopLayout {
 static FVector2D FloorMin(){return {-2370,30};}
 static FVector2D FloorMax(){return {2770,4970};}
 static FVector ZoneCenter(){return {1855.6,1863.36,20};}
 static FVector SecondZoneCenter(){return {1855.6,4136.64,20};}
 static FVector Machine(int32 Id){return {-1300,Id==1?800.:Id==2?2300.:3800.,20};}
 static FVector InputRack(int32 Id){return {-700,Machine(Id).Y-(Id==2?200:300),20};}
 static FVector OutputRack(int32 Id){return {-700,Machine(Id).Y+(Id==2?300:200),20};}
 static FVector InputDock(int32 Id){return InputRack(Id)+FVector(136,0,0);}
 static FVector InputStock(int32 Id){return InputRack(Id)+FVector(0,0,40);}
 static FVector OutputDock(int32 Id){return {-1300,Machine(Id).Y+450,20};}
 static FVector OutputStock(int32 Id){return OutputDock(Id)+FVector(100,0,0);}
 static FVector SteelArrival(){return {2600,3000,20};}
 static FVector PurchasedArrival(EFoundrylinePart Part){return {Part==EFoundrylinePart::EndCap?1000.:Part==EFoundrylinePart::RunningGear?1500.:2000.,3300,20};}
 // Four separate stalls in the clear strip between the main aisle and people walkway.
 static FVector DriverHome(int32 Index){return {100,Index<2?650.+Index*600.:4050.+(Index-2)*600.,20};}
 static FBox2D DriverParkingBounds(int32 Index){const FVector2D P(DriverHome(Index));return {P-FVector2D(100,280),P+FVector2D(100,220)};}
 static FBox2D PickupClearance(){return {{1300,1450},{2800,1850}};}
 static FBox2D PickupBay(){return {{2800,1350},{4000,1950}};}
 static TArray<FFoundrylineWorkshopRack> Racks();
 static TArray<FFoundrylineWorkshopRack> CutShopRacks(){return {{InputRack(1),0,TEXT("RAW STEEL / 60 x 120 IN")},{OutputRack(1),0,TEXT("CUT PARTS / PALLETS")}};}
 static TArray<FBox2D> CutShopSolids();
 static TArray<FBox2D> Solids();
 static TArray<FFoundrylineFactorySlot> Slots();
 static const TCHAR* SlotName(int32 Id);
};
