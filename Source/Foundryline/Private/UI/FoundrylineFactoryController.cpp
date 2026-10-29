#include "UI/FoundrylinePlayerController.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylineBuildSite.h"
#include "InputKeyEventArgs.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "Grid/FoundrylineFloorGuide.h"
#include "Core/FoundrylineUnits.h"
AFoundrylineFactoryPlant* AFoundrylinePlayerController::FactoryPlant()const{for(TActorIterator<AFoundrylineFactoryPlant> It(GetWorld());It;++It){auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());if(!R||R->CanOperate(this,It->BusinessCompanyId,It->BusinessPlantId))return *It;}return nullptr;}
void AFoundrylinePlayerController::SendFactory(FFoundrylineFactoryCommand C){FactoryStatus.Reset();ServerFactoryCommand(C,++FactorySequence);}
void AFoundrylinePlayerController::ServerFactoryCommand_Implementation(const FFoundrylineFactoryCommand& C,uint32 Sequence){
 if(Sequence<=LastFactorySequence)return;LastFactorySequence=Sequence;double Now=GetWorld()->GetTimeSeconds();if(Now-FactoryRateWindow>=1){FactoryRateWindow=Now;FactoryRateCount=0;}if(++FactoryRateCount>20){ClientFactoryReply(TEXT("Please wait before sending more factory actions"));return;}
 FString E;auto* Plant=FactoryPlant();if(!Plant||!Plant->Execute(this,C,E))ClientFactoryReply(E.IsEmpty()?TEXT("Plant unavailable"):E);
}
void AFoundrylinePlayerController::ClientFactoryReply_Implementation(const FString& E){FactoryStatus=E;if(!E.IsEmpty())OperationsHireRequest.Invalidate();if(OperationsScreen==1&&OperationsPage==7)OperationsStatus=E;}
bool AFoundrylinePlayerController::FactoryOverHUD()const{float X,Y;int W,H;GetViewportSize(W,H);if(!GetMousePosition(X,Y)||W<=0||H<=0)return false;const float S=H/1080.f;if(FactoryMachine>0||X<420*S||X>W-210*S&&Y<140*S)return true;if((bFactoryOverlay||FactorySelectedPart>=0)&&X>W-330*S&&Y>150*S&&Y<(FactorySelectedPart>=0?640:400)*S)return true;return false;}
int32 AFoundrylinePlayerController::FocusedComputer()const{if(!IsWalking()||!PlayerCameraManager||!FactoryPlant())return 0;const FVector Eye=PlayerCameraManager->GetCameraLocation(),Forward=PlayerCameraManager->GetCameraRotation().Vector();int Best=0;float Distance=260;for(const auto& Machine:FactoryPlant()->Snapshot.Machines){const int I=Machine.Id;const FVector Delta=FactoryPlant()->ScreenWorldPosition(I)-Eye;const float D=Delta.Size();if(D<Distance&&FVector::DotProduct(Delta.GetSafeNormal(),Forward)>.35){Distance=D;Best=I;}}return Best;}
void AFoundrylinePlayerController::OpenFactoryComputer(int Id){if(auto* P=FactoryPlant()){if(P->Snapshot.bCutPartsStarter){bFactoryAuto=false;if(Id>0&&!P->Snapshot.Machines.ContainsByPredicate([&](const auto& M){return M.Id==Id;}))return;}}FactoryMachine=Id;FactoryTool=NAME_None;bFactoryDragging=false;
 if(Id>0){bFactoryMenu=true;FactoryProgramIndex=0;if(!bFactoryInputLocked){SetIgnoreLookInput(true);SetIgnoreMoveInput(true);bFactoryInputLocked=true;}FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);bShowMouseCursor=true;}
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
  if((P.Key==EKeys::MouseScrollUp||P.Key==EKeys::MouseScrollDown)&&IsInputKeyDown(EKeys::LeftAlt)){FactoryAction(P.Key==EKeys::MouseScrollUp?TEXT("WidthPlus"):TEXT("WidthMinus"));return true;}
 }
 if(P.Key==EKeys::RightMouseButton&&P.Event==IE_Pressed){FactoryTool=NAME_None;bFactoryDragging=false;return true;}
 if(FactoryTool==TEXT("Zone")&&P.Event==IE_Pressed&&(P.Key==EKeys::R||P.Key==EKeys::V)){FactoryAction(P.Key==EKeys::R?TEXT("ZoneRotate"):TEXT("ZoneSide"));return true;}
 if(P.Key==EKeys::LeftMouseButton){if(FactoryOverHUD()){Super::InputKey(P);if(P.Event==IE_Released)bFactoryDragging=false;return true;}FVector A;if(!FactoryFloorCursor(A)){if(P.Event==IE_Released)bFactoryDragging=false;return true;}
  if(FactoryTool==TEXT("Zone")||FactoryTool==TEXT("Crossing")){if(P.Event==IE_Pressed){if(!PreviewFactoryFloor(A)){FactoryStatus=FactoryPlacementHint;return true;}CommitFactoryFloorBrush();}return true;}
  if(P.Event==IE_Pressed){if(FactoryTool.IsNone()){auto* Plant=FactoryPlant();FFoundrylineFactorySimulation View;View.State=Plant->Snapshot;for(const auto& Z:Plant->Snapshot.Zones)for(auto Part:{EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings}){const FVector P=View.BufferPosition(Z,Part);const auto Size=FFoundrylineWorkZoneLayout::BaySize(Z,Part);if(FBox2D(FVector2D(P)-Size/2,FVector2D(P)+Size/2).IsInside(FVector2D(A))){FactoryZone=Z.Id;FactorySelectedPart=int(Part);FactoryPage=0;return true;}}float Best=500;int Pick=0;for(const auto& Machine:Plant->Snapshot.Machines){const int I=Machine.Id;const float D=FVector::Dist2D(A,Plant->MachineWorldPosition(I));if(D<Best){Best=D;Pick=I;}}if(Pick){OpenFactoryComputer(Pick);return true;}for(const auto& Z:Plant->Snapshot.Zones)if(FFoundrylineWorkZoneLayout::Bounds(Z).IsInside(FVector2D(A))){FactoryZone=Z.Id;FactoryPage=0;return true;}return true;}
   if(FactoryTool==TEXT("Erase")){auto* Plant=FactoryPlant();for(int I=Plant->Snapshot.Lanes.Num()-1;I>=0;--I)if(FBox2D(Plant->Snapshot.Lanes[I].Min,Plant->Snapshot.Lanes[I].Max).IsInside(FVector2D(A))){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::EraseLane;C.Id=Plant->Snapshot.Lanes[I].Id;SendFactory(C);break;}return true;}
   FactoryDragStart=FactoryDragEnd=A;bFactoryDragging=true;PreviewFactoryFloor(A);
  }else if(P.Event==IE_Released&&bFactoryDragging){FactoryDragEnd=A;const bool Valid=PreviewFactoryFloor(A);bFactoryDragging=false;if(!Valid){FactoryStatus=FactoryPlacementHint;return true;}CommitFactoryFloorBrush();}return true;
 }return false;
}
void AFoundrylinePlayerController::TickFactory(float Dt){
 if(!IsLocalController())return;auto* Plant=FactoryPlant();if(!Plant)return;Plant->bLocalOverlay=bFactoryOverlay;
 FVector Cursor;const bool Active=bFactoryMenu&&!IsWalking()&&!FactoryMachine&&!FactoryTool.IsNone()&&FactoryTool!=TEXT("Erase")&&!FactoryOverHUD()&&FactoryFloorCursor(Cursor);
 if(Active&&!FactoryFloorGuide){FActorSpawnParameters P;P.Owner=this;FactoryFloorGuide=GetWorld()->SpawnActor<AFoundrylineFloorGuide>(FVector::ZeroVector,FRotator::ZeroRotator,P);}
 if(Active){if(bFactoryDragging)FactoryDragEnd=Cursor;PreviewFactoryFloor(Cursor);}
 if(FactoryFloorGuide)FactoryFloorGuide->Show(FBox2D(Plant->Snapshot.FloorMin,Plant->Snapshot.FloorMax),FactoryPreviewBounds,FMath::Max(FactoryGridStep(),15.24),bFactoryGrid,bFactoryPresetValid,Active);
 const bool Show=Active&&FactoryTool==TEXT("Zone");
 if(Show&&!FactoryProductPreview){FActorSpawnParameters P;P.Owner=this;FactoryProductPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);auto* C=FactoryProductPreview->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Products/Tankers/StraightBarrel/SM_Tanker_Body.SM_Tanker_Body")));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));for(int I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,UMaterialInstanceDynamic::Create(M,this));}
 if(Show&&!FactoryWelderPreview){FActorSpawnParameters P;P.Owner=this;FactoryWelderPreview=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);auto* C=FactoryWelderPreview->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Complete.SM_Welder_Complete")));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryPreview.M_FactoryPreview"));for(int I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,UMaterialInstanceDynamic::Create(M,this));}
 if(FactoryProductPreview)FactoryProductPreview->SetActorHiddenInGame(!Show);if(FactoryWelderPreview)FactoryWelderPreview->SetActorHiddenInGame(!Show);
 if(!Show)return;
 FFoundrylineFactoryZone Z;if(!FFoundrylineWorkZoneLayout::Create(FactoryPreviewA,FactoryZonePreset,FactoryZoneQuarter,FactoryZoneSide,Z))return;
 if(FactoryProductPreview){FactoryProductPreview->SetActorLocationAndRotation(FFoundrylineWorkZoneLayout::ProductCenter(Z)+FFoundrylineWorkZoneLayout::Rotation(Z).RotateVector(FVector(404,0,0)),FFoundrylineWorkZoneLayout::Rotation(Z));auto* C=FactoryProductPreview->GetStaticMeshComponent();for(int I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))M->SetVectorParameterValue(TEXT("GlowColor"),bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(1,.05,.015));}
 if(FactoryWelderPreview){FactoryWelderPreview->SetActorLocationAndRotation(FFoundrylineWorkZoneLayout::WelderPosition(Z),FFoundrylineWorkZoneLayout::Rotation(Z));auto* C=FactoryWelderPreview->GetStaticMeshComponent();for(int I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))M->SetVectorParameterValue(TEXT("GlowColor"),bFactoryPresetValid?FLinearColor(.025,.48,1):FLinearColor(1,.05,.015));}
}
void AFoundrylinePlayerController::FactoryAction(FName Name){auto* Plant=FactoryPlant();if(!Plant)return;const auto& S=Plant->Snapshot;const FString A=Name.ToString();FFoundrylineFactoryCommand C;using Act=EFoundrylineFactoryAction;
 if(A==TEXT("GridStep")){FactorySnapIndex=(FactorySnapIndex+1)%3;return;}
 if(A==TEXT("EdgeSnap")){bFactoryEdgeSnap=!bFactoryEdgeSnap;return;}
 if(A==TEXT("GridVisible")){bFactoryGrid=!bFactoryGrid;return;}
 if(A==TEXT("ZoneGap")){FactoryGapIndex=(FactoryGapIndex+1)%3;return;}
 if(A==TEXT("ToolDone")){FactoryTool=NAME_None;bFactoryDragging=false;return;}
 if(A.StartsWith(TEXT("ZonePreset"))){FactoryZonePreset=FMath::Clamp(FCString::Atoi(*A.RightChop(10)),0,2);return;}
 if(A==TEXT("ZoneNext")){if(!S.Zones.IsEmpty()){int I=S.Zones.IndexOfByPredicate([&](const auto& Z){return Z.Id==FactoryZone;});I=FMath::Max(0,I);FactoryZone=S.Zones[(I+1)%S.Zones.Num()].Id;FactorySelectedPart=-1;}return;}
 if(A==TEXT("ZoneRotate")){FactoryZoneQuarter=(FactoryZoneQuarter+1)%4;return;}
 if(A==TEXT("ZoneSide")){FactoryZoneSide=-FactoryZoneSide;return;}
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
 if(A==TEXT("PartNext")){TArray<FFoundrylineFactoryProgram> List=S.Programs.FilterByPredicate([&](const auto& P){return P.Machine==FactoryMachine;});if(List.Num()){FactoryProgramIndex=(FactoryProgramIndex+1)%List.Num();}return;}
 if(A==TEXT("BatchMinus")||A==TEXT("BatchPlus")){FactoryBatch=FMath::Clamp(FactoryBatch+(A.EndsWith(TEXT("Plus"))?1:-1),1,200);return;}
 if(A==TEXT("Auto")){if(S.bCutPartsStarter){bFactoryAuto=false;return;}bFactoryAuto=!bFactoryAuto;return;}
 if(A==TEXT("ToolZone")||A==TEXT("ToolForklift")||A==TEXT("ToolPedestrian")||A==TEXT("ToolCrossing")||A==TEXT("ToolErase")){bFactoryDragging=false;FactoryStatus.Reset();FactoryTool=FName(*A.RightChop(4));if(FactoryTool==TEXT("Pedestrian"))FactoryWidth=121.92f;else if(FactoryWidth<300)FactoryWidth=365.76f;return;}
 if(A==TEXT("WidthMinus")||A==TEXT("WidthPlus")){const int Feet=FMath::RoundToInt(FactoryWidth/30.48f)+(A.EndsWith(TEXT("Plus"))?1:-1);FactoryWidth=FMath::Clamp(Feet,FactoryTool==TEXT("Pedestrian")?4:10,20)*30.48f;return;}
 if(A==TEXT("Direction")){FactoryDirection=(FactoryDirection+1)%3;return;}
 if(A==TEXT("StoragePrev")||A==TEXT("StorageNext")){FactoryStoragePage=FMath::Clamp(FactoryStoragePage+(A==TEXT("StorageNext")?1:-1),0,FMath::Max(0,(S.Slots.Num()-1)/5));return;}
 if(A==TEXT("RouteNext")){int I=S.Routes.IndexOfByPredicate([&](const auto& R){return R.Id==FactoryRoute;});if(S.Routes.Num())FactoryRoute=S.Routes[(I+1)%S.Routes.Num()].Id;return;}
 TArray<int> Endpoints={10,11,1,2,3};for(const auto& R:S.Slots)Endpoints.Add(R.Id);for(const auto& Z:S.Zones)Endpoints.Add(Z.Id);
 if(A==TEXT("EndpointNext")){FactoryEndpoint=(FactoryEndpoint+1)%Endpoints.Num();return;}
 if(!FactoryZone&&S.Zones.Num())FactoryZone=S.Zones[0].Id;if(!FactorySlot&&S.Slots.Num())FactorySlot=S.Slots[0].Id;
 if(A==TEXT("Run")){C.Action=Act::Run;C.Flag=!S.Running;}
 else if(A==TEXT("Speed1")||A==TEXT("Speed4")||A==TEXT("Speed8")){C.Action=Act::Speed;C.Count=FCString::Atoi(*A.RightChop(5));}
 else if(A==TEXT("Speed")){C.Action=Act::Speed;C.Count=S.Speed==1?4:S.Speed==4?8:1;}
 else if(A==TEXT("Order")){C.Action=Act::Order;C.Id=FactoryZone;}
 else if(A==TEXT("Collect")){C.Action=Act::Collect;C.Id=FactoryZone;}
 else if(A==TEXT("Welder")||A==TEXT("Driver")){OpenOperations(1);if(OperationsScreen==1){OperationsPage=7;OperationsWorkspaceTab=0;OperationsEmployeeChoice=0;}else FactoryStatus=TEXT("Use the office computer to hire employees and assign jobs");return;}
 else if(A==TEXT("Steel")){C.Action=Act::ReceiveSteel;C.Count=10;}
 else if(A==TEXT("Kits")){C.Action=Act::ReceiveKits;}
 else if(A==TEXT("Save")){C.Action=Act::Save;}
 else if(A==TEXT("Load")){C.Action=Act::Load;}
 else if(A==TEXT("NewRoute")){C.Action=Act::AddRoute;}
 else if(A==TEXT("Source")||A==TEXT("Destination")){C.Action=A==TEXT("Source")?Act::LinkSource:Act::LinkDestination;C.Id=FactoryRoute;C.Other=Endpoints[FMath::Clamp(FactoryEndpoint,0,Endpoints.Num()-1)];if(const auto* R=S.Routes.FindByPredicate([&](const auto& X){return X.Id==FactoryRoute;}))C.Flag=!(A==TEXT("Source")?R->Sources:R->Destinations).Contains(C.Other);}
 else if(A==TEXT("AssignNext")){auto* Slot=S.Slots.FindByPredicate([&](const auto& R){return R.Id==FactorySlot;});if(!Slot)return;C.Action=Act::AssignSlot;C.Id=Slot->Id;C.Part=EFoundrylinePart((int(Slot->Part)+1)%10);}
 else if(A==TEXT("Apply")){auto List=S.Programs.FilterByPredicate([&](const auto& P){return P.Machine==FactoryMachine;});if(!List.IsValidIndex(FactoryProgramIndex))return;C.Action=Act::Program;C.Id=FactoryMachine;C.Part=List[FactoryProgramIndex].Part;C.Count=FactoryBatch;C.Flag=S.bCutPartsStarter?false:bFactoryAuto;}
 else if(A==TEXT("MachinePause")){auto* M=S.Machines.FindByPredicate([&](const auto& X){return X.Id==FactoryMachine;});if(!M)return;C.Action=Act::MachineEnabled;C.Id=M->Id;C.Flag=!M->Enabled;}
 else return;SendFactory(C);
}
void AFoundrylinePlayerController::CommitFactoryNumber(){if(!FactoryEditText.IsEmpty()){const float V=FCString::Atof(*FactoryEditText);if(FactoryEditField==TEXT("BatchValue"))FactoryBatch=FMath::Clamp(FMath::RoundToInt(V),1,200);}FactoryEditField=NAME_None;}
