#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylineMachineMotion.h"
#include "Components/InstancedStaticMeshComponent.h"
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
 Add(TEXT("CableChain"),Gantry)->SetVisibility(false);
 if(GetNetMode()!=NM_DedicatedServer){EnergyChain=NewObject<UInstancedStaticMeshComponent>(this,TEXT("ArticulatedEnergyChain"));EnergyChain->SetupAttachment(Gantry);EnergyChain->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));EnergyChain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Rubber.M_MH_Rubber")));EnergyChain->SetMobility(EComponentMobility::Movable);EnergyChain->SetCollisionEnabled(ECollisionEnabled::NoCollision);EnergyChain->SetCanEverAffectNavigation(false);EnergyChain->RegisterComponent();AddInstanceComponent(EnergyChain);for(int I=0;I<64;++I)EnergyChain->AddInstance(FTransform::Identity);}
 StockSheet=Add(TEXT("StockSheet"),RootComponent,true);
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
void AFoundrylinePlasmaTable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylinePlasmaTable,BusinessCompanyId);DOREPLIFETIME(AFoundrylinePlasmaTable,BusinessPlantId);DOREPLIFETIME(AFoundrylinePlasmaTable,MachineId);DOREPLIFETIME(AFoundrylinePlasmaTable,Cycle);DOREPLIFETIME(AFoundrylinePlasmaTable,bFactoryAutomation);}
namespace {
 FFoundrylinePlasmaPose ToolCycle(double T,float W,float L){
  FFoundrylinePlasmaPose P;const FVector Park=AFoundrylinePlasmaTable::AuthoredTorchTip()+FVector(0,0,4),Start(-W/2,-L/2,100.95);
  const auto Ease=[](double X){return FFoundrylineMachineMotion::Ease(float(X));};
  if(T<4){P.Tip=FMath::Lerp(Park,Start+FVector(0,0,4),Ease(T/4));return P;}
  if(T<4.8){P.Tip=Start+FVector(0,0,4*(1-Ease((T-4)/.8)));return P;}
  if(T<5.25){P.Tip=Start;P.bCutting=true;return P;} // Pierce dwell at the real nozzle.
  if(T<24.75){const double U=(T-5.25)/4.875;const int Side=FMath::Clamp(FMath::FloorToInt(U),0,3);const double F=Ease(U-Side);P.bCutting=true;
   P.Tip=Side==0?Start+FVector(W*F,0,0):Side==1?Start+FVector(W,L*F,0):Side==2?Start+FVector(W*(1-F),L,0):Start+FVector(0,L*(1-F),0);return P;}
  P.Tip=T<25.75?Start+FVector(0,0,4*Ease(T-24.75)):FMath::Lerp(Start+FVector(0,0,4),Park,Ease((T-25.75)/3.25));return P;
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
 // Constant-length articulated return chain: fixed supply, folded loop and carriage end.
 if(EnergyChain&&!FMath::IsNearlyEqual(LastChainX,float(Pose.Tip.X),.001f)){LastChainX=Pose.Tip.X;constexpr double Length=300,Radius=8,Fixed=-170;const double Fold=(Fixed+Pose.Tip.X+Length-PI*Radius)/2,Run=Fold-Fixed;
  for(int I=0;I<64;++I){const double D=(I+.5)*Length/64;FVector Point;double Pitch=0;if(D<Run)Point={Fixed+D,-91.15,167};else if(D<Run+PI*Radius){const double A=(D-Run)/Radius;Point={Fold+Radius*FMath::Sin(A),-91.15,175-Radius*FMath::Cos(A)};Pitch=FMath::RadiansToDegrees(A);}else{Point={Fold-(D-Run-PI*Radius),-91.15,183};Pitch=180;}EnergyChain->UpdateInstanceTransform(I,FTransform(FRotator(Pitch,0,0),Point,FVector(.043,.065,.035)),false,I==63,true);}
 }
 // Flexible hoses follow both axes; the supply end stays anchored to the power unit.
 const FVector TorchLead[]={Pose.Tip+FVector(0,0,41.05),Pose.Tip+FVector(0,-4,83),FVector(Pose.Tip.X,-80+Delta.Y,195),FVector(Pose.Tip.X,-91.15+Delta.Y,183)};
 const FVector SupplyLead[]={FVector(-218.2,-119.96,40),FVector(-205,-90,4),FVector(-195,-80+Delta.Y,90),FVector(-170,-91.15+Delta.Y,167)};
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
