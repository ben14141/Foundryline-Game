#include "Production/FoundrylineFactoryPlant.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Production/FoundrylineWorkshopLayout.h"
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
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"

AFoundrylineFactoryPlant::AFoundrylineFactoryPlant(){bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);SetNetUpdateFrequency(2);PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Plant")));}
void AFoundrylineFactoryPlant::BeginPlay(){Super::BeginPlay();if(HasAuthority()){if(GetWorld()->GetMapName().Contains(TEXT("L_BasicShopStarter"))){Model.State=FFoundrylineFactoryState();Model.State.bCutPartsStarter=true;Model.SeedCommerce();Model.State.Lanes.Reset();Model.State.FloorMin={-2370,30};Model.State.FloorMax={-30,1770};Model.Solids.Reset();Model.State.Status=TEXT("Use the office computer to plan and equip your shop");}else if(bCutPartsStarter)Model.SeedCutPartsShop();else Model.Seed(true);Model.State.CompanyId=BusinessCompanyId;Model.State.PlantId=BusinessPlantId;if(Model.State.Commerce.Enabled){if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld()))Model.State.Commerce.CashCents=R->InitialiseEconomy(BusinessCompanyId,BusinessPlantId,Model.State.Commerce.CashCents);OperationsState=GetWorld()->SpawnActor<AFoundrylineOperationsState>();if(OperationsState){OperationsState->CompanyId=BusinessCompanyId;OperationsState->PlantId=BusinessPlantId;}}RefreshSnapshot();}SetupVisuals();}
void AFoundrylineFactoryPlant::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineFactoryPlant,bCutPartsStarter);DOREPLIFETIME(AFoundrylineFactoryPlant,BusinessCompanyId);DOREPLIFETIME(AFoundrylineFactoryPlant,BusinessPlantId);DOREPLIFETIME(AFoundrylineFactoryPlant,Snapshot);DOREPLIFETIME(AFoundrylineFactoryPlant,PresentationEpoch);}
void AFoundrylineFactoryPlant::RefreshSnapshot(){RefreshShippingPickup();Snapshot=Model.State;auto* GS=GetWorld()->GetGameState();PresentationEpoch=GS?GS->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();PublishCommerce();}
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
 if(C.Action==EFoundrylineFactoryAction::Program||C.Action==EFoundrylineFactoryAction::QueueMachineJob||C.Action==EFoundrylineFactoryAction::CancelMachineBatch){const auto* Controller=Cast<AFoundrylinePlayerController>(P);FVector Eye;FRotator View;P->GetPlayerViewPoint(Eye,View);if(!Controller||!Controller->IsWalking()||FVector::Dist(Eye,ScreenWorldPosition(C.Id))>325){E=TEXT("Program production at this machine's physical terminal");return false;}}
 const auto BeforeCommand=Model;
 bool Ok=false;using FactoryAct=EFoundrylineFactoryAction;
 switch(C.Action){
 case FactoryAct::UndoPlanner:case FactoryAct::RedoPlanner:{auto* Controller=Cast<AFoundrylinePlayerController>(P);auto& From=C.Action==FactoryAct::UndoPlanner?PlannerUndoStates:PlannerRedoStates;auto& To=C.Action==FactoryAct::UndoPlanner?PlannerRedoStates:PlannerUndoStates;
  if(!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking()||Model.State.Running||!Model.State.Jobs.IsEmpty())E=TEXT("Pause at the planner and finish active deliveries before undoing layout changes");
  else if(From.IsEmpty())E=TEXT("No floor-layout change to restore");
  else if(Model.State.Revision!=PlannerHistoryRevision||PlannerHistoryPlayer!=P->PlayerState->GetPlayerId())E=TEXT("The shared plant changed since your edit; it cannot be undone safely");
  else {auto Candidate=Model;Candidate.State=From.Last();Candidate.State.Revision=Model.State.Revision;Candidate.Solids=FFoundrylineWorkshopLayout::PlantSolids(Candidate.State);if(Candidate.Valid(E)){To.Add(Model.State);From.Pop();Model=MoveTemp(Candidate);Ok=true;}}break;}
 case FactoryAct::Run:Model.State.Running=C.Flag;Ok=true;break;
 case FactoryAct::Speed:if(C.Count==1||C.Count==4||C.Count==8){Model.State.Speed=C.Count;Ok=true;}break;
 case FactoryAct::Order:Ok=Model.Order(C.Id,E);break;
 case FactoryAct::Collect:Ok=Model.Collect(C.Id,E);break;
 case FactoryAct::ReceiveSteel:Ok=Model.ReceiveSteel(C.Count,E);break;
 case FactoryAct::ReceiveKits:Ok=Model.ReceiveKits(E);break;
 case FactoryAct::Program:Ok=Model.Program(C.Id,C.Part,C.Count,C.Flag,E);if(Ok&&Model.State.Commerce.Enabled)Model.State.Running=true;break;
 case FactoryAct::QueueMachineJob:Ok=Model.QueueMachineJob(C.Id,C.Other,C.Count,E,C.Part);if(Ok)Model.State.Running=true;break;
 case FactoryAct::CancelMachineBatch:if(auto* M=Model.State.Machines.FindByPredicate([&](const auto& V){return V.Id==C.Id;})){if(C.Other==-1&&M->HasQueuedProgram){M->HasQueuedProgram=false;++Model.State.Revision;Ok=true;}else if(M->Queue.IsValidIndex(C.Other)){M->Queue.RemoveAt(C.Other);++Model.State.Revision;Ok=true;}}break;
 case FactoryAct::MachineEnabled:if(auto* M=Model.State.Machines.FindByPredicate([&](const auto& X){return X.Id==C.Id;})){if(!M->bStored){M->Enabled=C.Flag;Ok=true;}}break;
 case FactoryAct::AddDriver:E=TEXT("Purchase a forklift, hire an employee, then assign the driving job in the ERP");break;
 case FactoryAct::AddWelder:E=TEXT("Hire an employee and assign a product-zone welding job in the ERP");break;
 case FactoryAct::BuyStoredMachine:{auto* Controller=Cast<AFoundrylinePlayerController>(P);
  if(C.TargetPlantId!=BusinessPlantId||!Controller||!Controller->IsWalking()||!Controller->IsNearOfficeComputer())E=TEXT("Buy production equipment at your business office computer");
  else Ok=Model.BuyStoredMachine(C.Id,E);break;}
 case FactoryAct::BuyPlanningEquipment:{auto* Controller=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Purchase equipment from the office computer Planning View");else if(C.Id>=4&&C.Id<=6)E=TEXT("Buy machines at the office computer first, then place your owned equipment here");else {auto Purchase=Model;if((C.Id==0||C.Id>=4)&&!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Purchase.State,GetWorld(),GetActorTransform(),C.Id==7?FFoundrylinePlanningEquipment::Bounds(C.A+FRotator(0,C.QuarterTurns*90,0).RotateVector(FVector(0,40,0)),C.QuarterTurns,FFoundrylinePlanningEquipment::Size(7)):C.Id==0?FFoundrylinePlannerItems::StationWorkspaceBounds(C.A,C.QuarterTurns,C.SupplySide<0):FFoundrylineFactorySimulation::MachineWorkspaceBounds(C.Id-3,C.A,C.QuarterTurns),E,C.Id==7?0:-1,C.Id==7?C.A.Z:20))break;Ok=Purchase.BuyPlanningEquipment(C.Id,C.A,C.QuarterTurns,C.Count,E,C.SupplySide<0);if(Ok)Model=MoveTemp(Purchase);}break;}
 case FactoryAct::EditPlanningItem:{if(C.Count==int32(EFoundrylinePlannerItem::OfficeDesk)&&FFoundrylineOffices::DeskInUse(Model.State,C.Id)){E=TEXT("Unassign this office before moving or removing its workstation");break;}auto* Controller=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Use your business office Planning View to edit equipment");else {auto Candidate=Model;
   if(!C.Flag&&C.Count==int32(EFoundrylinePlannerItem::OfficeDesk)&&!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FFoundrylinePlanningEquipment::Bounds(C.A+FRotator(0,(C.QuarterTurns<0?0:C.QuarterTurns)*90,0).RotateVector(FVector(0,40,0)),C.QuarterTurns<0?0:C.QuarterTurns,FFoundrylinePlanningEquipment::Size(7)),E,C.Id,C.A.Z))break;
   if(!C.Flag&&C.Count==int32(EFoundrylinePlannerItem::Machine))if(const auto* Machine=Candidate.State.Machines.FindByPredicate([&](const auto& M){return M.Id==C.Id;})){
    if(!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FFoundrylineFactorySimulation::MachineWorkspaceBounds(Machine->Kind(),C.A,C.QuarterTurns<0?Machine->QuarterTurns:C.QuarterTurns),E))break;
   }
   if(!C.Flag&&C.Count==int32(EFoundrylinePlannerItem::Station))if(const auto* Station=Candidate.State.WeldingStations.FindByPredicate([&](const auto& S){return S.Id==C.Id;})){if(!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FFoundrylinePlannerItems::StationWorkspaceBounds(C.A,C.QuarterTurns<0?Station->QuarterTurns:C.QuarterTurns,Station->bSwapSides),E))break;}
   if(!C.Flag&&C.Count==int32(EFoundrylinePlannerItem::WorkZone))if(const auto* Zone=Candidate.State.Zones.FindByPredicate([&](const auto& Z){return Z.Id==C.Id;})){const auto OldBounds=FFoundrylineWorkZoneLayout::Bounds(*Zone);const FVector2D Half=OldBounds.GetSize()*.5;FVector2D RotatedHalf=Half;if(C.QuarterTurns>=0&&(C.QuarterTurns-Zone->QuarterTurns+4)%2)Swap(RotatedHalf.X,RotatedHalf.Y);if(!FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FBox2D(FVector2D(C.A)-RotatedHalf,FVector2D(C.A)+RotatedHalf),E))break;}
   Ok=Candidate.EditPlanningItem(C.Count,C.Id,C.A,C.Flag,E,C.QuarterTurns,C.B);if(Ok)Model=MoveTemp(Candidate);
  }break;}
 case FactoryAct::ConfigureSupplyBin:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Supply bin belongs to another plant");else Ok=Model.ConfigureSupplyBin(C.Id,C.Count,E);break;
 case FactoryAct::AssignSlot:Ok=Model.AssignSlot(C.Id,C.Part,E);break;
 case FactoryAct::AssignOffice:{auto* PC=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!PC||!PC->IsWalking()||!PC->IsNearOfficeComputer())E=TEXT("Assign offices at your business computer");else Ok=FFoundrylineOffices::Assign(GetWorld(),Model,GetActorTransform(),C.OfficeBuilding,C.OfficeRoom,C.Count,C.Id,E);break;}
 case FactoryAct::ReleaseOffice:{auto* PC=Cast<AFoundrylinePlayerController>(P);if(C.TargetPlantId!=BusinessPlantId||!PC||!PC->IsWalking()||!PC->IsNearOfficeComputer())E=TEXT("Manage offices at your business computer");else Ok=FFoundrylineOffices::Release(Model,C.Id,E);break;}
 case FactoryAct::HireEmployee:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Employee hiring belongs to another business");else if(C.Count<0||C.Count>5)E=TEXT("Invalid employee role");else Ok=Model.HireEmployee(EFoundrylineEmployeeRole(C.Count),C.RequestId,E);break;
 case FactoryAct::AssignEmployee:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("Employee assignment belongs to another plant");else if(C.Count<0||C.Count>4)E=TEXT("Invalid employee role");else Ok=Model.AssignEmployee(C.Id,EFoundrylineEmployeeRole(C.Count),C.Other,E,C.QuarterTurns);break; // QuarterTurns carries the welding-station slot (0 A, 1 B).
 case FactoryAct::AddPresetZone:if(!Model.State.Running){auto Candidate=Model;FFoundrylineFactoryZone Zone;if(!FFoundrylineWorkZoneLayout::Create(C.A,C.Id,C.QuarterTurns,C.SupplySide,Zone))E=TEXT("Choose a valid work-zone preset");else if(FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FFoundrylineWorkZoneLayout::Bounds(Zone),E)){Ok=Candidate.AddPresetZone(C.A,C.Id,C.QuarterTurns,C.SupplySide,C.Other,E);if(Ok)Model=MoveTemp(Candidate);}}else E=TEXT("Pause before placing a work zone");break;
 case FactoryAct::AddWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else {auto* Controller=Cast<AFoundrylinePlayerController>(P);if(!Controller||!Controller->bPropertyViewAccess||Controller->IsWalking())E=TEXT("Use the office computer Planning View");else {auto Candidate=Model;if(FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FFoundrylinePlannerItems::StationWorkspaceBounds(C.A,C.QuarterTurns,C.SupplySide<0),E)){Ok=Candidate.BuyPlanningEquipment(0,C.A,C.QuarterTurns,1,E,C.SupplySide<0);if(Ok)Model=MoveTemp(Candidate);}}}break;
 case FactoryAct::RemoveWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else Ok=Model.RemoveWeldingStation(C.Id,E);break;
 case FactoryAct::ConfigureWeldingStation:if(C.TargetPlantId!=BusinessPlantId)E=TEXT("That welding station belongs to another plant");else Ok=Model.ConfigureWeldingStation(C.Id,C.Other,C.Count,E);break;
 case FactoryAct::AddZone:if(!Model.State.Running){auto Candidate=Model;const FVector2D Min(FMath::Min(C.A.X,C.B.X),FMath::Min(C.A.Y,C.B.Y));FVector2D Max(FMath::Max(C.A.X,C.B.X),FMath::Max(C.A.Y,C.B.Y));Max.X=FMath::Max(Max.X,Min.X+FFoundrylineFactoryCatalog::TankerDropCm().X);Max.Y+=200+FFoundrylineFactoryCatalog::TankerDropCm().Y;if(FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),FBox2D(Min,Max),E)){Ok=Candidate.AddZone(C.A,C.B,E);if(Ok)Model=MoveTemp(Candidate);}}else E=TEXT("Pause before changing the floor layout");break;
 case FactoryAct::PaintLane:if(!Model.State.Running&&C.Id>=0&&C.Id<=2){if(C.Id==0&&C.Other==-1){auto Candidate=Model;const FBox2D Bounds(FVector2D(FMath::Min(C.A.X,C.B.X),FMath::Min(C.A.Y,C.B.Y)),FVector2D(FMath::Max(C.A.X,C.B.X),FMath::Max(C.A.Y,C.B.Y)));if(FFoundrylinePlannerItems::PrepareOwnedBuildingFloor(Candidate.State,GetWorld(),GetActorTransform(),Bounds,E)){Ok=Candidate.PaintForkliftArea(C.A,C.B,E);if(Ok)Model=MoveTemp(Candidate);}}else if(C.Id==0&&C.WidthCm<360)E=TEXT("New forklift areas must be at least 11 ft 10 in wide");else Ok=Model.PaintLane(EFoundrylineFloorLane(C.Id),C.A,C.B,C.WidthCm,C.Other,E);}else E=TEXT("Pause before painting forklift areas");break;
 case FactoryAct::EraseLane:if(!Model.State.Running){const int Removed=Model.State.Lanes.RemoveAll([&](const auto& L){return L.Id==C.Id;});Ok=Removed>0;if(Ok){for(auto& D:Model.State.Drivers){D.Path.Reset();D.Distance=0;}for(auto& W:Model.State.Walkers){W.Path.Reset();W.Distance=0;W.Retry=0;}for(auto& O:Model.State.Operators){O.Path.Reset();O.Distance=0;O.Retry=0;}}}else E=TEXT("Pause before removing floor routes");break;
 case FactoryAct::AddRoute:if(Model.State.Routes.Num()<8){FFoundrylineFactoryRoute R;R.Id=Model.State.NextId++;R.Name=FString::Printf(TEXT("Supply route %d"),Model.State.Routes.Num()+1);Model.State.Routes.Add(R);Ok=true;}break;
 case FactoryAct::LinkSource:case FactoryAct::LinkDestination:{auto* R=Model.State.Routes.FindByPredicate([&](const auto& X){return X.Id==C.Id;});bool Endpoint=C.Other==10||C.Other==11||Model.State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==C.Other;})||Model.State.Machines.ContainsByPredicate([&](const auto& M){return M.Id==C.Other;})||Model.State.Zones.ContainsByPredicate([&](const auto& Z){return Z.Id==C.Other;})||Model.State.WeldingStations.ContainsByPredicate([&](const auto& S){return S.Id==C.Other;});if(R&&Endpoint){auto& List=C.Action==FactoryAct::LinkSource?R->Sources:R->Destinations;if(C.Flag)List.AddUnique(C.Other);else List.Remove(C.Other);Ok=true;}break;}
 case FactoryAct::Save:case FactoryAct::Load:if(P->IsLocalController())Ok=C.Action==FactoryAct::Save?SaveFactory(E):LoadFactory(E);else E=TEXT("The host saves and loads the shared plant");break;
 }
 if(Ok&&Model.State.Commerce.CashCents!=BeforeCommand.State.Commerce.CashCents){auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());if(!Registry||!Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,BeforeCommand.State.Commerce.CashCents,Model.State.Commerce.CashCents)){Model=BeforeCommand;E=TEXT("Business balance changed; purchase was not applied");Ok=false;}}
 if(Ok){++Model.State.Revision;if(C.Action==FactoryAct::EditPlanningItem){PlannerUndoStates.Add(BeforeCommand.State);if(PlannerUndoStates.Num()>12)PlannerUndoStates.RemoveAt(0);PlannerRedoStates.Reset();PlannerHistoryRevision=Model.State.Revision;PlannerHistoryPlayer=P->PlayerState->GetPlayerId();}else if(C.Action==FactoryAct::UndoPlanner||C.Action==FactoryAct::RedoPlanner)PlannerHistoryRevision=Model.State.Revision;RefreshSnapshot();bRebuild=true;ForceNetUpdate();}else if(E.IsEmpty())E=TEXT("That factory action is unavailable");return Ok;
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
 Candidate.MigrateCurrentParts();if(!Candidate.Valid(E))return false;
 if(Candidate.State.CompanyId!=Model.State.CompanyId||Candidate.State.PlantId!=Model.State.PlantId){E=TEXT("This save belongs to a different company or plant");return false;}
 auto* Property=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(S->Property));auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
 if(Property&&Placement)for(const auto& Object:Property->Objects)Placement->EnsureGeneratedDefinition(Object.DefinitionId);
 if(!Property||!Placement){E=TEXT("Saved property is incompatible; factory left unchanged");return false;}
 if(!FFoundrylineOffices::ValidateProperty(Candidate.State,Property->Objects,Placement,GetActorTransform(),E))return false;
 // Reserve the account change first. RestoreSnapshot validates its complete candidate
 // before changing placement; roll the account back if that atomic restore rejects.
 auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());int64 OldCash=0;bool CashReserved=false;
 if(Candidate.State.Commerce.Enabled){const auto* Company=Registry?Registry->GetLedger().Company(BusinessCompanyId):nullptr;if(!Company||Company->PlantId!=BusinessPlantId){E=TEXT("Could not restore company balance; factory left unchanged");return false;}OldCash=Company->BalanceCents;if(!Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,OldCash,Candidate.State.Commerce.CashCents)){E=TEXT("Could not restore company balance; factory left unchanged");return false;}CashReserved=true;}
 if(Placement->RestoreSnapshot(Property,true)!=EFoundrylinePlacementError::None){if(CashReserved)Registry->CommitPlantCash(BusinessCompanyId,BusinessPlantId,Candidate.State.Commerce.CashCents,OldCash);E=TEXT("Saved property is incompatible; factory and account left unchanged");return false;}

 Model=MoveTemp(Candidate);ShippingPickupPlacementRevision=-1;RefreshSnapshot();EnsureCommerceEquipment();bRebuild=true;SimClock=0;ForceNetUpdate();return true;
}
FString AFoundrylineFactoryPlant::GetLedgerDigest()const{TArray<uint8> Bytes;FMemoryWriter Writer(Bytes);FObjectAndNameAsStringProxyArchive Ar(Writer,false);auto Copy=Snapshot;Copy.Revision=0;Copy.Commerce={};FFoundrylineFactoryState::StaticStruct()->SerializeItem(Ar,&Copy,nullptr);return FString::Printf(TEXT("%08x"),FCrc::MemCrc32(Bytes.GetData(),Bytes.Num()));}
FString AFoundrylineFactoryPlant::GetFactoryReport()const{FFoundrylineFactorySimulation V;V.State=Snapshot;int Progress=0,Mask=0,Reserved=0,Carry=0,Arrived=0;for(const auto& Z:Snapshot.Zones){Mask|=Z.CompletedMask;for(int Q:Z.Installed)Progress+=Q;}for(const auto& P:Snapshot.Pallets){Reserved+=P.Reserved;if(P.Place==EFoundrylinePalletPlace::Forks)Carry+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Receiving&&P.Part==EFoundrylinePart::Sheet)Arrived+=P.Quantity;}int Diagonal=0,Curved=0;for(const auto& D:Snapshot.Drivers)for(int I=1;I<D.Path.Num();++I){const auto A=D.Path[I]-D.Path[I-1];if(FMath::Abs(A.X)>1&&FMath::Abs(A.Y)>1)++Diagonal;if(I>1){const auto B=D.Path[I-1]-D.Path[I-2];if(A.Size()>1&&B.Size()>1){const double Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(B.Rotation().Yaw,A.Rotation().Yaw));if(Turn>1&&Turn<30&&A.Size()<150&&B.Size()<150)++Curved;}}}
 return FString::Printf(TEXT("{\"running\":%s,\"revision\":%lld,\"pallets\":%d,\"jobs\":%d,\"reserved\":%d,\"carried\":%d,\"installed\":%d,\"mask\":%d,\"raw\":%d,\"arrival\":%d,\"finished\":%d,\"mass\":%.3f,\"diagonal_segments\":%d,\"curved_segments\":%d}"),Snapshot.Running?TEXT("true"):TEXT("false"),Snapshot.Revision,Snapshot.Pallets.Num(),Snapshot.Jobs.Num(),Reserved,Carry,Progress,Mask,V.Total(EFoundrylinePart::Sheet),Arrived,Snapshot.FinishedProducts,V.SteelMass(),Diagonal,Curved);}
