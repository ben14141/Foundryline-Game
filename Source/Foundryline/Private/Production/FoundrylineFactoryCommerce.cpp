#include "Production/FoundrylineFactoryPlant.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Networking/FoundrylineOperationsState.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Logistics/FoundrylineCarrierSceneActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
void AFoundrylineFactoryPlant::PublishCommerce(){if(HasAuthority()&&OperationsState){OperationsState->Data=Snapshot.Commerce;OperationsState->ForceNetUpdate();}}
FVector AFoundrylineFactoryPlant::MachineWorldPosition(int32 Id)const{const auto* M=Snapshot.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});return GetActorTransform().TransformPosition(M&&!M->Origin.IsNearlyZero()?M->Origin:MachinePosition(Id));}
FVector AFoundrylineFactoryPlant::ScreenWorldPosition(int32 Id)const{const auto* M=Snapshot.Machines.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!M)return MachineWorldPosition(Id);const FVector Offset=M->Kind()==1?FVector(269,197,148):M->Kind()==2?FVector(180,215,152):FVector(115,277.6,152);return GetActorTransform().TransformPosition(M->Origin+FRotator(0,M->QuarterTurns*90,0).RotateVector(Offset));}
bool AFoundrylineFactoryPlant::ExecuteCommerce(APlayerController* P,const FFoundrylineCommerceCommand& C,FString& E){
 auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());if(!HasAuthority()||!P||P->GetWorld()!=GetWorld()||!P->PlayerState||P->PlayerState->IsOnlyASpectator()||!GetWorld()->GetGameState()||!GetWorld()->GetGameState()->PlayerArray.Contains(P->PlayerState)||C.PlantId!=BusinessPlantId||!R||!R->CanOperate(P,BusinessCompanyId,BusinessPlantId)){E=TEXT("Join this company before changing its operations");return false;}
 const auto* Company=R->GetLedger().Company(BusinessCompanyId);if(!Company){E=TEXT("Company account unavailable");return false;}const int64 Cash=Company->BalanceCents;auto Candidate=Model;Candidate.State.Commerce.CashCents=Cash;if(!Candidate.CommerceCommand(C,E)||!Candidate.Valid(E))return false;
 if(!R->CommitPlantCash(BusinessCompanyId,BusinessPlantId,Cash,Candidate.State.Commerce.CashCents)){E=TEXT("Company balance changed; refresh and try again");return false;}Model=MoveTemp(Candidate);RefreshSnapshot();bRebuild=true;ForceNetUpdate();return true;
}
void AFoundrylineFactoryPlant::EndPlay(const EEndPlayReason::Type Reason){for(auto& Pair:OfficeDeskActors)if(IsValid(Pair.Value))Pair.Value->Destroy();OfficeDeskActors.Reset();Super::EndPlay(Reason);}
void AFoundrylineFactoryPlant::EnsureCommerceEquipment(){
 EnsureMachineActors();
 UpdateOfficeDecorations();
 if(!HasAuthority())Model.Solids=FFoundrylineWorkshopLayout::PlantSolids(Snapshot);
 if(!HasAuthority()||!Model.State.Commerce.Enabled)return;
 for(auto It=OfficeDeskActors.CreateIterator();It;++It)if(!Model.State.OfficeDesks.ContainsByPredicate([&](const auto& D){return D.Id==It.Key();})){if(IsValid(It.Value()))It.Value()->Destroy();It.RemoveCurrent();}
 for(const auto& D:Model.State.OfficeDesks){const FTransform At(GetActorQuat()*FRotator(0,D.QuarterTurns*90,0).Quaternion(),GetActorTransform().TransformPosition(D.Center));auto& Desk=OfficeDeskActors.FindOrAdd(D.Id);if(!IsValid(Desk)){Desk=GetWorld()->SpawnActorDeferred<AFoundrylineOperationsTerminal>(AFoundrylineOperationsTerminal::StaticClass(),At,this);if(Desk){Desk->Terminal=1;Desk->PlanningDeskId=D.Id;Desk->CompanyId=BusinessCompanyId;Desk->PlantId=BusinessPlantId;Desk->FinishSpawning(At);}}else if(!Desk->GetActorTransform().Equals(At,.01)){Desk->SetActorTransform(At);Desk->ForceNetUpdate();}}
 int Index=0;
 for(const auto& B:Model.State.Commerce.Bookings){if(B.Status==EFoundrylineBookingState::Booked||B.Status==EFoundrylineBookingState::Delivered||B.Status==EFoundrylineBookingState::InTransit||Index>=8)continue;if(!CarrierVisuals.IsValidIndex(Index)){auto* V=GetWorld()->SpawnActor<AFoundrylineCarrierSceneActor>();V->SetOwner(this);CarrierVisuals.Add(V);}auto* V=CarrierVisuals[Index++].Get();if(!V)continue;V->SetActorHiddenInGame(false);V->SetActorEnableCollision(true);V->SetBooking(B,Model.State.Commerce,GetActorTransform(),Model.State.Pallets);}
 for(int I=Index;I<CarrierVisuals.Num();++I)if(CarrierVisuals[I]){CarrierVisuals[I]->SetActorHiddenInGame(true);CarrierVisuals[I]->SetActorEnableCollision(false);}
}
