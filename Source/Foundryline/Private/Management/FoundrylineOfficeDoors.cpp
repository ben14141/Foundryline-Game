#include "Production/FoundrylineFactoryPlant.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Buildings/FoundrylineOfficeStoreyLink.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "EngineUtils.h"
void AFoundrylineFactoryPlant::UpdateOfficeDoors(){
 auto* P=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();if(!P)return;const auto& State=HasAuthority()?Model.State:Snapshot;
 FString Signature;for(const auto& A:State.OfficeAssignments)Signature+=A.BuildingId.ToString()+A.RoomId.ToString()+FString::Printf(TEXT("/%d/%d/%d:"),A.Floor,A.EmployeeId,A.OccupantName.Len())+A.OccupantName;
 if(OfficeDoorPropertyRevision==P->GetRevision()&&OfficeDoorSignature==Signature)return;OfficeDoorPropertyRevision=P->GetRevision();OfficeDoorSignature=Signature;
 TMap<FGuid,const FFoundrylineOfficeAssignment*> Desired;const auto Rooms=FFoundrylineOffices::Rooms(GetWorld(),State,GetActorTransform());
 for(const auto& A:State.OfficeAssignments)if(const auto* R=Rooms.FindByPredicate([&](const auto& R){return R.BuildingId==A.BuildingId&&R.RoomId==A.RoomId&&R.Floor==A.Floor;}))for(auto Id:R->DoorIds)Desired.Add(Id,&A);
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("FoundrylinePropertyVisual")))for(auto* Door:UFoundrylineOfficeStoreyLink::Doors(*It)){
  const auto* O=P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Door->BuildingObjectId&&O.bOfficeAnnex&&O.CompanyId==State.CompanyId&&O.PlantId==State.PlantId;});if(!O)continue;
  if(const auto* const* A=Desired.Find(Door->OpeningId))Door->SetOfficeOccupant((*A)->OccupantName,(*A)->EmployeeId?TEXT("ENGINEER"):TEXT("PRESIDENT"));else Door->SetOfficeOccupant(TEXT(""),TEXT(""));
 }
}
