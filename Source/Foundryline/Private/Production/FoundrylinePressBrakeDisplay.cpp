#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Net/UnrealNetwork.h"
#include "Construction/FoundrylineEquipmentFootprintComponent.h"
#include "Construction/FoundrylineEquipmentCollision.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Production/FoundrylineMachineMotion.h"
AFoundrylinePressBrakeDisplay::AFoundrylinePressBrakeDisplay(){
 bReplicates=true;Machine=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PressBrake"));SetRootComponent(Machine);Machine->SetMobility(EComponentMobility::Movable);Machine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 auto* Footprint=CreateDefaultSubobject<UFoundrylineEquipmentFootprintComponent>(TEXT("PlacementArea"));Footprint->SetupAttachment(RootComponent);Footprint->EquipmentId=TEXT("PressBrake");
}
void AFoundrylinePressBrakeDisplay::BeginPlay(){Super::BeginPlay();const FString Folder=TEXT("/Game/Foundryline/Production/Equipment/PressBrake/SM_PressBrake_");Machine->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(Folder+TEXT("Body"))));FFoundrylineEquipmentCollision::AddPawnBody(Machine);
 for(const TCHAR* N:{TEXT("Ram"),TEXT("UpperPunch"),TEXT("PistonRods"),TEXT("LowerDie"),TEXT("BackGauge"),TEXT("OperatorConsole"),TEXT("FootPedal"),TEXT("ControlCables")}){auto* C=NewObject<UStaticMeshComponent>(this);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Machine);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(Folder+N)));if(!C->GetStaticMesh())continue;C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);if(FString(N)==TEXT("Ram")||FString(N)==TEXT("UpperPunch")||FString(N)==TEXT("PistonRods"))Moving.Add(C);if(FString(N)==TEXT("BackGauge"))Gauge=C;if(FString(N)==TEXT("OperatorConsole"))C->SetRelativeLocation({180,215,0});if(FString(N)==TEXT("FootPedal"))C->SetRelativeLocation({190,85,0});if(FString(N)==TEXT("OperatorConsole"))FFoundrylineEquipmentCollision::AddPawnBody(C);}
 auto Part=[&](const TCHAR* Name){auto* C=NewObject<UStaticMeshComponent>(this,Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Machine);C->SetRelativeLocation({52,0,104.1});C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetVisibility(false);return C;};FlatPart=Part(TEXT("BrakeBlank"));BentPart=Part(TEXT("BrakeBentPart"));

}
void AFoundrylinePressBrakeDisplay::SetFactoryPresentation(float Phase,bool Active,bool HasWork,EFoundrylinePart Part){
 const float P=HasWork?FMath::Clamp(Phase,0.f,1.f):0,Z=FFoundrylineMachineMotion::BrakeStroke(P);for(auto C:Moving)C->SetRelativeLocation({0,0,Z});
 if(Gauge)Gauge->SetRelativeLocation({P<.12f?FMath::Lerp(-12.f,0.f,P/.12f):P<.85f?0.f:FMath::Lerp(0.f,-12.f,(P-.85f)/.15f),0,0});
 if(!FlatPart||!BentPart)return;if(Part!=ShownPart){ShownPart=Part;const bool Rail=Part==EFoundrylinePart::FrameRail;const FString Root=TEXT("/Game/Foundryline/Products/Components/Fabrication/SM_");FlatPart->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(Root+(Rail?TEXT("RailBlank"):TEXT("BracketBlank")))));BentPart->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(Root+(Rail?TEXT("FrameRail"):TEXT("BentBracket")))));}
 FlatPart->SetVisibility(HasWork&&P<.45f);BentPart->SetVisibility(HasWork&&P>=.45f);const float Feed=FMath::Clamp(P/.12f,0.f,1.f);FlatPart->SetRelativeLocation({FMath::Lerp(80.f,52.f,Feed),0,104.1});
}

void AFoundrylinePressBrakeDisplay::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylinePressBrakeDisplay,BusinessCompanyId);DOREPLIFETIME(AFoundrylinePressBrakeDisplay,BusinessPlantId);}
