#include "FoundrylineNetworkTestTools.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylinePlasmaTable.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Production/FoundrylinePressBrakeDisplay.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
bool UFoundrylineNetworkTestTools::StartEquipmentExercise(APlayerController* Host){
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||Host->GetWorld()->WorldType!=EWorldType::PIE)return false;
 for(TActorIterator<AFoundrylineFactoryPlant> It(Host->GetWorld());It;++It){auto& M=It->Model;if(M.State.Running||M.State.Jobs.Num())return false;for(const auto& Machine:M.State.Machines)if(Machine.Remaining>0)return false;
  const double Mass=M.SteelMass();auto Copy=M;TArray<int32> Remove;
  for(auto& P:Copy.State.Pallets)if(P.Place==EFoundrylinePalletPlace::MachineOutput){auto* Dest=Copy.State.Pallets.FindByPredicate([&](const auto& D){return (D.Place==EFoundrylinePalletPlace::Rack||D.Place==EFoundrylinePalletPlace::Buffer)&&D.Part==P.Part&&!D.Reserved&&FMath::IsNearlyEqual(D.UnitLb,P.UnitLb)&&D.Quantity+P.Quantity<=FFoundrylineFactoryCatalog::Part(P.Part).Capacity;});if(!Dest)return false;Dest->Quantity+=P.Quantity;Remove.Add(P.Id);}
  Copy.State.Pallets.RemoveAll([&](const auto& P){return Remove.Contains(P.Id);});
  const EFoundrylinePart Inputs[]={EFoundrylinePart::Sheet,EFoundrylinePart::RailBlank,EFoundrylinePart::ShellBlank},Outputs[]={EFoundrylinePart::RailBlank,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection};FString Error;
  for(int I=0;I<3;++I){auto* Source=Copy.State.Pallets.FindByPredicate([&](const auto& P){return P.Place==EFoundrylinePalletPlace::Rack&&P.Part==Inputs[I]&&P.Quantity>P.Reserved;});if(!Source)return false;auto P=*Source;--Source->Quantity;P.Id=Copy.State.NextId++;P.Quantity=1;P.Reserved=0;P.Place=EFoundrylinePalletPlace::MachineInput;P.Owner=I+1;P.Position=Copy.State.Machines[I].InputStock;Copy.State.Pallets.Add(P);if(!Copy.Program(I+1,Outputs[I],1,false,Error))return false;}
  Copy.State.Pallets.RemoveAll([](const auto& P){return !P.Quantity;});if(FMath::Abs(Copy.SteelMass()-Mass)>.1||!Copy.Valid(Error))return false;Copy.State.Running=true;Copy.Step(.1f);M=MoveTemp(Copy);It->ForceNetUpdate();return true;
 }
#endif
 return false;
}
FString UFoundrylineNetworkTestTools::EquipmentMotionReport(APlayerController* Player){
 auto J=MakeShared<FJsonObject>();if(!Player)return TEXT("{}");
 for(TActorIterator<AFoundrylineFactoryPlant> It(Player->GetWorld());It;++It){J->SetBoolField(TEXT("running"),It->Snapshot.Running);for(const auto& M:It->Snapshot.Machines){J->SetNumberField(FString::Printf(TEXT("remaining%d"),M.Id),M.Remaining);int Output=0;for(const auto& P:It->Snapshot.Pallets)if(P.Place==EFoundrylinePalletPlace::MachineOutput&&P.Owner==M.Id)Output+=P.Quantity;J->SetNumberField(FString::Printf(TEXT("output%d"),M.Id),Output);}}
 auto Vector=[&](const FString& Name,FVector V){TArray<TSharedPtr<FJsonValue>> A;for(double N:{V.X,V.Y,V.Z})A.Add(MakeShared<FJsonValueNumber>(N));J->SetArrayField(Name,A);};
 for(TActorIterator<AFoundrylinePlasmaTable> It(Player->GetWorld());It;++It){Vector(TEXT("torch"),It->GetTorchTip());J->SetBoolField(TEXT("cutting"),It->IsCutting());J->SetNumberField(TEXT("sparks"),It->PlasmaEffect->GetVisibleSparkCount());}
 for(TActorIterator<AFoundrylinePressBrakeDisplay> It(Player->GetWorld());It;++It){TArray<UStaticMeshComponent*> C;It->GetComponents(C);for(auto* P:C){if(P->GetStaticMesh()&&P->GetStaticMesh()->GetName()==TEXT("SM_PressBrake_Ram"))J->SetNumberField(TEXT("ram"),P->GetRelativeLocation().Z);if(P->GetFName()==TEXT("BrakeBentPart"))J->SetBoolField(TEXT("bent_part"),P->IsVisible());if(P->GetFName()==TEXT("BrakeBlank"))J->SetBoolField(TEXT("brake_blank"),P->IsVisible());}}
 for(TActorIterator<AFoundrylineMaterialHandlingCell> It(Player->GetWorld());It;++It){TArray<UStaticMeshComponent*> C;It->GetComponents(C);for(auto* P:C){if(P->GetFName()==TEXT("SheetRoller_LowerFront"))J->SetNumberField(TEXT("roller"),P->GetRelativeRotation().Pitch);if(P->GetFName()==TEXT("SheetRoller_Upper"))J->SetNumberField(TEXT("pressure_height"),P->GetRelativeLocation().Z);if(auto* S=Cast<UInstancedStaticMeshComponent>(P);S&&S->GetFName()==TEXT("FormingShellBlank")){J->SetBoolField(TEXT("forming_visible"),S->IsVisible());J->SetNumberField(TEXT("forming_segments"),S->GetInstanceCount());if(S->GetInstanceCount()){FTransform T;S->GetInstanceTransform(0,T);Vector(TEXT("forming_edge"),T.GetLocation());J->SetNumberField(TEXT("forming_angle"),T.Rotator().Pitch);}}}}
 FString Text;auto W=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,W);return Text;
}
