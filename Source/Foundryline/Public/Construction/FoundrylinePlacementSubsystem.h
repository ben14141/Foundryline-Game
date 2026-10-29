#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Construction/FoundrylinePlaceableDefinition.h"
#include "SaveSystem/FoundrylinePlacementSave.h"
#include "Networking/FoundrylineActionContext.h"
#include "FoundrylinePlacementSubsystem.generated.h"

UENUM()
enum class EFoundrylinePlacementAction : uint8 { Build, Move, Rotate, Remove, AddOpening, RemoveOpening, ReplaceOpenings, SetColumnBay, ReplaceObject, RestoreRemoved, SetLandscaping, SetCrosswalks, SetBuildingColors, Duplicate, SetTruckZones, SetRouteNodes };
enum class EFoundrylinePlacementError : uint8
{
	None, NotAuthority, StaleRevision, InvalidIdentity, UnknownDefinition, InvalidDefinition,
	InvalidTransform, InvalidRegion, AccessDenied, Occupied, UnknownObject, InvalidAction,
	UnsupportedSave, DuplicateId, TooManyActions, CostOverflow, CostLimitExceeded, Busy, Pending
};

USTRUCT()
struct FOUNDRYLINE_API FFoundrylinePlacementRequest
{
 GENERATED_BODY()
	UPROPERTY() EFoundrylinePlacementAction Action = EFoundrylinePlacementAction::Build;
	UPROPERTY() int64 ExpectedRevision = 0;
	UPROPERTY() FGuid ObjectId;
	UPROPERTY() FName DefinitionId;
	UPROPERTY() FIntVector Anchor = FIntVector::ZeroValue;
	UPROPERTY() uint8 QuarterTurns = 0;
	UPROPERTY() FFoundrylineOpening Opening;
	UPROPERTY() TArray<FFoundrylineOpening> Openings;
	UPROPERTY() int32 ColumnBayCm = 600;
	UPROPERTY() FFoundrylinePlacedObject Replacement;
 UPROPERTY() FFoundrylineBuildingColors Colors;
 UPROPERTY() TArray<FFoundrylineTruckZone> TruckZones;
 UPROPERTY() TArray<FFoundrylineRouteNode> RouteNodes;
	UPROPERTY() bool bLandscaping = true;
	UPROPERTY() bool bOfficeAnnex = false;
	UPROPERTY() TArray<FFoundrylineOfficeSpace> OfficeAreas;
	UPROPERTY() TArray<int32> Crosswalks;
};

struct FOUNDRYLINE_API FFoundrylinePlacementResult
{
	EFoundrylinePlacementError Error = EFoundrylinePlacementError::None;
	FGuid ObjectId;
	int64 Revision = 0;
	bool IsSuccess() const { return Error == EFoundrylinePlacementError::None; }
};

struct FOUNDRYLINE_API FFoundrylinePlacementChange
{
	FGuid ObjectId;
	bool bHadPrevious = false;
	bool bHasCurrent = false;
	FFoundrylinePlacedObject Previous;
	FFoundrylinePlacedObject Current;
	FFoundrylineGridBounds PreviousBounds;
	FFoundrylineGridBounds CurrentBounds;
	bool bAffectsNavigation = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FFoundrylinePlacementChanged, const FFoundrylinePlacementChange&);

struct FOUNDRYLINE_API FFoundrylinePlanResult
{
	EFoundrylinePlacementError Error = EFoundrylinePlacementError::None;
	int32 FailedAction = INDEX_NONE;
	int64 CostCents = 0;
	int64 Revision = 0;
	TArray<FFoundrylinePlacementResult> Actions;
	bool IsSuccess() const { return Error == EFoundrylinePlacementError::None; }
};

// Data authority only: no mesh spawning, per-frame tick, networking transport or cash spending.
UCLASS()
class FOUNDRYLINE_API UFoundrylinePlacementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	bool RegisterDefinition(UFoundrylinePlaceableDefinition* Definition);
	// Bootstrap empty sites only; live land/surface edits need their own transaction later.
	EFoundrylinePlacementError InitializeRegions(const TArray<FFoundrylineBuildRegion>& InRegions);
	FFoundrylinePlacementResult Validate(const FFoundrylinePlacementRequest& Request,
		const FFoundrylinePlacementContext& Context) const;
	FFoundrylinePlacementResult Execute(const FFoundrylinePlacementRequest& Request,
		const FFoundrylinePlacementContext& Context);
	// Quotes do not mutate state. All requests reference the same current world revision.
	FFoundrylinePlanResult PreviewBatch(const TArray<FFoundrylinePlacementRequest>& Requests,
		const FFoundrylinePlacementContext& Context,bool bLocalPreview=false) const;
	// All-or-nothing placement commit. Cost cap is confirmation, NOT a bank balance.
	FFoundrylinePlanResult ExecuteBatch(const TArray<FFoundrylinePlacementRequest>& Requests,
		const FFoundrylinePlacementContext& Context, int64 MaxAuthorizedCostCents);
	FFoundrylinePlacementChanged OnPlacementChanged;
	UFoundrylinePlacementSave* CreateSnapshot() const;
	// All-or-nothing validation. Live host replacement advances revision and emits visual changes; bootstrap remains the default.
	EFoundrylinePlacementError RestoreSnapshot(const UFoundrylinePlacementSave* Snapshot, bool bReplaceExisting=false);
 bool EnsureGeneratedDefinition(FName DefinitionId);
 // Client mirror ingestion only; never grants execution authority.
 EFoundrylinePlacementError ApplyNetworkSnapshot(const TArray<FFoundrylinePlacedObject>& InObjects,const TArray<FFoundrylineBuildRegion>& InRegions,int64 InRevision);
 EFoundrylinePlacementError RegisterBusinessRegions(const TArray<FFoundrylineBuildRegion>& Additional);
 EFoundrylinePlacementError RegisterBusinessRegion(const FFoundrylineBuildRegion& Region){return RegisterBusinessRegions({Region});}
 const TArray<FFoundrylineBuildRegion>& GetRegions() const {return Regions;}
	const TArray<FFoundrylinePlacedObject>& GetObjects() const { return Objects; }
	int64 GetRevision() const { return Revision; }
	const UFoundrylinePlaceableDefinition* FindDefinition(FName DefinitionId) const;

private:
	bool IsAuthority() const;
	FFoundrylinePlacementResult ValidateAgainst(const FFoundrylinePlacementRequest& Request,
		const FFoundrylinePlacementContext& Context, const TArray<FFoundrylinePlacedObject>& Against,bool bLocalPreview=false) const;
	FFoundrylinePlanResult StageBatch(const TArray<FFoundrylinePlacementRequest>& Requests,
		const FFoundrylinePlacementContext& Context, TArray<FFoundrylinePlacedObject>& Staged,
		TArray<FFoundrylinePlacementChange>& Changes,bool bLocalPreview=false) const;
	static EFoundrylinePlacementError ValidateRegions(const TArray<FFoundrylineBuildRegion>& InRegions);
	EFoundrylinePlacementError ValidateObject(const FFoundrylinePlacedObject& Candidate,
		const TArray<FFoundrylinePlacedObject>& Against,
		const TArray<FFoundrylineBuildRegion>& InRegions) const;
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UFoundrylinePlaceableDefinition>> Definitions;
	UPROPERTY(Transient)
	TArray<FFoundrylinePlacedObject> Objects;
	UPROPERTY(Transient)
	TArray<FFoundrylineBuildRegion> Regions;
	int64 Revision = 0;
	bool bApplyingChanges = false;
	// Server-owned tombstones: undo cannot inject arbitrary identities or ownership.
	TMap<FGuid, FFoundrylinePlacedObject> RemovedObjects;
};
