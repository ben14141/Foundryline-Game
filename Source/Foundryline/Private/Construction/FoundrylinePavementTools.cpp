#include "Algo/Reverse.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Pathing/FoundrylineDrivingLayout.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePavementGeometry.h"
#include "Misc/Crc.h"
namespace {
using G=FFoundrylinePavementGeometry;using Poly=G::FPolygon;
FTransform Pose(const FFoundrylinePlacedObject& O){return FTransform(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));}
void SnapStreet(TArray<FVector2D>& Controls,const UFoundrylinePlacementSubsystem* Placement,bool Spline=false){
 if(Controls.Num()<2)return;double Best=201;int EndIndex=-1;FVector2D Target,Normal;
 for(int Index:{0,Controls.Num()-1})for(const auto& O:Placement->GetObjects()){
  if(O.CompanyId!=FGuid(0xF001,99,1,1))continue;const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D||!FFoundrylineDrivingLayout::IsPaved(D->FunctionId))continue;const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,O.Anchor,O.QuarterTurns);const auto P=Controls[Index];
  const FVector2D Min(B.Min.X,B.Min.Y),Max(B.Max.X,B.Max.Y);for(int Side=0;Side<4;++Side){const bool X=Side%2==1;const double Edge=Side==0?Min.Y:Side==1?Max.X:Side==2?Max.Y:Min.X,Along=X?P.Y:P.X,Low=X?Min.Y:Min.X,High=X?Max.Y:Max.X,Distance=FMath::Abs((X?P.X:P.Y)-Edge);if(Along<Low+50||Along>High-50||Distance>=Best)continue;const FVector2D Out=Side==0?FVector2D(0,-1):Side==1?FVector2D(1,0):Side==2?FVector2D(0,1):FVector2D(-1,0);const auto Snap=X?FVector2D(Edge,P.Y):FVector2D(P.X,Edge);if(FVector2D::DotProduct(P-Snap,Out)<-.1)continue;if(Controls.Num()==2){const auto Direction=(Controls[1]-Controls[0]).GetSafeNormal();if(FMath::Abs(Direction.X*Out.Y-Direction.Y*Out.X)>.05)continue;}Best=Distance;EndIndex=Index;Target=Snap;Normal=Out;}
 }
 if(EndIndex<0)return;Controls[EndIndex]=Target;if(Controls.Num()==3||Spline){const int Handle=Spline?(EndIndex==0?1:Controls.Num()-2):1;const double Reach=FMath::Max(400.,FVector2D::DotProduct(Controls[Handle]-Target,Normal));Controls[Handle]=Target+Normal*Reach;}
}
// Local magnetism to owned paving. Public street alignment remains the priority.
void SnapPaving(TArray<FVector2D>& Controls,const UFoundrylinePlacementSubsystem* P,FGuid Company,FGuid Exclude={},bool Spline=false){
 if(Controls.Num()<2)return;SnapStreet(Controls,P,Spline);uint8 PublicEnds=0;
 for(const auto& O:P->GetObjects())if(O.CompanyId==FGuid(0xF001,99,1,1)){const auto* D=P->FindDefinition(O.DefinitionId);if(!D||!FFoundrylineDrivingLayout::IsPaved(D->FunctionId))continue;const auto T=Pose(O).Inverse();for(int E:{0,Controls.Num()-1}){const auto L=FVector2D(T.TransformPosition(FVector(Controls[E],0)));if(L.X>=-1&&L.Y>=-1&&L.X<=D->Footprint.Max.X+1&&L.Y<=D->Footprint.Max.Y+1)PublicEnds|=E==0?1:2;}}
 for(int End:{0,Controls.Num()-1}){if(PublicEnds&(End==0?1:2))continue;double Best=100;FVector2D Target=Controls[End],Normal;bool Found=false;
  const auto Inward=(Controls[End==0?1:Controls.Num()-2]-Controls[End]).GetSafeNormal();
  for(const auto& O:P->GetObjects()){if(O.ObjectId==Exclude||O.CompanyId!=Company)continue;const auto* D=P->FindDefinition(O.DefinitionId);if(!D||!FFoundrylineDrivingLayout::IsPaved(D->FunctionId))continue;const auto T=Pose(O);const auto Raw=O.Pavement.Polygons.IsEmpty()?G::Rectangle({0,0},{double(D->Footprint.Max.X),double(D->Footprint.Max.Y)}):O.Pavement.Polygons;
   for(const auto& Poly:Raw)for(int I=0;I<Poly.Outer.Num();++I){const auto A=FVector2D(T.TransformPosition(FVector(Poly.Outer[I],0))),B=FVector2D(T.TransformPosition(FVector(Poly.Outer[(I+1)%Poly.Outer.Num()],0))),Edge=(B-A).GetSafeNormal(),Out=FVector2D(Edge.Y,-Edge.X);if(Controls.Num()==2&&FMath::Abs(Inward.X*Out.Y-Inward.Y*Out.X)>.12)continue;
    const auto Q=FVector2D(FMath::ClosestPointOnSegment(FVector(Controls[End],0),FVector(A,0),FVector(B,0)));const double Distance=FVector2D::Distance(Q,Controls[End]);if(Distance<Best&&FVector2D::DotProduct(Inward,Out)>.2){Best=Distance;Target=Q;Normal=Out;Found=true;}
   }
  }
  if(Found){Controls[End]=Target;if((Controls.Num()==3||Spline)&&!PublicEnds){const int Handle=Spline?(End==0?1:Controls.Num()-2):1;const double Reach=FMath::Max(400.,FVector2D::DotProduct(Controls[Handle]-Target,Normal));Controls[Handle]=Target+Normal*Reach;}}
 }
}
void EntranceConnections(FFoundrylinePlacedObject& O,const UFoundrylinePlacementSubsystem* P,const TArray<FFoundrylinePlacedObject>* Future=nullptr){
 auto& D=O.Pavement;D.EntranceEnds=0;if(!D.Lanes||D.EntranceRadiusCm<=0||D.RoadControls.Num()<2)return;const auto T=Pose(O);
 for(int End=0;End<2;++End){const auto& C=D.RoadControls;const auto At=End?C.Last():C[0],Inward=End?(C[C.Num()-2]-At).GetSafeNormal():(C[1]-At).GetSafeNormal();const auto W=T.TransformPosition(FVector(At-Inward*5,0));
  for(const auto& Other:Future?*Future:P->GetObjects()){if(Other.ObjectId==O.ObjectId)continue;const auto* Def=P->FindDefinition(Other.DefinitionId);if(!Def||!FFoundrylineDrivingLayout::IsPaved(Def->FunctionId))continue;const auto Local=FVector2D(Pose(Other).InverseTransformPosition(W));
   if(Other.Pavement.Polygons.IsEmpty()?(Local.X>=-.1&&Local.Y>=-.1&&Local.X<=Def->Footprint.Max.X+.1&&Local.Y<=Def->Footprint.Max.Y+.1):G::Contains(Other.Pavement.Polygons,Local,.1)){D.EntranceEnds|=1<<End;break;}
  }
 }
}
// Rebase the complete saved record: markings and logistics keep their world positions.
bool Normalize(FFoundrylinePlacedObject& O,UFoundrylinePlacementSubsystem* P,bool KeepEnvelope=false){
 auto& D=O.Pavement;auto Orient=[](TArray<FVector2D>& V,bool Clockwise){double Area=0;for(int I=0;I<V.Num();++I){const auto A=V[I],B=V[(I+1)%V.Num()];Area+=A.X*B.Y-B.X*A.Y;}if((Area<0)!=Clockwise)Algo::Reverse(V);};for(auto& P:D.Polygons){Orient(P.Outer,false);for(auto& H:P.Holes)Orient(H.Points,true);}auto B=G::Bounds(D.Polygons);if(!B.bIsValid)return false;
 if(KeepEnvelope)if(const auto* Def=P->FindDefinition(O.DefinitionId)){B+=FVector2D(0,0);B+=FVector2D(Def->Footprint.Max.X,Def->Footprint.Max.Y);}
 const FVector2D Low(FMath::FloorToDouble((B.Min.X+.01)/25)*25,FMath::FloorToDouble((B.Min.Y+.01)/25)*25),High(FMath::CeilToDouble((B.Max.X-.01)/25)*25,FMath::CeilToDouble((B.Max.Y-.01)/25)*25);
 const int W=FMath::Max(100,FMath::RoundToInt(High.X-Low.X)),H=FMath::Max(100,FMath::RoundToInt(High.Y-Low.Y));
 const FName Id(*FString::Printf(TEXT("Pavement_%d_%d"),W,H));if(!P->EnsureGeneratedDefinition(Id))return false;
 G::Translate(D,-Low);O.Anchor+=FIntVector(FRotator(0,O.QuarterTurns*90,0).RotateVector(FVector(Low,0)));
 for(auto& N:O.RouteNodes){N.Position-=FIntPoint(FMath::RoundToInt(Low.X),FMath::RoundToInt(Low.Y));}
 for(auto& Z:O.TruckZones){Z.Min-=FIntPoint(FMath::RoundToInt(Low.X),FMath::RoundToInt(Low.Y));Z.Max-=FIntPoint(FMath::RoundToInt(Low.X),FMath::RoundToInt(Low.Y));}
 O.DefinitionId=Id;return true;
}
void AsArea(FFoundrylinePavementData& D){D.Lanes=0;D.Spline=false;D.RoadWidthCm=0;D.RoadControls.Reset();D.EntranceRadiusCm=0;D.EntranceEnds=0;}
FFoundrylinePlacementRequest Replace(const FFoundrylinePlacedObject& O,int64 Revision){FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=O.ObjectId;R.ExpectedRevision=Revision;R.Replacement=O;return R;}
FFoundrylinePlacementRequest Build(const FFoundrylinePlacedObject& O,int64 Revision){FFoundrylinePlacementRequest R;R.ExpectedRevision=Revision;R.DefinitionId=O.DefinitionId;R.Anchor=O.Anchor;R.QuarterTurns=O.QuarterTurns;R.Pavement=O.Pavement;return R;}
}
bool AFoundrylineBuildSite::CommitPavementBatch(const TArray<FFoundrylinePlacementRequest>& Requests){
 TArray<FFoundrylinePlacementRequest> Inverse;for(int I=Requests.Num()-1;I>=0;--I){const auto& R=Requests[I];if(R.Action==EFoundrylinePlacementAction::Build){FFoundrylinePlacementRequest Undo;Undo.Action=EFoundrylinePlacementAction::Remove;Inverse.Add(Undo);}else if(const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==R.ObjectId;})){auto Undo=Replace(*O,0);if(R.Action==EFoundrylinePlacementAction::Remove)Undo.Action=EFoundrylinePlacementAction::RestoreRemoved;Inverse.Add(Undo);}}
 const auto Result=ExecuteConstructionBatch(Requests);if(SentToHost(Result.Error))return true;if(!Result.IsSuccess()){Status=PlacementErrorText(Result.Error);return false;}
 for(int I=0;I<Requests.Num();++I)if(Requests[I].Action==EFoundrylinePlacementAction::Build){const int Index=Requests.Num()-1-I;if(Inverse.IsValidIndex(Index))Inverse[Index].ObjectId=Result.Actions[I].ObjectId;}
 FFoundrylinePlacementRequest Marker;Marker.DefinitionId=TEXT("__CompoundHistory");Marker.ObjectId=FGuid::NewGuid();CompoundHistory.Add(Marker.ObjectId,MoveTemp(Inverse));RecordUndo(Marker);SelectedPavementMark.Invalidate();HidePreview();return true;
}
bool AFoundrylineBuildSite::PavementShape(FName Mode,const TArray<FVector2D>& Points,float Width,uint8 Lanes,bool Truck,bool Curbs,bool Preview){
 if(!bReady||Points.Num()<2||Points.Num()>64)return false;const FString InputKey=FString::SanitizeFloat(DraftEntranceRadiusCm)+FString::Printf(TEXT("%s_%u_%.3f_%d_%d_%d_%lld"),*Mode.ToString(),FCrc::MemCrc32(Points.GetData(),Points.Num()*sizeof(FVector2D)),Width,Lanes,Truck,Curbs,Placement->GetRevision());if(Preview&&IsValid(PreviewActor)&&InputKey==PavementInputKey)return PavementInputValid;const auto C=LocalContext();const auto Revision=Placement->GetRevision();
 FFoundrylinePlacedObject Draft;Draft.CompanyId=C.CompanyId;Draft.PlantId=C.PlantId;Draft.Pavement.Curbs=Curbs;
 TArray<FVector> JoinGuides;auto& Data=Draft.Pavement;const bool Road=Mode==TEXT("Road")||Mode==TEXT("SplineRoad");
 if(Road){Data.Spline=Mode==TEXT("SplineRoad");Data.Lanes=Lanes;Data.TruckAccess=Truck;Data.RoadWidthCm=Width;Data.RoadControls=Points;SnapPaving(Data.RoadControls,Placement,C.CompanyId,{},Data.Spline);Data.EntranceRadiusCm=DraftEntranceRadiusCm;EntranceConnections(Draft,Placement);Data.Polygons=G::RoadSurface(Data);G::RoadPaint(Data,true);}
 if(Road)for(int End=0;End<2;++End){const int Index=End?Data.RoadControls.Num()-1:0;const auto At=Data.RoadControls[Index];if((Data.EntranceEnds&(1<<End))||FVector2D::Distance(At,Points[Index])>.1){const auto F=(Data.RoadControls[End?Index-1:1]-At).GetSafeNormal(),N=FVector2D(-F.Y,F.X);JoinGuides.Add(FVector(At-N*80,26));JoinGuides.Add(FVector(At+N*80,26));}}
 if(!Road&&Mode==TEXT("Polygon")){Poly P;P.Outer=Points;Data.Polygons={P};}
 else if(!Road)Data.Polygons=G::Rectangle(Points[0],Points.Last());
 if(Data.Polygons.IsEmpty()){Status=TEXT("Entrance radius will not fit. Straighten the approach or reduce the radius.");if(Preview)HidePreview();DraftBlocked(Status,!Preview);return false;}
 if(!Normalize(Draft,Placement))return false;TArray<FFoundrylinePlacementRequest> Requests;
 if(Mode==TEXT("Erase")){
  const auto Cut=G::Transform(Data.Polygons,Pose(Draft));
  for(const auto& O:Placement->GetObjects()){if(O.CompanyId!=C.CompanyId||O.PlantId!=C.PlantId||O.Pavement.Polygons.IsEmpty())continue;const auto LocalCut=G::Transform(Cut,Pose(O).Inverse());if(!G::Intersects(O.Pavement.Polygons,LocalCut))continue;
   auto Edited=O;TArray<Poly> Result;if(!G::Boolean(O.Pavement.Polygons,LocalCut,true,Result))return false;
   auto R=Replace(Edited,Revision);if(Result.IsEmpty()){R.Action=EFoundrylinePlacementAction::Remove;}else{Edited.Pavement.Polygons=Result;AsArea(Edited.Pavement);R.Replacement=Edited;}Requests.Add(R);
  }
  if(Requests.IsEmpty()){Status=TEXT("Drag across your pavement to cut it away");return false;}
 }else{
  // Merge area patches only. Road records keep their centerlines for width and direction edits.
  FGuid Primary;TArray<FFoundrylinePlacedObject> Merged;
  if(!Road){auto World=G::Transform(Data.Polygons,Pose(Draft));bool Expanded=true;TSet<FGuid> Taken;
   while(Expanded){Expanded=false;for(const auto& O:Placement->GetObjects()){
    if(Taken.Contains(O.ObjectId)||O.CompanyId!=C.CompanyId||O.PlantId!=C.PlantId||O.Pavement.Polygons.IsEmpty()||O.Pavement.Lanes)continue;
    const auto Shape=G::Transform(O.Pavement.Polygons,Pose(O));const auto A=G::Bounds(World),B=G::Bounds(Shape);
    if(!A.ExpandBy(.2).Intersect(B))continue;TArray<Poly> Joined;if(!G::Boolean(World,Shape,false,Joined))return false;
    // Disconnected islands do not become one selectable record.
    if(Joined.Num()>=World.Num()+Shape.Num())continue;
    World=MoveTemp(Joined);Taken.Add(O.ObjectId);Merged.Add(O);Expanded=true;
   }}
   if(!Merged.IsEmpty()){
    const auto Base=Merged[0];Primary=Base.ObjectId;Draft=Base;Draft.QuarterTurns=0;Draft.Anchor=FIntVector::ZeroValue;Draft.Pavement.Polygons=World;Draft.Pavement.Curbs=Curbs;Draft.Pavement.Markings.Reset();Draft.RouteNodes.Reset();Draft.TruckZones.Reset();
    for(const auto& O:Merged){const auto T=Pose(O);for(auto M:O.Pavement.Markings){for(auto& P:M.Points)P=FVector2D(T.TransformPosition(FVector(P,0)));Draft.Pavement.Markings.Add(M);}for(auto N:O.RouteNodes){const auto W=T.TransformPosition(FVector(N.Position.X,N.Position.Y,0));N.Position={int32(FMath::RoundToInt(W.X)),int32(FMath::RoundToInt(W.Y))};Draft.RouteNodes.Add(N);}for(const auto& Z:O.TruckZones){const auto A=T.TransformPosition(FVector(Z.Min.X,Z.Min.Y,0)),B=T.TransformPosition(FVector(Z.Max.X,Z.Max.Y,0));FFoundrylineTruckZone R;R.Min={int32(FMath::RoundToInt(FMath::Min(A.X,B.X))),int32(FMath::RoundToInt(FMath::Min(A.Y,B.Y)))};R.Max={int32(FMath::RoundToInt(FMath::Max(A.X,B.X))),int32(FMath::RoundToInt(FMath::Max(A.Y,B.Y)))};Draft.TruckZones.Add(R);}}
    if(!Normalize(Draft,Placement))return false;
    for(int I=1;I<Merged.Num();++I){auto R=Replace(Merged[I],Revision);R.Action=EFoundrylinePlacementAction::Remove;Requests.Add(R);}
   }
  }
  Requests.Add(Primary.IsValid()?Replace(Draft,Revision):Build(Draft,Revision));
 }
 // Reconnect saved road mouths when a yard is added or trimmed later, in the same transaction.
 TArray<FFoundrylinePlacedObject> Future=Placement->GetObjects();TSet<FGuid> EditedIds;
 for(const auto& R:Requests){EditedIds.Add(R.ObjectId);if(R.Action==EFoundrylinePlacementAction::Build)Future.Add(Draft);else if(R.Action==EFoundrylinePlacementAction::Remove)Future.RemoveAll([&](const auto& O){return O.ObjectId==R.ObjectId;});else if(auto* Existing=Future.FindByPredicate([&](const auto& O){return O.ObjectId==R.ObjectId;}))*Existing=R.Replacement;}
 for(const auto& O:Placement->GetObjects())if(!EditedIds.Contains(O.ObjectId)&&O.CompanyId==C.CompanyId&&O.PlantId==C.PlantId&&O.Pavement.Lanes&&O.Pavement.EntranceRadiusCm>0){auto Updated=O;EntranceConnections(Updated,Placement,&Future);if(Updated.Pavement.EntranceEnds!=O.Pavement.EntranceEnds){Updated.Pavement.Polygons=G::RoadSurface(Updated.Pavement);if(!Normalize(Updated,Placement))return false;Requests.Add(Replace(Updated,Revision));}}
 const auto Validation=Placement->PreviewBatch(Requests,C,true);PavementPreviewAreaFt2=G::Area(Draft.Pavement.Polygons)/929.0304;
 if(Preview){const bool Valid=Validation.IsSuccess();PreviewError=Validation.Error;DraftReason=Valid?TEXT("Release to place / paint and curbs can be adjusted later"):PlacementErrorText(Validation.Error);bHasPreviewFeedback=true;
  const uint32 Hash=FCrc::MemCrc32(Points.GetData(),Points.Num()*sizeof(FVector2D));ShowPreview(Draft,Valid,FString::Printf(TEXT("Pave_%s_%u_%.1f_%d_%d_%lld_%.1f"),*Mode.ToString(),Hash,Width,Lanes,Curbs,Revision,DraftEntranceRadiusCm));PreviewJoinLines=MoveTemp(JoinGuides);PavementInputKey=InputKey;PavementInputValid=Valid;return Valid;}
 if(!Validation.IsSuccess()){Status=PlacementErrorText(Validation.Error);DraftBlocked(Status,true);return false;}
 if(!CommitPavementBatch(Requests))return false;Status=Mode==TEXT("Erase")?TEXT("Pavement trimmed. Check markings and routes."):TEXT("Pavement placed. Add markings when ready.");return true;
}
bool AFoundrylineBuildSite::AddPavementMark(EFoundrylinePavementMark Kind,const TArray<FVector2D>& World,float Width,bool Preview){
 if(World.Num()<2||int(Kind)>7||!FMath::IsFinite(Width)||Width<10||Width>3000||FVector2D::Distance(World[0],World.Last())<10){if(Preview)HidePreview();Status=TEXT("Drag a larger marking on pavement");return false;}const FIntVector Cursor(FVector(World[0],0));const auto* O=Pick(Cursor);if(!O||O->Pavement.Polygons.IsEmpty()){if(Preview)HidePreview();Status=TEXT("Start the marking on your pavement");return false;}
 auto Edited=*O;FFoundrylinePavementMarking M;M.Id=FGuid::NewGuid();M.Kind=Kind;M.WidthCm=Width;const auto T=Pose(*O).Inverse();for(const auto P:World)M.Points.Add(FVector2D(T.TransformPosition(FVector(P,0))));
 const bool Fits=G::MarkingFits(Edited.Pavement,M);Edited.Pavement.Markings.Add(M);auto R=Replace(Edited,Placement->GetRevision());const auto V=ValidateConstruction(R);
 if(Preview){auto PaintDraft=Edited;PaintDraft.Pavement.Markings={M};PreviewError=Fits?V.Error:EFoundrylinePlacementError::InvalidTransform;bHasPreviewFeedback=true;DraftReason=Fits?TEXT("Release to paint"):TEXT("The whole marking must fit on pavement");ShowPreview(PaintDraft,Fits&&V.IsSuccess(),FString::Printf(TEXT("PavePaint_%d_%s_%s_%.1f_%lld"),int(Kind),*World[0].ToString(),*World.Last().ToString(),Width,Placement->GetRevision()));return Fits&&V.IsSuccess();}
 if(!Fits){Status=TEXT("The complete marking must fit on pavement");DraftBlocked(Status,true);return false;}
 if(!CommitPavementBatch({R}))return false;SelectedObject=Edited.ObjectId;SelectedPavementMark=M.Id;Status=TEXT("Paint applied. Select markings to move or remove them.");return true;
}
bool AFoundrylineBuildSite::SelectPavementMark(FIntVector Cursor){
 SelectedPavementMark.Invalidate();const auto* O=Pick(Cursor);if(!O||O->Pavement.Polygons.IsEmpty())return false;const auto P=FVector2D(Pose(*O).InverseTransformPosition(FVector(Cursor)));double Best=75;
 for(const auto& M:O->Pavement.Markings)for(const auto& L:G::PaintLines(M)){const double D=FMath::PointDistToSegment(FVector(P,0),FVector(L.Key,0),FVector(L.Value,0));if(D<Best){Best=D;SelectedPavementMark=M.Id;}}
 SelectedObject=O->ObjectId;Status=SelectedPavementMark.IsValid()?TEXT("Marking selected. Move or Delete edits only this paint."):TEXT("Click near a painted line to select it");return SelectedPavementMark.IsValid();
}
bool AFoundrylineBuildSite::PavementOption(FName Action){
 const auto* O=GetSelectedObject();if(!O)return false;auto Edited=*O;auto& D=Edited.Pavement;
 if(Action==TEXT("Convert")){
  const auto* Def=Placement->FindDefinition(O->DefinitionId);if(!Def||!FFoundrylineDrivingLayout::IsPaved(Def->FunctionId)||!O->Pavement.Polygons.IsEmpty())return false;const double W=Def->Footprint.Max.X,H=Def->Footprint.Max.Y;D.Polygons=G::Rectangle({0,0},{W,H});D.Curbs=Def->FunctionId==TEXT("Parking")||Def->FunctionId==TEXT("Road2")||Def->FunctionId==TEXT("Road4");Edited.Crosswalks.Reset();
  if(Def->FunctionId==TEXT("Parking")){
   for(const auto& Island:FFoundrylineDrivingLayout::Islands(W,H,O->bLandscaping)){TArray<Poly> Cut;if(!G::Boolean(D.Polygons,G::Rectangle(Island.Min,Island.Max),true,Cut))return false;D.Polygons=MoveTemp(Cut);}
   const auto Bays=FFoundrylineDrivingLayout::Parking(W,H,UFoundrylineVehiclePathSubsystem::Entrances(*Placement,*O));int Index=0;
   while(Index<Bays.Num()){const auto First=Bays[Index];double End=First.Bounds.Max.X;int Next=Index+1;while(Next<Bays.Num()&&FMath::Abs(Bays[Next].Bounds.Min.Y-First.Bounds.Min.Y)<1&&FMath::Abs(Bays[Next].Bounds.Min.X-End)<1){End=Bays[Next].Bounds.Max.X;++Next;}
    if(End-First.Bounds.Min.X>=274.32){FFoundrylinePavementMarking M;M.Id=FGuid::NewGuid();M.Kind=EFoundrylinePavementMark::Parking;M.WidthCm=550;M.Points=First.Heading<0?TArray<FVector2D>{{First.Bounds.Min.X,First.Bounds.Min.Y},{End,First.Bounds.Min.Y}}:TArray<FVector2D>{{End,First.Bounds.Max.Y},{First.Bounds.Min.X,First.Bounds.Max.Y}};D.Markings.Add(M);}Index=Next;
   }
  }else if(Def->FunctionId==TEXT("Road2")||Def->FunctionId==TEXT("Road4")||Def->FunctionId==TEXT("TruckAccess")||Def->FunctionId==TEXT("Driveway")||Def->FunctionId==TEXT("Service")){
   D.TruckAccess=Def->FunctionId==TEXT("TruckAccess");D.Lanes=Def->FunctionId==TEXT("Road4")?4:W-(D.Curbs?36:0)>=G::MinimumWidth(2,D.TruckAccess)?2:1;D.RoadWidthCm=W-(D.Curbs?36:0);D.RoadControls={{W/2,0},{W/2,H}};if(D.RoadWidthCm<G::MinimumWidth(D.Lanes,D.TruckAccess)){D.Lanes=0;D.RoadWidthCm=0;D.RoadControls.Reset();}else{D.Polygons=G::RoadSurface(D);G::RoadPaint(D,true);}
  }
  if(!Normalize(Edited,Placement))return false;return CommitPavementBatch({Replace(Edited,Placement->GetRevision())});
 }
 if(O->Pavement.Polygons.IsEmpty())return false;
 if(Action==TEXT("RotatePaint")||Action==TEXT("FlipPaint")){
  auto* M=D.Markings.FindByPredicate([&](const auto& V){return V.Id==SelectedPavementMark;});if(!M)return false;
  if(Action==TEXT("FlipPaint"))Algo::Reverse(M->Points);
  else {FBox2D Bounds(ForceInit);for(const auto& L:G::PaintLines(*M)){Bounds+=L.Key;Bounds+=L.Value;}const auto Center=Bounds.GetCenter();for(auto& Point:M->Points){const auto V=Point-Center;Point=Center+FVector2D(-V.Y,V.X);}}
  if(!G::MarkingFits(D,*M)){Status=TEXT("The rotated marking needs more pavement space");DraftBlocked(Status,true);return false;}
  M->RoadGenerated=false;return CommitPavementBatch({Replace(Edited,Placement->GetRevision())});
 }
 if(Action==TEXT("Curbs")){D.Curbs=!D.Curbs;if(D.Lanes)D.Polygons=G::RoadSurface(D);}
 else if(Action==TEXT("RadiusMore")||Action==TEXT("RadiusLess")){if(!D.Lanes)return false;D.EntranceRadiusCm=FMath::Clamp(D.EntranceRadiusCm+(Action==TEXT("RadiusMore")?152.4f:-152.4f),0.f,1524.f);EntranceConnections(Edited,Placement);D.Polygons=G::RoadSurface(D);if(D.Polygons.IsEmpty()){Status=TEXT("Radius needs more space. Straighten the approach or reduce the radius.");DraftBlocked(Status,true);return false;}if(!Normalize(Edited,Placement))return false;}
 else if(Action==TEXT("StripeRoad")){if(!D.Lanes)return false;G::RoadPaint(D,true);}
 else if(Action==TEXT("ClearMark")){if(!SelectedPavementMark.IsValid())return false;D.Markings.RemoveAll([&](const auto& M){return M.Id==SelectedPavementMark;});}
 else if(Action==TEXT("WidthMore")||Action==TEXT("WidthLess")){if(!D.Lanes)return false;D.RoadWidthCm=FMath::Clamp(D.RoadWidthCm+(Action==TEXT("WidthMore")?30.48f:-30.48f),float(G::MinimumWidth(D.Lanes,D.TruckAccess)),2400.f);D.Polygons=G::RoadSurface(D);if(!Normalize(Edited,Placement,true))return false;}
 else return false;
 G::RoadPaint(D);if(D.Lanes&&!Normalize(Edited,Placement,true))return false;return CommitPavementBatch({Replace(Edited,Placement->GetRevision())});
}
bool AFoundrylineBuildSite::PavementEditRequest(FName Tool,FIntVector Start,FIntVector End,FFoundrylinePlacementRequest& R){
 const auto* O=GetSelectedObject();if(!O||O->Pavement.Polygons.IsEmpty())return false;R=Replace(*O,Placement->GetRevision());auto& D=R.Replacement.Pavement;const auto T=Pose(*O).Inverse();const FVector2D A(T.TransformPosition(FVector(Start))),B(T.TransformPosition(FVector(End)));
 if(SelectedPavementMark.IsValid()&&(Tool==TEXT("Move")||Tool==TEXT("Demolish"))){if(Tool==TEXT("Demolish"))D.Markings.RemoveAll([&](const auto& M){return M.Id==SelectedPavementMark;});else for(auto& M:D.Markings)if(M.Id==SelectedPavementMark)for(auto& P:M.Points)P+=B-A;return true;}
 if(D.Lanes&&(Tool==TEXT("Move")||Tool==TEXT("Rotate"))){
  if(Tool==TEXT("Move"))FFoundrylineGrid::SnapWorld(FVector(O->Anchor+End-Start),EFoundrylineSnap::Fine25cm,R.Replacement.Anchor);
  else {const auto* Def=Placement->FindDefinition(O->DefinitionId);const auto Old=FFoundrylineGrid::RotateBounds(Def->Footprint,O->Anchor,O->QuarterTurns);R.Replacement.QuarterTurns=(O->QuarterTurns+1)%4;const auto New=FFoundrylineGrid::RotateBounds(Def->Footprint,FIntVector::ZeroValue,R.Replacement.QuarterTurns);R.Replacement.Anchor=Old.Min-New.Min;}
  EntranceConnections(R.Replacement,Placement);D.Polygons=G::RoadSurface(D);return Normalize(R.Replacement,Placement);
 }
 if(Tool==TEXT("PaveRound")){AsArea(D);if(!G::RoundCorner(D.Polygons,A,FVector2D::Distance(A,B)))return false;return Normalize(R.Replacement,Placement,true);}
 if(D.Spline&&(Tool==TEXT("PaveInsert")||Tool==TEXT("PaveRemovePoint"))){
  if(Tool==TEXT("PaveRemovePoint")){if(D.RoadControls.Num()<=2){Status=TEXT("A road needs at least two points");return false;}int Index=0;double Best=MAX_dbl;for(int I=0;I<D.RoadControls.Num();++I){const double Distance=FVector2D::DistSquared(A,D.RoadControls[I]);if(Distance<Best){Best=Distance;Index=I;}}if(Best>FMath::Square(200.)){Status=TEXT("Click near the road point you want to remove");return false;}D.RoadControls.RemoveAt(Index);}
  else {if(D.RoadControls.Num()>=32){Status=TEXT("This road already has 32 points");return false;}const auto Path=G::RoadPath(D.RoadControls,true);double Best=MAX_dbl;FVector2D Point=A;for(int I=1;I<Path.Num();++I){const auto Q=FVector2D(FMath::ClosestPointOnSegment(FVector(A,0),FVector(Path[I-1],0),FVector(Path[I],0)));const double Distance=FVector2D::DistSquared(A,Q);if(Distance<Best){Best=Distance;Point=Q;}}if(Best>FMath::Square(D.RoadWidthCm*.5+100)){Status=TEXT("Click on the selected road to add a point");return false;}int Segment=0;Best=MAX_dbl;for(int I=1;I<D.RoadControls.Num();++I){const auto Q=FVector2D(FMath::ClosestPointOnSegment(FVector(Point,0),FVector(D.RoadControls[I-1],0),FVector(D.RoadControls[I],0)));const double Distance=FVector2D::DistSquared(Point,Q);if(Distance<Best){Best=Distance;Segment=I-1;}}D.RoadControls.Insert(Point,Segment+1);}
  EntranceConnections(R.Replacement,Placement);D.Polygons=G::RoadSurface(D);G::RoadPaint(D);return Normalize(R.Replacement,Placement);
 }
 if(Tool!=TEXT("Resize")&&Tool!=TEXT("PaveBend"))return false;
 if(D.Lanes){if(!D.Spline&&D.RoadControls.Num()==2&&Tool==TEXT("PaveBend"))D.RoadControls.Insert((D.RoadControls[0]+D.RoadControls[1])/2,1);int Index=0;double Best=MAX_dbl;for(int I=0;I<D.RoadControls.Num();++I){const double Distance=FVector2D::Distance(A,D.RoadControls[I]);if(Distance<Best){Best=Distance;Index=I;}}if(!D.Spline&&Tool==TEXT("PaveBend"))Index=1;D.RoadControls[Index]=B;TArray<FVector2D> WorldControls;const auto World=Pose(*O);for(const auto P:D.RoadControls)WorldControls.Add(FVector2D(World.TransformPosition(FVector(P,0))));SnapPaving(WorldControls,Placement,O->CompanyId,O->ObjectId,D.Spline);for(int I=0;I<D.RoadControls.Num();++I)D.RoadControls[I]=FVector2D(T.TransformPosition(FVector(WorldControls[I],0)));EntranceConnections(R.Replacement,Placement);D.Polygons=G::RoadSurface(D);}
 else {int PolyIndex=-1,Vertex=-1;double Best=MAX_dbl;for(int P=0;P<D.Polygons.Num();++P)for(int I=0;I<D.Polygons[P].Outer.Num();++I){const double Distance=FVector2D::Distance(A,D.Polygons[P].Outer[I]);if(Distance<Best){Best=Distance;PolyIndex=P;Vertex=I;}}if(PolyIndex<0)return false;
  auto& Loop=D.Polygons[PolyIndex].Outer;int Edge=-1;double EdgeDist=80;for(int I=0;I<Loop.Num();++I){const double Dist=FMath::PointDistToSegment(FVector(A,0),FVector(Loop[I],0),FVector(Loop[(I+1)%Loop.Num()],0));if(Dist<EdgeDist&&FVector2D::Distance(A,Loop[I])>100&&FVector2D::Distance(A,Loop[(I+1)%Loop.Num()])>100){EdgeDist=Dist;Edge=I;}}
  if(Edge>=0){Loop[Edge]+=B-A;Loop[(Edge+1)%Loop.Num()]+=B-A;}else Loop[Vertex]=B;
 }
 G::RoadPaint(D);return Normalize(R.Replacement,Placement,true);
}
