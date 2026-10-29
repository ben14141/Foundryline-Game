#include "Construction/FoundrylineEquipmentFootprintComponent.h"
#include "Construction/FoundrylinePlaceableDefinition.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "UI/FoundrylinePlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/FoundrylineUnits.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

UFoundrylineEquipmentFootprintComponent::UFoundrylineEquipmentFootprintComponent(){bAutoActivate=true;PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=true;PrimaryComponentTick.TickInterval=.25f;}
void UFoundrylineEquipmentFootprintComponent::BeginPlay(){
 Super::BeginPlay();Definition=LoadObject<UFoundrylinePlaceableDefinition>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Production/Equipment/Placement/DA_%s.DA_%s"),*EquipmentId.ToString(),*EquipmentId.ToString()));
 if(!Definition){SetComponentTickEnabled(false);return;}
 if(auto* Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>())Placement->RegisterDefinition(Definition);
 if(GetNetMode()==NM_DedicatedServer){SetComponentTickEnabled(false);return;}
 auto Make=[&](const TCHAR* Material){auto* M=NewObject<UInstancedStaticMeshComponent>(GetOwner());M->SetupAttachment(this);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);M->SetCastShadow(false);M->SetVisibility(false);M->RegisterComponent();GetOwner()->AddInstanceComponent(M);return M;};
 Grid=Make(TEXT("/Game/Foundryline/Production/Equipment/Placement/MI_EquipmentGrid.MI_EquipmentGrid"));Boundary=Make(TEXT("/Game/Foundryline/Production/Equipment/Placement/MI_EquipmentBoundary.MI_EquipmentBoundary"));
 auto Line=[](UInstancedStaticMeshComponent* Mesh,FVector A,FVector B,float Width){FVector D=B-A;Mesh->AddInstance(FTransform(D.ToOrientationQuat(),(A+B)*.5,FVector(D.Size()/100,Width/100,.01)));};
 const auto& Work=Definition->WorkingClearance;const auto& Solid=Definition->Footprint;
 for(int32 X=FMath::CeilToInt(Work.Min.X/100.f)*100;X<Work.Max.X;X+=100)Line(Grid,FVector(X,Work.Min.Y,3),FVector(X,Work.Max.Y,3),.8);
 for(int32 Y=FMath::CeilToInt(Work.Min.Y/100.f)*100;Y<Work.Max.Y;Y+=100)Line(Grid,FVector(Work.Min.X,Y,3),FVector(Work.Max.X,Y,3),.8);
 auto Rect=[&](UInstancedStaticMeshComponent* Mesh,const FFoundrylineGridBounds& B,float Width){
  const FVector P[]={FVector(B.Min.X,B.Min.Y,4),FVector(B.Max.X,B.Min.Y,4),FVector(B.Max.X,B.Max.Y,4),FVector(B.Min.X,B.Max.Y,4)};
  for(int I=0;I<4;++I)Line(Mesh,P[I],P[(I+1)%4],Width);
 };
 Rect(Grid,Solid,2);Rect(Boundary,Work,3);
 Label=NewObject<UTextRenderComponent>(GetOwner());Label->SetupAttachment(this);Label->SetText(FText::FromString(FFoundrylineUnits::Display(FString::Printf(TEXT("%s  |  %.1f x %.1f m WORK AREA"),*EquipmentId.ToString(),(Work.Max.X-Work.Min.X)/100.f,(Work.Max.Y-Work.Min.Y)/100.f))));Label->SetWorldSize(15);Label->SetTextRenderColor(FColor(100,210,255));Label->SetRelativeLocationAndRotation(FVector(Work.Min.X+20,Work.Min.Y+25,5),FRotator(90,0,0));Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);Label->SetVisibility(false);Label->RegisterComponent();GetOwner()->AddInstanceComponent(Label);
 SetComponentTickEnabled(true);
}
void UFoundrylineEquipmentFootprintComponent::ShowPlacementArea(bool Show){bRequested=Show;if(!Show){if(Grid)Grid->SetVisibility(false);if(Boundary)Boundary->SetVisibility(false);if(Label)Label->SetVisibility(false);}}
void UFoundrylineEquipmentFootprintComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function){
 Super::TickComponent(Dt,Type,Function);auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
 auto* Player=Cast<AFoundrylinePlayerController>(UGameplayStatics::GetPlayerController(this,0));
 const bool Show=(bRequested||(Player&&Player->bShowPlanningGrid))&&(!Player||!Player->IsWalking())&&Camera&&Camera->GetCameraLocation().Z>GetComponentLocation().Z+300&&FVector::DistSquared(Camera->GetCameraLocation(),GetComponentLocation())<FMath::Square(6000.f);
 if(Grid)Grid->SetVisibility(Show);if(Boundary)Boundary->SetVisibility(Show);if(Label)Label->SetVisibility(Show);
}
