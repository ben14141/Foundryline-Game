#pragma once
#include "CoreMinimal.h"
#include "Orders/FoundrylineCommerceTypes.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Inventory/FoundrylineAssemblySupply.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "FoundrylineFactoryTypes.generated.h"

UENUM(BlueprintType)
enum class EFoundrylinePart:uint8 {Sheet,Bracket,RailBlank,FrameRail,ShellBlank,ShellSection,EndCap,RunningGear,Fittings,BentBracket,MountingPlate,GussetPlate,ReinforcementPlate,SupportChannel,FrameSection,LightKit,HeadacheRack,Count};
UENUM()
enum class EFoundrylinePalletPlace:uint8 {Receiving,Rack,MachineInput,MachineOutput,Buffer,Forks,Pickup,Inbound,Outbound,InTransit,StationIn,StationOut};
UENUM()
enum class EFoundrylineFloorLane:uint8 {Forklift,Pedestrian,Crossing};

USTRUCT()
struct FFoundrylineFactoryPallet {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) EFoundrylinePart Part=EFoundrylinePart::Sheet;
 UPROPERTY(SaveGame) int32 Quantity=0;
 UPROPERTY(SaveGame) int32 Reserved=0;
 UPROPERTY(SaveGame) int32 OrderId=0;
 UPROPERTY(SaveGame) int32 BookingId=0;
 UPROPERTY(SaveGame) bool bLegacyFiveStack=false;
 UPROPERTY(SaveGame) EFoundrylinePalletPlace Place=EFoundrylinePalletPlace::Receiving;
 UPROPERTY(SaveGame) int32 Owner=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) float UnitLb=0;
};
USTRUCT()
struct FFoundrylineFactoryProgram {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Machine=1;
 UPROPERTY(SaveGame) EFoundrylinePart Part=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) float Seconds=8;
 UPROPERTY(SaveGame) float SteelLb=3;
};
USTRUCT()
struct FFoundrylineFactorySlot {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) EFoundrylinePart Part=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Dock=FVector::ZeroVector;
 UPROPERTY(SaveGame) int32 MachineId=0;
 UPROPERTY(SaveGame) bool bInputSupply=false;
 UPROPERTY(SaveGame) bool bMachineOutput=false;
};
USTRUCT()
struct FFoundrylineMachineBatch {
 GENERATED_BODY()
 UPROPERTY(SaveGame) EFoundrylinePart Part=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) int32 Quantity=0;
 UPROPERTY(SaveGame) int32 OrderId=0;
};
USTRUCT()
struct FFoundrylineFactoryMachine {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 // Machine identity is independent from equipment type (1 plasma, 2 brake, 3 roller).
 UPROPERTY(SaveGame) int32 MachineType=0;
 UPROPERTY(SaveGame) int32 QuarterTurns=0;
 UPROPERTY(SaveGame) int32 OutputSlot=0;
 UPROPERTY(SaveGame) int32 ActiveOrderId=0;
 UPROPERTY(SaveGame) bool bRackOutput=false;
 UPROPERTY(SaveGame) TArray<FFoundrylineMachineBatch> Queue;
 int32 Kind()const{return MachineType?MachineType:Id;}
 UPROPERTY(SaveGame) FVector Origin=FVector::ZeroVector;
 UPROPERTY(SaveGame) bool bStored=false;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactorySlot> StoredSlots;
 UPROPERTY(SaveGame) FVector InputDock=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector OutputDock=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector InputStock=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector OutputStock=FVector::ZeroVector;
 UPROPERTY(SaveGame) EFoundrylinePart Making=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) float Remaining=0;
 UPROPERTY(SaveGame) bool Enabled=true;
 UPROPERTY(SaveGame) bool Auto=true;
 UPROPERTY(SaveGame) int32 BatchRemaining=0;
 UPROPERTY(SaveGame) float BedLb=0;
 UPROPERTY(SaveGame) float ReservedLb=0;
 UPROPERTY(SaveGame) float Duration=0;
 UPROPERTY(SaveGame) bool HasQueuedProgram=false;
 UPROPERTY(SaveGame) EFoundrylinePart NextPart=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) int32 NextBatch=1;
 UPROPERTY(SaveGame) bool NextAuto=true;
 // Game minutes spent loading the next sheet onto the plasma bed (economy v1).
 UPROPERTY(SaveGame) float SheetLoad=0;
};
USTRUCT()
struct FFoundrylineFactoryZone {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FVector Min=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Max=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector DropMin=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector DropMax=FVector::ZeroVector;
 // -1 preserves existing custom zones. New placements use the product preset catalog.
 UPROPERTY(SaveGame) int32 Preset=-1;
 UPROPERTY(SaveGame) int32 QuarterTurns=0;
 UPROPERTY(SaveGame) int32 SupplySide=1;
 UPROPERTY(SaveGame) bool Ordered=false;
 UPROPERTY(SaveGame) int32 CompletedMask=0;
 UPROPERTY(SaveGame) int32 ActiveStep=-1;
 UPROPERTY(SaveGame) float Work=0;
 UPROPERTY(SaveGame) int32 Welders=1;
 UPROPERTY(SaveGame) TArray<int32> ReservedParts;
 UPROPERTY(SaveGame) TArray<int32> Installed;
 UPROPERTY(SaveGame) int32 ProductsCompleted=0;
 UPROPERTY(SaveGame) float ReservedLb=0;
 UPROPERTY(SaveGame) float InstalledLb=0;
};
USTRUCT()
struct FFoundrylineFactoryLane {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) EFoundrylineFloorLane Kind=EFoundrylineFloorLane::Forklift;
 UPROPERTY(SaveGame) FVector2D Min=FVector2D::ZeroVector;
 UPROPERTY(SaveGame) FVector2D Max=FVector2D::ZeroVector;
 UPROPERTY(SaveGame) int32 Direction=0;
};
USTRUCT()
struct FFoundrylineFactoryRoute {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString Name;
 UPROPERTY(SaveGame) TArray<int32> Sources;
 UPROPERTY(SaveGame) TArray<int32> Destinations;
 UPROPERTY(SaveGame) bool Enabled=true;
};
USTRUCT()
struct FFoundrylineFactoryJob {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) int32 SourcePallet=0;
 UPROPERTY(SaveGame) int32 Destination=0;
 UPROPERTY(SaveGame) EFoundrylinePalletPlace DestinationPlace=EFoundrylinePalletPlace::Rack;
 UPROPERTY(SaveGame) EFoundrylinePart Part=EFoundrylinePart::Bracket;
 UPROPERTY(SaveGame) int32 Quantity=0;
 UPROPERTY(SaveGame) int32 Driver=0;
 UPROPERTY(SaveGame) int32 CarryPallet=0;
 UPROPERTY(SaveGame) int32 Priority=0;
 UPROPERTY(SaveGame) EFoundrylineForkTask Task=EFoundrylineForkTask::Storage;
 UPROPERTY(SaveGame) EFoundrylinePalletPlace SourcePlace=EFoundrylinePalletPlace::Rack;
 UPROPERTY(SaveGame) int32 OrderId=0;
 UPROPERTY(SaveGame) int32 BookingId=0;
 UPROPERTY(SaveGame) FVector Pickup=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Drop=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector StoredAt=FVector::ZeroVector;
 UPROPERTY(SaveGame) int32 Phase=0; // 0 queued, 1 pickup travel, 2 pickup, 3 delivery travel, 4 unload.
 UPROPERTY(SaveGame) float Handling=0;
};
USTRUCT()
struct FFoundrylineFactoryDriver {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 EmployeeId=0;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) int32 Route=1;
 // Receiving, Production, Shipping: -1 disables a category; 0..2 are displayed as 1..3.
 UPROPERTY(SaveGame) TArray<int32> Priorities={0,1,2};
 UPROPERTY(SaveGame) int32 Job=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Parking=FVector::ZeroVector;
 UPROPERTY(SaveGame) float ParkingYaw=90;
 UPROPERTY(SaveGame) TArray<FVector> Path;
 UPROPERTY(SaveGame) float Distance=0;
 UPROPERTY(SaveGame) float Yaw=0;
 UPROPERTY(SaveGame) bool Waiting=false;
 UPROPERTY(SaveGame) float WaitFor=0;
};
USTRUCT()
struct FFoundrylineFactoryWalker {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) FVector Home=FVector(-2300,300,20);
 UPROPERTY(SaveGame) float Retry=0;
 UPROPERTY(SaveGame) float Yaw=0;
 UPROPERTY(SaveGame) float Speed=0;
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) int32 Zone=0;
 // Schema 12: welding station this welder works at, exclusive with Zone. Schema 13: their slot there (0 Welder A, 1 Welder B).
 UPROPERTY(SaveGame) int32 Station=0;
 UPROPERTY(SaveGame) int32 StationSide=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) TArray<FVector> Path;
 UPROPERTY(SaveGame) float Distance=0;
 UPROPERTY(SaveGame) bool Waiting=false;
};
// Placed 19 ft 6 in x 13 ft welding station (bench, two welder sides, two machines, IN/OUT). Geometry derives from FFoundrylineWeldingStationLayout;
// only the transform and options are stored, never generated mesh/component handles.
USTRUCT()
struct FFoundrylineWeldingStationRecord {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) int32 Number=1;
 UPROPERTY(SaveGame) FVector Center=FVector::ZeroVector;
 UPROPERTY(SaveGame) int32 QuarterTurns=0;
 // False: IN at local left (0-4 ft), OUT at right. True swaps both ports.
 UPROPERTY(SaveGame) bool bSwapSides=false;
 // Forklift stop along each port's 8 ft length, in whole feet from centre (-2..2), chosen at placement
 // so a station beside an aisle is reachable without leaving the reserved approach.
 UPROPERTY(SaveGame) int32 InDockOffsetFt=0;
 UPROPERTY(SaveGame) int32 OutDockOffsetFt=0;
 // Schema 11: assembly recipe (FFoundrylineFactoryCatalog::Recipe id, 0 = none) and units still to make.
 // Supplies are only requested while both are set.
 UPROPERTY(SaveGame) int32 Recipe=0;
 UPROPERTY(SaveGame) int32 BatchRemaining=0;
 // Time spent transferring the single pallet on the IN port into the station's own bins.
 UPROPERTY(SaveGame) float UnloadSeconds=0;
 // Schema 12: the one unit on the bench. Its whole kit leaves the bins when it starts and becomes the finished product
 // only when it reaches OUT. Stage 0 none, 1 fit-up, 2 welding, 3 final assembly (lights), 4 moving to OUT.
 UPROPERTY(SaveGame) int32 UnitRecipe=0;
 UPROPERTY(SaveGame) int32 Stage=0;
 UPROPERTY(SaveGame) float Work=0;
 UPROPERTY(SaveGame) TArray<int32> Reserved;
 UPROPERTY(SaveGame) float ReservedLb=0;
 UPROPERTY(SaveGame) int32 UnitsCompleted=0;
 // Presentation only: work advanced on the last step (sparks follow this, never the stage alone).
 UPROPERTY() bool bAdvancing=false;
};
USTRUCT()
struct FFoundrylineFactoryState {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Version=15;
 UPROPERTY(SaveGame,NotReplicated) FFoundrylineCommerceState Commerce;
 UPROPERTY(SaveGame) bool bCutPartsStarter=false;
 UPROPERTY(SaveGame) FGuid CompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(SaveGame) FGuid PlantId=FGuid(0xF001,2,1,1);
 UPROPERTY(SaveGame) int32 NextId=2000;
 UPROPERTY(SaveGame) int64 Revision=0;
 UPROPERTY(SaveGame) bool Running=false;
 UPROPERTY(SaveGame) float Speed=1;
 UPROPERTY(SaveGame) int32 RawConsumed=0;
 UPROPERTY(SaveGame) float ScrapLb=0;
 UPROPERTY(SaveGame) float FinishedSteelLb=0;
 UPROPERTY(SaveGame) int32 FinishedProducts=0;
 UPROPERTY(SaveGame) int32 LastDriver=0;
 UPROPERTY(SaveGame) float ReceivedSteelLb=0;
 UPROPERTY(SaveGame) FVector2D FloorMin=FVector2D(-2370,1230);
 UPROPERTY(SaveGame) FVector2D FloorMax=FVector2D(1170,4170);
 // Shipping preparation follows an owned loading-door preset. Legacy saves keep
 // their original staging coordinates until all committed freight has cleared.
 UPROPERTY(SaveGame) bool bPresetPickup=false;
 UPROPERTY(SaveGame) bool bPickupLayoutPending=false;
 UPROPERTY(SaveGame) FGuid PickupOpeningId;
 UPROPERTY(SaveGame) FVector PickupOrigin=FVector(-850,2000,20);
 UPROPERTY(SaveGame) float PickupYaw=0;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryPallet> Pallets;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactorySlot> Slots;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryMachine> Machines;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryZone> Zones;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryLane> Lanes;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryRoute> Routes;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryDriver> Drivers;
 // Paid office purchases waiting for a planner parking position.
 UPROPERTY(SaveGame) int32 UnplacedForklifts=0;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryJob> Jobs;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryWalker> Walkers;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryOperator> Operators;
 UPROPERTY(SaveGame) TArray<FFoundrylineStandbyDriver> StandbyDrivers; // Schema 8 migration only.
 UPROPERTY(SaveGame) TArray<FFoundrylineStandbyDriver> UnassignedEmployees;
 UPROPERTY(SaveGame) TArray<FGuid> HireRequests;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryProgram> Programs;
 // Schema 10: welding stations. Schema 9 saves load with none. Schema 11 adds station recipes and supply buffers.
 UPROPERTY(SaveGame) TArray<FFoundrylineWeldingStationRecord> WeldingStations;
 UPROPERTY(SaveGame) TArray<FFoundrylineAssemblySupplyBin> SupplyBins;
 UPROPERTY(SaveGame) TArray<FFoundrylinePartsCollector> Collectors;
 UPROPERTY(SaveGame) TArray<FFoundrylinePlacedStorageRack> StorageRacks;
 UPROPERTY(SaveGame) TArray<FFoundrylinePlacedOfficeDesk> OfficeDesks;
 UPROPERTY(SaveGame) FString Status;
};

struct FFoundrylinePartSpec {const TCHAR* Name;int32 Capacity;float UnitLb=0;};
struct FFoundrylineAssemblyStep {const TCHAR* Name;int32 Prerequisites;float Seconds;TArray<int32> Parts;};
// Parts bought ready-made: fixed unit weight and supplier price. A delivered bundle or crate holds the part's Capacity.
struct FFoundrylineBoughtPart {float UnitLb=0;int64 PriceCents=0;};
// Welding-station product data: the finished part and its per-unit bill (indexed by EFoundrylinePart).
// TwoWelderRate: balance value for the shared work rate with both station welders at work (one welder is 1.0).
struct FFoundrylineAssemblyRecipe {int32 Id=0;const TCHAR* Name=TEXT("");EFoundrylinePart Output=EFoundrylinePart::Count;TArray<int32> Bill;float TwoWelderRate=1.7f;};
struct FOUNDRYLINE_API FFoundrylineFactoryCatalog {
 static constexpr int32 PartCount=17;
 static FFoundrylinePartSpec Part(EFoundrylinePart Id);
 static constexpr double StandardPalletCargoLb=1500;
 static double PalletCargoLimitLb(EFoundrylinePart Id);
 static bool FitsPalletWeight(EFoundrylinePart Id,int32 Quantity,double UnitLb);
 // Stable IDs are retained for old saves and archived development maps.
 static bool AvailableNow(EFoundrylinePart Id);
 static TArray<FFoundrylineFactoryProgram> StarterPrograms(bool Brake=false);
 static TArray<FFoundrylineFactoryProgram> MachinePrograms(int32 Id,int32 Kind,bool Current);
 // Static mesh object path used to show a part on pallets, racks and trucks.
 static FString PartMesh(EFoundrylinePart Id);
 static bool Bought(EFoundrylinePart Id,FFoundrylineBoughtPart* Out=nullptr);
 // The four shared assembly categories, in display order: Steel Plates, Steel Brackets, Frame Sections, Light Kits.
 static TArray<EFoundrylinePart> AssemblyParts(){return {EFoundrylinePart::MountingPlate,EFoundrylinePart::BentBracket,EFoundrylinePart::FrameSection,EFoundrylinePart::LightKit};}
 static const TCHAR* AssemblyName(EFoundrylinePart Id);
 static constexpr int32 RecipeCount=1;
 static bool Recipe(int32 Id,FFoundrylineAssemblyRecipe& Out);
 static TArray<FFoundrylineAssemblyStep> TankerSteps();
 static TArray<int32> TankerBill();
 static TArray<FFoundrylineFactoryProgram> DefaultPrograms();
 static FVector2D DropSizeCm(EFoundrylinePart Part);
 static FVector2D TankerZoneCm(){return {2000,1000};}
 static FVector2D TankerDropCm(){return {1500,400};}
 static constexpr float TankerDiameterCm=235;
 static constexpr float TankerLengthCm=1191.2246f;
 static constexpr float TankerClearHeightCm=450;
};
