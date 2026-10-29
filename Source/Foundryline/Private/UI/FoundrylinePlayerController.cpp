#include "UI/FoundrylinePlayerController.h"
#include "Components/InputComponent.h"
#include "World/FoundrylineViewMath.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Construction/FoundrylineSizing.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

AFoundrylinePlayerController::AFoundrylinePlayerController()
{
	LogisticsTrip.SmallTruck=true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
}

void AFoundrylinePlayerController::BeginPlay()
{
	Super::BeginPlay();
 if(HasAuthority())GetWorldTimerManager().SetTimer(OfficeStartupTimer,this,&AFoundrylinePlayerController::TryStartAtOffice,.25f,true);
	if (!IsLocalController()) return;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	if(auto* Viewport=GetWorld()->GetGameViewport())
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}
}

void AFoundrylinePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
 InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AFoundrylinePlayerController::RotateSurfaceDraft);
 InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AFoundrylinePlayerController::ToggleWalk);
 InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AFoundrylinePlayerController::UseDoor);
	InputComponent->BindAction(TEXT("ToggleGrid"), IE_Pressed, this, &AFoundrylinePlayerController::ToggleGrid);
	InputComponent->BindAction(TEXT("SnapStructural"), IE_Pressed, this, &AFoundrylinePlayerController::StructuralSnap);
	InputComponent->BindAction(TEXT("SnapFine"), IE_Pressed, this, &AFoundrylinePlayerController::FineSnap);
	InputComponent->BindAction(TEXT("Snap10cm"), IE_Pressed, this, &AFoundrylinePlayerController::TenCmSnap);
	InputComponent->BindAction(TEXT("Snap5cm"), IE_Pressed, this, &AFoundrylinePlayerController::FiveCmSnap);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFoundrylinePlayerController::CancelBuild);
	InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AFoundrylinePlayerController::UndoBuild);
 InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AFoundrylinePlayerController::RedoBuild);
}

bool AFoundrylinePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
 if(HandleOperationsInput(Params))return true;
 if(HandleFactoryInput(Params))return true;
 if(HandleMaterialInput(Params)) return true;
 if(HandleFreightInput(Params)) return true;
	// Handle pointer events while SceneViewport still holds that event's coordinates.
	// Deferred action delegates can otherwise see the release position for both ends.
	const bool OverHUD=IsOverBuildHUD();
	if(!IsWalking() && IsLocalController() && OverHUD &&
		(Params.Key==EKeys::MouseScrollUp || Params.Key==EKeys::MouseScrollDown || Params.Key==EKeys::MouseWheelAxis)) return true;
	const bool Handled=Super::InputKey(Params);
	if(!IsWalking() && IsLocalController() && Params.Key==EKeys::LeftMouseButton)
	{
		if(Params.Event==IE_Pressed && !OverHUD) SelectCell();
		else if(Params.Event==IE_Released) ReleaseBuild();
		return true;
	}
	return Handled;
}

void AFoundrylinePlayerController::ApplyRoofVisibility()
{
 if(GetNetMode()!=NM_Standalone && !IsLocalController()) return; // Each network player owns their roof cutaway and lighting view.
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) It->SetRoofsVisible(AreRoofsShown());
}
void AFoundrylinePlayerController::ToggleRoofs()
{
 if(IsWalking()) return;
 bTopDownRoofsVisible=!bTopDownRoofsVisible;ApplyRoofVisibility();
}
bool AFoundrylinePlayerController::IsWalking() const {return Cast<AFoundrylineWalkCharacter>(GetPawn())!=nullptr;}
void AFoundrylinePlayerController::ApplyViewInputMode()
{
 if(!IsLocalController()) return;
 bShowMouseCursor=!IsWalking();
 if(IsWalking()) {FInputModeGameOnly Mode;SetInputMode(Mode);}else {FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
}
void AFoundrylinePlayerController::OnPossess(APawn* NewPawn) {Super::OnPossess(NewPawn);ApplyViewInputMode();ApplyRoofVisibility();}
void AFoundrylinePlayerController::OnRep_Pawn() {Super::OnRep_Pawn();ApplyViewInputMode();ApplyRoofVisibility();}
void AFoundrylinePlayerController::ToggleWalk() {if(LocalFreightRig || LocalMaterialCell)return;if(IsWalking()){FactoryStatus=TEXT("Open the office computer to enter Property View");return;}ChooseTool(NAME_None);if(HasAuthority()) ServerToggleWalk_Implementation();else ServerToggleWalk();}
void AFoundrylinePlayerController::ServerToggleWalk_Implementation()
{
 if(IsWalking()) {if(!bPropertyViewAccess)return;if(IsValid(BuildCamera)) {Possess(BuildCamera);if(IsValid(Walker)) Walker->Destroy();Walker=nullptr;}return;}
 if(!GetPawn()) return;BuildCamera=GetPawn();
 FVector Spawn=BuildCamera->GetActorLocation()+FVector(0,-300,150),OfficeTarget=FVector::ZeroVector;bool FaceOffice=false;FRotator Facing(0,90,0);
 auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 // Prefer the office's outdoor entrance; keep a clear 2 m approach outside its shell.
 for(const auto& O:Placement->GetObjects()) if(O.bOfficeAnnex)
 {
  const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D) continue;
  const auto* Door=O.Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==1;});
  if(!Door) Door=O.Openings.FindByPredicate([](const auto& V){return V.Kind==EFoundrylineOpeningKind::OfficeDoor && V.Edge==2;});if(!Door) continue;
  const FVector Entry=Door->Edge==1?FVector(D->Footprint.Max.X+200,Door->OffsetCm+50,120):FVector(Door->OffsetCm+50,D->Footprint.Max.Y+200,120);
  Spawn=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector(Entry);Facing=FRotator(0,O.QuarterTurns*90+(Door->Edge==1?180:270),0);break;
 }
 if(bHasOfficeReturn){Spawn=OfficeReturn.GetLocation();Facing=OfficeReturn.Rotator();}else{auto* Plant=FactoryPlant();for(TActorIterator<AFoundrylineOperationsTerminal> It(GetWorld());It;++It)if(Plant&&It->Terminal==1&&It->PlantId==Plant->BusinessPlantId){Spawn=It->GetActorLocation()+FVector(0,170,95);OfficeTarget=It->InteractionPoint();FaceOffice=true;Facing=(It->InteractionPoint()-(Spawn+FVector(0,0,80))).Rotation();break;}}
 FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
 if(!bHasOfficeReturn){const FVector Offsets[]={FVector(0,0,0),FVector(-100,0,0),FVector(100,0,0),FVector(180,0,0),FVector(-180,0,0),FVector(0,-70,0),FVector(100,-70,0),FVector(-100,-70,0)};const int Start=PlayerState?FMath::Abs(PlayerState->GetPlayerId())%8:0;for(int I=0;I<8&&!Walker;++I)Walker=GetWorld()->SpawnActor<AFoundrylineWalkCharacter>(Spawn+Offsets[(Start+I)%8],FRotator(0,Facing.Yaw,0),Params);}else Walker=GetWorld()->SpawnActor<AFoundrylineWalkCharacter>(Spawn,FRotator(0,Facing.Yaw,0),Params);
 if(Walker) {if(FaceOffice&&!bHasOfficeReturn)Facing=(OfficeTarget-(Walker->GetActorLocation()+FVector(0,0,80))).Rotation();Possess(Walker);SetControlRotation(Facing);bNeedsOfficeSpawn=false;bPropertyViewAccess=false;bHasOfficeReturn=false;GetWorldTimerManager().ClearTimer(PropertyViewTimer);ClientPropertyViewMode(false);}
}
UFoundrylinePersonnelDoorComponent* AFoundrylinePlayerController::FocusedDoor() const
{
 if(!IsWalking()) return nullptr;FVector Eye;FRotator View;GetPlayerViewPoint(Eye,View);FHitResult Hit;FCollisionQueryParams Query;Query.AddIgnoredActor(GetPawn());
 GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+View.Vector()*300,ECC_Visibility,Query);return Cast<UFoundrylinePersonnelDoorComponent>(Hit.GetComponent());
}
void AFoundrylinePlayerController::UseDoor() {if(auto* Door=FocusedDoor()) ServerUseDoor(Door->BuildingObjectId,Door->OpeningId);}
void AFoundrylinePlayerController::ServerUseDoor_Implementation(FGuid ObjectId,FGuid OpeningId)
{
 if(!IsWalking()) return;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) if(It->UseDoor(GetPawn(),ObjectId,OpeningId)) break;
}

void AFoundrylinePlayerController::ChooseTool(FName Tool)
{
	BuildTool = Tool;
 // Every newly selected pavement tool draws an area, even after legacy presets.
 if(FFoundrylineSizing::Surface(Tool)) bExactSurface=false;
 ApplySurfaceDraft();
	bDraggingBuild = false;
	ActiveSnap = EFoundrylineSnap::Structural100cm;
	bShowPlanningGrid = !Tool.IsNone() && Tool!=TEXT("Select");
	for (TActorIterator<AFoundrylineBuildSite> It(GetWorld()); It; ++It)
	{
		It->HidePreview();It->ClearConstructionFeedback(); if(!Tool.IsNone() && !AFoundrylineBuildSite::IsEditTool(Tool)) It->SelectedObject.Invalidate();
		if(AFoundrylineBuildSite::IsEditTool(Tool)) {It->Status=TEXT("Select: click a section. Move/resize: drag it. Remove: click; Z restores. Door: drag an opening.");break;}
		if(Tool==TEXT("Columns")) {It->Status=TEXT("Click inside a Tier 2 shop to cycle column bays: 4 / 6 / 8 m. Z undoes."); break;}
		It->Status = Tool.IsNone() ? TEXT("Choose a construction tool") :
			(Tool == TEXT("Personnel") || Tool == TEXT("Garage") || Tool == TEXT("TruckDoor") || Tool==TEXT("LoadingDock")) ? TEXT("Point at an existing wall, then click to install") : TEXT("Hold left mouse and drag. Release to build. Right-click cancels.");
		if(bExactSurface && FFoundrylineSizing::Surface(Tool)) It->Status=TEXT("Click once to place. R rotates. Right-click cancels.");
		break;
	}
}

bool AFoundrylinePlayerController::IsOverBuildHUD() const
{
 if(bFactoryMenu)return FactoryOverHUD();
	float X=0, Y=0;
	int32 Width=0, Height=0; GetViewportSize(Width,Height);
	if(!GetMousePosition(X,Y)) return false;
 if(Width<=0 || Height<=0) return false;
 if(BuildHUDCanvasSize.X>0 && BuildHUDCanvasSize.Y>0) {X*=BuildHUDCanvasSize.X/Width;Y*=BuildHUDCanvasSize.Y/Height;Width=FMath::RoundToInt32(BuildHUDCanvasSize.X);Height=FMath::RoundToInt32(BuildHUDCanvasSize.Y);}
 const float S=FMath::Min(1.5f,Height/1320.f);
 if(FactoryPlant()&&X>Width-210.f*Height/1080.f&&Y>=186.f*Height/1080.f&&Y<=230.f*Height/1080.f)return true;
 if(X>=700*S && X<=950*S && Y>=Height-60*S && Y<=Height-18*S) return true;
 if(X>=440*S && X<=670*S && Y>=Height-60*S && Y<=Height-18*S) return true;
 const float RightBottom=bLogisticsOverlay?(LogisticsPanel==0?572:264):FFoundrylineSizing::Surface(BuildTool)?(BuildTool==TEXT("TruckAccess")?556:496):264;
 const float RightWidth=!bLogisticsOverlay && FFoundrylineSizing::Surface(BuildTool)?390:270;
 if(bLogisticsOverlay && LogisticsPanel==0 && bNetworkAdvanced && X>Width-656*S && Y>=600*S && Y<=1154*S) return true;
 return X<400*S || (X>Width-RightWidth*S && Y<RightBottom*S && Y>264*S) || (X>Width-270*S && Y<(bLogisticsOverlay?176:124)*S);
}

void AFoundrylinePlayerController::RedoBuild()
{
 if(GetNetMode()!=NM_Standalone) return;
 bDraggingBuild=false;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {It->Redo();break;}
}

void AFoundrylinePlayerController::UndoBuild()
{
 if(GetNetMode()!=NM_Standalone) return;
	bDraggingBuild=false;
	for (TActorIterator<AFoundrylineBuildSite> It(GetWorld()); It; ++It) { It->Undo(); break; }
}

bool AFoundrylinePlayerController::GetCursorAnchor(FIntVector& OutAnchorCm) const
{
	if (!IsLocalController()) return false;
	FVector Origin, Direction;
	if (!DeprojectMousePositionToWorld(Origin, Direction)) return false;
	if (BuildTool == TEXT("Personnel") || BuildTool == TEXT("Garage") || BuildTool == TEXT("TruckDoor") || BuildTool==TEXT("LoadingDock") || BuildTool==TEXT("MoveDoor") || BuildTool==TEXT("RoomDoor") || BuildTool==TEXT("RoomWindow"))
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 500000.f, ECC_Visibility))
			return FFoundrylineGrid::SnapWorld(FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, 0), EFoundrylineSnap::Structural100cm, OutAnchorCm);
	}
	// Flat-plane intersection deliberately ignores props/roofs so the grid remains stable.
	return FFoundrylineViewMath::GroundAnchor(Origin, Direction, GetPlacementSnap(), OutAnchorCm);
}

void AFoundrylinePlayerController::SelectCell()
{
 if(bConstructionPending) return;
	if (IsOverBuildHUD()) return;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) if(It->IsPropertyBusy()) return;
	FIntVector Anchor;
	if (!GetCursorAnchor(Anchor)) return;
 if(bLogisticsEdit) {LogisticsClick(Anchor);return;}
 if(bExactSurface && FFoundrylineSizing::Surface(BuildTool)) {ApplySurfaceDraft();for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {It->BuildRectangle(BuildTool,Anchor,ExactSurfaceEnd(Anchor));break;}return;}
	if(BuildTool==TEXT("Columns"))
	{
		for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) { It->AdjustColumns(Anchor); break; }
		return;
	}
	if(BuildTool.IsNone()) {for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->BeginEdit(TEXT("Select"),Anchor);break;}return;}
 if (!BuildTool.IsNone())
	{
		for (TActorIterator<AFoundrylineBuildSite> It(GetWorld()); It; ++It)
		{
			if(AFoundrylineBuildSite::IsEditTool(BuildTool)) {if(It->BeginEdit(BuildTool,Anchor)) {DragStart=DragEnd=Anchor;bDraggingBuild=true;}}
			else if(BuildTool==TEXT("Landscape") || BuildTool==TEXT("Crosswalk")) It->ApplyGroundTool(BuildTool,Anchor);
			else if(BuildTool==TEXT("RoomDoor") || BuildTool==TEXT("RoomWindow")) It->OfficeOpening(BuildTool,Anchor);
			else if (BuildTool == TEXT("Personnel") || BuildTool == TEXT("Garage") || BuildTool == TEXT("TruckDoor") || BuildTool==TEXT("LoadingDock")) It->PlaceDoor(BuildTool, Anchor);
			else { DragStart = DragEnd = Anchor; bDraggingBuild = true; }
			return;
		}
	}
	OnCellSelected.Broadcast(Anchor, GetPlacementSnap());
}

void AFoundrylinePlayerController::ReleaseBuild()
{
	if (!bDraggingBuild) return;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) if(It->IsPropertyBusy()) {bDraggingBuild=false;return;}
	bDraggingBuild = false;
	FIntVector End=DragEnd, Cursor;
	// Some PIE capture modes restore the cursor to its press position on release.
	// Retain the last rendered draft endpoint instead of collapsing that draft.
	if(GetCursorAnchor(Cursor) && Cursor!=DragStart) End=Cursor;
 if(bLogisticsEdit && (BuildTool==TEXT("RoutePaint") || BuildTool==TEXT("RouteErase"))) {
  InvalidateLogisticsTest();
  LogisticsStatus=TEXT("Access updated. Select a loading destination, then an exit, and test the trip");
  if(!IsOverBuildHUD()) for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {It->EditTruckRouting(DragStart,End,BuildTool==TEXT("RouteErase"));break;}
  return;
 }
	for (TActorIterator<AFoundrylineBuildSite> It(GetWorld()); It; ++It)
	{
		if (!IsOverBuildHUD())
		{
			if(AFoundrylineBuildSite::IsEditTool(BuildTool)) It->CommitEdit(BuildTool,DragStart,End);
			else if(BuildTool==TEXT("OfficeArea") || BuildTool==TEXT("OfficeRoom")) It->OfficeRectangle(BuildTool,DragStart,End);
			else It->BuildRectangle(BuildTool, DragStart, End);
		}
		if(BuildTool!=TEXT("Select")) It->HidePreview(); break;
	}
}

void AFoundrylinePlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
 if(IsLocalController()&&OperationsScreen&&!FactoryPlant())OpenOperations(0);
 if(HasAuthority()&&bPropertyViewAccess){auto* Plant=FactoryPlant();if(!Plant||Plant->BusinessPlantId!=PropertyViewPlant)ServerToggleWalk_Implementation();}

 TickFactory(DeltaSeconds);if(bFactoryMenu)return;
 if(LocalMaterialCell) {TickMaterialHandling(DeltaSeconds);return;}
 if(LocalFreightRig) {TickFreightDrive(DeltaSeconds);return;}
 if(IsWalking()) return;
 if(IsLocalController() && bLogisticsOverlay) {if(bTripPlaying) TripPreviewClock+=DeltaSeconds;if(bNetworkPlaying) NetworkClock+=DeltaSeconds;DrawLogistics();}
 if(IsLocalController() && bLogisticsEdit) return;
	if (IsLocalController())
	{
		FIntVector BuildAnchor;
		for (TActorIterator<AFoundrylineBuildSite> It(GetWorld()); It; ++It)
		{
			FIntVector SelectionCursor=FIntVector::ZeroValue;const bool HasCursor=!IsOverBuildHUD() && GetCursorAnchor(SelectionCursor);
            It->SelectionOutlines.Reset();
            if(!IsWalking()) It->RecordSelection(FIntVector::ZeroValue,false);
            if(!IsWalking() && HasCursor && (BuildTool.IsNone() || AFoundrylineBuildSite::IsEditTool(BuildTool)) && !bDraggingBuild) It->RecordSelection(SelectionCursor,true);
            if (It->IsPropertyBusy() || BuildTool.IsNone() || IsOverBuildHUD() || !GetCursorAnchor(BuildAnchor)) It->HidePreview();
			else if (BuildTool == TEXT("Personnel") || BuildTool == TEXT("Garage") || BuildTool == TEXT("TruckDoor") || BuildTool==TEXT("LoadingDock")) It->PreviewDoor(BuildTool, BuildAnchor);
			else if(BuildTool==TEXT("Columns")) It->PreviewColumns(BuildAnchor);
			else if(BuildTool==TEXT("Landscape") || BuildTool==TEXT("Crosswalk")) It->ApplyGroundTool(BuildTool,BuildAnchor,true);
			else if(BuildTool==TEXT("RoomDoor") || BuildTool==TEXT("RoomWindow")) It->OfficeOpening(BuildTool,BuildAnchor,true);
			else if (bDraggingBuild) { DragEnd=BuildAnchor; if(AFoundrylineBuildSite::IsEditTool(BuildTool)) It->PreviewEdit(BuildTool,DragStart,DragEnd);else if(BuildTool==TEXT("OfficeArea") || BuildTool==TEXT("OfficeRoom")) It->OfficeRectangle(BuildTool,DragStart,DragEnd,true);else It->PreviewRectangle(BuildTool, DragStart, DragEnd); }
			else if(bExactSurface && FFoundrylineSizing::Surface(BuildTool)) It->PreviewRectangle(BuildTool,BuildAnchor,ExactSurfaceEnd(BuildAnchor));
            else if(BuildTool!=TEXT("Select")) It->HidePreview();
			break;
		}
	}
}

void AFoundrylinePlayerController::TryStartAtOffice(){if(!HasAuthority())return;if(!bNeedsOfficeSpawn||IsWalking()){GetWorldTimerManager().ClearTimer(OfficeStartupTimer);return;}if(!GetPawn())return;auto* Plant=FactoryPlant();if(GetWorld()->GetTimeSeconds()>10&&(!Plant||!Plant->bCutPartsStarter)){bNeedsOfficeSpawn=false;GetWorldTimerManager().ClearTimer(OfficeStartupTimer);return;}for(TActorIterator<AFoundrylineOperationsTerminal> It(GetWorld());It;++It)if(Plant&&It->Terminal==1&&It->PlantId==Plant->BusinessPlantId){ServerToggleWalk_Implementation();if(IsWalking())GetWorldTimerManager().ClearTimer(OfficeStartupTimer);break;}}

void AFoundrylinePlayerController::ValidatePropertyView(){if(!HasAuthority()||!bPropertyViewAccess){GetWorldTimerManager().ClearTimer(PropertyViewTimer);return;}auto* Plant=FactoryPlant();if(!Plant||Plant->BusinessPlantId!=PropertyViewPlant)ServerToggleWalk_Implementation();}
