#pragma once
#include "CoreMinimal.h"
#include "FoundrylineAssemblySupply.generated.h"

// Small-part material definitions can be extended without adding another collector algorithm.
USTRUCT(BlueprintType)
struct FFoundrylineSupplyItem {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) int32 Part=0;
 UPROPERTY(EditAnywhere) float HandlingLb=1;
 UPROPERTY(EditAnywhere) int32 MaxUnitsPerCompartment=100;
};

USTRUCT()
struct FFoundrylineSupplyCompartment {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Part=0;
 UPROPERTY(SaveGame) int32 RefillTarget=20;
 UPROPERTY(SaveGame) float CapacityLb=100;
};

USTRUCT()
struct FFoundrylineAssemblySupplyBin {
 GENERATED_BODY()
 // One default bin per area in the legacy tanker adapter; other recipes can supply their own targets.
 UPROPERTY(SaveGame) int32 Area=0;
 UPROPERTY(SaveGame) int32 Compartments=4;
 UPROPERTY(SaveGame) float PayloadLb=400;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Access=FVector::ZeroVector;
 UPROPERTY(SaveGame) float Yaw=0;
 UPROPERTY(SaveGame) TArray<FFoundrylineSupplyCompartment> Slots;
};

USTRUCT()
struct FFoundrylineCollectorLoad {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 SourcePallet=0;
 UPROPERTY(SaveGame) int32 Part=0;
 UPROPERTY(SaveGame) int32 Quantity=0;
 UPROPERTY(SaveGame) float UnitLb=0; // Actual existing inventory mass, not a second invented material ledger.
 UPROPERTY(SaveGame) FVector Pickup=FVector::ZeroVector;
 UPROPERTY(SaveGame) bool Picked=false;
};

USTRUCT()
struct FFoundrylinePartsCollector {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) int32 Area=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Home=FVector::ZeroVector;
 UPROPERTY(SaveGame) float Yaw=0;
 UPROPERTY(SaveGame) float PayloadLb=300;
 UPROPERTY(SaveGame) int32 MaxUnits=100;
 UPROPERTY(SaveGame) TArray<FFoundrylineCollectorLoad> Load;
 UPROPERTY(SaveGame) TArray<FVector> Path;
 UPROPERTY(SaveGame) float Distance=0;
 UPROPERTY(SaveGame) float Retry=0;
 UPROPERTY(SaveGame) float Handling=0;
 UPROPERTY(SaveGame) float Speed=0;
 UPROPERTY(SaveGame) bool ReleaseRequested=false;
 UPROPERTY(SaveGame) FString Status;
};

struct FFoundrylineWeldingStationRecord;
struct FOUNDRYLINE_API FFoundrylineAssemblySupplyCatalog {
 static FTransform Transform(const FFoundrylineWeldingStationRecord& Station);
 static TArray<FFoundrylineSupplyItem> Items();
 static const FFoundrylineSupplyItem* Find(int32 Part,const TArray<FFoundrylineSupplyItem>& Items);
 static FVector BinSize(int32 Compartments);
 static int32 Fit(float Pounds,float UnitLb,int32 Space);
};
