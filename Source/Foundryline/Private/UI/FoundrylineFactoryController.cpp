#include "Logistics/FoundrylineRackAccess.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "FoundrylineBuildCatalog.h"
#include "FoundrylineOperationFeedback.h"
#include "UI/FoundrylineBuildHUD.h"
#include "Inventory/FoundrylineAssemblySupply.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Audio/FoundrylineUIAudio.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylineBuildSite.h"
#include "InputKeyEventArgs.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "Grid/FoundrylineFloorGuide.h"
#include "Core/FoundrylineUnits.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Components/TextRenderComponent.h"
AFoundrylineFactoryPlant* AFoundrylinePlayerController::FactoryPlant()const{for(TActorIterator<AFoundrylineFactoryPlant> It(GetWorld());It;++It){auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());if(!R||R->CanOperate(this,It->BusinessCompanyId,It->BusinessPlantId))return *It;}return nullptr;}
void AFoundrylinePlayerController::SendFactory(FFoundrylineFactoryCommand C){FactoryStatus.Reset();FoundrylineOperationFeedback::BeginFactory(this,C);ServerFactoryCommand(C,++FactorySequence);}
void AFoundrylinePlayerController::ServerFactoryCommand_Implementation(const FFoundrylineFactoryCommand& C,uint32 Sequence){
 if(Sequence<=LastFactorySequence)return;LastFactorySequence=Sequence;double Now=GetWorld()->GetTimeSeconds();if(Now-FactoryRateWindow>=1){FactoryRateWindow=Now;FactoryRateCount=0;}if(++FactoryRateCount>20){ClientFactoryReply(TEXT("Please wait before sending more factory actions"));return;}
 FString E;auto* Plant=FactoryPlant();if(!Plant||!Plant->Execute(this,C,E))ClientFactoryReply(E.IsEmpty()?TEXT("Plant unavailable"):E);else if(FoundrylineOperationFeedback::FactoryHasReceipt(C))ClientFactoryReply(TEXT("FL_OK|")+FoundrylineOperationFeedback::FactoryReceipt(C,Plant->Snapshot));
}
void AFoundrylinePlayerController::ClientFactoryReply_Implementation(const FString& E){
 const bool Confirmed=E.StartsWith(TEXT("FL_OK|"));const FString Error=Confirmed?FString():E;FoundrylineOperationFeedback::FinishFactory(this);
 if(bPlannerPending){bPlannerPending=false;PlannerStatus=Error;if(Error.IsEmpty())bLastPlannerEdit=true;}
 if(Confirmed){UFoundrylineUIAudio::Play(this,TEXT("Confirm"));ConfirmOperations(E.RightChop(6));if(E.Contains(TEXT("Shop saved:")))for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)It->Status=E.RightChop(6);}
 else if(!Error.IsEmpty()){UFoundrylineUIAudio::Play(this,TEXT("Error"));OperationsStatus=Error;OperationsStatusAt=GetWorld()->GetRealTimeSeconds();OperationsWatch=0;}
 FactoryStatus=Error;if(!Error.IsEmpty())OperationsHireRequest.Invalidate();
}
bool AFoundrylinePlayerController::FactoryOverHUD()const{if(bPropertyViewAccess&&!IsWalking()&&!FactoryMachine)if(auto* H=Cast<AFoundrylineBuildHUD>(GetHUD()))return FoundrylineBuildCatalogOverUI(H,this);float X,Y;int W,H;GetViewportSize(W,H);if(!GetMousePosition(X,Y)||W<=0||H<=0)return false;const float S=H/1080.f;if(FactoryMachine>0||X<420*S||X>W-300*S&&Y<205*S)return true;if((bFactoryOverlay||FactorySelectedPart>=0)&&X>W-330*S&&Y>150*S&&Y<(FactorySelectedPart>=0?640:400)*S)return true;return false;}
int32 AFoundrylinePlayerController::FocusedComputer()const{if(!IsWalking()||!PlayerCameraManager||!FactoryPlant())return 0;const FVector Eye=PlayerCameraManager->GetCameraLocation(),Forward=PlayerCameraManager->GetCameraRotation().Vector();int Best=0;float Distance=260;for(const auto& Machine:FactoryPlant()->Snapshot.Machines){if(Machine.bStored)continue;const int I=Machine.Id;const FVector Delta=FactoryPlant()->ScreenWorldPosition(I)-Eye;const float D=Delta.Size();if(D<Distance&&FVector::DotProduct(Delta.GetSafeNormal(),Forward)>.35){Distance=D;Best=I;}}return Best;}
void AFoundrylinePlayerController::OpenFactoryComputer(int Id){if(Id>0)if(auto* Plant=FactoryPlant();Plant&&Plant->Snapshot.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Id&&M.bStored;})){FactoryStatus=TEXT("Place this owned machine from Planning before using its terminal");return;}if(Id>0&&Id!=4){auto* Plant=FactoryPlant();if(!Plant||!IsWalking()||!PlayerCameraManager||FVector::Dist(PlayerCameraManager->GetCameraLocation(),Plant->ScreenWorldPosition(Id))>300){FactoryStatus=TEXT("Walk to this machine terminal and press E");return;}}if(auto* P=FactoryPlant()){if(P->Snapshot.bCutPartsStarter){bFactoryAuto=false;if(Id>0&&!P->Snapshot.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Id;}))return;}}if(FactoryMachine!=Id)UFoundrylineUIAudio::Play(this,Id>0?TEXT("Open"):TEXT("Close"));FactoryMachine=Id;FactoryTool=NAME_None;bFactoryDragging=false;
 if(Id>0){bFactoryMenu=true;FactoryProgramIndex=0;FactoryStoragePage=0;if(!bFactoryInputLocked){SetIgnoreLookInput(true);SetIgnoreMoveInput(true);bFactoryInputLocked=true;}FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);bShowMouseCursor=true;}
 else {if(bFactoryInputLocked){SetIgnoreLookInput(false);SetIgnoreMoveInput(false);bFactoryInputLocked=false;}if(IsWalking())bFactoryMenu=false;ApplyViewInputMode();}}
bool AFoundrylinePlayerController::HandleFactoryInput(const FInputKeyEventArgs& P){if(!IsLocalController()||!FactoryPlant())return false;
 if(FactoryMachine>0&&!FactoryEditField.IsNone()){
  if(P.Key==EKeys::LeftMouseButton&&P.Event==IE_Pressed)CommitFactoryNumber();
  else if(P.Event==IE_Pressed||P.Event==IE_Repeat){if(P.Key==EKeys::Enter){CommitFactoryNumber();return true;}if(P.Key==EKeys::Escape){FactoryEditField=NAME_None;return true;}if(P.Key==EKeys::BackSpace){if(!FactoryEditText.IsEmpty())FactoryEditText.LeftChopInline(1);return true;}
   const FKey Digits[]={EKeys::Zero,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};for(int I=0;I<10;++I)if(P.Key==Digits[I]){if(FactoryEditText.Len()<8)FactoryEditText+=FString::FromInt(I);return true;}if(P.Key==EKeys::Period||P.Key==EKeys::Decimal){if(!FactoryEditText.Contains(TEXT(".")))FactoryEditText+=TEXT(".");return true;}if(P.Key!=EKeys::LeftMouseButton)return true;
  }
 }
 if(P.Event==IE_Pressed&&P.Key==EKeys::P){if(!IsWalking())FactoryAction(TEXT("Toggle"));return true;}
 if(P.Event==IE_Pressed&&P.Key==EKeys::Escape&&bFactoryMenu){if(FactoryMachine)OpenFactoryComputer(0);else if(!FactoryTool.IsNone()){FactoryTool=NAME_None;bFactoryDragging=false;}else FactoryAction(TEXT("Toggle"));return true;}
 if(P.Event==IE_Pressed&&P.Key==EKeys::E&&!FocusedDoor()){if(FactoryMachine)OpenFactoryComputer(0);else if(int Id=FocusedComputer())OpenFactoryComputer(Id);else return false;return true;}
 if(FactoryMachine>0){if(P.Key==EKeys::LeftMouseButton){Super::InputKey(P);return true;}if(P.Key==EKeys::F||P.Key==EKeys::R||P.Key==EKeys::Z||P.Key==EKeys::Y||P.Key==EKeys::MouseScrollDown||P.Key==EKeys::MouseScrollUp)return true;return false;}
 if(!bFactoryMenu||IsWalking())return false;
 if(!FactoryTool.IsNone()&&P.Event==IE_Pressed){
  if(P.Key==EKeys::G){FactoryAction(TEXT("GridVisible"));return true;}
  if(P.Key==EKeys::Tab){FactoryAction(TEXT("GridStep"));return true;}
  if((P.Key==EKeys::MouseScrollUp||P.Key==EKeys::MouseScrollDown)&&IsInputKeyDown(EKeys::LeftAlt)&&FactoryTool!=TEXT("Forklift")){FactoryAction(P.Key==EKeys::MouseScrollUp?TEXT("WidthPlus"):TEXT("WidthMinus"));return true;}
 }
 if(P.Key==EKeys::RightMouseButton&&P.Event==IE_Pressed){FactoryTool=NAME_None;bFactoryDragging=false;return true;}
 if(FactoryTool==TEXT("Zone")&&P.Event==IE_Pressed&&(P.Key==EKeys::R||P.Key==EKeys::V)){FactoryAction(P.Key==EKeys::R?TEXT("ZoneRotate"):TEXT("ZoneSide"));return true;}
 if((FactoryTool==TEXT("Station")||FactoryTool==TEXT("Equipment"))&&P.Event==IE_Pressed&&(P.Key==EKeys::R||P.Key==EKeys::V)){FactoryAction(P.Key==EKeys::R?TEXT("StationRotate"):TEXT("StationSide"));return true;}
 if(P.Key==EKeys::LeftMouseButton){if(FactoryOverHUD()){Super::InputKey(P);if(P.Event==IE_Released)bFactoryDragging=false;return true;}FVector A;if(!FactoryFloorCursor(A)){if(P.Event==IE_Released)bFactoryDragging=false;return true;}
  if(FactoryTool==TEXT("Zone")||FactoryTool==TEXT("Crossing")||FactoryTool==TEXT("Station")||FactoryTool==TEXT("Equipment")){if(P.Event==IE_Pressed){if(!PreviewFactoryFloor(A)){FactoryStatus=FactoryPlacementHint;return true;}CommitFactoryFloorBrush();}return true;}
  if(P.Event==IE_Pressed){if(FactoryTool.IsNone()){auto* Plant=FactoryPlant();FFoundrylineFactorySimulation View;View.State=Plant->Snapshot;for(const auto& Z:Plant->Snapshot.Zones)for(auto Part:{EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings}){const FVector P=View.BufferPosition(Z,Part);const auto Size=FFoundrylineWorkZoneLayout::BaySize(Z,Part);if(FBox2D(FVector2D(P)-Size/2,FVector2D(P)+Size/2).IsInside(FVector2D(A))){FactoryZone=Z.Id;FactorySelectedPart=int(Part);FactoryPage=0;return true;}}for(const auto& S:Plant->Snapshot.WeldingStations)if(FFoundrylineWeldingStationLayout::Bounds(S).IsInside(FVector2D(A))){FactoryStation=S.Id;FactoryStatus.Reset();return true;}FactoryStation=0;float Best=500;int Pick=0;for(const auto& Machine:Plant->Snapshot.Machines){const int I=Machine.Id;const float D=FVector::Dist2D(A,Plant->MachineWorldPosition(I));if(D<Best){Best=D;Pick=I;}}if(Pick){OpenFactoryComputer(Pick);return true;}for(const auto& Z:Plant->Snapshot.Zones)if(FFoundrylineWorkZoneLayout::Bounds(Z).IsInside(FVector2D(A))){FactoryZone=Z.Id;FactoryPage=0;return true;}return true;}
   if(FactoryTool==TEXT("Erase")){auto* Plant=FactoryPlant();for(int I=Plant->Snapshot.Lanes.Num()-1;I>=0;--I)if(FBox2D(Plant->Snapshot.Lanes[I].Min,Plant->Snapshot.Lanes[I].Max).IsInside(FVector2D(A))){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::EraseLane;C.Id=Plant->Snapshot.Lanes[I].Id;SendFactory(C);break;}return true;}
   FactoryDragStart=FactoryDragEnd=A;bFactoryDragging=true;PreviewFactoryFloor(A);
  }else if(P.Event==IE_Released&&bFactoryDragging){FactoryDragEnd=A;const bool Valid=PreviewFactoryFloor(A);bFactoryDragging=false;if(!Valid){FactoryStatus=FactoryPlacementHint;return true;}CommitFactoryFloorBrush();}return true;
 }return false;
}
void AFoundrylinePlayerController::TickFactory(float Dt){
 if(!IsLocalController())return;auto* Plant=FactoryPlant();if(!Plant)return;Plant->bLocalOverlay=bFactoryOverlay;Plant->SetOfficeDecorationFloor(OfficePlanningLevel,IsWalking());
 for(TActorIterator<AFoundrylineOperationsTerminal> It(GetWorld());It;++It)if(It->Terminal==1){const bool Show=IsWalking()||int(It->GetActorLocation().Z>300)==OfficePlanningLevel;if(auto* Mesh=It->FindComponentByClass<UStaticMeshComponent>())Mesh->SetVisibility(Show,true);}
 FVector Cursor;const bool Active=bFactoryMenu&&!IsWalking()&&!FactoryMachine&&!FactoryTool.IsNone()&&FactoryTool!=TEXT("Erase")&&!FactoryOverHUD()&&FactoryFloorCursor(Cursor);
 if(Active&&!FactoryFloorGuide){FActorSpawnParameters P;P.Owner=this;FactoryFloorGuide=GetWorld()->SpawnActor<AFoundrylineFloorGuide>(FVector::ZeroVector,FRotator::ZeroRotator,P);}
 if(Active){if(bFactoryDragging)FactoryDragEnd=Cursor;PreviewFactoryFloor(Cursor);}
 auto WorldBox=[&](const FBox2D& B){FBox2D World(ForceInit);for(int X:{0,1})for(int Y:{0,1})World+=FVector2D(Plant->GetActorTransform().TransformPosition(FVector(X?B.Max.X:B.Min.X,Y?B.Max.Y:B.Min.Y,20)));return World;};
 if(FactoryFloorGuide)FactoryFloorGuide->Show(WorldBox(FBox2D(Plant->Snapshot.FloorMin,Plant->Snapshot.FloorMax)),WorldBox(FactoryPreviewBounds),FMath::Max(FactoryGridStep(),15.24),bFactoryGrid,bFactoryPresetValid,Active&&!(FactoryTool==TEXT("Equipment")&&FactoryEquipmentChoice==7));
 // Station hologram: bench, fixture, full-size rack, both worker spots, IN/OUT and outside access, all from the shared layout.
 const auto* MovingMachine=bPlannerMoving&&PlannerKind==int(EFoundrylinePlannerItem::Machine)?Plant->Snapshot.Machines.FindByPredicate([&](const auto& M){return M.Id==PlannerId;}):nullptr;
 if(MovingMachine){FactoryEquipmentChoice=MovingMachine->Kind()+3;FactoryPreviewA=PlannerAt;FactoryStationQuarter=PlannerQuarter;bFactoryPresetValid=bPlannerValid;}
 const bool ShowEquipment=MovingMachine||Active&&FactoryTool==TEXT("Equipment");
 if(ShowEquipment&&!FactoryEquipmentPreview){FActorSpawnParameters Params;Params.Owner=this;FactoryEquipmentPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,Params);FactoryEquipmentAccessPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,Params);for(auto* Actor:{FactoryEquipmentPreview.Get(),FactoryEquipmentAccessPreview.Get()}){auto* Mesh=Actor->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);}}
 if(FactoryEquipmentPreview){FactoryEquipmentPreview->SetActorHiddenInGame(!ShowEquipment);FactoryEquipmentAccessPreview->SetActorHiddenInGame(!ShowEquipment||FactoryEquipmentChoice==3||FactoryEquipmentChoice==7);if(ShowEquipment){auto* Mesh=FactoryEquipmentPreview->GetStaticMeshComponent();const auto Path=FFoundrylinePlanningEquipment::Mesh(FactoryEquipmentChoice,FactoryRackLevels);auto* Asset=LoadObject<UStaticMesh>(nullptr,*Path);auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));if(Mesh->GetStaticMesh()!=Asset){TArray<USceneComponent*> OldParts;Mesh->GetChildrenComponents(true,OldParts);for(int Index=OldParts.Num()-1;Index>=0;--Index)OldParts[Index]->DestroyComponent();Mesh->SetStaticMesh(Asset);for(int Slot=0;Slot<Mesh->GetNumMaterials();++Slot)Mesh->SetMaterial(Slot,UMaterialInstanceDynamic::Create(Base,this));
 if(FactoryEquipmentChoice==7){auto* Chair=NewObject<UStaticMeshComponent>(FactoryEquipmentPreview);Chair->SetupAttachment(Mesh);Chair->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Office/SM_Chairs_01.SM_Chairs_01")));Chair->SetCollisionEnabled(ECollisionEnabled::NoCollision);Chair->SetRelativeLocationAndRotation(FVector(0,-85,0),FRotator::ZeroRotator);for(int Slot=0;Slot<Chair->GetNumMaterials();++Slot)Chair->SetMaterial(Slot,UMaterialInstanceDynamic::Create(Base,this));Chair->RegisterComponent();}
 if(FactoryEquipmentChoice==3){const TCHAR* Names[]={TEXT("MastOuter"),TEXT("MastInner"),TEXT("Carriage"),TEXT("Wheel_FL"),TEXT("Wheel_RL"),TEXT("Wheel_FR"),TEXT("Wheel_RR")};const FVector Locations[]={{28,0,16},{28,0,28},{28,0,28},{0,59.59,41.84},{-196.67,56.81,35.66},{0,-59.59,41.79},{-197.33,-58.44,34.75}};
 for(int Index=0;Index<7;++Index){auto* Part=NewObject<UStaticMeshComponent>(FactoryEquipmentPreview);Part->SetupAttachment(Mesh);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(FString(TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_"))+Names[Index])));Part->SetRelativeLocation(Locations[Index]);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(false);Part->RegisterComponent();FactoryEquipmentPreview->AddInstanceComponent(Part);for(int Slot=0;Slot<Part->GetNumMaterials();++Slot)Part->SetMaterial(Slot,UMaterialInstanceDynamic::Create(Base,this));}}
 if(FactoryEquipmentChoice>=4&&FactoryEquipmentChoice<=6){const int Kind=FactoryEquipmentChoice-3;const FString Folder=Kind==1?TEXT("PlasmaTable"):Kind==2?TEXT("PressBrake"):TEXT("SheetRoller");TArray<FString> Names=Kind==1?TArray<FString>{TEXT("SlatBed"),TEXT("Gantry"),TEXT("TorchCarriage"),TEXT("Torch"),TEXT("OperatorConsole"),TEXT("PowerUnit"),TEXT("Extraction")}:Kind==2?TArray<FString>{TEXT("Ram"),TEXT("UpperPunch"),TEXT("PistonRods"),TEXT("LowerDie"),TEXT("BackGauge"),TEXT("OperatorConsole"),TEXT("FootPedal")}:TArray<FString>{};
  auto Ghost=[&](FString PartPath,FVector At,FRotator Turn=FRotator::ZeroRotator){auto* Part=NewObject<UStaticMeshComponent>(FactoryEquipmentPreview);Part->SetupAttachment(Mesh);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*PartPath));Part->SetRelativeLocationAndRotation(At,Turn);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(false);Part->RegisterComponent();FactoryEquipmentPreview->AddInstanceComponent(Part);for(int Slot=0;Slot<Part->GetNumMaterials();++Slot)Part->SetMaterial(Slot,UMaterialInstanceDynamic::Create(Base,this));};
  for(const auto& Name:Names)Ghost(TEXT("/Game/Foundryline/Production/Equipment/")+Folder+TEXT("/SM_")+Folder+TEXT("_")+Name,FVector::ZeroVector);
  const double Side=Kind==1?600:500;Ghost(TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels"),{Side,250,0},FRotator(0,90,0));if(Kind==1)Ghost(TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels"),{-600,0,0},FRotator(0,90,0));else Ghost(TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels"),{Side,-250,0},FRotator(0,90,0));
  if(Kind==3){Ghost(TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_OperatorConsole"),FVector::ZeroVector);Ghost(TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_UpperCarriers"),{0,0,127.4});Ghost(TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_Upper"),{0,0,127.4});Ghost(TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_LowerFront"),{-22,0,92});Ghost(TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_LowerRear"),{22,0,92});}
 }
 }const FRotator EquipmentRotation(0,FactoryStationQuarter*90+(FactoryEquipmentChoice==7?180:0),0);FactoryEquipmentPreview->SetActorLocationAndRotation(Plant->GetActorTransform().TransformPosition(FactoryPreviewA+(FactoryEquipmentChoice==3?EquipmentRotation.RotateVector(FVector(65,0,0)):FVector::ZeroVector)),(Plant->GetActorQuat()*EquipmentRotation.Quaternion()).Rotator());const auto Colour=bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(2.4,.1,.04);for(int Slot=0;Slot<Mesh->GetNumMaterials();++Slot)if(auto* Material=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot)))Material->SetVectorParameterValue(TEXT("GlowColor"),Colour);
 TArray<UStaticMeshComponent*> GhostParts;FactoryEquipmentPreview->GetComponents(GhostParts);for(auto* Part:GhostParts)for(int Slot=0;Slot<Part->GetNumMaterials();++Slot)if(auto* Material=Cast<UMaterialInstanceDynamic>(Part->GetMaterial(Slot)))Material->SetVectorParameterValue(TEXT("GlowColor"),Colour);
 auto* Access=FactoryEquipmentAccessPreview->GetStaticMeshComponent();if(!Access->GetStaticMesh()){Access->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Access->SetMaterial(0,UMaterialInstanceDynamic::Create(Base,this));}const auto Area=FactoryEquipmentChoice>=4&&FactoryEquipmentChoice<=6?FFoundrylineFactorySimulation::MachineWorkspaceBounds(FactoryEquipmentChoice-3,FactoryPreviewA,FactoryStationQuarter):FFoundrylinePlanningEquipment::Service(FactoryPreviewA,FactoryStationQuarter,FFoundrylinePlanningEquipment::Size(FactoryEquipmentChoice,FactoryRackLevels).X);FactoryEquipmentAccessPreview->SetActorTransform(FTransform(Plant->GetActorQuat(),Plant->GetActorTransform().TransformPosition(FVector(Area.GetCenter(),23)),FVector(Area.GetSize().X/100,Area.GetSize().Y/100,.015)));if(auto* Material=Cast<UMaterialInstanceDynamic>(Access->GetMaterial(0)))Material->SetVectorParameterValue(TEXT("GlowColor"),Colour*.35f);
 UStaticMeshComponent* Rear=nullptr;TInlineComponentArray<UStaticMeshComponent*> AccessParts(FactoryEquipmentAccessPreview);for(auto* C:AccessParts)if(C->ComponentHasTag(TEXT("RackRearAccess")))Rear=C;
 const bool Rack=FactoryEquipmentChoice==1||FactoryEquipmentChoice==2;
 if(Rack&&!Rear){Rear=NewObject<UStaticMeshComponent>(FactoryEquipmentAccessPreview);Rear->ComponentTags.Add(TEXT("RackRearAccess"));Rear->SetupAttachment(Access);Rear->SetAbsolute(true,true,true);Rear->SetStaticMesh(Access->GetStaticMesh());Rear->SetMaterial(0,UMaterialInstanceDynamic::Create(Base,this));Rear->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rear->SetCanEverAffectNavigation(false);Rear->SetCastShadow(false);Rear->RegisterComponent();FactoryEquipmentAccessPreview->AddInstanceComponent(Rear);}
 if(Rear)Rear->SetVisibility(Rack);
 if(Rack){auto Check=Plant->Model;Check.State=Plant->Snapshot;Check.Solids=FFoundrylineWorkshopLayout::PlantSolids(Check.State);const auto Back=FFoundrylinePlanningEquipment::Service(FactoryPreviewA,(FactoryStationQuarter+2)%4,FFoundrylinePlanningEquipment::Size(FactoryEquipmentChoice,FactoryRackLevels).X);
  Rear->SetWorldTransform(FTransform(Plant->GetActorQuat(),Plant->GetActorTransform().TransformPosition(FVector(Back.GetCenter(),23)),FVector(Back.GetSize().X/100,Back.GetSize().Y/100,.015)));
  auto Tint=[&](UStaticMeshComponent* C,const FBox2D& A){if(auto* Material=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))Material->SetVectorParameterValue(TEXT("GlowColor"),(FFoundrylineRackAccess::Clear(Check,A)?FLinearColor(.025,.48,1):FLinearColor(2.4,.1,.04))*.35f);};Tint(Access,Area);Tint(Rear,Back);
 }
 }}
 const bool ShowStation=Active&&FactoryTool==TEXT("Station");
 if(ShowStation&&FactoryStationPreview.IsEmpty()){auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));
  const TCHAR* Machine=TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Machine.SM_Welder_Machine");
  const TCHAR* Paths[]={TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_StandardWeldingBench.SM_StandardWeldingBench"),TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureKit.SM_WeldingFixtureKit"),TEXT("/Game/Foundryline/Products/HeadacheRack/SM_HeadacheRack.SM_HeadacheRack"),nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,Machine,Machine};
  // 13 hologram parts, a solid four-bar border round each of the IN and OUT bays (13..20), each OUT pallet's outline (21..28),
  // then each IN supply pallet's outline (29..36).
  for(int I=0;I<37;++I){FActorSpawnParameters SP;SP.Owner=this;auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,SP);if(!A)continue;auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);UStaticMesh* Mesh=I<13&&Paths[I]?LoadObject<UStaticMesh>(nullptr,Paths[I]):nullptr;if(!Mesh)Mesh=LoadObject<UStaticMesh>(nullptr,I==3||I==4?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube"));C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);for(int M=0;M<C->GetNumMaterials();++M)C->SetMaterial(M,UMaterialInstanceDynamic::Create(Glow,this));FactoryStationPreview.Add(A);
   if(I==5||I==6){auto* Label=NewObject<UTextRenderComponent>(A);Label->SetupAttachment(A->GetRootComponent());Label->SetUsingAbsoluteLocation(true);Label->SetUsingAbsoluteRotation(true);Label->SetUsingAbsoluteScale(true);
    Label->SetText(FText::FromString(I==5?TEXT("IN"):TEXT("OUT")));Label->SetHorizontalAlignment(EHTA_Center);Label->SetVerticalAlignment(EVRTA_TextCenter);Label->SetWorldSize(70);Label->SetCastShadow(false);Label->RegisterComponent();FactoryStationLabels.Add(Label);}}
 }
 if(ShowStation&&!FactorySupplyPreview){FActorSpawnParameters Params;Params.Owner=this;FactorySupplyPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,Params);auto* Mesh=FactorySupplyPreview->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/AssemblySupply/SM_AssemblySupplyBin_4.SM_AssemblySupplyBin_4")));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));for(int Slot=0;Slot<Mesh->GetNumMaterials();++Slot)Mesh->SetMaterial(Slot,UMaterialInstanceDynamic::Create(Glow,this));}
 if(FactorySupplyPreview){FactorySupplyPreview->SetActorHiddenInGame(!ShowStation);if(ShowStation){FFoundrylineWeldingStationRecord Preview;Preview.Center=FactoryPreviewA;Preview.QuarterTurns=FactoryStationQuarter;Preview.bSwapSides=bFactoryStationSwap;FactorySupplyPreview->SetActorTransform(FFoundrylineAssemblySupplyCatalog::Transform(Preview)*Plant->GetActorTransform());auto* Mesh=FactorySupplyPreview->GetStaticMeshComponent();for(int Slot=0;Slot<Mesh->GetNumMaterials();++Slot)if(auto* Material=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot)))Material->SetVectorParameterValue(TEXT("GlowColor"),bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(2.4,.1,.04));}}
 for(AStaticMeshActor* A:FactoryStationPreview)if(A)A->SetActorHiddenInGame(!ShowStation);
 if(ShowStation){using Layout=FFoundrylineWeldingStationLayout;FFoundrylineWeldingStationRecord S;S.Center=FactoryPreviewA;S.QuarterTurns=FactoryStationQuarter;S.bSwapSides=bFactoryStationSwap;
  const FLinearColor Glow=bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(2.4,.1,.04);const auto Yaw=Layout::Rotation(S);
  auto Pad=[&](const FBox2D& B,float Height){return FTransform(FRotator::ZeroRotator,FVector(B.GetCenter(),20+Height/2),FVector(B.GetSize().X/100,B.GetSize().Y/100,Height/100));};
  const FTransform Poses[]={Layout::BenchTransform(S),Layout::BenchTransform(S),Layout::ProductTransform(S),FTransform(Yaw,Layout::WorkPoint(S,0)+FVector(0,0,1),FVector(.6,.6,.02)),FTransform(Yaw,Layout::WorkPoint(S,1)+FVector(0,0,1),FVector(.6,.6,.02)),Pad(Layout::Port(S,false),2),Pad(Layout::Port(S,true),2),Pad(Layout::ForkApproach(S,false),.6),Pad(Layout::ForkApproach(S,true),.6),Pad(Layout::WorkerApproach(S,0),.6),Pad(Layout::WorkerApproach(S,1),.6),Layout::MachineTransform(S,0),Layout::MachineTransform(S,1)};
  // IN and OUT bays (with their forklift approaches) in the painted colours: a deep, saturated blue for IN, distinct from the
  // light-blue station hologram, and green for OUT, each with a solid border. Everything is red when the spot is invalid.
  const FLinearColor InGlow=bFactoryPresetValid?FLinearColor(0,.035,3.2):Glow,OutGlow=bFactoryPresetValid?FLinearColor(.02,1.6,.08):Glow;
  auto Bar=[&](FVector2D A,FVector2D B){const FVector2D C=(A+B)/2,Size=(B-A).GetAbs()+FVector2D(6,6);return FTransform(FRotator::ZeroRotator,FVector(C,24),FVector(Size.X/100,Size.Y/100,.04));};
  auto Border=[&](const FBox2D& B,int Edge){return Edge==0?Bar(B.Min,FVector2D(B.Max.X,B.Min.Y)):Edge==1?Bar(FVector2D(B.Min.X,B.Max.Y),B.Max):Edge==2?Bar(B.Min,FVector2D(B.Min.X,B.Max.Y)):Bar(FVector2D(B.Max.X,B.Min.Y),B.Max);};
  // The OUT and IN pallet outlines are white on their bays so each pallet reads on its own.
  const FLinearColor PalletGlow=bFactoryPresetValid?FLinearColor(2.6,2.6,2.6):FLinearColor(2.6,1.4,1.2);
  for(int I=0;I<FactoryStationPreview.Num()&&I<37;++I)if(AStaticMeshActor* A=FactoryStationPreview[I]){if(I>=33){A->SetActorHiddenInGame(true);continue;}A->SetActorTransform((I<13?Poses[I]:I<21?Border(Layout::Port(S,I>=17),(I-13)%4):I<29?Border(Layout::OutPalletRect(S,(I-21)/4),(I-21)%4):Border(Layout::InPalletRect(S,(I-29)/4),(I-29)%4))*Plant->GetActorTransform());auto* C=A->GetStaticMeshComponent();
   if(I>=21){A->SetActorScale3D(A->GetActorScale3D()*FVector(1,1,1.5));A->AddActorWorldOffset(FVector(0,0,2));}
   const FLinearColor Colour=I==5?InGlow:I==6?OutGlow:I==7?InGlow*.45f:I==8?OutGlow*.45f:I>=13&&I<17?InGlow*1.6f:I>=21?PalletGlow:I>=17?OutGlow*1.6f:Glow;for(int M=0;M<C->GetNumMaterials();++M)if(auto* D=Cast<UMaterialInstanceDynamic>(C->GetMaterial(M)))D->SetVectorParameterValue(TEXT("GlowColor"),Colour);}
  // Labels stay white when the spot is invalid, and float just above table height there, so they read through the red tint
  // even when the invalid spot is on top of other equipment.
  for(int I=0;I<FactoryStationLabels.Num();++I)if(auto* Label=FactoryStationLabels[I].Get()){const bool Out=I==1;Label->SetWorldLocationAndRotation(Layout::PortCenter(S,Out)+FVector(0,0,bFactoryPresetValid?6:130),FRotator(90,Yaw.Yaw+(Layout::PortOnLeft(S,Out)?0:180),0));
   Label->SetTextRenderColor(!bFactoryPresetValid?FColor::White:Out?FColor(60,235,90):FColor(40,90,255));}
 }
 UpdateFactoryFlowPreview(ShowEquipment,ShowStation);
 const bool Show=Active&&FactoryTool==TEXT("Zone");
 if(Show&&!FactoryProductPreview){FActorSpawnParameters P;P.Owner=this;FactoryProductPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);auto* C=FactoryProductPreview->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Products/Tankers/StraightBarrel/SM_Tanker_Body.SM_Tanker_Body")));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));for(int I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,UMaterialInstanceDynamic::Create(M,this));}
 if(Show&&!FactoryWelderPreview){FActorSpawnParameters P;P.Owner=this;FactoryWelderPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);auto* C=FactoryWelderPreview->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Complete.SM_Welder_Complete")));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));for(int I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,UMaterialInstanceDynamic::Create(M,this));}
 if(FactoryProductPreview)FactoryProductPreview->SetActorHiddenInGame(!Show);if(FactoryWelderPreview)FactoryWelderPreview->SetActorHiddenInGame(!Show);
 if(!Show)return;
 FFoundrylineFactoryZone Z;if(!FFoundrylineWorkZoneLayout::Create(FactoryPreviewA,FactoryZonePreset,FactoryZoneQuarter,FactoryZoneSide,Z))return;
 if(FactoryProductPreview){FactoryProductPreview->SetActorTransform(FTransform(FFoundrylineWorkZoneLayout::Rotation(Z),FFoundrylineWorkZoneLayout::ProductCenter(Z)+FFoundrylineWorkZoneLayout::Rotation(Z).RotateVector(FVector(404,0,0)))*Plant->GetActorTransform());auto* C=FactoryProductPreview->GetStaticMeshComponent();for(int I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))M->SetVectorParameterValue(TEXT("GlowColor"),bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(1,.05,.015));}
 if(FactoryWelderPreview){FactoryWelderPreview->SetActorTransform(FTransform(FFoundrylineWorkZoneLayout::Rotation(Z),FFoundrylineWorkZoneLayout::WelderPosition(Z))*Plant->GetActorTransform());auto* C=FactoryWelderPreview->GetStaticMeshComponent();for(int I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))M->SetVectorParameterValue(TEXT("GlowColor"),bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(1,.05,.015));}
}
void AFoundrylinePlayerController::FactoryAction(FName Name){auto* Plant=FactoryPlant();if(!Plant)return;const auto& S=Plant->Snapshot;const FString A=Name.ToString();FFoundrylineFactoryCommand C;using FactoryAct=EFoundrylineFactoryAction;
 if(A==TEXT("GridStep")){FactorySnapIndex=(FactorySnapIndex+1)%3;return;}
 if(A==TEXT("EdgeSnap")){bFactoryEdgeSnap=!bFactoryEdgeSnap;return;}
 if(A==TEXT("GridVisible")){bFactoryGrid=!bFactoryGrid;return;}
 if(A==TEXT("ZoneGap")){FactoryGapIndex=(FactoryGapIndex+1)%3;return;}
 if(A==TEXT("ToolDone")){FactoryTool=NAME_None;bFactoryDragging=false;return;}
 if(A.StartsWith(TEXT("ZonePreset"))){FactoryZonePreset=FMath::Clamp(FCString::Atoi(*A.RightChop(10)),0,2);return;}
 if(A==TEXT("ZoneNext")){if(!S.Zones.IsEmpty()){int I=S.Zones.IndexOfByPredicate([&](const auto& Z){return Z.Id==FactoryZone;});I=FMath::Max(0,I);FactoryZone=S.Zones[(I+1)%S.Zones.Num()].Id;FactorySelectedPart=-1;}return;}
 if(A==TEXT("ZoneRotate")){FactoryZoneQuarter=(FactoryZoneQuarter+1)%4;return;}
 if(A==TEXT("ZoneSide")){FactoryZoneSide=-FactoryZoneSide;return;}
 if(A.StartsWith(TEXT("ShopItem"))){FactoryEquipmentChoice=FCString::Atoi(*A.RightChop(8));if(FactoryEquipmentChoice<0||FactoryEquipmentChoice>7)return;if(IsWalking()||!bPropertyViewAccess){FactoryStatus=TEXT("Use the office computer Planning View to place equipment");return;}if(S.Running){C.Action=FactoryAct::Run;C.Flag=false;SendFactory(C);FactoryStatus=TEXT("Pausing this business for equipment placement");}FactoryTool=FactoryEquipmentChoice==0?TEXT("Station"):TEXT("Equipment");bFactoryDragging=false;return;}
 if(A==TEXT("RackLevel")){FactoryRackLevels=FactoryRackLevels%3+1;return;}
 if(A==TEXT("StationRotate")){FactoryStationQuarter=(FactoryStationQuarter+1)%4;return;}
 if(A==TEXT("StationSide")){bFactoryStationSwap=!bFactoryStationSwap;return;}
 // Station supplies: the headache-rack batch is product data; bought parts use the normal paid supplier booking.
 if(A==TEXT("StationSupply")){const auto* St=S.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==FactoryStation;});if(!St)return;C.Action=FactoryAct::ConfigureWeldingStation;C.Id=St->Id;C.Other=St->Recipe?0:1;C.Count=St->Recipe?0:10;C.TargetPlantId=Plant->BusinessPlantId;SendFactory(C);return;}
 if(A.StartsWith(TEXT("StationBuy"))){const auto* St=S.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==FactoryStation;});const FString Value=A.RightChop(10);if(!St||!Value.IsNumeric())return;const int Part=FCString::Atoi(*Value);if(!FFoundrylineFactoryCatalog::Bought(EFoundrylinePart(FMath::Clamp(Part,0,255))))return;
  FFoundrylineFactorySimulation View;View.State=S;FFoundrylineCommerceCommand Buy;Buy.Action=EFoundrylineCommerceAction::BuyParts;Buy.Id=St->Id;Buy.Choice=Part;Buy.Quantity=FMath::Min(FMath::Max(0,View.StationShortfall(*St,EFoundrylinePart(Part))-View.StationRackSupply(EFoundrylinePart(Part))),FFoundrylineFactoryCatalog::Part(EFoundrylinePart(Part)).Capacity);if(Buy.Quantity>0)SendOperations(Buy);return;}
 if(A==TEXT("StationRemove")){
if(!S.WeldingStations.ContainsByPredicate([&](const auto& V){return V.Id==FactoryStation;}))return;C.Action=FactoryAct::RemoveWeldingStation;C.Id=FactoryStation;C.TargetPlantId=Plant->BusinessPlantId;FactoryStation=0;SendFactory(C);return;}
 if(A.StartsWith(TEXT("LoadJob"))||A.StartsWith(TEXT("QueueJob"))){const bool Queue=A.StartsWith(TEXT("QueueJob"));const int Order=FCString::Atoi(*A.RightChop(Queue?8:7));const auto* O=S.Commerce.Orders.FindByPredicate([&](const auto& V){return V.Id==Order&&V.Status==EFoundrylineOrderState::Accepted;});const auto* Machine=S.Machines.FindByPredicate([&](const auto& V){return V.Id==FactoryMachine;});if(!O||!Machine)return;const auto Programs=S.Programs.FilterByPredicate([&](const auto& V){return V.Machine==FactoryMachine;});if(Programs.IsEmpty())return;EFoundrylinePart Part=EFoundrylinePart(O->Part);if(Machine->Kind()==1&&Part==EFoundrylinePart::BentBracket)Part=EFoundrylinePart::Bracket;if(O->Part==uint8(EFoundrylinePart::HeadacheRack))Part=Programs[FMath::Clamp(FactoryProgramIndex,0,Programs.Num()-1)].Part;const int Index=Programs.IndexOfByPredicate([&](const auto& V){return V.Part==Part;});if(Index<0){FactoryStatus=TEXT("This job needs a different machine or component recipe");return;}FactoryProgramIndex=Index;if(Queue){C.Action=FactoryAct::QueueMachineJob;C.Id=FactoryMachine;C.Other=Order;C.Part=Part;C.Count=0;SendFactory(C);}else {FFoundrylineFactorySimulation Preview;Preview.State=S;FString Error;if(Preview.QueueMachineJob(FactoryMachine,Order,0,Error,Part)){FactoryBatch=Preview.State.Machines.FindByPredicate([&](const auto& V){return V.Id==FactoryMachine;})->Queue.Last().Quantity;FactoryStatus=TEXT("Job loaded / review quantity, then queue the job");}else FactoryStatus=Error;}return;}
 if(A.StartsWith(TEXT("CancelBatch"))){C.Action=FactoryAct::CancelMachineBatch;C.Id=FactoryMachine;C.Other=FCString::Atoi(*A.RightChop(11));SendFactory(C);return;}
 if(A==TEXT("BatchValue")){FactoryEditField=Name;FactoryEditText.Reset();return;}
 if(A==TEXT("Toggle")){const bool Show=!bFactoryMenu;if(FactoryMachine)OpenFactoryComputer(0);bFactoryMenu=Show;bFactoryDragging=false;FactoryTool=NAME_None;BuildTool=NAME_None;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)It->HidePreview();if(bLogisticsOverlay)ToggleLogisticsOverlay();return;}
 if(A==TEXT("ReturnOffice")){if(bPropertyViewAccess&&!IsWalking())ToggleWalk();return;}
 if(A==TEXT("CloseComputer")){OpenFactoryComputer(0);return;}
 if(A==TEXT("Requirements")){OpenFactoryComputer(4);return;}
 if(A.StartsWith(TEXT("PartDetail"))){FactorySelectedPart=FMath::Clamp(FCString::Atoi(*A.RightChop(10)),0,9);return;}
 if(A==TEXT("Overlay")||A==TEXT("FloorOverlay")){bFactoryOverlay=!bFactoryOverlay;return;}
 if(A.StartsWith(TEXT("Page"))){FactoryPage=FCString::Atoi(*A.RightChop(4));FactoryTool=NAME_None;bFactoryDragging=false;return;}
 if(A.StartsWith(TEXT("Computer"))){OpenFactoryComputer(FCString::Atoi(*A.RightChop(8)));return;}
 if(A.StartsWith(TEXT("Slot"))){FactorySlot=FCString::Atoi(*A.RightChop(4));return;}
 if(A==TEXT("QueuePrev")||A==TEXT("QueueNext")){FactoryStoragePage=FMath::Max(0,FactoryStoragePage+(A==TEXT("QueueNext")?1:-1));return;}
 if(A==TEXT("PartNext")){TArray<FFoundrylineFactoryProgram> List=S.Programs.FilterByPredicate([&](const auto& P){return P.Machine==FactoryMachine;});if(List.Num()){FactoryProgramIndex=(FactoryProgramIndex+1)%List.Num();}return;}
 if(A==TEXT("BatchMinus")||A==TEXT("BatchPlus")){FactoryBatch=FMath::Clamp(FactoryBatch+(A.EndsWith(TEXT("Plus"))?1:-1),1,200);return;}
 if(A==TEXT("Auto")){if(S.bCutPartsStarter){bFactoryAuto=false;return;}bFactoryAuto=!bFactoryAuto;return;}
 if(A==TEXT("ToolStation")&&FactoryTool==TEXT("Station")){FactoryTool=NAME_None;bFactoryDragging=false;return;}
 if(A==TEXT("ToolZone")||A==TEXT("ToolStation")||A==TEXT("ToolForklift")||A==TEXT("ToolPedestrian")||A==TEXT("ToolCrossing")||A==TEXT("ToolErase")){bFactoryDragging=false;FactoryStatus.Reset();FactoryTool=FName(*A.RightChop(4));if(FactoryTool==TEXT("Pedestrian"))FactoryWidth=121.92f;else if(FactoryWidth<300)FactoryWidth=365.76f;return;}
 if(A==TEXT("WidthMinus")||A==TEXT("WidthPlus")){if(FactoryTool==TEXT("Forklift"))return;const int Feet=FMath::RoundToInt(FactoryWidth/30.48f)+(A.EndsWith(TEXT("Plus"))?1:-1);FactoryWidth=FMath::Clamp(Feet,FactoryTool==TEXT("Pedestrian")?4:10,20)*30.48f;return;}
 if(A==TEXT("Direction")){if(FactoryTool==TEXT("Forklift"))return;FactoryDirection=(FactoryDirection+1)%3;return;}
 if(A==TEXT("StoragePrev")||A==TEXT("StorageNext")){FactoryStoragePage=FMath::Clamp(FactoryStoragePage+(A==TEXT("StorageNext")?1:-1),0,FMath::Max(0,(S.Slots.Num()-1)/5));return;}
 if(A==TEXT("RouteNext")){int I=S.Routes.IndexOfByPredicate([&](const auto& R){return R.Id==FactoryRoute;});if(S.Routes.Num())FactoryRoute=S.Routes[(I+1)%S.Routes.Num()].Id;return;}
 TArray<int> Endpoints={10,11,1,2,3};for(const auto& R:S.Slots)Endpoints.Add(R.Id);for(const auto& Z:S.Zones)Endpoints.Add(Z.Id);
 if(A==TEXT("EndpointNext")){FactoryEndpoint=(FactoryEndpoint+1)%Endpoints.Num();return;}
 if(!FactoryZone&&S.Zones.Num())FactoryZone=S.Zones[0].Id;if(!FactorySlot&&S.Slots.Num())FactorySlot=S.Slots[0].Id;
 if(A==TEXT("Run")){C.Action=FactoryAct::Run;C.Flag=!S.Running;}
 else if(A==TEXT("Speed1")||A==TEXT("Speed4")||A==TEXT("Speed8")){C.Action=FactoryAct::Speed;C.Count=FCString::Atoi(*A.RightChop(5));}
 else if(A==TEXT("Speed")){C.Action=FactoryAct::Speed;C.Count=S.Speed==1?4:S.Speed==4?8:1;}
 else if(A==TEXT("Order")){C.Action=FactoryAct::Order;C.Id=FactoryZone;}
 else if(A==TEXT("Collect")){C.Action=FactoryAct::Collect;C.Id=FactoryZone;}
 else if(A==TEXT("Welder")||A==TEXT("Driver")){OpenOperations(1);if(OperationsScreen==1){OperationsPage=7;OperationsWorkspaceTab=0;OperationsEmployeeChoice=0;}else FactoryStatus=TEXT("Use the office computer to hire employees and assign jobs");return;}
 else if(A==TEXT("Steel")){C.Action=FactoryAct::ReceiveSteel;C.Count=10;}
 else if(A==TEXT("Kits")){C.Action=FactoryAct::ReceiveKits;}
 else if(A==TEXT("Save")){C.Action=FactoryAct::Save;}
 else if(A==TEXT("Load")){C.Action=FactoryAct::Load;}
 else if(A==TEXT("NewRoute")){C.Action=FactoryAct::AddRoute;}
 else if(A==TEXT("Source")||A==TEXT("Destination")){C.Action=A==TEXT("Source")?FactoryAct::LinkSource:FactoryAct::LinkDestination;C.Id=FactoryRoute;C.Other=Endpoints[FMath::Clamp(FactoryEndpoint,0,Endpoints.Num()-1)];if(const auto* R=S.Routes.FindByPredicate([&](const auto& X){return X.Id==FactoryRoute;}))C.Flag=!(A==TEXT("Source")?R->Sources:R->Destinations).Contains(C.Other);}
 else if(A==TEXT("AssignNext")){auto* Slot=S.Slots.FindByPredicate([&](const auto& R){return R.Id==FactorySlot;});if(!Slot)return;C.Action=FactoryAct::AssignSlot;C.Id=Slot->Id;C.Part=EFoundrylinePart((int(Slot->Part)+1)%10);}
 else if(A==TEXT("Apply")){auto List=S.Programs.FilterByPredicate([&](const auto& P){return P.Machine==FactoryMachine;});if(!List.IsValidIndex(FactoryProgramIndex))return;C.Action=FactoryAct::Program;C.Id=FactoryMachine;C.Part=List[FactoryProgramIndex].Part;C.Count=FactoryBatch;C.Flag=S.bCutPartsStarter?false:bFactoryAuto;}
 else if(A==TEXT("MachinePause")){auto* M=S.Machines.FindByPredicate([&](const auto& X){return X.Id==FactoryMachine;});if(!M)return;C.Action=FactoryAct::MachineEnabled;C.Id=M->Id;C.Flag=!M->Enabled;}
 else return;SendFactory(C);
}
void AFoundrylinePlayerController::CommitFactoryNumber(){if(!FactoryEditText.IsEmpty()){const float V=FCString::Atof(*FactoryEditText);if(FactoryEditField==TEXT("BatchValue"))FactoryBatch=FMath::Clamp(FMath::RoundToInt(V),1,200);}FactoryEditField=NAME_None;}
