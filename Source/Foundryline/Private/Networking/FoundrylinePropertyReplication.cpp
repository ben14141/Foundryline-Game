#include "Construction/FoundrylineBuildSite.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "UI/FoundrylinePlayerController.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

void AFoundrylineBuildSite::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(AFoundrylineBuildSite,NetworkState);
}
int64 AFoundrylineBuildSite::GetPropertyRevision() const {return Placement?Placement->GetRevision():0;}
FString AFoundrylineBuildSite::GetPropertyDigest() const
{
 if(!Placement) return TEXT("NOT_READY");
 auto* Snapshot=NewObject<UFoundrylinePlacementSave>();Snapshot->Objects=Placement->GetObjects();Snapshot->Regions=Placement->GetRegions();Snapshot->Revision=Placement->GetRevision();
 TArray<uint8> Bytes;UGameplayStatics::SaveGameToMemory(Snapshot,Bytes);
 return FString::Printf(TEXT("%08X"),FCrc::MemCrc32(Bytes.GetData(),Bytes.Num()));
}
void AFoundrylineBuildSite::PublishNetworkState()
{
 if(!HasAuthority() || !Placement || !bReady) return;
 NetworkState.Revision=Placement->GetRevision();NetworkState.Objects=Placement->GetObjects();NetworkState.Regions=Placement->GetRegions();NetworkState.bStarterProperty=bStarterProperty;NetworkState.Doors.Reset();
 for(const auto& Pair:Visuals) if(IsValid(Pair.Value))
 {
  TInlineComponentArray<UFoundrylinePersonnelDoorComponent*> Doors;Pair.Value->GetComponents(Doors);
  for(const auto* D:Doors) {FFoundrylineDoorState State;State.ObjectId=Pair.Key;State.OpeningId=D->OpeningId;State.bOpen=D->IsOpen();NetworkState.Doors.Add(State);}
 }
 ForceNetUpdate();
}
void AFoundrylineBuildSite::ReceiveNetworkState()
{
 if(HasAuthority() || !Placement || NetworkState.Revision<=0) return;
 bStarterProperty=NetworkState.bStarterProperty;
 DoorStates.Reset();for(const auto& D:NetworkState.Doors) DoorStates.Add(D.OpeningId,D.bOpen);
 const auto Error=Placement->ApplyNetworkSnapshot(NetworkState.Objects,NetworkState.Regions,NetworkState.Revision);
 if(Error!=EFoundrylinePlacementError::None) {Status=TEXT("Property synchronization rejected; waiting for valid server state");return;}
 bReady=true;
 if(bStarterProperty) DrawLotBoundary();
 for(const auto& Pair:Visuals) if(IsValid(Pair.Value))
 {
  TInlineComponentArray<UFoundrylinePersonnelDoorComponent*> Doors;Pair.Value->GetComponents(Doors);
  for(auto* D:Doors) D->ApplyNetworkOpen(DoorStates.Contains(D->OpeningId)?DoorStates[D->OpeningId]:true);
 }
 for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It) if(It->IsLocalController()) {SetRoofsVisible(It->AreRoofsShown());break;}
 Status=TEXT("Shared property synchronized / build together");
}
bool AFoundrylineBuildSite::UseDoor(APawn* User,FGuid ObjectId,FGuid OpeningId)
{
 if(!HasAuthority() || !IsValid(User) || User->GetWorld()!=GetWorld()) return false;
 const auto* Actor=Visuals.Find(ObjectId);if(!Actor || !IsValid(*Actor)) return false;
 TInlineComponentArray<UFoundrylinePersonnelDoorComponent*> Doors;(*Actor)->GetComponents(Doors);
 for(auto* D:Doors) if(D->OpeningId==OpeningId && D->TryToggle(User)) {DoorStates.Add(OpeningId,D->IsOpen());PublishNetworkState();return true;}
 return false;
}
