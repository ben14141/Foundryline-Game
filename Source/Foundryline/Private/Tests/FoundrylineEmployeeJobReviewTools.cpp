#include "FoundrylineNetworkTestTools.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UFoundrylineNetworkTestTools::SetupEmployeeJobOffice(APlayerController* Player){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->HasAuthority()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;
 auto* Plant=PC->FactoryPlant();if(!Plant)return false;
 for(TActorIterator<AFoundrylineOperationsTerminal> It(PC->GetWorld());It;++It)if(It->Terminal==1&&It->PlantId==Plant->BusinessPlantId&&It->CompanyId==Plant->BusinessCompanyId)return true;
 const FTransform At(FRotator::ZeroRotator,Plant->GetActorTransform().TransformPosition(FVector(-3420,2750,22)));
 auto* Terminal=PC->GetWorld()->SpawnActorDeferred<AFoundrylineOperationsTerminal>(AFoundrylineOperationsTerminal::StaticClass(),At);if(!Terminal)return false;Terminal->Terminal=1;Terminal->CompanyId=Plant->BusinessCompanyId;Terminal->PlantId=Plant->BusinessPlantId;Terminal->FinishSpawning(At);return true;
#else
 return false;
#endif
}
