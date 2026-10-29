#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Logistics/FoundrylineTrafficSubsystem.h"
#include "EngineUtils.h"
#include "Async/Async.h"
#include "Engine/World.h"
bool AFoundrylinePlayerController::NetworkAction(FName Action) {
 if(Action==TEXT("NetPanel") || Action==TEXT("TripPanel") || Action==TEXT("TrafficPanel")) {
  LogisticsPanel=Action==TEXT("NetPanel")?0:Action==TEXT("TripPanel")?1:2;bLogisticsOverlay=bLogisticsEdit=true;
  RouteNetwork=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());
  ChooseTool(LogisticsPanel==0?FName(RouteNetwork.Nodes.IsEmpty()?TEXT("NetDraw"):TEXT("NetSelect")):LogisticsPanel==1?FName(TEXT("TripEntrance")):FName(NAME_None));return true;
 }
 if(!Action.ToString().StartsWith(TEXT("Net")) && !Action.ToString().StartsWith(TEXT("Traffic"))) return false;
 bLogisticsOverlay=bLogisticsEdit=true;
 if(Action==TEXT("NetAdvanced")) {bNetworkAdvanced=!bNetworkAdvanced;return true;}
 if(Action==TEXT("NetTwoWay")) {bNetworkTwoWay=!bNetworkTwoWay;return true;}
 if(Action==TEXT("NetReverse")) {bNetworkReverse=!bNetworkReverse;return true;}
 if(Action==TEXT("NetSweep")) {bNetworkSweep=!bNetworkSweep;return true;}
 if(Action==TEXT("NetPlay")) {if(NetworkCheck.Success) bNetworkPlaying=!bNetworkPlaying;return true;}
 if(Action==TEXT("NetNew")) {NetworkSelected.Invalidate();NetworkStart.Invalidate();NetworkFinish.Invalidate();++RouteGeneration;NetworkCheck={};bNetworkPlaying=false;ChooseTool(TEXT("NetDraw"));NetworkStatus=TEXT("Click paving for the first point. Keep clicking to extend. Right-click to finish.");return true;}
 if(Action==TEXT("NetStart") || Action==TEXT("NetFinish")) {if(RouteNetwork.Find(NetworkSelected)) {if(Action==TEXT("NetStart")) NetworkStart=NetworkSelected;else NetworkFinish=NetworkSelected;++RouteGeneration;NetworkCheck={};bNetworkPlaying=false;NetworkStatus=TEXT("Endpoint set. Select another node for the opposite endpoint, then Check Route");}else NetworkStatus=TEXT("Select a node on the map first");return true;}
 if(Action==TEXT("NetTruck") || Action==TEXT("NetBox") || Action==TEXT("NetCar")) {LogisticsTrip.Semi=Action!=TEXT("NetCar");LogisticsTrip.SmallTruck=Action==TEXT("NetBox");LogisticsGateRevision=-1;++RouteGeneration;NetworkCheck={};bNetworkPlaying=false;return true;}
 if(Action==TEXT("NetCheck")) {
  if(bRoutePending) return true;const auto G=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());
  if(!G.Find(NetworkStart) || !G.Find(NetworkFinish) || NetworkStart==NetworkFinish) {NetworkStatus=TEXT("Draw a new route with at least two points. Start and finish are automatic.");return true;}
  const auto M=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>()->GetMap();const auto V=LogisticsTrip.Vehicle();const auto From=NetworkStart,To=NetworkFinish;
  const auto Wait=GetWorld()->GetSubsystem<UFoundrylineTrafficSubsystem>()->GetLedger().Delays();
  NetworkCheck={};bRoutePending=true;NetworkClock=0;bNetworkPlaying=false;const uint32 Generation=++RouteGeneration;NetworkStatus=TEXT("Checking rounded turns and every cab / trailer pose...");TWeakObjectPtr<AFoundrylinePlayerController> Weak=this;
  Async(EAsyncExecution::ThreadPool,[Weak,G,M,V,From,To,Wait,Generation](){auto Result=G.Check(*M,V,From,To,Wait);AsyncTask(ENamedThreads::GameThread,[Weak,Result=MoveTemp(Result),Generation]() mutable {
   if(!Weak.IsValid()) return;Weak->bRoutePending=false;
   if(Weak->RouteGeneration!=Generation || Weak->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->GetRevision()!=Result.Revision) return;
   Weak->NetworkStatus=Result.Reason;Weak->NetworkCheck=MoveTemp(Result);Weak->bNetworkPlaying=Weak->NetworkCheck.Success;
  });});return true;
 }
 if(Action==TEXT("TrafficTest")) {
  // Explicit local rehearsal. Does not create real deliveries or alter the live server ledger.
  FFoundrylineRouteNetwork G;for(int32 I=0;I<5;++I) {FFoundrylineWorldRouteNode N;N.Data.Id=FGuid::NewGuid();N.Position={double((I-2)*4000),0};G.Nodes.Add(N);}
  auto Link=[&](int A,int B){FFoundrylineRouteLink E;E.To=G.Nodes[B].Data.Id;G.Nodes[A].Data.Links.Add(E);};Link(0,1);Link(1,2);Link(2,1);Link(1,0);Link(3,1);Link(1,4);
  FFoundrylineTrafficLedger L;FString Why;const auto V=FFoundrylineVehicleProfile::Truck();const FGuid A=FGuid::NewGuid(),B=FGuid::NewGuid(),C=FGuid::NewGuid();
  auto Enter=[&](FGuid Id,int X,int Y,int Z,float Space,double Time){return L.Enter(G,Id,G.Nodes[X].Data.Id,G.Nodes[Y].Data.Id,G.Nodes[Z].Data.Id,V,Space,Time,Why);};
  const bool Blocked=!Enter(A,0,1,2,1000,0);L.CancelWaiting(A);
  const bool First=Enter(A,0,1,2,5000,1),Opposing=!Enter(B,2,1,0,5000,2),Merge=!Enter(C,3,1,4,5000,3);
  L.RearCleared(A);const bool Fair=!Enter(C,3,1,4,5000,4) && Enter(B,2,1,0,5000,5);L.RearCleared(B);const bool Clear=Enter(C,3,1,4,5000,6);
  TrafficStatus=Blocked && First && Opposing && Merge && Fair && Clear?TEXT("REHEARSAL PASS / blocked exit, opposing traffic, merging, FIFO priority and trailer-clear release"):TEXT("REHEARSAL FAILED / review traffic rules");return true;
 }
 if(Action==TEXT("NetRadius") || Action==TEXT("NetFit") || Action==TEXT("NetStaging") || Action==TEXT("NetDelete")) {
  for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {FGuid Out;if(It->EditRouteNetwork(Action==TEXT("NetFit") && !LogisticsTrip.Semi?FName(TEXT("NetFitCar")):Action==TEXT("NetFit") && LogisticsTrip.SmallTruck?FName(TEXT("NetFitSmall")):Action,NetworkSelected,{},Out)) {NetworkSelected=Out;NetworkStatus=Action==TEXT("NetFit")?TEXT("Vehicle turning radius set. Test the full swept clearance again"):TEXT("Node settings updated. Check the route again");}else NetworkStatus=TEXT("Select an owned route node first");break;}return true;
 }
 ChooseTool(Action);bShowPlanningGrid=false;NetworkStatus=Action==TEXT("NetDraw")?TEXT("Click paving to extend from the selected node. Click an existing node to join"):Action==TEXT("NetUnlink")?TEXT("Click a connected node to remove that link. Two-way removes both directions"):Action==TEXT("NetMove")?TEXT("Click paving to move the selected node. Its links remain attached"):TEXT("Click a node to inspect it, set endpoints, or start a branch");return true;
}
void AFoundrylinePlayerController::NetworkClick(FVector2D Point) {
 if(IsConstructionPending()) return;RouteNetwork=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());
 if(BuildTool==TEXT("NetSelect")) {double Best=FMath::Square(400.);FGuid Selected;for(const auto& N:RouteNetwork.Nodes) {const double D=FVector2D::DistSquared(Point,N.Position);if(D<Best){Best=D;Selected=N.Data.Id;}}NetworkSelected=Selected;NetworkStatus=Selected.IsValid()?TEXT("Point selected. Open POINT SETTINGS to move it, adjust turn space, or mark waiting."):TEXT("Click a numbered node, or choose Draw Route to start");return;}
 if(BuildTool!=TEXT("NetDraw") && BuildTool!=TEXT("NetMove") && BuildTool!=TEXT("NetUnlink")) return;
 if(NetworkSelected.IsValid() && !RouteNetwork.Find(NetworkSelected)) {NetworkStatus=TEXT("Waiting for the selected node to synchronize. If it was deleted, select another node or start New");return;}
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {FGuid Out;if(It->EditRouteNetwork(BuildTool,NetworkSelected,Point,Out,bNetworkTwoWay,bNetworkReverse)) {if(BuildTool==TEXT("NetDraw")) {if(!NetworkStart.IsValid()) NetworkStart=NetworkSelected.IsValid()?NetworkSelected:Out;NetworkFinish=Out;}NetworkSelected=Out;NetworkStatus=bNetworkReverse?TEXT("Backing link added. Keep the cab and trailer aligned; test before use"):TEXT("Point added. Click to continue or join a blue point. Right-click, then Test Drive.");if(BuildTool==TEXT("NetMove")) ChooseTool(TEXT("NetSelect"));}else NetworkStatus=TEXT("Cannot edit here. Use owned paving, respect node limits, and wait for shared edits");break;}
}
FString AFoundrylinePlayerController::GetNetworkSummary() const {
 const auto G=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());int32 Links=0;for(const auto& N:G.Nodes) Links+=N.Data.Links.Num();
 return FString::Printf(TEXT("%lld/%d/%d/%d/%d/%d"),G.Revision,G.Nodes.Num(),Links,G.BrokenLinks,NetworkCheck.Success,NetworkCheck.Route.Poses.Num());
}
