#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Networking/FoundrylinePropertyState.h"
#include "FoundrylineBuildSite.generated.h"
class UMaterialInterface;

UCLASS()
class FOUNDRYLINE_API AFoundrylineBuildSite : public AActor
{
	GENERATED_BODY()
public:
	AFoundrylineBuildSite();
	virtual void BeginPlay() override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 UFUNCTION(BlueprintPure,Category="Property") FString GetPropertyDigest() const;
 UFUNCTION(BlueprintPure,Category="Property") int64 GetPropertyRevision() const;
 bool EditRouteNetwork(FName Action,FGuid Selected,FVector2D Point,FGuid& OutNode,bool TwoWay=false,bool Reverse=false);
 UFUNCTION(BlueprintCallable,Category="Logistics") bool EditTruckRouting(FIntVector Start,FIntVector End,bool Erase=false);
 bool UseDoor(class APawn* User,FGuid ObjectId,FGuid OpeningId);
 void PublishNetworkState();
 void ReceiveConstructionResult(EFoundrylinePlacementError Error);
 FFoundrylinePlanResult ApplyRemoteConstruction(class APlayerController* Player,const TArray<FFoundrylinePlacementRequest>& Requests);
 UFUNCTION(BlueprintPure,Category="Construction") FString GetConstructionFeedbackTitle() const;
 UFUNCTION(BlueprintPure,Category="Construction") FString GetConstructionFeedbackText() const;
 UFUNCTION(BlueprintPure,Category="Construction") bool IsConstructionFeedbackError() const;
 void ClearConstructionFeedback();
 static FString PlacementErrorText(EFoundrylinePlacementError Error);

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UFUNCTION(BlueprintCallable,Category="Construction") bool BuildRectangle(FName Tool, FIntVector Start, FIntVector End);
	void PreviewRectangle(FName Tool, FIntVector Start, FIntVector End);
	bool PlaceDoor(FName Tool, FIntVector Cursor);
	bool AdjustColumns(FIntVector Cursor, int32 SpacingCm=0);
	bool bLargeShowcase = false;
 bool bStarterProperty = false;
 bool bProductionWorkshop = false;
 bool bCutPartsStarter = false;
	void PreviewDoor(FName Tool, FIntVector Cursor);
	void PreviewColumns(FIntVector Cursor);
	void HidePreview();
	void Undo();
 void Redo();
 bool CanUndo() const {return !UndoStack.IsEmpty();}
 bool CanRedo() const {return !RedoStack.IsEmpty();}
 const FFoundrylinePlacedObject* GetSelectedObject() const;
 const FFoundrylineOpening* GetSelectedOpening() const;
 FString GetSelectedItemLabel() const;
 bool SetSelectedSign(const FString& Name,float Facing,const FFoundrylineBuildingColors& Colors);
 bool PavementShape(FName Mode,const TArray<FVector2D>& WorldPoints,float WidthCm=731.52f,uint8 Lanes=0,bool Truck=false,bool Curbs=true,bool Preview=false);
 bool AddPavementMark(EFoundrylinePavementMark Kind,const TArray<FVector2D>& WorldPoints,float WidthCm,bool Preview=false);
 bool PavementOption(FName Action);
 bool SelectPavementMark(FIntVector Cursor);
 FGuid SelectedPavementMark;
 float DraftEntranceRadiusCm=304.8f;
 double PavementPreviewAreaFt2=0;
 FFoundrylinePlacedObject LastPavementPreview;
 bool bPavementPreviewActive=false;
 bool RenderRoadsideJoins(AActor* Actor,const FFoundrylinePlacedObject& Object);
 AActor* RenderPavement(const FFoundrylinePlacedObject& Object,bool Preview,bool Valid,bool PaintOnly=false);
 const FFoundrylinePlacedObject* ObjectAt(FIntVector Cursor) const {return Pick(Cursor);}
 // Selection/hover outlines are recorded here each frame and drawn by the HUD, so they survive Shipping builds.
 struct FSelectionOutline{FVector2D Min,Max;float Z=0;bool Hover=false;float Height=0;bool Blocked=false;};
 TArray<FSelectionOutline> SelectionOutlines;
 void RecordSelection(FIntVector Cursor,bool Hover);
 UFUNCTION(BlueprintCallable,Category="Property") void SaveProperty(const FString& Slot=TEXT("Property01"));
 UFUNCTION(BlueprintCallable,Category="Property") void LoadProperty(const FString& Slot=TEXT("Property01"));
 bool IsPropertyBusy() const;
 bool HasUnsavedChanges() const;
 FString PropertyStatus=TEXT("NOT SAVED");
 void SetRoofsVisible(bool Visible);
 bool AreRoofsVisible() const {return bRoofsVisible;}
 UFUNCTION(BlueprintCallable,Category="Construction") bool PaintSelected(int32 Channel,FLinearColor Color);
 const FFoundrylinePlacedObject* GetSelectedBuilding() const;
	bool OfficeRectangle(FName Tool,FIntVector Start,FIntVector End,bool bPreview=false);
	bool OfficeOpening(FName Tool,FIntVector Cursor,bool bPreview=false);
	UFUNCTION(BlueprintCallable,Category="Construction") bool BeginEdit(FName Tool, FIntVector Cursor);
	void PreviewEdit(FName Tool, FIntVector Start, FIntVector End);
	bool CommitEdit(FName Tool, FIntVector Start, FIntVector End);
	bool ApplyGroundTool(FName Tool, FIntVector Cursor, bool bPreview=false);
	FGuid SelectedObject;
	static bool IsEditTool(FName Tool) {return Tool==TEXT("UpgradeBay") || Tool==TEXT("Select") || Tool==TEXT("Move") || Tool==TEXT("Resize") || Tool==TEXT("Demolish") || Tool==TEXT("MoveDoor") || Tool==TEXT("Rotate") || Tool==TEXT("Duplicate") || Tool==TEXT("PaveRound") || Tool==TEXT("PaveBend");}
	FFoundrylineGridBounds PreviewBounds;
 TArray<FVector> PreviewJoinLines;
 bool bHasPreviewBounds=false;
 int32 DraftTruckWidthCm=2400;
 uint8 DraftAreaRotation=0;
 bool bExactDraftPlacement=false;
 bool IsReady() const { return bReady; }
 // Company/plant of the local player; the trusted server re-checks every commit.
 FFoundrylinePlacementContext LocalContext() const;
	FString Status = TEXT("Choose Building or a road type, then drag on the ground");
	static constexpr int32 SlabHeightCm = 20;
	static constexpr int32 WallThicknessCm = 30;
	static constexpr int32 WallHeightCm = 600;
	static constexpr int32 HighBayWallHeightCm = 1200;
private:
 FFoundrylinePlacementResult ValidateConstruction(const FFoundrylinePlacementRequest& Request);
 FFoundrylinePlanResult ExecuteConstructionBatch(const TArray<FFoundrylinePlacementRequest>& Requests);
 FFoundrylinePlacementResult ExecuteConstruction(const FFoundrylinePlacementRequest& Request);
 void RecordConstructionResult(EFoundrylinePlacementError Error,bool Commit);
 bool SentToHost(EFoundrylinePlacementError Error);
 void DraftBlocked(const FString& Reason,bool Commit=false);
 bool HasActionFeedback() const;
 bool bHasPreviewFeedback=false;
 EFoundrylinePlacementError PreviewError=EFoundrylinePlacementError::None;
 FString DraftReason;
 EFoundrylinePlacementError ActionError=EFoundrylinePlacementError::None;
 FString ActionReason;
 double ActionFeedbackUntil=-1;
 UPROPERTY(ReplicatedUsing=ReceiveNetworkState) FFoundrylinePropertyState NetworkState;
 UFUNCTION() void ReceiveNetworkState();
 void DrawLotBoundary();
 UPROPERTY(Transient) TObjectPtr<AActor> BoundaryActor;
	const FFoundrylinePlacedObject* Pick(FIntVector Cursor) const;
	const FFoundrylinePlacedObject* PickOffice(FIntVector Cursor) const;
	bool EditRequest(FName Tool,FIntVector Start,FIntVector End,FFoundrylinePlacementRequest& Out);
	FFoundrylineOpening SelectedOpening;
 bool PavementEditRequest(FName Tool,FIntVector Start,FIntVector End,FFoundrylinePlacementRequest& Request);
 bool CommitPavementBatch(const TArray<FFoundrylinePlacementRequest>& Requests);
 TMap<FGuid,TArray<FFoundrylinePlacementRequest>> CompoundHistory;
 FString PavementInputKey;
 bool PavementInputValid=false;
 FGuid SelectedRoom;
 const FFoundrylinePlacedObject* PickOpening(FIntVector Cursor,FFoundrylineOpening& Opening,FGuid& Room) const;
 bool OpeningEditRequest(FIntVector Cursor,bool Remove,FFoundrylinePlacementRequest& Request) const;
 void RecordOpeningOutline(const FFoundrylinePlacedObject& Object,const FFoundrylineOpening& Opening,FGuid Room,bool Hover,bool Valid=true);
	// SnapEdges selects which rectangle edges may magnetize; bExtend allows dragging out of a building.
	bool RectangleRequest(FName Tool, FIntVector Start, FIntVector End, FFoundrylinePlacementRequest& Out,uint8 SnapEdges=15,bool bExtend=true);
	TArray<FFoundrylineGridBounds> SnapTargets(bool bBuilding,FGuid Exclude) const;
	void SnapRoadEnds(bool Horizontal,int32 Width,FIntVector& Center,FIntVector& A,FIntVector& B) const;
	bool DoorRequest(FName Tool, FIntVector Cursor, FFoundrylinePlacementRequest& Out) const;
	void Changed(const FFoundrylinePlacementChange& Change);
	AActor* RenderManufacturingSign(const FFoundrylinePlacedObject& Object,bool Preview,bool Valid);
	AActor* Render(const FFoundrylinePlacedObject& Object, bool bPreview=false, bool bValid=true, bool bOpeningOnly=false, bool bColumnsOnly=false, bool bOfficeOnly=false);
	void ShowPreview(const FFoundrylinePlacedObject& Object, bool bValid, const FString& Key);
	void ClearGrass(const FFoundrylineGridBounds& Bounds, int32 MarginCm=30);
 void ClearPavementGrass(const FFoundrylinePlacedObject& Object);
	void SeedSite();
 void SeedStarterProperty();
 void SeedProductionWorkshop();
 void SeedCutPartsShop();
 void AddCeilingLights(AActor* Actor,const FFoundrylinePlacedObject& Object,FVector Size);
	UPROPERTY(Transient) TObjectPtr<UFoundrylinePlacementSubsystem> Placement;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UMaterialInterface>> Materials;
	UPROPERTY(Transient) TMap<FGuid, TObjectPtr<AActor>> Visuals;
	UPROPERTY(Transient) TObjectPtr<AActor> PreviewActor;
	FFoundrylinePlacementContext Context;
	TArray<FFoundrylinePlacementRequest> UndoStack;
 TArray<FFoundrylinePlacementRequest> RedoStack;
 void RecordUndo(const FFoundrylinePlacementRequest& Request);
 void ReplayHistory(bool Redo);
 int64 SavedRevision=-1;
 int64 SavingRevision=-1;
 FDelegateHandle SaveHandle;
 void SaveCompleted(const struct FFoundrylineSaveResult& Result);
	FDelegateHandle ChangeHandle;
	FString PreviewKey;
 TMap<FGuid,bool> DoorStates;
 bool bRoofsVisible=false;
	bool bReady=false;
	bool bConnected=false;
};
