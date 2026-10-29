#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "World/FoundrylineCameraPawn.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
void AFoundrylinePlayerController::ToggleFreightDrive() {
 if(!IsLocalController() || IsWalking()) return;
 if(LocalMaterialCell) ToggleMaterialHandling();
 if(LocalFreightRig) {ServerFreightCommand(LocalFreightRig,11);LocalFreightRig=nullptr;if(GetPawn()) GetPawn()->SetActorTickEnabled(true);}
 else for(TActorIterator<AFoundrylineFreightRig> It(GetWorld());It;++It) {LocalFreightRig=*It;if(GetPawn())GetPawn()->AddTickPrerequisiteActor(*It);ChooseTool(NAME_None);ServerFreightCommand(*It,10);break;}
 FreightForward=FreightReverse=FreightLeft=FreightRight=false;
}
bool AFoundrylinePlayerController::HandleFreightInput(const FInputKeyEventArgs& P) {
 if(!IsLocalController()) return false;
 if(P.Key==EKeys::V) {if(P.Event==IE_Pressed) ToggleFreightDrive();return true;}
 if(!LocalFreightRig) return false;
 const bool Down=P.Event!=IE_Released;
 if(P.Key==EKeys::W) FreightForward=Down;else if(P.Key==EKeys::S) FreightReverse=Down;
 else if(P.Key==EKeys::A) FreightLeft=Down;else if(P.Key==EKeys::D) FreightRight=Down;
 else if(P.Key==EKeys::C || P.Key==EKeys::G || P.Key==EKeys::U) {if(P.Event==IE_Pressed) ServerFreightCommand(LocalFreightRig,P.Key==EKeys::C?1:P.Key==EKeys::G?2:3);}
 else return false;
 return true;
}
void AFoundrylinePlayerController::TickFreightDrive(float Dt) {
 if(!IsValid(LocalFreightRig)) {LocalFreightRig=nullptr;if(GetPawn())GetPawn()->SetActorTickEnabled(true);return;}
 FreightInputClock+=Dt;if(FreightInputClock>=.1f) {FreightInputClock=0;ServerFreightDrive(LocalFreightRig,float(FreightForward)-float(FreightReverse),float(FreightRight)-float(FreightLeft));}
 // Camera follows the interpolated vehicle after its visual tick.
}
void AFoundrylinePlayerController::ServerFreightCommand_Implementation(AFoundrylineFreightRig* Rig,uint8 Action) {
 if(!IsValid(Rig) || Rig->GetWorld()!=GetWorld() || IsWalking()) return;
 if(Action==10 || Action==11) Rig->Claim(this,Action==10);else Rig->Command(this,Action);
}
void AFoundrylinePlayerController::ServerFreightDrive_Implementation(AFoundrylineFreightRig* Rig,float Throttle,float Steering) {
 if(IsValid(Rig) && Rig->GetWorld()==GetWorld() && !IsWalking()) Rig->Drive(this,Throttle,Steering);
}
