#include "Production/FoundrylineFactoryPlant.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Engine/World.h"

void AFoundrylineFactoryPlant::UpdateEmployees(float Dt){
 if(bRebuild||CachedEmployees.IsEmpty()){
  CachedEmployees=FFoundrylineEmployeeRoster::Build(Snapshot);
  TSet<FString> Keys;for(const auto& E:CachedEmployees)Keys.Add(E.Key);
  for(auto It=EmployeeVisuals.CreateIterator();It;++It)if(!Keys.Contains(It.Key())){if(auto* V=It.Value().Get()){TArray<USceneComponent*> Descendants;V->GetChildrenComponents(true,Descendants);for(int I=Descendants.Num()-1;I>=0;--I)Descendants[I]->DestroyComponent();V->DestroyComponent();}EmployeeDisplayPositions.Remove(It.Key());EmployeeDisplayYaw.Remove(It.Key());EmployeeDisplayDistance.Remove(It.Key());It.RemoveCurrent();}
 }
 for(const auto& E:CachedEmployees){
  if(E.CompanyId!=BusinessCompanyId||E.PlantId!=BusinessPlantId)continue;
  auto& Entry=EmployeeVisuals.FindOrAdd(E.Key);if(!Entry){Entry=NewObject<UFoundrylineEmployeeVisualComponent>(this);Entry->SetupAttachment(RootComponent);Entry->RegisterComponent();AddInstanceComponent(Entry);Entry->Appearance=EmployeeAppearance;Entry->InitializeVisual();}
  auto* V=Entry.Get();auto Display=E;USceneComponent* Parent=RootComponent;FVector Position=E.Position;float Yaw=E.Yaw;
  if(E.bSeated){int DriverIndex=Snapshot.Drivers.IndexOfByPredicate([&](const auto& D){return D.Id==E.VehicleId;});if(Forks.IsValidIndex(DriverIndex)){Parent=Forks[DriverIndex];Position=UFoundrylineEmployeeVisualComponent::ForkliftSeatOffset();Yaw=0;}else Position+=FRotator(0,Yaw,0).RotateVector(UFoundrylineEmployeeVisualComponent::ForkliftSeatOffset());}
  else{auto& P=EmployeeDisplayPositions.FindOrAdd(E.Key,E.Position);const FVector Previous=P;P=FMath::VInterpTo(P,E.Position,Dt,10);const float Travel=FVector::Dist2D(P,Previous);auto& Distance=EmployeeDisplayDistance.FindOrAdd(E.Key,0);Distance+=Travel;Display.Distance=Distance;Display.bMoving=E.bMoving&&Travel>.01f;Display.Speed=Dt>0?Travel/Dt:0;Position=P;auto& A=EmployeeDisplayYaw.FindOrAdd(E.Key,E.Yaw);A+=FMath::FindDeltaAngleDegrees(A,E.Yaw)*FMath::Min(1.f,Dt*10);Yaw=A;}
  if(V->GetAttachParent()!=Parent)V->AttachToComponent(Parent,FAttachmentTransformRules::KeepRelativeTransform);V->SetRelativeLocationAndRotation(Position,FRotator(0,Yaw,0));V->Present(Display,Dt);
 }
 UpdateSupplyVisuals(Dt);
}
