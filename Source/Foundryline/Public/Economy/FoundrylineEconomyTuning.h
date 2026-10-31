#pragma once
#include "CoreMinimal.h"
#include "Production/FoundrylineFactoryTypes.h"
#include "Orders/FoundrylineCommerceTypes.h"

// Economy tuning: every price, wage and process time of the cut-parts and welding chain in one place (economy v1,
// 29 October 2026; see Docs/Planning/ECONOMY_REVIEW_WELDING.md). Money is integer cents. Times are game minutes, which are
// plant seconds (TickCommerce adds the plant step to the commerce clock). Systems read these values; none are copied
// elsewhere. Include this from .cpp files only.
struct FOUNDRYLINE_API FFoundrylineEconomyTuning {
 // Company start and equipment.
 static constexpr int64 StartingCashCents=400000;
 static constexpr int64 OfficeWorkstationCents=50000;
 // Starter assembly expansion leaves working capital for steel and freight.
 static constexpr int64 FabStationCents=100000;
 static constexpr int64 PartsRackBaseCents=25000;
 static constexpr int64 PartsRackExtraLevelCents=12500;
 static constexpr int64 FinishedRackBaseCents=35000;
 static constexpr int64 FinishedRackExtraLevelCents=17500;
 static constexpr int64 PressBrakeCents=450000;
 static constexpr int64 ForkliftCents=120000;
 // Purchasing.
 static constexpr int64 SteelCentsPerLb=100;
 static constexpr int64 SupplierFreightCents=2500;
 static constexpr int64 FrameSectionCents=1800;
 static constexpr int64 LightKitCents=2500;
 // Customer prices for cut parts and products, and headache-rack order sizes.
 static int64 SalePriceCents(EFoundrylinePart Part);
 static constexpr int32 RackOrderMin=2;
 static constexpr int32 RackOrderMax=5;
 // Headache-rack offers kept open on Sales while the business has a welding station; refilled before cut-part offers.
 static constexpr int32 RackOffersOnSale=2;
 // Carriers that collect customer pallets: fee, lead and transit minutes, deck length.
 static FFoundrylineCarrierSpec Carrier(int32 Id);
 // Wages per game hour while a person holds a job (hiring is free; a hire without a job is not paid).
 static constexpr int64 WelderWageCentsPerHour=1800;
 static constexpr int64 OperatorWageCentsPerHour=1500;
 static constexpr int64 DriverWageCentsPerHour=1500;
 // Running costs per game minute: a machine while it cuts or forms, a forklift while it is on a job.
 static constexpr int64 MachineRunningCentsPerMinute=1;
 static constexpr int64 ForkliftJobCentsPerMinute=3;
 // Process times.
 static constexpr float SheetLoadMinutes=5;
 static constexpr float BrakeInstallMinutes=120;
 static constexpr float ForkliftInstallMinutes=30;
 static constexpr float ForkHandlingMinutes=2;
 static constexpr float StationUnloadMinutes=8;
 // Machine programs: cut or forming minutes and steel pounds per part.
 static TArray<FFoundrylineFactoryProgram> MachinePrograms();
 // Headache rack at a welding station: fit-up, welding, lights, lift to OUT (one welder), and the two-welder rate.
 static float RackStageMinutes(int32 Stage){return Stage==1?30.f:Stage==2?90.f:Stage==3?20.f:Stage==4?6.f:0.f;}
 static constexpr float TwoWelderRate=1.7f;
};
