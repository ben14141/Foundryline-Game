#pragma once
#include "CoreMinimal.h"
#include "FoundrylineEmployeeTypes.generated.h"
UENUM(BlueprintType) enum class EFoundrylineEmployeeRole:uint8 {Operator,Welder,ForkliftDriver,Unassigned,PartsCollector};
UENUM(BlueprintType) enum class EFoundrylineEmployeeActivity:uint8 {Idle,Walking,Waiting,Operating,Welding,Driving,Handling,Paused};
UENUM(BlueprintType) enum class EFoundrylineEmployeeWait:uint8 {None,Unassigned,NoRoute,Traffic,Materials,OutputFull,Paused};
USTRUCT() struct FFoundrylineFactoryOperator {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) int32 Machine=0;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Home=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector WorkPoint=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Target=FVector::ZeroVector;
 UPROPERTY(SaveGame) TArray<FVector> Path;
 UPROPERTY(SaveGame) float Distance=0;
 UPROPERTY(SaveGame) float Yaw=0;
 UPROPERTY(SaveGame) float Speed=0;
 UPROPERTY(SaveGame) float Retry=0;
 UPROPERTY(SaveGame) EFoundrylineEmployeeActivity Activity=EFoundrylineEmployeeActivity::Idle;
 UPROPERTY(SaveGame) EFoundrylineEmployeeWait Wait=EFoundrylineEmployeeWait::None;
};
USTRUCT() struct FFoundrylineStandbyDriver {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FVector Home=FVector::ZeroVector;
 UPROPERTY(SaveGame) float Yaw=0;
};
struct FFoundrylineFactoryState;
// A projection, not a second employee entity. Driver/welder IDs come from their existing records.
struct FFoundrylineEmployeeRecord {
 int32 Id=0,Assignment=0,VehicleId=0;
 // Welding-station slot (0 Welder A, 1 Welder B) when Assignment is a station.
 int32 StationSlot=0;
 EFoundrylineEmployeeRole Role=EFoundrylineEmployeeRole::Operator;
 FGuid CompanyId,PlantId;
 FString Key,Label,CurrentJob;
 FVector Position=FVector::ZeroVector,Home=FVector::ZeroVector,WorkPoint=FVector::ZeroVector;
 TArray<FVector> Path;
 float Yaw=0,Speed=0,Distance=0,WorkPhase=0;
 EFoundrylineEmployeeActivity Activity=EFoundrylineEmployeeActivity::Idle;
 EFoundrylineEmployeeWait Wait=EFoundrylineEmployeeWait::None;
 bool bMoving=false,bWorking=false,bSeated=false;
};
struct FOUNDRYLINE_API FFoundrylineEmployeeRoster {
 static TArray<FFoundrylineEmployeeRecord> Build(const FFoundrylineFactoryState& State);
 static const TCHAR* RoleName(EFoundrylineEmployeeRole Role);
 static const TCHAR* ActivityName(EFoundrylineEmployeeActivity Activity);
 static const TCHAR* WaitName(EFoundrylineEmployeeWait Wait);
};
