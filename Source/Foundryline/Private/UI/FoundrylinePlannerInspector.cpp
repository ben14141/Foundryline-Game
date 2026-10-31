#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AFoundrylineBuildHUD::DrawPlannerInspector(AFoundrylinePlayerController* PC){
 if(!PC->bPropertyViewAccess||PC->IsLogisticsOverlayVisible())return;if(DrawSignInspector(PC))return;if(PC->IsPavementTool())return;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)if(const auto* O=It->GetSelectedObject();O&&(!O->Pavement.Polygons.IsEmpty()||(It->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->FindDefinition(O->DefinitionId)&&FFoundrylineDrivingLayout::IsPaved(It->GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>()->FindDefinition(O->DefinitionId)->FunctionId))))return;const auto* Plant=PC->FactoryPlant();if(!Plant)return;
 const auto Items=FFoundrylinePlannerItems::List(Plant->Snapshot);const auto* Selected=Items.FindByPredicate([&](const auto& V){return int32(V.Kind)==PC->PlannerKind&&V.Id==PC->PlannerId;});
 TArray<const FFoundrylinePlannerItem*> Stored;for(const auto& V:Items)if(V.Stored)Stored.Add(&V);if(!Selected&&Stored.IsEmpty())return;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),X=Canvas->ClipX/S-400,Y=280,W=376;
 const FLinearColor Paper(FColor::FromHex(TEXT("F7F5F1"))),Ink(FColor::FromHex(TEXT("1F2327"))),Grey(FColor::FromHex(TEXT("5B6268"))),Orange(FColor::FromHex(TEXT("F07A2F"))),Muted(FColor::FromHex(TEXT("E8E7E2")));
 auto R=[&](float A,float B,float C,float D,FLinearColor Color){DrawRect(Color,A*S,B*S,C*S,D*S);};
 auto T=[&](FString Text,float A,float B,float Scale=1.15f,FLinearColor Color=FLinearColor::White){DrawText(Text,Color,A*S,B*S,GEngine->GetMediumFont(),Scale*S);};
 auto Button=[&](FString Key,FString Label,float B,bool Primary){const FName Id(*(TEXT("Planner")+Key));float& Hover=HoverAmounts.FindOrAdd(Id);Hover=FMath::FInterpTo(Hover,HoveredBox==Id?1.f:0.f,GetWorld()->GetDeltaSeconds(),14);R(X+18,B,W-36,42,Primary?Orange:FMath::Lerp(Muted,FLinearColor(1,.86f,.72f),Hover));T(Label,X+30,B+13,1.1f,Ink);if(!PC->bPlannerPending)AddHitBox({(X+18)*S,B*S},{(W-36)*S,42*S},Id,true);};
 const float H=(Selected?346.f:52.f)+Stored.Num()*52.f;R(X+4,Y+4,W,H,FLinearColor(0,0,0,.2f));R(X,Y,W,H,Paper);R(X,Y,4,H,Orange);
 float Row=Y+18;
 if(Selected){T(Selected->Name,X+18,Row,1.2f,Ink);Row+=31;T(PC->bPlannerPending?TEXT("WAITING FOR HOST"):Selected->Stored?TEXT("IN EQUIPMENT STORAGE"):Plant->Snapshot.Running?TEXT("PAUSE PRODUCTION TO EDIT"):TEXT("COMPANY FLOOR LAYOUT"),X+18,Row,.9f,Grey);Row+=32;
  Button(TEXT("Move"),Selected->Stored?TEXT("PLACE ON FLOOR"):Selected->Kind==EFoundrylinePlannerItem::Pallet?TEXT("MOVE TO A RACK / FORKLIFT"):TEXT("MOVE ITEM"),Row,true);Row+=51;
  if(!Selected->Stored){Button(TEXT("Inspect"),TEXT("OPEN CONTROLS / INSPECT"),Row,false);Row+=51;Button(TEXT("Delete"),Selected->Kind==EFoundrylinePlannerItem::Machine?TEXT("REMOVE TO EQUIPMENT STORAGE"):Selected->Kind==EFoundrylinePlannerItem::Pallet?TEXT("SCRAP PALLET"):TEXT("REMOVE ITEM"),Row,false);Row+=51;}
  else {T(TEXT("The machine and its sheet are kept."),X+18,Row+8,.95f,Grey);Row+=43;}
  Button(TEXT("Cancel"),TEXT("CLEAR SELECTION"),Row,false);Row+=54;
  const FString Message=PC->PlannerStatus;int Offset=0;for(int Line=0;Line<3&&Offset<Message.Len();++Line){int Count=FMath::Min(42,Message.Len()-Offset);if(Offset+Count<Message.Len()){const int Space=Message.Mid(Offset,Count).Find(TEXT(" "),ESearchCase::CaseSensitive,ESearchDir::FromEnd);if(Space>10)Count=Space;}T(Message.Mid(Offset,Count),X+18,Row+Line*19,.88f,Grey);Offset+=Count;while(Offset<Message.Len()&&Message[Offset]==TCHAR(' '))++Offset;}
  Row=Y+346;
 }
 if(!Stored.IsEmpty()){if(!Selected){T(TEXT("STORED EQUIPMENT"),X+18,Row,1.1f,Ink);Row=Y+52;}for(const auto* V:Stored){Button(FString::Printf(TEXT("Stored%d"),V->Id),TEXT("PLACE / ")+V->Name,Row,false);Row+=52;}}
}
