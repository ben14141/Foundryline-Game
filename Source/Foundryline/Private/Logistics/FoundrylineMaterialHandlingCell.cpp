#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Logistics/FoundrylineForkliftBeacon.h"
#include "Production/FoundrylineMachineMotion.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Construction/FoundrylineEquipmentFootprintComponent.h"
#include "Construction/FoundrylineEquipmentCollision.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
namespace {
 constexpr float PalletHeight=17.5568f;
 const FVector RackCenter(-550,400,0),RollerCenter(300,-550,0),RollerInput(70,-550,0),MastPivot(28,0,16);
 bool Overlap2D(FVector A,FVector2D Half,float Yaw,FVector B,FVector2D Other) {
  const float C=FMath::Cos(FMath::DegreesToRadians(Yaw)),S=FMath::Sin(FMath::DegreesToRadians(Yaw));const FVector2D D(B.X-A.X,B.Y-A.Y);
  return FMath::Abs(D.X)<FMath::Abs(C)*Half.X+FMath::Abs(S)*Half.Y+Other.X && FMath::Abs(D.Y)<FMath::Abs(S)*Half.X+FMath::Abs(C)*Half.Y+Other.Y
   && FMath::Abs(D.X*C+D.Y*S)<Half.X+Other.X*FMath::Abs(C)+Other.Y*FMath::Abs(S)
   && FMath::Abs(-D.X*S+D.Y*C)<Half.Y+Other.X*FMath::Abs(S)+Other.Y*FMath::Abs(C);
 }
}
AFoundrylineMaterialHandlingCell::AFoundrylineMaterialHandlingCell(){
 bReplicates=true;SetNetUpdateFrequency(15);SetNetCullDistanceSquared(FMath::Square(40000.f));PrimaryActorTick.bCanEverTick=true;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Cell")));
 auto* Footprint=CreateDefaultSubobject<UFoundrylineEquipmentFootprintComponent>(TEXT("RollerPlacementArea"));Footprint->SetupAttachment(RootComponent);Footprint->SetRelativeLocation(RollerCenter);Footprint->EquipmentId=TEXT("SheetRoller");
 ForkRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Forklift"));ForkRoot->SetupAttachment(RootComponent);
 MastRoot=CreateDefaultSubobject<USceneComponent>(TEXT("MastTilt"));MastRoot->SetupAttachment(ForkRoot);MastRoot->SetRelativeLocation(MastPivot);
 InnerMastRoot=CreateDefaultSubobject<USceneComponent>(TEXT("InnerMast"));InnerMastRoot->SetupAttachment(MastRoot);
 CarriageRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Carriage"));CarriageRoot->SetupAttachment(MastRoot);
 auto* Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("ForkliftCollision"));Collision->SetupAttachment(ForkRoot);Collision->SetRelativeLocation({-95,0,135});Collision->SetBoxExtent({145,73,130});Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}
void AFoundrylineMaterialHandlingCell::BeginPlay(){
 Super::BeginPlay();
 auto Mesh=[&](const TCHAR* Name,const TCHAR* Path,USceneComponent* Parent,FVector Location){auto* M=NewObject<UStaticMeshComponent>(this,FName(Name));M->SetMobility(EComponentMobility::Movable);M->SetupAttachment(Parent);M->SetRelativeLocation(Location);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->RegisterComponent();AddInstanceComponent(M);return M;};
 Mesh(TEXT("Forklift_Body"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Body.SM_Forklift_Body"),ForkRoot,{0.0f,0.0f,0.0f});FFoundrylineForkliftBeacon::Attach(this,ForkRoot);
 Mesh(TEXT("Forklift_MastOuter"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_MastOuter.SM_Forklift_MastOuter"),MastRoot,{0.0f,0.0f,0.0f});
 Mesh(TEXT("Forklift_MastInner"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_MastInner.SM_Forklift_MastInner"),InnerMastRoot,{0.0f,0.0f,0.0f});
 Mesh(TEXT("Forklift_Carriage"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Carriage.SM_Forklift_Carriage"),CarriageRoot,{0.0f,0.0f,0.0f});
 Wheels.Add(Mesh(TEXT("Forklift_Wheel_FL"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Wheel_FL.SM_Forklift_Wheel_FL"),ForkRoot,{-0.0079f,59.59131f,41.83884f}));Radius.Add(41.0621f);Steers.Add(false);
 Wheels.Add(Mesh(TEXT("Forklift_Wheel_RL"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Wheel_RL.SM_Forklift_Wheel_RL"),ForkRoot,{-196.671f,56.81446f,35.65904f}));Radius.Add(34.7428f);Steers.Add(true);
 Wheels.Add(Mesh(TEXT("Forklift_Wheel_FR"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Wheel_FR.SM_Forklift_Wheel_FR"),ForkRoot,{0.04091f,-59.59131f,41.7871f}));Radius.Add(41.0622f);Steers.Add(false);
 Wheels.Add(Mesh(TEXT("Forklift_Wheel_RR"),TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Wheel_RR.SM_Forklift_Wheel_RR"),ForkRoot,{-197.33359f,-58.44237f,34.75395f}));Radius.Add(34.7429f);Steers.Add(true);
 auto* RollerBody=Mesh(TEXT("SheetRoller_Body"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_Body.SM_SheetRoller_Body"),RootComponent,RollerCenter+FVector(0.0f,0.0f,0.0f));FFoundrylineEquipmentCollision::AddPawnBody(RollerBody);
 auto* Console=Mesh(TEXT("SheetRoller_OperatorConsole"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_OperatorConsole.SM_SheetRoller_OperatorConsole"),RootComponent,RollerCenter);FFoundrylineEquipmentCollision::AddPawnBody(Console);
 UpperCarriers=Mesh(TEXT("SheetRoller_UpperCarriers"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_UpperCarriers.SM_SheetRoller_UpperCarriers"),RootComponent,RollerCenter+FVector(0,0,127.4));
 Rollers.Add(Mesh(TEXT("SheetRoller_LowerFront"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_LowerFront.SM_SheetRoller_LowerFront"),RootComponent,RollerCenter+FVector(-22.0f,0.0f,92.0f)));
 Rollers.Add(Mesh(TEXT("SheetRoller_LowerRear"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_LowerRear.SM_SheetRoller_LowerRear"),RootComponent,RollerCenter+FVector(22.0f,0.0f,92.0f)));
 Rollers.Add(Mesh(TEXT("SheetRoller_Upper"),TEXT("/Game/Foundryline/Production/Equipment/SheetRoller/SM_SheetRoller_Upper.SM_SheetRoller_Upper"),RootComponent,RollerCenter+FVector(0.0f,0.0f,127.4f)));
 if(!bFactoryAutomation){for(int32 End:{-1,1})Mesh(*FString::Printf(TEXT("RackUpright%d"),End),TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Upright.SM_Rack_Upright"),RootComponent,GetRackCenter()+FVector(0,End*201.735,0));
 for(int32 Level=0;Level<3;++Level){
  const FVector P=GetRackPosition(Level)-FVector(0,0,20);
  Mesh(*FString::Printf(TEXT("RackShelf%d"),Level),TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Shelf400.SM_Rack_Shelf400"),RootComponent,P);
  auto* C=NewObject<UBoxComponent>(this);C->SetupAttachment(RootComponent);C->SetRelativeLocation(P+FVector(0,0,10));C->SetBoxExtent({55.8,201.7,10});C->SetCollisionProfileName(TEXT("BlockAllDynamic"));C->RegisterComponent();AddInstanceComponent(C);
 }
 }
 for(int32 I=0;I<2;++I){
  auto* Root=NewObject<USceneComponent>(this,FName(*FString::Printf(TEXT("Stock%d"),I)));Root->SetupAttachment(RootComponent);Root->RegisterComponent();AddInstanceComponent(Root);StockRoots.Add(Root);
  for(int32 Side:{-1,1})Mesh(*FString::Printf(TEXT("Pallet%d_%d"),I,Side),TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood.SM_Pallet_Wood"),Root,{0,Side*65.f,0});
  auto* Plates=NewObject<UInstancedStaticMeshComponent>(this);Plates->SetupAttachment(Root);Plates->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Materials/Steel/SM_SteelSheet_60x120.SM_SteelSheet_60x120")));Plates->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plates->RegisterComponent();AddInstanceComponent(Plates);Sheets.Add(Plates);DrawnCounts.Add(-1);
 }
 if(!bFactoryAutomation)for(int32 Side:{-1,1}){
  auto* Post=NewObject<UBoxComponent>(this);Post->SetupAttachment(RootComponent);Post->SetRelativeLocation(GetRackCenter()+FVector(0,Side*201.735,210));Post->SetBoxExtent({57.5,6.3,210});Post->SetCollisionProfileName(TEXT("BlockAllDynamic"));Post->SetCanEverAffectNavigation(false);Post->RegisterComponent();AddInstanceComponent(Post);
 }
 auto* MachineCollision=NewObject<UBoxComponent>(this);MachineCollision->SetupAttachment(RootComponent);MachineCollision->SetRelativeLocation(RollerCenter+FVector(45,0,85));MachineCollision->SetBoxExtent({110,263,85});MachineCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));MachineCollision->SetCanEverAffectNavigation(false);MachineCollision->RegisterComponent();AddInstanceComponent(MachineCollision);
 FeedSheet=Mesh(TEXT("ProcessingSheet"),TEXT("/Game/Foundryline/Inventory/Materials/Steel/SM_SteelSheet_60x120.SM_SteelSheet_60x120"),RootComponent,RollerInput+FVector(0,0,115));FeedSheet->SetRelativeRotation(FRotator(0,90,0));
 CurvedOutput=Mesh(TEXT("RolledBlank"),TEXT("/Game/Foundryline/Products/Components/Steel/SM_SteelSheet_RolledBlank.SM_SteelSheet_RolledBlank"),RootComponent,RollerCenter+FVector(170,0,25));
 if(bFactoryAutomation){
  FormingSheet=NewObject<UInstancedStaticMeshComponent>(this,TEXT("FormingShellBlank"));FormingSheet->SetMobility(EComponentMobility::Movable);FormingSheet->SetupAttachment(RootComponent);FormingSheet->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));FormingSheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);FormingSheet->SetCanEverAffectNavigation(false);
  if(auto* Steel=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Products/Components/Fabrication/SM_ShellBlank.SM_ShellBlank")))FormingSheet->SetMaterial(0,Steel->GetMaterial(0));
  FormingSheet->RegisterComponent();AddInstanceComponent(FormingSheet);for(int I=0;I<32;++I)FormingSheet->AddInstance(FTransform::Identity);FormingSheet->SetVisibility(false);
 }
 auto* Stand=Mesh(TEXT("OutputSkid"),TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood.SM_Pallet_Wood"),RootComponent,RollerCenter+FVector(210,0,0));
 if(HasAuthority()&&!bFactoryAutomation) {
  FFoundrylineSheetStock A;A.Id=FGuid::NewGuid();A.FloorPosition=ArrivalPositionCm;FFoundrylineSheetStock B;B.Id=FGuid::NewGuid();B.Place=EFoundrylineSheetPlace::Rack;B.RackLevel=1;Stock={A,B};
  Feedback=TEXT("WASD drive; Q/E lift. Space picks or drops a load. Rack keys 1/2/3 store it.");
 }
 PreviousPose=DisplayPose=Pose;RebuildStock();UpdateVisuals(0);
}
void AFoundrylineMaterialHandlingCell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{
 DOREPLIFETIME(AFoundrylineMaterialHandlingCell,bFactoryAutomation);
 DOREPLIFETIME(AFoundrylineMaterialHandlingCell,RackOffsetCm);
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,Pose);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,Stock);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,BusinessCompanyId);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,BusinessPlantId);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,Driver);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,Feedback);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,Carried);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,TransferStock);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,TransferAction);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,RollerRemaining);DOREPLIFETIME(AFoundrylineMaterialHandlingCell,RolledBlanks);
}
FVector AFoundrylineMaterialHandlingCell::RackPosition(int32 Level){return RackCenter+FVector(0,0,Level==0?40:Level==1?170:300);}
FVector AFoundrylineMaterialHandlingCell::GetRackCenter()const{return RackCenter+RackOffsetCm;}
FVector AFoundrylineMaterialHandlingCell::GetRackPosition(int32 Level)const{return RackPosition(Level)+RackOffsetCm;}
FVector AFoundrylineMaterialHandlingCell::GetRackWorldPosition(int32 Level)const{return GetActorTransform().TransformPosition(GetRackPosition(Level));}
FVector AFoundrylineMaterialHandlingCell::LoadMount(const FFoundrylineForkliftPose& P)const{return P.Position+FRotator(0,P.Yaw,0).RotateVector(MastPivot+FRotator(P.Tilt,0,0).RotateVector(FVector(136,0,P.Lift)-MastPivot));}
FVector AFoundrylineMaterialHandlingCell::LoadPosition(int32 I)const{
 if(!Stock.IsValidIndex(I))return FVector::ZeroVector;const auto& S=Stock[I];
 return S.Place==EFoundrylineSheetPlace::Carried?LoadMount(Pose):S.Place==EFoundrylineSheetPlace::Rack?GetRackPosition(S.RackLevel):S.Place==EFoundrylineSheetPlace::RollerInput?RollerInput:S.FloorPosition;
}
FVector AFoundrylineMaterialHandlingCell::ForkliftWorldPosition()const{return ForkRoot->GetComponentLocation();}
int32 AFoundrylineMaterialHandlingCell::TotalSheets()const{int32 N=0;for(const auto& S:Stock)N+=S.Count;return N;}
int32 AFoundrylineMaterialHandlingCell::VisibleSheetCount(int32 I)const{return Sheets.IsValidIndex(I)?Sheets[I]->GetInstanceCount():0;}
void AFoundrylineMaterialHandlingCell::OnRep_Stock(){RebuildStock();}
void AFoundrylineMaterialHandlingCell::RebuildStock(){
 for(int32 I=0;I<Sheets.Num() && I<Stock.Num();++I){if(DrawnCounts[I]==Stock[I].Count)continue;DrawnCounts[I]=Stock[I].Count;Sheets[I]->ClearInstances();
  for(int32 Index:FFoundrylineSteelSheet::VisibleIndices(Stock[I].Count))Sheets[I]->AddInstance(FTransform(FVector(0,0,PalletHeight+(Index+.5)*FFoundrylineSteelSheet::ThicknessCm)));
 }
}
bool AFoundrylineMaterialHandlingCell::Claim(APlayerController* P,bool Take){
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&Take&&!R->CanOperate(P,BusinessCompanyId,BusinessPlantId)){return false;}

 if(bFactoryAutomation)return false;
 if(!HasAuthority() || !P || P->GetWorld()!=GetWorld() || !P->PlayerState)return false;
 if(Take){if(IsValid(Driver) && Driver!=P->PlayerState)return false;Driver=P->PlayerState;}
 else{if(Driver!=P->PlayerState)return false;Driver=nullptr;ThrottleInput=SteerInput=LiftInput=0;}
 ForceNetUpdate();return true;
}
void AFoundrylineMaterialHandlingCell::Drive(APlayerController* P,float T,float S,float L){
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&!R->CanOperate(P,BusinessCompanyId,BusinessPlantId)){return;}

 if(!HasAuthority() || !P || !Driver || Driver!=P->PlayerState || !FMath::IsFinite(T) || !FMath::IsFinite(S) || !FMath::IsFinite(L))return;
 ThrottleInput=FMath::Clamp(T,-1.f,1.f);SteerInput=FMath::Clamp(S,-1.f,1.f);LiftInput=FMath::Clamp(L,-1.f,1.f);InputAge=0;
}
bool AFoundrylineMaterialHandlingCell::Command(APlayerController* P,uint8 A){
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&!R->CanOperate(P,BusinessCompanyId,BusinessPlantId)){return false;}

 if(!HasAuthority() || !P || !Driver || Driver!=P->PlayerState)return false;
 const bool Result=A==1?(Carried==INDEX_NONE?PickNearest():PlaceCarried(EFoundrylineSheetPlace::Floor)):A>=2 && A<=4?PlaceCarried(EFoundrylineSheetPlace::Rack,A-2):A==5?PlaceCarried(EFoundrylineSheetPlace::RollerInput):A==6?StartRoller():false;ForceNetUpdate();return Result;
}
bool AFoundrylineMaterialHandlingCell::PickNearest(){
 if(!HasAuthority() || TransferAction || Carried!=INDEX_NONE)return false;
 int32 Best=INDEX_NONE;double Distance=150;
 for(int32 I=0;I<Stock.Num();++I){if(Stock[I].Place==EFoundrylineSheetPlace::Carried)continue;const double Horizontal=FVector::Dist2D(LoadMount(Pose),LoadPosition(I));const double D=Horizontal+FMath::Abs(LoadMount(Pose).Z-LoadPosition(I).Z)*.2;if(Horizontal<150 && D<Distance){Distance=D;Best=I;}}
 if(Best==INDEX_NONE){Feedback=TEXT("Drive the forks closer to a load, then press Space.");return false;}
 if(Stock[Best].Pounds()+80>3200){Feedback=TEXT("This load exceeds the forklift's game capacity.");return false;}
 TransferStock=Best;TransferAction=1;TargetLift=LoadPosition(Best).Z;Feedback=TEXT("Aligning fork height and collecting the load.");return true;
}
bool AFoundrylineMaterialHandlingCell::PlaceCarried(EFoundrylineSheetPlace Place,int32 Level){
 if(!HasAuthority() || TransferAction || !Stock.IsValidIndex(Carried) || (Place==EFoundrylineSheetPlace::Rack && (Level<0 || Level>2)))return false;
 const FVector Target=Place==EFoundrylineSheetPlace::Rack?GetRackPosition(Level):Place==EFoundrylineSheetPlace::RollerInput?RollerInput:LoadMount(Pose)*FVector(1,1,0);
 if(FVector::Dist2D(LoadMount(Pose),Target)>150){Feedback=TEXT("Drive closer to the selected rack or roller input.");return false;}
 for(int32 I=0;I<Stock.Num();++I)if(I!=Carried){
  if(Place==EFoundrylineSheetPlace::Rack && Stock[I].Place==Place && Stock[I].RackLevel==Level){Feedback=TEXT("That rack level already holds a load.");return false;}
  if(Place==EFoundrylineSheetPlace::RollerInput && Stock[I].Place==Place){Feedback=TEXT("The roller input already holds a load.");return false;}
  if(Place==EFoundrylineSheetPlace::Floor && Stock[I].Place==Place && FVector::Dist2D(LoadPosition(I),Target)<250){Feedback=TEXT("Leave more room between floor loads.");return false;}
 }
 TargetPlace=Place;TargetLevel=Level;TargetLift=Target.Z;TransferStock=Carried;TransferAction=2;Feedback=TEXT("Lowering or raising the load into its storage position.");return true;
}
bool AFoundrylineMaterialHandlingCell::StartRoller(){
 if(!HasAuthority() || RollerRemaining>0 || TransferAction)return false;
 for(auto& S:Stock)if(S.Place==EFoundrylineSheetPlace::RollerInput && S.Consume(1)){
  RollerRemaining=6;Feedback=TEXT("Roller demonstration: one sheet reserved; forming one curved blank.");RebuildStock();ForceNetUpdate();return true;
 }
 Feedback=TEXT("Put a sheet load at the roller input first (key 4).");return false;
}
bool AFoundrylineMaterialHandlingCell::ClearMove(const FFoundrylineForkliftPose& P)const{
 const FVector Center=P.Position+FRotator(0,P.Yaw,0).RotateVector({-95,0,135});
 if(Overlap2D(Center,{145,73},P.Yaw,GetRackCenter(),{57.5,208}) || Overlap2D(Center,{145,73},P.Yaw,RollerCenter,{155,263}))return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(MaterialForkliftClearance),false,this);
 const FVector World=GetActorTransform().TransformPosition(Center);const FQuat Rot=GetActorQuat()*FRotator(0,P.Yaw,0).Quaternion();
 if(GetWorld()->OverlapBlockingTestByChannel(World,Rot,ECC_Pawn,FCollisionShape::MakeBox(FVector(145,73,130)),Q))return false;
 if(Stock.IsValidIndex(Carried)){
  const FVector Load=GetActorTransform().TransformPosition(LoadMount(P)+FVector(0,0,12));
  if(GetWorld()->OverlapBlockingTestByChannel(Load,Rot,ECC_Pawn,FCollisionShape::MakeBox(FVector(76.2,152.4,12)),Q))return false;
 }
 return true;
}
void AFoundrylineMaterialHandlingCell::Step(float Dt){
 if(!HasAuthority() || Dt<=0 || Dt>.1f)return;
 InputAge+=Dt;if(InputAge>.35f || !IsValid(Driver))ThrottleInput=SteerInput=LiftInput=0;
 if(RollerRemaining>0){RollerRemaining=FMath::Max(0.f,RollerRemaining-Dt);if(RollerRemaining==0){++RolledBlanks;Feedback=TEXT("Curved blank complete. Exact stock and output counts updated.");ForceNetUpdate();}}
 float Target=ThrottleInput*(ThrottleInput<0?120:180);if(TransferAction)Target=0;else if(Pose.Lift>80)Target=FMath::Clamp(Target,-70.f,70.f);
 Pose.Speed=FMath::FInterpConstantTo(Pose.Speed,Target,Dt,Target==0?280:100);Pose.Steering=FMath::FInterpConstantTo(Pose.Steering,-SteerInput*50,Dt,70);
 if(!TransferAction)TargetLift=FMath::Clamp(Pose.Lift+LiftInput*85*Dt,0.f,330.f);
 if(FMath::Abs(Pose.Speed)<1 || !TransferAction)Pose.Lift=FMath::FInterpConstantTo(Pose.Lift,TargetLift,Dt,85);
 const float TiltTarget=TransferAction?0:Carried==INDEX_NONE?0:4;Pose.Tilt=FMath::FInterpConstantTo(Pose.Tilt,TiltTarget,Dt,14);
 if(TransferAction && FMath::Abs(Pose.Speed)<1 && FMath::Abs(Pose.Lift-TargetLift)<.5 && FMath::Abs(Pose.Tilt)<.1){
  if(Stock.IsValidIndex(TransferStock)){
   auto& S=Stock[TransferStock];
   if(TransferAction==1){S.Place=EFoundrylineSheetPlace::Carried;Carried=TransferStock;Feedback=TEXT("Load collected. Sheet count is unchanged.");}
   else{S.Place=TargetPlace;S.RackLevel=TargetLevel;S.FloorPosition=LoadMount(Pose)*FVector(1,1,0);S.FloorYaw=Pose.Yaw;Carried=INDEX_NONE;Feedback=TEXT("Load stored. Sheet count is unchanged.");}
  }
  TransferAction=0;TransferStock=INDEX_NONE;ForceNetUpdate();
 }
 if(FMath::Abs(Pose.Speed)<.01)return;
 auto P=Pose;const float D=Pose.Speed*Dt;P.Yaw-=FMath::RadiansToDegrees(D/197.f*FMath::Tan(FMath::DegreesToRadians(Pose.Steering)));P.Position+=FRotator(0,(Pose.Yaw+P.Yaw)*.5f,0).Vector()*D;
 if(!ClearMove(P)){Pose.Speed=0;Feedback=TEXT("Blocked. Reposition the forklift or lower the load.");return;}
 P.Travel+=D;Pose=P;
}
void AFoundrylineMaterialHandlingCell::UpdateVisuals(float Dt){
 float T=FMath::Clamp(Accumulator*30,0.f,1.f);
 if(HasAuthority()){
  DisplayPose=Pose;DisplayPose.Position=FMath::Lerp(PreviousPose.Position,Pose.Position,T);DisplayPose.Yaw=PreviousPose.Yaw+FMath::FindDeltaAngleDegrees(PreviousPose.Yaw,Pose.Yaw)*T;
  DisplayPose.Lift=FMath::Lerp(PreviousPose.Lift,Pose.Lift,T);DisplayPose.Tilt=FMath::Lerp(PreviousPose.Tilt,Pose.Tilt,T);DisplayPose.Travel=FMath::Lerp(PreviousPose.Travel,Pose.Travel,T);DisplayPose.Steering=FMath::Lerp(PreviousPose.Steering,Pose.Steering,T);
 }else{
  DisplayPose.Position=FMath::VInterpTo(DisplayPose.Position,Pose.Position,Dt,18);DisplayPose.Yaw=FMath::FInterpTo(DisplayPose.Yaw,DisplayPose.Yaw+FMath::FindDeltaAngleDegrees(DisplayPose.Yaw,Pose.Yaw),Dt,18);DisplayPose.Lift=FMath::FInterpTo(DisplayPose.Lift,Pose.Lift,Dt,18);DisplayPose.Tilt=FMath::FInterpTo(DisplayPose.Tilt,Pose.Tilt,Dt,18);DisplayPose.Travel=FMath::FInterpTo(DisplayPose.Travel,Pose.Travel,Dt,18);DisplayPose.Steering=FMath::FInterpTo(DisplayPose.Steering,Pose.Steering,Dt,18);
 }
 ForkRoot->SetRelativeLocationAndRotation(DisplayPose.Position,FRotator(0,DisplayPose.Yaw,0));MastRoot->SetRelativeRotation(FRotator(DisplayPose.Tilt,0,0));CarriageRoot->SetRelativeLocation({0,0,DisplayPose.Lift});InnerMastRoot->SetRelativeLocation({0,0,FMath::Max(0.f,DisplayPose.Lift-120)});
 for(int32 I=0;I<Wheels.Num();++I)Wheels[I]->SetRelativeRotation(FRotator(FMath::Fmod(-FMath::RadiansToDegrees(DisplayPose.Travel/Radius[I]),360.f),Steers[I]?DisplayPose.Steering:0,0));
 RebuildStock();
 for(int32 I=0;I<StockRoots.Num() && I<Stock.Num();++I){
  const auto& S=Stock[I];const bool Carry=S.Place==EFoundrylineSheetPlace::Carried;StockRoots[I]->SetRelativeLocation(Carry?LoadMount(DisplayPose):LoadPosition(I));
  StockRoots[I]->SetRelativeRotation(FRotator(Carry?DisplayPose.Tilt:0,Carry?DisplayPose.Yaw:S.Place==EFoundrylineSheetPlace::RollerInput?0:S.Place==EFoundrylineSheetPlace::Rack?0:S.FloorYaw,0));
 }
 const float Phase=RollerRemaining>0?1-RollerRemaining/6:0;DisplayPhase=FMath::FInterpTo(DisplayPhase,Phase,Dt,18);
 for(int32 I=0;I<Rollers.Num();++I)Rollers[I]->SetRelativeRotation(FRotator(FMath::Fmod(DisplayPhase*2160*(I==2?-12.f/17.4f:1.f),360.f),0,0));
 FeedSheet->SetVisibility(RollerRemaining>0 && DisplayPhase<.8f);FeedSheet->SetRelativeRotation(FRotator(0,FMath::Clamp(DisplayPhase*5,0.f,1.f)*90,0));FeedSheet->SetRelativeLocation(FMath::Lerp(RollerInput+FVector(-40,0,115),RollerCenter+FVector(80,0,115),DisplayPhase));CurvedOutput->SetVisibility(RolledBlanks>0);
}
void AFoundrylineMaterialHandlingCell::Tick(float Dt){Super::Tick(Dt);if(bFactoryAutomation){ForkRoot->SetVisibility(false,true);TArray<UPrimitiveComponent*> ForkComponents;GetComponents(ForkComponents);for(auto* C:ForkComponents)if(C->IsAttachedTo(ForkRoot))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);for(auto R:StockRoots)R->SetVisibility(false,true);return;}if(HasAuthority()){Accumulator+=FMath::Min(Dt,.1f);for(int32 I=0;I<4 && Accumulator>=1.f/30;++I){PreviousPose=Pose;Step(1.f/30);Accumulator-=1.f/30;}}UpdateVisuals(Dt);}
void AFoundrylineMaterialHandlingCell::SetFactoryRoller(float Phase,bool Active,bool HasWork){
 if(!bFactoryAutomation)return;const float P=FMath::Clamp(Phase,0.f,1.f);if(FMath::IsNearlyEqual(P,LastFactoryPhase,.00001f)&&LastFactoryWork==HasWork)return;LastFactoryPhase=P;LastFactoryWork=HasWork;FeedSheet->SetVisibility(false);CurvedOutput->SetVisibility(false);
 const float Feed=150*FFoundrylineMachineMotion::Ease(P);for(int I=0;I<Rollers.Num();++I)Rollers[I]->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(Feed/(I==2?-17.4f:12.f)),0,0));
 if(!FormingSheet)return;FormingSheet->SetVisibility(HasWork);if(!HasWork){if(Rollers.IsValidIndex(2))Rollers[2]->SetRelativeLocation(RollerCenter+FVector(0,0,127.4));if(UpperCarriers)UpperCarriers->SetRelativeLocation(RollerCenter+FVector(0,0,127.4));return;}
 // One real shell blank, represented by bounded thin strips while it bends. No inventory is spawned here.
 const double Length=117.5*PI/3,Width=138.95375,Thickness=.3175,Angle=FFoundrylineMachineMotion::Ease((P-.15)/.65)*PI/3,BendRadius=Angle>.0001?Length/Angle:1.e9;
 const double Sag=BendRadius-FMath::Sqrt(FMath::Max(0.,BendRadius*BendRadius-22*22)),Height=104-Sag;
 const FVector Center=RollerCenter+FVector(-85+Feed,0,Height);
 for(int I=0;I<32;++I){const double U=(I+.5)/32.-.5,A=U*Angle;const FVector Position=Center+FVector(Angle>.0001?BendRadius*FMath::Sin(A):U*Length,0,Angle>.0001?BendRadius*(1-FMath::Cos(A)):0);
  const double Chord=Angle>.0001?2*BendRadius*FMath::Sin(Angle/64):Length/32;
  FormingSheet->UpdateInstanceTransform(I,FTransform(FRotator(FMath::RadiansToDegrees(A),0,0),Position,FVector(Chord/100,Width/100,Thickness/100)),false,I==31,true);
 }
 const double LoadLift=6*(1-FMath::Min(FFoundrylineMachineMotion::Ease((P-.05)/.15),FFoundrylineMachineMotion::Ease((1-P)/.15)));
 const FVector UpperPosition=RollerCenter+FVector(0,0,Height+17.4+Thickness/2+LoadLift);if(Rollers.IsValidIndex(2))Rollers[2]->SetRelativeLocation(UpperPosition);if(UpperCarriers)UpperCarriers->SetRelativeLocation(UpperPosition);
}
