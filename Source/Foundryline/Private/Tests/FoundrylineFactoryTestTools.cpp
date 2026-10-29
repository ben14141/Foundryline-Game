#include "FoundrylineNetworkTestTools.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Employees/FoundrylineWeldingHoodComponent.h"
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

bool UFoundrylineNetworkTestTools::QueueWeldingStation(APlayerController* Player,FVector Center,int32 Quarter,bool bSwap,bool bForeignPlant,bool bRemove,int32 StationId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Center,Quarter,bSwap,bForeignPlant,bRemove,StationId](float){if(Weak.IsValid())if(auto* Plant=Weak->FactoryPlant()){FFoundrylineFactoryCommand C;C.Action=bRemove?EFoundrylineFactoryAction::RemoveWeldingStation:EFoundrylineFactoryAction::AddWeldingStation;C.A=Center;C.QuarterTurns=Quarter;C.SupplySide=bSwap?-1:1;C.Id=StationId;C.TargetPlantId=bForeignPlant?FGuid::NewGuid():Plant->BusinessPlantId;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::QueueFactoryLane(APlayerController* Player,int32 Kind,FVector A,FVector B,float Width){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;TWeakObjectPtr<AFoundrylinePlayerController> Weak=PC;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Kind,A,B,Width](float){if(Weak.IsValid()){FFoundrylineFactoryCommand C;C.Action=EFoundrylineFactoryAction::PaintLane;C.Id=Kind;C.A=A;C.B=B;C.WidthCm=Width;C.Other=0;Weak->SendFactory(C);}return false;}));return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::WeldingStationReport(APlayerController* Player){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC)return TEXT("{}");auto* Plant=PC->FactoryPlant();if(!Plant)return TEXT("{}");
 // The host reads its authoritative model, so short events such as a pallet on IN are not missed between snapshots.
 FFoundrylineFactorySimulation View;View.State=Plant->HasAuthority()?Plant->Model.State:Plant->Snapshot;
 FString Items;for(const auto& S:View.State.WeldingStations){FString Bins;for(auto P:FFoundrylineFactoryCatalog::AssemblyParts())Bins+=FString::Printf(TEXT("%s\"%d\":[%d,%d,%d]"),Bins.IsEmpty()?TEXT(""):TEXT(","),int(P),View.StationStock(S,P,false),View.StationStock(S,P),View.StationTarget(S,P));
  const auto* In=View.State.Pallets.FindByPredicate(
[&](const auto& A){return A.Place==EFoundrylinePalletPlace::StationIn&&A.Owner==S.Id;});
  Items+=FString::Printf(TEXT("%s{\"id\":%d,\"number\":%d,\"x\":%.2f,\"y\":%.2f,\"quarter\":%d,\"swap\":%s,\"recipe\":%d,\"batch\":%d,\"in_part\":%d,\"bins\":{%s},\"supply\":\"%s\",\"stage\":%d,\"work\":%.2f,\"advancing\":%s,\"welder_ready\":%s,\"units\":%d,\"out\":%d,\"welders\":[%s],\"at_work\":%d,\"rate\":%.2f,\"guns_in_hand\":%d,\"sparks\":%d}"),Items.IsEmpty()?TEXT(""):TEXT(","),S.Id,S.Number,S.Center.X,S.Center.Y,S.QuarterTurns,S.bSwapSides?TEXT("true"):TEXT("false"),S.Recipe,S.BatchRemaining,In?int(In->Part):-1,*Bins,*View.WeldingStationStatus(S),
   S.Stage,S.Work,S.bAdvancing?TEXT("true"):TEXT("false"),View.StationWelderReady(S)?TEXT("true"):TEXT("false"),S.UnitsCompleted,View.OutTotal(S),
   *FString::Printf(TEXT("%d,%d"),View.StationWelder(S,0)?View.StationWelder(S,0)->Id:0,View.StationWelder(S,1)?View.StationWelder(S,1)->Id:0),View.StationWelders(S,true),View.StationRate(S),Plant->StationGunsInHand(View.State.WeldingStations.IndexOfByPredicate([&](const auto& V){return V.Id==S.Id;})),Plant->StationSparksActive(View.State.WeldingStations.IndexOfByPredicate([&](const auto& V){return V.Id==S.Id;})));}
 return FString::Printf(TEXT("{\"authority\":%s,\"lanes\":%d,\"revision\":%d,\"status\":\"%s\",\"stations\":[%s]}"),Plant->HasAuthority()?TEXT("true"):TEXT("false"),Plant->Snapshot.Lanes.Num(),Plant->Snapshot.Revision,*PC->FactoryStatus.Replace(TEXT("\""),TEXT("'")),*Items);
#else
 return TEXT("{}");
#endif
}
bool UFoundrylineNetworkTestTools::SetWeldingOutStacks(APlayerController* Host,int32 StationId,int32 First,int32 Second){
#if WITH_EDITOR
 using Layout=FFoundrylineWeldingStationLayout;auto* PC=Cast<AFoundrylinePlayerController>(Host);if(!PC||!PC->HasAuthority()||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;auto* Plant=PC->FactoryPlant();if(!Plant)return false;
 auto& M=Plant->Model;const auto* S=M.State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==StationId;});if(!S||First<0||Second<0||First>Layout::RacksPerOutPallet||Second>Layout::RacksPerOutPallet)return false;
 M.State.Pallets.RemoveAll([&](const auto& A){return A.Owner==StationId&&A.Place==EFoundrylinePalletPlace::StationOut;});
 for(int Slot=0;Slot<2;++Slot)if(const int Q=Slot?Second:First){FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=EFoundrylinePart::HeadacheRack;A.Quantity=Q;A.Place=EFoundrylinePalletPlace::StationOut;A.Owner=StationId;A.Position=Layout::OutPalletPoint(*S,Slot);A.UnitLb=2*2+4*5+6*16+2*4.5f;M.State.Pallets.Add(A);}
 ++M.State.Revision;PC->FactoryStation=StationId;FString E;return M.Valid(E);
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SetupWeldingAssemblyReview(APlayerController* Host,int32 StationId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Host);if(!PC||!PC->HasAuthority()||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;auto* Plant=PC->FactoryPlant();if(!Plant)return false;
 auto& M=Plant->Model;const auto* S=M.State.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==StationId;});if(!S)return false;
 for(auto [P,Q,Lb]:{TTuple<EFoundrylinePart,int,float>{EFoundrylinePart::MountingPlate,4,2},{EFoundrylinePart::Bracket,8,5},{EFoundrylinePart::FrameSection,12,16},{EFoundrylinePart::LightKit,4,4.5f}}){
  // Top up only what the bins can still take, counting deliveries already booked for them.
  const int Room=FMath::Min(Q,FFoundrylineWeldingStationLayout::BinCapacity(P)-M.StationStock(*S,P));if(Room<=0)continue;
  FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=Room;A.Place=EFoundrylinePalletPlace::Buffer;A.Owner=StationId;A.Position=FFoundrylineWeldingStationLayout::BinPoint(*S,P);A.UnitLb=Lb;M.State.Pallets.Add(A);}
 M.State.Running=true;++M.State.Revision;PC->FactoryStation=StationId;return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SetupWeldingSupplyReview(APlayerController* Host,int32 StationId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Host);if(!PC||!PC->HasAuthority()||!PC->IsLocalController()||PC->GetWorld()->WorldType!=EWorldType::PIE)return false;auto* Plant=PC->FactoryPlant();if(!Plant)return false;
 auto& M=Plant->Model;auto Stock=[&](EFoundrylinePart P,int Slot,int Q,float Lb){auto* S=M.State.Slots.FindByPredicate([&](const auto& V){return V.Id==Slot;});if(!S)return;S->Part=P;FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=Q;A.Place=EFoundrylinePalletPlace::Rack;A.Owner=Slot;A.Position=S->Position;A.UnitLb=Lb;M.State.Pallets.Add(A);};
 Stock(EFoundrylinePart::Bracket,107,20,5);Stock(EFoundrylinePart::MountingPlate,108,10,2);M.State.Running=true;++M.State.Revision;PC->FactoryStation=StationId;return true;
#else
 return false;
#endif
}
bool UFoundrylineNetworkTestTools::SelectWeldingStation(APlayerController* Player,int32 StationId){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return false;PC->FactoryStation=StationId;PC->FactoryTool=NAME_None;return true;
#else
 return false;
#endif
}
FString UFoundrylineNetworkTestTools::PreviewWeldingStation(

APlayerController* Player,FVector Center,int32 Quarter,bool bSwap,bool bCommit){
#if WITH_EDITOR
 auto* PC=Cast<AFoundrylinePlayerController>(Player);if(!PC||!PC->IsLocalController())return TEXT("{}");
 PC->bFactoryMenu=true;PC->FactoryTool=TEXT("Station");PC->FactoryStationQuarter=Quarter;PC->bFactoryStationSwap=bSwap;PC->FactoryStationCheckKey.Reset();PC->bFactoryEdgeSnap=false;
 const bool Valid=PC->PreviewFactoryFloor(Center);const bool Sent=bCommit&&PC->CommitFactoryFloorBrush();
 return FString::Printf(TEXT("{\"sent\":%s,\"valid\":%s,\"hint\":\"%s\",\"x\":%.2f,\"y\":%.2f}"),Sent?TEXT("true"):TEXT("false"),Valid?TEXT("true"):TEXT("false"),*PC->FactoryPlacementHint.Replace(TEXT("\""),TEXT("'")),PC->FactoryPreviewA.X,PC->FactoryPreviewA.Y);
#else
 return TEXT("{}");
#endif
}
FString UFoundrylineNetworkTestTools::WeldingHoodReport(APlayerController* Player){
#if WITH_EDITOR
 if(!Player)return TEXT("{}");int32 Equipped=0,Closed=0;FString Angles;
 for(TActorIterator<AActor> It(Player->GetWorld());It;++It){TArray<UFoundrylineWeldingHoodComponent*> Hoods;It->GetComponents(Hoods);
  for(auto* H:Hoods){if(!H->IsEquipped())continue;++Equipped;Closed+=H->IsVisorClosed();Angles+=FString::Printf(TEXT("%s%.0f"),Angles.IsEmpty()?TEXT(""):TEXT(","),H->GetVisorAngleDegrees());}}
 return FString::Printf(TEXT("{\"equipped\":%d,\"closed\":%d,\"angles\":[%s]}"),Equipped,Closed,*Angles);
#else
 return TEXT("{}");
#endif
}
