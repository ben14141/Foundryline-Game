#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "World/FoundrylineCameraPawn.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
void AFoundrylinePlayerController::ToggleMaterialHandling(){
 if(!IsLocalController() || IsWalking())return;
 if(LocalFreightRig)ToggleFreightDrive();
 if(LocalMaterialCell){ServerMaterialCommand(LocalMaterialCell,11);LocalMaterialCell=nullptr;}
 else for(TActorIterator<AFoundrylineMaterialHandlingCell> It(GetWorld());It;++It){LocalMaterialCell=*It;ChooseTool(NAME_None);if(GetPawn())GetPawn()->AddTickPrerequisiteActor(*It);ServerMaterialCommand(*It,10);break;}
 MaterialForward=MaterialReverse=MaterialLeft=MaterialRight=MaterialUp=MaterialDown=false;
}
bool AFoundrylinePlayerController::HandleMaterialInput(const FInputKeyEventArgs& P){
 if(!IsLocalController())return false;
 if(P.Key==EKeys::K){if(P.Event==IE_Pressed)ToggleMaterialHandling();return true;}
 if(!LocalMaterialCell)return false;const bool Down=P.Event!=IE_Released;
 if(P.Key==EKeys::W)MaterialForward=Down;else if(P.Key==EKeys::S)MaterialReverse=Down;else if(P.Key==EKeys::A)MaterialLeft=Down;else if(P.Key==EKeys::D)MaterialRight=Down;else if(P.Key==EKeys::Q)MaterialDown=Down;else if(P.Key==EKeys::E)MaterialUp=Down;
 else if(P.Key==EKeys::SpaceBar || P.Key==EKeys::One || P.Key==EKeys::Two || P.Key==EKeys::Three || P.Key==EKeys::Four || P.Key==EKeys::R){if(P.Event==IE_Pressed)ServerMaterialCommand(LocalMaterialCell,P.Key==EKeys::SpaceBar?1:P.Key==EKeys::One?2:P.Key==EKeys::Two?3:P.Key==EKeys::Three?4:P.Key==EKeys::Four?5:6);}
 else return false;return true;
}
void AFoundrylinePlayerController::TickMaterialHandling(float Dt){
 if(!IsValid(LocalMaterialCell)){LocalMaterialCell=nullptr;return;}
 if(!LocalMaterialCell->bFactoryAutomation&&!IsValid(LocalMaterialCell->Driver)){MaterialClaimClock+=Dt;if(MaterialClaimClock>=1){MaterialClaimClock=0;ServerMaterialCommand(LocalMaterialCell,10);}}else MaterialClaimClock=0;
 MaterialInputClock+=Dt;if(MaterialInputClock>=.1f){MaterialInputClock=0;ServerMaterialDrive(LocalMaterialCell,float(MaterialForward)-float(MaterialReverse),float(MaterialRight)-float(MaterialLeft),float(MaterialUp)-float(MaterialDown));}
}
void AFoundrylinePlayerController::ServerMaterialCommand_Implementation(AFoundrylineMaterialHandlingCell* Cell,uint8 Action){
 if(!IsValid(Cell) || Cell->GetWorld()!=GetWorld() || IsWalking())return;
 if(Action==10 || Action==11)Cell->Claim(this,Action==10);else Cell->Command(this,Action);
}
void AFoundrylinePlayerController::ServerMaterialDrive_Implementation(AFoundrylineMaterialHandlingCell* Cell,float T,float S,float L){if(IsValid(Cell) && Cell->GetWorld()==GetWorld() && !IsWalking())Cell->Drive(this,T,S,L);}
