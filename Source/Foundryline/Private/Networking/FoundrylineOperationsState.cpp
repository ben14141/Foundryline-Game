#include "Networking/FoundrylineOperationsState.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
AFoundrylineOperationsState::AFoundrylineOperationsState(){bReplicates=true;SetReplicateMovement(false);SetNetUpdateFrequency(4);PrimaryActorTick.bCanEverTick=true;}
void AFoundrylineOperationsState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineOperationsState,CompanyId);DOREPLIFETIME(AFoundrylineOperationsState,PlantId);DOREPLIFETIME(AFoundrylineOperationsState,Data);}
bool AFoundrylineOperationsState::IsNetRelevantFor(const AActor* Viewer,const AActor*,const FVector&)const{const auto* PC=Cast<APlayerController>(Viewer);const auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());return Registry&&Registry->CanOperate(PC,CompanyId,PlantId);}
void AFoundrylineOperationsState::Tick(float Dt){Super::Tick(Dt);if(HasAuthority())return;for(TActorIterator<AFoundrylineFactoryPlant> It(GetWorld());It;++It)if(It->BusinessCompanyId==CompanyId&&It->BusinessPlantId==PlantId){It->Snapshot.Commerce=Data;break;}}
