#include "World/FoundrylineGameMode.h"
#include "World/FoundrylineTreeCollision.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "World/FoundrylineCameraPawn.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineBuildHUD.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Production/FoundrylineWeldingStation.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Inventory/FoundrylinePartsArea.h"
#include "Inventory/FoundrylineRawSteelReceivingArea.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AFoundrylineGameMode::AFoundrylineGameMode()
{
	DefaultPawnClass = AFoundrylineCameraPawn::StaticClass();
	PlayerControllerClass = AFoundrylinePlayerController::StaticClass();
	HUDClass = AFoundrylineBuildHUD::StaticClass();
}

void AFoundrylineGameMode::BeginPlay()
{
	Super::BeginPlay();
 GetWorld()->SpawnActor<AFoundrylineTreeCollision>();
 auto* Businesses=AFoundrylineBusinessRegistry::Find(GetWorld());if(!Businesses)Businesses=GetWorld()->SpawnActor<AFoundrylineBusinessRegistry>();
	auto* Site=GetWorld()->SpawnActorDeferred<AFoundrylineBuildSite>(AFoundrylineBuildSite::StaticClass(),FTransform::Identity);
	if(Site) { Site->bCutPartsStarter=bCutPartsStarter;Site->bStarterProperty=!FParse::Param(FCommandLine::Get(),TEXT("FoundrylineShowcase"));Site->bLargeShowcase=!Site->bStarterProperty;Site->bProductionWorkshop=Site->bStarterProperty&&!FParse::Param(FCommandLine::Get(),TEXT("FoundrylineLegacyStarter"));Site->FinishSpawning(FTransform::Identity); }
 if(Site && Site->bStarterProperty) {
  const bool New=Site->bProductionWorkshop;
  if(bCutPartsStarter){
   for(int I=1;I<=2;++I){const FTransform At(FRotator::ZeroRotator,I==1?FVector(-2940,540,22):FVector(220,1300,20));auto* Terminal=GetWorld()->SpawnActorDeferred<AFoundrylineOperationsTerminal>(AFoundrylineOperationsTerminal::StaticClass(),At);if(Terminal){Terminal->Terminal=I;Terminal->FinishSpawning(At);}}
   const FTransform T(FRotator::ZeroRotator,AFoundrylineFactoryPlant::MachinePosition(1));
   auto* Plasma=GetWorld()->SpawnActorDeferred<AFoundrylinePlasmaTable>(AFoundrylinePlasmaTable::StaticClass(),T);if(Plasma){Plasma->bFactoryAutomation=true;Plasma->FinishSpawning(T);}
   auto* Plant=GetWorld()->SpawnActorDeferred<AFoundrylineFactoryPlant>(AFoundrylineFactoryPlant::StaticClass(),FTransform::Identity);if(Plant){Plant->bCutPartsStarter=true;Plant->FinishSpawning(FTransform::Identity);}return;
  }
  const FTransform RigTransform(FRotator::ZeroRotator,New?FVector(2950,1650,20):FVector(2200,2200,20));
  auto* Rig=GetWorld()->SpawnActorDeferred<AFoundrylineFreightRig>(AFoundrylineFreightRig::StaticClass(),RigTransform);
  if(Rig){if(New){Rig->bTractorOnly=true;Rig->State.Tractor=FVector::ZeroVector;}Rig->FinishSpawning(RigTransform);}
  const FTransform CellTransform(FRotator::ZeroRotator,New?AFoundrylineFactoryPlant::MachinePosition(3)-FVector(300,-550,0):FVector(-1900,2600,20));
  auto* Cell=GetWorld()->SpawnActorDeferred<AFoundrylineMaterialHandlingCell>(AFoundrylineMaterialHandlingCell::StaticClass(),CellTransform);
  if(Cell){if(New){Cell->bFactoryAutomation=true;Cell->RackOffsetCm=FVector(2000,-3200,0);Cell->ArrivalPositionCm=FVector(700,-2800,0);}Cell->FinishSpawning(CellTransform);}
  if(!New)GetWorld()->SpawnActor<AFoundrylineWeldingStation>(FVector(-900,3200,20),FRotator::ZeroRotator);
  const FTransform PlasmaT(FRotator::ZeroRotator,New?AFoundrylineFactoryPlant::MachinePosition(1):FVector(-400,3900,20));auto* Plasma=GetWorld()->SpawnActorDeferred<AFoundrylinePlasmaTable>(AFoundrylinePlasmaTable::StaticClass(),PlasmaT);if(Plasma){Plasma->bFactoryAutomation=New;Plasma->FinishSpawning(PlasmaT);}
  GetWorld()->SpawnActor<AFoundrylinePressBrakeDisplay>(New?AFoundrylineFactoryPlant::MachinePosition(2):FVector(-1300,4200,20),FRotator::ZeroRotator);
  if(New){GetWorld()->SpawnActor<AFoundrylineFactoryPlant>();}
 }
}

void AFoundrylineGameMode::PostLogin(APlayerController* Player){Super::PostLogin(Player);auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());if(!R)R=GetWorld()->SpawnActor<AFoundrylineBusinessRegistry>();if(R)R->RegisterPlayer(Player);}
void AFoundrylineGameMode::Logout(AController* Player){Super::Logout(Player);if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld()))R->RefreshDirectory();}
