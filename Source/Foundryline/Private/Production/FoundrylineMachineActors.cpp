#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void AFoundrylineFactoryPlant::EnsureMachineActors(){
 for(const auto& M:Snapshot.Machines){
  auto Match=[&](const auto* A){return A&&A->MachineId==M.Id&&A->BusinessCompanyId==BusinessCompanyId&&A->BusinessPlantId==BusinessPlantId;};
  auto& Actor=MachineActors.FindOrAdd(M.Id);
  if(!IsValid(Actor)){
   if(M.Kind()==1){for(TActorIterator<AFoundrylinePlasmaTable> It(GetWorld());It;++It)if(Match(*It)){Actor=*It;break;}}
   else if(M.Kind()==2){for(TActorIterator<AFoundrylinePressBrakeDisplay> It(GetWorld());It;++It)if(Match(*It)){Actor=*It;break;}}
   else if(M.Kind()==3){for(TActorIterator<AFoundrylineMaterialHandlingCell> It(GetWorld());It;++It)if(Match(*It)){Actor=*It;break;}}
  }
  const FRotator Turn(0,M.QuarterTurns*90,0);const FVector Local=M.Origin-(M.Kind()==3?Turn.RotateVector({300,550,0}):FVector::ZeroVector);
  const FTransform At(GetActorQuat()*Turn.Quaternion(),GetActorTransform().TransformPosition(Local));
  if(!IsValid(Actor)&&HasAuthority()){
   if(M.Kind()==1){auto* A=GetWorld()->SpawnActorDeferred<AFoundrylinePlasmaTable>(AFoundrylinePlasmaTable::StaticClass(),At);if(A){A->MachineId=M.Id;A->BusinessCompanyId=BusinessCompanyId;A->BusinessPlantId=BusinessPlantId;A->bFactoryAutomation=true;A->FinishSpawning(At);Actor=A;}}
   else if(M.Kind()==2){auto* A=GetWorld()->SpawnActorDeferred<AFoundrylinePressBrakeDisplay>(AFoundrylinePressBrakeDisplay::StaticClass(),At);if(A){A->MachineId=M.Id;A->BusinessCompanyId=BusinessCompanyId;A->BusinessPlantId=BusinessPlantId;A->FinishSpawning(At);Actor=A;}}
   else if(M.Kind()==3){auto* A=GetWorld()->SpawnActorDeferred<AFoundrylineMaterialHandlingCell>(AFoundrylineMaterialHandlingCell::StaticClass(),At);if(A){A->MachineId=M.Id;A->BusinessCompanyId=BusinessCompanyId;A->BusinessPlantId=BusinessPlantId;A->bFactoryAutomation=true;A->FinishSpawning(At);Actor=A;}}
  }
  if(Actor){Actor->SetActorTransform(At);Actor->SetActorHiddenInGame(M.bStored);Actor->SetActorEnableCollision(!M.bStored);}
 }
}
