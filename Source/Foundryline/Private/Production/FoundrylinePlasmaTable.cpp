#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Construction/FoundrylineEquipmentFootprintComponent.h"
#include "Construction/FoundrylineEquipmentCollision.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

AFoundrylinePlasmaTable::AFoundrylinePlasmaTable(){
 bReplicates=true;SetReplicateMovement(false);SetNetUpdateFrequency(2);SetNetCullDistanceSquared(FMath::Square(10000.f));
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=1.f/30;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Table")));
 auto* Footprint=CreateDefaultSubobject<UFoundrylineEquipmentFootprintComponent>(TEXT("PlacementArea"));Footprint->SetupAttachment(RootComponent);Footprint->EquipmentId=TEXT("PlasmaTable");
 Gantry=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gantry"));Gantry->SetupAttachment(RootComponent);
 Carriage=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchCarriage"));Carriage->SetupAttachment(Gantry);
 Torch=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Torch"));Torch->SetupAttachment(Carriage);
 for(auto* M:{Gantry.Get(),Carriage.Get(),Torch.Get()}){M->SetMobility(EComponentMobility::Movable);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);}
 PlasmaEffect=CreateDefaultSubobject<UFoundrylineWeldingEffectComponent>(TEXT("PlasmaEffect"));PlasmaEffect->SetIsReplicated(false);PlasmaEffect->SetupAttachment(Torch);PlasmaEffect->SetRelativeLocation(AuthoredTorchTip());PlasmaEffect->bPlasmaCutting=true;
}
double AFoundrylinePlasmaTable::ServerTime()const{auto* State=GetWorld()->GetGameState();return State?State->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();}
void AFoundrylinePlasmaTable::BeginPlay(){
 Super::BeginPlay();
 auto Asset=[](const FString& Name){return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Foundryline/Production/Equipment/PlasmaTable/SM_PlasmaTable_")+Name+TEXT(".SM_PlasmaTable_")+Name));};
 Gantry->SetStaticMesh(Asset(TEXT("Gantry")));Carriage->SetStaticMesh(Asset(TEXT("TorchCarriage")));Torch->SetStaticMesh(Asset(TEXT("Torch")));
 auto Add=[&](const TCHAR* Name,USceneComponent* Parent,bool bStock=false){
  auto* M=NewObject<UStaticMeshComponent>(this,FName(Name));M->SetupAttachment(Parent);
  M->SetStaticMesh(bStock?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Products/Components/Steel/SM_PlasmaTable_StockSheet.SM_PlasmaTable_StockSheet")):Asset(Name));
  M->SetMobility(EComponentMobility::Movable);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);M->RegisterComponent();AddInstanceComponent(M);return M;
 };
 for(const TCHAR* Name:{TEXT("Body"),TEXT("SlatBed"),TEXT("OperatorConsole"),TEXT("PowerUnit"),TEXT("Extraction")}){auto* Part=Add(Name,RootComponent);if(FString(Name)==TEXT("OperatorConsole"))Part->SetRelativeLocation({32,0,0});if(FString(Name)!=TEXT("SlatBed"))FFoundrylineEquipmentCollision::AddPawnBody(Part);}
 Add(TEXT("CableChain"),Gantry);StockSheet=Add(TEXT("StockSheet"),RootComponent,true);
 for(int32 I=0;I<6;++I){
  auto* Lead=NewObject<USplineMeshComponent>(this);Lead->SetMobility(EComponentMobility::Movable);Lead->SetupAttachment(RootComponent);Lead->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Lead->SetForwardAxis(ESplineMeshAxis::Z,false);Lead->SetStartScale(FVector2D(.038));Lead->SetEndScale(FVector2D(.038));Lead->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lead->SetCanEverAffectNavigation(false);Lead->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Rubber.M_MH_Rubber")));Lead->RegisterComponent();AddInstanceComponent(Lead);Leads.Add(Lead);
 }
 if(HasAuthority()&&!bFactoryAutomation)SetOperating(true);
 if(GetNetMode()==NM_DedicatedServer)SetActorTickEnabled(false);
}
void AFoundrylinePlasmaTable::SetOperating(bool Active){
 if(!HasAuthority() || Cycle.bOperating==Active)return;
 Cycle.bOperating=Active;Cycle.Epoch=ServerTime();ForceNetUpdate();
 if(!Active){bCutting=false;PlasmaEffect->SetLocalPlasmaActive(false);}
}
void AFoundrylinePlasmaTable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylinePlasmaTable,BusinessCompanyId);DOREPLIFETIME(AFoundrylinePlasmaTable,BusinessPlantId);DOREPLIFETIME(AFoundrylinePlasmaTable,Cycle);DOREPLIFETIME(AFoundrylinePlasmaTable,bFactoryAutomation);}
namespace {
 FFoundrylinePlasmaPose ToolCycle(double T,float W,float L){
  FFoundrylinePlasmaPose P;const FVector Park=AFoundrylinePlasmaTable::AuthoredTorchTip()+FVector(0,0,4),Start(-W/2,-L/2,100.95);
  if(T<4){P.Tip=FMath::Lerp(Park,Start+FVector(0,0,4),T/4);return P;}
  if(T<5){P.Tip=Start+FVector(0,0,4*(5-T));return P;}
  if(T<25){float D=(T-5)*(2*W+2*L)/20;P.bCutting=true;if(D<W)P.Tip=Start+FVector(D,0,0);else if(D<W+L)P.Tip=Start+FVector(W,D-W,0);else if(D<2*W+L)P.Tip=Start+FVector(2*W+L-D,L,0);else P.Tip=Start+FVector(0,2*W+2*L-D,0);return P;}
  P.Tip=T<26?Start+FVector(0,0,4*(T-25)):FMath::Lerp(Start+FVector(0,0,4),Park,(T-26)/3);return P;
 }
}
FFoundrylinePlasmaPose AFoundrylinePlasmaTable::EvaluateCycle(double Elapsed){return ToolCycle(FMath::Fmod(FMath::Max(0.,Elapsed),29.),20,40);}
FFoundrylinePlasmaPose AFoundrylinePlasmaTable::EvaluateFactoryCycle(float Phase,EFoundrylinePart Part){return ToolCycle(FMath::Clamp(double(Phase),0.,1.)*28.99999,Part==EFoundrylinePart::RailBlank?30:Part==EFoundrylinePart::ShellBlank?123.05:25,Part==EFoundrylinePart::RailBlank?304.8:Part==EFoundrylinePart::ShellBlank?138.95:35);}
void AFoundrylinePlasmaTable::Tick(float DeltaSeconds){
 Super::Tick(DeltaSeconds);
 const auto Pose=bFactoryAutomation?(FactoryHasWork?EvaluateFactoryCycle(FactoryPhase,FactoryPart):FFoundrylinePlasmaPose{AuthoredTorchTip()+FVector(0,0,4),false}):Cycle.bOperating?EvaluateCycle(ServerTime()-Cycle.Epoch):FFoundrylinePlasmaPose{AuthoredTorchTip()+FVector(0,0,4),false};
 if(bFactoryAutomation&&StockSheet)StockSheet->SetVisibility(FactoryHasStock);
 const FVector Delta=Pose.Tip-AuthoredTorchTip();
 Gantry->SetRelativeLocation(FVector(0,Delta.Y,0));Carriage->SetRelativeLocation(FVector(Delta.X,0,0));Torch->SetRelativeLocation(FVector(0,0,Delta.Z));
 // Flexible hoses follow both axes; the supply end stays anchored to the power unit.
 const FVector TorchLead[]={Pose.Tip+FVector(0,0,41.05),Pose.Tip+FVector(0,-4,83),FVector(-130,-80+Delta.Y,195),FVector(-174.2,-91.15+Delta.Y,167)};
 const FVector SupplyLead[]={FVector(-218.2,-119.96,40),FVector(-205,-90,4),FVector(-195,-80+Delta.Y,90),TorchLead[3]};
 for(int32 Group=0;Group<2;++Group){const FVector* Points=Group==0?TorchLead:SupplyLead;
  for(int32 I=0;I<3;++I)if(Leads.IsValidIndex(Group*3+I)){
   const FVector StartTangent=I==0?Points[1]-Points[0]:(Points[I+1]-Points[I-1])*.5;
   const FVector EndTangent=I==2?Points[3]-Points[2]:(Points[I+2]-Points[I])*.5;
   Leads[Group*3+I]->SetStartAndEnd(Points[I],StartTangent,Points[I+1],EndTangent);
  }
 }
 bCutting=(bFactoryAutomation?FactoryActive:Cycle.bOperating)&&Pose.bCutting;PlasmaEffect->SetLocalPlasmaActive(bCutting);
}
FVector AFoundrylinePlasmaTable::GetTorchTip()const{return PlasmaEffect->GetComponentLocation();}
