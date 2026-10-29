#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "Construction/FoundrylineSizing.h"
#include "Core/FoundrylineUnits.h"

void AFoundrylineBuildHUD::DrawText(const FString& T,FLinearColor C,float X,float Y,UFont* Font,float Scale,bool ScalePosition){Super::DrawText(FFoundrylineUnits::Display(T),C,X,Y,Font,Scale,ScalePosition);}
void AFoundrylineBuildHUD::GetTextSize(const FString& T,float& W,float& H,UFont* Font,float Scale)const{Super::GetTextSize(FFoundrylineUnits::Display(T),W,H,Font,Scale);}

namespace {const TCHAR* PaintHex[]={TEXT("F7F5F1"),TEXT("1F2327"),TEXT("F07A2F"),TEXT("4B5258"),TEXT("9AA0A6"),TEXT("2F6B8F"),TEXT("62745B"),TEXT("946953")};FLinearColor Brand(const TCHAR* Hex) {return FLinearColor(FColor::FromHex(Hex));}}

void AFoundrylineBuildHUD::DrawHUD()
{
	Super::DrawHUD();auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(!Canvas || !PC) return;
 PC->BuildHUDCanvasSize={Canvas->ClipX,Canvas->ClipY};
 // The brand mark is shared by every screen, so load it before the first-person card can draw.
 if(!Logo) Logo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Foundryline/UI/Brand/T_FoundrylineLogo.T_FoundrylineLogo"));
 if(DrawOperations(PC))return;if(DrawFactory(PC))return;
 DrawMaterialHandling(PC);if(PC->LocalMaterialCell) return;
 DrawFreightControls(PC);if(PC->LocalFreightRig) return;
 if(!PC->IsWalking()) {DrawSelectionOutlines(PC);DrawPlacementGuide(PC);}
 if(PC->IsLogisticsOverlayVisible() && !PC->IsWalking()) DrawLogisticsMap(PC);
	const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),Dt=FMath::Min(GetWorld()->GetDeltaSeconds(),.1f);
	const auto Ink=Brand(TEXT("1F2327")),Graphite=Brand(TEXT("4B5258")),Steel=Brand(TEXT("9AA0A6")),Paper=Brand(TEXT("F7F5F1")),Orange=Brand(TEXT("F07A2F")),Muted=Brand(TEXT("E8E7E2"));
	auto* Font=GEngine->GetMediumFont();
	auto Rect=[&](float X,float Y,float W,float H,FLinearColor C){DrawRect(C,X*S,Y*S,W*S,H*S);};
	auto Line=[&](float X,float Y,float A,float B,FLinearColor C,float Width=2.f){DrawLine(X*S,Y*S,A*S,B*S,C,Width*S);};
	auto Text=[&](const FString& V,float X,float Y,float Size,FLinearColor C){DrawText(V,C,X*S,Y*S,Font,Size*S);};
	auto Shape=[&](float X,float Y,float W,float H,FLinearColor C)
	{
		const FVector2D P[]={{X*S,Y*S},{(X+W-12)*S,Y*S},{(X+W)*S,(Y+12)*S},{(X+W)*S,(Y+H)*S},{X*S,(Y+H)*S}};
		for(int32 I=1;I<4;++I) {FCanvasTriangleItem T(P[0],P[I],P[I+1],GWhiteTexture);T.SetColor(C);Canvas->DrawItem(T);}
	};
	auto Animated=[&](FName Name,float X,float Y,float W,float H,bool Selected)
	{
		auto& Hover=HoverAmounts.FindOrAdd(Name);Hover=FMath::FInterpTo(Hover,HoveredBox==Name?1.f:0.f,Dt,14.f);
		auto& Press=PressAmounts.FindOrAdd(Name);Press=FMath::FInterpTo(Press,0.f,Dt,20.f);
		const float Lift=Hover*4-Press*3;
		Rect(X+Lift,Y,W-Lift,H,Selected?Orange:FMath::Lerp(Ink,Graphite,Hover*.7f));
		const auto Accent=Selected?Ink:Orange;
		if(Selected || Hover>.05f) Rect(X+Lift,Y+9,3,H-18,Accent);
		if(Hover>.01f) Line(X+8,Y+H-1,X+W-2,Y+H-1,FLinearColor(Orange.R,Orange.G,Orange.B,Hover),2);
		AddHitBox({X*S,Y*S},{W*S,H*S},Name,true);
		return Hover;
	};
 const float ViewX=Canvas->ClipX/S-270;
 if(!PC->IsWalking()&&PC->bPropertyViewAccess){Animated(TEXT("WalkView"),ViewX-60,24,178,40,false);Text(TEXT("RETURN TO OFFICE [F]"),ViewX-50,37,1.05f,Paper);}
 if(!PC->IsWalking())
 {
  Animated(TEXT("RoofView"),ViewX+128,24,118,40,PC->AreRoofsShown());Text(PC->AreRoofsShown()?TEXT("ROOFS ON"):TEXT("ROOFS OFF"),ViewX+138,37,1.2f,PC->AreRoofsShown()?Ink:Paper);
 }

 if(!PC->IsWalking()) {
  Animated(TEXT("LogisticsOverlay"),ViewX,76,246,40,PC->IsLogisticsOverlayVisible());Text(TEXT("LOGISTICS"),ViewX+14,89,1.35f,PC->IsLogisticsOverlayVisible()?Ink:Paper);
  if(PC->IsLogisticsOverlayVisible()) {Animated(TEXT("EditLogistics"),ViewX,128,246,40,PC->IsEditingLogistics());Text(PC->IsEditingLogistics()?TEXT("EDITING ROUTES"):TEXT("EDIT ROUTES"),ViewX+14,141,1.25f,PC->IsEditingLogistics()?Ink:Paper);}
 }
 if(PC->IsWalking())
 {
  const float CX=Canvas->ClipX/S/2,CY=Canvas->ClipY/S/2;Line(CX-5,CY,CX+5,CY,Paper,1);Line(CX,CY-5,CX,CY+5,Paper,1);
  Rect(24,24,374,78,Paper);Rect(24,24,4,78,Orange);if(Logo)DrawTexture(Logo,38*S,35*S,132*S,48.1*S,0,0,1,1);if(auto* Plant=PC->FactoryPlant()){Text(FFoundrylineCommerceCatalog::Money(Plant->Snapshot.Commerce.CashCents),190,38,1.3f,Ink);Text(FFoundrylineCommerceCatalog::Date(Plant->Snapshot.Commerce.Clock),190,69,.93f,Graphite);}Rect(24,Canvas->ClipY/S-62,555,42,Paper);Text(TEXT("WASD  Walk     Mouse  Look     E  Interact"),38,Canvas->ClipY/S-49,1.13f,Ink);
  if(auto* Door=PC->FocusedDoor()){Rect(CX-130,CY+40,260,46,Paper);Rect(CX-130,CY+40,4,46,Orange);Text(Door->IsOpen()?TEXT("[E] CLOSE DOOR"):TEXT("[E] OPEN DOOR"),CX-108,CY+56,1.25f,Ink);}
  return;
 }
	Rect(400,0,12,Canvas->ClipY/S,FLinearColor(0,0,0,.18));Rect(0,0,400,Canvas->ClipY/S,Paper);Rect(398,0,2,Canvas->ClipY/S,Ink);
    AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){Site=*It;break;}
    // A fixed footer keeps history and persistence reachable in every construction tab.
    const float Bottom=Canvas->ClipY/S;
    Rect(0,Bottom-184,398,184,Ink);Line(24,Bottom-184,376,Bottom-184,Orange,3);
    auto FooterButton=[&](FName Key,const TCHAR* Label,float X,float Y,float W,bool Enabled)
    {
        if(Enabled) Animated(Key,X,Y,W,42,false);else Shape(X,Y,W,42,Graphite*.55f);
        Text(Label,X+12,Y+13,1.35f,Enabled?Paper:Steel);
    };
 if(PC->IsLogisticsOverlayVisible()) {
  Text(PC->LogisticsPanel==1?TEXT("TRIP PREVIEW / YOUR VIEW"):TEXT("ROUTE NETWORK / COMPANY"),24,Bottom-159,1.35f,Paper);
  Text(TEXT("Nodes, links and driving zones are shared."),24,Bottom-127,1.15f,Steel);
  FooterButton(TEXT("SaveProperty"),GetNetMode()==NM_Client?TEXT("HOST SAVES ROUTE NETWORK"):TEXT("SAVE PROPERTY & ROUTES"),24,Bottom-96,352,GetNetMode()!=NM_Client && Site && !Site->IsPropertyBusy());
  Text(TEXT("Preview does not schedule a delivery."),24,Bottom-32,1.15f,Orange);
 } else {
    FooterButton(TEXT("Undo"),GetNetMode()==NM_Standalone?TEXT("UNDO  [Z]"):TEXT("UNDO / SOLO"),24,Bottom-169,170,GetNetMode()==NM_Standalone && Site && Site->CanUndo() && !Site->IsPropertyBusy());
    FooterButton(TEXT("Redo"),GetNetMode()==NM_Standalone?TEXT("REDO  [Y]"):TEXT("REDO / SOLO"),206,Bottom-169,170,GetNetMode()==NM_Standalone && Site && Site->CanRedo() && !Site->IsPropertyBusy());
    FooterButton(TEXT("SaveProperty"),TEXT("SAVE PROPERTY"),24,Bottom-115,170,GetNetMode()!=NM_Client && Site && !Site->IsPropertyBusy());
    FooterButton(TEXT("LoadProperty"),bConfirmLoad?TEXT("CONFIRM LOAD"):TEXT("LOAD PROPERTY"),206,Bottom-115,170,GetNetMode()!=NM_Client && Site && !Site->IsPropertyBusy());
    Text(GetNetMode()==NM_Client?FString(TEXT("SHARED PROPERTY  /  HOST SAVES")):Site?(Site->IsPropertyBusy()?Site->PropertyStatus:Site->HasUnsavedChanges()?TEXT("PROPERTY 01  /  UNSAVED CHANGES"):TEXT("PROPERTY 01  /  SAVED")):TEXT("PROPERTY 01"),24,Bottom-56,1.25f,Orange);
    const int32 Step=FFoundrylineGrid::SnapStep(PC->GetPlacementSnap());
    Text(bConfirmLoad?TEXT("Click load again to replace current changes"):FString::Printf(TEXT("%s SNAP  /  Right-click cancels  /  1:1 SCALE"),Step==100?TEXT("1 m"):*FString::Printf(TEXT("%d cm"),Step)),24,Bottom-30,1.12f,Steel);
 }
	if(Logo) DrawTexture(Logo,24*S,22*S,352*S,128.3f*S,0,0,1,1,FLinearColor::White);
	else Text(TEXT("FOUNDRYLINE"),24,72,2.8f,Ink);
	Line(24,169,91,169,Orange,4);Line(101,169,376,169,Steel,1);
	if(PC->IsLogisticsOverlayVisible()) {
  const TCHAR* Keys[]={TEXT("NetPanel"),TEXT("TripPanel"),TEXT("TrafficPanel")};const TCHAR* Names[]={TEXT("ROUTES"),TEXT("TRIPS"),TEXT("TRAFFIC")};
  for(int32 I=0;I<3;++I) {Animated(FName(Keys[I]),24+I*120,188,112,44,PC->LogisticsPanel==I);Text(Names[I],39+I*120,203,1.4f,PC->LogisticsPanel==I?Ink:Paper);}
 } else {Text(TEXT("CONSTRUCTION"),24,188,2.f,Ink);}
 if(PC->IsLogisticsOverlayVisible() && PC->LogisticsPanel!=1) {DrawNetworkPanel(PC);return;}
 if(PC->IsLogisticsOverlayVisible()) {
  auto* Paths=GetWorld()->GetSubsystem<UFoundrylineVehiclePathSubsystem>();const auto Map=Paths->GetMap();const auto& Trip=PC->LogisticsTrip;const auto& Result=PC->TripResult;
  const auto Green=Brand(TEXT("24835C")),Blue=Brand(TEXT("287EB1")),Red=Brand(TEXT("B64032"));
  int32 Painted=0;for(const auto& Surf:Map->Surfaces) Painted+=Surf.TruckZones.Num();
  const bool Progress[]={Trip.HasEntry,!Trip.Semi || Painted>0,Trip.TargetId.IsValid(),Trip.HasExit,Result.Success};
  const int32 ActiveStep=PC->BuildTool==TEXT("TripEntrance")?0:PC->BuildTool==TEXT("RoutePaint") || PC->BuildTool==TEXT("RouteErase")?1:PC->BuildTool==TEXT("TripDestination")?2:PC->BuildTool==TEXT("TripExit")?3:4;
  for(int32 I=0;I<5;++I) Rect(24+I*72,241,64,5,I==ActiveStep?Orange:Progress[I]?Green:Steel);
  auto Wrap=[&](const FString& V,float X,float Y,int32 Width,int32 Limit,float Size,FLinearColor C){TArray<FString> Words;V.ParseIntoArrayWS(Words);FString Row;int32 N=0;for(const auto& Word:Words){if(Row.Len()+Word.Len()>Width && !Row.IsEmpty()){Text(Row,X,Y+N*20,Size,C);Row.Empty();if(++N>=Limit) return;}if(!Row.IsEmpty()) Row+=TEXT(" ");Row+=Word;}if(!Row.IsEmpty()) Text(Row,X,Y+N*20,Size,C);};
  const bool ZoneTool=PC->BuildTool==TEXT("RoutePaint") || PC->BuildTool==TEXT("RouteErase");const bool Error=(Result.Revision>=0 && !Result.Success) || (ZoneTool && Site && Site->IsConstructionFeedbackError());
  Rect(24,254,352,104,Muted);Rect(24,254,5,104,Error?Red:Result.Success?Green:Orange);
  Text(PC->IsRouteCheckPending()?TEXT("CHECKING BOTH DIRECTIONS"):Error?TEXT("TRIP NEEDS ATTENTION"):Result.Success?TEXT("FULL TRIP CONNECTED"):(ActiveStep==0?TEXT("STEP 1 / CHOOSE ENTRANCE"):ActiveStep==1?TEXT("STEP 2 / PAINT TRUCK ACCESS"):ActiveStep==2?TEXT("STEP 3 / CHOOSE DESTINATION"):ActiveStep==3?TEXT("STEP 4 / CHOOSE EXIT"):TEXT("STEP 5 / TEST THE TRIP")),38,268,1.4f,Error?Red:Ink);
  FString Message=ZoneTool && Site && Site->IsConstructionFeedbackError()?Site->GetConstructionFeedbackText():PC->LogisticsStatus;
  Wrap(Message,38,297,39,3,1.35f,Graphite);
  Animated(TEXT("TripBox"),24,376,112,44,Trip.Semi && Trip.SmallTruck);Text(TEXT("BOX TRUCK"),34,391,1.15f,Trip.Semi && Trip.SmallTruck?Ink:Paper);
  Animated(TEXT("TripTruck"),144,376,112,44,Trip.Semi && !Trip.SmallTruck);Text(TEXT("FULL SEMI"),155,391,1.15f,Trip.Semi && !Trip.SmallTruck?Ink:Paper);
  Animated(TEXT("TripCar"),264,376,112,44,!Trip.Semi);Text(TEXT("CAR"),299,391,1.15f,!Trip.Semi?Ink:Paper);
  auto Step=[&](FName Key,const TCHAR* Number,const TCHAR* Title,const FString& Detail,float Y,bool Complete){const bool Active=PC->BuildTool==Key;Animated(Key,24,Y,352,78,Active);Rect(38,Y+16,36,36,Complete?Green:Paper);Text(Complete?TEXT("OK"):Number,43,Y+25,1.5f,Complete?Paper:Ink);Text(Title,88,Y+15,1.65f,Active?Ink:Paper);Text(Detail,88,Y+46,1.2f,Active?Ink:Steel);};
  Step(TEXT("TripEntrance"),TEXT("1"),TEXT("ENTRANCE"),Trip.HasEntry?Trip.Entrance.Label:TEXT("Click a gate on the map"),436,Trip.HasEntry);
  if(Trip.Semi) {
   int32 Zones=0;for(const auto& Surf:Map->Surfaces) Zones+=Surf.TruckZones.Num();
   Text(TEXT("2   PAINT WHERE TRUCKS MAY DRIVE"),26,532,1.45f,Ink);
   Animated(TEXT("RoutePaint"),24,562,228,50,PC->BuildTool==TEXT("RoutePaint"));Text(TEXT("PAINT DRIVING AREA"),40,579,1.35f,PC->BuildTool==TEXT("RoutePaint")?Ink:Paper);
   Animated(TEXT("RouteErase"),264,562,112,50,PC->BuildTool==TEXT("RouteErase"));Text(TEXT("ERASE"),282,579,1.3f,PC->BuildTool==TEXT("RouteErase")?Ink:Paper);
   Text(Zones?TEXT("Green fill = allowed. Test checks turning room."):TEXT("No green area yet. Drag over connected paving."),25,622,1.15f,Zones?Green:Red);
  } else {Rect(24,532,352,105,Muted);Text(TEXT("2   USE ROADS & PARKING AISLES"),38,549,1.4f,Blue);Wrap(TEXT("Cars use connected paving automatically. Truck paint is not needed for parking."),38,578,39,2,1.25f,Graphite);}
  Step(TEXT("TripDestination"),TEXT("3"),Trip.Semi?TEXT("LOADING / STAGING"):TEXT("PARKING SPACE"),Trip.TargetId.IsValid()?FFoundrylineLogisticsTrip::TargetLabel(*Map,Trip.TargetId):Trip.Semi?TEXT("Click a numbered amber marker"):TEXT("Click a blue bay"),658,Trip.TargetId.IsValid());
  Step(TEXT("TripExit"),TEXT("4"),TEXT("EXIT TO THE STREET"),Trip.HasExit?Trip.Departure.Label:TEXT("A two-way gate can be used again"),750,Trip.HasExit);
  const bool Ready=Trip.Ready(*Map);Animated(TEXT("TripTest"),24,846,352,66,Ready && !PC->IsRouteCheckPending());Text(PC->IsRouteCheckPending()?TEXT("CHECKING TRIP..."):TEXT("5   TEST THE WHOLE TRIP"),43,869,1.7f,Ready && !PC->IsRouteCheckPending()?Ink:Paper);
  const FString In=Result.Inbound.Success?TEXT("ARRIVAL OK"):Result.Revision>=0?TEXT("ARRIVAL BLOCKED"):TEXT("ARRIVAL --"),Out=Result.Outbound.Success?TEXT("EXIT OK"):Result.Revision>=0 && Result.Inbound.Success?TEXT("EXIT BLOCKED"):TEXT("EXIT --");
  Text(In+TEXT("   >   STOP   >   ")+Out,25,925,1.25f,Result.Success?Green:Error?Red:Graphite);
  if(Result.Success) {Animated(TEXT("TripPlay"),24,956,170,44,PC->bTripPlaying);Text(PC->bTripPlaying?TEXT("PAUSE PREVIEW"):TEXT("PLAY PREVIEW"),37,971,1.25f,PC->bTripPlaying?Ink:Paper);Animated(TEXT("TripRestart"),206,956,170,44,false);Text(TEXT("RESTART"),237,971,1.3f,Paper);}
  else Text(TEXT("A test checks arrival AND departure."),25,971,1.25f,Graphite);
  const TCHAR* FilterKeys[]={TEXT("ShowBothRoutes"),TEXT("ShowArrival"),TEXT("ShowDeparture")};const TCHAR* FilterLabels[]={TEXT("BOTH PATHS"),TEXT("ARRIVAL"),TEXT("DEPARTURE")};
  for(int32 I=0;I<3;++I) {const float X=24+I*120;Animated(FName(FilterKeys[I]),X,1005,112,36,PC->LogisticsRouteView==I);Text(FilterLabels[I],X+10,1017,1.1f,PC->LogisticsRouteView==I?Ink:Paper);if(I) Line(X+6,1039,X+106,1039,I==1?Blue:Orange,3);}
  Text(TEXT("VISIBLE AREAS"),25,1055,1.15f,Graphite);
  Animated(TEXT("TruckLayer"),24,1078,170,40,PC->bShowTruckLayer);Text(TEXT("TRUCK ACCESS"),42,1091,1.2f,PC->bShowTruckLayer?Ink:Paper);
  Animated(TEXT("ParkingLayer"),206,1078,170,40,PC->bShowParkingLayer);Text(FString::Printf(TEXT("PARKING / %d"),Paths->GetParkingTargetCount()),222,1091,1.2f,PC->bShowParkingLayer?Ink:Paper);
  return;
 }
 if(!bEditTab && !bOfficeTab && !bPaintTab && FFoundrylineSizing::Surface(PC->BuildTool)) DrawSurfaceSizing(PC);
 const FLinearColor FeedbackColor=Site && Site->IsConstructionFeedbackError()?FLinearColor(0.85f,0.08f,0.06f):FLinearColor(0.03f,0.4f,0.85f);
 Rect(24,254,352,92,Muted);Rect(24,254,3,92,FeedbackColor);
 Text(Site?Site->GetConstructionFeedbackTitle():TEXT("CONSTRUCTION"),38,263,1.3f,FeedbackColor);
 FString Status=Site?Site->GetConstructionFeedbackText():TEXT("Choose a tool to start building.");
 TArray<FString> Words;Status.ParseIntoArrayWS(Words);FString Row;int32 RowIndex=0;
 for(const auto& Word:Words) {if(Row.Len()+Word.Len()>41 && !Row.IsEmpty()){Text(Row,38,285+RowIndex*20,1.35f,Ink);Row.Empty();if(++RowIndex==3) break;}if(!Row.IsEmpty()) Row+=TEXT(" ");Row+=Word;}
 if(RowIndex<3 && !Row.IsEmpty()) Text(Row,38,285+RowIndex*20,1.35f,Ink);
	const TCHAR* Tabs[]={TEXT("BuildTab"),TEXT("EditTab"),TEXT("OfficeTab"),TEXT("PaintTab")};const TCHAR* TabLabels[]={TEXT("BUILD"),TEXT("EDIT"),TEXT("OFFICE"),TEXT("PAINT")};
	for(int32 I=0;I<4;++I){const bool Active=I==3?bPaintTab:I==2?bOfficeTab:I==1?bEditTab:!bEditTab&&!bOfficeTab&&!bPaintTab;const float X=24+I*90;const FName Key(Tabs[I]);Rect(X,354,82,46,Active?Muted:Paper);Text(TabLabels[I],X+10,368,1.35f,Ink);if(Active) Line(X,400,X+82,400,Orange,3);AddHitBox({X*S,354*S},{82*S,46*S},Key,true);}
 if(bEditTab)
 {
    const auto* O=Site?Site->GetSelectedObject():nullptr;
    const auto* P=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
    const auto* D=O&&P?P->FindDefinition(O->DefinitionId):nullptr;
    Text(TEXT("PROPERTY EDITOR"),24,430,1.5f,Graphite);
    Rect(24,478,352,122,Muted);Rect(24,478,4,122,O?Orange:Steel);
    FString Title=TEXT("SELECT AN OBJECT");
    if(D) Title=O->bOfficeAnnex?TEXT("OFFICE ANNEX"):D->FunctionId==TEXT("BuildingShell")?(D->Footprint.Max.Z>620?TEXT("TIER 2 SHOP"):TEXT("TIER 1 SHOP")):D->FunctionId.ToString().ToUpper();
    Text(Title,40,496,1.9f,Ink);
    Text(D?FString::Printf(TEXT("%.1f x %.1f m  /  %d degrees"),D->Footprint.Max.X/100.f,D->Footprint.Max.Y/100.f,O->QuarterTurns*90):TEXT("Click a building, road or parking area"),40,533,1.35f,Graphite);
    Text(O?TEXT("Orange outline  /  Corner resize handles"):TEXT("Hover to highlight  /  Click to inspect"),40,562,1.2f,Graphite);
    const TCHAR* Keys[]={TEXT("Select"),TEXT("Move"),TEXT("Resize"),TEXT("Rotate"),TEXT("Duplicate"),TEXT("MoveDoor"),TEXT("Demolish"),TEXT("Cancel")};
    const TCHAR* Labels[]={TEXT("SELECT"),TEXT("MOVE"),TEXT("RESIZE"),TEXT("ROTATE 90"),TEXT("DUPLICATE"),TEXT("MOVE DOOR"),TEXT("DELETE"),TEXT("CANCEL")};
    const TCHAR* Tips[]={TEXT("Click to inspect"),TEXT("Drag to relocate"),TEXT("Drag nearest corner"),TEXT("Click to turn"),TEXT("Drag a new copy"),TEXT("Drag an opening"),TEXT("Click / Undo restores"),TEXT("Return to selection")};
    for(int32 I=0;I<8;++I)
    {
        const float X=24+(I%2)*182,Y=620+(I/2)*91;const bool Active=PC->BuildTool==FName(Keys[I]);
        const FName Key(Keys[I]);auto& Hover=HoverAmounts.FindOrAdd(Key);Hover=FMath::FInterpTo(Hover,HoveredBox==Key?1.f:0.f,Dt,14.f);
        Rect(X,Y,170,78,Active?Muted:FMath::Lerp(Paper,Muted,Hover*.6f));if(Active) Rect(X,Y+12,3,48,Orange);
        Text(Labels[I],X+15,Y+17,1.4f,Ink);if(Active || Hover>.1f) Text(Tips[I],X+15,Y+48,1.05f,Graphite);
        Line(X+15,Y+78,X+155,Y+78,Muted,1);AddHitBox({X*S,Y*S},{170*S,78*S},Key,true);
    }
    Text(TEXT("PARKING & STREET DETAILS"),24,1012,1.25f,Graphite);
    Animated(TEXT("Landscape"),24,1045,170,52,PC->BuildTool==TEXT("Landscape"));Text(TEXT("GREENERY"),44,1063,1.4f,Paper);
    Animated(TEXT("Crosswalk"),206,1045,170,52,PC->BuildTool==TEXT("Crosswalk"));Text(TEXT("CROSSWALK"),221,1063,1.4f,Paper);
    return;
 }
 if(bPaintTab)
 {
  const auto* Building=Site?Site->GetSelectedBuilding():nullptr;
  Text(TEXT("SELECT IN EDIT  /  THEN PICK A FINISH"),24,430,1.25f,Graphite);
  Text(Building?(Building->bOfficeAnnex?TEXT("OFFICE COLORS"):TEXT("SHOP COLORS")):TEXT("NO BUILDING SELECTED"),24,478,2.f,Ink);
  const TCHAR* Titles[]={TEXT("EXTERIOR WALLS"),TEXT("INTERIOR WALLS"),TEXT("TRIM / SECONDARY")};
  for(int32 C=0;C<3;++C)
  {
   const float Y=540+C*155;Text(Titles[C],24,Y,1.65f,Ink);
   for(int32 I=0;I<8;++I)
   {
    const float X=24+I*44;const auto Color=Brand(PaintHex[I]);const FName Key(*FString::Printf(TEXT("Paint_%d_%d"),C,I));
    const auto Current=Building?(C==0?Building->Colors.Exterior:C==1?Building->Colors.Interior:Building->Colors.Trim):FLinearColor::Transparent;
    Rect(X,Y+33,38,48,Current.Equals(Color,.001f)?Orange:Steel);Rect(X+3,Y+36,32,42,Color);
    if(HoveredBox==Key) Line(X,Y+86,X+38,Y+86,Orange,3);
    AddHitBox({X*S,(Y+33)*S},{38*S,48*S},Key,true);
   }
  }
  Text(TEXT("First three: Foundryline cream / coal / orange"),24,1020,1.2f,Graphite);

  return;
 }
	if(!bEditTab && !bOfficeTab)
	{
		const TCHAR* Names[]={TEXT("Structures"),TEXT("Roads"),TEXT("Access")};const TCHAR* Titles[]={TEXT("SHOPS"),TEXT("SITE"),TEXT("DOORS")};
		for(int32 I=0;I<3;++I) {const float X=24+I*120;const FName Key(*FString::Printf(TEXT("Category%s"),Names[I]));auto& A=HoverAmounts.FindOrAdd(Key);A=FMath::FInterpTo(A,HoveredBox==Key?1.f:0.f,Dt,14.f);const bool Active=BuildCategory==Names[I];Rect(X,418,112,36,FMath::Lerp(Muted,Paper,A));Text(Titles[I],X+20,429,1.4f,Active?Ink:Graphite);Line(X,455,X+112,455,Active?Orange:Steel,Active?3.f:1.f);AddHitBox({X*S,418*S},{112*S,38*S},Key,true);}
	}
	else Text(bOfficeTab?TEXT("EXTERIOR ANNEX  /  ROOMS & ACCESS"):TEXT("SELECT  /  ADJUST  /  RESTORE"),24,430,1.3f,Graphite);
	TArray<FName> Tools=bOfficeTab?TArray<FName>{TEXT("OfficeArea"),TEXT("OfficeRoom"),TEXT("RoomDoor"),TEXT("RoomWindow")}:bEditTab?TArray<FName>{TEXT("Select"),TEXT("Move"),TEXT("Resize"),TEXT("MoveDoor"),TEXT("Demolish"),TEXT("Landscape"),TEXT("Crosswalk")}:
		TArray<FName>{TEXT("Building"),TEXT("Shop2"),TEXT("Service"),TEXT("Road2"),TEXT("Road4"),TEXT("Parking"),TEXT("Personnel"),TEXT("Garage"),TEXT("TruckDoor"),TEXT("Columns"),TEXT("Crosswalk"),TEXT("Driveway"),TEXT("TruckAccess"),TEXT("FreightYard"),TEXT("LoadingDock"),TEXT("Walkway")};
	TArray<FString> Labels=bOfficeTab?TArray<FString>{TEXT("EXTERIOR OFFICE"),TEXT("DRAW ROOM"),TEXT("OFFICE DOOR"),TEXT("OFFICE WINDOW")}:bEditTab?TArray<FString>{TEXT("SELECT SECTION"),TEXT("MOVE SECTION"),TEXT("RESIZE SECTION"),TEXT("RELOCATE DOOR"),TEXT("REMOVE SECTION"),TEXT("PARKING GREENERY"),TEXT("CROSSWALK")}:
		TArray<FString>{TEXT("TIER 1"),TEXT("TIER 2 SHOP"),TEXT("SERVICE ROAD"),TEXT("TWO-LANE ROAD"),TEXT("FOUR-LANE ROAD"),TEXT("PARKING LOT"),TEXT("PERSONNEL DOOR"),TEXT("GARAGE DOOR"),TEXT("SEMI ENTRANCE"),TEXT("COLUMN BAYS"),TEXT("CROSSWALK"),TEXT("PARKING ENTRANCE"),TEXT("TRUCK DRIVEWAY"),TEXT("LOADING PAVEMENT"),TEXT("LOADING DOOR"),TEXT("WALKWAY")};
	TArray<FString> Details=bOfficeTab?TArray<FString>{TEXT("8 m minimum  /  Attach to shop wall"),TEXT("4 m minimum  /  3 m high partitions"),TEXT("Factory glazed leaf  /  1 x 2.5 m"),TEXT("Manual  /  Exterior or room wall")}:bEditTab?TArray<FString>{TEXT("Click inside a built section"),TEXT("Drag a section  /  1 m snap"),TEXT("Drag a corner  /  Opposite stays fixed"),TEXT("Drag a door to another exterior wall"),TEXT("Click to remove  /  Z restores"),TEXT("Toggle grass islands and edging"),TEXT("Click a road  /  Zebra + curb ramps")}:
		TArray<FString>{TEXT("6 m  /  Light workshop"),TEXT("12 m  /  High-bay manufacturing"),TEXT("4 m wide"),TEXT("8 m wide  /  T and X junctions"),TEXT("14 m wide  /  Marked lanes"),TEXT("6 x 12 m minimum / Rotate layout with R"),TEXT("Factory shop leaf  /  1.2 x 2.5 m"),TEXT("4 x 4.2 m opening"),TEXT("5 x 5 m clear opening"),TEXT("Click a shop  /  4, 6 or 8 m"),TEXT("Click a road  /  Zebra + curb ramps"),TEXT("6 m driveway  /  Automatic curb cuts"),TEXT("Drag to connect / Adjustable width"),TEXT("Drag any rectangle / Level asphalt"),TEXT("Click a wall / Snaps to floor level"),TEXT("2 m minimum / Concrete pedestrian path")};
	if(!bEditTab && !bOfficeTab)
	{
		for(int32 I=Tools.Num()-1;I>=0;--I)
		{
			const bool Structure=Tools[I]==TEXT("Building") || Tools[I]==TEXT("Shop2") || Tools[I]==TEXT("Columns");
			const bool Door=Tools[I]==TEXT("Personnel") || Tools[I]==TEXT("Garage") || Tools[I]==TEXT("TruckDoor");
			if((BuildCategory==TEXT("Structures") && !Structure) || (BuildCategory==TEXT("Access") && !Door) || (BuildCategory==TEXT("Roads") && (Structure || Door))) {Tools.RemoveAt(I);Labels.RemoveAt(I);Details.RemoveAt(I);}
		}
	}
 const bool SiteTools=!bOfficeTab && BuildCategory==TEXT("Roads");
 float Y=478;
 if(SiteTools) {
  const TCHAR* Keys[]={TEXT("Streets"),TEXT("Parking"),TEXT("Freight")};const TCHAR* Titles[]={TEXT("ROADS"),TEXT("PARKING"),TEXT("LOADING")};
  for(int32 I=0;I<3;++I) {const float X=24+I*120;const FName Key(*FString::Printf(TEXT("SiteCategory%s"),Keys[I]));const bool Active=SiteCategory==Keys[I];Rect(X,Y,112,36,Active?Muted:Paper);Text(Titles[I],X+12,Y+12,1.2f,Ink);if(Active) Line(X,Y+36,X+112,Y+36,Orange,2);AddHitBox({X*S,Y*S},{112*S,36*S},Key,true);}
  for(int32 I=Tools.Num()-1;I>=0;--I) {const bool Parking=Tools[I]==TEXT("Parking") || Tools[I]==TEXT("Driveway"),Freight=Tools[I]==TEXT("TruckAccess") || Tools[I]==TEXT("FreightYard") || Tools[I]==TEXT("LoadingDock");if((SiteCategory==TEXT("Parking") && !Parking) || (SiteCategory==TEXT("Freight") && !Freight) || (SiteCategory==TEXT("Streets") && (Parking || Freight))) {Tools.RemoveAt(I);Labels.RemoveAt(I);Details.RemoveAt(I);}}
  if(SiteCategory==TEXT("Freight")) {
   Tools={TEXT("FreightYard"),TEXT("TruckAccess"),TEXT("LoadingDock")};
   Labels={TEXT("1  DRAW PAVEMENT"),TEXT("2  CONNECT DRIVEWAY"),TEXT("3  ADD LOADING DOOR")};
   Details={TEXT("Drag a rectangle beside the shop"),TEXT("Drag between pavement and road"),TEXT("Click the shop wall / Snaps in place")};
  }
  Y=542;
 }
	for(int32 I=0;I<Tools.Num();++I)
	{
		const bool Active=PC->BuildTool==Tools[I];auto& Hover=HoverAmounts.FindOrAdd(Tools[I]);Hover=FMath::FInterpTo(Hover,HoveredBox==Tools[I]?1.f:0.f,Dt,14.f);
  const bool ShowDetail=Active||Hover>.1f||(SiteTools&&SiteCategory==TEXT("Freight"));FString First=FFoundrylineUnits::Display(Details[I]),Second;
  if(ShowDetail){TArray<FString> DetailWords;First.ParseIntoArrayWS(DetailWords);First.Reset();for(int J=0;J<DetailWords.Num();++J){const FString Trial=First.IsEmpty()?DetailWords[J]:First+TEXT(" ")+DetailWords[J];float Width,Height;GetTextSize(Trial,Width,Height,Font,1.12f*S);if(Width>320*S&&!First.IsEmpty()){for(int K=J;K<DetailWords.Num();++K){if(!Second.IsEmpty())Second+=TEXT(" ");Second+=DetailWords[K];}break;}First=Trial;}}
  const float RowHeight=ShowDetail&&!Second.IsEmpty()?90:70;
  Rect(24,Y,352,RowHeight,Active?Muted:FMath::Lerp(Paper,Muted,Hover*.6f));if(Active) Rect(24,Y+10,3,RowHeight-24,Orange);
  Text(Labels[I],40+Hover*2,Y+(Active || Hover>.1f || (SiteTools && SiteCategory==TEXT("Freight"))?12:24),1.65f,Ink);
  if(ShowDetail){Text(First,40+Hover*2,Y+43,1.12f,Graphite);if(!Second.IsEmpty())Text(Second,40+Hover*2,Y+63,1.12f,Graphite);}
  Line(40,Y+RowHeight,360,Y+RowHeight,Muted,1);AddHitBox({24*S,Y*S},{352*S,RowHeight*S},Tools[I],true);
		Y+=RowHeight+12;
	}

}

void AFoundrylineBuildHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);PressAmounts.FindOrAdd(BoxName)=1.f;
 if(BoxName==TEXT("MaterialHandling")){if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))Player->ToggleMaterialHandling();return;}
 if(BoxName.ToString().StartsWith(TEXT("MaterialAction"))){if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))if(Player->LocalMaterialCell)Player->ServerMaterialCommand(Player->LocalMaterialCell,FCString::Atoi(*BoxName.ToString().RightChop(14)));return;}
 if(BoxName==TEXT("FreightDrive")) {if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))Player->ToggleFreightDrive();return;}
	auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(!PC) return;
 if(BoxName.ToString().StartsWith(TEXT("Ops"))){FString Action=BoxName.ToString().RightChop(3);int32 Suffix=INDEX_NONE;if(Action.FindChar(TCHAR(64),Suffix))Action.LeftInline(Suffix);PC->OperationsAction(FName(*Action));return;}
 if(BoxName.ToString().StartsWith(TEXT("Factory"))){PC->FactoryAction(FName(*BoxName.ToString().RightChop(7)));return;}
    AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){Site=*It;break;}
    if((Site && Site->IsPropertyBusy()) || (PC->IsConstructionPending() && BoxName!=TEXT("WalkView") && BoxName!=TEXT("RoofView"))) return;
 if(GetNetMode()==NM_Client && (BoxName==TEXT("SaveProperty") || BoxName==TEXT("LoadProperty"))) return;
    if(BoxName==TEXT("LogisticsOverlay")) {PC->ToggleLogisticsOverlay();return;}
 if(BoxName==TEXT("EditLogistics")) {PC->ToggleLogisticsEdit();return;}
 if(PC->SurfaceSizingAction(BoxName)) return;
 if(BoxName.ToString().StartsWith(TEXT("Net")) || BoxName.ToString().StartsWith(TEXT("Traffic")) || BoxName.ToString().StartsWith(TEXT("Trip")) || BoxName==TEXT("TruckLayer") || BoxName==TEXT("ParkingLayer") || BoxName==TEXT("ShowBothRoutes") || BoxName==TEXT("ShowArrival") || BoxName==TEXT("ShowDeparture") || BoxName==TEXT("RoutePaint") || BoxName==TEXT("RouteErase")) {PC->LogisticsAction(BoxName);return;}
 if(BoxName==TEXT("SaveProperty")) {bConfirmLoad=false;PC->ChooseTool(NAME_None);if(Site) Site->SaveProperty();return;}
    if(BoxName==TEXT("LoadProperty"))
    {
        if(Site && Site->HasUnsavedChanges() && !bConfirmLoad) {bConfirmLoad=true;Site->Status=TEXT("Load replaces unsaved changes. Click Confirm Load to continue.");return;}
        bConfirmLoad=false;PC->ChooseTool(NAME_None);if(Site) Site->LoadProperty();return;
    }
    bConfirmLoad=false;
    if(BoxName==TEXT("Redo")) {PC->RedoBuild();return;}
	if(BoxName==TEXT("RoofView")) {PC->ToggleRoofs();return;}
 if(BoxName==TEXT("WalkView")) {PC->ToggleWalk();return;}
 if(BoxName.ToString().StartsWith(TEXT("Paint_")))
 {TArray<FString> Parts;BoxName.ToString().ParseIntoArray(Parts,TEXT("_"));if(Parts.Num()==3) {const int32 C=FCString::Atoi(*Parts[1]),I=FCString::Atoi(*Parts[2]);if(I>=0 && I<8) for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->PaintSelected(C,Brand(PaintHex[I]));break;}}}
 else if(BoxName.ToString().StartsWith(TEXT("SiteCategory"))) {SiteCategory=FName(*BoxName.ToString().RightChop(12));PC->bExactSurface=false;PC->ChooseTool(NAME_None);}
 else if(BoxName.ToString().StartsWith(TEXT("Category"))) {BuildCategory=FName(*BoxName.ToString().RightChop(8));PC->ChooseTool(NAME_None);}
	else if(BoxName==TEXT("BuildTab") || BoxName==TEXT("EditTab") || BoxName==TEXT("OfficeTab") || BoxName==TEXT("PaintTab")) {bPaintTab=BoxName==TEXT("PaintTab");bEditTab=BoxName==TEXT("EditTab");bOfficeTab=BoxName==TEXT("OfficeTab");
  const FGuid Selected=Site?Site->SelectedObject:FGuid();PC->ChooseTool(NAME_None);
  if(Site) Site->SelectedObject=Selected;
  if(bEditTab) PC->ChooseTool(TEXT("Select"));
 }
	else if(BoxName==TEXT("Undo")) PC->UndoBuild();else PC->ChooseTool(BoxName==TEXT("Cancel")?NAME_None:BoxName);
}
void AFoundrylineBuildHUD::NotifyHitBoxBeginCursorOver(FName BoxName) {Super::NotifyHitBoxBeginCursorOver(BoxName);HoveredBox=BoxName;}
void AFoundrylineBuildHUD::NotifyHitBoxEndCursorOver(FName BoxName) {Super::NotifyHitBoxEndCursorOver(BoxName);if(HoveredBox==BoxName) HoveredBox=NAME_None;}
