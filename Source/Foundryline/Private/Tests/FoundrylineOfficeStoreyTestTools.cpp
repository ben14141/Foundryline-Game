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
 }
#endif
 FString Out;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));return Out;
}
