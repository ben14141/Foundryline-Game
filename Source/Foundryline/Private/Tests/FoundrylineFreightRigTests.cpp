#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineFreightRigTest,"Foundryline.Logistics.TractorTankerCoupling",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineFreightRigTest::RunTest(const FString& Parameters) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* R=W->SpawnActor<AFoundrylineFreightRig>();R->DispatchBeginPlay();
 auto* P=W->SpawnActor<APlayerController>();P->PlayerState=W->SpawnActor<APlayerState>();
 auto* Other=W->SpawnActor<APlayerController>();Other->PlayerState=W->SpawnActor<APlayerState>();
 TestTrue(TEXT("One player can claim tractor"),R->Claim(P,true));TestFalse(TEXT("Second player cannot steal tractor"),R->Claim(Other,true));
 TestFalse(TEXT("Distant hitch cannot couple"),R->TryCouple());
 R->State.Tractor={-42,0,0};R->State.TractorYaw=100;TestFalse(TEXT("Angled hitch rejected"),R->TryCouple());
 R->State.TractorYaw=0;R->State.Speed=-220;TestFalse(TEXT("Unsafe approach speed rejected"),R->TryCouple());
 R->State.Tractor={-120,90,0};R->State.TractorYaw=25;R->State.Speed=-100;TestTrue(TEXT("Nearby offset angled approach snaps automatically"),R->TryCouple());
 TestTrue(TEXT("Kingpin and fifth wheel align"),R->Hitch(true).Equals(R->Hitch(false),.01));
 R->Step(1.f/30);
 TestTrue(TEXT("Coupling starts automatic leg retraction"),R->State.Gear<1);
 for(int32 I=0;I<70;++I)R->Step(1.f/30);
 TestTrue(TEXT("Landing gear fully retracts after coupling"),R->State.Gear<.01f);
 R->Drive(Other,1,1);const auto Before=R->State.Tractor;for(int32 I=0;I<10;++I)R->Step(1.f/30);
 TestTrue(TEXT("Non-driver input cannot move tractor"),R->State.Tractor.Equals(Before));
 for(int32 I=0;I<40;++I){R->Drive(P,1,.3f);R->Step(1.f/30);}
 TestTrue(TEXT("Truck moves and wheels accumulate travel"),R->State.TractorTravel>20);
 TestTrue(TEXT("Trailer follows and wheels accumulate travel"),R->State.TrailerTravel>20);
 TestTrue(TEXT("Hitch remains connected during a turn"),R->Hitch(true).Equals(R->Hitch(false),.01));
 TestFalse(TEXT("Moving truck cannot lower landing legs"),R->SetGear(true));
 for(int32 I=0;I<80;++I)R->Step(1.f/30);
 TestTrue(TEXT("Stale control input stops vehicle"),FMath::Abs(R->State.Speed)<1);
 TestTrue(TEXT("Detach request automatically lowers the legs"),R->Uncouple());
 TestTrue(TEXT("Trailer remains attached while legs lower"),R->State.Coupled && R->State.Detaching);
 for(int32 I=0;I<70;++I)R->Step(1.f/30);
 TestTrue(TEXT("Trailer automatically detaches once supported"),!R->State.Coupled && !R->State.Detaching && R->State.Gear>.99f);const auto Parked=R->State.Trailer;
 TestFalse(TEXT("Unhitched legs cannot retract"),R->SetGear(false));
 for(int32 I=0;I<40;++I){R->Drive(P,1,0);R->Step(1.f/30);}
 TestTrue(TEXT("Detached trailer stays parked"),R->State.Trailer.Equals(Parked));
 for(int32 I=0;I<400;++I){R->Drive(P,1,0);R->Tick(1.f/120);}
 int32 StationaryFrames=0;float MaxDelta=0;auto Visual=R->TractorWorldPosition();
 for(int32 I=0;I<120;++I){R->Drive(P,1,0);R->Tick(1.f/120);const auto Now=R->TractorWorldPosition();const float D=FVector::Dist(Now,Visual);if(D<.001f)++StationaryFrames;MaxDelta=FMath::Max(MaxDelta,D);Visual=Now;}
 TestEqual(TEXT("120 Hz visuals have no held frames between 30 Hz simulation steps"),StationaryFrames,0);
 TestTrue(TEXT("Visual frame displacement stays below four cm"),MaxDelta<4);
 TestTrue(TEXT("Driver can release tractor"),R->Claim(P,false));
 W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPickupTractorTest,"Foundryline.Logistics.PickupTractorStaging",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPickupTractorTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* R=W->SpawnActor<AFoundrylineFreightRig>(FVector(2950,1650,20),FRotator::ZeroRotator);R->bTractorOnly=true;R->State.Tractor=FVector::ZeroVector;R->DispatchBeginPlay();
 bool TrailerHidden=false,TrailerClear=false;TArray<USceneComponent*> Components;R->GetComponents(Components);for(auto* C:Components){if(C->GetName()==TEXT("Tanker"))TrailerHidden=!C->IsVisible();if(C->GetName()==TEXT("TrailerCollision"))TrailerClear=Cast<UBoxComponent>(C)->GetCollisionEnabled()==ECollisionEnabled::NoCollision;}
 TestTrue(TEXT("Pickup tractor does not bring a second tanker into the bay"),TrailerHidden&&TrailerClear);TestFalse(TEXT("Tractor cannot couple to an invisible practice trailer"),R->TryCouple());TestTrue(TEXT("Tractor rear clears the loading wall"),R->GetActorLocation().X-90>2800);TestEqual(TEXT("Tractor is centred on the tanker pickup door"),R->GetActorLocation().Y,1650.);
 auto* P=W->SpawnActor<APlayerController>();P->PlayerState=W->SpawnActor<APlayerState>();TestTrue(TEXT("Pickup tractor can still be driven"),R->Claim(P,true));for(int I=0;I<40;++I){R->Drive(P,1,0);R->Step(1.f/30);}TestTrue(TEXT("Tractor-only travel remains functional"),R->State.TractorTravel>20&&!R->State.Coupled);W->DestroyWorld(false);return true;
}
#endif
