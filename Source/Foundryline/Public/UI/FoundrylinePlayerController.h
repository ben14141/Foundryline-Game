#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Grid/FoundrylineGrid.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Pathing/FoundrylineVehiclePath.h"
#include "Logistics/FoundrylineLogisticsTrip.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "FoundrylinePlayerController.generated.h"

class AFoundrylineFreightRig;
class AFoundrylineMaterialHandlingCell;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFoundrylineCellSelected, FIntVector, AnchorCm, EFoundrylineSnap, Snap);

UCLASS()
class FOUNDRYLINE_API AFoundrylinePlayerController : public APlayerController
{
	GENERATED_BODY()
public:
 bool bPropertyViewAccess=false;
 int32 PlannerKind=0,PlannerId=0;
 FString PlannerItemName,PlannerStatus;
 bool bPlannerMoving=false,bPlannerDrag=false,bPlannerPending=false,bPlannerValid=false;
 FIntVector PlannerGrab=FIntVector::ZeroValue;
 FVector PlannerAt=FVector::ZeroVector,PlannerOrigin=FVector::ZeroVector;
 bool SelectPlannerItem(FIntVector Cursor);
 void StartPlannerMove(FIntVector Cursor,bool Drag);
 void PlannerAction(FName Action);
 void SignAction(FName Action);
 void FinishSignName(const FString& Name,bool Apply);
 TSharedPtr<class SWidget> SignNameWidget;
 bool HandlePlannerInput(const FInputKeyEventArgs& Params);
 void TickPlannerSelection(bool HasCursor,FIntVector Cursor);
 bool IsPavementTool() const {return BuildTool.ToString().StartsWith(TEXT("Pave"));}
 bool HandlePavementInput(const FInputKeyEventArgs& Params);
 void TickPavement();
 void PavementAction(FName Action);
 bool PaveCurved=false,PaveCurbs=true,PaveTruck=false,PaveAwaitBend=false,PaveFlipMark=false,PavePaintAngleSnap=true;
 uint8 PaveLanes=2;
 float PaveWidthCm=731.52f,PaveMarkDepthCm=550;
 EFoundrylinePavementMark PaveMark=EFoundrylinePavementMark::Parking;
 TArray<FVector2D> PavePoints;
 int PaveVehicle=-1;
 int PavePalettePage=0; // Pave, Paint, Edit.
 bool bPaveAdvanced=false,bPaveDirectDrag=false;
 FName PaveDirectTool;
 FGuid PaveDirectObject;
 FBox2D PavePanelBounds=FBox2D(ForceInit);
 bool FindPavementHandle(FIntVector& At,FName& Tool,FString& Hint) const;
 TArray<FVector> PaveSweepLines;
 FString PaveSweepStatus,PaveSweepKey;
 bool PaveSweepClear=false;
 void UpdatePavementSweep(const TArray<FVector2D>& WorldControls,float Width,bool Curbs,int Lanes=0);

 void EnterPropertyView();
 UFUNCTION(Server,Reliable) void ServerEnterPropertyView();
 UFUNCTION(Client,Reliable) void ClientPropertyViewMode(bool Active);
 bool IsNearOfficeComputer()const;
 int32 OperationsSort=0,OperationsDocumentTab=0,OperationsWorkspaceTab=0;
 bool bOperationsSortDescending=false;
 FGuid OperationsHireRequest;
 int32 OperationsEmployeeChoice=0,OperationsJobRole=0,OperationsJobPage=0;
 int32 OperationsScreen=0,OperationsPage=0,OperationsOrder=0,OperationsBooking=0,OperationsCarrier=1,OperationsSheets=2,OperationsListPage=0,OperationsInspect=0;
 double OperationsDelay=60;
 bool bOperationsLocked=false,bOperationsPending=false;
 FString OperationsStatus;
 // Local confirmation toasts: success text is shown only after the server accepts (or the snapshot shows the change).
 FString OperationsConfirm,OperationsPendingConfirm;double OperationsConfirmAt=-100,OperationsStatusAt=-100,OperationsWatchAt=0;
 uint8 OperationsWatch=0;int32 OperationsWatchId=0,OperationsWatchTarget=0;
 void ConfirmOperations(const FString& Text);
 TMap<int32,int32> OperationsPallets;
 uint32 OperationsSequence=0,LastOperationsSequence=0;
 double OperationsRateStart=0;int32 OperationsRate=0;
 bool HandleOperationsInput(const FInputKeyEventArgs& Params);
 void OpenOperations(int32 Screen);
 UFUNCTION(BlueprintCallable,Category="Operations") void OperationsAction(FName Action);
 void SendOperations(FFoundrylineCommerceCommand Command);
 UFUNCTION(Server,Reliable) void ServerOperationsCommand(const FFoundrylineCommerceCommand& Command,uint32 Sequence);
 UFUNCTION(Client,Reliable) void ClientOperationsResult(uint32 Sequence,bool Success,const FString& Error);
 bool bFactoryMenu=false,bFactoryOverlay=false,bFactoryDragging=false,bFactoryAuto=true;
 int32 FactoryPage=0,FactoryMachine=0,FactoryProgramIndex=0,FactoryBatch=10,FactoryZone=0,FactorySlot=0,FactoryRoute=1,FactoryEndpoint=0,FactoryDirection=0;
 int32 FactoryZonePreset=2,FactoryZoneQuarter=0,FactoryZoneSide=1;
 // Welding station tool: rotation, IN/OUT swap and the selected placed station.
 int32 FactoryStation=0,FactoryStationQuarter=0;
 bool bFactoryStationSwap=false;
 // Placement checks route forklifts and people, so the preview reuses a result until the snapped pose or layout changes.
 FString FactoryStationCheckKey,FactoryStationCheckError;bool bFactoryStationCheckValid=false;double FactoryStationCheckAt=-10;
 UPROPERTY(Transient) TArray<TObjectPtr<class AStaticMeshActor>> FactoryStationPreview;
 // Flat IN / OUT labels on the station hologram's port bays.
 UPROPERTY(Transient) TArray<TObjectPtr<class UTextRenderComponent>> FactoryStationLabels;
 bool bFactoryPresetValid=false;
 FString FactoryPlacementHint;
 float FactoryWidth=365.76f;
 int32 FactorySnapIndex=1,FactoryGapIndex=0;
 bool bFactoryEdgeSnap=true,bFactoryGrid=true,bFactorySnapped=false;
 FVector FactoryPreviewA=FVector::ZeroVector,FactoryPreviewB=FVector::ZeroVector;
 FBox2D FactoryPreviewBounds=FBox2D(FVector2D::ZeroVector,FVector2D(1));
 FString FactoryBrushDimensions;
 double FactoryGridStep()const;
 bool FactoryFloorCursor(FVector& Out)const;
 bool PreviewFactoryFloor(FVector Cursor);
 bool CommitFactoryFloorBrush();
 UPROPERTY(Transient) TObjectPtr<class AFoundrylineFloorGuide> FactoryFloorGuide;
 FName FactoryTool;
 int32 FactorySelectedPart=-1,FactoryStoragePage=0;
 FVector FactoryDragStart=FVector::ZeroVector,FactoryDragEnd=FVector::ZeroVector;
 FString FactoryStatus;
 FName FactoryEditField;
 FString FactoryEditText;
 void CommitFactoryNumber();
 class AFoundrylineFactoryPlant* FactoryPlant()const;
 bool FactoryOverHUD()const;
 bool HandleFactoryInput(const FInputKeyEventArgs& Params);
 void TickFactory(float Dt);
 int32 FocusedComputer()const;
 void OpenFactoryComputer(int32 Id);
 UFUNCTION(BlueprintCallable,Category="Factory") void FactoryAction(FName Action);
 UFUNCTION(BlueprintPure,Category="Factory") int32 GetOpenFactoryComputer()const{return FactoryMachine;}
 UFUNCTION(BlueprintPure,Category="Factory") FString GetFactoryFeedback()const{return FactoryStatus;}
 UFUNCTION(Server,Reliable) void ServerFactoryCommand(const FFoundrylineFactoryCommand& Command,uint32 Sequence);
 UFUNCTION(Client,Reliable) void ClientFactoryReply(const FString& Error);
 void SendFactory(FFoundrylineFactoryCommand Command);
 uint32 FactorySequence=0,LastFactorySequence=0;
 double FactoryRateWindow=0;
 int32 FactoryRateCount=0;
 bool bFactoryInputLocked=false;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> FactoryProductPreview;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> FactoryWelderPreview;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> FactorySupplyPreview;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> FactoryEquipmentPreview;
 UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> FactoryEquipmentAccessPreview;
 int32 FactoryEquipmentChoice=1,FactoryRackLevels=1;
	AFoundrylinePlayerController();
 void ToggleMaterialHandling();
 void TickMaterialHandling(float Dt);
 bool HandleMaterialInput(const FInputKeyEventArgs& Params);
 UPROPERTY(Transient) TObjectPtr<AFoundrylineMaterialHandlingCell> LocalMaterialCell;
 UFUNCTION(Server,Reliable,BlueprintCallable,Category="Material Handling") void ServerMaterialCommand(AFoundrylineMaterialHandlingCell* Cell,uint8 Action);
 UFUNCTION(Server,Unreliable,BlueprintCallable,Category="Material Handling") void ServerMaterialDrive(AFoundrylineMaterialHandlingCell* Cell,float Throttle,float Steering,float Lift);
 bool MaterialForward=false,MaterialReverse=false,MaterialLeft=false,MaterialRight=false,MaterialUp=false,MaterialDown=false;
 float MaterialInputClock=0,MaterialClaimClock=0;
 void ToggleFreightDrive();
 void TickFreightDrive(float Dt);
 bool HandleFreightInput(const FInputKeyEventArgs& Params);
 UPROPERTY(Transient) TObjectPtr<AFoundrylineFreightRig> LocalFreightRig;
 UFUNCTION(Server,Reliable,BlueprintCallable,Category="Logistics") void ServerFreightCommand(AFoundrylineFreightRig* Rig,uint8 Action);
 UFUNCTION(Server,Unreliable,BlueprintCallable,Category="Logistics") void ServerFreightDrive(AFoundrylineFreightRig* Rig,float Throttle,float Steering);
 bool FreightForward=false,FreightReverse=false,FreightLeft=false,FreightRight=false;
 float FreightInputClock=0,FreightClaimClock=0;

	virtual void PlayerTick(float DeltaSeconds) override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	UFUNCTION(BlueprintPure, Category="Grid")
	bool GetCursorAnchor(FIntVector& OutAnchorCm) const;
	UPROPERTY(BlueprintAssignable, Category="Grid")
	FFoundrylineCellSelected OnCellSelected;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grid")
	EFoundrylineSnap ActiveSnap = EFoundrylineSnap::Structural100cm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grid")
	bool bShowPlanningGrid = false;
	FName BuildTool;
 EFoundrylineSnap GetPlacementSnap() const;
	void ChooseTool(FName Tool);
 bool bExactSurface=false,bRotateSurface=false;
 TMap<FName,FIntPoint> SurfaceSizes;
 FIntPoint GetConstructionSizeCm() const;
 UFUNCTION(BlueprintCallable,Category="Construction") void SetConstructionSize(int32 WidthMeters,int32 DepthMeters,bool Exact);
 bool SurfaceSizingAction(FName Action);
 FIntVector ExactSurfaceEnd(FIntVector Start) const;
 void ApplySurfaceDraft();
 void RotateSurfaceDraft();
 UFUNCTION(BlueprintCallable,Category="View") void ToggleWalk();
 UFUNCTION(BlueprintCallable,Category="View") void ToggleRoofs();
 UFUNCTION(BlueprintPure,Category="View") bool AreRoofsShown() const {return IsWalking() || bTopDownRoofsVisible;}
 bool IsWalking() const;
 UFUNCTION(BlueprintCallable,Category="Logistics") void ToggleLogisticsOverlay();
 UFUNCTION(BlueprintCallable,Category="Logistics") void ToggleLogisticsEdit();
 UFUNCTION(BlueprintPure,Category="Logistics") bool IsLogisticsOverlayVisible() const {return bLogisticsOverlay;}
 UFUNCTION(BlueprintPure,Category="Logistics") bool IsEditingLogistics() const {return bLogisticsEdit;}
 FString LogisticsStatus=TEXT("Truck zones are hidden in normal view");
 void ChooseLogisticsTool(FName Tool);
 UFUNCTION(BlueprintCallable,Category="Logistics") void LogisticsAction(FName Action);
 UFUNCTION(BlueprintCallable,Category="Logistics") void SelectLogisticsPoint(FVector Point);
 UFUNCTION(BlueprintPure,Category="Logistics") FString GetLogisticsSummary() const;
 bool bNetworkAdvanced=false;
 uint8 LogisticsPanel=1; // Shared network / local trip / traffic rules.
 FFoundrylineRouteNetwork RouteNetwork;
 FFoundrylineNetworkCheck NetworkCheck;
 FGuid NetworkSelected,NetworkStart,NetworkFinish;
 bool bNetworkTwoWay=false,bNetworkReverse=false,bNetworkSweep=true,bNetworkPlaying=false;
 float NetworkClock=0;
 FString NetworkStatus=TEXT("Draw a new route. Click paving to add points. Right-click to finish, then Test Drive.");
 FString TrafficStatus=TEXT("No live fleet yet. Test the right-of-way rules below.");
 bool NetworkAction(FName Action);
 void NetworkClick(FVector2D Point);
 UFUNCTION(BlueprintPure,Category="Logistics") FString GetNetworkSummary() const;
 FFoundrylineLogisticsTrip LogisticsTrip;
 FFoundrylineLogisticsTripResult TripResult;
 TArray<FFoundrylineLogisticsGate> TruckGates,CarGates;int64 LogisticsGateRevision=-1;
 bool bShowTruckLayer=true,bShowParkingLayer=true,bTripPlaying=false;
 uint8 LogisticsRouteView=0; // 0 both paths, 1 arrival, 2 departure; player-local presentation only.
 float TripPreviewClock=0;TArray<float> TripPoseTimes;TArray<FFoundrylineVehiclePose> TripPreviewPoses;
 bool LogisticsPreviewPose(FFoundrylineVehiclePose& Pose,FString& Phase) const;
 bool IsRouteCheckPending() const {return bRoutePending;}
 bool LogisticsBrush(FIntVector& A,FIntVector& B) const {A=DragStart;B=DragEnd;return bLogisticsEdit && bDraggingBuild;}
 void InvalidateLogisticsTest();

 class UFoundrylinePersonnelDoorComponent* FocusedDoor() const;
 UFUNCTION(BlueprintCallable,Category="Door") void UseDoor();
	void UndoBuild();
 void RedoBuild();
 bool SubmitConstruction(const TArray<FFoundrylinePlacementRequest>& Requests);
 UFUNCTION(BlueprintPure,Category="Construction") bool IsConstructionPending() const {return bConstructionPending;}
 UFUNCTION(BlueprintPure,Category="Construction") int32 GetLastConstructionError() const {return LastConstructionError;}
 UFUNCTION(BlueprintPure,Category="Construction") int32 GetConstructionReplyCount() const {return ConstructionReplyCount;}

	bool IsOverBuildHUD() const;
 FVector2D BuildHUDCanvasSize=FVector2D::ZeroVector;
 bool IsConstructionDragging(FIntVector& Start,FIntVector& End) const {Start=DragStart;End=DragEnd;return bDraggingBuild;}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;

private:
 bool bLogisticsOverlay=false,bLogisticsEdit=false,bRoutePending=false;
 uint32 RouteGeneration=0;
 void LogisticsClick(FIntVector Cursor);
 void DrawLogistics();
 UFUNCTION(Server,Reliable) void ServerConstruct(const TArray<FFoundrylinePlacementRequest>& Requests,uint32 Sequence);
 UFUNCTION(Client,Reliable) void ClientConstructionResult(uint32 Sequence,uint8 Error);
 uint32 ConstructionSequence=0,LastServerConstructionSequence=0;
 double ConstructionRateWindow=0;
 int32 ConstructionRateCount=0;
 bool bConstructionPending=false;
 int32 LastConstructionError=0,ConstructionReplyCount=0;
 UFUNCTION(Server,Reliable) void ServerToggleWalk();
 UFUNCTION(Server,Reliable) void ServerUseDoor(FGuid ObjectId,FGuid OpeningId);
 void ApplyViewInputMode();
 void ApplyRoofVisibility();
 bool bTopDownRoofsVisible=false;
 bool bNeedsOfficeSpawn=true,bHasOfficeReturn=false;
 void TryStartAtOffice();
 FTimerHandle OfficeStartupTimer,PropertyViewTimer;
 void ValidatePropertyView();
 FTransform OfficeReturn;
 FGuid PropertyViewPlant;
 UPROPERTY() TObjectPtr<APawn> BuildCamera;
 UPROPERTY() TObjectPtr<class AFoundrylineWalkCharacter> Walker;
 virtual void OnPossess(APawn* Pawn) override;
 virtual void OnRep_Pawn() override;
	void ToggleGrid() { bShowPlanningGrid = !bShowPlanningGrid; }
	void StructuralSnap() { ActiveSnap = EFoundrylineSnap::Structural100cm; }
	void FineSnap() { ActiveSnap = EFoundrylineSnap::Fine25cm; }
	void TenCmSnap() { ActiveSnap = EFoundrylineSnap::Precision10cm; }
	void FiveCmSnap() { ActiveSnap = EFoundrylineSnap::Precision5cm; }
	void SelectCell();
	void ReleaseBuild();
	void CancelBuild();
	bool bDraggingBuild = false;
	FIntVector DragStart = FIntVector::ZeroValue;
	FIntVector DragEnd = FIntVector::ZeroValue;
};
