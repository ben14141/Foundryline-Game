#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Logistics/FoundrylineCarrierSceneActor.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "World/FoundrylineWalkCharacter.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterHaulerAnimation,"Foundryline.Logistics.StarterHaulerAssemblyAndAnimation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterHaulerAnimation::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AFoundrylineCarrierSceneActor>();
 FFoundrylineFreightBooking B;B.Id=1;B.Carrier=1;B.Status=EFoundrylineBookingState::Approaching;B.DispatchAt=0;B.ArrivalAt=20;
 FFoundrylineCommerceState S;S.Clock=0;A->SetBooking(B,S,FTransform::Identity,{});A->Tick(.1f);
 TArray<UStaticMeshComponent*> All;A->GetComponents(All);UStaticMeshComponent *Front=nullptr,*Rear=nullptr,*Body=nullptr,*Gate=nullptr;int Wheels=0;
 for(auto* C:All){if(C->GetName()==TEXT("Body"))Body=C;if(C->GetName()==TEXT("LoadingSideGate"))Gate=C;if(C->GetStaticMesh()){const FString N=C->GetStaticMesh()->GetName();if(N.StartsWith(TEXT("SM_Wheel")))++Wheels;if(N==TEXT("SM_Wheel0"))Front=C;if(N==TEXT("SM_Wheel2"))Rear=C;}}
 TestEqual(TEXT("Six separately pivoted wheels imported"),Wheels,6);TestTrue(TEXT("Actual purchased flatbed body"),Body&&Body->GetStaticMesh()&&Body->GetStaticMesh()->GetName()==TEXT("SM_StarterFlatbed"));
 if(!Front||!Rear||!Gate){AddError(TEXT("Missing wheel or gate component"));W->DestroyWorld(false);return false;}
 const FQuat Initial=Front->GetRelativeRotation().Quaternion();bool Steered=false,Spun=false;
 for(int I=1;I<=20;++I){S.Clock=I;A->SetBooking(B,S,FTransform::Identity,{});A->Tick(.1f);const FVector FA=Front->GetRelativeRotation().Quaternion().RotateVector(FVector::RightVector),RA=Rear->GetRelativeRotation().Quaternion().RotateVector(FVector::RightVector);Steered|=FVector::DotProduct(FA,RA)<.999;Spun|=Initial.AngularDistance(Front->GetRelativeRotation().Quaternion())>.2;}
 TestTrue(TEXT("Front axle steers through a real route turn"),Steered);TestTrue(TEXT("Wheels rotate as the displayed truck travels"),Spun);
 B.Status=EFoundrylineBookingState::Loading;B.BerthAt=20;B.DockedAt=32;S.Clock=32;A->SetBooking(B,S,FTransform::Identity,{});for(int I=0;I<30;++I)A->Tick(.1f);
 const auto Up=Gate->GetRelativeRotation().Quaternion().RotateVector(FVector::UpVector);TestTrue(TEXT("Loading gate folds down 180 degrees when docked"),Up.Z<-.99);
 TestTrue(TEXT("Body retains authored unit scale"),Body&&Body->GetRelativeScale3D().Equals(FVector::OneVector,.001));
 const FVector Root=A->GetActorLocation();FHitResult WalkHit;const auto* WalkCapsule=GetDefault<AFoundrylineWalkCharacter>()->GetCapsuleComponent();const float HalfHeight=WalkCapsule->GetUnscaledCapsuleHalfHeight();const auto Capsule=FCollisionShape::MakeCapsule(WalkCapsule->GetUnscaledCapsuleRadius(),HalfHeight);const bool BlocksPawn=W->SweepSingleByChannel(WalkHit,Root+FVector(-500,0,HalfHeight),Root+FVector(0,0,HalfHeight),FQuat::Identity,ECC_Pawn,Capsule);TestTrue(TEXT("Actual first-person capsule sweep is blocked by truck body"),BlocksPawn&&WalkHit.GetActor()==A);
 FHitResult ClearHit;TestFalse(TEXT("Truck collision does not create invisible wall behind its footprint"),W->SweepSingleByChannel(ClearHit,Root+FVector(-500,-700,HalfHeight),Root+FVector(0,-700,HalfHeight),FQuat::Identity,ECC_Pawn,Capsule));TestFalse(TEXT("Pawn-only truck body does not obstruct dynamic loading helpers"),W->SweepSingleByChannel(ClearHit,Root+FVector(-500,0,HalfHeight),Root+FVector(0,0,HalfHeight),FQuat::Identity,ECC_WorldDynamic,Capsule));
 B.Status=EFoundrylineBookingState::Departing;B.DepartedAt=32;A->SetBooking(B,S,FTransform::Identity,{});for(int I=0;I<30;++I)A->Tick(.1f);TestTrue(TEXT("Loading gate closes for travel"),Gate->GetRelativeRotation().Quaternion().RotateVector(FVector::UpVector).Z>.99);
 FFoundrylineFactoryPallet P;P.Id=17;P.Part=EFoundrylinePart::Bracket;P.Quantity=40;P.Place=EFoundrylinePalletPlace::Outbound;P.Owner=1;P.BookingId=1;B.Manifest={{17,uint8(P.Part),40,5}};A->SetBooking(B,S,FTransform::Identity,{P});
 TArray<UInstancedStaticMeshComponent*> Instances;A->GetComponents(Instances);int Visible=0;for(auto* C:Instances)if(C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==TEXT("SM_BracketBlank"))Visible+=C->GetInstanceCount();TestEqual(TEXT("Cut-part pallet shows ten bounded samples for a forty-part load"),Visible,10);
 P.Part=EFoundrylinePart::Sheet;P.Quantity=1;B.Manifest={{17,uint8(P.Part),1,255.25f}};A->SetBooking(B,S,FTransform::Identity,{P});Instances.Reset();A->GetComponents(Instances);int Supports=0;bool Longitudinal=false;for(auto* C:Instances)if(C->GetStaticMesh()){if(C->GetStaticMesh()->GetName()==TEXT("SM_Pallet_Wood"))Supports+=C->GetInstanceCount();if(C->GetStaticMesh()->GetName()==TEXT("SM_SteelSheet_60x120")) {FTransform T;if(C->GetInstanceTransform(0,T,false))Longitudinal=T.GetRotation().Equals(FQuat::Identity,.001);}}TestEqual(TEXT("Sheet freight uses two wood supports"),Supports,2);TestTrue(TEXT("Sheet long axis retained along truck bed"),Longitudinal);
 W->DestroyWorld(false);return true;
}
#endif
