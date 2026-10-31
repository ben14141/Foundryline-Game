#include "UI/FoundrylinePlayerController.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace {
// Employee roots are presentation foot roots, including the actual forklift attachment.
// Sample torso/head positions rather than the floor or the forklift's simulation origin.
bool EmployeeAim(FVector Eye,FVector Forward,const UFoundrylineEmployeeVisualComponent* Visual,bool Seated,FVector& Target,double& Score){
 bool Found=false;const double Bottom=Seated?155.:85.,Top=Seated?235.:178.;
 for(int Sample=0;Sample<7;++Sample){
  const FVector Point=Visual->GetComponentTransform().TransformPosition(FVector(0,0,FMath::Lerp(Bottom,Top,Sample/6.)));
  const FVector Delta=Point-Eye;const double Distance=Delta.Size();if(Distance<1||Distance>250)continue;
  const double Alignment=FVector::DotProduct(Delta/Distance,Forward);if(Alignment<.965)continue;
  const double Candidate=(1-Alignment)*1000+Distance*.025;
  if(Candidate<Score){Score=Candidate;Target=Point;Found=true;}
 }
 return Found;
}
bool BelongsToEmployee(const USceneComponent* Component,const UFoundrylineEmployeeVisualComponent* Employee){
 for(auto* Parent=Component;Parent;Parent=Parent->GetAttachParent())if(Parent==Employee)return true;
 return false;
}
}

int32 AFoundrylinePlayerController::FocusedEmployeeId() const {
 if(!IsLocalController()||!IsWalking()||!PlayerCameraManager||OperationsScreen||FactoryMachine)return 0;
 auto* Plant=FactoryPlant();if(!Plant||!GetPawn())return 0;
 const auto Staff=FFoundrylineEmployeeRoster::Build(Plant->Snapshot);
 TInlineComponentArray<UFoundrylineEmployeeVisualComponent*> Visuals;Plant->GetComponents(Visuals);
 const FVector Eye=PlayerCameraManager->GetCameraLocation(),Forward=PlayerCameraManager->GetCameraRotation().Vector();
 int32 Selected=0;double Best=MAX_dbl;
 for(const auto& Employee:Staff){
  if(Employee.CompanyId!=Plant->BusinessCompanyId||Employee.PlantId!=Plant->BusinessPlantId)continue;
  const FName Identity(*Employee.Key);
  const auto* Entry=Visuals.FindByPredicate([&](const auto* V){return IsValid(V)&&V->IsRegistered()&&V->IsVisible()&&V->ComponentTags.Contains(Identity);});
  if(!Entry)continue;const auto* Visual=*Entry;FVector Target;double Candidate=Best;
  if(!EmployeeAim(Eye,Forward,Visual,Employee.bSeated,Target,Candidate))continue;
  FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundrylineEmployeeUse),false,GetPawn());
  // Do not ignore the whole plant: its racks and machinery must still block sight.
  if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_Visibility,Query)&&Hit.Distance+5<FVector::Distance(Eye,Target)&&!BelongsToEmployee(Hit.GetComponent(),Visual))continue;
  Best=Candidate;Selected=Employee.Id;
 }
 return Selected;
}

bool AFoundrylinePlayerController::InspectFocusedEmployee(){
 const int32 Id=FocusedEmployeeId();auto* Plant=FactoryPlant();if(!Id||!Plant)return false;
 const auto Staff=FFoundrylineEmployeeRoster::Build(Plant->Snapshot);
 const auto* Employee=Staff.FindByPredicate([&](const auto& E){return E.Id==Id&&E.CompanyId==Plant->BusinessCompanyId&&E.PlantId==Plant->BusinessPlantId;});
 if(!Employee)return false;
 // OpenOperations performs the normal cursor, look/movement lock and close handling.
 OpenOperations(4);if(OperationsScreen!=4)return false;
 EmployeeInspectId=Id;EmployeeInspectPlantId=Plant->BusinessPlantId;
 OperationsPage=7;OperationsWorkspaceTab=0;OperationsEmployeeChoice=Id;OperationsJobPage=OperationsListPage=0;
 OperationsJobRole=Employee->Role==EFoundrylineEmployeeRole::PartsCollector?4:int(Employee->Role)<=2?int(Employee->Role):0;
 OperationsStatus.Reset();return true;
}

bool AFoundrylinePlayerController::IsInspectedEmployeeAvailable() const {
 auto* Plant=FactoryPlant();if(!Plant||!EmployeeInspectId||EmployeeInspectPlantId!=Plant->BusinessPlantId)return false;
 return FFoundrylineEmployeeRoster::Build(Plant->Snapshot).ContainsByPredicate([&](const auto& E){return E.Id==EmployeeInspectId&&E.CompanyId==Plant->BusinessCompanyId&&E.PlantId==Plant->BusinessPlantId;});
}
