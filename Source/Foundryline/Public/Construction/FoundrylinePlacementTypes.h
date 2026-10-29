#pragma once

#include "CoreMinimal.h"
#include "Pathing/FoundrylineRouteNetwork.h"
#include "FoundrylinePlacementTypes.generated.h"

UENUM()
enum class EFoundrylineOpeningKind : uint8 { Personnel, Garage, ShopWindow, TruckGarage, OfficeDoor, LoadingDock };

USTRUCT()
struct FOUNDRYLINE_API FFoundrylineOpening
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) FGuid OpeningId;
	// South, east, north, west; offset is measured along local X or Y.
	UPROPERTY(SaveGame) uint8 Edge = 0;
	UPROPERTY(SaveGame) int32 OffsetCm = 100;
	UPROPERTY(SaveGame) EFoundrylineOpeningKind Kind = EFoundrylineOpeningKind::Personnel;
	UPROPERTY(SaveGame) bool bAutomatic = false;
	// Height above the finished floor; older records keep the Tier 1 default.
	UPROPERTY(SaveGame) int32 WindowSillCm = 100;
	int32 WidthCm() const { return Kind == EFoundrylineOpeningKind::LoadingDock ? 350 : Kind == EFoundrylineOpeningKind::TruckGarage ? 500 : Kind == EFoundrylineOpeningKind::Garage ? 400 : Kind == EFoundrylineOpeningKind::ShopWindow ? 200 : Kind == EFoundrylineOpeningKind::Personnel ? 120 : 100; }
	int32 HeightCm() const { return Kind == EFoundrylineOpeningKind::LoadingDock ? 350 : Kind == EFoundrylineOpeningKind::TruckGarage ? 500 : Kind == EFoundrylineOpeningKind::Garage ? 420 : Kind == EFoundrylineOpeningKind::ShopWindow ? 180 : 250; }
	int32 BottomCm() const { return Kind == EFoundrylineOpeningKind::ShopWindow ? WindowSillCm : 0; }
};

// Local centimetre rectangles, owned and saved with their host building.
USTRUCT()
struct FOUNDRYLINE_API FFoundrylineOfficeSpace
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) FGuid SpaceId;
	UPROPERTY(SaveGame) FIntPoint Min = FIntPoint::ZeroValue;
	UPROPERTY(SaveGame) FIntPoint Max = FIntPoint::ZeroValue;
	UPROPERTY(SaveGame) TArray<FFoundrylineOpening> Openings;
};

USTRUCT()
struct FOUNDRYLINE_API FFoundrylineBuildingColors
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FLinearColor Exterior = FLinearColor(FColor::FromHex(TEXT("ED7B2D")));
 UPROPERTY(SaveGame) FLinearColor Interior = FLinearColor(FColor::FromHex(TEXT("F7F5F1")));
 UPROPERTY(SaveGame) FLinearColor Trim = FLinearColor(FColor::FromHex(TEXT("30373D")));
 bool IsValid() const {for(const auto C:{Exterior,Interior,Trim}) for(int32 I=0;I<4;++I) if(!FMath::IsFinite(C.Component(I)) || C.Component(I)<0 || C.Component(I)>1) return false;return true;}
};

USTRUCT()
struct FFoundrylineTruckZone
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FIntPoint Min=FIntPoint::ZeroValue;
 UPROPERTY(SaveGame) FIntPoint Max=FIntPoint::ZeroValue;
};

USTRUCT(BlueprintType)
struct FOUNDRYLINE_API FFoundrylinePlacedObject
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid ObjectId;
 UPROPERTY(SaveGame) FFoundrylineBuildingColors Colors;
 UPROPERTY(SaveGame) TArray<FFoundrylineTruckZone> TruckZones;
 UPROPERTY(SaveGame) TArray<FFoundrylineRouteNode> RouteNodes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid CompanyId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid PlantId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FName DefinitionId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FIntVector Anchor = FIntVector::ZeroValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	uint8 QuarterTurns = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	bool bAutoFurnished = false;
	UPROPERTY(VisibleAnywhere, SaveGame) bool bOfficeAnnex = false;
	UPROPERTY(VisibleAnywhere, SaveGame)
	TArray<FFoundrylineOpening> Openings;
	UPROPERTY(VisibleAnywhere, SaveGame)
	int32 ColumnBayCm = 600;
	UPROPERTY(VisibleAnywhere, SaveGame) bool bLandscaping = true;
	// Local longitudinal centres, snapped to metres. Zebra crossings include curb ramps.
	UPROPERTY(VisibleAnywhere, SaveGame) TArray<int32> Crosswalks;
	UPROPERTY(VisibleAnywhere, SaveGame) TArray<FFoundrylineOfficeSpace> OfficeAreas;
	UPROPERTY(VisibleAnywhere, SaveGame) TArray<FFoundrylineOfficeSpace> OfficeRooms;
};
