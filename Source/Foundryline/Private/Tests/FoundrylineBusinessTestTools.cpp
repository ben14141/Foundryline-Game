#include "FoundrylineNetworkTestTools.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
FString UFoundrylineNetworkTestTools::ScopedPlantDigest(APlayerController* P){if(!P)return {};TArray<FString> Values;for(TActorIterator<AFoundrylineFactoryPlant> It(P->GetWorld());It;++It)Values.Add(It->BusinessCompanyId.ToString()+It->GetLedgerDigest());Values.Sort();return FString::Join(Values,TEXT("/"));}
FString UFoundrylineNetworkTestTools::ExerciseBusinessFoundation(APlayerController* Host,int32 Phase){
 auto J=MakeShared<FJsonObject>();
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||Host->GetWorld()->WorldType!=EWorldType::PIE)return TEXT("{}");auto* W=Host->GetWorld();auto* R=AFoundrylineBusinessRegistry::Find(W);if(!R)return TEXT("{}");TArray<APlayerController*> Guests;for(TActorIterator<APlayerController> It(W);It;++It)if(*It!=Host&&It->PlayerState)Guests.Add(*It);Guests.Sort([](const auto& A,const auto& B){return A.PlayerState->GetPlayerId()<B.PlayerState->GetPlayerId();});if(Guests.Num()<3)return TEXT("{}");auto* A=Guests[0];auto* B=Guests[1];auto* G=Guests[2];FString E;
 if(Phase==0){J->SetBoolField(TEXT("world_bound_after_site_init"),R->IsWorldBound());J->SetBoolField(TEXT("four_registered_members"),R->GetLedger().State.Members.Num()==4);
  J->SetBoolField(TEXT("default_coop_access"),R->CanOperate(Host,R->GetLedger().StarterCompany(),R->GetLedger().StarterPlant())&&R->CompanyFor(A)==R->CompanyFor(Host));
  const auto PlotA=R->GetLedger().State.Plots[1].Id,PlotB=R->GetLedger().State.Plots[2].Id;
  J->SetBoolField(TEXT("two_independent_businesses"),R->CreateBusiness(A,TEXT("Street Works A"),PlotA,E)&&R->CreateBusiness(B,TEXT("Street Works B"),PlotB,E));
  J->SetStringField(TEXT("create_error"),E);const auto CA=R->CompanyFor(A),CB=R->CompanyFor(B);J->SetBoolField(TEXT("plot_claim_is_exclusive"),!R->CreateBusiness(G,TEXT("Steal"),PlotA,E));
  J->SetBoolField(TEXT("uninvited_join_rejected"),!R->Accept(G,CA,E));J->SetBoolField(TEXT("owner_invite_and_accept"),R->Invite(A,G,E)&&R->Accept(G,CA,E)&&R->CompanyFor(G)==CA);
  AFoundrylineFactoryPlant* Starter=nullptr;AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineFactoryPlant> It(W);It;++It)if(It->BusinessCompanyId==R->GetLedger().StarterCompany())Starter=*It;for(TActorIterator<AFoundrylineBuildSite> It(W);It;++It)Site=*It;
  if(!Starter||!Site)return TEXT("{}");FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::ReceiveSteel;C.Count=1;const auto Before=Starter->GetLedgerDigest();J->SetBoolField(TEXT("foreign_inventory_command_denied"),!Starter->Execute(A,C,E)&&Before==Starter->GetLedgerDigest());
  auto* Placement=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto* Shop=Placement->GetObjects().FindByPredicate([&](const auto& O){const auto* D=Placement->FindDefinition(O.DefinitionId);return O.CompanyId==R->GetLedger().StarterCompany()&&D&&D->FunctionId==TEXT("BuildingShell")&&!O.bOfficeAnnex;});
  if(Shop){const auto Original=*Shop;FFoundrylinePlacementRequest Q;Q.Action=EFoundrylinePlacementAction::SetBuildingColors;Q.ObjectId=Original.ObjectId;Q.ExpectedRevision=Placement->GetRevision();J->SetBoolField(TEXT("foreign_construction_denied"),Site->ApplyRemoteConstruction(A,{Q}).Error==EFoundrylinePlacementError::AccessDenied);
   Q={};Q.Action=EFoundrylinePlacementAction::Build;Q.DefinitionId=Original.DefinitionId;Q.Anchor=Original.Anchor+FIntVector(12000,0,0);Q.ExpectedRevision=Placement->GetRevision();const auto Built=Site->ApplyRemoteConstruction(A,{Q});J->SetBoolField(TEXT("own_plot_construction_allowed"),Built.IsSuccess());if(Built.IsSuccess()){Q.Action=EFoundrylinePlacementAction::SetBuildingColors;Q.ObjectId=Built.Actions[0].ObjectId;Q.ExpectedRevision=Placement->GetRevision();J->SetBoolField(TEXT("neighbour_cannot_edit_new_building"),Site->ApplyRemoteConstruction(B,{Q}).Error==EFoundrylinePlacementError::AccessDenied);}}
  for(int I=0;I<2;++I){const auto Id=I?CB:CA;const auto* Company=R->GetLedger().Company(Id);const FTransform T(FRotator::ZeroRotator,FVector(I?-12000:12000,0,0));auto* P=W->SpawnActorDeferred<AFoundrylineFactoryPlant>(AFoundrylineFactoryPlant::StaticClass(),T);P->bCutPartsStarter=true;P->BusinessCompanyId=Id;P->BusinessPlantId=Company->PlantId;P->FinishSpawning(T);P->Model.State.Machines[0].BatchRemaining=0;P->Tick(0);}
  const int64 BaseA=R->GetLedger().Company(CA)->BalanceCents,BaseB=R->GetLedger().Company(CB)->BalanceCents,BaseStarter=R->GetLedger().Company(R->GetLedger().StarterCompany())->BalanceCents;J->SetBoolField(TEXT("separate_cash_balances"),R->AdjustBalance(CA,10000,E)&&R->AdjustBalance(CB,20000,E)&&R->GetLedger().Company(CA)->BalanceCents==BaseA+10000&&R->GetLedger().Company(CB)->BalanceCents==BaseB+20000&&R->GetLedger().Company(R->GetLedger().StarterCompany())->BalanceCents==BaseStarter);
  auto* Rig=W->SpawnActor<AFoundrylineFreightRig>(FVector(20000,5000,20),FRotator::ZeroRotator);J->SetBoolField(TEXT("foreign_vehicle_control_denied"),Rig&&!Rig->Claim(A,true)&&Rig->Claim(Host,true));if(Rig)Rig->Claim(Host,false);
  J->SetBoolField(TEXT("legacy_world_load_guarded"),!Starter->LoadFactory(E));
 }else{
  const auto Id=R->CompanyFor(A);J->SetBoolField(TEXT("member_removal_revokes_access"),R->RemoveMember(A,G,E)&&!R->CompanyFor(G).IsValid()&&!R->CanOperate(G,Id,R->GetLedger().Company(Id)->PlantId));J->SetBoolField(TEXT("revoked_member_not_auto_reenrolled"),!R->RegisterPlayer(G));J->SetBoolField(TEXT("explicit_reinvite_restores_access"),R->Invite(A,G,E)&&R->Accept(G,Id,E)&&R->CanOperate(G,Id,R->GetLedger().Company(Id)->PlantId));
 }
 J->SetBoolField(TEXT("ledger_valid"),R->GetLedger().Valid(E));
#endif
 FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);return Text;
}

FString UFoundrylineNetworkTestTools::OwnBusinessReport(APlayerController* P){auto J=MakeShared<FJsonObject>();if(P){if(auto* R=AFoundrylineBusinessRegistry::Find(P->GetWorld())){J->SetStringField(TEXT("company"),R->CompanyFor(P).ToString());J->SetBoolField(TEXT("has_company"),R->CompanyFor(P).IsValid());for(TActorIterator<AFoundrylineFactoryPlant> It(P->GetWorld());It;++It)if(R->CanOperate(P,It->BusinessCompanyId,It->BusinessPlantId)){J->SetBoolField(TEXT("running"),It->Snapshot.Running);J->SetStringField(TEXT("plant_digest"),It->GetLedgerDigest());J->SetStringField(TEXT("plant"),It->BusinessPlantId.ToString());if(It->Snapshot.Machines.Num())J->SetNumberField(TEXT("remaining"),It->Snapshot.Machines[0].Remaining);}}}FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);return Text;}
