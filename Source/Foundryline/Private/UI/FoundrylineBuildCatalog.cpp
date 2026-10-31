#include "FoundrylineBuildCatalog.h"
#include "FoundrylineUISurface.h"
#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Audio/FoundrylineUIAudio.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace {
struct FEntry {FString Group,Name,Action,Description,Spec;int Mode=0,Icon=0;FString PreviewName;int Ready=0,Owned=0;};
struct FHit {FBox2D Bounds;FName Action;};
struct FState {bool Open=false,SearchFocus=false;FString Group=TEXT("Expansion"),Search,Active,PreviewAction;int Page=0,Levels=1,Hover=-1;FBox2D Panel;TArray<FHit> Hits;TArray<FEntry> Items;TMap<FName,float> Hovering,Pressing;};
TMap<TWeakObjectPtr<AFoundrylineBuildHUD>,FState> States;
FState& State(AFoundrylineBuildHUD* H){for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();return States.FindOrAdd(H);}
AFoundrylineBuildSite* Site(const AFoundrylinePlayerController* PC){for(TActorIterator<AFoundrylineBuildSite> It(PC->GetWorld());It;++It)return *It;return nullptr;}
bool Available(const AFoundrylinePlayerController* P){return P&&!P->IsWalking()&&!P->OperationsScreen&&!P->FactoryMachine&&!P->LocalFreightRig&&!P->LocalMaterialCell&&(P->bPropertyViewAccess||!P->FactoryPlant());}
bool Mouse(const AFoundrylinePlayerController* P,FVector2D& Out){float X,Y;if(!P->GetMousePosition(X,Y))return false;int W,H;P->GetViewportSize(W,H);if(W<=0||H<=0)return false;FIntRect R(0,0,W,H);if(auto* L=P->GetLocalPlayer();L&&L->ViewportClient&&L->ViewportClient->Viewport){FSceneViewProjectionData D;if(L->GetProjectionData(L->ViewportClient->Viewport,D))R=D.GetConstrainedViewRect();}Out={(X-R.Min.X)*P->BuildHUDCanvasSize.X/R.Width(),(Y-R.Min.Y)*P->BuildHUDCanvasSize.Y/R.Height()};return true;}
TArray<FEntry> Entries(AFoundrylinePlayerController* P){TArray<FEntry> E={
 {TEXT("Production"),TEXT("Fabrication workstation"),TEXT("0"),TEXT("Welding bench, machine and compartmented input bin. Assign a welder after placing."),TEXT("19 ft 6 in x 13 ft / Pauses this business"),1,1},
 {TEXT("Production"),TEXT("Forklift area"),TEXT("ToolForklift"),TEXT("Drag a rectangular forklift driving area. Join areas to connect the shop; leave room for two forklifts to pass."),TEXT("Click and drag a rectangle / two-truck clear width"),4,3},
 {TEXT("Storage"),TEXT("Parts rack"),TEXT("1"),TEXT("Standard pallet racking for parts. The front loading aisle may join another aisle; rack bodies cannot overlap."),TEXT("Choose 1, 2 or 3 levels below"),1,2},
 {TEXT("Storage"),TEXT("Finished goods rack"),TEXT("2"),TEXT("Wide-bay pallet racking for products waiting to ship."),TEXT("Choose 1, 2 or 3 levels below"),1,2},
 {TEXT("Office"),TEXT("Turning staircase"),TEXT("OfficeStairs"),TEXT("Place a U-turn staircase inside an open office. Adds a second floor, stair opening, landings and handrails."),TEXT("About 9 ft 10 in x 16 ft 10 in / R rotates"),0,1},
 {TEXT("Office"),TEXT("Straight staircase"),TEXT("OfficeStairsStraight"),TEXT("A single straight flight with handrails and an upper floor opening. Adds a second office floor."),TEXT("About 4 ft 6 in x 25 ft 1 in / R rotates"),0,1},
 {TEXT("Office"),TEXT("Edit ground floor"),TEXT("Floor0"),TEXT("Show and furnish the ground office floor."),TEXT("Planning view only"),8,0},
 {TEXT("Office"),TEXT("Edit second floor"),TEXT("Floor1"),TEXT("Show and furnish the upper office floor after building stairs."),TEXT("Planning view only"),8,0},
 {TEXT("Office"),TEXT("Desk & computer"),TEXT("7"),TEXT("Office workstation with a triple-monitor computer and chair. Purchase additional desks for your office layout."),TEXT("$500 / R rotates / click to buy and place"),1,1},
 {TEXT("Office"),TEXT("Office addition"),TEXT("OfficeArea"),TEXT("Draw an office beside a shop or as a separate building. Joined offices reuse the shared wall."),TEXT("Click and drag / 1:1 scale"),0,0},
 {TEXT("Office"),TEXT("Interior room"),TEXT("OfficeRoom"),TEXT("Divide an existing office into rooms. Outer walls are reused."),TEXT("Click and drag inside an office"),0,0},
 {TEXT("Office"),TEXT("Office door"),TEXT("RoomDoor"),TEXT("Install a personnel door in an office exterior or interior room wall."),TEXT("Click an office wall"),0,4},
 {TEXT("Office"),TEXT("Office window"),TEXT("RoomWindow"),TEXT("Manually place an office window. Offices never receive automatic wall windows."),TEXT("Click an office wall"),0,4},
 {TEXT("Office"),TEXT("Interior glass partition"),TEXT("OfficeInteriorGlass"),TEXT("Floor-to-ceiling interior glass. Replaces the partition and clears covered wall art. Doors stay separate."),TEXT("Click an interior office partition"),0,4},
 {TEXT("Office"),TEXT("Full-height glass wall"),TEXT("OfficeGlass"),TEXT("Glaze an office wall span with framed panels. Existing doors remain clear."),TEXT("Click an exterior office wall"),0,4},
 {TEXT("Expansion"),TEXT("Standard shop"),TEXT("Building"),TEXT("Draw a shop footprint. Drag out from an existing shop to expand it and open the shared wall."),TEXT("About 20 ft wall height / click and drag"),0,0},
 {TEXT("Expansion"),TEXT("Tall shop"),TEXT("Shop2"),TEXT("Taller shop shell with high windows and columns, suitable for larger equipment."),TEXT("About 39 ft wall height / click and drag"),0,0},
 {TEXT("Expansion"),TEXT("Personnel door"),TEXT("Personnel"),TEXT("Install a pedestrian entrance in an exterior building wall."),TEXT("Click a wall / E opens the door"),0,4},
 {TEXT("Expansion"),TEXT("Garage door"),TEXT("Garage"),TEXT("A grade-level opening for forklifts and smaller vehicles."),TEXT("13 ft 1 in wide / click a wall"),0,4},
 {TEXT("Expansion"),TEXT("Semi entrance"),TEXT("TruckDoor"),TEXT("A larger ground-level opening for trucks and tall products."),TEXT("16 ft 5 in clear width and height"),0,4},
 {TEXT("Expansion"),TEXT("Loading bay"),TEXT("LoadingBay"),TEXT("Complete small-truck bay: door, bollards and useful paint. Select the placed bay to upgrade it."),TEXT("Small truck first / upgrades available"),0,4},
 {TEXT("Expansion"),TEXT("Column spacing"),TEXT("Columns"),TEXT("Adjust the spacing of structural shop columns without changing their physical size."),TEXT("Click a shop to cycle spacing"),0,0},
 {TEXT("Decorations"),TEXT("Company sign"),TEXT("Sign"),TEXT("Freestanding manufacturing sign. After placing, select it to edit the business name, colors and facing."),TEXT("12 ft panel / click to place / R rotates"),0,5},
 {TEXT("Site"),TEXT("Pavement rectangle"),TEXT("Rect"),TEXT("Drag a paved area for yards, driveways or shipping. Paint markings afterward."),TEXT("Click and drag / level with the shop floor"),2,3},
 {TEXT("Site"),TEXT("Custom pavement"),TEXT("Polygon"),TEXT("Click corners to shape your paved area. Finish with Enter."),TEXT("Click corners / Enter finishes"),2,3},
 {TEXT("Site"),TEXT("Two-way driveway"),TEXT("RoadStandard"),TEXT("Connect pavement using a standard two-way road with an adjustable curve."),TEXT("Click start and end / R switches curve"),2,3},
 {TEXT("Site"),TEXT("One-way driveway"),TEXT("RoadOneWay"),TEXT("A narrower directional driveway for an entrance or exit."),TEXT("One lane / click start and end"),2,3},
 {TEXT("Site"),TEXT("Truck driveway"),TEXT("RoadTruck"),TEXT("A wider road preset for heavy vehicles. Check turns using the logistics overlay."),TEXT("Truck preset / curved connections"),2,3},
 {TEXT("Site"),TEXT("Parking lot"),TEXT("Parking"),TEXT("Draw a parking area with standard spaces and aisle markings."),TEXT("Click and drag / R rotates the layout"),0,3},
 {TEXT("Site"),TEXT("Sidewalk"),TEXT("Walkway"),TEXT("Draw a concrete pedestrian connection between parking and entrances."),TEXT("Click and drag"),0,3},
 {TEXT("Site"),TEXT("Parking markings"),TEXT("Mark0"),TEXT("Add parking-space markings to existing pavement."),TEXT("Place on pavement"),2,3},
 {TEXT("Site"),TEXT("White line"),TEXT("Mark1"),TEXT("Paint a white boundary or guide on paved areas."),TEXT("Drag on pavement"),2,3},
 {TEXT("Site"),TEXT("Yellow line"),TEXT("Mark2"),TEXT("Paint a yellow boundary or separation line."),TEXT("Drag on pavement"),2,3},
 {TEXT("Site"),TEXT("Direction arrow"),TEXT("Mark4"),TEXT("Mark the intended direction of traffic on pavement."),TEXT("Place on pavement / R rotates"),2,3},
 {TEXT("Production"),TEXT("Plasma table"),TEXT("4"),TEXT("Place a plasma table with sheet supply and a four-pallet output rack. Each machine has its own operator and program."),TEXT("Click to place / R rotates"),1,1},
 {TEXT("Production"),TEXT("Press brake"),TEXT("5"),TEXT("Place another press brake with its own input and output storage."),TEXT("Click to place / R rotates"),1,1},
 {TEXT("Production"),TEXT("Sheet roller"),TEXT("6"),TEXT("Place a sheet roller. Additional roller recipes will be introduced with larger products."),TEXT("Click to place / R rotates"),1,1},
 {TEXT("Logistics"),TEXT("Forklift"),TEXT("3"),TEXT("Purchase and place a forklift, then assign its driver and route."),TEXT("Shop route created automatically"),1,1},
 {TEXT("Logistics"),TEXT("Logistics overlay"),TEXT("Overlay"),TEXT("Inspect vehicle routes, parking and driving clearance."),TEXT("Local overlay / shared routes"),3,3},
 {TEXT("Logistics"),TEXT("Create truck route"),TEXT("NetNew"),TEXT("Connect route nodes for delivery vehicles. Turning clearance still matters."),TEXT("Click route nodes"),3,3},
 {TEXT("Logistics"),TEXT("Edit truck routes"),TEXT("NetPanel"),TEXT("Select and connect existing route nodes."),TEXT("Shared business logistics"),3,3}
 };if(auto* Plant=P->FactoryPlant())for(auto& Entry:E)if(Entry.Mode==1){
  const int Item=FCString::Atoi(*Entry.Action);if(Item<3||Item>6)continue;
  Entry.Mode=6;
  if(Item==3){Entry.Ready=Plant->Snapshot.UnplacedForklifts;Entry.Owned=Plant->Snapshot.Drivers.Num()+Entry.Ready;}
  else for(const auto& Machine:Plant->Snapshot.Machines)if(Machine.Kind()==Item-3){++Entry.Owned;if(Machine.bStored)++Entry.Ready;}
  Entry.Spec=FString::Printf(TEXT("%d owned / %d unplaced. Buy equipment at the office computer."),Entry.Owned,Entry.Ready);
  if(!Entry.Ready)Entry.Description=TEXT("Purchase this equipment from Production > Buy equipment at your office computer. Then return here to place your owned item.");
 }
 return E;}
// Catalogue presentation follows the original logo, consistently across categories.
FLinearColor BrandCharcoal(){return FLinearColor(FColor::FromHex(TEXT("1F2327")));}
FLinearColor BrandOrange(){return FLinearColor(FColor::FromHex(TEXT("F07A2F")));}
FLinearColor CategoryColor(const FString&){return BrandOrange();}
UTexture2D* Preview(const FEntry& E,int Levels){FString Name=E.PreviewName.IsEmpty()?E.Action:E.PreviewName;if(E.Action==TEXT("OfficeInteriorGlass"))Name=TEXT("OfficeGlass");if(E.Action.StartsWith(TEXT("Floor")))Name=TEXT("OfficeArea");if(E.Mode==1||E.Mode==6)Name=E.Action==TEXT("7")?TEXT("OfficeDesk"):E.Action==TEXT("0")?TEXT("Workstation"):E.Action==TEXT("3")?TEXT("Forklift"):E.Action==TEXT("4")?TEXT("Plasma"):E.Action==TEXT("5")?TEXT("Brake"):E.Action==TEXT("6")?TEXT("Roller"):(E.Action==TEXT("1")?TEXT("Parts"):TEXT("Goods"))+FString::FromInt(Levels);static TMap<FString,TStrongObjectPtr<UTexture2D>> Images;auto& T=Images.FindOrAdd(Name);if(!T.IsValid())T.Reset(LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Foundryline/UI/BuildCatalogue/T_Catalog_")+Name+TEXT(".T_Catalog_")+Name)));return T.Get();}
TArray<int> Filter(const FState& S){TArray<int> Out;for(int I=0;I<S.Items.Num();++I){const auto& E=S.Items[I];if(!S.Search.IsEmpty()?!(E.Name+TEXT(" ")+E.Group).Contains(S.Search):E.Group!=S.Group)continue;Out.Add(I);}return Out;}
void Activate(AFoundrylineBuildHUD* H,AFoundrylinePlayerController* P,const FEntry& E){auto& S=State(H);
 int StoredMachine=0;
 if(E.Mode==6){auto* Plant=P->FactoryPlant();if(!Plant)return;const int Item=FCString::Atoi(*E.Action);
  if(Item==3){if(Plant->Snapshot.UnplacedForklifts<1){S.PreviewAction=E.Action;return;}}
  else {const auto* Owned=Plant->Snapshot.Machines.FindByPredicate([&](const auto& M){return M.Kind()==Item-3&&M.bStored;});if(!Owned){S.PreviewAction=E.Action;return;}StoredMachine=Owned->Id;}
 }
 S.Open=false;S.SearchFocus=false;S.Active=E.Name;if(P->IsLogisticsOverlayVisible())P->ToggleLogisticsOverlay();P->ChooseTool(NAME_None);P->bFactoryMenu=false;P->FactoryTool=NAME_None;P->PlannerKind=P->PlannerId=0;
 if(E.Mode==6){if(auto* Plant=P->FactoryPlant();Plant&&Plant->Snapshot.Running)P->FactoryAction(TEXT("Run"));
  if(StoredMachine)P->PlannerAction(FName(*FString::Printf(TEXT("Stored%d"),StoredMachine)));
  else {P->bFactoryMenu=true;P->FactoryAction(TEXT("ShopItem3"));}
 }
 else if(E.Mode==8){P->SetOfficePlanningLevel(FCString::Atoi(*E.Action.Right(1)));}
 else if(E.Mode==1){P->bFactoryMenu=true;P->FactoryRackLevels=S.Levels;P->FactoryAction(FName(*(TEXT("ShopItem")+E.Action)));}
 else if(E.Mode==4){if(auto* Plant=P->FactoryPlant();Plant&&Plant->Snapshot.Running)P->FactoryAction(TEXT("Run"));P->bFactoryMenu=true;P->FactoryAction(FName(*E.Action));}
 else if(E.Mode==5)P->PlannerAction(FName(*E.Action));
 else if(E.Mode==2){P->PavePalettePage=E.Action.StartsWith(TEXT("Mark"))?1:0;if(E.Action.StartsWith(TEXT("Road"))){P->PavementAction(TEXT("Road"));P->PavementAction(E.Action==TEXT("RoadTruck")?TEXT("TruckPreset"):TEXT("StandardPreset"));if(E.Action==TEXT("RoadOneWay"))P->PavementAction(TEXT("OneWay"));}else P->PavementAction(FName(*E.Action));}
 else if(E.Mode==3){if(E.Action==TEXT("Overlay"))P->ToggleLogisticsOverlay();else P->LogisticsAction(FName(*E.Action));}
 else P->ChooseTool(FName(*E.Action));UFoundrylineUIAudio::Play(P,TEXT("Click"));}
}

bool FoundrylineBuildCatalogOpen(AFoundrylineBuildHUD* H){return H&&State(H).Open;}
bool FoundrylineBuildCatalogOverUI(AFoundrylineBuildHUD* H,const AFoundrylinePlayerController* P){if(!H||!Available(P))return false;const auto& S=State(H);if(S.Open)return true;FVector2D M;if(!Mouse(P,M))return false;for(const auto& Hit:S.Hits)if(Hit.Bounds.IsInsideOrOn(M))return true;
 if(P->PavePanelBounds.bIsValid&&P->PavePanelBounds.IsInsideOrOn(M))return true;const float Scale=FMath::Min(1.5f,P->BuildHUDCanvasSize.Y/1320.f);bool SelectedSign=false;if(auto* B=Site(P))if(const auto* O=B->GetSelectedObject())SelectedSign=O->DefinitionId==TEXT("ManufacturingSign");if((SelectedSign||P->PlannerKind)&&M.X>P->BuildHUDCanvasSize.X-400*Scale&&M.Y>280*Scale&&M.Y<850*Scale)return true;return false;}
bool HandleFoundrylineBuildCatalogAction(AFoundrylineBuildHUD* H,AFoundrylinePlayerController* P,FName Action){const FString A=Action.ToString();if(A==TEXT("Select")||A==TEXT("Move")||A==TEXT("Resize")||A==TEXT("Demolish")){State(H).Active=A==TEXT("Demolish")?TEXT("Remove / select an item"):A+TEXT(" / select an item");return false;}if(!A.StartsWith(TEXT("Catalog")))return false;if(!Available(P))return true;auto& S=State(H);S.Pressing.FindOrAdd(Action)=1;
 if(A==TEXT("CatalogFloor0")||A==TEXT("CatalogFloor1")){P->SetOfficePlanningLevel(A.EndsWith(TEXT("1"))?1:0);S.Active=P->OfficePlanningLevel?TEXT("SECOND OFFICE FLOOR"):TEXT("GROUND FLOOR");return true;}
 if(A==TEXT("CatalogOpen")){S.Open=!S.Open;S.SearchFocus=false;S.Page=0;S.Items=Entries(P);return true;}
 if(A==TEXT("CatalogClose")){S.Open=false;return true;}
 if(A==TEXT("CatalogSearch")){S.SearchFocus=true;return true;}
 if(A==TEXT("CatalogClear")){S.Search.Reset();S.SearchFocus=true;S.Page=0;return true;}
 if(A.StartsWith(TEXT("CatalogGroup"))){S.Group=A.RightChop(12);S.Search.Reset();S.SearchFocus=false;S.Page=0;S.Hover=-1;return true;}
 if(A.StartsWith(TEXT("CatalogItem"))){const int I=FCString::Atoi(*A.RightChop(11));S.Items=Entries(P);if(S.Items.IsValidIndex(I))Activate(H,P,S.Items[I]);return true;}
 if(A.StartsWith(TEXT("CatalogLevel"))){S.Levels=FMath::Clamp(FCString::Atoi(*A.RightChop(12)),1,3);return true;}
 if(A==TEXT("CatalogNext")){++S.Page;S.PreviewAction.Reset();return true;}if(A==TEXT("CatalogPrev")){S.Page=FMath::Max(0,S.Page-1);S.PreviewAction.Reset();return true;}
 if(A==TEXT("CatalogSave")){if(P->FactoryPlant())P->FactoryAction(TEXT("Save"));else if(auto* B=Site(P))B->SaveProperty();return true;}
 if(A==TEXT("CatalogOffice")){P->FactoryAction(TEXT("ReturnOffice"));return true;}
 if(A==TEXT("CatalogRun")){P->FactoryAction(TEXT("Run"));return true;}
 if(A.StartsWith(TEXT("CatalogRoute"))){P->LogisticsAction(FName(*A.RightChop(12)));return true;}
 return true;}

bool HandleFoundrylineBuildCatalogInput(AFoundrylineBuildHUD* H,AFoundrylinePlayerController* P,const FInputKeyEventArgs& K){if(!H||!Available(P))return false;auto& S=State(H);const bool Press=K.Event==IE_Pressed||K.Event==IE_Repeat;
 if(Press&&(K.Key==EKeys::Q||K.Key==EKeys::B||K.Key==EKeys::Tab||K.Key==EKeys::P)&&(!S.Open||!S.SearchFocus)){if(K.Key==EKeys::P)S.Group=TEXT("Production");HandleFoundrylineBuildCatalogAction(H,P,TEXT("CatalogOpen"));return true;}
 if(S.Open&&Press&&(K.Key==EKeys::Escape||K.Key==EKeys::RightMouseButton)){S.Open=false;return true;}
 if(K.Key==EKeys::LeftMouseButton){if(!Press)return S.Open;FVector2D M=FVector2D::ZeroVector;if(Mouse(P,M))for(int I=S.Hits.Num()-1;I>=0;--I)if(S.Hits[I].Bounds.IsInsideOrOn(M)){const auto A=S.Hits[I].Action;H->NotifyHitBoxClick(A);return true;}if(S.Open){if(!S.Panel.IsInsideOrOn(M))S.Open=false;return true;}return false;}
 if(!S.Open)return false;
 if(Press){if(K.Key==EKeys::MouseScrollDown){++S.Page;return true;}if(K.Key==EKeys::MouseScrollUp){S.Page=FMath::Max(0,S.Page-1);return true;}if(K.Key==EKeys::BackSpace){S.Search.LeftChopInline(1);S.SearchFocus=true;S.Page=0;return true;}if(K.Key==EKeys::Enter){const auto F=Filter(S);if(F.Num()==1)Activate(H,P,S.Items[F[0]]);return true;}FString C=K.Key.GetFName().ToString();if(K.Key==EKeys::SpaceBar)C=TEXT(" ");if(C.Len()==1&&(FChar::IsAlnum(C[0])||C==TEXT(" "))&&S.Search.Len()<48){S.Search+=C.ToLower();S.SearchFocus=true;S.Page=0;}}
 return K.Key!=EKeys::MouseX&&K.Key!=EKeys::MouseY;}

void DrawFoundrylineBuildCatalog(AFoundrylineBuildHUD* H,AFoundrylinePlayerController* P,UCanvas* Canvas){if(!H||!Canvas)return;auto& S=State(H);S.Hits.Reset();if(!Available(P)){S.Open=false;return;}S.Items=Entries(P);AHUD* Draw=H;const float W=Canvas->ClipX,Height=Canvas->ClipY,Scale=FMath::Min3((W-64)/1180.f,(Height-64)/760.f,2.f),Dt=FMath::Min(P->GetWorld()->GetDeltaSeconds(),.1f);const auto Paper=FLinearColor(FColor::FromHex(TEXT("F7F5F1"))),Ink=FLinearColor(FColor::FromHex(TEXT("1F2327"))),Grey=FLinearColor(FColor::FromHex(TEXT("4B5258"))),Orange=FLinearColor(FColor::FromHex(TEXT("F07A2F"))),Pale=FLinearColor(FColor::FromHex(TEXT("E1E2E1")));FVector2D Pointer;const bool MouseValid=Mouse(P,Pointer);
 auto Rect=[&](float X,float Y,float A,float B,FLinearColor C){Draw->DrawRect(C,X,Y,A,B);};
 auto Text=[&](const FString& T,float X,float Y,float Size,FLinearColor C){Draw->DrawText(T,C,X,Y,GEngine->GetMediumFont(),Size*Scale);};
 auto Hit=[&](FName A,float X,float Y,float Width,float High){S.Hits.Add({FBox2D({X,Y},{X+Width,Y+High}),A});};
 const float ToolScale=FMath::Clamp(FMath::Min(W/1920.f,Height/1080.f),.75f,1.15f);bool ToolbarPaint=true;
 auto Button=[&](FName A,const FString& Label,float X,float Y,float Width,float High,bool Selected=false){const float Unit=ToolbarPaint?ToolScale:Scale;const bool Over=MouseValid&&FBox2D({X,Y},{X+Width,Y+High}).IsInsideOrOn(Pointer);auto& V=S.Hovering.FindOrAdd(A);V=FMath::FInterpTo(V,Over?1.f:0.f,Dt,18);auto& Press=S.Pressing.FindOrAdd(A);Press=FMath::FInterpTo(Press,0.f,Dt,9);const float Offset=(-V+Press*1.1f)*Unit;
  const auto Base0=Selected?FMath::Lerp(Orange,FLinearColor(FColor::FromHex(TEXT("F9954F"))),V):ToolbarPaint?FMath::Lerp(Ink,Grey,V*.8f):FMath::Lerp(Paper,FLinearColor(FColor::FromHex(TEXT("FFF1E6"))),V);const auto Base=FMath::Lerp(Base0,Ink,Press*.12f);const auto Rim=Selected?FLinearColor(FColor::FromHex(TEXT("BE5920"))):FMath::Lerp(ToolbarPaint?Grey:Pale,Orange,V);
  FoundrylineUISurface::Panel(Canvas,X,Y+Offset,Width,High,Unit,FMath::Lerp(Base,FLinearColor::White,.025f),FMath::Lerp(Base,Ink,.035f),Rim,2+V,7);
  float TW=0,TH=0;Draw->GetTextSize(Label,TW,TH,GEngine->GetMediumFont(),1.05f*Unit);const float Fit=FMath::Min(1.05f*Unit,(Width-24*Unit)/FMath::Max(1.f,TW)*1.05f*Unit);const float FitHeight=TH*Fit/FMath::Max(.1f,1.05f*Unit);Draw->DrawText(Label,ToolbarPaint&&!Selected?Paper:Ink,X+12*Unit,Y+(High-FitHeight)*.5f+Offset,GEngine->GetMediumFont(),Fit);Hit(A,X,Y,Width,High);
 };
 auto Wrap=[&](const FString& T,float X,float Y,float Width,float Size,FLinearColor C){TArray<FString> Words;T.ParseIntoArray(Words,TEXT(" "));FString Line;for(const auto& Word:Words){const auto Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float TW,TH;Draw->GetTextSize(Next,TW,TH,GEngine->GetMediumFont(),Size*Scale);if(TW>Width&&!Line.IsEmpty()){Text(Line,X,Y,Size,C);Y+=23*Scale;Line=Word;}else Line=Next;}if(!Line.IsEmpty())Text(Line,X,Y,Size,C);return Y+25*Scale;};
 if(!S.Open){Button(TEXT("CatalogFloor0"),TEXT("GROUND"),W*.5f-150*Scale,12*Scale,140*Scale,42*Scale,P->OfficePlanningLevel==0);Button(TEXT("CatalogFloor1"),TEXT("OFFICE 2"),W*.5f,12*Scale,140*Scale,42*Scale,P->OfficePlanningLevel==1);}
 const float BarY=Height-74*ToolScale,CX=W*.5f;
 const bool Editing=P->bPlannerMoving||P->BuildTool==TEXT("Select")||P->BuildTool==TEXT("Move")||P->BuildTool==TEXT("Resize")||P->BuildTool==TEXT("Demolish");
 FoundrylineUISurface::Panel(Canvas,CX-367*ToolScale,BarY-6*ToolScale,734*ToolScale,66*ToolScale,ToolScale,Ink,Grey,Grey,3,10);
 Button(TEXT("CatalogOpen"),TEXT("BUILD  [Q]"),CX-358*ToolScale,BarY+2*ToolScale,168*ToolScale,50*ToolScale,!Editing);Button(TEXT("Select"),TEXT("SELECT"),CX-184*ToolScale,BarY+2*ToolScale,128*ToolScale,50*ToolScale,P->BuildTool==TEXT("Select"));Button(TEXT("Move"),TEXT("MOVE"),CX-50*ToolScale,BarY+2*ToolScale,120*ToolScale,50*ToolScale,P->BuildTool==TEXT("Move")||P->bPlannerMoving&&!P->bPlannerResizing);Button(TEXT("Resize"),TEXT("RESIZE"),CX+76*ToolScale,BarY+2*ToolScale,128*ToolScale,50*ToolScale,P->BuildTool==TEXT("Resize")||P->bPlannerMoving&&P->bPlannerResizing);Button(TEXT("Demolish"),TEXT("REMOVE"),CX+210*ToolScale,BarY+2*ToolScale,146*ToolScale,50*ToolScale,P->BuildTool==TEXT("Demolish"));
 Button(TEXT("CatalogOffice"),TEXT("OFFICE  [F]"),W-374*ToolScale,20*ToolScale,166*ToolScale,46*ToolScale);Button(TEXT("RoofView"),P->AreRoofsShown()?TEXT("ROOFS ON"):TEXT("ROOFS OFF"),W-198*ToolScale,20*ToolScale,178*ToolScale,46*ToolScale,P->AreRoofsShown());
 if(auto* Plant=P->FactoryPlant()){FoundrylineUISurface::Panel(Canvas,20*ToolScale,20*ToolScale,305*ToolScale,46*ToolScale,ToolScale,Paper,Paper,Pale,2,7);Draw->DrawText(FFoundrylineCommerceCatalog::Money(Plant->Snapshot.Commerce.CashCents),Ink,34*ToolScale,33*ToolScale,GEngine->GetMediumFont(),1.2f*ToolScale);Button(TEXT("CatalogRun"),Plant->Snapshot.Running?TEXT("PAUSE"):TEXT("RUN"),188*ToolScale,22*ToolScale,135*ToolScale,42*ToolScale,Plant->Snapshot.Running);}
 Button(TEXT("CatalogSave"),TEXT("SAVE"),20*ToolScale,Height-66*ToolScale,110*ToolScale,46*ToolScale);if(P->IsLogisticsOverlayVisible()){const TCHAR* Keys[]={TEXT("NetDraw"),TEXT("NetSelect"),TEXT("NetMove"),TEXT("NetCheck")};const TCHAR* Labels[]={TEXT("DRAW ROUTE"),TEXT("SELECT NODE"),TEXT("MOVE NODE"),TEXT("TEST ROUTE")};FoundrylineUISurface::Panel(Canvas,14*ToolScale,77*ToolScale,642*ToolScale,55*ToolScale,ToolScale,Ink,Grey,Grey,2,9);for(int I=0;I<4;++I)Button(FName(*(FString(TEXT("CatalogRoute"))+Keys[I])),Labels[I],(20+I*158)*ToolScale,83*ToolScale,150*ToolScale,43*ToolScale,P->BuildTool==FName(Keys[I]));}
 if(!S.Open){FString Name=S.Active;if(Name.IsEmpty())Name=TEXT("Choose a build item with Q");const auto* B=Site(P);FString Hint;if(!P->FactoryTool.IsNone())Hint=P->FactoryPlacementHint;else if(B)Hint=B->GetConstructionFeedbackText();if(P->IsLogisticsOverlayVisible())Hint=P->NetworkStatus;if(Hint.IsEmpty()&&B)Hint=B->Status;if(Hint.Len()>105)Hint=Hint.Left(102)+TEXT("...");FoundrylineUISurface::Panel(Canvas,CX-367*ToolScale,BarY-89*ToolScale,734*ToolScale,74*ToolScale,ToolScale,Paper,Paper,Pale,2,9);Draw->DrawText(Name,Ink,CX-349*ToolScale,BarY-75*ToolScale,GEngine->GetMediumFont(),1.16f*ToolScale);float TW=0,TH=0;Draw->GetTextSize(Hint,TW,TH,GEngine->GetMediumFont(),.9f*ToolScale);const float Fit=FMath::Min(.9f*ToolScale,700*ToolScale/FMath::Max(1.f,TW)*.9f*ToolScale);Draw->DrawText(Hint,(!P->FactoryTool.IsNone()&&!P->bFactoryPresetValid||B&&B->IsConstructionFeedbackError())?FLinearColor(.75f,.16f,.08f):Grey,CX-349*ToolScale,BarY-45*ToolScale,GEngine->GetMediumFont(),Fit);return;}
 ToolbarPaint=false;
 Rect(0,0,W,Height,FLinearColor(0,0,0,.56));S.Hits.Reset();const float X=(W-1180*Scale)*.5f,Y=(Height-760*Scale)*.5f;S.Panel=FBox2D({X,Y},{X+1180*Scale,Y+760*Scale});Rect(X+7*Scale,Y+9*Scale,1180*Scale,760*Scale,FLinearColor(0,0,0,.28));Rect(X,Y,1180*Scale,760*Scale,Paper);Rect(X,Y,1180*Scale,88*Scale,BrandCharcoal());Rect(X,Y,1180*Scale,5*Scale,Orange);Text(TEXT("BUILD CATALOGUE"),X+28*Scale,Y+25*Scale,1.65,FLinearColor::White);Text(TEXT("Choose an item, then place its hologram."),X+28*Scale,Y+62*Scale,1.02,FLinearColor(1,.88,.76));Button(TEXT("CatalogClose"),TEXT("CLOSE  [ESC]"),X+992*Scale,Y+21*Scale,162*Scale,45*Scale);
 Rect(X+24*Scale,Y+100*Scale,1132*Scale,47*Scale,Pale);Text(S.Search.IsEmpty()?TEXT("Search buildables..."):S.Search+(S.SearchFocus?TEXT("|"):TEXT("")),X+40*Scale,Y+114*Scale,1.12,Grey);Hit(TEXT("CatalogSearch"),X+24*Scale,Y+100*Scale,1132*Scale,47*Scale);Rect(X+24*Scale,Y+144*Scale,1132*Scale,3*Scale,S.SearchFocus?Orange:Pale);if(!S.Search.IsEmpty())Button(TEXT("CatalogClear"),TEXT("CLEAR"),X+1050*Scale,Y+103*Scale,100*Scale,40*Scale);
 // Persistent dark navigation gives the cards a quieter, clearer workspace.
 Rect(X+12*Scale,Y+160*Scale,199*Scale,560*Scale,Ink);
 const TCHAR* Groups[]={TEXT("Production"),TEXT("Storage"),TEXT("Office"),TEXT("Expansion"),TEXT("Site"),TEXT("Logistics"),TEXT("Decorations")};
 for(int I=0;I<7;++I){const float GY=Y+(172+I*59)*Scale;const FName Key(*(FString(TEXT("CatalogGroup"))+Groups[I]));const bool Selected=S.Search.IsEmpty()&&S.Group==Groups[I];const bool Over=MouseValid&&FBox2D({X+24*Scale,GY},{X+199*Scale,GY+49*Scale}).IsInsideOrOn(Pointer);auto& V=S.Hovering.FindOrAdd(Key);V=FMath::FInterpTo(V,Over?1.f:0.f,Dt,16);const auto NavBase=Selected?Orange:FMath::Lerp(Ink,Grey,V*.75f);FoundrylineUISurface::Panel(Canvas,X+24*Scale,GY,175*Scale,49*Scale,Scale,FMath::Lerp(NavBase,FLinearColor::White,.035f),NavBase,Selected?FLinearColor(FColor::FromHex(TEXT("BE5920"))):FMath::Lerp(Ink,Orange,V*.5f),Selected?2.f:V,6);Text(Groups[I],X+(36+V*3)*Scale,GY+13*Scale,1.0,Selected?Ink:Paper);int Count=0;for(const auto& E:S.Items)if(E.Group==Groups[I])++Count;Text(FString::FromInt(Count),X+178*Scale,GY+16*Scale,.74,Selected?Ink:FLinearColor(.65,.67,.68));Hit(Key,X+24*Scale,GY,175*Scale,49*Scale);}
 Rect(X+28*Scale,Y+600*Scale,167*Scale,1*Scale,Grey);Text(TEXT("OFFICE PLANNING"),X+29*Scale,Y+620*Scale,.78,Orange);Wrap(TEXT("Your company. Your property."),X+29*Scale,Y+648*Scale,158*Scale,.82,Paper);

 const int PerPage=6;const auto Filtered=Filter(S);const int Pages=FMath::Max(1,FMath::DivideAndRoundUp(Filtered.Num(),PerPage));S.Page=FMath::Clamp(S.Page,0,Pages-1);S.Hover=-1;Text(S.Search.IsEmpty()?S.Group.ToUpper():TEXT("SEARCH RESULTS"),X+225*Scale,Y+164*Scale,1.2,Ink);Text(FString::Printf(TEXT("%d AVAILABLE"),Filtered.Num()),X+690*Scale,Y+168*Scale,.8,Grey);
 auto Icon=[&](int Kind,float A,float B,FLinearColor C){const float U=Scale;auto L=[&](float x,float y,float a,float b){Draw->DrawLine(A+x*U,B+y*U,A+a*U,B+b*U,C,2*U);};if(Kind==0||Kind==4){L(0,38,0,10);L(0,10,20,0);L(20,0,40,10);L(40,10,40,38);L(0,38,40,38);if(Kind==4){L(12,38,12,15);L(12,15,30,15);L(30,15,30,38);}}else if(Kind==2){for(int i=0;i<3;++i)L(0,5+i*15,42,5+i*15);L(2,0,2,40);L(40,0,40,40);}else if(Kind==5){L(0,0,44,0);L(0,0,0,26);L(44,0,44,26);L(0,26,44,26);L(7,26,7,41);L(37,26,37,41);}else if(Kind==3){L(3,0,3,40);L(39,0,39,40);for(int i=0;i<3;++i)L(21,i*15,21,i*15+8);}else{L(0,8,44,8);L(0,8,0,20);L(44,8,44,20);L(0,20,44,20);L(7,20,7,40);L(37,20,37,40);L(25,0,25,8);}};
 auto PriceLabel=[&](const FEntry& E){return E.Mode==6?(E.Ready?FString::Printf(TEXT("%d UNPLACED"),E.Ready):FString(TEXT("BUY AT OFFICE"))):E.Mode==5?FString(TEXT("OWNED")):E.Mode==1?FFoundrylineCommerceCatalog::Money(FFoundrylinePlanningEquipment::Price(FCString::Atoi(*E.Action),S.Levels)):FString(TEXT("PLACE & BUILD"));};
 auto Limited=[&](const FString& Value,float A,float B,float Width,float Size,FLinearColor Colour,int Lines){
  TArray<FString> Words;Value.ParseIntoArray(Words,TEXT(" "));FString Line;int Row=0;
  for(int Word=0;Word<Words.Num();++Word){const FString Next=Line.IsEmpty()?Words[Word]:Line+TEXT(" ")+Words[Word];float TW=0,TH=0;Draw->GetTextSize(Next,TW,TH,GEngine->GetMediumFont(),Size*Scale);
   if(TW>Width-(Row+1>=Lines?18*Scale:0)&&!Line.IsEmpty()){if(Row+1>=Lines){Text(Line+TEXT("..."),A,B+Row*23*Scale,Size,Colour);return;}Text(Line,A,B+Row*23*Scale,Size,Colour);++Row;Line=Words[Word];}else Line=Next;
  }if(!Line.IsEmpty())Text(Line,A,B+Row*23*Scale,Size,Colour);
 };
 // Six roomy cards keep real equipment silhouettes legible, rather than shrinking
 // a model into a tiny strip above a crowded title and price.
 for(int N=0;N<PerPage&&S.Page*PerPage+N<Filtered.Num();++N){
  const int I=Filtered[S.Page*PerPage+N];const auto& E=S.Items[I];const float A=X+(225+(N%2)*310)*Scale,B=Y+(205+(N/2)*155)*Scale;
  const FName Key(*FString::Printf(TEXT("CatalogItem%d"),I));const bool Over=MouseValid&&FBox2D({A,B},{A+296*Scale,B+141*Scale}).IsInsideOrOn(Pointer);
  auto& V=S.Hovering.FindOrAdd(Key);V=FMath::FInterpTo(V,Over?1.f:0.f,Dt,15);const float Lift=V*2*Scale;
  FoundrylineUISurface::Panel(Canvas,A,B-Lift,296*Scale,141*Scale,Scale,FMath::Lerp(FLinearColor::White,FLinearColor(FColor::FromHex(TEXT("FFF6EE"))),V),Paper,FMath::Lerp(Pale,Orange,V),2+V,8);
  const auto PreviewBack=FMath::Lerp(FLinearColor(FColor::FromHex(TEXT("E9E8E4"))),FLinearColor(FColor::FromHex(TEXT("F8DFCA"))),V);
  FoundrylineUISurface::Round(Canvas,A+5*Scale,B+(5-2*V)*Scale,151*Scale,131*Scale,6*Scale,PreviewBack,PreviewBack);
  if(auto* Texture=Preview(E,S.Levels))Draw->DrawTexture(Texture,A-7*Scale,B+(14-4*V)*Scale,176*Scale,110*Scale,0,0,1,1);else Icon(E.Icon,A+55*Scale,B+43*Scale,CategoryColor(E.Group));
  Limited(E.Name,A+169*Scale,B+(19-2*V)*Scale,112*Scale,.96,Ink,3);
  Rect(A+169*Scale,B+101*Scale,108*Scale,1*Scale,Pale);Text(PriceLabel(E),A+169*Scale,B+114*Scale,.85,E.Mode==5?Grey:Orange);
  Hit(Key,A,B,296*Scale,141*Scale);if(Over){S.Hover=I;S.PreviewAction=E.Action;}
 }
 if(Filtered.IsEmpty())Wrap(TEXT("No matching items. Try a shorter name or choose another category."),X+240*Scale,Y+210*Scale,510*Scale,1.15,Grey);
 int Detail=S.Hover;if(Detail<0)for(int I:Filtered)if(S.Items[I].Action==S.PreviewAction){Detail=I;break;}if(Detail<0&&Filtered.Num())Detail=Filtered[FMath::Min(S.Page*PerPage,Filtered.Num()-1)];Rect(X+852*Scale,Y+172*Scale,304*Scale,498*Scale,Pale);
 if(S.Items.IsValidIndex(Detail)){
  const auto& E=S.Items[Detail];FoundrylineUISurface::Panel(Canvas,X+852*Scale,Y+172*Scale,304*Scale,498*Scale,Scale,Paper,Paper,Pale,2,8);
  FoundrylineUISurface::Round(Canvas,X+853*Scale,Y+173*Scale,302*Scale,204*Scale,7*Scale,FLinearColor(FColor::FromHex(TEXT("E3E4E1"))),FLinearColor(FColor::FromHex(TEXT("F0ECE6"))));
  Rect(X+853*Scale,Y+173*Scale,302*Scale,36*Scale,BrandCharcoal());Rect(X+853*Scale,Y+208*Scale,302*Scale,3*Scale,Orange);Text(E.Group.ToUpper(),X+872*Scale,Y+184*Scale,.88,FLinearColor::White);
  if(auto* Texture=Preview(E,S.Levels))Draw->DrawTexture(Texture,X+863*Scale,Y+207*Scale,282*Scale,176*Scale,0,0,1,1);
  Limited(E.Name,X+872*Scale,Y+391*Scale,265*Scale,1.22,Ink,2);Text(PriceLabel(E),X+872*Scale,Y+446*Scale,1.12,Orange);Rect(X+872*Scale,Y+475*Scale,264*Scale,1*Scale,Pale);
  Limited(E.Description,X+872*Scale,Y+488*Scale,264*Scale,.94,Grey,3);
  if(E.Mode==1&&(E.Action==TEXT("1")||E.Action==TEXT("2"))){Text(TEXT("RACK LEVELS"),X+872*Scale,Y+565*Scale,.81,Grey);for(int I=1;I<=3;++I)Button(FName(*FString::Printf(TEXT("CatalogLevel%d"),I)),FString::FromInt(I),X+(872+(I-1)*88)*Scale,Y+584*Scale,78*Scale,32*Scale,S.Levels==I);}
  else Limited(E.Spec,X+872*Scale,Y+566*Scale,264*Scale,.87,Ink,2);
  if(E.Mode==6&&!E.Ready){FoundrylineUISurface::Panel(Canvas,X+872*Scale,Y+628*Scale,264*Scale,32*Scale,Scale,Pale,Pale,Pale,1,6);Text(TEXT("PURCHASE AT OFFICE"),X+887*Scale,Y+635*Scale,.86,Grey);}
  else Button(FName(*FString::Printf(TEXT("CatalogItem%d"),Detail)),E.Mode==6?TEXT("PLACE OWNED ITEM   >"):E.Mode==1?TEXT("BUY & PLACE   >"):TEXT("PLACE ITEM   >"),X+872*Scale,Y+628*Scale,264*Scale,32*Scale,true);
 }

 if(Pages>1){Button(TEXT("CatalogPrev"),TEXT("PREVIOUS"),X+225*Scale,Y+676*Scale,148*Scale,43*Scale);Text(FString::Printf(TEXT("%d ITEMS  /  PAGE %d OF %d"),Filtered.Num(),S.Page+1,Pages),X+395*Scale,Y+690*Scale,.85,Grey);Button(TEXT("CatalogNext"),TEXT("NEXT"),X+684*Scale,Y+676*Scale,148*Scale,43*Scale);}else Text(FString::Printf(TEXT("%d ITEMS"),Filtered.Num()),X+225*Scale,Y+690*Scale,.85,Grey);Text(TEXT("Type to search   /   Enter selects one result   /   Esc closes"),X+225*Scale,Y+734*Scale,.87,Grey);
}

FString FoundrylineBuildCatalogReport(AFoundrylineBuildHUD* H,const AFoundrylinePlayerController* P){auto J=MakeShared<FJsonObject>();if(!H||!P)return TEXT("{}");const auto& S=State(H);J->SetBoolField(TEXT("open"),S.Open);J->SetStringField(TEXT("group"),S.Group);J->SetStringField(TEXT("search"),S.Search);J->SetNumberField(TEXT("levels"),S.Levels);J->SetStringField(TEXT("active"),S.Active);J->SetNumberField(TEXT("page"),S.Page);J->SetNumberField(TEXT("matches"),Filter(S).Num());J->SetNumberField(TEXT("canvas_width"),P->BuildHUDCanvasSize.X);J->SetNumberField(TEXT("canvas_height"),P->BuildHUDCanvasSize.Y);
 int W=0,High=0;P->GetViewportSize(W,High);FIntRect R(0,0,W,High);if(auto* L=P->GetLocalPlayer();L&&L->ViewportClient&&L->ViewportClient->Viewport){FSceneViewProjectionData D;if(L->GetProjectionData(L->ViewportClient->Viewport,D))R=D.GetConstrainedViewRect();}
 TArray<TSharedPtr<FJsonValue>> Hits;for(const auto& V:S.Hits){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("action"),V.Action.ToString());FString Name=V.Action.ToString();if(Name.StartsWith(TEXT("CatalogItem"))){const int I=FCString::Atoi(*Name.RightChop(11));if(S.Items.IsValidIndex(I))O->SetStringField(TEXT("item"),S.Items[I].Name);}const auto C=V.Bounds.GetCenter();O->SetNumberField(TEXT("x"),R.Min.X+C.X*R.Width()/FMath::Max(1.,P->BuildHUDCanvasSize.X));O->SetNumberField(TEXT("y"),R.Min.Y+C.Y*R.Height()/FMath::Max(1.,P->BuildHUDCanvasSize.Y));O->SetNumberField(TEXT("width"),V.Bounds.GetSize().X);O->SetNumberField(TEXT("height"),V.Bounds.GetSize().Y);Hits.Add(MakeShared<FJsonValueObject>(O));}J->SetArrayField(TEXT("buttons"),Hits);FString Out;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Out));return Out;}
