#include "UI/FoundrylineBuildHUD.h"
#include "FoundrylineBuildCatalog.h"
#include "FoundrylineOperationFeedback.h"
#include "FoundrylineUISurface.h"
#include "Audio/FoundrylineUIAudio.h"
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
 Super::DrawHUD();auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(!Canvas||!PC)return;
 PC->BuildHUDCanvasSize={Canvas->ClipX,Canvas->ClipY};
 if(!Logo)Logo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Foundryline/UI/Brand/T_FoundrylineLogo.T_FoundrylineLogo"));
 if(DrawOperations(PC)||DrawFactory(PC)){FoundrylineOperationFeedback::Draw(this,PC,Canvas);return;}DrawMaterialHandling(PC);if(PC->LocalMaterialCell){FoundrylineOperationFeedback::Draw(this,PC,Canvas);return;}DrawFreightControls(PC);if(PC->LocalFreightRig){FoundrylineOperationFeedback::Draw(this,PC,Canvas);return;}
 const float S=FMath::Clamp(FMath::Min(Canvas->ClipY/900.f,Canvas->ClipX/1440.f),.65f,1.4f);
 const auto Ink=Brand(TEXT("1F2327")),Graphite=Brand(TEXT("4B5258")),Paper=Brand(TEXT("F7F5F1")),Orange=Brand(TEXT("F07A2F")),Muted=Brand(TEXT("E8E7E2"));
 auto* Font=GEngine->GetMediumFont();
 auto Rect=[&](float X,float Y,float W,float H,FLinearColor C){DrawRect(C,X*S,Y*S,W*S,H*S);};
 auto Panel=[&](float X,float Y,float W,float H){FoundrylineUISurface::Panel(Canvas,X*S,Y*S,W*S,H*S,S,Paper,FMath::Lerp(Paper,Muted,.28f),Muted,2,8);};
 auto Line=[&](float X,float Y,float A,float B,FLinearColor C,float Width=2.f){DrawLine(X*S,Y*S,A*S,B*S,C,Width*S);};
 auto Text=[&](const FString& V,float X,float Y,float Size,FLinearColor C){DrawText(V,C,X*S,Y*S,Font,Size*S);};
 if(PC->IsWalking())
 {
  const float CX=Canvas->ClipX/S/2,CY=Canvas->ClipY/S/2,Bottom=Canvas->ClipY/S;
  // Quiet, centred reticle; a dark outline keeps it readable against bright steel.
  Line(CX-3,CY,CX+3,CY,Ink,4);Line(CX,CY-3,CX,CY+3,Ink,4);
  Line(CX-2,CY,CX+2,CY,Paper,2);Line(CX,CY-2,CX,CY+2,Paper,2);
  // Compact instrument bar: brand, money and workshop clock have distinct visual weight.
  Panel(24,24,660,94);Rect(32,24,644,3,Orange);
  if(Logo)DrawTexture(Logo,43*S,44*S,156*S,56.9f*S,0,0,1,1);
  Line(220,43,220,100,Muted,1);
  Text(TEXT("BUSINESS FUNDS"),240,40,.78f,Graphite);
  if(auto* Plant=PC->FactoryPlant()){
   const auto& State=Plant->Snapshot;
   Text(FFoundrylineCommerceCatalog::Money(State.Commerce.CashCents),240,62,1.65f,Ink);
   FoundrylineUISurface::Panel(Canvas,480*S,40*S,184*S,60*S,S,Ink,Ink,Graphite,1,6);
   Text(FFoundrylineCommerceCatalog::Date(State.Commerce.Clock),494,49,1.02f,Paper);
   Rect(495,81,6,6,State.Running?FLinearColor(.15f,.65f,.32f):Orange);
   Text(State.Running?TEXT("WORKSHOP RUNNING"):TEXT("WORKSHOP PAUSED"),509,77,.7f,Paper);
  }
  Panel(24,Bottom-65,610,41);Rect(25,Bottom-55,3,21,Orange);
  Text(TEXT("WASD"),42,Bottom-51,1.05f,Ink);Text(TEXT("Walk"),108,Bottom-51,1.05f,Graphite);
  Text(TEXT("MOUSE"),181,Bottom-51,1.05f,Ink);Text(TEXT("Look"),259,Bottom-51,1.05f,Graphite);
  Text(TEXT("E"),331,Bottom-51,1.05f,Ink);Text(TEXT("Use / inspect"),354,Bottom-51,1.05f,Graphite);
  Text(TEXT("OFFICE COMPUTER  /  Planning & expansion"),24,Bottom-91,.9f,Paper);
  if(auto* Door=PC->FocusedDoor())DrawFirstPersonPrompt(Door->IsOpen()?TEXT("Close door"):TEXT("Open door"),TEXT("PERSONNEL DOOR"));
  FoundrylineOperationFeedback::Draw(this,PC,Canvas);return;
 }
 if(!FoundrylineBuildCatalogOpen(this)){
  DrawSelectionOutlines(PC);DrawPlacementGuide(PC);DrawPavementTools(PC);DrawPlannerInspector(PC);
  if(PC->IsLogisticsOverlayVisible())DrawLogisticsMap(PC);
 }
 DrawFoundrylineBuildCatalog(this,PC,Canvas);FoundrylineOperationFeedback::Draw(this,PC,Canvas);
}

void AFoundrylineBuildHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);PressAmounts.FindOrAdd(BoxName)=1.f;
 const FString SoundKey=BoxName.ToString();UFoundrylineUIAudio::Play(GetOwningPlayerController(),SoundKey.Contains(TEXT("Tab"))||SoundKey.Contains(TEXT("Page"))||SoundKey.Contains(TEXT("Category"))?TEXT("Tab"):TEXT("Click"));
 if(BoxName==TEXT("MaterialHandling")){if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))Player->ToggleMaterialHandling();return;}
 if(BoxName.ToString().StartsWith(TEXT("MaterialAction"))){if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))if(Player->LocalMaterialCell)Player->ServerMaterialCommand(Player->LocalMaterialCell,FCString::Atoi(*BoxName.ToString().RightChop(14)));return;}
 if(BoxName==TEXT("FreightDrive")) {if(auto* Player=Cast<AFoundrylinePlayerController>(GetOwningPlayerController()))Player->ToggleFreightDrive();return;}
	auto* PC=Cast<AFoundrylinePlayerController>(GetOwningPlayerController());if(!PC) return;
 if(HandleFoundrylineBuildCatalogAction(this,PC,BoxName))return;
 if(BoxName.ToString().StartsWith(TEXT("Wheel"))){PlanningWheelAction(BoxName);return;}
 if(BoxName.ToString().StartsWith(TEXT("Pave"))){PC->PavementAction(FName(*BoxName.ToString().RightChop(4)));return;}
 if(BoxName!=TEXT("Sign")&&BoxName.ToString().StartsWith(TEXT("Sign"))){PC->SignAction(FName(*BoxName.ToString().RightChop(4)));return;}
 if(BoxName.ToString().StartsWith(TEXT("Planner"))){PC->PlannerAction(FName(*BoxName.ToString().RightChop(7)));return;}
 if(PC->PlannerKind&&(BoxName==TEXT("Move")||BoxName==TEXT("Demolish")||BoxName==TEXT("Cancel"))){PC->PlannerAction(BoxName==TEXT("Demolish")?FName(TEXT("Delete")):BoxName);return;}
 if(BoxName.ToString().StartsWith(TEXT("Ops"))){FString Action=BoxName.ToString().RightChop(3);int32 Suffix=INDEX_NONE;if(Action.FindChar(TCHAR(64),Suffix))Action.LeftInline(Suffix);PC->OperationsAction(FName(*Action));return;}
 if(BoxName.ToString().StartsWith(TEXT("Factory"))){PC->FactoryAction(FName(*BoxName.ToString().RightChop(7)));return;}
    AFoundrylineBuildSite* Site=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){Site=*It;break;}
    if((Site && Site->IsPropertyBusy()) || (PC->IsConstructionPending() && BoxName!=TEXT("WalkView") && BoxName!=TEXT("RoofView"))) return;
 if(GetNetMode()==NM_Client && (BoxName==TEXT("SaveProperty") || BoxName==TEXT("LoadProperty"))) return;
    if(BoxName==TEXT("LogisticsOverlay")) {PC->ToggleLogisticsOverlay();return;}
 if(BoxName==TEXT("EditLogistics")) {PC->ToggleLogisticsEdit();return;}
 if(BoxName==TEXT("Demolish")&&Site&&Site->GetSelectedObject()){Site->CommitEdit(TEXT("Demolish"),FIntVector::ZeroValue,FIntVector::ZeroValue);PC->ChooseTool(TEXT("Select"));return;}
 if(PC->SurfaceSizingAction(BoxName)) return;
 if(BoxName.ToString().StartsWith(TEXT("Net")) || BoxName.ToString().StartsWith(TEXT("Traffic")) || BoxName.ToString().StartsWith(TEXT("Trip")) || BoxName==TEXT("TruckLayer") || BoxName==TEXT("ParkingLayer") || BoxName==TEXT("ShowBothRoutes") || BoxName==TEXT("ShowArrival") || BoxName==TEXT("ShowDeparture") || BoxName==TEXT("RoutePaint") || BoxName==TEXT("RouteErase")) {PC->LogisticsAction(BoxName);return;}
 if(BoxName==TEXT("SaveProperty")) {bConfirmLoad=false;PC->ChooseTool(NAME_None);if(PC->FactoryPlant())PC->FactoryAction(TEXT("Save"));else if(Site)Site->SaveProperty();return;}
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
void AFoundrylineBuildHUD::NotifyHitBoxBeginCursorOver(FName BoxName) {Super::NotifyHitBoxBeginCursorOver(BoxName);HoveredBox=BoxName;UFoundrylineUIAudio::Play(GetOwningPlayerController(),TEXT("Hover"));}
void AFoundrylineBuildHUD::NotifyHitBoxEndCursorOver(FName BoxName) {Super::NotifyHitBoxEndCursorOver(BoxName);if(HoveredBox==BoxName) HoveredBox=NAME_None;}
