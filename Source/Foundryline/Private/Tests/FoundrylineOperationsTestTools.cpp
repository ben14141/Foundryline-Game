#include "FoundrylineNetworkTestTools.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "World/FoundrylineCameraPawn.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {FString UIJson(const TSharedRef<FJsonObject>& J){FString S;auto W=TJsonWriterFactory<>::Create(&S);FJsonSerializer::Serialize(J,W);return S;}}
bool UFoundrylineNetworkTestTools::SetupOperationsReview(APlayerController* Player,int32 Scene){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;auto* Plant=PC->FactoryPlant();if(!Plant)return false;PC->OpenOperations(0);PC->OpenFactoryComputer(0);
 if(Scene==1||Scene==2){if(!PC->IsWalking()){PC->ToggleWalk();if(!PC->IsWalking())return false;}AFoundrylineOperationsTerminal* T=nullptr;for(TActorIterator<AFoundrylineOperationsTerminal> It(PC->GetWorld());It;++It)if(It->PlantId==Plant->BusinessPlantId&&It->Terminal==Scene)T=*It;if(!T||!PC->GetPawn())return false;const FVector At=T->GetActorLocation()+FVector(0,170,90);const FVector Eye=At+FVector(0,0,80);const FRotator Facing=(T->InteractionPoint()-Eye).Rotation();PC->GetPawn()->SetActorLocationAndRotation(At,FRotator(0,Facing.Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation(Facing);PC->SetViewTarget(PC->GetPawn());return true;}
 if(PC->IsWalking()){AFoundrylineOperationsTerminal* Office=nullptr;for(TActorIterator<AFoundrylineOperationsTerminal> It(PC->GetWorld());It;++It)if(It->Terminal==1&&It->PlantId==Plant->BusinessPlantId)Office=*It;if(!Office)return false;PC->GetPawn()->SetActorLocation(Office->GetActorLocation()+FVector(0,170,90),false,nullptr,ETeleportType::TeleportPhysics);PC->OpenOperations(1);PC->EnterPropertyView();if(PC->IsWalking())return false;}if(PC->AreRoofsShown())PC->ToggleRoofs();
 if(Scene==4){PC->bFactoryMenu=false;if(auto* P=Cast<AFoundrylineCameraPawn>(PC->GetPawn()))P->FocusOnWorldPosition(Plant->GetActorTransform().TransformPosition(FVector(-500,650,0)));PC->SetViewTarget(PC->GetPawn());return true;}
 const FVector Local=Scene==3?FVector(3300,3400,2700):FVector(3900,4900,6500);const FVector Target=Scene==3?FVector(1200,1700,100):FVector(-900,600,0);const FVector At=Plant->GetActorTransform().TransformPosition(Local);auto* Camera=PC->GetWorld()->SpawnActor<ACameraActor>(At,(Plant->GetActorTransform().TransformPosition(Target)-At).Rotation());if(!Camera)return false;Camera->GetCameraComponent()->SetFieldOfView(Scene==3?55:50);PC->SetViewTarget(Camera);PC->bFactoryMenu=false;return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::OperationsUIReport(APlayerController* Player){auto J=MakeShared<FJsonObject>();
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||PC->GetWorld()->WorldType!=EWorldType::PIE)return TEXT("{}");if(PC->GetPawn()){const auto L=PC->GetPawn()->GetActorLocation();J->SetStringField(TEXT("pawn_position"),L.ToString());}J->SetBoolField(TEXT("planner_pending"),PC->bPlannerPending);J->SetBoolField(TEXT("sign_name_editor"),PC->SignNameWidget.IsValid());J->SetStringField(TEXT("planner_status"),PC->PlannerStatus);J->SetNumberField(TEXT("planner_kind"),PC->PlannerKind);J->SetBoolField(TEXT("property_access"),PC->bPropertyViewAccess);J->SetBoolField(TEXT("near_office"),PC->IsNearOfficeComputer());J->SetNumberField(TEXT("screen"),PC->OperationsScreen);J->SetNumberField(TEXT("page"),PC->OperationsPage);J->SetNumberField(TEXT("sort"),PC->OperationsSort);J->SetBoolField(TEXT("sort_descending"),PC->bOperationsSortDescending);J->SetNumberField(TEXT("document_tab"),PC->OperationsDocumentTab);J->SetNumberField(TEXT("workspace_tab"),PC->OperationsWorkspaceTab);J->SetNumberField(TEXT("list_page"),PC->OperationsListPage);J->SetNumberField(TEXT("selected_employee"),PC->OperationsEmployeeChoice);J->SetNumberField(TEXT("job_role"),PC->OperationsJobRole);J->SetNumberField(TEXT("job_page"),PC->OperationsJobPage);J->SetNumberField(TEXT("selected_order"),PC->OperationsOrder);J->SetNumberField(TEXT("selected_booking"),PC->OperationsBooking);J->SetNumberField(TEXT("machine"),PC->FactoryMachine);J->SetBoolField(TEXT("walking"),PC->IsWalking());J->SetBoolField(TEXT("roofs"),PC->AreRoofsShown());J->SetBoolField(TEXT("cursor"),PC->bShowMouseCursor);J->SetBoolField(TEXT("ignore_move"),PC->IsMoveInputIgnored());J->SetBoolField(TEXT("ignore_look"),PC->IsLookInputIgnored());J->SetNumberField(TEXT("selected_pallets"),PC->OperationsPallets.Num());J->SetStringField(TEXT("status"),PC->OperationsStatus);J->SetNumberField(TEXT("focused_machine"),PC->FocusedComputer());TArray<TSharedPtr<FJsonValue>> Ts;for(TActorIterator<AFoundrylineOperationsTerminal> It(PC->GetWorld());It;++It){auto T=MakeShared<FJsonObject>();T->SetNumberField(TEXT("kind"),It->Terminal);T->SetStringField(TEXT("plant"),It->PlantId.ToString());auto* Mesh=It->FindComponentByClass<UStaticMeshComponent>();T->SetBoolField(TEXT("mesh"),Mesh&&Mesh->GetStaticMesh());T->SetBoolField(TEXT("collision"),Mesh&&Mesh->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);Ts.Add(MakeShared<FJsonValueObject>(T));}J->SetArrayField(TEXT("terminals"),Ts);
#endif
 return UIJson(J);}

AActor* UFoundrylineNetworkTestTools::CreateReviewCamera(APlayerController* Player){
#if WITH_EDITOR
 if(Player&&Player->IsLocalController()&&Player->GetWorld()->WorldType==EWorldType::PIE){FActorSpawnParameters P;P.ObjectFlags|=RF_Transient;return Player->GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);}
#endif
 return nullptr;
}
