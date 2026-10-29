#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
UFoundrylinePersonnelDoorComponent::UFoundrylinePersonnelDoorComponent()
{
 PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=false;
 SetIsReplicatedByDefault(false);SetMobility(EComponentMobility::Movable);
}
void UFoundrylinePersonnelDoorComponent::InitializeSwing(float Closed,float Swing,FGuid Id,bool Open)
{
 ClosedYaw=Closed;SwingYaw=Swing;OpeningId=Id;bOpen=Open;Progress=Open?1:0;
 SetRelativeRotation(FRotator(0,ClosedYaw+SwingYaw*Progress,0));SetComponentTickEnabled(false);
}
bool UFoundrylinePersonnelDoorComponent::TryToggle(APawn* User)
{
 if((GetWorld() && GetWorld()->GetNetMode()==NM_Client) || !GetOwner() || !GetOwner()->HasAuthority() || !User || User->GetWorld()!=GetWorld()) return false;
 FVector Eye;FRotator View;User->GetActorEyesViewPoint(Eye,View);
 if(FVector::DistSquared(Eye,GetComponentLocation()+FVector(0,0,110))>FMath::Square(300.f)) return false;
 FHitResult Hit;FCollisionQueryParams Query;Query.AddIgnoredActor(User);
 if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+View.Vector()*300,ECC_Visibility,Query) || Hit.GetComponent()!=this) return false;
 bOpen=!bOpen;StateChanged();GetOwner()->ForceNetUpdate();return true;
}
void UFoundrylinePersonnelDoorComponent::StateChanged() {SetComponentTickEnabled(true);}
void UFoundrylinePersonnelDoorComponent::TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* Function)
{
 Super::TickComponent(Dt,Tick,Function);
 Progress=FMath::FInterpConstantTo(Progress,bOpen?1.f:0.f,Dt,2.f);
 const float Ease=Progress*Progress*(3-2*Progress);
 SetRelativeRotation(FRotator(0,ClosedYaw+SwingYaw*Ease,0));
 if(Progress==(bOpen?1.f:0.f)) SetComponentTickEnabled(false);
}
void UFoundrylinePersonnelDoorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UFoundrylinePersonnelDoorComponent,bOpen);DOREPLIFETIME(UFoundrylinePersonnelDoorComponent,ClosedYaw);DOREPLIFETIME(UFoundrylinePersonnelDoorComponent,SwingYaw);
}

void UFoundrylinePersonnelDoorComponent::ApplyNetworkOpen(bool Open)
{
 if(bOpen==Open) return;bOpen=Open;StateChanged();
}
