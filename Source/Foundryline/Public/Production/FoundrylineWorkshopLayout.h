#pragma once
#include "Production/FoundrylineFactoryTypes.h"

// One authored layout shared by scene geometry, stock positions and navigation.
struct FFoundrylineWorkshopRack {
 FVector Center;
 float Yaw=0;
 FString Name;
};
struct FOUNDRYLINE_API FFoundrylineWorkshopLayout {
 // SM_Rack_Shelf400 is 20 cm tall overall, but only thin edge lips reach that: the beams a pallet rests on top out at
 // 12.77 cm (deck plate 11.22 cm). Each shelf is set so its beam tops meet the pallet underside at the slot height
 // (rack centre + 40 cm + 130 cm per level), instead of the lips, which left pallets floating about 3 in up.
 static constexpr double ShelfBearingCm=12.77;
 static double SlotHeight(int32 Level){return 40+Level*130;}
 static double ShelfOrigin(int32 Level){return SlotHeight(Level)-ShelfBearingCm;}
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
 // Painted starter areas that stay clear of new stations: outbound staging, forklift bay and burn-table cell.
 // Starter building slab; the commerce floor extends outside onto the truck apron.
 static FBox2D CutShopInterior(){return {{-2370,30},{370,2170}};}
 static TArray<FBox2D> CutShopReserved(){return {FBox2D({-1100,1850},{-600,2170}),FBox2D({0,220},{200,720}),FBox2D({-1600,400},{-1000,1100})};}
 static TArray<FBox2D> Solids();
 // Equipment footprint for a replicated plant state, matching what the server seeded/installed.
 static TArray<FBox2D> PlantSolids(const FFoundrylineFactoryState& State);
 static TArray<FFoundrylineFactorySlot> Slots();
 static const TCHAR* SlotName(int32 Id);
};
