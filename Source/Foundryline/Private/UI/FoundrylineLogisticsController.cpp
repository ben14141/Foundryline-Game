#include "UI/FoundrylinePlayerController.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "EngineUtils.h"
void AFoundrylinePlayerController::ToggleLogisticsOverlay() {
 if(IsWalking()) return;bLogisticsOverlay=!bLogisticsOverlay;
 if(!bLogisticsOverlay) {bLogisticsEdit=false;bTripPlaying=false;ChooseTool(NAME_None);}
}
void AFoundrylinePlayerController::ToggleLogisticsEdit() {
 if(IsWalking()) return;bLogisticsEdit=!bLogisticsEdit;
 if(bLogisticsEdit) {bLogisticsOverlay=true;if(BuildTool.IsNone() || !BuildTool.ToString().StartsWith(TEXT("Trip"))) ChooseLogisticsTool(TEXT("TripEntrance"));}
 else ChooseTool(NAME_None);
}
void AFoundrylinePlayerController::CancelBuild() {
 if(bLogisticsEdit) {if(LogisticsPanel==0) {ChooseTool(TEXT("NetSelect"));NetworkStatus=TEXT("Route finished. Press TEST DRIVE to check every turn and the full trailer.");}bDraggingBuild=false;LogisticsStatus=TEXT("Selection cancelled. Choose a step in the sidebar to continue");return;}ChooseTool(NAME_None);
}
void AFoundrylinePlayerController::InvalidateLogisticsTest() {++RouteGeneration;TripResult={};TripPreviewPoses.Reset();TripPoseTimes.Reset();bTripPlaying=false;TripPreviewClock=0;}
void AFoundrylinePlayerController::ChooseLogisticsTool(FName Tool) {
 ChooseTool(Tool);bShowPlanningGrid=false;
 LogisticsStatus=Tool==TEXT("TripEntrance")?TEXT("Click a numbered entrance where your driveway meets the public street"):
 Tool==TEXT("RoutePaint")?TEXT("Drag a filled truck driving area on paving. Leave turning room for the trailer"):
 Tool==TEXT("RouteErase")?TEXT("Drag over green areas to remove truck access"):
 Tool==TEXT("TripDestination")?(LogisticsTrip.Semi?TEXT("Click a numbered amber loading door or staging bay"):TEXT("Click a blue parking bay")):
 Tool==TEXT("TripExit")?TEXT("Click a numbered gate to choose the exit. Entrance and exit may share a gate"):TEXT("Choose a step, then test the whole trip");
}
void AFoundrylinePlayerController::LogisticsAction(FName Action) {
 if(!IsLocalController()) return;
 if(NetworkAction(Action)) return;
 if(Action==TEXT("ShowBothRoutes") || Action==TEXT("ShowArrival") || Action==TEXT("ShowDeparture")) {LogisticsRouteView=Action==TEXT("ShowArrival")?1:Action==TEXT("ShowDeparture")?2:0;return;}
 if(Action==TEXT("TruckLayer")) {bShowTruckLayer=!bShowTruckLayer;return;}
 if(Action==TEXT("ParkingLayer")) {bShowParkingLayer=!bShowParkingLayer;return;}
 bLogisticsEdit=true;bLogisticsOverlay=true;
 if(Action==TEXT("TripPlay")) {if(TripResult.Success) bTripPlaying=!bTripPlaying;return;}
 if(Action==TEXT("TripRestart")) {TripPreviewClock=0;bTripPlaying=TripResult.Success;return;}
 if(Action==TEXT("TripTruck") || Action==TEXT("TripBox") || Action==TEXT("TripCar")) {
  const bool Semi=Action!=TEXT("TripCar"),Small=Action==TEXT("TripBox");if(LogisticsTrip.Semi!=Semi || LogisticsTrip.SmallTruck!=Small) {LogisticsTrip={};LogisticsTrip.Semi=Semi;LogisticsTrip.SmallTruck=Small;LogisticsGateRevision=-1;InvalidateLogisticsTest();}
  bShowTruckLayer=Semi;bShowParkingLayer=!Semi;ChooseLogisticsTool(TEXT("TripEntrance"));return;
 }
 if(Action==TEXT("TripTest")) {
  if(bRoutePending) return;const auto M=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();
  if(!LogisticsTrip.Ready(*M)) {LogisticsStatus=TEXT("Complete ENTRANCE, DESTINATION and EXIT before testing");return;}
  InvalidateLogisticsTest();bRoutePending=true;ChooseTool(TEXT("TripTest"));bShowPlanningGrid=false;
  LogisticsStatus=TEXT("Checking the complete trip: arrival, destination fit and departure...");
  const auto Trip=LogisticsTrip;const uint32 Generation=RouteGeneration;TWeakObjectPtr<AFoundrylinePlayerController> Weak=this;
  Async(EAsyncExecution::ThreadPool,[Weak,M,Trip,Generation](){auto R=FFoundrylineLogisticsTrip::Test(*M,Trip);
   AsyncTask(ENamedThreads::GameThread,[Weak,R=MoveTemp(R),Generation]() mutable {
    if(!Weak.IsValid()) return;Weak->bRoutePending=false;if(Weak->RouteGeneration!=Generation) return;
    if(Weak->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->GetRevision()!=R.Revision) {Weak->LogisticsStatus=TEXT("The layout changed. Test the trip again");return;}
    Weak->LogisticsStatus=R.Reason;Weak->TripResult=MoveTemp(R);Weak->TripPreviewClock=0;Weak->bTripPlaying=Weak->TripResult.Success;
    if(Weak->TripResult.Success) {
     float Clock=0;auto AddLeg=[&](const FFoundrylineVehicleRoute& Leg){for(int32 I=0;I<Leg.Poses.Num();++I) {if(I) Clock+=FVector2D::Distance(Leg.Poses[I-1].Position,Leg.Poses[I].Position)/(Leg.Poses[I].Reverse?200.f:600.f);Weak->TripPreviewPoses.Add(Leg.Poses[I]);Weak->TripPoseTimes.Add(Clock);}};
     AddLeg(Weak->TripResult.Inbound);Clock+=3;AddLeg(Weak->TripResult.Outbound);
    }
   });
  });return;
 }
 // Legacy editor helpers use the same guided actions.
 if(Action==TEXT("RouteCar")) {LogisticsAction(TEXT("TripCar"));return;}
 if(Action==TEXT("RouteSemi")) {LogisticsAction(TEXT("TripTruck"));return;}
 ChooseLogisticsTool(Action);
}
void AFoundrylinePlayerController::SelectLogisticsPoint(FVector Point) {if(IsLocalController() && bLogisticsEdit) LogisticsClick(FIntVector(FMath::RoundToInt(Point.X),FMath::RoundToInt(Point.Y),0));}
void AFoundrylinePlayerController::LogisticsClick(FIntVector Cursor) {
 if(LogisticsPanel==0) {NetworkClick(FVector2D(Cursor.X,Cursor.Y));return;}
 if(BuildTool==TEXT("RoutePaint") || BuildTool==TEXT("RouteErase")) {DragStart=DragEnd=Cursor;bDraggingBuild=true;return;}
 if(BuildTool!=TEXT("TripEntrance") && BuildTool!=TEXT("TripDestination") && BuildTool!=TEXT("TripExit")) return;
 const auto M=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();FString Message;const auto Step=BuildTool;
 if(LogisticsTrip.Select(*M,Step,FVector2D(Cursor.X,Cursor.Y),Message)) {
  InvalidateLogisticsTest();if(Step==TEXT("TripEntrance")) ChooseLogisticsTool(LogisticsTrip.Semi?TEXT("RoutePaint"):TEXT("TripDestination"));
  else if(Step==TEXT("TripDestination")) ChooseLogisticsTool(TEXT("TripExit"));else ChooseLogisticsTool(TEXT("TripTest"));
 }
 LogisticsStatus=Message;
}
FString AFoundrylinePlayerController::GetLogisticsSummary() const {
 return FString::Printf(TEXT("%s/%d/%d/%d/%d/%d/%d"),LogisticsTrip.Semi?TEXT("TRUCK"):TEXT("CAR"),LogisticsTrip.HasEntry,LogisticsTrip.TargetId.IsValid(),LogisticsTrip.HasExit,bRoutePending,TripResult.Success,TripPreviewPoses.Num());
}
bool AFoundrylinePlayerController::LogisticsPreviewPose(FFoundrylineVehiclePose& Pose,FString& Phase) const {
 if(!TripResult.Success || TripPreviewPoses.Num()<2 || TripPoseTimes.Last()<=0) return false;
 const float Clock=FMath::Fmod(TripPreviewClock,TripPoseTimes.Last()+2);int32 I=0;while(I+1<TripPoseTimes.Num() && TripPoseTimes[I+1]<Clock) ++I;
 Pose=TripPreviewPoses[I];if(I+1<TripPoseTimes.Num()) {const float A=FMath::Clamp((Clock-TripPoseTimes[I])/FMath::Max(.001f,TripPoseTimes[I+1]-TripPoseTimes[I]),0.f,1.f);const auto& B=TripPreviewPoses[I+1];Pose.Position=FMath::Lerp(Pose.Position,B.Position,A);Pose.Yaw+=FMath::FindDeltaAngleRadians(Pose.Yaw,B.Yaw)*A;Pose.TrailerYaw+=FMath::FindDeltaAngleRadians(Pose.TrailerYaw,B.TrailerYaw)*A;}
 const int32 Arrived=TripResult.Inbound.Poses.Num()-1;
 Phase=I<Arrived?(Pose.Reverse?TEXT("REVERSING TO DESTINATION"):TEXT("ARRIVING")):I==Arrived?TEXT("STOPPED AT DESTINATION"):TEXT("DEPARTING");return true;
}
void AFoundrylinePlayerController::DrawLogistics() {
 auto* Paths=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>();const auto M=Paths->GetMap();
 if(LogisticsGateRevision!=M->Revision) {
  const bool HadSelected=RouteNetwork.Find(NetworkSelected)!=nullptr,HadStart=RouteNetwork.Find(NetworkStart)!=nullptr,HadFinish=RouteNetwork.Find(NetworkFinish)!=nullptr;
  RouteNetwork=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());
  if(NetworkCheck.Revision>=0 && NetworkCheck.Revision!=M->Revision) {NetworkCheck={};bNetworkPlaying=false;NetworkStatus=TEXT("Shared layout changed. Check the route again");}
  if(HadSelected && !RouteNetwork.Find(NetworkSelected)) NetworkSelected.Invalidate();
  if(HadStart && !RouteNetwork.Find(NetworkStart)) NetworkStart.Invalidate();
  if(HadFinish && !RouteNetwork.Find(NetworkFinish)) NetworkFinish.Invalidate();
  TruckGates=FFoundrylineLogisticsTrip::Gates(*M,true,LogisticsTrip.SmallTruck);CarGates=FFoundrylineLogisticsTrip::Gates(*M,false);LogisticsGateRevision=M->Revision;
  const auto& Gates=LogisticsTrip.Semi?TruckGates:CarGates;
  auto Refresh=[&](FFoundrylineLogisticsGate& Old,bool& Present){if(!Present) return;if(const auto* Current=Gates.FindByPredicate([&](const auto& G){return G.SurfaceId==Old.SurfaceId;})) Old=*Current;else Present=false;};
  Refresh(LogisticsTrip.Entrance,LogisticsTrip.HasEntry);Refresh(LogisticsTrip.Departure,LogisticsTrip.HasExit);
  if(LogisticsTrip.TargetId.IsValid() && !M->Targets.ContainsByPredicate([&](const auto& T){return T.Id==LogisticsTrip.TargetId;})) LogisticsTrip.TargetId.Invalidate();
 }
 if(TripResult.Revision>=0 && TripResult.Revision!=M->Revision) {InvalidateLogisticsTest();LogisticsStatus=TEXT("Access or layout changed. Test the trip again");}
 if(bLogisticsEdit && bDraggingBuild && !IsOverBuildHUD()) {FIntVector P;if(GetCursorAnchor(P)) DragEnd=P;}
}
