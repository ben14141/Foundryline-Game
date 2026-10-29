#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineTrafficSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
void AFoundrylineBuildHUD::DrawNetworkPanel(AFoundrylinePlayerController* PC) {
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),Dt=GetWorld()->GetDeltaSeconds();auto* Font=GEngine->GetMediumFont();
 const FLinearColor Ink(FColor::FromHex(TEXT("1F2327"))),Paper(FColor::FromHex(TEXT("F7F5F1"))),Orange(FColor::FromHex(TEXT("F07A2F"))),Steel(FColor::FromHex(TEXT("9AA0A6"))),Muted(FColor::FromHex(TEXT("E8E7E2"))),Green(FColor::FromHex(TEXT("24835C"))),Red(FColor::FromHex(TEXT("B64032")));
 auto Box=[&](float X,float Y,float W,float H,FLinearColor C){DrawRect(C,X*S,Y*S,W*S,H*S);};
 auto Text=[&](FString V,float X,float Y,float Size,FLinearColor C){DrawText(V,C,X*S,Y*S,Font,Size*S);};
 auto Wrap=[&](FString V,float Y,int32 Width,int32 Max=3){TArray<FString> Words;V.ParseIntoArrayWS(Words);FString Row;int32 I=0;for(const auto& W:Words){if(Row.Len()+W.Len()>Width && !Row.IsEmpty()){Text(Row,40,Y+I*22,1.32f,Ink);Row.Empty();if(++I>=Max) return;}if(!Row.IsEmpty()) Row+=TEXT(" ");Row+=W;}if(!Row.IsEmpty()) Text(Row,40,Y+I*22,1.32f,Ink);};
 auto Button=[&](FName Key,FString Title,FString Detail,float X,float Y,float W,bool Active=false) {
  auto& H=HoverAmounts.FindOrAdd(Key);H=FMath::FInterpTo(H,HoveredBox==Key?1.f:0.f,Dt,14);auto& P=PressAmounts.FindOrAdd(Key);P=FMath::FInterpTo(P,0.f,Dt,20);
  const bool Primary=Key==TEXT("NetCheck") || Key==TEXT("TrafficTest");
  Box(X,Y+P*2,W,62,Primary?Ink:Active?Muted:FMath::Lerp(Paper,Muted,H));
  if(Active) Box(X,Y+9,3,44,Orange);
  Text(Title,X+14+H*2,Y+12,1.4f,Primary?Paper:Ink);
  if(Primary || Active || H>.1f) Text(Detail,X+14+H*2,Y+38,1.08f,Primary?Steel:Ink);
  AddHitBox({X*S,Y*S},{W*S,62*S},Key,true);
 };
 if(PC->LogisticsPanel==2) {
  Text(TEXT("TRAFFIC CONTROL"),24,256,2.f,Ink);Text(TEXT("SEMI CLEARANCE / SHARED RIGHT-OF-WAY"),25,289,1.15f,Green);
  Box(24,320,352,113,Ink);Text(TEXT("SHARED RIGHT-OF-WAY"),40,337,1.7f,Orange);Text(TEXT("Conflicting movements take turns"),40,373,1.2f,Paper);Text(TEXT("Clear exits keep the property moving"),40,398,1.15f,Steel);
  auto Rule=[&](FString N,FString Title,FString Detail,float Y){Box(24,Y,352,101,Muted);Box(24,Y,4,101,Orange);Text(N,40,Y+17,1.7f,Orange);Text(Title,83,Y+17,1.5f,Ink);TArray<FString> Words;Detail.ParseIntoArrayWS(Words);FString Row;int32 I=0;for(const auto& W:Words){if(Row.Len()+W.Len()>33){Text(Row,83,Y+47+I*18,1.15f,Ink);Row.Empty();++I;}if(!Row.IsEmpty())Row+=TEXT(" ");Row+=W;}Text(Row,83,Y+47+I*18,1.15f,Ink);};
  Rule(TEXT("01"),TEXT("KEEP THE EXIT CLEAR"),TEXT("Whole truck plus 3 m gap must fit before entering."),454);
  Rule(TEXT("02"),TEXT("YIELD AT MERGES"),TEXT("Earlier conflicting arrivals go first. Clear routes may continue."),567);
  Rule(TEXT("03"),TEXT("WAIT FOR THE TRAILER"),TEXT("Junction stays reserved until the rear clears, including backing."),680);
  Rule(TEXT("04"),TEXT("AVOID QUEUED LINKS"),TEXT("Route cost includes waiting. Never bypass your permitted paving."),793);
  Button(TEXT("TrafficTest"),TEXT("REHEARSE TRAFFIC RULES"),TEXT("Local synthetic test / no deliveries created"),24,912,352);
  Box(24,990,352,124,Muted);Wrap(PC->TrafficStatus,1006,39,4);
  return;
 }
 const auto& G=PC->RouteNetwork;const auto* N=G.Find(PC->NetworkSelected);int32 Links=0;for(const auto& V:G.Nodes) Links+=V.Data.Links.Num();
 Text(TEXT("PROPERTY ROUTES"),24,256,2.f,Ink);Text(FString::Printf(TEXT("%d points  /  %d connections"),G.Nodes.Num(),Links),25,289,1.15f,Ink);
 AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {Site=*It;break;}const bool EditError=Site && Site->IsConstructionFeedbackError();
 const auto& Check=PC->NetworkCheck;const auto Color=EditError?Red:Check.Success?(Check.Comfortable?Green:Orange):Check.Revision>=0?Red:Orange;
 Box(24,316,352,114,Muted);Box(24,316,4,114,Color);Text(EditError?TEXT("EDIT BLOCKED"):PC->IsRouteCheckPending()?TEXT("CHECKING TRAILER SWEEP"):Check.Success?(Check.Comfortable?TEXT("ROUTE CLEAR"):TEXT("ROUTE FITS / TIGHT")):Check.Revision>=0?TEXT("ROUTE NEEDS SPACE"):TEXT("DRAW / CONNECT / CHECK"),40,332,1.45f,Color);Wrap(EditError?Site->GetConstructionFeedbackText():PC->NetworkStatus,362,39,3);
 const auto Number=[&](FGuid Id){const int32 I=G.Nodes.IndexOfByPredicate([&](const auto& V){return V.Data.Id==Id;});return I<0?FString(TEXT("--")):FString::Printf(TEXT("%02d"),I+1);};
 Text(TEXT("1   DRAW YOUR ROUTE"),25,447,1.4f,Ink);
 Button(TEXT("NetNew"),TEXT("DRAW A NEW ROUTE"),TEXT("Click paving to add points / right-click to finish"),24,476,352,PC->BuildTool==TEXT("NetDraw"));
 Box(24,552,352,84,Muted);Text(TEXT("CLICK AN EXISTING POINT TO JOIN"),40,567,1.35f,Ink);
 Text(TEXT("Start and finish update as you draw."),40,597,1.25f,Ink);
 Text(TEXT("2   CHECK THE WHOLE VEHICLE"),25,654,1.4f,Ink);
 Button(TEXT("NetCheck"),PC->IsRouteCheckPending()?TEXT("CHECKING TURNING SPACE..."):TEXT("TEST DRIVE"),TEXT("Checks every turn, the cab and the full trailer"),24,684,352,Check.Success);
 Text(TEXT("START ")+Number(PC->NetworkStart)+TEXT("  >  FINISH ")+Number(PC->NetworkFinish),40,765,1.5f,Ink);
 Button(TEXT("NetPlay"),PC->bNetworkPlaying?TEXT("PAUSE TEST DRIVE"):TEXT("PLAY TEST DRIVE"),TEXT("Preview only / no delivery is scheduled"),24,800,352,PC->bNetworkPlaying);
 Button(TEXT("NetSelect"),TEXT("EDIT A ROUTE POINT"),TEXT("Select a numbered point on the map"),24,886,352,PC->BuildTool==TEXT("NetSelect"));
 Button(TEXT("NetAdvanced"),PC->bNetworkAdvanced?TEXT("HIDE POINT SETTINGS"):TEXT("POINT SETTINGS +"),TEXT("Move / delete / radius / traffic direction"),24,974,352,PC->bNetworkAdvanced);
 if(N) Text(FString::Printf(TEXT("Selected point %02d / turn space %.0f m"),G.Nodes.IndexOfByPredicate([&](const auto& V){return V.Data.Id==N->Data.Id;})+1,N->Data.RadiusCm/100.f),25,1060,1.2f,Ink);
 // Advanced edits stay with the selected point. Primary drawing needs no endpoints menu.
 if(PC->bNetworkAdvanced) {
  const float AX=Canvas->ClipX/S-656,AY=600;Box(AX,AY,632,554,Paper);Box(AX,AY,4,554,Orange);
  Text(N?TEXT("SELECTED POINT SETTINGS"):TEXT("SELECT A NUMBERED POINT FIRST"),AX+18,AY+18,1.6f,Ink);
  Button(TEXT("NetMove"),TEXT("MOVE POINT"),TEXT("Click its new position"),AX+18,AY+60,188,PC->BuildTool==TEXT("NetMove"));Button(TEXT("NetDelete"),TEXT("DELETE POINT"),TEXT("Removes its connections"),AX+222,AY+60,188);
  Button(TEXT("NetUnlink"),TEXT("UNLINK"),TEXT("Click connected point"),AX+426,AY+60,188,PC->BuildTool==TEXT("NetUnlink"));
  Button(TEXT("NetRadius"),TEXT("MORE TURN SPACE"),TEXT("Increase preferred radius"),AX+18,AY+138,290);Button(TEXT("NetFit"),TEXT("MINIMUM TURN"),!PC->LogisticsTrip.Semi?TEXT("4 m car turning radius"):PC->LogisticsTrip.SmallTruck?TEXT("7 m box truck turning radius"):TEXT("10 m semi rear-axle radius"),AX+324,AY+138,290);
  Button(TEXT("NetStaging"),TEXT("WAITING POINT"),TEXT("Mark staging / truck must still fit"),AX+18,AY+216,290,N && N->Data.Staging);
  Button(TEXT("NetTwoWay"),PC->bNetworkTwoWay?TEXT("BOTH DIRECTIONS"):TEXT("ONE-WAY"),TEXT("Applies to NEW connections"),AX+324,AY+216,290,PC->bNetworkTwoWay);
  Button(TEXT("NetReverse"),PC->bNetworkReverse?TEXT("BACKING CONNECTION"):TEXT("FORWARD CONNECTION"),TEXT("Applies to NEW connections"),AX+18,AY+294,596,PC->bNetworkReverse);
  Text(TEXT("OPTIONAL / TEST ONLY PART OF A NETWORK"),AX+18,AY+376,1.25f,Ink);
  Button(TEXT("NetStart"),TEXT("TEST START HERE"),TEXT("Use selected point"),AX+18,AY+408,290);Button(TEXT("NetFinish"),TEXT("TEST FINISH HERE"),TEXT("Use selected point"),AX+324,AY+408,290);
  Text(TEXT("Blue arrows = forward / amber arrows = backing"),AX+18,AY+502,1.25f,Ink);
 }
 // Vehicle and envelope controls are map-side, kept out of the fixed persistence footer.
 const float X=Canvas->ClipX/S-270;
 Button(TEXT("NetBox"),TEXT("BOX TRUCK"),TEXT("8 m rigid truck / 7 m turning radius"),X,276,246,PC->LogisticsTrip.Semi && PC->LogisticsTrip.SmallTruck);
 Button(TEXT("NetTruck"),TEXT("SEMI + TRAILER"),TEXT("Requires a larger manoeuvring area"),X,350,246,PC->LogisticsTrip.Semi && !PC->LogisticsTrip.SmallTruck);
 Button(TEXT("NetCar"),TEXT("PASSENGER CAR"),TEXT("Independent vehicle profile"),X,424,246,!PC->LogisticsTrip.Semi);
 Button(TEXT("NetSweep"),TEXT("SWEPT CLEARANCE"),TEXT("Full vehicle envelope"),X,498,246,PC->bNetworkSweep);
}
