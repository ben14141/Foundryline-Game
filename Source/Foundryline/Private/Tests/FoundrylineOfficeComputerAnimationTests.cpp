#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeComputerClip,"Foundryline.Employees.OfficeComputer.SeatedTypingAndMouse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeComputerClip::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AActor>();auto* V=NewObject<UFoundrylineEmployeeVisualComponent>(A);A->SetRootComponent(V);V->RegisterComponent();A->AddInstanceComponent(V);
 for(int I=0;I<40;++I)V->PresentOfficeComputer(.1f,7);
 auto Read=[&](){TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(V->GetPresentationReport()),J);return J;};
 auto Typing=Read();if(!TestTrue(TEXT("Typing report exists"),Typing.IsValid())){W->DestroyWorld(false);return false;}
 TestTrue(TEXT("Computer work is seated"),Typing->GetBoolField(TEXT("seated")));TestFalse(TEXT("No standing capsule on the office chair"),Typing->GetBoolField(TEXT("walkingCollision")));
 TestTrue(TEXT("Office worker uses the real employee skeleton"),Typing->GetBoolField(TEXT("skeletal")));
 TestTrue(TEXT("Typing left palm reaches desktop height"),FMath::Abs(Typing->GetNumberField(TEXT("palm0LocalZ"))-82)<3);
 TestTrue(TEXT("Typing right palm reaches desktop height"),FMath::Abs(Typing->GetNumberField(TEXT("palm1LocalZ"))-82)<3);
 TestTrue(TEXT("Both feet stay on the floor"),FMath::Abs(Typing->GetNumberField(TEXT("foot0WorldZ")))<.1&&FMath::Abs(Typing->GetNumberField(TEXT("foot1WorldZ")))<.1);
 TArray<USceneComponent*> Before;A->GetComponents(Before);
 for(int I=0;I<50;++I)V->PresentOfficeComputer(.1f,7);auto Mouse=Read();
 TestNotEqual(TEXT("Mouse phase changes the hand pose"),Mouse->GetNumberField(TEXT("palm1LocalX")),Typing->GetNumberField(TEXT("palm1LocalX")));
 TestTrue(TEXT("Mouse palm remains at desk height"),FMath::Abs(Mouse->GetNumberField(TEXT("palm1LocalZ"))-82)<3);
 A->SetActorLocationAndRotation({12000,-8000,20},FRotator(0,90,0));V->PresentOfficeComputer(.1f,7);auto Rotated=Read();TestTrue(TEXT("Feet remain grounded at a rotated translated business"),FMath::Abs(Rotated->GetNumberField(TEXT("foot0WorldZ"))-20)<.1);
 TArray<USceneComponent*> After;A->GetComponents(After);TestEqual(TEXT("Animation reuses one body"),After.Num(),Before.Num());W->DestroyWorld(false);return true;
}
#endif
