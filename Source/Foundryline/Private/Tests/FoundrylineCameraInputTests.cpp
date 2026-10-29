#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "World/FoundrylineCameraPawn.h"
#include "UI/FoundrylinePlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineWheelZoomTest,"Foundryline.View.WheelZoomStops",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineWheelZoomTest::RunTest(const FString&)
{
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);
 auto* Player=World->SpawnActor<AFoundrylinePlayerController>();auto* Camera=World->SpawnActor<AFoundrylineCameraPawn>();Player->SetAsLocalPlayerController();Player->Possess(Camera);
 auto* Input=NewObject<UInputComponent>(Camera);Camera->SetupPlayerInputComponent(Input);
 auto* Arm=Camera->FindComponentByClass<USpringArmComponent>();const float Initial=Arm->TargetArmLength;
 auto Scroll=[&](FKey Key){for(const auto& Binding:Input->KeyBindings) if(Binding.Chord.Key==Key && Binding.KeyEvent==IE_Pressed) {Binding.KeyDelegate.Execute(Key);return true;}return false;};
 auto Settle=[&](){for(int32 I=0;I<300;++I) Camera->Tick(1.f/60);};
 TestTrue(TEXT("Local camera tick is active"),Camera->IsLocallyControlled());
 TestTrue(TEXT("Wheel down is delivered as one scroll event"),Scroll(EKeys::MouseScrollDown));Settle();
 const float Out=Arm->TargetArmLength;TestTrue(TEXT("One wheel notch zooms out once"),FMath::IsNearlyEqual(Out,Initial*1.12f,.1f));
 Settle();TestTrue(TEXT("Idle frames never continue zooming out"),FMath::IsNearlyEqual(Arm->TargetArmLength,Out,.01f));
 TestTrue(TEXT("Wheel up has a discrete event"),Scroll(EKeys::MouseScrollUp));Settle();TestTrue(TEXT("Opposite notch restores the original distance"),FMath::IsNearlyEqual(Arm->TargetArmLength,Initial,.1f));
 const float In=Arm->TargetArmLength;Settle();TestTrue(TEXT("Idle frames never continue zooming in"),FMath::IsNearlyEqual(Arm->TargetArmLength,In,.01f));
 World->DestroyWorld(false);return true;
}
#endif
