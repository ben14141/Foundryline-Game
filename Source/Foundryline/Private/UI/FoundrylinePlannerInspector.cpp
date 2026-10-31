#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "FoundrylineUISurface.h"

void AFoundrylineBuildHUD::DrawPlannerInspector(AFoundrylinePlayerController* PC){
 if(!PC->bPropertyViewAccess||PC->IsLogisticsOverlayVisible())return;if(DrawSignInspector(PC))return;if(PC->IsPavementTool())return;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)if(const auto* O=It->GetSelectedObject();O&&(!O->Pavement.Polygons.IsEmpty()||(It->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->FindDefinition(O->DefinitionId)&&FFoundrylineDrivingLayout::IsPaved(It->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->FindDefinition(O->DefinitionId)->FunctionId))))return;const auto* Plant=PC->FactoryPlant();if(!Plant)return;
 const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Selected=Items.FindByPredicate([&](const auto& V){return int32(V.Kind)==PC->PlannerKind&&V.Id==PC->PlannerId;});
 TArray<const FFoundrylinePlannerItem*> Stored;for(const auto& V:Items)if(V.Stored)Stored.Add(&V);if(!Selected&&Stored.IsEmpty())return;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),X=Canvas->ClipX/S-400,Y=280,W=376;
 const FLinearColor Paper(FColor::FromHex(TEXT("F7F5F1"))),White(FColor::FromHex(TEXT("FFFFFF"))),Ink(FColor::FromHex(TEXT("1F2327"))),Grey(FColor::FromHex(TEXT("5B6268"))),Orange(FColor::FromHex(TEXT("F07A2F"))),Muted(FColor::FromHex(TEXT("E8E7E2"))),Line(FColor::FromHex(TEXT("DCE1E5"))),SoftOrange(FColor::FromHex(TEXT("FFF1E6")));
 auto R=[&](float A,float B,float C,float D,FLinearColor Color){DrawRect(Color,A*S,B*S,C*S,D*S);};
 auto T=[&](FString Text,float A,float B,float Scale=1.15f,FLinearColor Color=FLinearColor::White){DrawText(Text,Color,A*S,B*S,GEngine->GetMediumFont(),Scale*S);};
 auto Panel=[&](float A,float B,float C,float D,FLinearColor Top,FLinearColor Bottom,FLinearColor Edge,float Elevation=1.f){FoundrylineUISurface::Panel(Canvas,A*S,B*S,C*S,D*S,S,Top,Bottom,Edge,Elevation,8);};
 auto Button=[&](FString Key,FString Label,float B,bool Primary,bool Allow=true){
  const FName Id(*(TEXT("Planner")+Key));const bool Enabled=Allow&&!PC->bPlannerPending;const float Dt=GetWorld()->GetDeltaSeconds();
  float& Hover=HoverAmounts.FindOrAdd(Id);Hover=FMath::FInterpTo(Hover,Enabled&&HoveredBox==Id?1.f:0.f,Dt,18);
  float& Press=PressAmounts.FindOrAdd(Id);Press=FMath::FInterpTo(Press,0.f,Dt,9);
  const float Lift=Hover*1.5f-Press;const FLinearColor Base=Enabled?(Primary?Orange:White):Muted;
  const auto Top=FMath::Lerp(Base,Primary?FLinearColor(1.f,.58f,.27f):SoftOrange,Hover*.55f);
  Panel(X+18,B-Lift,W-36,44,Top,Base,Primary?Orange:Line,Enabled?1.f+Hover:0.f);
  if(Primary)R(X+30,B+14-Lift,3,16,Ink);
  T(Label,X+(Primary?43:30),B+14-Lift,1.02f,Enabled?Ink:Grey);
  if(Enabled)AddHitBox({(X+18)*S,B*S},{(W-36)*S,44*S},Id,true);
 };
 const bool Lane=Selected&&Selected->Kind==EFoundrylinePlannerItem::Lane&&!Selected->Stored;
 const float SelectedH=Selected?(Lane?452.f:400.f):0.f;
 const float StoredHead=Stored.IsEmpty()?0.f:52.f;
 const int MaxStored=FMath::Max(0,FMath::FloorToInt((Canvas->ClipY/S-Y-28-SelectedH-StoredHead)/50));
 const int Shown=FMath::Min(MaxStored,Stored.Num());
 const float H=SelectedH+StoredHead+Shown*50+(Shown<Stored.Num()?28:0);
 Panel(X,Y,W,H,Paper,Paper,Line,4);
 float Row=Y+18;
 if(Selected){
  FoundrylineUISurface::Round(Canvas,(X+1)*S,(Y+1)*S,(W-2)*S,78*S,7*S,Ink,Ink);
  R(X+18,Y+18,4,40,Orange);
  const FString Name=Selected->Name.Len()>35?Selected->Name.Left(32)+TEXT("..."):Selected->Name;
  T(Name,X+32,Y+19,1.12f,White);
  T(PC->bPlannerPending?TEXT("WAITING FOR HOST"):Selected->Stored?TEXT("IN EQUIPMENT STORAGE"):Plant->Snapshot.Running?TEXT("PAUSE PRODUCTION TO EDIT"):Lane?TEXT("FORKLIFT FLOOR ROUTE"):TEXT("COMPANY FLOOR LAYOUT"),X+32,Y+49,.82f,FLinearColor(.78f,.8f,.82f));
  Row=Y+96;
  Button(TEXT("Move"),Selected->Stored?TEXT("PLACE ON FLOOR"):Selected->Kind==EFoundrylinePlannerItem::Pallet?TEXT("MOVE TO A RACK / FORKLIFT"):Selected->Kind==EFoundrylinePlannerItem::FixedRack&&Selected->MachineId?TEXT("MOVE MACHINE WORKSPACE"):TEXT("MOVE ITEM"),Row,true);Row+=52;
  if(Lane){Button(TEXT("Resize"),TEXT("RESIZE LANE / DRAG A CORNER"),Row,false);Row+=52;}
  if(!Selected->Stored){
   Button(TEXT("Inspect"),TEXT("OPEN CONTROLS / INSPECT"),Row,false);Row+=52;
   Button(TEXT("Delete"),Selected->Kind==EFoundrylinePlannerItem::FixedRack&&Selected->MachineId?TEXT("RACK ATTACHED TO MACHINE"):Selected->Kind==EFoundrylinePlannerItem::Machine?TEXT("MOVE TO EQUIPMENT STORAGE"):Selected->Kind==EFoundrylinePlannerItem::Pallet?TEXT("SCRAP PALLET"):TEXT("REMOVE ITEM"),Row,false,!(Selected->Kind==EFoundrylinePlannerItem::FixedRack&&Selected->MachineId));Row+=52;
  }else{T(TEXT("Your machine and its sheet are kept."),X+18,Row+14,.94f,Grey);Row+=52;}
  Button(TEXT("Cancel"),TEXT("CLEAR SELECTION"),Row,false);Row+=58;
  T(Selected->Kind==EFoundrylinePlannerItem::Pallet?TEXT("A driver transports the selected pallet."):TEXT("R: rotate while moving   /   Right-click: cancel"),X+18,Row,.83f,Grey);Row+=26;
  const FString Message=PC->PlannerStatus;int Offset=0;
  for(int N=0;N<3&&Offset<Message.Len();++N){int Count=FMath::Min(42,Message.Len()-Offset);if(Offset+Count<Message.Len()){const int Space=Message.Mid(Offset,Count).Find(TEXT(" "),ESearchCase::CaseSensitive,ESearchDir::FromEnd);if(Space>10)Count=Space;}T(Message.Mid(Offset,Count),X+18,Row+N*19,.85f,Grey);Offset+=Count;while(Offset<Message.Len()&&Message[Offset]==TCHAR(' '))++Offset;}
  Row=Y+SelectedH;
 }
 if(!Stored.IsEmpty()){
  if(Selected)R(X+18,Row,W-36,1,Line);
  T(FString::Printf(TEXT("STORED EQUIPMENT  /  %d"),Stored.Num()),X+18,Row+17,.96f,Ink);Row+=52;
  for(int I=0;I<Shown;++I){const auto* V=Stored[I];Button(FString::Printf(TEXT("Stored%d"),V->Id),TEXT("PLACE / ")+V->Name,Row,false);Row+=50;}
  if(Shown<Stored.Num())T(TEXT("Open Production to place other stored machines."),X+18,Row+8,.8f,Grey);
 }
}
