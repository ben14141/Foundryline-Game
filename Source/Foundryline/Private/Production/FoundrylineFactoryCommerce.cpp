#include "Production/FoundrylineFactoryPlant.h"
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
FVector AFoundrylineFactoryPlant::ScreenWorldPosition(int32 Id)const{return MachineWorldPosition(Id)+(Id==1?FVector(269,197,148):Id==2?FVector(180,215,152):FVector(115,157.6,152));}
bool AFoundrylineFactoryPlant::ExecuteCommerce(APlayerController* P,const FFoundrylineCommerceCommand& C,FString& E){
 auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());if(!HasAuthority()||!P||P->GetWorld()!=GetWorld()||!P->PlayerState||P->PlayerState->IsOnlyASpectator()||!GetWorld()->GetGameState()||!GetWorld()->GetGameState()->PlayerArray.Contains(P->PlayerState)||C.PlantId!=BusinessPlantId||!R||!R->CanOperate(P,BusinessCompanyId,BusinessPlantId)){E=TEXT("Join this company before changing its operations");return false;}
 const auto* Company=R->GetLedger().Company(BusinessCompanyId);if(!Company){E=TEXT("Company account unavailable");return false;}const int64 Cash=Company->BalanceCents;auto Candidate=Model;Candidate.State.Commerce.CashCents=Cash;if(!Candidate.CommerceCommand(C,E)||!Candidate.Valid(E))return false;
 if(!R->CommitPlantCash(BusinessCompanyId,BusinessPlantId,Cash,Candidate.State.Commerce.CashCents)){E=TEXT("Company balance changed; refresh and try again");return false;}Model=MoveTemp(Candidate);RefreshSnapshot();bRebuild=true;ForceNetUpdate();return true;
}
void AFoundrylineFactoryPlant::EnsureCommerceEquipment(){
 const bool Installed=bCutPartsStarter&&(HasAuthority()?Model.State.Commerce.BrakeOwned:Snapshot.Machines.ContainsByPredicate([](const auto& M){return M.Id==2;}));
 if(!HasAuthority()&&bCutPartsStarter){Model.Solids=FFoundrylineWorkshopLayout::CutShopSolids();if(Installed){Model.Solids.Emplace(FVector2D(-2025,1675),FVector2D(-1675,2025));Model.Solids.Emplace(FVector2D(-1560,1642),FVector2D(-1440,2058));}}
 if(!Installed&&bCutPartsStarter){if(HasAuthority()&&IsValid(Brake)){Brake->Destroy();Brake=nullptr;}for(auto C:CommerceRackComponents)if(C)C->DestroyComponent();CommerceRackComponents.Reset();bCommerceRackBuilt=false;}
 if(Installed&&HasAuthority()&&!IsValid(Brake)){const auto* M=Model.State.Machines.FindByPredicate([](const auto& V){return V.Id==2;});if(M){const FTransform T(GetActorRotation(),GetActorTransform().TransformPosition(M->Origin));auto* New=GetWorld()->SpawnActorDeferred<AFoundrylinePressBrakeDisplay>(AFoundrylinePressBrakeDisplay::StaticClass(),T);if(New){New->BusinessCompanyId=BusinessCompanyId;New->BusinessPlantId=BusinessPlantId;New->FinishSpawning(T);Brake=New;}}}
 if(Installed&&!bCommerceRackBuilt){bCommerceRackBuilt=true;auto Add=[&](const TCHAR* N,FVector At){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(FString(TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_"))+N)));C->SetRelativeLocation(At);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);CommerceRackComponents.Add(C);};if(GetNetMode()!=NM_DedicatedServer){for(int Side:{-1,1})Add(TEXT("Rack_Upright"),{-1500,1850+Side*201.735,20});for(int I=0;I<3;++I)Add(TEXT("Rack_Shelf400"),{-1500,1850,40.+I*130});}auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetRelativeLocation({-1500,1850,230});Box->SetBoxExtent({60,208,210});Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->RegisterComponent();AddInstanceComponent(Box);CommerceRackComponents.Add(Box);}
 if(!HasAuthority()||!Model.State.Commerce.Enabled)return;int Index=0;
 for(const auto& B:Model.State.Commerce.Bookings){if(B.Status==EFoundrylineBookingState::Booked||B.Status==EFoundrylineBookingState::Delivered||B.Status==EFoundrylineBookingState::InTransit||Index>=8)continue;if(!CarrierVisuals.IsValidIndex(Index)){auto* V=GetWorld()->SpawnActor<AFoundrylineCarrierSceneActor>();V->SetOwner(this);CarrierVisuals.Add(V);}auto* V=CarrierVisuals[Index++].Get();if(!V)continue;V->SetActorHiddenInGame(false);V->SetActorEnableCollision(true);V->SetBooking(B,Model.State.Commerce,GetActorTransform(),Model.State.Pallets);}
 for(int I=Index;I<CarrierVisuals.Num();++I)if(CarrierVisuals[I]){CarrierVisuals[I]->SetActorHiddenInGame(true);CarrierVisuals[I]->SetActorEnableCollision(false);}
}
