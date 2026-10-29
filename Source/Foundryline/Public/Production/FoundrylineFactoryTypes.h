#pragma once
#include "CoreMinimal.h"
#include "Orders/FoundrylineCommerceTypes.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "FoundrylineFactoryTypes.generated.h"

UENUM(BlueprintType)
enum class EFoundrylinePart:uint8 {Sheet,Bracket,RailBlank,FrameRail,ShellBlank,ShellSection,EndCap,RunningGear,Fittings,BentBracket,MountingPlate,GussetPlate,ReinforcementPlate,SupportChannel,Count};
UENUM()
enum class EFoundrylinePalletPlace:uint8 {Receiving,Rack,MachineInput,MachineOutput,Buffer,Forks,Pickup,Inbound,Outbound,InTransit};
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
};
USTRUCT()
struct FFoundrylineFactoryMachine {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FVector Origin=FVector::ZeroVector;
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
 UPROPERTY(SaveGame) TArray<int32> Priorities={0,1,2};
 UPROPERTY(SaveGame) int32 Job=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Parking=FVector::ZeroVector;
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
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) TArray<FVector> Path;
 UPROPERTY(SaveGame) float Distance=0;
 UPROPERTY(SaveGame) bool Waiting=false;
};
USTRUCT()
struct FFoundrylineFactoryState {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Version=9;
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
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryPallet> Pallets;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactorySlot> Slots;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryMachine> Machines;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryZone> Zones;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryLane> Lanes;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryRoute> Routes;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryDriver> Drivers;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryJob> Jobs;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryWalker> Walkers;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryOperator> Operators;
 UPROPERTY(SaveGame) TArray<FFoundrylineStandbyDriver> StandbyDrivers; // Schema 8 migration only.
 UPROPERTY(SaveGame) TArray<FFoundrylineStandbyDriver> UnassignedEmployees;
 UPROPERTY(SaveGame) TArray<FGuid> HireRequests;
 UPROPERTY(SaveGame) TArray<FFoundrylineFactoryProgram> Programs;
 UPROPERTY(SaveGame) FString Status;
};

struct FFoundrylinePartSpec {const TCHAR* Name;int32 Capacity;};
struct FFoundrylineAssemblyStep {const TCHAR* Name;int32 Prerequisites;float Seconds;TArray<int32> Parts;};
struct FOUNDRYLINE_API FFoundrylineFactoryCatalog {
 static constexpr int32 PartCount=14;
 static FFoundrylinePartSpec Part(EFoundrylinePart Id);
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
