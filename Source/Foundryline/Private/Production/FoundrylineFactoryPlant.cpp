#include "Production/FoundrylineFactoryPlant.h"
#include "UI/FoundrylinePlayerController.h"
#include "Networking/FoundrylineOperationsState.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/Crc.h"
#include "Components/BoxComponent.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"

AFoundrylineFactoryPlant::AFoundrylineFactoryPlant(){bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);SetNetUpdateFrequency(2);PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Plant")));}
void AFoundrylineFactoryPlant::BeginPlay(){Super::BeginPlay();if(HasAuthority()){if(GetWorld()->GetMapName().Contains(TEXT("L_BasicShopStarter"))){Model.State=FFoundrylineFactoryState();Model.State.bCutPartsStarter=true;Model.SeedCommerce();Model.State.Lanes.Reset();Model.State.FloorMin={-2370,30};Model.State.FloorMax={-30,1770};Model.Solids.Reset();Model.State.Status=TEXT("Use the office computer to plan and equip your shop");}else if(bCutPartsStarter)Model.SeedCutPartsShop();else Model.Seed(true);Model.State.CompanyId=BusinessCompanyId;Model.State.PlantId=BusinessPlantId;if(Model.State.Commerce.Enabled){if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld()))Model.State.Commerce.CashCents=R->InitialiseEconomy(BusinessCompanyId,BusinessPlantId,Model.State.Commerce.CashCents);OperationsState=GetWorld()->SpawnActor<AFoundrylineOperationsState>();if(OperationsState){OperationsState->CompanyId=BusinessCompanyId;OperationsState->PlantId=BusinessPlantId;}}RefreshSnapshot();}SetupVisuals();}
void AFoundrylineFactoryPlant::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineFactoryPlant,bCutPartsStarter);DOREPLIFETIME(AFoundrylineFactoryPlant,BusinessCompanyId);DOREPLIFETIME(AFoundrylineFactoryPlant,BusinessPlantId);DOREPLIFETIME(AFoundrylineFactoryPlant,Snapshot);DOREPLIFETIME(AFoundrylineFactoryPlant,PresentationEpoch);}
void AFoundrylineFactoryPlant::RefreshSnapshot(){Snapshot=Model.State;auto* GS=GetWorld()->GetGameState();PresentationEpoch=GS?GS->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();PublishCommerce();}
void AFoundrylineFactoryPlant::OnRep_State(){bRebuild=true;}
FVector AFoundrylineFactoryPlant::MachinePosition(int32 Id){return FFoundrylineWorkshopLayout::Machine(Id);}
FVector AFoundrylineFactoryPlant::ScreenPosition(int32 Id){return MachinePosition(Id)+(Id==1?FVector(269,197,148):Id==2?FVector(180,215,152):FVector(115,277.6,152));}
void AFoundrylineFactoryPlant::Tick(float Dt){Super::Tick(Dt);if(HasAuthority()){
 auto* BusinessRegistry=AFoundrylineBusinessRegistry::Find(GetWorld());int64 BeforeCash=Model.State.Commerce.CashCents;if(Model.State.Commerce.Enabled&&BusinessRegistry){if(const auto* C=BusinessRegistry->GetLedger().Company(BusinessCompanyId))Model.State.Commerce.CashCents=BeforeCash=C->BalanceCents;}
 SimClock+=FMath::Min(Dt,.25f);while(SimClock>=.1f){Model.Step(.1f);SimClock-=.1f;}
 if(Model.State.Commerce.Enabled&&BusinessRegistry)BusinessRegistry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,BeforeCash,Model.State.Commerce.CashCents);EnsureCommerceEquipment();
 PublishClock+=Dt;if(PublishClock>=.25f){PublishClock=0;RefreshSnapshot();bRebuild=true;}
 }if(!HasAuthority()&&bRebuild)EnsureCommerceEquipment();if(bRebuild){RebuildPlanningLayout();UpdateCollision();RebuildWeldingStations();}UpdateEmployees(Dt);if(GetNetMode()!=NM_DedicatedServer){if(bRebuild){RebuildVisuals();bRebuild=false;}Animate(Dt);}else bRebuild=false;
 UpdatePhysicalObjects(Dt);}
void AFoundrylineFactoryPlant::UpdateCollision(){
 if(ProductCollision.Num()!=Snapshot.Zones.Num()||WelderCollision.Num()!=Snapshot.Zones.Num()){
  for(auto C:ProductCollision)C->DestroyComponent();ProductCollision.Reset();for(auto C:WelderCollision)C->DestroyComponent();WelderCollision.Reset();
  for(const auto& Z:Snapshot.Zones){
   auto Box=[&](FVector Half){auto* C=NewObject<UBoxComponent>(this);C->SetupAttachment(RootComponent);C->SetBoxExtent(Half);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->SetCollisionProfileName(TEXT("BlockAllDynamic"));C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);return C;};
   ProductCollision.Add(Box({665,130,180}));auto* Machine=Box({65,40,85});Machine->ComponentTags.Add(TEXT("FoundrylineZoneWelder"));WelderCollision.Add(Machine);
  }
 }
 for(int I=0;I<ProductCollision.Num();++I){const auto& Z=Snapshot.Zones[I];const auto R=FFoundrylineWorkZoneLayout::Rotation(Z);ProductCollision[I]->SetRelativeLocationAndRotation(FFoundrylineWorkZoneLayout::ProductCenter(Z)+FVector(0,0,180),R);ProductCollision[I]->SetCollisionEnabled(Z.InstalledLb>0?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);WelderCollision[I]->SetRelativeLocationAndRotation(FFoundrylineWorkZoneLayout::WelderPosition(Z)+FVector(0,0,80),R);}
}
bool AFoundrylineFactoryPlant::Execute(APlayerController* P,const FFoundrylineFactoryCommand& C,FString& E){
 if(!HasAuthority()||!P||P->GetWorld()!=GetWorld()||!P->PlayerState||P->PlayerState->IsOnlyASpectator()||!GetWorld()->GetGameState()||!GetWorld()->GetGameState()->PlayerArray.Contains(P->PlayerState)){E=TEXT("Join the shared plant before editing production");return false;}
 if(auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());Registry&&!Registry->CanOperate(P,Model.State.CompanyId,Model.State.PlantId)){E=TEXT("This plant belongs to another business");return false;}
 const auto BeforeCommand=Model;
 bool Ok=false;using FactoryAct=EFoundrylineFactoryAction;
 switch(C.Action){
 case FactoryAct::Run:Model.State.Running=C.Flag;Ok=true;break;
 case FactoryAct::Speed:if(C.Count==1||C.Count==4||C.Count==8){Model.State.Speed=C.Count;Ok=true;}break;
 case FactoryAct::Order:Ok=Model.Order(C.Id,E);break;
 case FactoryAct::Collect:Ok=Model.Collect(C.Id,E);break;
 case FactoryAct::ReceiveSteel:Ok=Model.ReceiveSteel(C.Count,E);break;
 case FactoryAct::ReceiveKits:Ok=Model.ReceiveKits(E);break;
 case FactoryAct::Program:Ok=Model.Program(C.Id,C.Part,C.Count,C.Flag,E);if(Ok&&Model.State.Commerce.Enabled)Model.State.Running=true;break;
 case FactoryAct::MachineEnabled:if(auto* M=Model.State.Machines.FindByPredicate([&](const auto& X){return X.Id==C.Id;})){if(!M->bStored){M->Enabled=C.Flag;Ok=true;}}break;
 case FactoryAct::AddDriver:E=TEXT("Purchase a forklift, hire an employee, then assign the driving job in the ERP");break;
 case FactoryAct::AddWelder:E=TEXT("Hire an employee and assign a product-zone welding job in the ERP");break;
 case FactoryAct::BuyPlanningEquipment:{auto* Controller=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Purchase equipment from the office computer Planning View");else Ok=Model.BuyPlanningEquipment(C.Id,C.A,C.QuarterTurns,C.Count,E,C.SupplySide<0);break;}
 case FactoryAct::EditPlanningItem:{auto* Controller=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Use your business office Planning View to edit equipment");else Ok=Model.EditPlanningItem(C.Count,C.Id,C.A,C.Flag,E);break;}
 case FactoryAct::ConfigureSupplyBin:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Supply bin belongs to another plant");else Ok=Model.ConfigureSupplyBin(C.Id,C.Count,E);break;
 case FactoryAct::AssignSlot:Ok=Model.AssignSlot(C.Id,C.Part,E);break;
 case FactoryAct::HireEmployee:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Employee hiring belongs to another business");else if(C.Count<0||C.Count>4)E=TEXT("Invalid employee role");else Ok=Model.HireEmployee(EFoundrylineEmployeeRole(C.Count),C.RequestId,E);break;
 case FactoryAct::AssignEmployee:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Employee assignment belongs to another plant");else if(C.Count<0||C.Count>4)E=TEXT("Invalid employee role");else Ok=Model.AssignEmployee(C.Id,EFoundrylineEmployeeRole(C.Count),C.Other,E,C.QuarterTurns);break; // QuarterTurns carries the welding-station slot (0 A, 1 B).
 case FactoryAct::AddPresetZone:if(!Model.State.Running)Ok=Model.AddPresetZone(C.A,C.Id,C.QuarterTurns,C.SupplySide,C.Other,E);else E=TEXT("Pause before placing a work zone");break;
 case FactoryAct::AddWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else {auto* Controller=Cast<AFoundrylinePlayerController>(P);if(!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Use the office computer Planning View");else Ok=Model.BuyPlanningEquipment(0,C.A,C.QuarterTurns,1,E,C.SupplySide<0);}break;
 case FactoryAct::RemoveWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else Ok=Model.RemoveWeldingStation(C.Id,E);break;
 case FactoryAct::ConfigureWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else Ok=Model.ConfigureWeldingStation(C.Id,C.Other,C.Count,E);break;
 case FactoryAct::AddZone:if(!Model.State.Running)Ok=Model.AddZone(C.A,C.B,E);else E=TEXT("Pause before changing the floor layout");break;
 case FactoryAct::PaintLane:if(!Model.State.Running&&C.Id>=0&&C.Id<=2)Ok=Model.PaintLane(EFoundrylineFloorLane(C.Id),C.A,C.B,C.WidthCm,C.Other,E);else E=TEXT("Pause before painting floor routes");break;
 case FactoryAct::EraseLane:if(!Model.State.Running){const int Removed=Model.State.Lanes.RemoveAll([&](const auto& L){return L.Id==C.Id;});Ok=Removed>0;if(Ok){for(auto& D:Model.State.Drivers){D.Path.Reset();D.Distance=0;}for(auto& W:Model.State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}for(auto& O:Model.State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}}}else E=TEXT("Pause before removing floor routes");break;
 case FactoryAct::AddRoute:if(Model.State.Routes.Num()<8){FFoundrylineFactoryRoute R;R.Id=Model.State.NextId++;R.Name=FString::Printf(TEXT("Supply route %d"),Model.State.Routes.Num()+1);Model.State.Routes.Add(R);Ok=true;}break;
 case FactoryAct::LinkSource:case FactoryAct::LinkDestination:{auto* R=Model.State.Routes.FindByPredicate([&](const auto& X){return X.Id==C.Id;});bool Endpoint=C.Other==10||C.Other==11||Model.State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==C.Other;})||Model.State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==C.Other;})||Model.State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==C.Other;})||Model.State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==C.Other;});if(R&&Endpoint){auto& List=C.Action==FactoryAct::LinkSource?R->Sources:R->Destinations;if(C.Flag)List.AddUnique(C.Other);else List.Remove(C.Other);Ok=true;}break;}
 case FactoryAct::Save:case FactoryAct::Load:if(P->IsLocalController())Ok=C.Action==FactoryAct::Save?SaveFactory(E):LoadFactory(E);else E=TEXT("The host saves and loads the shared plant");break;
 }
 if(Ok&&Model.State.Commerce.CashCents!=BeforeCommand.State.Commerce.CashCents){auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());if(!Registry||!Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,BeforeCommand.State.Commerce.CashCents,Model.State.Commerce.CashCents)){Model=BeforeCommand;E=TEXT("Business balance changed; purchase was not applied");Ok=false;}}
 if(Ok){++Model.State.Revision;RefreshSnapshot();bRebuild=true;ForceNetUpdate();}else if(E.IsEmpty())E=TEXT("That factory action is unavailable");return Ok;
}
bool AFoundrylineFactoryPlant::SaveFactory(FString& E,const FString& TestSlot){if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&R->HasMultipleBusinesses()){E=TEXT("Multiple businesses require a combined world save");return false;}if(!HasAuthority()){E=TEXT("Host only");return false;}if(!Model.Valid(E))return false;
 auto* S=NewObject<UFoundrylineFactorySave>();S->Factory=Model.State;auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 if(!Placement||!UGameplayStatics::SaveGameToMemory(Placement->CreateSnapshot(),S->Property)){E=TEXT("Could not snapshot the property");return false;}
 if(!UGameplayStatics::SaveGameToSlot(S,(TestSlot.IsEmpty()?(bCutPartsStarter?TEXT("FoundrylineCutParts01"):TEXT("FoundrylineFactory01")):TestSlot),0)){E=TEXT("Could not write factory save");return false;}return true;
}
bool AFoundrylineFactoryPlant::LoadFactory(FString& E,const FString& TestSlot){if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&R->HasMultipleBusinesses()){E=TEXT("Multiple businesses require a combined world save");return false;}if(!HasAuthority()){E=TEXT("Host only");return false;}auto* S=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromSlot((TestSlot.IsEmpty()?(bCutPartsStarter?TEXT("FoundrylineCutParts01"):TEXT("FoundrylineFactory01")):TestSlot),0));if(!S){E=TEXT("No saved factory in this slot");return false;}
 if(S->Factory.bCutPartsStarter!=bCutPartsStarter){E=TEXT("This save belongs to a different shop scenario");return false;}
 FFoundrylineFactorySimulation Candidate;Candidate.State=S->Factory;Candidate.Solids=FFoundrylineWorkshopLayout::PlantSolids(Candidate.State);
 if(Candidate.State.Version==5){Candidate.State.Version=6;for(auto& Z:Candidate.State.Zones){Z.ReservedParts.SetNumZeroed(FFoundrylineFactoryCatalog::PartCount);Z.Installed.SetNumZeroed(FFoundrylineFactoryCatalog::PartCount);}for(auto& M:Candidate.State.Machines)M.Origin=FFoundrylineWorkshopLayout::Machine(M.Id);Candidate.State.Programs=FFoundrylineFactoryCatalog::DefaultPrograms();if(Candidate.State.bCutPartsStarter){Candidate.State.Programs.RemoveAll([](const auto& P){return P.Machine!=1||P.Part!=EFoundrylinePart::Bracket&&P.Part!=EFoundrylinePart::MountingPlate&&P.Part!=EFoundrylinePart::GussetPlate&&P.Part!=EFoundrylinePart::ReinforcementPlate;});const auto LegacyDrivers=Candidate.State.Drivers;Candidate.SeedCommerce();for(const auto& Driver:LegacyDrivers)if(Driver.Job)if(auto* Current=Candidate.State.Drivers.FindByPredicate([&](const auto& D){return D.Id==Driver.Id;}))*Current=Driver;}}
 Candidate.MigrateEmployees();Candidate.MigrateEmployeeNames();Candidate.MigrateGenericEmployees();
 // Schema 10 adds welding stations (a schema 9 plant loads with none); schema 11 adds bought parts and station supplies.

 Candidate.MigrateWeldingStations();Candidate.MigrateAssemblySupply();

 // Migrate only the old authored parking positions; preserve jobs and player-authored state.
 for(int I=0;I<Candidate.State.Drivers.Num();++I){auto& D=Candidate.State.Drivers[I];const FVector OldHome(100,I<2?400.+I*500.:3600.+(I-2)*500.,20);if(D.Parking.Equals(OldHome,1)){
  const bool Parked=!D.Job&&FVector::Dist2D(D.Position,D.Parking)<20;D.Parking=FFoundrylineWorkshopLayout::DriverHome(I);
  if(Parked){D.Position=D.Parking;D.Yaw=90;}if(!D.Job){D.Path.Reset();D.Distance=0;}
 }}
 bool AdjustedStarterLanes=false;
 for(auto& L:Candidate.State.Lanes){const auto OldMin=L.Min,OldMax=L.Max;
  if(L.Kind==EFoundrylineFloorLane::Forklift&&L.Min.Equals({-300,2300},.01)&&L.Max.Equals({2600,2700},.01)){L.Min.Y+=50;L.Max.Y+=50;}
  if(L.Kind==EFoundrylineFloorLane::Forklift&&L.Min.Equals({-300,3300},.01)&&L.Max.Equals({2600,3700},.01)){L.Min.Y-=50;L.Max.Y-=50;}
  if(L.Kind==EFoundrylineFloorLane::Pedestrian&&L.Min.Equals({400,1400},.01)&&L.Max.Equals({1900,1500},.01))L.Max.X=941.2;
  if(L.Kind==EFoundrylineFloorLane::Pedestrian&&L.Min.Equals({400,4500},.01)&&L.Max.Equals({1900,4600},.01))L.Max.X=941.2;
  AdjustedStarterLanes|=!L.Min.Equals(OldMin,.01)||!L.Max.Equals(OldMax,.01);
 }
 if(AdjustedStarterLanes){for(auto& D:Candidate.State.Drivers){D.Path.Reset();D.Distance=0;}for(auto& W:Candidate.State.Walkers){W.Path.Reset();W.Distance=0;}}
 if(!Candidate.Valid(E))return false;
 if(Candidate.State.CompanyId!=Model.State.CompanyId||Candidate.State.PlantId!=Model.State.PlantId){E=TEXT("This save belongs to a different company or plant");return false;}
 auto* Property=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(S->Property));auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 if(Property&&Placement)for(const auto& Object:Property->Objects)Placement->EnsureGeneratedDefinition(Object.DefinitionId);
 if(!Property||!Placement){E=TEXT("Saved property is incompatible; factory left unchanged");return false;}
 // Reserve the account change first. RestoreSnapshot validates its complete candidate
 // before changing placement; roll the account back if that atomic restore rejects.
 auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());int64 OldCash=0;bool CashReserved=false;
 if(Candidate.State.Commerce.Enabled){const auto* Company=Registry?Registry->GetLedger().Company(BusinessCompanyId):nullptr;if(!Company||Company->PlantId!=BusinessPlantId){E=TEXT("Could not restore company balance; factory left unchanged");return false;}OldCash=Company->BalanceCents;if(!Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,OldCash,Candidate.State.Commerce.CashCents)){E=TEXT("Could not restore company balance; factory left unchanged");return false;}CashReserved=true;}
 if(Placement->RestoreSnapshot(Property,true)!=EFoundrylinePlacementError::None){if(CashReserved)Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,Candidate.State.Commerce.CashCents,OldCash);E=TEXT("Saved property is incompatible; factory and account left unchanged");return false;}

 Model=MoveTemp(Candidate);RefreshSnapshot();EnsureCommerceEquipment();bRebuild=true;SimClock=0;ForceNetUpdate();return true;
}
FString AFoundrylineFactoryPlant::GetLedgerDigest()const{TArray<uint8> Bytes;FMemoryWriter Writer(Bytes);FObjectAndNameAsStringProxyArchive Ar(Writer,false);auto Copy=Snapshot;Copy.Revision=0;Copy.Commerce={};FFoundrylineFactoryState::StaticStruct()->SerializeItem(Ar,&Copy,nullptr);return FString::Printf(TEXT("%08x"),FCrc::MemCrc32(Bytes.GetData(),Bytes.Num()));}
FString AFoundrylineFactoryPlant::GetFactoryReport()const{FFoundrylineFactorySimulation V;V.State=Snapshot;int Progress=0,Mask=0,Reserved=0,Carry=0,Arrived=0;for(const auto& Z:Snapshot.Zones){Mask|=Z.CompletedMask;for(int Q:Z.Installed)Progress+=Q;}for(const auto& P:Snapshot.Pallets){Reserved+=P.Reserved;if(P.Place==EFoundrylinePalletPlace::Forks)Carry+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Receiving&&P.Part==EFoundrylinePart::Sheet)Arrived+=P.Quantity;}int Diagonal=0,Curved=0;for(const auto& D:Snapshot.Drivers)for(int I=1;I<D.Path.Num();++I){const auto A=D.Path[I]-D.Path[I-1];if(FMath::Abs(A.X)>1&&FMath::Abs(A.Y)>1)++Diagonal;if(I>1){const auto B=D.Path[I-1]-D.Path[I-2];if(A.Size()>1&&B.Size()>1){const double Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(B.Rotation().Yaw,A.Rotation().Yaw));if(Turn>1&&Turn<30&&A.Size()<150&&B.Size()<150)++Curved;}}}
 return FString::Printf(TEXT("{\"running\":%s,\"revision\":%lld,\"pallets\":%d,\"jobs\":%d,\"reserved\":%d,\"carried\":%d,\"installed\":%d,\"mask\":%d,\"raw\":%d,\"arrival\":%d,\"finished\":%d,\"mass\":%.3f,\"diagonal_segments\":%d,\"curved_segments\":%d}"),Snapshot.Running?TEXT("true"):TEXT("false"),Snapshot.Revision,Snapshot.Pallets.Num(),Snapshot.Jobs.Num(),Reserved,Carry,Progress,Mask,V.Total(EFoundrylinePart::Sheet),Arrived,Snapshot.FinishedProducts,V.SteelMass(),Diagonal,Curved);}
