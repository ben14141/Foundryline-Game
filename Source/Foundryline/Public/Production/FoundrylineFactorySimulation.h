#pragma once
#include "Production/FoundrylineFactoryTypes.h"
#include "Pathing/FoundrylineFloorLaneGeometry.h"

// Plain deterministic simulation: no meshes, RPCs, player cameras or per-part actors.
struct FOUNDRYLINE_API FFoundrylineFactorySimulation {
 FFoundrylineFactoryState State;
 TArray<FBox2D> Solids;
 // The practice scene starts stocked; recipe tests can start with raw materials only.
 void Seed(bool StockedWorkshop=false);
 void SeedCutPartsShop(bool Commerce=true);
 void AddMachineOperator(int32 Machine,bool StartAtWork=false);
 void MigrateEmployees();
 void MigrateEmployeeNames();
 void MigrateGenericEmployees();
 // Schema 9 gains welding stations; schema 10 gains bought assembly parts (zone part arrays grow) and station supplies.
 void MigrateWeldingStations();
 static constexpr int32 HireLimitTotal=12;
 static constexpr int32 HireLimitPerRole=4;
 int32 EmployeeHireCapacity(EFoundrylineEmployeeRole Role)const;
 bool HireEmployee(EFoundrylineEmployeeRole Role,FGuid RequestId,FString& Error);
 bool AssignDriverEmployee(int32 Employee,int32 Vehicle,FString& Error);
 FString NewEmployeeName(int32 Id,bool Random=true)const;
 bool FindEmployeeWaitingPosition(FVector& Position)const;
 void UpdateOperators(float Dt);
 bool CanEmployeeAdvance(FVector From,FVector To,int32 EmployeeId)const;
 bool TryEmployeePassing(FVector From,FVector To,int32 EmployeeId,TArray<FVector>& Path,float& Distance)const;
 bool AssignEmployee(int32 Id,EFoundrylineEmployeeRole Role,int32 Target,FString& Error);
 bool AssignEmployeeInternal(int32 Id,EFoundrylineEmployeeRole Role,int32 Target,FString& Error);
 bool MachineStaffReady(int32 Machine)const;
 bool EmployeesValid(FString& Error)const;
 FVector OperatorWorkPoint(int32 Machine)const;
 void AddOperatorAccessLanes(int32 Machine);
 bool CommerceCommand(const FFoundrylineCommerceCommand& Command,FString& Error);
 void SeedCommerce();
 void TickCommerce(float Dt);
 void PlanCommerce();
 bool CommerceValid(FString& Error)const;
 bool InstallBrake(FString& Error);
 void CommerceExpense(int64 Cents,const FString& Label);
 void CommerceIncome(int64 Cents,const FString& Label);
 void RefreshCommerceOffers();
 static FVector PickupPosition(int32 Slot);
 static FVector CarrierPalletPosition(int32 Slot);
 static FVector CarrierDock(int32 Slot);
 void Step(float Seconds);
 bool Valid(FString& Error)const;
 bool PaintLane(EFoundrylineFloorLane Kind,FVector A,FVector B,float Width,int32 Direction,FString& Error);
 bool AddZone(FVector A,FVector B,FString& Error);
 bool AddPresetZone(FVector Center,int32 Preset,int32 QuarterTurns,int32 Side,int32 Route,FString& Error);
 bool CanPlaceZone(const FFoundrylineFactoryZone& Zone,FString& Error)const;
 // Validates a candidate and chooses its reachable forklift dock offsets.
 bool CanPlaceWeldingStation(FFoundrylineWeldingStationRecord& Station,FString& Error)const;
 bool WeldingStationAccess(FFoundrylineWeldingStationRecord& Station,FString& Error)const;
 bool AddWeldingStation(FVector Center,int32 QuarterTurns,bool SwapSides,FString& Error);
 bool RemoveWeldingStation(int32 Id,FString& Error);
 bool WeldingStationsValid(FString& Error)const;
 bool OverlapsWeldingStation(const FBox2D& Box,bool IncludeApproaches)const;
 // Recipe 0 or batch 0 stops new supply requests; parts already in the bins stay there.
 bool ConfigureWeldingStation(int32 Id,int32 Recipe,int32 Batch,FString& Error);
 // Units of a part committed to a station: in its bins, on its IN port, on a forklift heading there or still on a booked supplier truck.
 int32 StationStock(const FFoundrylineWeldingStationRecord& Station,EFoundrylinePart Part,bool IncludeIncoming=true)const;
 // Bounded buffer: two kits of the recipe, capped by the station's bins. Zero when the station needs no more parts.
 int32 StationTarget(const FFoundrylineWeldingStationRecord& Station,EFoundrylinePart Part)const;
 int32 StationShortfall(const FFoundrylineWeldingStationRecord& Station,EFoundrylinePart Part)const{return FMath::Max(0,StationTarget(Station,Part)-StationStock(Station,Part));}
 // Free stock a forklift could bring from this business's racks.
 int32 StationRackSupply(EFoundrylinePart Part)const;
 FString WeldingStationStatus(const FFoundrylineWeldingStationRecord& Station)const;
 bool IsStationStock(const FFoundrylineFactoryPallet& Pallet)const;
 bool AssignSlot(int32 Slot,EFoundrylinePart Part,FString& Error);
 bool Order(int32 Zone,FString& Error);
 bool Collect(int32 Zone,FString& Error);
 bool ReceiveSteel(int32 Sheets,FString& Error);
 bool ReceiveKits(FString& Error);
 bool AddDriver(int32 Route,FString& Error,bool Staff=true);
 bool AddWelder(int32 Zone,FString& Error);
 bool Program(int32 Machine,EFoundrylinePart Part,int32 Batch,bool Auto,FString& Error);
 TArray<FVector> Path(FVector Start,FVector End,bool Pedestrian=false)const;
 int32 Available(EFoundrylinePart Part,EFoundrylinePalletPlace Place,int32 Owner=0)const;
 int32 Incoming(EFoundrylinePart Part,int32 Owner)const;
 int32 Total(EFoundrylinePart Part)const;
 float SteelMass()const;
 bool ReadyForWork(const FFoundrylineFactoryZone& Zone)const;
 static FVector Along(const TArray<FVector>& Points,float Distance,float* Yaw=nullptr);
 static float Length(const TArray<FVector>& Points);
 FVector BufferPosition(const FFoundrylineFactoryZone& Zone,EFoundrylinePart Part)const;
 FString ZoneStatus(const FFoundrylineFactoryZone& Zone)const;
 const TArray<FFoundrylineFloorCrossing>& FloorCrossings()const;
private:
 mutable uint32 CrossingHash=0;
 mutable TArray<FFoundrylineFloorCrossing> CachedCrossings;
 void Plan();
 void PlanWeldingStations();
 void UnloadWeldingStations(float Dt);
 bool StationPortBusy(const FFoundrylineWeldingStationRecord& Station)const;

 void Machines(float Dt);
 void Assembly(float Dt);
 void Dispatch();
 void Travel(float Dt);
 void ReplanYield(FFoundrylineFactoryDriver& Driver,FVector Target);
 bool MakeJob(int32 Pallet,int32 Destination,EFoundrylinePalletPlace Place,int32 Count,FVector Dock,FVector Store,int32 Priority);
 int32 AddPallet(EFoundrylinePart Part,int32 Count,EFoundrylinePalletPlace Place,int32 Owner,FVector Position,float UnitLb=0);
 bool Consume(EFoundrylinePart Part,int32 Count,EFoundrylinePalletPlace Place,int32 Owner,float* Mass=nullptr);
 int32 Wanted(EFoundrylinePart Part)const;
 bool Walkable(FVector2D Point,bool Pedestrian)const;
};
