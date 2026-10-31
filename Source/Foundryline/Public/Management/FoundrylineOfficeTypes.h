#pragma once
#include "CoreMinimal.h"
#include "FoundrylineOfficeTypes.generated.h"
USTRUCT() struct FFoundrylineOfficeEmployee {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) FVector Home=FVector::ZeroVector;
};
USTRUCT() struct FFoundrylineOfficeDecoration {
 GENERATED_BODY()
 UPROPERTY(SaveGame) uint8 Kind=0;
 UPROPERTY(SaveGame) bool HiddenByGlazing=false;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) uint8 Quarter=0;
};
USTRUCT() struct FFoundrylineOfficeAssignment {
 GENERATED_BODY()
 UPROPERTY(SaveGame) int32 Id=0;
 UPROPERTY(SaveGame) FGuid BuildingId;
 UPROPERTY(SaveGame) FGuid RoomId;
 UPROPERTY(SaveGame) uint8 Floor=0;
 // Zero is the company's owner/president, never a second employee/avatar.
 UPROPERTY(SaveGame) int32 EmployeeId=0;
 UPROPERTY(SaveGame) int32 DeskId=0;
 UPROPERTY(SaveGame) uint8 FurnishingVersion=1;
 UPROPERTY(SaveGame) FVector DeskPosition=FVector::ZeroVector;
 UPROPERTY(SaveGame) uint8 DeskQuarter=0;
 UPROPERTY(SaveGame) FString Label;
 UPROPERTY(SaveGame) FString OccupantName;
 UPROPERTY(SaveGame) TArray<FFoundrylineOfficeDecoration> Decorations;
};
