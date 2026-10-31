#include "UI/FoundrylineBuildHUD.h"
#include "FoundrylineUISurface.h"
#include "Economy/FoundrylinePayroll.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineERPViewModel.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "UObject/StrongObjectPtr.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

// Game-facing office menu, sharing the build catalogue palette and navigation hierarchy.
// Presentation only. Every business change still goes through the existing validated owning-controller commands;
// selection, tabs and help are per-player local state, so co-op players browse independently.
bool AFoundrylineBuildHUD::DrawBusinessDesktop(AFoundrylinePlayerController* PC)
{
 auto* Plant=PC->FactoryPlant();if(!Plant)return false;const bool EmployeeView=PC->OperationsScreen==4;if(EmployeeView){PC->OperationsPage=7;PC->OperationsWorkspaceTab=0;PC->OperationsEmployeeChoice=PC->EmployeeInspectId;}
 const auto& S=Plant->Snapshot;const auto& C=S.Commerce;
 const auto VM=FFoundrylineERPViewModel::Build(C,FFoundrylineERPViewModel::CompanyName(GetWorld(),Plant->BusinessCompanyId));
 const auto Guide=FFoundrylineERPViewModel::Guide(S);
 auto Hex=[](const TCHAR* V){return FLinearColor(FColor::FromHex(V));};
 const auto Ink=Hex(TEXT("1F2327")),Muted=Hex(TEXT("4B5258")),Faint=Hex(TEXT("8D99A3")),Line=Hex(TEXT("DCE1E5")),Paper=Hex(TEXT("FFFFFF")),Surface=Hex(TEXT("F7F5F1")),Rail=Hex(TEXT("F2F0EC")),
  Orange=Hex(TEXT("F07A2F")),SoftOrange=Hex(TEXT("FFF1E6")),Green=Hex(TEXT("2E7D57")),SoftGreen=Hex(TEXT("E8F4ED")),Blue=Hex(TEXT("F07A2F")),SoftBlue=Hex(TEXT("F3EEE8")),
  Red=Hex(TEXT("B23B36")),SoftRed=Hex(TEXT("FBEBEA")),Amber=Hex(TEXT("A86A12")),Dark=Hex(TEXT("1F2327")),Graphite=Hex(TEXT("4B5258"));
 const float Scale=FMath::Min((Canvas->ClipY-64)/1000.f,(Canvas->ClipX-64)/1740.f),SW=Canvas->ClipX/Scale,SH=Canvas->ClipY/Scale;
 const float W=1740,H=1000,X=(SW-W)/2,Y=(SH-H)/2,NavW=304,HeadH=88,FootH=32;
 const float CX=X+NavW+28,CY=Y+HeadH+22,CW=W-NavW-56,CB=Y+H-FootH-18;
 int Page=PC->OperationsPage;if(Page==1)Page=0;if(Page==8)Page=4;
 HelpTexts.Reset();
 // UE5.8 Canvas requires a UFont cache selector even with an explicit Slate typeface.
 if(!ERPFont){ERPFont=NewObject<UFont>(this,TEXT("ERP_RuntimeCanvasFont"));ERPFont->FontCacheType=EFontCacheType::Runtime;}
 const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
 auto FontOf=[&](int Size,bool Bold){return FCoreStyle::GetDefaultFontStyle(Bold?TEXT("Bold"):TEXT("Regular"),FMath::Max(8,FMath::RoundToInt(Size*Scale)));};
 auto Width=[&](const FString& T,int Size,bool Bold){return float(Measure->Measure(T,FontOf(Size,Bold)).X)/Scale;};
 auto Rect=[&](float A,float B,float D,float E,FLinearColor Col){DrawRect(Col,A*Scale,B*Scale,D*Scale,E*Scale);};
 auto SurfacePanel=[&](float A,float B,float D,float E,FLinearColor Top,FLinearColor Bottom,FLinearColor Edge,float Elevation=1.f,float Radius=6.f){FoundrylineUISurface::Panel(Canvas,A*Scale,B*Scale,D*Scale,E*Scale,Scale,Top,Bottom,Edge,Elevation,Radius);};
 auto Round=[&](float A,float B,float D,float E,FLinearColor Col,float Radius=6.f){FoundrylineUISurface::Round(Canvas,A*Scale,B*Scale,D*Scale,E*Scale,Radius*Scale,Col,Col);};
 auto Border=[&](float A,float B,float D,float E,FLinearColor Col){Rect(A,B,D,1,Col);Rect(A,B+E-1,D,1,Col);Rect(A,B,1,E,Col);Rect(A+D-1,B,1,E,Col);};
 auto Text=[&](FString T,float A,float B,int Size=15,bool Bold=false,FLinearColor Col=FLinearColor::White,float MaxW=0.f){
  const auto Font=FontOf(Size,Bold);
  if(MaxW>0&&Measure->Measure(T,Font).X>MaxW*Scale){int Lo=0,Hi=T.Len();while(Lo<Hi){const int Mid=(Lo+Hi+1)/2;if(Measure->Measure(T.Left(Mid)+TEXT("..."),Font).X<=MaxW*Scale)Lo=Mid;else Hi=Mid-1;}T=T.Left(Lo)+TEXT("...");}
  FCanvasTextItem Item(FVector2D(A*Scale,B*Scale),FText::FromString(T),Font,Col);Item.Font=ERPFont;Canvas->DrawItem(Item);
 };
 auto RightText=[&](const FString& T,float Right,float B,int Size,bool Bold,FLinearColor Col){Text(T,Right-Width(T,Size,Bold),B,Size,Bold,Col);};
 // Word-wrapped paragraph; returns the height used.
 auto Wrap=[&](const FString& T,float A,float B,float MaxW,int Size=15,bool Bold=false,FLinearColor Col=FLinearColor::White,int MaxLines=4,float LineH=0){
  if(LineH<=0)LineH=Size*1.45f;TArray<FString> Words;T.ParseIntoArray(Words,TEXT(" "));FString Current;int Lines=0;
  for(int I=0;I<Words.Num();++I){const FString Next=Current.IsEmpty()?Words[I]:Current+TEXT(" ")+Words[I];if(!Current.IsEmpty()&&Width(Next,Size,Bold)>MaxW){if(Lines==MaxLines-1){Current=Next;continue;}Text(Current,A,B+Lines++*LineH,Size,Bold,Col,MaxW);Current=Words[I];}else Current=Next;}
  if(!Current.IsEmpty())Text(Current,A,B+Lines++*LineH,Size,Bold,Col,MaxW);return Lines*LineH;
 };
 auto Cash=[](int64 Cents){FString Whole=FString::Printf(TEXT("%lld"),FMath::Abs(Cents)/100);for(int I=Whole.Len()-3;I>0;I-=3)Whole.InsertAt(I,TCHAR(','));return FString::Printf(TEXT("%s$%s.%02lld"),Cents<0?TEXT("-"):TEXT(""),*Whole,FMath::Abs(Cents)%100);};
 auto Lb=[](double V){return FString::Printf(TEXT("%.0f lb"),V);};
 // Hit area with optional hover help. Disabled controls still explain themselves but cannot be clicked.
 // Hit area with optional hover help. Disabled controls still explain themselves but cannot be clicked.
 // Press is the click flash (set by NotifyHitBoxClick); both are local presentation only.
 float Press=0;bool bClickableHover=false;
 auto Hit=[&](const FString& Key,float A,float B,float D,float E,bool Enabled=true,const FString& Help=FString()){
  const FName Id(*FString::Printf(TEXT("Ops%s@%d_%d"),*(Enabled?Key:TEXT("Help")+Key),FMath::RoundToInt(A),FMath::RoundToInt(B)));const float Dt=GetWorld()->GetDeltaSeconds();
  float& V=HoverAmounts.FindOrAdd(Id);V=FMath::FInterpTo(V,Enabled&&HoveredBox==Id?1.f:0.f,Dt,22);float& P=PressAmounts.FindOrAdd(Id);P=FMath::FInterpTo(P,0.f,Dt,7);Press=Enabled?P:0;
  if(Enabled&&HoveredBox==Id)bClickableHover=true;
  if(Enabled||!Help.IsEmpty())AddHitBox({A*Scale,B*Scale},{D*Scale,E*Scale},Id,true);if(!Help.IsEmpty())HelpTexts.Add(Id,Help);return V;
 };
 auto HelpArea=[&](const FString& Name,float A,float B,float D,float E,const FString& Help){Hit(TEXT("Help")+Name,A,B,D,E,false,Help);};
 // Hover: orange outline and tint on anything clickable; press: brief darker flash.
 auto Hover=[&](float A,float B,float D,float E,float V){if(V<.02f&&Press<.02f)return;const FLinearColor Edge=FLinearColor::LerpUsingHSV(Line,Blue,FMath::Max(V,Press));Rect(A,B,D,2,Edge);Rect(A,B+E-2,D,2,Edge);Rect(A,B,2,E,Edge);Rect(A+D-2,B,2,E,Edge);if(Press>.02f)Rect(A,B,D,E,FLinearColor(Blue.R,Blue.G,Blue.B,.16f*Press));};
 enum class EStyle:uint8 {Primary,Secondary,Ghost,Tab,Danger};
 auto Button=[&](const FString& Key,const FString& Label,float A,float B,float D,float E,EStyle Style=EStyle::Secondary,bool Enabled=true,const FString& Help=FString(),bool Selected=false){
  const float V=Hit(Key,A,B,D,E,Enabled,Help),Down=Press;const int Size=E>=48?17:E>=40?15:14;const float Offset=Enabled?Down*1.1f-V*1.1f:0;
  const bool Primary=Style==EStyle::Primary||(Style==EStyle::Tab&&Selected);
  FLinearColor Base=!Enabled?Hex(TEXT("E5E5E2")):Primary?FMath::Lerp(Orange,Hex(TEXT("F9954F")),V):Selected?SoftOrange:FMath::Lerp(Style==EStyle::Ghost?Surface:Paper,SoftOrange,V*.65f);
  if(Enabled)Base=FMath::Lerp(Base,Primary?Hex(TEXT("D96827")):Line,Down*.4f);
  const auto Edge=!Enabled?Line:Primary?Hex(TEXT("BE5920")):Selected?Orange:FMath::Lerp(Line,Orange,V);
  SurfacePanel(A,B+Offset,D,E,FMath::Lerp(Base,Paper,Primary?.08f:.015f),FMath::Lerp(Base,Ink,Primary?.06f:.025f),Edge,Enabled?FMath::Max(.4f,2+V-Down*1.7f):0,6);
  const auto Col=!Enabled?Faint:Style==EStyle::Danger?Red:Ink;
  Text(Label,A+(D-FMath::Min(D-24,Width(Label,Size,true)))/2,B+Offset+(E-Size*1.25f)/2,Size,true,Col,D-24);
 };
 auto Card=[&](float A,float B,float D,float E,const FString& Title=FString(),FLinearColor Accent=FLinearColor::Transparent){SurfacePanel(A,B,D,E,Paper,Hex(TEXT("FCFBF9")),Line,1,8);if(Accent.A>0)Rect(A+8,B,D-16,3,Accent);if(!Title.IsEmpty())Text(Title.ToUpper(),A+20,B+18,12,true,Muted,D-40);};
 auto Chip=[&](const FString& T,float A,float B,FLinearColor Col,FLinearColor Back){const float D=Width(T,12,true)+20;Round(A,B,D,24,Back,12);Text(T,A+10,B+4,12,true,Col);return D;};
 auto Bar=[&](float A,float B,float D,float Fraction,FLinearColor Col,float E=8){Round(A,B,D,E,Surface,E*.5f);const float Fill=D*FMath::Clamp(Fraction,0.f,1.f);if(Fill>1)Round(A,B,Fill,E,Col,E*.5f);};
 // Simple drawn pictograms, so no external icon assets are needed.
 auto Icon=[&](int Kind,float A,float B,float Sz,FLinearColor Col){auto R=[&](float U,float V,float D,float E){Rect(A+U*Sz,B+V*Sz,D*Sz,E*Sz,Col);};
  switch(Kind){case 0:R(.42f,.08f,.16f,.1f);R(.3f,.18f,.4f,.1f);R(.18f,.28f,.64f,.1f);R(.22f,.42f,.56f,.48f);break;
   case 1:R(.2f,.08f,.6f,.84f);Rect(A+.3f*Sz,B+.2f*Sz,.4f*Sz,.64f*Sz,Paper);R(.3f,.3f,.4f,.07f);R(.3f,.48f,.4f,.07f);R(.3f,.66f,.25f,.07f);break;
   case 2:R(.1f,.22f,.8f,.12f);R(.1f,.44f,.8f,.12f);R(.1f,.66f,.8f,.12f);break;
   case 3:R(.3f,.3f,.4f,.4f);R(.42f,.1f,.16f,.8f);R(.1f,.42f,.8f,.16f);Rect(A+.42f*Sz,B+.42f*Sz,.16f*Sz,.16f*Sz,Paper);break;
   case 4:R(.05f,.25f,.55f,.45f);R(.62f,.4f,.32f,.3f);R(.15f,.72f,.16f,.16f);R(.68f,.72f,.16f,.16f);break;
   case 5:R(.36f,.1f,.28f,.28f);R(.2f,.46f,.6f,.44f);break;
   default:R(.08f,.25f,.84f,.5f);Rect(A+.14f*Sz,B+.31f*Sz,.72f*Sz,.38f*Sz,Paper);R(.4f,.36f,.2f,.28f);break;}
 };
 auto Mark=[&](const FString& Name,float A,float B,float Size){
  const TCHAR* Names[]={TEXT("Hartwell Fabrication"),TEXT("Northfield Repairs"),TEXT("Red Cedar Equipment"),TEXT("Mason Agricultural"),TEXT("Alder Workshop"),TEXT("Union Conveyor Co."),TEXT("Foundry Steel Supply"),TEXT("Swift Van Couriers"),TEXT("Foundry Lane Haulage"),TEXT("County Freight"),TEXT("Interstate Pallet Lines"),TEXT("Ridgeline Truck Outfitters"),TEXT("Crossroads Fleet Service"),TEXT("Prairie Pickup Accessories")};
  const TCHAR* Keys[]={TEXT("Hartwell"),TEXT("Northfield"),TEXT("RedCedar"),TEXT("Mason"),TEXT("Alder"),TEXT("Union"),TEXT("FoundrySteel"),TEXT("Swift"),TEXT("FoundryLane"),TEXT("County"),TEXT("Interstate"),TEXT("Ridgeline"),TEXT("Crossroads"),TEXT("Prairie")};int K=INDEX_NONE;for(int I=0;I<14;++I)if(Name==Names[I]){K=I;break;}
  static TMap<FString,TStrongObjectPtr<UTexture2D>> Marks;
  if(K!=INDEX_NONE){auto& Image=Marks.FindOrAdd(Keys[K]);if(!Image.IsValid())Image.Reset(LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/Foundryline/UI/Companies/T_Company_"))+Keys[K]+TEXT(".T_Company_")+Keys[K])));if(Image.IsValid()){DrawTexture(Image.Get(),A*Scale,B*Scale,Size*Scale,Size*Scale,0,0,1,1);return;}}
  Round(A,B,Size,Size,Graphite,Size*.18f);Text(Name.IsEmpty()?TEXT("?"):Name.Left(1).ToUpper(),A+Size*.3f,B+Size*.19f,FMath::RoundToInt(Size*.5f),true,Paper);
 };
 auto Pager=[&](int Count,int Rows,float A,float B,float D){if(Count<=Rows)return;const int P=FMath::Clamp(PC->OperationsListPage,0,FMath::Max(0,(Count-1)/Rows));Text(FString::Printf(TEXT("Showing %d-%d of %d"),P*Rows+1,FMath::Min((P+1)*Rows,Count),Count),A,B+9,13,false,Muted);Button(TEXT("Previous"),TEXT("< Back"),A+D-210,B,100,36,EStyle::Secondary,P>0,TEXT("Show the previous page. The mouse wheel also scrolls."));Button(TEXT("Next"),TEXT("More >"),A+D-100,B,100,36,EStyle::Secondary,(P+1)*Rows<Count,TEXT("Show the next page. The mouse wheel also scrolls."));};
 auto First=[&](int Count,int Rows){return FMath::Clamp(PC->OperationsListPage,0,FMath::Max(0,(Count-1)/Rows))*Rows;};
 auto Tabs=[&](TArray<TTuple<FString,FString,bool,FString>> Items,float A,float B){float At=A;Rect(A,B+43,CW,1,Line);for(const auto& T:Items){const float D=Width(T.Get<1>(),15,true)+52;Button(T.Get<0>(),T.Get<1>(),At,B,D,44,EStyle::Tab,true,T.Get<3>(),T.Get<2>());At+=D+4;}};
 auto PageHeader=[&](const FString& Title,const FString& Sub){const FString Copy=(Title==TEXT("Orders")||Title==TEXT("Contracts"))?TEXT("Choose customer work. Follow each job to delivery."):Title==TEXT("Buy steel")?TEXT("Order sheets and keep your next batch supplied."):Title==TEXT("Production")?TEXT("Set jobs. Keep your machines working."):Title==TEXT("Inventory")?TEXT("Your steel, parts and finished assemblies."):Title==TEXT("Trucks")?TEXT("Carriers, deliveries and pickup bookings."):Title==TEXT("Team")?TEXT("Build your crew. Give everyone a job."):Title==TEXT("Money")?TEXT("Income, costs and customer payments."):TEXT("Your next job starts here.");Text(Title,CX,CY,30,true,Ink,CW);Text(Copy,CX,CY+43,15,false,Muted,CW);HelpArea(TEXT("PageIntro"),CX,CY+40,CW,26,Sub);};
 auto PartName=[](int32 Part){return FString(FFoundrylineFactoryCatalog::Part(EFoundrylinePart(Part)).Name);};
 auto Terms=[](EFoundrylinePayment P){return FString(P==EFoundrylinePayment::Upfront?TEXT("Pays when you accept"):P==EFoundrylinePayment::PerDelivery?TEXT("Pays for each delivery"):TEXT("Pays when all parts arrive"));};
 auto PlaceName=[](EFoundrylinePalletPlace P){const TCHAR* N[]={TEXT("Receiving area"),TEXT("Storage rack"),TEXT("Plasma input"),TEXT("Machine output"),TEXT("Buffer"),TEXT("On a forklift"),TEXT("Loading bay"),TEXT("Delivery truck"),TEXT("Pickup truck"),TEXT("On the road"),TEXT("Welding station IN"),TEXT("Welding station OUT")};return FString(N[FMath::Clamp(int(P),0,11)]);};
 auto BookingStatus=[&](const FFoundrylineFreightBooking& B){const TCHAR* N[]={TEXT("Booked"),TEXT("Truck on the way"),TEXT("Waiting for the bay"),B.Inbound?TEXT("Unloading"):TEXT("Loading"),TEXT("Leaving"),TEXT("On the road"),TEXT("Delivered")};return FString(N[FMath::Clamp(int(B.Status),0,6)]);};
 const double SheetLb=FFoundrylineSteelSheet::PoundsEach();
 auto PartPreview=[](int32 Part){return Part==0?FString(TEXT("SteelSheets")):FString::Printf(TEXT("Part%d"),Part);};
 const auto* Plasma=S.Machines.FindByPredicate([](const auto& M){return M.Id==1;});

 // Same charcoal header and restrained orange framing as the build catalogue.
 Rect(0,0,SW,SH,FLinearColor(0,0,0,.62f));Rect(X+7,Y+9,W,H,FLinearColor(0,0,0,.25f));Rect(X,Y,W,H,Surface);Rect(X,Y,W,HeadH,Dark);Rect(X,Y,W,5,Orange);
 Text(TEXT("WORKSHOP"),X+28,Y+22,26,true,Paper);Text(VM.CompanyName.IsEmpty()?FString(TEXT("Your business")):VM.CompanyName,X+28,Y+57,14,false,Hex(TEXT("D8D7D3")),450);
 Text(FFoundrylineCommerceCatalog::Date(C.Clock),X+W-650,Y+38,16,false,Hex(TEXT("D8D7D3")),250);
 RightText(C.Enabled?Cash(C.CashCents):FString(TEXT("--")),X+W-185,Y+30,26,true,Paper);HelpArea(TEXT("Cash"),X+W-400,Y+20,225,50,TEXT("Company cash available for steel, equipment and freight. Shared with your teammates."));
 Button(TEXT("Close"),TEXT("CLOSE  [E]"),X+W-155,Y+24,130,44,EStyle::Secondary,true,TEXT("Close the computer and return to walking. E or Esc also closes it."));

 // Illustrated navigation uses the same transparent assets as the main menu cards.
 auto MenuImage=[&](const TCHAR* Name,float A,float B,float D,float E){static TMap<FString,TStrongObjectPtr<UTexture2D>> Images;auto& Image=Images.FindOrAdd(Name);if(!Image.IsValid())Image.Reset(LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/Foundryline/UI/BuildCatalogue/T_Catalog_"))+Name+TEXT(".T_Catalog_")+Name)));if(Image.IsValid())DrawTexture(Image.Get(),A*Scale,B*Scale,D*Scale,E*Scale,0,0,1,1);};
 Rect(X,Y+HeadH,NavW,H-HeadH-FootH,Rail);Rect(X+12,Y+HeadH+12,NavW-12,H-HeadH-FootH-24,Dark);
 Text(TEXT("YOUR BUSINESS"),X+30,Y+HeadH+18,11,true,Hex(TEXT("AEB2B5")));
 struct FNav {int Page;const TCHAR* Label;const TCHAR* Preview;int Badge;const TCHAR* Help;};
 const FNav Nav[]={
  {9,TEXT("Home"),TEXT("Building"),0,TEXT("Your next job and business shortcuts.")},
  {0,TEXT("Contracts"),TEXT("MenuOrders"),Guide.OffersWaiting,TEXT("Find customer orders, accept them and follow deliveries.")},
  {3,TEXT("Buy steel"),TEXT("SteelSheets"),0,TEXT("Order sheets for your next batch.")},
  {10,TEXT("Production"),TEXT("Stored1"),0,TEXT("Set machine jobs, check progress and buy equipment.")},
  {11,TEXT("Inventory"),TEXT("Parts2"),S.Pallets.Num(),TEXT("Steel, parts, assemblies and every stored pallet.")},
  {4,TEXT("Trucks"),TEXT("TruckDoor"),VM.Summary.OpenBookings,TEXT("Carrier bookings, incoming deliveries and outgoing pickups.")},
  {7,TEXT("Team"),TEXT("MenuTeam"),FFoundrylineEmployeeRoster::Build(S).Num(),TEXT("Hire your crew and assign their jobs.")},
  {5,TEXT("Money"),TEXT("MenuMoney"),0,TEXT("Your income, costs and customer payments.")}};
 for(int I=0;I<UE_ARRAY_COUNT(Nav);++I){const auto& N=Nav[I];if(EmployeeView&&N.Page!=7)continue;const bool On=Page==N.Page||(N.Page==0&&Page==2)||(N.Page==10&&Page==6);const float At=Y+HeadH+32+(EmployeeView?0:I)*59;const float V=Hit(FString::Printf(TEXT("Page%d"),N.Page),X+24,At,NavW-36,54,true,N.Help);
  const auto NavBase=On?Orange:FMath::Lerp(Dark,Graphite,V*.85f);SurfacePanel(X+24,At,NavW-36,54,FMath::Lerp(NavBase,Paper,On?.06f:.025f),NavBase,On?Hex(TEXT("BD5B24")):FMath::Lerp(Dark,Orange,V*.6f),On?2.f:V,6);SurfacePanel(X+30,At+4,68,46,On?Paper:FMath::Lerp(Hex(TEXT("E1DFDA")),Paper,V),Rail,Line,0,4);MenuImage(N.Preview,X+32,At+7-2*V,64,40);
  const float NameW=NavW-(N.Badge>0?157:132);Wrap(N.Label,X+109+V*2,At+(Width(N.Label,16,true)>NameW?8:17),NameW,16,true,On?Ink:Surface,2,18);
  if(N.Badge>0){const FString Count=FString::FromInt(N.Badge);const float BW=FMath::Max(26.f,Width(Count,12,true)+16);Round(X+NavW-32-BW,At+14,BW,26,On?Dark:Graphite,13);Text(Count,X+NavW-32-BW+(BW-Width(Count,12,true))/2,At+20,12,true,Paper);}}
 const float ControlY=Y+HeadH+548;
 Rect(X+28,ControlY,NavW-52,1,Graphite);Text(TEXT("WORKSHOP TIME"),X+32,ControlY+22,12,true,Hex(TEXT("C7C9CA")));
 for(int I=0;I<3;++I){const int Rate=I==0?1:I==1?4:8;Button(FString::Printf(TEXT("Speed%d"),Rate),FString::Printf(TEXT("%dx"),Rate),X+28+I*88,ControlY+48,80,38,S.Speed==Rate?EStyle::Primary:EStyle::Secondary,!PC->bOperationsPending,TEXT("Change how quickly workshop time passes."),S.Speed==Rate);}
 Button(TEXT("Run"),S.Running?TEXT("PAUSE TIME"):TEXT("RESUME TIME"),X+28,ControlY+98,NavW-52,44,S.Running?EStyle::Secondary:EStyle::Primary,!PC->bOperationsPending,TEXT("Pause or resume people, machines and trucks."));
 const bool Prop=PC->bPropertyViewAccess;Button(Prop?TEXT("ReturnOffice"):TEXT("PropertyView"),Prop?TEXT("OFFICE"):TEXT("PLANNER"),X+28,Y+H-FootH-86,NavW-52,54,EStyle::Primary,!EmployeeView&&(Prop||PC->IsNearOfficeComputer()),TEXT("Open the property planner from your office computer."));MenuImage(TEXT("Building"),X+36,Y+H-FootH-79,64,40);

 // Footer: last error from the server, otherwise a hint.
 Rect(X,Y+H-FootH,W,FootH,Hex(TEXT("E8EBEE")));Rect(X,Y+H-FootH,W,1,Line);
 if(!PC->OperationsStatus.IsEmpty()){Rect(X,Y+H-FootH,W,FootH,SoftRed);Rect(X,Y+H-FootH,5,FootH,Red);Text(TEXT("Could not do that: ")+PC->OperationsStatus,X+20,Y+H-FootH+11,14,true,Red,W-40);}
 else {Text(!PC->OperationsConfirm.IsEmpty()&&GetWorld()->GetRealTimeSeconds()-PC->OperationsConfirmAt<5?PC->OperationsConfirm:FString(TEXT("Hover for help  /  E or Esc to close")),X+28,Y+H-FootH+8,12,false,Muted,W-350);RightText(PC->bOperationsPending?TEXT("Sending..."):S.Running?TEXT("WORKSHOP RUNNING"):TEXT("TIME PAUSED"),X+W-28,Y+H-FootH+8,12,true,Muted);}

 if(!C.Enabled&&(Plant->bCutPartsStarter||Page!=7)){Text(Plant->bCutPartsStarter?TEXT("Connecting to your company..."):TEXT("Business finance is not enabled in this workshop. Team & jobs are available."),CX,CY+20,18,true,Muted,CW);return true;}
 const auto* Selected=C.Orders.FindByPredicate([&](const auto& O){return O.Id==PC->OperationsOrder;});
 const auto* Booking=C.Bookings.FindByPredicate([&](const auto& B){return B.Id==PC->OperationsBooking;});
 const auto Roster=FFoundrylineEmployeeRoster::Build(S);

 if(Page==9){
  PageHeader(TEXT("Your workshop"),TEXT("Pick your next move. Build, produce and grow your business."));
  const float Top=CY+86,HeroH=244;Rect(CX,Top,CW,HeroH,Dark);Rect(CX+CW-380,Top,380,HeroH,Graphite);
  Rect(CX,Top,6,HeroH,Orange);Text(TEXT("YOUR NEXT JOB"),CX+28,Top+22,13,true,Orange);RightText(FString::Printf(TEXT("STEP %d / 6"),FMath::Min(6,int(Guide.Step)+1)),CX+CW-28,Top+22,12,true,Paper);
  const float CopyW=CW-420;float At=Top+51;At+=Wrap(Guide.Title,CX+28,At,CopyW,28,true,Paper,2,34)+8;Wrap(Guide.Detail,CX+28,At,CopyW,15,false,Hex(TEXT("D5D6D4")),2,22);
  if(Guide.Step==EFoundrylineGuideStep::ProgramPlasma&&!Guide.bWeldedOrder){Text(TEXT("Use the plasma terminal"),CX+28,Top+HeroH-58,18,true,Paper,CopyW);Text(TEXT("Set the part and batch at the machine."),CX+28,Top+HeroH-32,13,false,Hex(TEXT("D5D6D4")),CopyW);}
  else Button(Guide.Action,Guide.ActionLabel+TEXT("  >"),CX+28,Top+HeroH-64,310,44,EStyle::Primary,!PC->bOperationsPending,TEXT("Open the next step. Purchases still require their normal confirmation."));
  if(Guide.OrderId&&Guide.Problems.Num()){const auto& Problem=Guide.Problems[0];const FString Label=Problem.Action==TEXT("Page7")?TEXT("Assign employees  >"):Problem.Action==TEXT("Run")?TEXT("Resume time  >"):TEXT("Review issue  >");Button(Problem.Action,Label,CX+360,Top+HeroH-64,220,44,EStyle::Secondary,!PC->bOperationsPending,Problem.Text);}
  static TWeakObjectPtr<UTexture2D> WeldingImage,CuttingImage;auto& WorkshopImage=Guide.bWeldedOrder?WeldingImage:CuttingImage;if(!WorkshopImage.IsValid())WorkshopImage=LoadObject<UTexture2D>(nullptr,Guide.bWeldedOrder?TEXT("/Game/Foundryline/UI/BuildCatalogue/T_Catalog_Workstation.T_Catalog_Workstation"):TEXT("/Game/Foundryline/UI/BuildCatalogue/T_Catalog_Stored1.T_Catalog_Stored1"));
  if(WorkshopImage.IsValid())DrawTexture(WorkshopImage.Get(),(CX+CW-350)*Scale,(Top+38)*Scale,300*Scale,187.5f*Scale,0,0,1,1);
  for(int I=0;I<6;++I)Rect(CX+CW-342+I*48,Top+HeroH-22,40,4,Guide.bStepDone[I]?Orange:Hex(TEXT("707579")));
  const float CardsTop=Top+288,TileW=(CW-36)/3,TileH=190;
  Text(TEXT("MANAGE YOUR BUSINESS"),CX,Top+264,13,true,Muted);
  struct FApp {const TCHAR* Action;const TCHAR* Name;FString Value;const TCHAR* Detail;const TCHAR* Preview;int Icon;};
  const FApp Apps[]={
   {TEXT("Page0"),TEXT("Contracts"),FString::Printf(TEXT("%d active / %d offers"),Guide.AcceptedOrders,Guide.OffersWaiting),TEXT("Choose work and track deliveries"),TEXT("MenuOrders"),1},
   {TEXT("Page3"),TEXT("Buy steel"),Lb(Guide.SteelOnHandLb)+TEXT(" in your shop"),TEXT("Order sheets for your next batch"),TEXT("SteelSheets"),2},
   {TEXT("Page10"),TEXT("Production"),FString::Printf(TEXT("%d machine%s installed"),S.Machines.Num(),S.Machines.Num()==1?TEXT(""):TEXT("s")),TEXT("Set jobs and manage equipment"),TEXT("Stored1"),3},
   {TEXT("Page11"),TEXT("Inventory"),FString::Printf(TEXT("%d tracked pallets"),S.Pallets.Num()),TEXT("Steel, parts and finished assemblies"),TEXT("Parts2"),4},
   {TEXT("Page7"),TEXT("Team"),FString::Printf(TEXT("%d employees"),Roster.Num()),TEXT("Hire people and assign their jobs"),TEXT("MenuTeam"),5},
   {TEXT("Page5"),TEXT("Money"),Cash(Guide.OwedToYouCents)+TEXT(" owed to you"),TEXT("See income, costs and payments"),TEXT("MenuMoney"),6}};
  static TMap<FString,TStrongObjectPtr<UTexture2D>> AppImages;
  for(int I=0;I<6;++I){const auto& App=Apps[I];const float A=CX+(I%3)*(TileW+18),B=CardsTop+(I/3)*(TileH+18);const float V=Hit(App.Action,A,B,TileW,TileH,true,App.Detail);SurfacePanel(A,B,TileW,TileH,FMath::Lerp(Paper,Dark,V),FMath::Lerp(Surface,Dark,V),FMath::Lerp(Line,Orange,V),2+V,8);Round(A+1,B+1,TileW-2,112,FMath::Lerp(Rail,SoftOrange,V),7);Rect(A+1,B+104,TileW-2,9,FMath::Lerp(Rail,SoftOrange,V));
   if(*App.Preview){auto& Image=AppImages.FindOrAdd(App.Preview);if(!Image.IsValid())Image.Reset(LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/Foundryline/UI/BuildCatalogue/T_Catalog_"))+App.Preview+TEXT(".T_Catalog_")+App.Preview)));if(Image.IsValid())DrawTexture(Image.Get(),(A+(TileW-190)/2)*Scale,(B-3*V)*Scale,190*Scale,118.75f*Scale,0,0,1,1);}
   else {Rect(A+TileW-84,B+18,54,54,Dark);Icon(App.Icon,A+TileW-75,B+27,36,Orange);}
   Text(App.Name,A+22,B+126,22,true,FMath::Lerp(Ink,Paper,V),TileW-65);Text(App.Value,A+22,B+159,13,false,FMath::Lerp(Muted,Orange,V),TileW-65);Text(TEXT(">"),A+TileW-38,B+133,20,true,Orange);Rect(A+8,B+TileH-2,TileW-16,2,FMath::Lerp(Line,Orange,V));}
 }
 else if(Page==0||Page==2){
  const bool Mine=Page==2;PageHeader(TEXT("Contracts"),Mine?TEXT("Your accepted work, delivery progress and payments."):TEXT("Customer offers. Deadlines start when you accept a contract."));
  TArray<const FFoundrylineCustomerOrder*> Rows;int OfferCount=0,MineCount=0;for(const auto& O:C.Orders){OfferCount+=O.Status==EFoundrylineOrderState::Offered;MineCount+=O.Status!=EFoundrylineOrderState::Offered;if((O.Status!=EFoundrylineOrderState::Offered)==Mine)Rows.Add(&O);}
  if(Mine)Rows.Sort([](const auto& A,const auto& B){return (A.Status==EFoundrylineOrderState::Completed)!=(B.Status==EFoundrylineOrderState::Completed)?A.Status!=EFoundrylineOrderState::Completed:A.Id>B.Id;});
  Tabs({MakeTuple(FString(TEXT("Page0")),FString::Printf(TEXT("Available contracts (%d)"),OfferCount),!Mine,FString(TEXT("Offers you can accept."))),MakeTuple(FString(TEXT("Page2")),FString::Printf(TEXT("Your contracts (%d)"),MineCount),Mine,FString(TEXT("Accepted and recently completed jobs.")))},CX,CY+74);
  const float Top=CY+134,LW=CW*.46f,RX=CX+LW+22,RW=CW-LW-22,RowH=144;const int Per=FMath::Max(1,int((CB-Top-50)/RowH)),F=First(Rows.Num(),Per);
  auto ShortTerms=[](EFoundrylinePayment Payment){return FString(Payment==EFoundrylinePayment::Upfront?TEXT("Pays upfront"):Payment==EFoundrylinePayment::PerDelivery?TEXT("Pays per delivery"):TEXT("Pays on completion"));};
  for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& O=*Rows[I];const float RY=Top+(I-F)*RowH;const bool On=O.Id==PC->OperationsOrder;const float V=Hit(FString::Printf(TEXT("Order%d"),O.Id),CX,RY,LW,RowH-8,true);
   SurfacePanel(CX,RY,LW,RowH-8,On?SoftOrange:Paper,FMath::Lerp(Surface,SoftOrange,V),On?Orange:FMath::Lerp(Line,Orange,V),1+V,8);
   Mark(O.Customer,CX+14,RY+10,34);Text(O.Customer,CX+58,RY+15,15,true,Ink,LW-240);RightText(Cash(int64(O.Quantity)*O.UnitPriceCents),CX+LW-18,RY+13,19,true,Ink);
   Round(CX+12,RY+50,132,76,Rail,6);MenuImage(*PartPreview(O.Part),CX+16,RY+47,124,77.5f);
   Text(FString::Printf(TEXT("%d x %s"),O.Quantity,*PartName(O.Part)),CX+160,RY+51,17,true,Ink,LW-180);Text(ShortTerms(O.Payment),CX+160,RY+80,13,false,Muted,LW-180);
   const FString Status=O.Status==EFoundrylineOrderState::Completed?TEXT("DELIVERED"):O.Late?TEXT("LATE"):O.Status==EFoundrylineOrderState::Accepted?TEXT("IN PROGRESS"):O.Rush?TEXT("RUSH JOB"):O.Email?TEXT("DIRECT OFFER"):TEXT("AVAILABLE");
   Chip(Status,CX+160,RY+106,O.Late?Red:O.Status==EFoundrylineOrderState::Completed?Green:Amber,O.Late?SoftRed:O.Status==EFoundrylineOrderState::Completed?SoftGreen:SoftOrange);
   const FString Due=O.Deadline>0?FFoundrylineCommerceCatalog::Date(O.Deadline):O.AllowedMinutes>0?FString::Printf(TEXT("%.0f game hours"),O.AllowedMinutes/60):FString(TEXT("No deadline"));RightText(Due,CX+LW-18,RY+111,12,false,O.Rush?Amber:Muted);
  }
  if(Rows.IsEmpty()){Card(CX,Top,LW,190);MenuImage(TEXT("MenuOrders"),CX+20,Top+20,120,75);Text(Mine?TEXT("No contracts yet"):TEXT("No new offers"),CX+154,Top+30,21,true,Ink,LW-180);Wrap(Mine?TEXT("Choose a customer job to get started."):TEXT("New customer offers arrive over time."),CX+154,Top+65,LW-180,14,false,Muted,2);if(Mine)Button(TEXT("Page0"),TEXT("Browse contracts  >"),CX+24,Top+128,LW-48,42,EStyle::Primary);}
  Pager(Rows.Num(),Per,CX,CB-38,LW);
  const auto* O=Selected&&Rows.Contains(Selected)?Selected:nullptr;Card(RX,Top,RW,CB-Top);
  if(!O){MenuImage(TEXT("MenuOrders"),RX+(RW-220)/2,Top+50,220,137.5f);Text(TEXT("Choose a contract"),RX+28,Top+218,24,true,Ink,RW-56);Wrap(TEXT("See the product, the payout and what your workshop needs before accepting."),RX+28,Top+264,RW-56,16,false,Muted,3);}
  else {
   const bool Welded=O->Part==uint8(EFoundrylinePart::HeadacheRack);const bool Bent=O->Part==uint8(EFoundrylinePart::BentBracket)||O->Part==uint8(EFoundrylinePart::SupportChannel)||O->Part==uint8(EFoundrylinePart::FrameRail);const int Left=FMath::Max(0,O->Quantity-O->Delivered);
   const auto Programs=FFoundrylineFactoryCatalog::DefaultPrograms();const auto Blank=O->Part==uint8(EFoundrylinePart::BentBracket)?EFoundrylinePart::Bracket:O->Part==uint8(EFoundrylinePart::SupportChannel)?EFoundrylinePart::ReinforcementPlate:O->Part==uint8(EFoundrylinePart::FrameRail)?EFoundrylinePart::RailBlank:EFoundrylinePart(O->Part);
   const auto* Program=Programs.FindByPredicate([&](const auto& P){return P.Machine==1&&P.Part==Blank;});const double NeedLb=Program?Left*double(Program->SteelLb):0;
   const bool BrakeStaffed=Roster.ContainsByPredicate([&](const auto& E){return E.Role==EFoundrylineEmployeeRole::Operator&&S.Machines.ContainsByPredicate([&](const auto& M){return M.Kind()==2&&!M.bStored&&M.Id==E.Assignment;});});
   const bool PlasmaReady=S.Machines.ContainsByPredicate([](const auto& M){return M.Kind()==1&&!M.bStored;}),BrakeReady=S.Machines.ContainsByPredicate([](const auto& M){return M.Kind()==2&&!M.bStored;});
   FFoundrylineAssemblyRecipe Kit;FFoundrylineFactoryCatalog::Recipe(1,Kit);auto KitNeed=[&](EFoundrylinePart P){return Kit.Bill.IsValidIndex(int(P))?Kit.Bill[int(P)]*Left:0;};
   auto OnHand=[&](EFoundrylinePart P){int N=0;for(const auto& X:S.Pallets)if(X.Part==P&&!X.BookingId&&X.Place!=EFoundrylinePalletPlace::InTransit)N+=X.Quantity-X.Reserved;return N;};
   Mark(O->Customer,RX+24,Top+18,54);Text(O->Customer.ToUpper(),RX+94,Top+25,17,true,Ink,RW-118);Text(O->Reference+TEXT(" / ")+(O->Email?TEXT("Direct customer offer"):TEXT("Customer contract")),RX+94,Top+53,12,false,Muted,RW-118);
   Round(RX+RW-274,Top+88,250,156,Rail,8);MenuImage(*PartPreview(O->Part),RX+RW-270,Top+88,242,151.25f);
   Wrap(PartName(O->Part),RX+24,Top+97,RW-320,25,true,Ink,2,30);Text(FString::Printf(TEXT("%d UNITS REQUESTED"),O->Quantity),RX+24,Top+173,14,true,Muted,RW-320);Chip(Welded?TEXT("WELDED ASSEMBLY"):Bent?TEXT("CUT + FORM"):TEXT("CUT PART"),RX+24,Top+205,Amber,SoftOrange);
   const float FactW=(RW-68)/2;Text(TEXT("TOTAL PAYOUT"),RX+24,Top+260,12,true,Muted);Text(Cash(int64(O->Quantity)*O->UnitPriceCents),RX+24,Top+280,29,true,Green,FactW);
   const FString Due=O->Deadline>0?FFoundrylineCommerceCatalog::Date(O->Deadline):O->AllowedMinutes>0?FString::Printf(TEXT("%.0f game hours"),O->AllowedMinutes/60):FString(TEXT("No deadline"));Text(O->Deadline>0?TEXT("DELIVER BY"):TEXT("TIME AFTER ACCEPTING"),RX+44+FactW,Top+260,12,true,Muted);Text(Due,RX+44+FactW,Top+284,19,true,O->Late?Red:Ink,FactW);
   Text(TEXT("PAYMENT"),RX+24,Top+327,12,true,Muted);Text(ShortTerms(O->Payment),RX+24,Top+347,16,true,Ink,FactW);HelpArea(TEXT("ContractPayment"),RX+24,Top+324,FactW,48,Terms(O->Payment));
   Text(TEXT("PER UNIT"),RX+44+FactW,Top+327,12,true,Muted);Text(Cash(O->UnitPriceCents),RX+44+FactW,Top+347,19,true,Ink,FactW);
   Rect(RX+24,Top+385,RW-48,1,Line);
   const FString Inputs=Welded?FString::Printf(TEXT("Per rack: %d plates, %d brackets, %d frames, %d lights"),Kit.Bill[int(EFoundrylinePart::MountingPlate)],Kit.Bill[int(EFoundrylinePart::Bracket)],Kit.Bill[int(EFoundrylinePart::FrameSection)],Kit.Bill[int(EFoundrylinePart::LightKit)]):Program?FString::Printf(TEXT("%s steel / %d sheet%s / %s"),*Lb(NeedLb),FMath::CeilToInt(NeedLb/SheetLb),FMath::CeilToInt(NeedLb/SheetLb)==1?TEXT(""):TEXT("s"),Bent?TEXT("Plasma + press brake"):TEXT("Plasma cutter")):FString(TEXT("Purchased component"));Text(Inputs,RX+24,Top+399,13,true,Muted,RW-48);
   float At=Top+435;
   if(O->Status==EFoundrylineOrderState::Offered){
    Text(TEXT("WORKSHOP CHECK"),RX+24,At,12,true,Muted);At+=24;
    auto Ready=[&](bool Ok,const FString& T){Round(RX+24,At+3,12,12,Ok?Green:Orange,3);Text(T,RX+48,At,13,false,Ok?Ink:Amber,RW-72);HelpArea(TEXT("ContractCheck")+T,RX+22,At-2,RW-44,22,T);At+=24;};
    if(Welded){Ready(!S.WeldingStations.IsEmpty(),S.WeldingStations.IsEmpty()?TEXT("Place an assembly workstation"):TEXT("Assembly workstation ready"));Ready(Guide.bWelder,Guide.bWelder?TEXT("Welder assigned"):TEXT("Assign a welder in Team"));Ready(Guide.bForkliftDriver,Guide.bForkliftDriver?TEXT("Forklift driver assigned"):TEXT("Assign a forklift driver in Team"));
     const int P=FMath::Max(0,KitNeed(EFoundrylinePart::MountingPlate)-OnHand(EFoundrylinePart::MountingPlate)),B=FMath::Max(0,KitNeed(EFoundrylinePart::Bracket)-OnHand(EFoundrylinePart::Bracket));Ready(!P&&!B,!P&&!B?FString(TEXT("Cut parts ready")):FString::Printf(TEXT("Still need %d plates and %d brackets"),P,B));
     const int Frames=FMath::Max(0,KitNeed(EFoundrylinePart::FrameSection)-OnHand(EFoundrylinePart::FrameSection)),Lights=FMath::Max(0,KitNeed(EFoundrylinePart::LightKit)-OnHand(EFoundrylinePart::LightKit));Ready(!Frames&&!Lights,!Frames&&!Lights?FString(TEXT("Bought components ready")):FString::Printf(TEXT("Still need %d frame sections and %d light kits"),Frames,Lights));
    }else{Ready(PlasmaReady&&(!Bent||BrakeReady),!PlasmaReady?FString(TEXT("Place a plasma cutter")):Bent&&!BrakeReady?FString(TEXT("Add a press brake")):FString(TEXT("Required machines installed")));const double Have=Guide.SteelOnHandLb+Guide.SteelIncomingLb-Guide.SteelNeededLb;Ready(Program&&Have>=NeedLb,Program?Have>=NeedLb?FString(TEXT("Steel covered by stock and incoming deliveries")):FString::Printf(TEXT("Need about %s more steel"),*Lb(NeedLb-FMath::Max(0.,Have))):FString(TEXT("Check component sourcing before accepting")));Ready(Guide.bOperator,Guide.bOperator?TEXT("Plasma operator assigned"):TEXT("Assign a plasma operator in Team"));Ready(Guide.bForkliftDriver,Guide.bForkliftDriver?TEXT("Forklift driver assigned"):TEXT("Assign a forklift driver in Team"));if(Bent)Ready(BrakeStaffed,BrakeStaffed?TEXT("Press-brake operator assigned"):TEXT("Assign a press-brake operator in Team"));}
    Ready(Guide.AcceptedOrders<6,FString::Printf(TEXT("%d of 6 active contract slots in use"),Guide.AcceptedOrders));Button(TEXT("Accept"),TEXT("Accept contract"),RX+24,CB-74,RW-48,52,EStyle::Primary,!PC->bOperationsPending&&Guide.AcceptedOrders<6,TEXT("Accept this customer contract. Its deadline starts now; production is managed separately."));
   }else{
    int Free=0,Bay=0;for(const auto& P:S.Pallets)if(int(P.Part)==O->Part&&!P.BookingId&&P.Place!=EFoundrylinePalletPlace::InTransit){Free+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Pickup)Bay+=P.Quantity;}
    int Trucks=0;for(const auto& B:C.Bookings)if(!B.Inbound&&B.OrderId==O->Id&&B.Status!=EFoundrylineBookingState::Delivered)for(const auto& Item:B.Manifest)Trucks+=Item.Quantity;
    Text(TEXT("PROGRESS"),RX+28,At,12,true,Muted);At+=24;const float BW=RW-56;const float Q=FMath::Max(1,O->Quantity);
    Rect(RX+28,At,BW,14,Surface);float BX=RX+28;auto Seg=[&](int N,FLinearColor Col){const float D=BW*FMath::Clamp(N/Q,0.f,1.f);Rect(BX,At,D,14,Col);BX+=D;};Seg(O->Delivered,Green);Seg(FMath::Min(Trucks,O->Quantity-O->Delivered),Blue);Seg(FMath::Min(Free,FMath::Max(0,O->Quantity-O->Delivered-Trucks)),Hex(TEXT("9FB7C6")));At+=22;
    Text(FString::Printf(TEXT("Delivered %d   /   On trucks %d   /   In stock %d (%d at the bay)   of %d"),O->Delivered,Trucks,Free,Bay,O->Quantity),RX+28,At,13,false,Muted,BW);HelpArea(TEXT("Progress"),RX+28,At-26,BW,46,TEXT("Green: delivered to the customer. Blue: loaded on booked trucks. Grey: finished parts in your shop that match this order."));At+=28;
    Text(FString::Printf(TEXT("Paid so far: %s of %s"),*Cash(O->PaidCents),*Cash(O->Quantity*O->UnitPriceCents)),RX+28,At,15,true,O->PaidCents>0?Green:Ink,BW);At+=34;
    if(O->Status==EFoundrylineOrderState::Accepted){
     if(Guide.OrderId==O->Id){Rect(RX+28,At,BW,40,SoftOrange);Text(TEXT("Next: ")+Guide.Title,RX+42,At+11,14,true,Amber,BW-28);At+=48;}
     const float Half=(BW-12)/2;
     int ReadyToMove=0;for(const auto& P:S.Pallets)if(int(P.Part)==O->Part&&!P.BookingId&&!P.Reserved&&(P.Place==EFoundrylinePalletPlace::Rack||P.Place==EFoundrylinePalletPlace::MachineOutput||P.Place==EFoundrylinePalletPlace::StationOut))ReadyToMove+=P.Quantity;
     int Moving=0;for(const auto& J:S.Jobs)if(J.OrderId==O->Id&&J.DestinationPlace==EFoundrylinePalletPlace::Pickup)Moving+=J.Quantity;const int StillToStage=FMath::Max(0,O->Quantity-O->Delivered-Trucks-Bay-Moving);ReadyToMove=FMath::Min(ReadyToMove,StillToStage);
     Button(TEXT("Assign"),ReadyToMove>0?TEXT("Move ready parts to bay"):StillToStage==0?TEXT("Already staged / booked"):TEXT("No parts ready"),RX+28,CB-76,Half,54,ReadyToMove>0?EStyle::Primary:EStyle::Ghost,ReadyToMove>0&&!PC->bOperationsPending,FString::Printf(TEXT("%d matching, unreserved parts are ready for this contract. %d are already moving to the bay. A real forklift load is requested only when stock is ready."),ReadyToMove,Moving));
     if(O->Part==uint8(EFoundrylinePart::HeadacheRack))Button(FString::Printf(TEXT("StationOrder%d"),O->Id),TEXT("Set welding station"),RX+40+Half,CB-76,Half,54,C.PickupOrder==O->Id?EStyle::Primary:EStyle::Secondary,!PC->bOperationsPending&&Guide.OrderId==O->Id&&Guide.PartsToMake>0,TEXT("Set the welding station's batch to the racks this order still needs. Racks already made or in progress are not counted again."));
     else {Rect(RX+40+Half,CB-76,Half,54,Surface);Text(TEXT("Use machine terminal"),RX+52+Half,CB-66,13,true,Muted,Half-24);Text(TEXT("Set parts and batches there"),RX+52+Half,CB-44,12,false,Muted,Half-24);}
    } else Text(O->Late?TEXT("Completed late."):TEXT("Completed and delivered."),RX+28,At,15,true,Green,BW);
   }
  }
 }
 else if(Page==3){
  PageHeader(TEXT("Buy steel"),TEXT("Every part is cut from steel sheet. Buy enough for your accepted orders; a truck brings it to your loading bay."));
  int Inbound=0;for(const auto& B:C.Bookings)Inbound+=B.Inbound;
  Tabs({MakeTuple(FString(TEXT("Workspace0")),FString(TEXT("Buy steel")),PC->OperationsWorkspaceTab==0,FString(TEXT("Order new steel sheets."))),MakeTuple(FString(TEXT("Workspace1")),FString::Printf(TEXT("Steel deliveries (%d)"),Inbound),PC->OperationsWorkspaceTab==1,FString(TEXT("Steel you've ordered and where each truck is.")))},CX,CY+74);
  const float Top=CY+134;
  if(PC->OperationsWorkspaceTab==0){
   const float LW=CW*.58f,RX=CX+LW+22,RW=CW-LW-22;const int N=PC->OperationsSheets;const double Pounds=SheetLb*N;const int64 Amount=FMath::RoundToInt64(Pounds*100),Total=Amount+2500;
   Card(CX,Top,LW,CB-Top);Mark(TEXT("Foundry Steel Supply"),CX+28,Top+26,52);Text(TEXT("Foundry Steel Supply"),CX+94,Top+28,18,true,Ink,LW-122);Text(TEXT("Your steel supplier"),CX+94,Top+54,13,false,Muted,LW-122);
   Text(TEXT("Steel sheet"),CX+28,Top+104,26,true,Ink);Wrap(FString::Printf(TEXT("60 x 120 in / 1/8 in thick / %.0f lb each / $1.00 per lb"),SheetLb),CX+28,Top+145,LW-310,14,false,Muted,3);MenuImage(TEXT("SteelSheets"),CX+LW-270,Top+90,240,150);
   float At=Top+246;
   {const bool Need=Guide.SheetsToBuy>0,Any=Guide.AcceptedOrders>0;Rect(CX+28,At,LW-56,72,Need?SoftOrange:Any?SoftGreen:Surface);Rect(CX+28,At,4,72,Need?Orange:Any?Green:Line);
    Text(Need?FString::Printf(TEXT("Your accepted orders need %d more sheet%s"),Guide.SheetsToBuy,Guide.SheetsToBuy==1?TEXT(""):TEXT("s")):Any?FString(TEXT("You have enough steel for your accepted orders")):FString(TEXT("Accept an order first to see how much steel you need")),CX+46,At+14,16,true,Need?Amber:Any?Green:Muted,LW-260);
    Text(FString::Printf(TEXT("Needed %s  /  In the shop %s  /  On the way %s"),*Lb(Guide.SteelNeededLb),*Lb(Guide.SteelOnHandLb),*Lb(Guide.SteelIncomingLb)),CX+46,At+42,13,false,Muted,LW-260);
    if(Need)Button(FString::Printf(TEXT("BuySheets%d"),FMath::Clamp(Guide.SheetsToBuy,1,10)),FString::Printf(TEXT("Use %d"),FMath::Clamp(Guide.SheetsToBuy,1,10)),CX+LW-196,At+16,150,40,EStyle::Secondary,true,TEXT("Set the quantity to exactly what your accepted orders still need."));}
   At+=100;Text(TEXT("HOW MANY SHEETS"),CX+28,At,12,true,Muted);HelpArea(TEXT("Qty"),CX+28,At-4,200,22,TEXT("You can buy 1 to 10 whole sheets per delivery. Leftover steel stays on your rack for the next order."));
   Button(TEXT("SheetsMinus"),TEXT("-"),CX+28,At+24,60,56,EStyle::Secondary,N>1,TEXT("One sheet fewer."));Text(FString::FromInt(N),CX+98+(70-Width(FString::FromInt(N),34,true))/2,At+30,34,true,Ink);Button(TEXT("SheetsPlus"),TEXT("+"),CX+178,At+24,60,56,EStyle::Secondary,N<10,TEXT("One sheet more (up to 10 per delivery)."));
   Text(FString::Printf(TEXT("= %s of steel"),*Lb(Pounds)),CX+260,At+40,16,false,Muted);
   At+=110;Text(TEXT("DELIVERY ARRIVES"),CX+28,At,12,true,Muted);Text(FFoundrylineCommerceCatalog::Date(C.Clock+PC->OperationsDelay),CX+28,At+22,22,true,Ink);HelpArea(TEXT("Arrival"),CX+28,At-4,260,56,TEXT("When the supplier's truck reaches your loading bay. It may wait if another truck is using the bay."));
   Button(TEXT("TimeMinus"),TEXT("Sooner -30m"),CX+300,At+14,150,42,EStyle::Secondary,PC->OperationsDelay>30,TEXT("Ask for the delivery 30 game minutes earlier."));Button(TEXT("TimePlus"),TEXT("Later +30m"),CX+460,At+14,150,42,EStyle::Secondary,true,TEXT("Ask for the delivery 30 game minutes later."));
   At+=86;Wrap(TEXT("The truck unloads at the loading bay, then your forklift driver puts the sheets on the steel rack next to the plasma cutter."),CX+28,At,LW-56,14,false,Muted,2);
   if(!Guide.bForkliftDriver){Rect(CX+28,CB-70,LW-56,46,SoftRed);Rect(CX+28,CB-70,4,46,Red);Text(TEXT("No forklift driver: delivered steel can't be unloaded."),CX+46,CB-56,14,true,Red,LW-220);Button(TEXT("Page7"),TEXT("Fix this >"),CX+LW-150,CB-64,110,34,EStyle::Secondary,true,TEXT("Open Team to assign a forklift driver."));}
   Card(RX,Top,RW,CB-Top,TEXT("Order summary"));
   auto Row=[&](const FString& L,const FString& V,float B,const FString& Help){Text(L,RX+24,B,15,false,Muted);RightText(V,RX+RW-24,B,16,true,Ink);HelpArea(TEXT("Sum")+L,RX+20,B-6,RW-40,32,Help);};
   Row(FString::Printf(TEXT("Steel: %d x %.0f lb"),N,SheetLb),Cash(Amount),Top+64,TEXT("Steel is sold by weight at $1.00 per lb."));Row(TEXT("Delivery"),Cash(2500),Top+102,TEXT("The supplier charges $25 per delivery, however many sheets are on it."));Rect(RX+24,Top+142,RW-48,1,Line);
   Text(TEXT("TOTAL"),RX+24,Top+162,12,true,Muted);Text(Cash(Total),RX+24,Top+182,36,true,Ink);HelpArea(TEXT("Total"),RX+20,Top+156,RW-40,74,TEXT("Paid from your cash when you confirm."));
   Row(TEXT("Cash after buying"),Cash(C.CashCents-Total),Top+250,TEXT("Your cash once this purchase is paid. Keep some for truck pickups ($25-$180)."));
   if(C.CashCents<Total)Text(TEXT("Not enough cash for this purchase."),RX+24,Top+290,14,true,Red,RW-48);
   Button(TEXT("BuySteel"),FString::Printf(TEXT("Buy %d sheet%s  /  %s"),N,N==1?TEXT(""):TEXT("s"),*Cash(Total)),RX+24,CB-78,RW-48,56,EStyle::Primary,!PC->bOperationsPending,TEXT("Pay now and book the supplier's delivery truck for the time shown."));
  } else {
   TArray<const FFoundrylineFreightBooking*> Rows;for(const auto& B:C.Bookings)if(B.Inbound)Rows.Add(&B);Rows.Sort([](const auto& A,const auto& B){return A.ArrivalAt>B.ArrivalAt;});
   Card(CX,Top,CW,CB-Top);const int Per=FMath::Max(1,int((CB-Top-110)/64)),F=First(Rows.Num(),Per);
   Text(TEXT("ORDER"),CX+24,Top+18,12,true,Muted);Text(TEXT("STEEL"),CX+CW*.2f,Top+18,12,true,Muted);Text(TEXT("ARRIVES"),CX+CW*.42f,Top+18,12,true,Muted);Text(TEXT("WHERE IT IS"),CX+CW*.66f,Top+18,12,true,Muted);Rect(CX+16,Top+44,CW-32,1,Line);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& B=*Rows[I];const float RY=Top+52+(I-F)*64;const float V=Hit(FString::Printf(TEXT("CalendarBooking%d"),B.Id),CX+8,RY,CW-16,58,true,TEXT("Open this delivery in Trucks."));if(V>.01f||(I-F)%2)Rect(CX+8,RY,CW-16,58,V>.01f?Hex(TEXT("F4E8DC")):Rail);Hover(CX+8,RY,CW-16,58,V);
    Text(FString::Printf(TEXT("PO-%04d"),B.Id),CX+24,RY+18,15,true,Ink);Text(B.Part?FFoundrylineCommerceCatalog::InboundLabel(B):FString::Printf(TEXT("%d sheet%s  /  %s"),B.Sheets,B.Sheets==1?TEXT(""):TEXT("s"),*Lb(B.Sheets*SheetLb)),
CX+CW*.2f,RY+18,15,false,Ink,CW*.2f);Text(FFoundrylineCommerceCatalog::Date(B.ArrivalAt),CX+CW*.42f,RY+18,15,false,Ink,CW*.22f);
    Chip(BookingStatus(B).ToUpper(),CX+CW*.66f,RY+17,B.Status==EFoundrylineBookingState::Delivered?Green:Blue,B.Status==EFoundrylineBookingState::Delivered?SoftGreen:SoftBlue);}
   if(Rows.IsEmpty()){Text(TEXT("No steel ordered yet"),CX+24,Top+70,20,true,Ink);Button(TEXT("Workspace0"),TEXT("Buy steel  >"),CX+24,Top+112,200,44,EStyle::Primary,true,TEXT("Order your first steel sheets."));}
   Pager(Rows.Num(),Per,CX+20,CB-52,CW-40);
  }
 }
 else if(Page==10||Page==6){
  PageHeader(TEXT("Production"),Page==10?TEXT("What each machine is doing. The plasma cutter turns steel sheet into parts."):TEXT("Add machines and forklifts as your business grows. New equipment needs a person assigned in Team."));
  Tabs({MakeTuple(FString(TEXT("Page10")),FString(TEXT("Machines")),Page==10,FString(TEXT("Live status of your machines."))),MakeTuple(FString(TEXT("Page6")),FString(TEXT("Buy equipment")),Page==6,FString(TEXT("Purchase machines or forklifts, then place your owned equipment from Planning.")))},CX,CY+74);
  const float Top=CY+134;
  if(Page==10){
   int Row=0;for(const auto& M:S.Machines){if(M.bStored)continue;const float RY=Top+Row++*196;if(RY+184>CB)break;const auto* Op=Roster.FindByPredicate([&](const auto& E){return E.Role==EFoundrylineEmployeeRole::Operator&&E.Assignment==M.Id;});
    const FString Name=M.Kind()==1?TEXT("Plasma cutter"):M.Kind()==2?TEXT("Press brake"):TEXT("Sheet roller");const int Left=M.Auto?0:M.BatchRemaining+(M.Remaining>0?1:0);
    const bool Cutting=S.Running&&M.Enabled&&M.Remaining>0;FString State;FLinearColor Col;
    if(M.Id==2&&C.BrakeInstallMinutes>0){State=FString::Printf(TEXT("INSTALLING / %.0f GAME MIN"),FMath::CeilToFloat(C.BrakeInstallMinutes));Col=Amber;}
    else if(!S.Running&&Left){State=TEXT("TIME PAUSED");Col=Amber;}else if(!M.Enabled){State=TEXT("MACHINE PAUSED");Col=Amber;}else if(Cutting){State=M.Kind()==1?TEXT("CUTTING"):TEXT("WORKING");Col=Green;}else if(Left&&!Op){State=TEXT("NEEDS AN OPERATOR");Col=Red;}else if(Left){State=M.Kind()==1?TEXT("WAITING FOR STEEL"):TEXT("WAITING FOR PARTS");Col=Amber;}else {State=TEXT("NO JOB");Col=Muted;}
    Card(CX,RY,CW,184);Rect(CX,RY,4,184,Col);MenuImage(M.Kind()==1?TEXT("Stored1"):M.Kind()==2?TEXT("Stored2"):TEXT("Stored3"),CX+16,RY+24,128,80);Text(Name,CX+156,RY+22,22,true,Ink,CW*.36f-180);Chip(State,CX+156,RY+58,Col,Col==Green?SoftGreen:Col==Red?SoftRed:Col==Muted?Surface:SoftOrange);
    HelpArea(FString::Printf(TEXT("MachineState%d"),M.Id),CX+70,RY+54,220,32,TEXT("Cutting: working now. Waiting for steel: the forklift still has to bring a sheet. Needs an operator: assign someone in Team. No job: program it with a part and quantity."));
    const float IX=CX+CW*.36f;Text(TEXT("OPERATOR"),IX,RY+24,12,true,Muted);Text(Op?Op->Label:FString(TEXT("Nobody assigned")),IX,RY+44,17,true,Op?Ink:Red,CW*.26f);
    Text(TEXT("MAKING"),IX,RY+90,12,true,Muted);Text(Left?FString::Printf(TEXT("%s  /  %d left"),*PartName(int(M.Making)),Left):FString(TEXT("Nothing programmed")),IX,RY+110,17,true,Ink,CW*.3f);
    HelpArea(FString::Printf(TEXT("MachineMaking%d"),M.Id),IX-4,RY+86,CW*.3f,50,TEXT("The part this machine is making and how many are left in the batch, including the one in progress."));
    if(M.Kind()==1){Text(TEXT("STEEL LOADED"),IX+CW*.32f,RY+24,12,true,Muted);Text(Lb(M.BedLb),IX+CW*.32f,RY+44,17,true,Ink);HelpArea(TEXT("Bed"),IX+CW*.32f-4,RY+20,170,50,TEXT("Steel already on the cutting table. Each part uses a fixed weight; leftovers carry over to the next part."));}
    const float Progress=M.Duration>0?1-M.Remaining/M.Duration:0;if(M.Remaining>0){Text(FString::Printf(TEXT("Current part %d%%"),FMath::RoundToInt(Progress*100)),IX+CW*.32f,RY+90,12,true,Muted);Bar(IX+CW*.32f,RY+114,CW*.18f,Progress,Orange);}
    const float BX=CX+CW-266;
    Text(TEXT("MACHINE CONTROLS"),BX,RY+28,12,true,Muted,240);Text(TEXT("Use machine terminal"),BX,RY+51,16,true,Ink,240);Text(TEXT("Walk to its screen and press E."),BX,RY+77,12,false,Muted,240);
    if(!Op)Button(TEXT("Page7"),TEXT("Assign operator"),BX,RY+112,240,44,EStyle::Secondary,true,TEXT("Open Team to put someone on this machine."));
   }
   // Welding stations: what each is building, why it waits, and what sits on its OUT pallets.
   for(const auto& St:S.WeldingStations){const float RY=Top+Row*196;if(RY+184>CB)break;++Row;const bool Building=St.Stage>0&&St.bAdvancing;const bool Held=Guide.StationId==St.Id&&(Guide.StationStatus.Contains(TEXT("Missing"))||Guide.StationStatus.Contains(TEXT("OUT full"))||Guide.StationStatus.Contains(TEXT("paused")));
    const FLinearColor Col=Building?Green:Held?Amber:St.Recipe?Blue:Muted;const FString StateText=Building?TEXT("WELDING"):Held?TEXT("HELD UP"):St.Recipe?TEXT("STARTING"):TEXT("NO BATCH");
    Card(CX,RY,CW,184);Rect(CX,RY,4,184,Col);MenuImage(TEXT("Workstation"),CX+16,RY+24,128,80);Text(FFoundrylineWeldingStationLayout::Name(St.Number),CX+156,RY+22,22,true,Ink,CW*.36f-180);Chip(StateText,CX+156,RY+58,Col,Col==Green?SoftGreen:Col==Amber?SoftOrange:Col==Blue?SoftBlue:Surface);
    const float IX=CX+CW*.36f;int Out=0;for(const auto& P:S.Pallets)if(P.Owner==St.Id&&P.Place==EFoundrylinePalletPlace::StationOut)Out+=P.Quantity;
    Text(TEXT("STATUS"),IX,RY+24,12,true,Muted);Wrap(Guide.StationId==St.Id?Guide.StationStatus:FString(St.Recipe?TEXT("Building headache racks"):TEXT("Choose a product and quantity")),IX,RY+44,CW*.3f,15,true,Ink,2);
    Text(TEXT("MAKING"),IX,RY+90,12,true,Muted);Text(St.Recipe||St.Stage?FString::Printf(TEXT("Headache racks  /  %d left"),St.BatchRemaining+(St.Stage?1:0)):FString(TEXT("Nothing set")),IX,RY+110,17,true,Ink,CW*.3f);
    Text(TEXT("ON OUT"),IX+CW*.32f,RY+24,12,true,Muted);Text(FString::Printf(TEXT("%d of %d"),Out,FFoundrylineWeldingStationLayout::OutPallets*FFoundrylineWeldingStationLayout::RacksPerOutPallet),IX+CW*.32f,RY+44,17,true,Ink);
    HelpArea(FString::Printf(TEXT("StationOut%d"),St.Id),IX+CW*.32f-4,RY+20,170,50,FString::Printf(TEXT("Finished racks on the station's two OUT pallets, %d per pallet. A forklift takes a pallet when it is full, when the batch ends, or when an order needs it."),FFoundrylineWeldingStationLayout::RacksPerOutPallet));
    if(Guide.StationId==St.Id&&Guide.bWeldedOrder&&Guide.Step==EFoundrylineGuideStep::ProgramPlasma)Button(FString::Printf(TEXT("StationOrder%d"),Guide.OrderId),FString::Printf(TEXT("Set for %s"),*Guide.OrderReference),CX+CW-266,RY+28,240,50,EStyle::Primary,!PC->bOperationsPending,TEXT("Set this station's batch to the racks the order still needs."));
   }
   if(!S.Machines.ContainsByPredicate([](const auto& M){return !M.bStored;})&&S.WeldingStations.IsEmpty()){Text(TEXT("No equipment on the floor yet."),CX,Top+20,20,true,Muted);Text(TEXT("Purchase equipment here, then open Planning to place your owned machines."),CX,Top+60,15,false,Muted,CW);}
   // How parts move through the shop, with live counts.
   {const float FT=Top+Row*196+8;if(FT+200<CB){Card(CX,FT,CW,CB-FT>260?250:CB-FT,TEXT("How parts move through your shop"));
    struct FStage {const TCHAR* Name;FString Value;const TCHAR* Who;int Icon;};
    const FStage Flow[]={{TEXT("Steel rack"),Lb(Guide.SteelOnHandLb),TEXT("Truck delivers, forklift stores"),2},{TEXT("Plasma cutter"),FString::Printf(TEXT("%d cutting"),Guide.PartsInProgress),TEXT("Operator cuts parts"),3},{TEXT("Finished parts"),FString::Printf(TEXT("%d parts"),Guide.FinishedParts-Guide.FinishedAtBay),TEXT("Forklift racks them"),1},{TEXT("Loading bay"),FString::Printf(TEXT("%d parts"),Guide.FinishedAtBay),TEXT("Staged for your order"),4},{TEXT("Customer"),FString::Printf(TEXT("%d pickup%s"),Guide.PickupsBooked,Guide.PickupsBooked==1?TEXT(""):TEXT("s")),TEXT("Truck delivers, you get paid"),6}};
    const float SW5=(CW-48-4*36)/5;for(int I=0;I<5;++I){const float A=CX+24+I*(SW5+36),B=FT+56;Rect(A,B,SW5,140,Rail);Border(A,B,SW5,140,Line);Icon(Flow[I].Icon,A+18,B+18,26,Graphite);Text(Flow[I].Name,A+56,B+20,15,true,Ink,SW5-70);Text(Flow[I].Value,A+18,B+60,22,true,Ink,SW5-36);Wrap(Flow[I].Who,A+18,B+96,SW5-36,12,false,Muted,2,17);
     if(I<4){Rect(A+SW5+8,B+68,18,4,Orange);Rect(A+SW5+20,B+62,4,16,Orange);Rect(A+SW5+24,B+65,3,10,Orange);}}}}
  } else {
   const float EW=(CW-22)/2,EH=(CB-Top-18)/2;
   struct FEquipmentOffer {int Kind;const TCHAR* Name;const TCHAR* Preview;const TCHAR* Description;int64 Price;};
   const FEquipmentOffer Offers[]={{1,TEXT("Plasma cutter"),TEXT("Stored1"),TEXT("Cuts bracket blanks and plates from steel sheets. Includes dedicated sheet input and four output pallet positions."),850000},{2,TEXT("Press brake"),TEXT("Stored2"),TEXT("Forms cut bracket blanks into formed brackets. Includes its connected material and pallet storage."),450000},{3,TEXT("Sheet roller"),TEXT("Stored3"),TEXT("Rolls sheet steel for larger fabrication work. Place the workspace before assigning its operator."),600000},{4,TEXT("Forklift"),TEXT("Forklift"),TEXT("Moves pallets between receiving, storage, machines and shipping. Assign a driver after placement."),120000}};
   for(int I=0;I<UE_ARRAY_COUNT(Offers);++I){const auto& Offer=Offers[I];const float A=CX+(I%2)*(EW+22),B=Top+(I/2)*(EH+18);const int Owned=Offer.Kind==4?S.UnplacedForklifts:S.Machines.FilterByPredicate([&](const auto& M){return M.Kind()==Offer.Kind&&M.bStored;}).Num();const bool Capacity=Offer.Kind==4?S.Drivers.Num()+S.UnplacedForklifts<4:S.Machines.Num()<12;const bool Affordable=C.CashCents>=Offer.Price&&!C.UnpaidCents,AtOffice=PC->IsWalking()&&PC->IsNearOfficeComputer();
    Card(A,B,EW,EH);Text(Offer.Name,A+22,B+20,22,true,Ink,EW-210);RightText(Cash(Offer.Price),A+EW-22,B+22,21,true,Orange);MenuImage(Offer.Preview,A+12,B+64,176,110);Wrap(Offer.Description,A+194,B+72,EW-218,14,false,Muted,4,20);Text(FString::Printf(TEXT("%d owned / waiting for placement"),Owned),A+22,B+EH-116,14,true,Owned?Green:Muted,EW-44);
    const float Half=(EW-56)/2;Button(FString::Printf(TEXT("BuyEquipment%d"),Offer.Kind),!AtOffice?TEXT("Use office computer"):!Capacity?TEXT("Equipment limit reached"):!Affordable?TEXT("Not enough available cash"):TEXT("Buy equipment"),A+22,B+EH-66,Half,44,EStyle::Primary,AtOffice&&Capacity&&Affordable&&!PC->bOperationsPending,TEXT("Purchase into owned equipment storage. It is not placed automatically and will not be charged again when placed."));Button(TEXT("PropertyView"),TEXT("Plan placement"),A+34+Half,B+EH-66,Half,44,EStyle::Secondary,Owned>0&&PC->IsNearOfficeComputer(),TEXT("Open Planning to choose and position equipment you already own."));
   }
  }
 }
 else if(Page==4||Page==11){
  const bool Trucks=Page==4;
  PageHeader(Trucks?TEXT("Trucks"):TEXT("Inventory"),Trucks?TEXT("Steel deliveries coming in and customer pickups going out, all through your one loading bay."):TEXT("Every pallet you own: steel sheets and finished parts, where they are and whether they're spoken for."));
  int Open=0;for(const auto& B:C.Bookings)Open+=B.Status!=EFoundrylineBookingState::Delivered;
  Text(Trucks?FString::Printf(TEXT("%d active carrier booking%s / shared loading bay"),Open,Open==1?TEXT(""):TEXT("s")):FString::Printf(TEXT("%d tracked pallets / exact quantities and reservations"),S.Pallets.Num()),CX,CY+84,15,true,Muted,CW);
  const float Top=CY+134;
  if(Trucks){
   const float BannerH=74;Card(CX,Top,CW,BannerH);Rect(CX,Top,4,BannerH,Guide.FinishedAtBay+Guide.FinishedAtBayProducts?Green:Line);
   Text((Guide.FinishedAtBay+Guide.FinishedAtBayProducts)?FString::Printf(TEXT("%d finished items are waiting at the loading bay"),(Guide.FinishedAtBay+Guide.FinishedAtBayProducts)):FString(TEXT("Nothing finished at the loading bay yet")),CX+24,Top+14,18,true,(Guide.FinishedAtBay+Guide.FinishedAtBayProducts)?Green:Ink,CW-380);
   Text(TEXT("Book a pickup on the loading tablet: tick the pallets, choose a truck that fits, confirm."),CX+24,Top+44,13,false,Muted,CW-380);
   Button(TEXT("Tablet"),TEXT("Book a pickup  >"),CX+CW-300,Top+12,276,50,(Guide.FinishedAtBay+Guide.FinishedAtBayProducts)?EStyle::Primary:EStyle::Secondary,true,TEXT("Open the loading tablet to choose pallets and book a pickup truck."));
   const float LTop=Top+BannerH+16,LW=CW*.56f,RX=CX+LW+22,RW=CW-LW-22;
   TArray<const FFoundrylineFreightBooking*> Rows;for(const auto& B:C.Bookings)Rows.Add(&B);Rows.Sort([](const auto& A,const auto& B){const bool AD=A.Status==EFoundrylineBookingState::Delivered,BD=B.Status==EFoundrylineBookingState::Delivered;return AD!=BD?!AD:AD?A.ArrivalAt>B.ArrivalAt:A.ArrivalAt<B.ArrivalAt;});
   const int Per=FMath::Max(1,int((CB-LTop-50)/80)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& B=*Rows[I];const float RY=LTop+(I-F)*80;const bool On=Booking&&B.Id==Booking->Id;const float V=Hit(FString::Printf(TEXT("Booking%d"),B.Id),CX,RY,LW,72,true);
    Rect(CX,RY,LW,72,On?SoftOrange:FMath::Lerp(Paper,Hex(TEXT("F4E8DC")),V));Border(CX,RY,LW,72,On?Orange:Line);if(On)Rect(CX,RY,4,72,Orange);else Hover(CX,RY,LW,72,V);
    Chip(B.Inbound?(B.Part?TEXT("PARTS IN"):TEXT("STEEL IN")):TEXT("PICKUP"),CX+18,RY+24,B.Inbound?Blue:Green,B.Inbound?SoftBlue:SoftGreen);int Qty=0;for(const auto& Item:B.Manifest)Qty+=Item.Quantity;
    Text(B.Inbound?FFoundrylineCommerceCatalog::InboundLabel(B):FString::Printf(TEXT("%d parts to %s"),Qty,*B.Customer),CX+120,RY+14,16,true,Ink,LW-330);Text((B.Status==EFoundrylineBookingState::Delivered?TEXT("Arrived "):TEXT("Arrives "))+FFoundrylineCommerceCatalog::Date(B.ArrivalAt),CX+120,RY+40,13,false,Muted,LW-330);
    const FString St=BookingStatus(B).ToUpper();Chip(St,CX+LW-Width(St,12,true)-38,RY+24,B.Status==EFoundrylineBookingState::Delivered?Green:B.Status==EFoundrylineBookingState::Waiting?Amber:Blue,B.Status==EFoundrylineBookingState::Delivered?SoftGreen:B.Status==EFoundrylineBookingState::Waiting?SoftOrange:SoftBlue);}
   if(Rows.IsEmpty()){Card(CX,LTop,LW,150);Text(TEXT("No trucks booked yet"),CX+24,LTop+26,20,true,Ink);Wrap(TEXT("Buying steel books a delivery truck. Booking a pickup on the loading tablet books a customer truck."),CX+24,LTop+62,LW-48,14,false,Muted,3);}
   Pager(Rows.Num(),Per,CX,CB-38,LW);
   Card(RX,LTop,RW,CB-LTop);
   if(!Booking)Wrap(TEXT("Select a truck on the left to see its details."),RX+24,LTop+28,RW-48,17,true,Muted,2);
   else {const auto& B=*Booking;Mark(B.Customer,RX+24,LTop+24,48);Text(B.Customer,RX+86,LTop+26,17,true,Ink,RW-110);Text(FString::Printf(TEXT("%s-%04d  /  %s"),B.Inbound?TEXT("PO"):TEXT("SHP"),B.Id,FFoundrylineCommerceCatalog::Carrier(B.Carrier).Vehicle),RX+86,LTop+50,13,false,Muted,RW-110);
    Text(BookingStatus(B),RX+24,LTop+100,24,true,Ink,RW-48);
    const TCHAR* Explain[]={TEXT("The truck will come at the booked time."),TEXT("The truck is driving to your shop."),TEXT("Another truck is using the loading bay; this one waits its turn."),B.Inbound?TEXT("Your forklift driver is unloading the steel onto the rack."):TEXT("Your forklift driver is loading the booked pallets."),TEXT("The truck is leaving your shop."),TEXT("On the road to the customer. Payment follows the order's terms when it arrives."),B.Inbound?TEXT("All steel from this truck has arrived."):TEXT("Delivered to the customer.")};
    float At=LTop+138;At+=Wrap(Explain[FMath::Clamp(int(B.Status),0,6)],RX+24,At,RW-48,15,false,Muted,3)+16;
    Text(TEXT("ARRIVAL"),RX+24,At,12,true,Muted);Text(FFoundrylineCommerceCatalog::Date(B.ArrivalAt),RX+24,At+20,17,true,Ink);Text(TEXT("FREIGHT FEE"),RX+RW/2,At,12,true,Muted);Text(Cash(B.OriginalFeeCents),RX+RW/2,At+20,17,true,Ink);HelpArea(TEXT("Fee"),RX+RW/2-4,At-4,RW/2-20,50,TEXT("What this truck booking cost when it was made."));At+=64;
    if(B.OrderId)Button(FString::Printf(TEXT("LinkedOrder%d"),B.OrderId),TEXT("Open this order  >"),RX+24,At,RW-48,42,EStyle::Secondary,true,TEXT("Open the customer order this pickup belongs to."));At+=56;
    if(B.Status==EFoundrylineBookingState::Booked&&C.Clock<B.DispatchAt){Rect(RX+24,At,RW-48,1,Line);At+=14;Text(TEXT("CHANGE THE TIME (COSTS HALF THE FEE)"),RX+24,At,12,true,Muted,RW-48);Text(FFoundrylineCommerceCatalog::Date(C.Clock+PC->OperationsDelay),RX+24,At+20,17,true,Ink);
     Button(TEXT("TimeMinus"),TEXT("Sooner"),RX+24,At+50,(RW-60)/2,40,EStyle::Secondary,true,TEXT("Move the new time 30 game minutes earlier."));Button(TEXT("TimePlus"),TEXT("Later"),RX+36+(RW-60)/2,At+50,(RW-60)/2,40,EStyle::Secondary,true,TEXT("Move the new time 30 game minutes later."));
     Button(TEXT("Rebook"),TEXT("Move truck to this time"),RX+24,CB-74,RW-48,52,EStyle::Primary,!PC->bOperationsPending,TEXT("Rebook this truck for the new time. Costs half of the original fee."));}}
  } else {
   // Summary by material, then every pallet.
   TMap<int32,FIntVector> ByPart;for(const auto& P:S.Pallets){if(P.Place==EFoundrylinePalletPlace::InTransit||P.Part==EFoundrylinePart::Sheet||P.Part==EFoundrylinePart::HeadacheRack)continue;auto& V=ByPart.FindOrAdd(int(P.Part));if(P.BookingId)V.Z+=P.Quantity;else {V.X+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Pickup)V.Y+=P.Quantity;}}
   const float SH2=104;float A=CX;const float SWd=(CW-3*16)/4;
   Card(A,Top,SWd,SH2);Rect(A,Top,4,SH2,Graphite);Text(TEXT("STEEL SHEET"),A+20,Top+16,12,true,Muted);Text(Lb(Guide.SteelOnHandLb),A+20,Top+38,25,true,Ink);Text(FString::Printf(TEXT("about %.1f sheets"),Guide.SteelOnHandLb/SheetLb),A+20,Top+74,13,false,Muted);HelpArea(TEXT("StockSteel"),A,Top,SWd,SH2,TEXT("All steel in the shop, including what's already on the plasma table."));A+=SWd+16;
   // Finished products (welded racks) are listed on their own, apart from cut parts.
   {const int Racks=Guide.FinishedOnOut+Guide.FinishedInStorage+Guide.FinishedAtBayProducts+Guide.FinishedOnTrucks;Card(A,Top,SWd,SH2);Rect(A,Top,4,SH2,Orange);Text(TEXT("FINISHED PRODUCTS"),A+20,Top+16,12,true,Muted,SWd-36);Text(FString::Printf(TEXT("%d headache rack%s"),Racks,Racks==1?TEXT(""):TEXT("s")),A+20,Top+38,22,true,Ink,SWd-36);
    Text(FString::Printf(TEXT("%d on OUT / %d stored / %d at bay / %d on trucks"),Guide.FinishedOnOut,Guide.FinishedInStorage,Guide.FinishedAtBayProducts,Guide.FinishedOnTrucks),A+20,Top+74,12,false,Muted,SWd-36);
    HelpArea(TEXT("StockFinished"),A,Top,SWd,SH2,TEXT("Products your welding station builds. On OUT: on the station's pallets. Stored: on a rack slot. At bay: staged for an order. On trucks: booked or on the way."));A+=SWd+16;}
   int Shown=0;for(const auto& P:ByPart){if(Shown++>=2)break;Card(A,Top,SWd,SH2);Rect(A,Top,4,SH2,Green);Text(PartName(P.Key).ToUpper(),A+20,Top+16,12,true,Muted,SWd-36);Text(FString::Printf(TEXT("%d free"),P.Value.X),A+20,Top+38,25,true,Ink);Text(FString::Printf(TEXT("%d at the bay  /  %d on trucks"),P.Value.Y,P.Value.Z),A+20,Top+74,13,false,Muted,SWd-36);HelpArea(FString::Printf(TEXT("StockPart%d"),P.Key),A,Top,SWd,SH2,TEXT("Free: finished and not yet booked on a truck. At the bay: ready for a pickup. On trucks: booked and leaving or gone."));A+=SWd+16;}
   if(ByPart.IsEmpty()&&A<CX+CW-20){Card(A,Top,CW-(A-CX),SH2);Text(TEXT("No finished parts yet"),A+20,Top+22,18,true,Ink);Text(TEXT("Parts appear here after the plasma cuts them."),A+20,Top+54,14,false,Muted);}
   const float TTop=Top+SH2+18;Card(CX,TTop,CW,CB-TTop);const float Col[]={0,.13f,.38f,.5f,.72f};const TCHAR* Names[]={TEXT("PALLET"),TEXT("CONTAINS"),TEXT("QUANTITY"),TEXT("WHERE"),TEXT("STATUS")};
   for(int I=0;I<5;++I)Text(Names[I],CX+24+CW*Col[I],TTop+18,12,true,Muted);Rect(CX+16,TTop+44,CW-32,1,Line);
   TArray<const FFoundrylineFactoryPallet*> Rows;for(const auto& P:S.Pallets)Rows.Add(&P);Rows.Sort([](const auto& A,const auto& B){return A.Part!=B.Part?A.Part<B.Part:A.Id<B.Id;});
   const int Per=FMath::Max(1,int((CB-TTop-110)/50)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& P=*Rows[I];const float RY=TTop+52+(I-F)*50;if((I-F)%2)Rect(CX+8,RY,CW-16,48,Rail);
    Text(FString::Printf(TEXT("PLT-%04d"),P.Id),CX+24,RY+14,14,true,Ink);MenuImage(*PartPreview(int(P.Part)),CX+24+CW*.13f,RY+6,52,32.5f);Text(P.Part==EFoundrylinePart::Sheet?FString(TEXT("Steel sheet")):PartName(int(P.Part)),CX+82+CW*.13f,RY+14,15,false,Ink,CW*.24f-58);
    Text(P.Part==EFoundrylinePart::Sheet?FString::Printf(TEXT("%d (%s)"),P.Quantity,*Lb(P.Quantity*P.UnitLb)):FString::FromInt(P.Quantity),CX+24+CW*.38f,RY+14,15,true,Ink);Text(PlaceName(P.Place),CX+24+CW*.5f,RY+14,15,false,Ink,CW*.21f);
    const FString St=P.BookingId?TEXT("ON A BOOKED TRUCK"):P.Reserved?TEXT("SET ASIDE"):TEXT("FREE");Chip(St,CX+24+CW*.72f,RY+12,P.BookingId?Blue:P.Reserved?Amber:Green,P.BookingId?SoftBlue:P.Reserved?SoftOrange:SoftGreen);}
   HelpArea(TEXT("StatusColumn"),CX+20+CW*.72f,TTop+12,CW*.25f,28,TEXT("Free: can be used or shipped. Set aside: reserved for a machine or order. On a booked truck: locked to that shipment."));
   if(Rows.IsEmpty()){Text(TEXT("Nothing in stock yet"),CX+24,TTop+70,20,true,Ink);Button(TEXT("Page3"),TEXT("Buy steel  >"),CX+24,TTop+110,200,44,EStyle::Primary,true,TEXT("Order steel sheets to start."));}
   Pager(Rows.Num(),Per,CX+20,CB-52,CW-40);
  }
 }
 else if(Page==7){
  TArray<FFoundrylineEmployeeRecord> Staff;for(const auto& E:Roster)if(E.CompanyId==Plant->BusinessCompanyId&&E.PlantId==Plant->BusinessPlantId)Staff.Add(E);Staff.Sort([](const auto& A,const auto& B){return A.Id>B.Id;});
  const bool HirePending=PC->OperationsHireRequest.IsValid()&&!S.HireRequests.Contains(PC->OperationsHireRequest);
  auto AssignmentName=[&](const FFoundrylineEmployeeRecord& E){if(!E.Assignment)return FString(TEXT("No job yet"));if(E.Role==EFoundrylineEmployeeRole::Operator){const auto* Machine=S.Machines.FindByPredicate([&](const auto& M){return M.Id==E.Assignment;});return Machine?FString::Printf(TEXT("%s / %03d"),Machine->Kind()==1?TEXT("Plasma cutter"):Machine->Kind()==2?TEXT("Press brake"):TEXT("Sheet roller"),Machine->Id):FString(TEXT("Machine unavailable"));}if(E.Role==EFoundrylineEmployeeRole::PartsCollector)return FString::Printf(TEXT("Assembly area %d"),E.Assignment);if(E.Role==EFoundrylineEmployeeRole::Welder){if(const auto* St=S.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;}))return FFoundrylineWeldingStationLayout::Name(St->Number)+(E.StationSlot?TEXT(" / Welder B"):TEXT(" / Welder A"));return FString::Printf(TEXT("Welding zone %d"),E.Assignment);}const int Index=S.Drivers.IndexOfByPredicate([&](const auto& D){return D.Id==E.VehicleId;});return Index>=0?FString::Printf(TEXT("Forklift %d"),Index+1):FString(TEXT("No job yet"));};
  PageHeader(TEXT("Team"),TEXT("Hiring adds a person. Then give them a job: run a machine or drive a forklift. Everyone in your co-op company shares this team."));
  // Who the starter shop needs.
  {const float RY=CY+70;float A=CX;auto Need=[&](const FString& Job,bool Ok,const FString& Who){const FString T=Job+TEXT(": ")+(Ok?Who:FString(TEXT("nobody")));const float D=Width(T,14,true)+52;Rect(A,RY,D,34,Ok?SoftGreen:SoftRed);Rect(A+12,RY+12,10,10,Ok?Green:Red);Text(T,A+32,RY+8,14,true,Ok?Green:Red);HelpArea(TEXT("Need")+Job,A,RY,D,34,Ok?FString(TEXT("Covered. This job is filled.")):FString(TEXT("Missing. Without this job filled, the starter loop stops. Pick an employee below and assign the job.")));A+=D+12;};
   Need(TEXT("Plasma operator"),Guide.bOperator,Guide.OperatorName);Need(TEXT("Forklift driver"),Guide.bForkliftDriver,Guide.DriverName);
   // Payroll: wages are paid for every game hour a person holds a job; a hire without a job is not paid.
   {const FString T=FString::Printf(TEXT("Payroll %s per game hour"),*Cash(FFoundrylinePayroll::PlantCentsPerHour(S)));const float D=Width(T,14,true)+40;Rect(A,RY,D,34,Surface);Text(T,A+20,RY+8,14,true,Ink);
    HelpArea(TEXT("Payroll"),A,RY,D,34,FString::Printf(TEXT("Wages per game hour while a person holds a job: welder %s, machine operator %s, forklift driver %s. A hire without a job is not paid. Paid every game minute from company cash; see Money."),*Cash(FFoundrylinePayroll::RateCentsPerHour(EFoundrylineEmployeeRole::Welder)),*Cash(FFoundrylinePayroll::RateCentsPerHour(EFoundrylineEmployeeRole::Operator)),*Cash(FFoundrylinePayroll::RateCentsPerHour(EFoundrylineEmployeeRole::ForkliftDriver))));}}
  const auto* EmployeeSelected=Staff.FindByPredicate([&](const auto& E){return E.Id==PC->OperationsEmployeeChoice;});
  Tabs({MakeTuple(FString(TEXT("Workspace0")),FString::Printf(TEXT("People & jobs (%d)"),Staff.Num()),PC->OperationsWorkspaceTab==0,FString(TEXT("Everyone you've hired and what they're doing."))),MakeTuple(FString(TEXT("Workspace2")),FString(TEXT("Hire")),PC->OperationsWorkspaceTab==2,FString(TEXT("Add a new person to your team."))),MakeTuple(FString(TEXT("Workspace1")),FString(TEXT("Forklift priorities")),PC->OperationsWorkspaceTab==1,FString(TEXT("Choose which forklift jobs come first.")))},CX,CY+116);
  const float Top=CY+176;
  if(PC->OperationsWorkspaceTab==0&&EmployeeSelected){
   const auto& E=*EmployeeSelected;const float LW=CW*.64f,RX=CX+LW+22,RW=CW-LW-22;const int JobRoleId=FMath::Clamp(PC->OperationsJobRole,0,4);const bool Returning=E.bMoving||(!E.Assignment&&FVector::Dist2D(E.Position,E.Home)>3);const bool CanAssign=!E.Assignment&&!Returning;
   Card(CX,Top,LW,CB-Top);Text(TEXT("Give ")+E.Label+TEXT(" a job"),CX+24,Top+22,22,true,Ink,LW-250);Button(EmployeeView?TEXT("Close"):TEXT("TeamRoster"),EmployeeView?TEXT("Close employee"):TEXT("< Back to team"),CX+LW-204,Top+18,180,40,EStyle::Secondary,true,TEXT("Back to the list of people."));
   Text(TEXT("1   CHOOSE THE KIND OF JOB"),CX+24,Top+76,12,true,Muted);const TCHAR* JobTitles[]={TEXT("Machine operator"),TEXT("Forklift driver"),TEXT("Welder"),TEXT("Parts collector")};const int JobValues[]={0,2,1,4};
   const TCHAR* JobHelp[]={TEXT("Runs a machine such as the plasma cutter. Must walk to the machine before it can work."),TEXT("Drives a forklift: unloads steel, feeds the plasma and stacks finished parts."),TEXT("Welds at a welding station (Welder A side) or a product work zone."),TEXT("Collects mixed parts by cart for an assembly area.")};
   for(int J=0;J<4;++J)Button(FString::Printf(TEXT("JobRole%d"),JobValues[J]),JobTitles[J],CX+24+J*(LW-48+12)/4,Top+98,(LW-48-36)/4,46,EStyle::Secondary,true,JobHelp[J],JobRoleId==JobValues[J]);
   Text(TEXT("2   CHOOSE WHERE THEY WORK"),CX+24,Top+170,12,true,Muted);
   struct FJobOption {int Id;FString Title,Detail;bool Occupied=false,Ready=true;int Slot=0;};TArray<FJobOption> Options;
   if(JobRoleId==0){for(const auto& M:S.Machines){if(M.bStored)continue;const auto* Assigned=Staff.FindByPredicate([&](const auto& Other){return Other.Id!=E.Id&&int(Other.Role)==JobRoleId&&Other.Assignment==M.Id;});Options.Add({M.Id,M.Kind()==1?TEXT("Plasma cutter"):M.Kind()==2?TEXT("Press brake"):TEXT("Sheet roller"),Assigned?FString(TEXT("Already run by "))+Assigned->Label:FString(TEXT("Free: no operator yet")),Assigned!=nullptr,true});}}
   else if(JobRoleId==2){for(int V=0;V<S.Drivers.Num();++V){const auto& D=S.Drivers[V];const bool Parked=!D.Job&&FVector::Dist2D(D.Position,D.Parking)<20;Options.Add({D.Id,FString::Printf(TEXT("Forklift %d"),V+1),D.EmployeeId?TEXT("Already has a driver"):Parked?TEXT("Free and parked"):TEXT("Driving back to its parking spot"),D.EmployeeId!=0&&D.EmployeeId!=E.Id,Parked});}}
   else if(JobRoleId==4){for(const auto& St:S.WeldingStations)Options.Add({St.Id,FFoundrylineWeldingStationLayout::Name(St.Number),TEXT("Automatic cart collection / Assembly Supply Bin"),false,true});}
   else{for(const auto& St:S.WeldingStations)for(int Slot=0;Slot<2;++Slot){const auto* Assigned=Staff.FindByPredicate([&](const auto& Other){return Other.Id!=E.Id&&Other.Role==EFoundrylineEmployeeRole::Welder&&Other.Assignment==St.Id&&Other.StationSlot==Slot;});
     Options.Add({St.Id,FFoundrylineWeldingStationLayout::Name(St.Number)+(Slot?TEXT(" / Welder B"):TEXT(" / Welder A")),Assigned?FString(TEXT("Already worked by "))+Assigned->Label:FString(Slot?TEXT("Second welder: 170% with both at work"):TEXT("Headache-rack assembly at the bench")),Assigned!=nullptr,true,Slot});}
    for(const auto& Z:S.Zones){const bool Occupied=Staff.ContainsByPredicate([&](const auto& Other){return Other.Id!=E.Id&&Other.Role==EFoundrylineEmployeeRole::Welder&&Other.Assignment==Z.Id;});Options.Add({Z.Id,FString::Printf(TEXT("Welding zone %d"),Z.Id),TEXT("Product assembly station"),Occupied,true});}}
   const int Per=FMath::Max(1,int((CB-Top-260)/80)),F=FMath::Clamp(PC->OperationsJobPage,0,FMath::Max(0,(Options.Num()-1)/Per))*Per;
   for(int I=F;I<FMath::Min(F+Per,Options.Num());++I){const auto& Option=Options[I];const float RY=Top+194+(I-F)*80;
    const bool Current=int(E.Role)==JobRoleId&&E.Assignment==Option.Id&&E.StationSlot==Option.Slot;
    const FString Detail=Current?FString(TEXT("Currently assigned to "))+E.Label:Option.Detail;
    Rect(CX+24,RY,LW-48,72,(I-F)%2?Rail:Paper);Border(CX+24,RY,LW-48,72,Line);Text(Option.Title,CX+42,RY+12,18,true,Ink,LW-280);Text(Detail,CX+42,RY+42,13,false,Current?Green:Option.Occupied?Amber:Muted,LW-280);
    const FString Label=Current?TEXT("Current job"):Option.Occupied?TEXT("Taken"):!Option.Ready?TEXT("Not ready"):TEXT("Assign here");
    Button(Option.Slot?FString::Printf(TEXT("AssignEmployee%d_%d_%d_%d"),E.Id,JobRoleId,Option.Id,Option.Slot):FString::Printf(TEXT("AssignEmployee%d_%d_%d"),E.Id,JobRoleId,Option.Id),Label,CX+LW-210,RY+14,170,44,EStyle::Primary,CanAssign&&!Option.Occupied&&Option.Ready&&!PC->bOperationsPending,Current?TEXT("This is their job now."):!CanAssign?TEXT("Release their current job first (on the right), and let them reach the waiting area."):Option.Occupied?TEXT("Someone else already works here."):TEXT("Give them this job. They'll walk over and start."));}
   if(Options.IsEmpty()){Text(JobRoleId==1?TEXT("No welding stations placed"):TEXT("No equipment installed"),CX+42,Top+210,20,true,Ink,LW-84);Wrap(JobRoleId==1?TEXT("Place a welding station first (Production > Welding station)."):JobRoleId==2?TEXT("Buy a forklift first (Production > Buy equipment). Hiring gives you a person only."):TEXT("Install a machine first."),CX+42,Top+246,LW-84,14,false,Muted,2);}
   if(Options.Num()>Per){Button(TEXT("JobPrevious"),TEXT("< Back"),CX+LW-250,CB-56,100,36,EStyle::Secondary,F>0);Button(TEXT("JobNext"),TEXT("More >"),CX+LW-140,CB-56,100,36,EStyle::Secondary,F+Per<Options.Num());}
   Card(RX,Top,RW,CB-Top,TEXT("This person"));Text(E.Label,RX+24,Top+50,21,true,Ink,RW-48);Text(AssignmentName(E)+(E.Assignment?FString::Printf(TEXT("  /  %s per game hour"),*Cash(FFoundrylinePayroll::RateCentsPerHour(E.Role))):FString()),RX+24,Top+84,16,true,E.Assignment?Ink:Orange,RW-48);Chip(FFoundrylineEmployeeRoster::ActivityName(E.Activity),RX+24,Top+118,E.bWorking?Green:E.bMoving?Blue:Muted,E.bWorking?SoftGreen:E.bMoving?SoftBlue:Surface);Wrap(E.CurrentJob,RX+24,Top+156,RW-48,13,false,Muted,2);Rect(RX+24,Top+206,RW-48,1,Line);
   if(E.Assignment){Wrap(TEXT("To change jobs, release this one first. They keep their name and come back to the waiting area."),RX+24,Top+224,RW-48,14,false,Ink,3);bool Safe=true;
    if(E.Role==EFoundrylineEmployeeRole::PartsCollector){const auto* Collector=S.Collectors.FindByPredicate([&](const auto& V){return V.Id==E.Id;});Safe=Collector&&!Collector->ReleaseRequested;}
    if(E.Role==EFoundrylineEmployeeRole::ForkliftDriver){const auto* D=S.Drivers.FindByPredicate([&](const auto& V){return V.Id==E.VehicleId;});Safe=D&&!D->Job&&FVector::Dist2D(D->Position,D->Parking)<20;}
    if(E.Role==EFoundrylineEmployeeRole::Operator){const auto* M=S.Machines.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;});Safe=M&&(M->Remaining<=0||!M->Enabled);}
    // A station keeps its unit's progress when its welder leaves, so release is always safe there.
    if(E.Role==EFoundrylineEmployeeRole::Welder){const auto* Z=S.Zones.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;});Safe=S.WeldingStations.ContainsByPredicate([&](const auto& V){return V.Id==E.Assignment;})||Z&&(!S.Running||Z->ActiveStep<0);}
    if(E.Role==EFoundrylineEmployeeRole::PartsCollector){for(int K=0;K<3;++K)Button(FString::Printf(TEXT("SupplyBin%d_%d"),E.Assignment,4+K*2),FString::Printf(TEXT("%d bins"),4+K*2),RX+24+K*(RW-48)/3,CB-190,(RW-60)/3,36,EStyle::Secondary,!S.Running,TEXT("Pause and finish incoming deliveries before upgrading."));}
    Button(FString::Printf(TEXT("AssignEmployee%d_3_0"),E.Id),TEXT("Release from this job"),RX+24,CB-130,RW-48,48,EStyle::Danger,Safe&&!PC->bOperationsPending,Safe?TEXT("Take them off this job. Work there stops until someone else is assigned."):TEXT("Wait until the current part is finished (or pause the machine) and the forklift is parked."));
    if(!Safe)Wrap(TEXT("Finish or pause the current work and park the forklift first."),RX+24,CB-72,RW-48,13,false,Amber,2);}
   else{Text(Returning?TEXT("Walking to the waiting area"):TEXT("Ready for a job"),RX+24,Top+224,17,true,Returning?Muted:Green,RW-48);Wrap(Returning?TEXT("You can assign a new job once they arrive."):TEXT("Choose a kind of job, then where they work."),RX+24,Top+256,RW-48,14,false,Muted,3);}
  }else if(PC->OperationsWorkspaceTab==0){
   Card(CX,Top,CW,CB-Top);const float C1=CX+CW*.28f,C2=CX+CW*.52f,C3=CX+CW*.80f;Text(TEXT("PERSON"),CX+24,Top+18,12,true,Muted);Text(TEXT("JOB"),C1,Top+18,12,true,Muted);Text(TEXT("DOING NOW"),C2,Top+18,12,true,Muted);Rect(CX+16,Top+44,CW-32,1,Line);
   const int Per=FMath::Max(1,int((CB-Top-110)/70)),F=First(Staff.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Staff.Num());++I){const auto& E=Staff[I];const float RY=Top+52+(I-F)*70;if((I-F)%2)Rect(CX+8,RY,CW-16,64,Rail);
    Icon(5,CX+24,RY+18,26,E.Assignment?Graphite:Orange);Text(E.Label,CX+62,RY+12,16,true,Ink,CW*.28f-70);Text(E.Assignment?FFoundrylineEmployeeRoster::RoleName(E.Role):TEXT("Not working yet"),CX+62,RY+38,13,false,Muted,CW*.28f-70);
    Text((E.Assignment?FString::Printf(TEXT("%s/h  /  "),*Cash(FFoundrylinePayroll::RateCentsPerHour(E.Role))):FString(TEXT("Unpaid until assigned  /  ")))+AssignmentName(E),C1,RY+12,16,true,E.Assignment?Ink:Orange,CW*.24f-14);Text(E.CurrentJob,C1,RY+38,12,false,Muted,CW*.24f-14);
    Chip(FFoundrylineEmployeeRoster::ActivityName(E.Activity),C2,RY+10,E.bWorking?Green:E.bMoving?Blue:Muted,E.bWorking?SoftGreen:E.bMoving?SoftBlue:Surface);Text(FFoundrylineEmployeeRoster::WaitName(E.Wait),C2,RY+40,12,false,Muted,CW*.27f);
    Button(FString::Printf(TEXT("SelectEmployee%d"),E.Id),E.Assignment?TEXT("Change job"):TEXT("Give a job"),C3,RY+12,CW*.2f-28,42,E.Assignment?EStyle::Secondary:EStyle::Primary,true,E.Assignment?TEXT("See or change this person's job."):TEXT("This person has nothing to do. Give them a job."));}
   if(Staff.IsEmpty()){Text(TEXT("No one hired yet"),CX+24,Top+70,20,true,Ink);Button(TEXT("Workspace2"),TEXT("Hire someone  >"),CX+24,Top+110,220,44,EStyle::Primary,true,TEXT("Hire your first employee."));}
   Pager(Staff.Num(),Per,CX+20,CB-52,CW-40);
  }else if(PC->OperationsWorkspaceTab==2){
   const float LW=CW*.58f,RX=CX+LW+22,RW=CW-LW-22;Card(CX,Top,LW,CB-Top);Icon(5,CX+28,Top+30,46,Orange);Text(TEXT("Hire a new person"),CX+92,Top+30,26,true,Ink,LW-120);Text(TEXT("They arrive in the waiting area, ready for a job."),CX+92,Top+66,15,false,Muted,LW-120);
   float At=Top+124;auto Step=[&](int N,const FString& T){Rect(CX+28,At,28,28,Surface);Text(FString::FromInt(N),CX+37,At+5,13,true,Ink);Text(T,CX+70,At+4,15,false,Ink,LW-100);At+=42;};
   Step(1,TEXT("Hire a person here."));Step(2,TEXT("Open People & jobs and choose them."));Step(3,TEXT("Pick a job and where they work."));
   Text(FString::Printf(TEXT("%d of 12 people in your business"),Staff.Num()),CX+28,At+12,15,true,Muted);
   Button(TEXT("HirePerson"),HirePending?TEXT("Hiring..."):Staff.Num()>=12?TEXT("Team is full (12)"):TEXT("Hire a person  /  free"),CX+28,CB-138,LW-56,56,EStyle::Primary,!HirePending&&Staff.Num()<12,TEXT("Add a randomly named person. Hiring and wages are free in this version."));
   Text(TEXT("Hiring is free for now. Machines and forklifts are bought separately."),CX+28,CB-62,13,false,Muted,LW-56);
   Card(RX,Top,RW,CB-Top,TEXT("Newest team members"));for(int I=0;I<FMath::Min(int((CB-Top-60)/70),Staff.Num());++I){const auto& E=Staff[I];const float RY=Top+48+I*70;if(I%2)Rect(RX+10,RY,RW-20,64,Rail);Text(E.Label,RX+24,RY+10,15,true,Ink,RW-210);Text(AssignmentName(E),RX+24,RY+36,13,false,E.Assignment?Muted:Orange,RW-210);Button(FString::Printf(TEXT("SelectEmployee%d"),E.Id),E.Assignment?TEXT("Change"):TEXT("Give a job"),RX+RW-164,RY+12,140,40,E.Assignment?EStyle::Secondary:EStyle::Primary,true,TEXT("Open this person's job."));}
  }else{
   Card(CX,Top,CW,CB-Top);Wrap(TEXT("Check a priority for each task: 1 first, then 2, then 3. Click the checked number to turn a task off. One driver can do all three."),CX+24,Top+20,CW-48,15,false,Muted,2);
   const TCHAR* Tasks[]={TEXT("Receiving"),TEXT("Production"),TEXT("Shipping")};
   for(int I=0;I<S.Drivers.Num()&&I<4;++I){const auto& D=S.Drivers[I];const float RY=Top+76+I*134;Rect(CX+20,RY,CW-40,120,Rail);Border(CX+20,RY,CW-40,120,Line);Icon(4,CX+40,RY+20,28,Graphite);Text(FString::Printf(TEXT("Forklift %d"),I+1),CX+80,RY+20,18,true,Ink);Text(D.EmployeeId?TEXT("Driver: ")+D.DisplayName:FString(TEXT("No driver")),CX+220,RY+22,15,false,D.EmployeeId?Muted:Red,CW*.4f);
    for(int J=0;J<3;++J){const int Rank=D.Priorities.IsValidIndex(J)?D.Priorities[J]:J;const float ColW=(CW-100)/3,AtX=CX+40+ColW*J+J*10;Text(Tasks[J],AtX,RY+53,13,true,Rank<0?Muted:Ink);for(int K=0;K<3;++K)Button(FString::Printf(TEXT("PrioritySet%d_%d_%d"),D.Id,J,K),FString::Printf(TEXT("%s %d"),Rank==K?TEXT("[x]"):TEXT("[ ]"),K+1),AtX+K*(ColW-10)/3,RY+75,(ColW-10)/3-4,34,EStyle::Secondary,!PC->bOperationsPending,TEXT("Choose priority 1, 2 or 3. Click the checked priority to disable this task. Changes apply after the current trip."),Rank==K);}}
   if(S.Drivers.IsEmpty())Text(TEXT("No forklifts owned."),CX+24,Top+90,18,true,Muted);
  }
 }
 else if(Page==5){
  PageHeader(TEXT("Money"),TEXT("Where your cash came from and where it went. Customers pay according to each order's terms."));
  Tabs({MakeTuple(FString(TEXT("Workspace0")),FString(TEXT("Overview")),PC->OperationsWorkspaceTab==0,FString(TEXT("Totals and every recent movement of money."))),MakeTuple(FString(TEXT("Workspace1")),FString(TEXT("Customer payments")),PC->OperationsWorkspaceTab==1,FString(TEXT("What each customer has paid.")))},CX,CY+74);
  const float Top=CY+134;
  if(PC->OperationsWorkspaceTab==0){
   struct FSum {const TCHAR* Label;int64 Value;const TCHAR* Help;FLinearColor Col;};
   const FSum Sums[]={{TEXT("Cash"),C.CashCents,TEXT("Money you can spend right now."),Orange},{TEXT("Earned from deliveries"),C.EarnedRevenueCents,TEXT("Value of parts customers have received. Upfront payments count once the parts arrive."),Green},
    {TEXT("Total spent"),C.CostCents,TEXT("Everything spent: steel, trucks, equipment and machine running costs."),Graphite},{TEXT("Unpaid bills"),C.UnpaidCents,TEXT("Running costs you couldn't pay when cash ran out. They are paid automatically from the next money that comes in."),C.UnpaidCents?Red:Graphite},
    {TEXT("Owed to you"),Guide.OwedToYouCents,TEXT("What customers will still pay on accepted orders, according to their terms."),Blue},{TEXT("Paid upfront, parts owed"),VM.Summary.CustomerAdvancesCents,TEXT("Customers who paid in advance and are still waiting for their parts."),Amber},
    {TEXT("Payroll per game hour"),Guide.PayrollCentsPerHour,TEXT("Wages paid every game hour to everyone holding a job (welder, machine operator, forklift driver). Shown as Wages lines below."),Graphite}};
   const float TW=(CW-3*14)/4;for(int I=0;I<4;++I){const float A=CX+I*(TW+14);Card(A,Top,TW,104);Rect(A,Top,4,104,Sums[I].Col);Text(FString(Sums[I].Label).ToUpper(),A+18,Top+16,11,true,Muted,TW-30);Text(I==6?Cash(Sums[I].Value)+TEXT("/h"):Cash(Sums[I].Value),A+18,Top+40,21,true,I==3&&Sums[I].Value?Red:Ink,TW-30);HelpArea(FString::Printf(TEXT("Sum%d"),I),A,Top,TW,104,Sums[I].Help);}
   Text(TEXT("Owed to you: ")+Cash(Guide.OwedToYouCents)+TEXT("    /    Customer advances: ")+Cash(VM.Summary.CustomerAdvancesCents)+TEXT("    /    Payroll: ")+Cash(Guide.PayrollCentsPerHour)+TEXT(" per game hour"),CX+4,Top+124,14,false,Muted,CW-8);const float LTop=Top+166;Card(CX,LTop,CW,CB-LTop,TEXT("Money in and out, newest first"));
   TArray<const FFoundrylineERPLedgerEntry*> Rows;for(const auto& V:VM.Ledger)Rows.Add(&V);Rows.Sort([](const auto& A,const auto& B){return A.At==B.At?A.SourceIndex>B.SourceIndex:A.At>B.At;});
   Text(TEXT("WHEN"),CX+24,LTop+48,12,true,Muted);Text(TEXT("WHAT"),CX+CW*.2f,LTop+48,12,true,Muted);RightText(TEXT("AMOUNT"),CX+CW-24,LTop+48,12,true,Muted);Rect(CX+16,LTop+72,CW-32,1,Line);
   const int Per=FMath::Max(1,int((CB-LTop-140)/46)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& V=*Rows[I];const float RY=LTop+80+(I-F)*46;if((I-F)%2)Rect(CX+8,RY,CW-16,44,Rail);Text(FFoundrylineCommerceCatalog::Date(V.At),CX+24,RY+13,14,false,Muted);Text(V.Label,CX+CW*.2f,RY+12,15,true,Ink,CW*.55f);RightText((V.MovementCents>0?TEXT("+"):TEXT(""))+Cash(V.MovementCents),CX+CW-24,RY+11,16,true,V.MovementCents>=0?Green:Ink);}
   if(Rows.IsEmpty())Text(TEXT("No money movements yet."),CX+24,LTop+90,16,false,Muted);
   Pager(Rows.Num(),Per,CX+20,CB-52,CW-40);
  } else {
   const float LW=CW*.5f,RX=CX+LW+22,RW=CW-LW-22;TArray<const FFoundrylineCustomerOrder*> Rows;for(const auto& O:C.Orders)if(O.Status!=EFoundrylineOrderState::Offered)Rows.Add(&O);Rows.Sort([](const auto& A,const auto& B){return A.Id>B.Id;});
   const int Per=FMath::Max(1,int((CB-Top-50)/88)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& O=*Rows[I];const float RY=Top+(I-F)*88;const bool On=O.Id==PC->OperationsOrder;const float V=Hit(FString::Printf(TEXT("Order%d"),O.Id),CX,RY,LW,80,true);Rect(CX,RY,LW,80,On?SoftOrange:FMath::Lerp(Paper,Hex(TEXT("F4E8DC")),V));Border(CX,RY,LW,80,On?Orange:Line);if(On)Rect(CX,RY,4,80,Orange);else Hover(CX,RY,LW,80,V);
    Mark(O.Customer,CX+16,RY+18,44);Text(O.Customer,CX+74,RY+14,16,true,Ink,LW-260);Text(O.Reference+TEXT("  /  ")+Terms(O.Payment),CX+74,RY+42,13,false,Muted,LW-260);RightText(Cash(O.PaidCents),CX+LW-18,RY+14,18,true,O.PaidCents?Green:Muted);RightText(TEXT("of ")+Cash(O.Quantity*O.UnitPriceCents),CX+LW-18,RY+44,13,false,Muted);}
   if(Rows.IsEmpty()){Card(CX,Top,LW,120);Wrap(TEXT("Accepted orders and their payments will appear here."),CX+24,Top+30,LW-48,16,false,Muted,2);}
   Pager(Rows.Num(),Per,CX,CB-38,LW);
   Card(RX,Top,RW,CB-Top);const auto* O=Selected&&Selected->Status!=EFoundrylineOrderState::Offered?Selected:nullptr;
   if(!O)Text(TEXT("Select an order on the left"),RX+24,Top+28,18,true,Muted,RW-48);
   else {Mark(O->Customer,RX+24,Top+24,48);Text(O->Customer,RX+86,Top+26,17,true,Ink,RW-110);Text(O->Reference,RX+86,Top+50,13,false,Muted,RW-110);
    Text(TEXT("PAID SO FAR"),RX+24,Top+100,12,true,Muted);Text(Cash(O->PaidCents),RX+24,Top+120,32,true,O->PaidCents?Green:Ink);Text(TEXT("of ")+Cash(O->Quantity*O->UnitPriceCents)+TEXT("  /  ")+Terms(O->Payment),RX+24,Top+164,14,false,Muted,RW-48);
    Bar(RX+24,Top+196,RW-48,float(O->PaidCents)/FMath::Max<int64>(1,O->Quantity*O->UnitPriceCents),Green,10);
    Text(FString::Printf(TEXT("%d of %d parts delivered"),O->Delivered,O->Quantity),RX+24,Top+218,14,false,Ink);Rect(RX+24,Top+254,RW-48,1,Line);
    Text(TEXT("PAYMENTS RECEIVED"),RX+24,Top+272,12,true,Muted);int N=0;for(const auto& D:VM.Documents)if(D.OrderId==O->Id&&D.Kind==EFoundrylineERPDocumentKind::CustomerPayment){const float RY=Top+298+N++*46;if(RY>CB-60)break;Rect(RX+24,RY,RW-48,40,Rail);Text(D.bHasRecordedAt?FFoundrylineCommerceCatalog::Date(D.RecordedAt):FString(TEXT("Payment")),RX+38,RY+11,14,false,Muted);RightText(TEXT("+")+Cash(D.AmountCents),RX+RW-38,RY+10,16,true,Green);}
    if(!N)Wrap(O->Payment==EFoundrylinePayment::OnCompletion?TEXT("No payment yet. This customer pays once every part has been delivered."):TEXT("No payment yet. Payment arrives when parts are delivered."),RX+24,Top+300,RW-48,14,false,Muted,3);}
  }
 }

 PC->CurrentMouseCursor=bClickableHover?EMouseCursor::Hand:EMouseCursor::Default;
 // Hover help popup, drawn last so it sits above everything.
 if(const FString* Help=HelpTexts.Find(HoveredBox)){float MX=0,MY=0;PC->GetMousePosition(MX,MY);MX/=Scale;MY/=Scale;const float TW=380;
  TArray<FString> Words;Help->ParseIntoArray(Words,TEXT(" "));int Lines=1;FString Current;for(const auto& Wd:Words){const FString Next=Current.IsEmpty()?Wd:Current+TEXT(" ")+Wd;if(!Current.IsEmpty()&&Width(Next,14,false)>TW-28){++Lines;Current=Wd;}else Current=Next;}
  const float TH=FMath::Min(6,Lines)*20+24;float A=MX+18,B=MY+24;if(A+TW>SW-8)A=MX-TW-12;if(B+TH>SH-8)B=MY-TH-12;Rect(A+3,B+3,TW,TH,FLinearColor(0,0,0,.18f));Rect(A,B,TW,TH,Dark);Rect(A,B,3,TH,Orange);Wrap(*Help,A+14,B+12,TW-28,14,false,Paper,6,20);}
 return true;
}
