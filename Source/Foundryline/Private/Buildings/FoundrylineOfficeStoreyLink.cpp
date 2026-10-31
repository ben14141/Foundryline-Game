#include "Buildings/FoundrylineOfficeStoreyLink.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
void UFoundrylineOfficeStoreyLink::EndPlay(const EEndPlayReason::Type Reason){if(IsValid(UpperActor))UpperActor->Destroy();Super::EndPlay(Reason);}
void UFoundrylineOfficeStoreyLink::Show(bool Visible,bool Roof){if(!IsValid(UpperActor))return;UpperActor->SetActorHiddenInGame(!Visible);TInlineComponentArray<USceneComponent*> Cs;UpperActor->GetComponents(Cs);for(auto* C:Cs)if(C->ComponentHasTag(TEXT("FoundrylineRoof")))C->SetVisibility(Roof);}

void UFoundrylineOfficeStoreyLink::OnComponentDestroyed(bool Hierarchy){if(IsValid(UpperActor))UpperActor->Destroy();Super::OnComponentDestroyed(Hierarchy);}
TArray<UFoundrylinePersonnelDoorComponent*> UFoundrylineOfficeStoreyLink::Doors(AActor* Actor){TArray<UFoundrylinePersonnelDoorComponent*> Out;if(!IsValid(Actor))return Out;Actor->GetComponents(Out);if(auto* Link=Actor->FindComponentByClass<UFoundrylineOfficeStoreyLink>();Link&&IsValid(Link->UpperActor)){TArray<UFoundrylinePersonnelDoorComponent*> Upper;Link->UpperActor->GetComponents(Upper);Out.Append(Upper);}return Out;}
