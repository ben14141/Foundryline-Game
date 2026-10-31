#include "FoundrylineNetworkTestTools.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "Buildings/FoundrylineOfficeStoreyLink.h"
#include "UI/FoundrylinePlayerController.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "EngineUtils.h"
#include "Management/FoundrylineOfficeAssignments.h"
#include "Management/FoundrylineOfficeDecorationComponent.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"

FString UFoundrylineNetworkTestTools::OfficeStoreyReport(APlayerController* Player){
 auto Root=MakeShared<FJsonObject>();
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);auto* Plant=PC?PC->FactoryPlant():nullptr;
 if(PC&&Plant){auto* P=PC->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();Root->SetNumberField(TEXT("view_level"),PC->OfficePlanningLevel);Root->SetStringField(TEXT("factory_status"),PC->FactoryStatus);Root->SetStringField(TEXT("operations_status"),PC->OperationsStatus);Root->SetBoolField(TEXT("walking"),PC->IsWalking());Root->SetBoolField(TEXT("roof"),PC->AreRoofsShown());
  if(auto* Pawn=PC->GetPawn().Get()){const auto V=Pawn->GetActorLocation();Root->SetArrayField(TEXT("pawn"),{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)});}
  for(TActorIterator<AFoundrylineBuildSite> It(PC->GetWorld());It;++It){Root->SetStringField(TEXT("digest"),It->GetPropertyDigest());Root->SetStringField(TEXT("status"),It->Status);break;}
  TArray<TSharedPtr<FJsonValue>> Offices;for(const auto& O:P->GetObjects())if(O.bOfficeAnnex&&O.CompanyId==Plant->BusinessCompanyId&&O.PlantId==Plant->BusinessPlantId){auto J=MakeShared<FJsonObject>();const auto* D=P->FindDefinition(O.DefinitionId);J->SetStringField(TEXT("id"),O.ObjectId.ToString());J->SetBoolField(TEXT("upper"),O.bOfficeSecondFloor);J->SetNumberField(TEXT("width"),D?D->Footprint.Max.X:0);J->SetNumberField(TEXT("depth"),D?D->Footprint.Max.Y:0);J->SetNumberField(TEXT("quarter"),O.QuarterTurns);J->SetArrayField(TEXT("anchor"),{MakeShared<FJsonValueNumber>(O.Anchor.X),MakeShared<FJsonValueNumber>(O.Anchor.Y),MakeShared<FJsonValueNumber>(O.Anchor.Z)});J->SetArrayField(TEXT("stair_origin"),{MakeShared<FJsonValueNumber>(O.OfficeStairOrigin.X),MakeShared<FJsonValueNumber>(O.OfficeStairOrigin.Y)});J->SetNumberField(TEXT("stair_quarter"),O.OfficeStairQuarter);J->SetNumberField(TEXT("stair_style"),O.OfficeStairStyle);
   TArray<TSharedPtr<FJsonValue>> Route;const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));for(auto V:FFoundrylineOfficeStairLayout::WalkRoute(O.OfficeStairOrigin,O.OfficeStairQuarter,O.OfficeStairStyle)){V=T.TransformPosition(V);Route.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)}));}J->SetArrayField(TEXT("route"),Route);J->SetNumberField(TEXT("upper_rooms"),O.OfficeRooms.FilterByPredicate([](const auto& R){return R.OfficeLevel==1;}).Num());J->SetNumberField(TEXT("upper_windows"),O.Openings.FilterByPredicate([](const auto& V){return V.OfficeLevel==1&&V.IsGlazing();}).Num());Offices.Add(MakeShared<FJsonValueObject>(J));
  }Root->SetArrayField(TEXT("offices"),Offices);int UpperActors=0,Visible=0;for(TActorIterator<AActor> It(PC->GetWorld());It;++It)if(auto* Link=It->FindComponentByClass<UFoundrylineOfficeStoreyLink>();Link&&IsValid(Link->UpperActor)){++UpperActors;Visible+=!Link->UpperActor->IsHidden();}Root->SetNumberField(TEXT("upper_actors"),UpperActors);Root->SetNumberField(TEXT("upper_visible"),Visible);
  const auto& S=Plant->HasAuthority()?Plant->Model.State:Plant->Snapshot;Root->SetNumberField(TEXT("upper_desks"),S.OfficeDesks.FilterByPredicate([](const auto& D){return D.Center.Z>300;}).Num());Root->SetNumberField(TEXT("cash"),S.Commerce.CashCents);
  auto Vec=[](FVector V){return TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};};
  TArray<TSharedPtr<FJsonValue>> Rooms,Assignments,People,Doors,Visuals;
  for(const auto& R:FFoundrylineOffices::Rooms(PC->GetWorld(),S,Plant->GetActorTransform())){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("building"),R.BuildingId.ToString(EGuidFormats::Digits));J->SetStringField(TEXT("room"),R.RoomId.ToString(EGuidFormats::Digits));J->SetNumberField(TEXT("floor"),R.Floor);J->SetNumberField(TEXT("doors"),R.DoorIds.Num());J->SetArrayField(TEXT("origin"),Vec(R.Frame.GetLocation()));J->SetArrayField(TEXT("min"),Vec(FVector(R.Bounds.Min,0)));J->SetArrayField(TEXT("max"),Vec(FVector(R.Bounds.Max,0)));Rooms.Add(MakeShared<FJsonValueObject>(J));}Root->SetArrayField(TEXT("rooms"),Rooms);
  for(const auto& A:S.OfficeAssignments){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("id"),A.Id);J->SetNumberField(TEXT("person"),A.EmployeeId);J->SetNumberField(TEXT("desk"),A.DeskId);J->SetNumberField(TEXT("floor"),A.Floor);J->SetNumberField(TEXT("quarter"),A.DeskQuarter);J->SetStringField(TEXT("name"),A.OccupantName);J->SetArrayField(TEXT("position"),Vec(A.DeskPosition));TArray<TSharedPtr<FJsonValue>> Decor;for(const auto& D:A.Decorations){auto K=MakeShared<FJsonObject>();K->SetNumberField(TEXT("kind"),D.Kind);K->SetNumberField(TEXT("quarter"),D.Quarter);K->SetArrayField(TEXT("position"),Vec(D.Position));K->SetBoolField(TEXT("hidden"),D.HiddenByGlazing);Decor.Add(MakeShared<FJsonValueObject>(K));}J->SetArrayField(TEXT("decor"),Decor);Assignments.Add(MakeShared<FJsonValueObject>(J));}Root->SetArrayField(TEXT("assignments"),Assignments);
  for(const auto& E:S.OfficeEmployees){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("id"),E.Id);J->SetStringField(TEXT("name"),E.DisplayName);People.Add(MakeShared<FJsonValueObject>(J));}Root->SetArrayField(TEXT("office_people"),People);
  for(TActorIterator<AActor> It(PC->GetWorld());It;++It)if(It->ActorHasTag(TEXT("FoundrylinePropertyVisual")))for(auto* Door:UFoundrylineOfficeStoreyLink::Doors(*It)){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Door->GetOfficeName());J->SetStringField(TEXT("title"),Door->GetOfficeTitle());J->SetStringField(TEXT("mesh"),Door->GetStaticMesh()?Door->GetStaticMesh()->GetName():TEXT(""));J->SetStringField(TEXT("id"),Door->OpeningId.ToString());J->SetArrayField(TEXT("position"),Vec(Door->GetComponentLocation()));J->SetBoolField(TEXT("open"),Door->IsOpen());Doors.Add(MakeShared<FJsonValueObject>(J));}Root->SetArrayField(TEXT("office_doors"),Doors);
  TArray<UFoundrylineEmployeeVisualComponent*> EmployeeComponents;Plant->GetComponents(EmployeeComponents);for(const auto* EmployeeVisual:EmployeeComponents)if(EmployeeVisual){TSharedPtr<FJsonObject> J;auto Reader=TJsonReaderFactory<>::Create(EmployeeVisual->GetPresentationReport());if(FJsonSerializer::Deserialize(Reader,J))Visuals.Add(MakeShared<FJsonValueObject>(J));}Root->SetArrayField(TEXT("employee_visuals"),Visuals);Root->SetNumberField(TEXT("decor_groups"),Plant->OfficeDecorationVisuals.Num());Root->SetNumberField(TEXT("desk_count"),S.OfficeDesks.Num());

 }
#endif
 FString Out;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));return Out;
}
