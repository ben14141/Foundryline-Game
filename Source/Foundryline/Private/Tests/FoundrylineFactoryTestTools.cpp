#include "FoundrylineNetworkTestTools.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UI/FoundrylinePlayerController.h"
#include "Containers/Ticker.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/Pawn.h"
bool UFoundrylineNetworkTestTools::PrepareEmptyFactoryZones(APlayerController* Host){
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||Host->GetWorld()->WorldType!=EWorldType::PIE)return false;
 for(TActorIterator<AFoundrylineFactoryPlant> It(Host->GetWorld());It;++It){const auto Solids=It->Model.Solids;It->Model.Seed();It->Model.Solids=Solids;It->Model.State.Zones.Reset();It->Model.State.Walkers.Reset();for(auto& R:It->Model.State.Routes)R.Destinations.RemoveAll([](int V){return V>=2000;});return true;}
#endif
 return false;
}
bool UFoundrylineNetworkTestTools::QueueFactoryPreset(APlayerController* Player,FVector Center,int32 Preset,int32 Quarter,int32 Side){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Center,Preset,Quarter,Side](float){if(Weak.IsValid()){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::AddPresetZone;C.A=Center;C.Id=Preset;C.QuarterTurns=Quarter;C.SupplySide=Side;C.Other=1;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::InspectFactoryZones(APlayerController* Player){
#if WITH_EDITOR
 if(Player&&Player->GetWorld()->WorldType==EWorldType::PIE)for(TActorIterator<AFoundrylineFactoryPlant> It(Player->GetWorld());It;++It){const auto& S=It->Snapshot;if(S.Zones.IsEmpty())return TEXT("empty");const auto& Z=S.Zones[0];return FString::Printf(TEXT("%d/%d/%d/%d/%d"),S.Zones.Num(),Z.Preset,Z.QuarterTurns,Z.SupplySide,S.Routes[0].Destinations.Contains(Z.Id));}
#endif
 return TEXT("unavailable");
}
bool UFoundrylineNetworkTestTools::QueueFactoryConsole(APlayerController* Player,int32 Machine){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||Machine<1||Machine>3)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Machine](float){if(Weak.IsValid()&&Weak->IsWalking()){APawn* Pawn=Weak->GetPawn();const FVector P=(Weak->FactoryPlant()?Weak->FactoryPlant()->ScreenWorldPosition(Machine):AFoundrylineFactoryPlant::ScreenPosition(Machine))+FVector(230,0,-60);Pawn->SetActorLocationAndRotation(P,FRotator(0,180,0),false,nullptr,ETeleportType::TeleportPhysics);Weak->SetControlRotation(FRotator(0,180,0));Weak->SetViewTarget(Pawn);}return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueueFactoryAction(APlayerController* Player,FName Action){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Action](float){if(Weak.IsValid()){if(Action==TEXT("Walk"))Weak->ToggleWalk();else if(Action==TEXT("WalkView")){if(!Weak->IsWalking())Weak->ToggleWalk();}else if(Action==TEXT("BuildView")){if(Weak->IsWalking())Weak->ToggleWalk();if(!Weak->IsWalking()&&Weak->AreRoofsShown())Weak->ToggleRoofs();}else if(Action==TEXT("Interact"))Weak->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::E,IE_Pressed,FPlatformTime::Cycles64()));else Weak->FactoryAction(Action);}return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::AdvanceFactoryTest(APlayerController* Host,int32 Steps){
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||Host->GetWorld()->WorldType!=EWorldType::PIE||Steps<1||Steps>200)return false;
 for(TActorIterator<AFoundrylineFactoryPlant> It(Host->GetWorld());It;++It){FString E;for(int I=0;I<Steps;++I){if(It->Model.Total(EFoundrylinePart::Sheet)<5&&It->Model.Available(EFoundrylinePart::Sheet,EFoundrylinePalletPlace::Receiving,10)==0)It->Model.ReceiveSteel(10,E);if(It->Model.State.Zones.Num()>1&&It->Model.Total(EFoundrylinePart::EndCap)<2)It->Model.ReceiveKits(E);It->Model.Step(.1f);}return true;}
#endif
 return false;
}


FString UFoundrylineNetworkTestTools::PreviewFactoryBrush(APlayerController* Player,FName Tool,FVector Start,FVector End,bool Dragging,bool Commit){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return TEXT("{}");
 PC->bFactoryMenu=true;PC->FactoryPage=2;PC->FactoryTool=Tool;PC->FactoryZonePreset=2;PC->FactoryZoneQuarter=0;PC->FactoryZoneSide=1;PC->FactoryDragStart=Start;PC->FactoryDragEnd=End;PC->bFactoryDragging=Dragging;
 const bool Valid=PC->PreviewFactoryFloor(End);const auto B=PC->FactoryPreviewBounds;const bool Sent=Commit&&PC->CommitFactoryFloorBrush();
 return FString::Printf(TEXT("{\"valid\":%s,\"snapped\":%s,\"sent\":%s,\"min\":[%.6f,%.6f],\"max\":[%.6f,%.6f]}"),Valid?TEXT("true"):TEXT("false"),PC->bFactorySnapped?TEXT("true"):TEXT("false"),Sent?TEXT("true"):TEXT("false"),B.Min.X,B.Min.Y,B.Max.X,B.Max.Y);
#else
 return TEXT("{}");
#endif
}
bool UFoundrylineNetworkTestTools::AimFactoryBrush(APlayerController* Player,FVector Point){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return false;FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(Point,Screen,false))return false;PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::StockRackReview(APlayerController* Host){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Host);if(!PC||!PC->HasAuthority()||PC->GetWorld()->WorldType!=EWorldType::PIE)return TEXT("{}");auto* Plant=PC->FactoryPlant();if(!Plant)return TEXT("{}");
 auto& M=Plant->Model;FString Items;
 for(const auto& S:M.State.Slots){if(M.State.Pallets.ContainsByPredicate([&](const auto& A){return A.Place==EFoundrylinePalletPlace::Rack&&A.Owner==S.Id;}))continue;
  FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=S.Part;A.Quantity=FMath::Max(1,FFoundrylineFactoryCatalog::Part(S.Part).Capacity/2);A.Place=EFoundrylinePalletPlace::Rack;A.Owner=S.Id;A.Position=S.Position;A.UnitLb=1;M.State.Pallets.Add(A);
  Items+=FString::Printf(TEXT("%s{\"slot\":%d,\"part\":%d,\"x\":%.1f,\"y\":%.1f,\"z\":%.1f}"),Items.IsEmpty()?TEXT(""):TEXT(","),S.Id,int(S.Part),S.Position.X,S.Position.Y,S.Position.Z);}
 ++M.State.Revision;return TEXT("{\"slots\":[")+Items+TEXT("]}");
#else
 return TEXT("{}");
#endif
}
