#include "FoundrylineNetworkTestTools.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
bool UFoundrylineNetworkTestTools::RequestLateJoin()
{
#if WITH_EDITOR
 if(GEditor && GEditor->PlayWorld) {GEditor->RequestLateJoin();return true;}
#endif
 return false;
}

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
FString UFoundrylineNetworkTestTools::InspectPlayer(APlayerController* PC)
{
 if(!PC || !PC->GetPawn()) return TEXT("No pawn");
 FVector Eye,PawnEye;FRotator View,PawnView;PC->GetPlayerViewPoint(Eye,View);PC->GetPawn()->GetActorEyesViewPoint(PawnEye,PawnView);
 FString Out=FString::Printf(TEXT("Pawn %s Eye %s View %s PawnEye %s PawnView %s"),*PC->GetPawn()->GetActorLocation().ToString(),*Eye.ToString(),*View.ToString(),*PawnEye.ToString(),*PawnView.ToString());
 for(bool Complex:{false,true}) {
 FHitResult Hit;FCollisionQueryParams Q;Q.bTraceComplex=Complex;Q.AddIgnoredActor(PC->GetPawn());
 PC->GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+View.Vector()*300,ECC_Visibility,Q);
 Out+=FString::Printf(TEXT(" Trace%d %s %s"),Complex,*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString());
 }
 return Out;
}

#include "Containers/Ticker.h"
#include "InputKeyEventArgs.h"
bool UFoundrylineNetworkTestTools::HoldCameraKey(APlayerController* PC,FName Name,float Seconds)
{
#if WITH_EDITOR
 if(!PC || !PC->IsLocalController() || Seconds<=0 || Seconds>2) return false;
 const FKey Key(Name);if(Key!=EKeys::W && Key!=EKeys::A && Key!=EKeys::S && Key!=EKeys::D && Key!=EKeys::Up && Key!=EKeys::Down && Key!=EKeys::Left && Key!=EKeys::Right) return false;
 TWeakObjectPtr<APlayerController> Weak=PC;
 PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,IE_Pressed,FPlatformTime::Cycles64()));
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Key,Remaining=Seconds](float Delta) mutable {
  Remaining-=Delta;if(!Weak.IsValid()) return false;if(Remaining>0) return true;
  Weak->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,IE_Released,FPlatformTime::Cycles64()));return false;
 }));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::DrivePlayer(APlayerController* PC,FVector Direction,float Seconds)
{
#if WITH_EDITOR
 if(!PC || !PC->IsLocalController() || !PC->GetPawn() || Seconds<=0 || Seconds>10 || Direction.IsNearlyZero()) return false;
 TWeakObjectPtr<APawn> Pawn=PC->GetPawn();Direction=Direction.GetSafeNormal();
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Pawn,Direction,Remaining=Seconds](float Delta) mutable {
  if(!Pawn.IsValid() || !Pawn->IsLocallyControlled()) return false;
  Pawn->AddMovementInput(Direction,1.f);Remaining-=Delta;return Remaining>0;
 }));
 return true;
#else
 return false;
#endif
}

#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineBuildHUD.h"

#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
FString UFoundrylineNetworkTestTools::DescribeConstruction(APlayerController* PC,FIntVector Cursor)
{
 if(!PC) return TEXT("NO PLAYER");
 for(TActorIterator<AFoundrylineBuildSite> It(PC->GetWorld());It;++It) {
  if(const auto* O=It->ObjectAt(Cursor)) return FString::Printf(TEXT("Id=%s Def=%s X=%d Y=%d Color=%.2f,%.2f,%.2f"),*O->ObjectId.ToString(),*O->DefinitionId.ToString(),O->Anchor.X,O->Anchor.Y,O->Colors.Interior.R,O->Colors.Interior.G,O->Colors.Interior.B);
 }
 return TEXT("EMPTY");
}
bool UFoundrylineNetworkTestTools::PreviewConstruction(APlayerController* PC,FIntVector Start,FIntVector End)
{
 if(!PC) return false;for(TActorIterator<AFoundrylineBuildSite> It(PC->GetWorld());It;++It) {It->PreviewRectangle(TEXT("Building"),Start,End);return !It->IsConstructionFeedbackError() && It->GetConstructionFeedbackTitle()==TEXT("READY TO PLACE");}return false;
}
bool UFoundrylineNetworkTestTools::ClientDirectExecutionBlocked(APlayerController* PC)
{
 if(!PC || PC->GetNetMode()!=NM_Client) return false;
 auto* P=PC->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto Revision=P->GetRevision();
 FFoundrylinePlacementRequest R;R.ExpectedRevision=Revision;R.DefinitionId=TEXT("Building_900_1300");R.Anchor={1000,4000,0};
 FFoundrylinePlacementContext C;C.CompanyId=FGuid(0xF001,1,1,1);C.PlantId=FGuid(0xF001,2,1,1);C.bCanConstruct=true;
 return P->Execute(R,C).Error==EFoundrylinePlacementError::NotAuthority && P->GetRevision()==Revision;
}
bool UFoundrylineNetworkTestTools::QueueConstructionTool(APlayerController* Player,FName Tool,FIntVector Start,FIntVector End,int32 Channel,FLinearColor Color)
{
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC || !PC->IsLocalController()) return false;
 TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Tool,Start,End,Channel,Color](float){
  if(!Weak.IsValid()) return false;
  auto* P=Weak->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
  for(TActorIterator<AFoundrylineBuildSite> It(Weak->GetWorld());It;++It) {
   if(Tool.ToString().StartsWith(TEXT("Key:"))){const FKey Key(FName(*Tool.ToString().RightChop(4)));for(auto Event:{IE_Pressed,IE_Released})Weak->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,Event,FPlatformTime::Cycles64()));}
   else if(Tool.ToString().StartsWith(TEXT("HUD:"))){if(auto* H=Cast<AFoundrylineBuildHUD>(Weak->GetHUD()))H->NotifyHitBoxClick(FName(*Tool.ToString().RightChop(4)));}
   else if(Tool==TEXT("PaveDown")||Tool==TEXT("PaveUp")||Tool==TEXT("PaveMove")||Tool==TEXT("PaveCancel")||Tool==TEXT("PaveEnter")||Tool==TEXT("PaveBack")){
    UFoundrylineNetworkTestTools::AimFactoryBrush(Weak.Get(),FVector(Start.X,Start.Y,20));if(Tool!=TEXT("PaveMove"))Weak->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Tool==TEXT("PaveCancel")?EKeys::RightMouseButton:Tool==TEXT("PaveEnter")?EKeys::Enter:Tool==TEXT("PaveBack")?EKeys::BackSpace:EKeys::LeftMouseButton,Tool==TEXT("PaveUp")?IE_Released:IE_Pressed,FPlatformTime::Cycles64()));
   }
   else if(Tool==TEXT("WheelClick")){Weak->SetMouseLocation(Start.X,Start.Y);for(const auto Event:{IE_Pressed,IE_Released})Weak->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::LeftMouseButton,Event,FPlatformTime::Cycles64()));}
   else if(Tool==TEXT("ChooseOfficeGlass")){Weak->ChooseTool(TEXT("OfficeGlass"));}
   else if(Tool==TEXT("SelectMode")){Weak->ChooseTool(TEXT("Select"));}
   else if(Tool==TEXT("SignEdit")){It->BeginEdit(TEXT("Select"),Start);if(const auto* O=It->GetSelectedObject()){auto Colors=O->Colors;Colors.Exterior=Color;It->SetSelectedSign(TEXT("IRON VALLEY WORKS"),float(End.X),Colors);}}
   else if(Tool==TEXT("PlannerSelect")){if(Weak->SelectPlannerItem(Start))It->SelectedObject.Invalidate();}
   else if(Tool==TEXT("SignName")){Weak->SignAction(TEXT("Name"));}
   else if(Tool==TEXT("SignClose")){Weak->FinishSignName(TEXT(""),false);}
   else if(Tool==TEXT("PlannerPlace")){Weak->PlannerKind=Channel;Weak->PlannerId=Start.X;Weak->PlannerAt=FVector(End.X,End.Y,20+Weak->OfficePlanningLevel*320);Weak->PlannerAction(TEXT("Place"));}
   else if(Tool==TEXT("PlannerStore")){Weak->PlannerKind=Channel;Weak->PlannerId=Start.X;Weak->PlannerAction(TEXT("Delete"));}
   else if(Tool==TEXT("Paint")) {It->BeginEdit(TEXT("Select"),Start);It->PaintSelected(Channel,Color);}
   else if(AFoundrylineBuildSite::IsEditTool(Tool)) {if(It->BeginEdit(Tool,Start)) It->CommitEdit(Tool,Start,End);}
   else if(Tool==TEXT("Foreign") || Tool==TEXT("Stale") || Tool==TEXT("Unknown")) {
    FFoundrylinePlacementRequest R;R.ExpectedRevision=P->GetRevision();R.Anchor=Start;R.DefinitionId=TEXT("UntrustedExternalMesh");
    if(Tool==TEXT("Foreign")) for(const auto& O:P->GetObjects()) if(O.CompanyId!=FGuid(0xF001,1,1,1)) {R.Action=EFoundrylinePlacementAction::Remove;R.ObjectId=O.ObjectId;break;}
    if(Tool==TEXT("Stale")) {R.ExpectedRevision=0;R.DefinitionId=TEXT("Building_900_1300");}
    Weak->SubmitConstruction({R});
   }
    else if(Tool==TEXT("GridTruck13")) {TGuardValue<int32> Width(It->DraftTruckWidthCm,1300);It->BuildRectangle(TEXT("TruckAccess"),Start,End);}
    else if(Tool==TEXT("OfficeStairs")||Tool==TEXT("OfficeStairsStraight")) It->PlaceOfficeStairs(Start,Channel,false,Tool==TEXT("OfficeStairsStraight")?1:0);
    else if(Tool==TEXT("OfficeFloor")) Weak->SetOfficePlanningLevel(Channel);
    else if(Tool==TEXT("RoomDoor")||Tool==TEXT("RoomWindow")) It->OfficeOpening(Tool,Start);
    else if(Tool==TEXT("OfficeArea") || Tool==TEXT("OfficeRoom")) It->OfficeRectangle(Tool,Start,End);
   else if(Tool==TEXT("Personnel") || Tool==TEXT("Garage") || Tool==TEXT("TruckDoor") || Tool==TEXT("OfficeGlass")) It->PlaceDoor(Tool,Start);
   else It->BuildRectangle(Tool,Start,End);
   break;
  }
  return false;
 }));return true;
#else
 return false;
#endif
}

#include "Pathing/FoundrylineVehiclePathSubsystem.h"
bool UFoundrylineNetworkTestTools::QueueTruckZone(APlayerController* Player,FIntVector Start,FIntVector End,bool Erase)
{
#if WITH_EDITOR
 if(!Player || !Player->IsLocalController()) return false;TWeakObjectPtr<APlayerController> Weak=Player;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Start,End,Erase](float){if(Weak.IsValid()) for(TActorIterator<AFoundrylineBuildSite> It(Weak->GetWorld());It;++It) {It->EditTruckRouting(Start,End,Erase);break;}return false;}));return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::DrivingInventory(APlayerController* Player) {if(!Player) return TEXT("NO PLAYER");auto* S=Player->GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>();return S->GetDrivingDigest();}
bool UFoundrylineNetworkTestTools::TruckPointAllowed(APlayerController* Player,FVector Point) {return Player && Player->GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap()->PointAllowed(FVector2D(Point.X,Point.Y),true,true,{});}

#include "SaveSystem/FoundrylineSaveSubsystem.h"
#include "Engine/GameInstance.h"
bool UFoundrylineNetworkTestTools::TestDrivingSave(APlayerController* Player,FString Slot,bool Load) {
#if WITH_EDITOR
 if(!Player || Player->GetNetMode()==NM_Client || !Slot.StartsWith(TEXT("DrivingQA_"))) return false;
 auto* Save=Player->GetGameInstance()->GetSubsystem<UFoundrylineSaveSubsystem>();auto* Placement=Player->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 return (Load?Save->LoadPlacement(Placement,Slot):Save->SavePlacement(Placement,Slot,false))==EFoundrylineSaveError::None;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::TestSaveBusy(APlayerController* Player) {return Player && Player->GetGameInstance()->GetSubsystem<UFoundrylineSaveSubsystem>()->IsBusy();}

bool UFoundrylineNetworkTestTools::QueueLogisticsAction(APlayerController* Player,FName Action,FVector Point,bool Click) {
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC || !PC->IsLocalController()) return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action,Point,Click](float){if(Weak.IsValid()) {Weak->LogisticsAction(Action);if(Click) Weak->SelectLogisticsPoint(Point);}return false;}));return true;
#else
 return false;
#endif
}

#include "Logistics/FoundrylineTrafficSubsystem.h"
bool UFoundrylineNetworkTestTools::ClientTrafficBlocked(APlayerController* Player) {
 if(!Player || Player->GetNetMode()!=NM_Client) return false;FString Reason;
 auto* S=Player->GetWorld()->GetSubsystem<UFoundrylineTrafficSubsystem>();const int32 Before=S->GetLedger().Active.Num();
 return !S->Enter(FGuid::NewGuid(),{},{},{},FFoundrylineVehicleProfile::Truck(),5000,Reason) && Reason==TEXT("Traffic is server-authoritative") && S->GetLedger().Active.Num()==Before;
}

#include "Logistics/FoundrylineFreightRig.h"
bool UFoundrylineNetworkTestTools::QueueFreightControl(APlayerController* Player,int32 Action,float Throttle,float Steering) {
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC || !PC->IsLocalController()) return false;
 TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action,Throttle,Steering](float){
  if(!Weak.IsValid()) return false;
  for(TActorIterator<AFoundrylineFreightRig> It(Weak->GetWorld());It;++It) {
   if(Action==0) Weak->ServerFreightDrive(*It,Throttle,Steering);else Weak->ServerFreightCommand(*It,uint8(Action));break;
  }return false;
 }));return true;
#else
 return false;
#endif
}

#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Production/FoundrylinePlasmaTable.h"
bool UFoundrylineNetworkTestTools::SetupWorkshopReview(APlayerController* Player,bool Interior){
#if WITH_EDITOR
 if(!Player||!Player->IsLocalController())return false;
 const FVector Location=Interior?FVector(4100,7200,5300):FVector(4800,-4800,7000);
 const FVector Target=Interior?FVector(200,2500,100):FVector(-300,1500,0);
 auto* Camera=Player->GetWorld()->SpawnActor<ACameraActor>(Location,(Target-Location).Rotation());if(!Camera)return false;
 Camera->GetCameraComponent()->SetFieldOfView(Interior?60:50);Player->SetViewTargetWithBlend(Camera,0);return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SetupPlasmaReview(APlayerController* Player,bool CloseUp){
#if WITH_EDITOR
 if(!Player || !Player->IsLocalController())return false;
 FVector Origin(-400,3900,20);for(TActorIterator<AFoundrylinePlasmaTable> It(Player->GetWorld());It;++It){Origin=It->GetActorLocation();break;}
 const FVector Location=Origin+(CloseUp?FVector(200,250,260):FVector(750,700,660));
 const FVector Target=Origin+(CloseUp?FVector(-20,-20,105):FVector(-30,0,75));
 auto* Camera=Player->GetWorld()->SpawnActor<ACameraActor>(Location,(Target-Location).Rotation());
 if(!Camera)return false;Camera->GetCameraComponent()->SetFieldOfView(CloseUp?48:55);Player->SetViewTargetWithBlend(Camera,0);return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SetupWeldingReview(APlayerController* Player){
#if WITH_EDITOR
 if(!Player || !Player->IsLocalController())return false;
 const FVector Location(-600,2960,200),Target(-800,3200,90);
 auto* Camera=Player->GetWorld()->SpawnActor<ACameraActor>(Location,(Target-Location).Rotation());
 if(!Camera)return false;Camera->GetCameraComponent()->SetFieldOfView(50);Player->SetViewTargetWithBlend(Camera,0);return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueueMaterialControl(APlayerController* Player,int32 Action,float Throttle,float Steering,float Lift){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC || !PC->IsLocalController())return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action,Throttle,Steering,Lift](float){if(!Weak.IsValid())return false;for(TActorIterator<AFoundrylineMaterialHandlingCell> It(Weak->GetWorld());It;++It){if(Action==0)Weak->ServerMaterialDrive(*It,Throttle,Steering,Lift);else Weak->ServerMaterialCommand(*It,uint8(Action));break;}return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SetupMaterialFixture(APlayerController* Host,int32 Scenario){
#if WITH_EDITOR
 if(!Host || !Host->HasAuthority() || Host->GetNetMode()==NM_Client)return false;
 for(TActorIterator<AFoundrylineMaterialHandlingCell> It(Host->GetWorld());It;++It){
  It->Pose.Speed=0;It->Pose.Steering=0;It->Pose.Tilt=0;
  if(Scenario==0){It->Pose.Position={86,0,0};It->Pose.Yaw=180;}
  if(Scenario==1){It->Pose.Position={-414,400,0};It->Pose.Yaw=180;}
  if(Scenario==2){It->Pose.Position={-66,-550,0};It->Pose.Yaw=0;}
  It->ForceNetUpdate();return true;
 }
#endif
 return false;
}
