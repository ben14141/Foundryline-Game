#include "Construction/FoundrylineBuildSite.h"
#include "Pathing/FoundrylineRouteNetwork.h"
bool AFoundrylineBuildSite::EditRouteNetwork(FName Action,FGuid Selected,FVector2D Point,FGuid& OutNode,bool TwoWay,bool Reverse) {
 if(!bReady || IsPropertyBusy() || !FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y)) return false;
 const auto G=FFoundrylineRouteNetwork::Read(*Placement);const auto* Old=G.Find(Selected);
 auto Editable=[&](const FFoundrylinePlacedObject& O){const auto* D=Placement->FindDefinition(O.DefinitionId);return D && FFoundrylineDrivingLayout::IsPaved(D->FunctionId) && O.CompanyId==Context.CompanyId && O.PlantId==Context.PlantId;};
 TMap<FGuid,TArray<FFoundrylineRouteNode>> Changes;
 auto Edit=[&](FGuid Host)->TArray<FFoundrylineRouteNode>& {if(!Changes.Contains(Host)) {const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==Host;});Changes.Add(Host,O->RouteNodes);}return Changes.FindChecked(Host);};
 auto Node=[&](FGuid Id)->FFoundrylineRouteNode* {const auto* W=G.Find(Id);return W?Edit(W->Surface).FindByPredicate([&](const auto& V){return V.Id==Id;}):nullptr;};
 auto Allowed=[&](const FFoundrylineWorldRouteNode* N){return N && N->Company==Context.CompanyId && N->Plant==Context.PlantId;};
 OutNode=Selected;
 if(Action==TEXT("NetDelete")) {
  if(!Allowed(Old)) return false;
  for(const auto& O:Placement->GetObjects()) if(Editable(O)) {
   auto Nodes=O.RouteNodes;bool Changed=Nodes.RemoveAll([&](const auto& N){return N.Id==Selected;})>0;
   for(auto& N:Nodes) Changed|=N.Links.RemoveAll([&](const auto& E){return E.To==Selected;})>0;
   if(Changed) Changes.Add(O.ObjectId,Nodes);
  }OutNode.Invalidate();
 } else if(Action==TEXT("NetUnlink")) {
  if(!Allowed(Old)) return false;const FFoundrylineWorldRouteNode* Target=nullptr;double Best=FMath::Square(400.);
  for(const auto& N:G.Nodes) if(Allowed(&N) && N.Data.Id!=Selected) {const double D=FVector2D::DistSquared(Point,N.Position);if(D<Best) {Best=D;Target=&N;}}
  if(!Target) return false;
  Node(Selected)->Links.RemoveAll([&](const auto& E){return E.To==Target->Data.Id;});
  if(TwoWay) Node(Target->Data.Id)->Links.RemoveAll([&](const auto& E){return E.To==Selected;});
 } else if(Action==TEXT("NetRadius") || Action==TEXT("NetFit") || Action==TEXT("NetFitSmall") || Action==TEXT("NetFitCar") || Action==TEXT("NetStaging")) {
  if(!Allowed(Old)) return false;auto* N=Node(Selected);
  if(Action==TEXT("NetStaging")) N->Staging=!N->Staging;
  else N->RadiusCm=Action==TEXT("NetFitCar")?400:Action==TEXT("NetFitSmall")?700:Action==TEXT("NetFit")?1000:N->RadiusCm>=2000?1000:N->RadiusCm+200;
 } else if(Action==TEXT("NetDraw") || Action==TEXT("NetMove")) {
  FIntVector WorldPoint;
  if(!FFoundrylineGrid::SnapWorld(FVector(Point,0),EFoundrylineSnap::Structural100cm,WorldPoint)) return false;
  Point={double(WorldPoint.X),double(WorldPoint.Y)};
  if(Action==TEXT("NetMove") && !Allowed(Old)) return false;
  const FFoundrylineWorldRouteNode* Nearest=nullptr;double Best=FMath::Square(250.);
  for(const auto& N:G.Nodes) if(Allowed(&N)) {const double D=FVector2D::DistSquared(Point,N.Position);if(D<Best) {Best=D;Nearest=&N;}}
  FGuid Target;FGuid Host;FIntPoint Local=FIntPoint::ZeroValue;
  if(Action==TEXT("NetDraw") && Nearest) {Target=Nearest->Data.Id;Host=Nearest->Surface;}
  else {
   const FFoundrylinePlacedObject* Surface=nullptr;
   for(const auto& O:Placement->GetObjects()) if(Editable(O)) {const auto* D=Placement->FindDefinition(O.DefinitionId);const auto P=FFoundrylineGrid::RotateOffset(WorldPoint-O.Anchor,(4-O.QuarterTurns%4)%4);if(P.X>=0 && P.Y>=0 && P.X<=D->Footprint.Max.X && P.Y<=D->Footprint.Max.Y) {Surface=&O;Local={P.X,P.Y};break;}}
   if(!Surface) {Status=TEXT("Place nodes on your roads, driveway or paved yard");return false;}
   Host=Surface->ObjectId;Target=Action==TEXT("NetMove")?Selected:FGuid::NewGuid();
   if(Action==TEXT("NetMove")) {auto Copy=*Node(Selected);Edit(Old->Surface).RemoveAll([&](const auto& N){return N.Id==Selected;});Copy.Position=Local;Edit(Host).Add(Copy);}
   else {FFoundrylineRouteNode N;N.Id=Target;N.Position=Local;Edit(Host).Add(N);}
  }
  if(Action==TEXT("NetDraw") && Allowed(Old) && Selected!=Target) {
   auto Add=[&](FGuid A,FGuid B){auto* N=A==Target?Edit(Host).FindByPredicate([&](const auto& V){return V.Id==A;}):Node(A);if(!N->Links.ContainsByPredicate([&](const auto& E){return E.To==B;})){FFoundrylineRouteLink L;L.To=B;L.Reverse=Reverse;N->Links.Add(L);}};
   Add(Selected,Target);if(TwoWay) Add(Target,Selected);
  }OutNode=Target;
 } else return false;
 if(Changes.IsEmpty()) return true;
 TArray<FFoundrylinePlacementRequest> Requests;
 for(const auto& Pair:Changes) {FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::SetRouteNodes;R.ExpectedRevision=Placement->GetRevision();R.ObjectId=Pair.Key;R.RouteNodes=Pair.Value;Requests.Add(R);}
 Requests.Sort([&](const auto& A,const auto& B){return Action==TEXT("NetMove") && Old && A.ObjectId==Old->Surface && B.ObjectId!=Old->Surface;});
 const auto R=ExecuteConstructionBatch(Requests);if(!R.IsSuccess() && R.Error!=EFoundrylinePlacementError::Pending) {Status=PlacementErrorText(R.Error);return false;}return true;
}
