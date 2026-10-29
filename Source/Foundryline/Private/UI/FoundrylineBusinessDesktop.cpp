#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineERPViewModel.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

// Office computer: one obvious next step, seven plain-language sections and hover help on every number and button.
// Presentation only. Every business change still goes through the existing validated owning-controller commands;
// selection, tabs and help are per-player local state, so co-op players browse independently.
bool AFoundrylineBuildHUD::DrawBusinessDesktop(AFoundrylinePlayerController* PC)
{
 auto* Plant=PC->FactoryPlant();if(!Plant)return false;
 const auto& S=Plant->Snapshot;const auto& C=S.Commerce;
 const auto VM=FFoundrylineERPViewModel::Build(C,FFoundrylineERPViewModel::CompanyName(GetWorld(),Plant->BusinessCompanyId));
 const auto Guide=FFoundrylineERPViewModel::Guide(S);
 auto Hex=[](const TCHAR* V){return FLinearColor(FColor::FromHex(V));};
 const auto Ink=Hex(TEXT("1F2A33")),Muted=Hex(TEXT("5B6873")),Faint=Hex(TEXT("8D99A3")),Line=Hex(TEXT("DCE1E5")),Paper=Hex(TEXT("FFFFFF")),Surface=Hex(TEXT("F1F3F5")),Rail=Hex(TEXT("F7F8F9")),
  Orange=Hex(TEXT("EA762B")),SoftOrange=Hex(TEXT("FFF1E6")),Green=Hex(TEXT("2E7D57")),SoftGreen=Hex(TEXT("E8F4ED")),Blue=Hex(TEXT("2F6B8F")),SoftBlue=Hex(TEXT("E9F1F7")),
  Red=Hex(TEXT("B23B36")),SoftRed=Hex(TEXT("FBEBEA")),Amber=Hex(TEXT("A86A12")),Dark=Hex(TEXT("1F2327")),Graphite=Hex(TEXT("4B5258"));
 const float Scale=FMath::Min(Canvas->ClipY/1040.f,Canvas->ClipX/1740.f),SW=Canvas->ClipX/Scale,SH=Canvas->ClipY/Scale;
 const float X=20,Y=(SH-1000)/2,W=SW-40,H=1000,NavW=236,HeadH=72,FootH=40;
 const float CX=X+NavW+28,CY=Y+HeadH+22,CW=W-NavW-56,CB=Y+H-FootH-18;
 int Page=PC->OperationsPage;if(Page==1)Page=0;if(Page==8)Page=4;
 HelpTexts.Reset();
 // UE5.8 Canvas requires a UFont cache selector even with an explicit Slate typeface.
 if(!ERPFont){ERPFont=NewObject<UFont>(this,TEXT("ERP_RuntimeCanvasFont"));ERPFont->FontCacheType=EFontCacheType::Runtime;}
 const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
 auto FontOf=[&](int Size,bool Bold){return FCoreStyle::GetDefaultFontStyle(Bold?TEXT("Bold"):TEXT("Regular"),FMath::Max(8,FMath::RoundToInt(Size*Scale)));};
 auto Width=[&](const FString& T,int Size,bool Bold){return float(Measure->Measure(T,FontOf(Size,Bold)).X)/Scale;};
 auto Rect=[&](float A,float B,float D,float E,FLinearColor Col){DrawRect(Col,A*Scale,B*Scale,D*Scale,E*Scale);};
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
  const float V=Hit(Key,A,B,D,E,Enabled,Help);const int Size=E>=48?17:E>=40?15:14;const float Lift=Enabled?-2.f*V:0.f;
  auto Label_=[&](FLinearColor Col,float Pad){Text(Label,A+(D-FMath::Min(D-Pad,Width(Label,Size,true)))/2,B+Lift+(E-Size*1.25f)/2,Size,true,Col,D-Pad);};
  if(Style==EStyle::Primary){if(!Enabled){Rect(A,B,D,E,Hex(TEXT("E4E7EA")));Label_(Faint,24);return;}Rect(A,B+E-3,D,3+2*V,Hex(TEXT("B5541A")));Rect(A,B+Lift,D,E,FMath::Lerp(FMath::Lerp(Orange,Hex(TEXT("F28A45")),V),Hex(TEXT("C2561A")),Press));if(V>.02f)Rect(A,B+Lift,D,2,FLinearColor(1,1,1,.35f*V));Label_(Paper,24);return;}
  if(Style==EStyle::Tab){if(Selected){Rect(A,B,D,E,Paper);Rect(A,B+E-3,D,3,Orange);}else if(V>.01f){Rect(A,B,D,E,FMath::Lerp(Surface,Paper,V));Rect(A+D*.5f*(1-V),B+E-3,D*V,3,Hex(TEXT("8FB2C9")));}Text(Label,A+(D-FMath::Min(D-20,Width(Label,Size,true)))/2,B+(E-Size*1.25f)/2,Size,true,Selected||V>.5f?Ink:Muted,D-20);return;}
  if(!Enabled){Rect(A,B,D,E,Surface);Border(A,B,D,E,Line);Label_(Faint,20);return;}
  const FLinearColor Base=Style==EStyle::Ghost?Surface:Paper;Rect(A,B,D,E,Selected?SoftOrange:FMath::Lerp(Base,Hex(TEXT("EEF4F8")),V));Border(A,B,D,E,Selected?Orange:Line);Hover(A,B,D,E,V);
  Label_(Style==EStyle::Danger?Red:V>.5f?Blue:Ink,20);
 }; auto Card=[&](float A,float B,float D,float E,const FString& Title=FString(),FLinearColor Accent=FLinearColor::Transparent){Rect(A,B,D,E,Paper);Border(A,B,D,E,Line);if(Accent.A>0)Rect(A,B,D,4,Accent);if(!Title.IsEmpty())Text(Title.ToUpper(),A+20,B+18,12,true,Muted,D-40);};
 auto Chip=[&](const FString& T,float A,float B,FLinearColor Col,FLinearColor Back){const float D=Width(T,12,true)+20;Rect(A,B,D,24,Back);Text(T,A+10,B+4,12,true,Col);return D;};
 auto Bar=[&](float A,float B,float D,float Fraction,FLinearColor Col,float E=8){Rect(A,B,D,E,Surface);Rect(A,B,D*FMath::Clamp(Fraction,0.f,1.f),E,Col);};
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
  const TCHAR* Names[]={TEXT("Hartwell Fabrication"),TEXT("Northfield Repairs"),TEXT("Red Cedar Equipment"),TEXT("Mason Agricultural"),TEXT("Alder Workshop"),TEXT("Union Conveyor Co."),TEXT("Foundry Steel Supply"),TEXT("Swift Van Couriers"),TEXT("Foundry Lane Haulage"),TEXT("County Freight"),TEXT("Interstate Pallet Lines")};int K=0;for(int I=0;I<11;++I)if(Name==Names[I])K=I;
  const FLinearColor Colors[]={Hex(TEXT("356176")),Hex(TEXT("697747")),Hex(TEXT("A24C37")),Hex(TEXT("716080")),Hex(TEXT("34766E"))};Rect(A,B,Size,Size,Colors[K%5]);
  auto M=[&](float U,float V,float R,float T){Rect(A+U*Size,B+V*Size,R*Size,T*Size,Paper);};
  if(K==0){M(.2f,.2f,.15f,.6f);M(.65f,.2f,.15f,.6f);M(.35f,.43f,.3f,.14f);}else if(K==1){for(int I=0;I<3;++I)M(.2f,.23f+I*.22f,.6f-I*.15f,.13f);}else if(K==2){M(.2f,.2f,.15f,.6f);M(.35f,.2f,.45f,.13f);M(.35f,.45f,.3f,.13f);M(.35f,.67f,.45f,.13f);}else if(K==3){for(int I=0;I<3;++I)M(.2f+I*.24f,.2f+I*.15f,.13f,.6f-I*.15f);}else if(K==4){M(.2f,.2f,.6f,.13f);M(.44f,.33f,.13f,.47f);M(.2f,.67f,.24f,.13f);}else if(K==5){M(.2f,.2f,.13f,.6f);M(.67f,.2f,.13f,.6f);M(.2f,.67f,.6f,.13f);}else if(K==6){M(.2f,.2f,.6f,.13f);M(.2f,.33f,.13f,.47f);M(.2f,.45f,.42f,.13f);}else if(K==7){for(int I=0;I<3;++I)M(.2f,.2f+I*.23f,.17f,.13f);M(.45f,.2f,.35f,.59f);}else if(K==8){M(.2f,.2f,.15f,.6f);M(.35f,.65f,.45f,.15f);M(.48f,.2f,.15f,.32f);}else if(K==9){M(.2f,.2f,.6f,.13f);M(.2f,.33f,.13f,.34f);M(.2f,.67f,.6f,.13f);}else for(int I=0;I<3;++I)M(.2f+I*.23f,.2f,.13f,.6f);
 };
 auto Pager=[&](int Count,int Rows,float A,float B,float D){if(Count<=Rows)return;const int P=FMath::Clamp(PC->OperationsListPage,0,FMath::Max(0,(Count-1)/Rows));Text(FString::Printf(TEXT("Showing %d-%d of %d"),P*Rows+1,FMath::Min((P+1)*Rows,Count),Count),A,B+9,13,false,Muted);Button(TEXT("Previous"),TEXT("< Back"),A+D-210,B,100,36,EStyle::Secondary,P>0,TEXT("Show the previous page. The mouse wheel also scrolls."));Button(TEXT("Next"),TEXT("More >"),A+D-100,B,100,36,EStyle::Secondary,(P+1)*Rows<Count,TEXT("Show the next page. The mouse wheel also scrolls."));};
 auto First=[&](int Count,int Rows){return FMath::Clamp(PC->OperationsListPage,0,FMath::Max(0,(Count-1)/Rows))*Rows;};
 auto Tabs=[&](TArray<TTuple<FString,FString,bool,FString>> Items,float A,float B){float At=A;Rect(A,B+43,CW,1,Line);for(const auto& T:Items){const float D=Width(T.Get<1>(),15,true)+44;Button(T.Get<0>(),T.Get<1>(),At,B,D,44,EStyle::Tab,true,T.Get<3>(),T.Get<2>());At+=D+4;}};
 auto PageHeader=[&](const FString& Title,const FString& Sub){Text(Title,CX,CY,28,true,Ink,CW);Text(Sub,CX,CY+41,15,false,Muted,CW);};
 auto PartName=[](int32 Part){return FString(FFoundrylineFactoryCatalog::Part(EFoundrylinePart(Part)).Name);};
 auto Terms=[](EFoundrylinePayment P){return FString(P==EFoundrylinePayment::Upfront?TEXT("Pays when you accept"):P==EFoundrylinePayment::PerDelivery?TEXT("Pays for each delivery"):TEXT("Pays when all parts arrive"));};
 auto PlaceName=[](EFoundrylinePalletPlace P){const TCHAR* N[]={TEXT("Receiving area"),TEXT("Storage rack"),TEXT("Plasma input"),TEXT("Machine output"),TEXT("Buffer"),TEXT("On a forklift"),TEXT("Loading bay"),TEXT("Delivery truck"),TEXT("Pickup truck"),TEXT("On the road")};return FString(N[FMath::Clamp(int(P),0,9)]);};
 auto BookingStatus=[&](const FFoundrylineFreightBooking& B){const TCHAR* N[]={TEXT("Booked"),TEXT("Truck on the way"),TEXT("Waiting for the bay"),B.Inbound?TEXT("Unloading"):TEXT("Loading"),TEXT("Leaving"),TEXT("On the road"),TEXT("Delivered")};return FString(N[FMath::Clamp(int(B.Status),0,6)]);};
 const double SheetLb=FFoundrylineSteelSheet::PoundsEach();
 const auto* Plasma=S.Machines.FindByPredicate([](const auto& M){return M.Id==1;});

 // Window frame and header: brand, company, time controls, cash, close.
 Rect(0,0,SW,SH,FLinearColor(0,0,0,.72f));Rect(X-2,Y-2,W+4,H+4,Dark);Rect(X,Y,W,H,Surface);Rect(X,Y,W,HeadH,Paper);Rect(X,Y,W,3,Orange);Rect(X,Y+HeadH-1,W,1,Line);
 if(!Logo)Logo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Foundryline/UI/Brand/T_FoundrylineLogo.T_FoundrylineLogo"));if(Logo)DrawTexture(Logo,(X+18)*Scale,(Y+11)*Scale,140*Scale,51*Scale,0,0,1,1);
 Rect(X+NavW-12,Y+16,1,42,Line);Text(VM.CompanyName.IsEmpty()?FString(TEXT("Your company")):VM.CompanyName,X+NavW+8,Y+13,20,true,Ink,420);Text(C.Enabled?TEXT("Office computer  /  ")+FFoundrylineCommerceCatalog::Date(C.Clock):TEXT("Office computer"),X+NavW+8,Y+41,14,false,Muted,420);
 HelpArea(TEXT("Clock"),X+NavW+4,Y+38,300,24,TEXT("Game day and time. One game hour passes in about a minute at 1x speed; 4x and 8x make waiting for steel, cutting and trucks faster."));
 {float R=X+W-18;Button(TEXT("Close"),TEXT("Close  [E]"),R-120,Y+16,120,40,EStyle::Secondary,true,TEXT("Close the computer and return to walking. E or Esc also closes it."));R-=144;
  const FString CashText=C.Enabled?Cash(C.CashCents):FString(TEXT("--"));const float CashW=FMath::Max(150.f,Width(CashText,24,true)+8);Text(TEXT("CASH"),R-CashW,Y+12,12,true,Muted);Text(CashText,R-CashW,Y+29,24,true,C.CashCents<0?Red:Ink);HelpArea(TEXT("Cash"),R-CashW,Y+10,CashW,52,TEXT("Money your company can spend right now. Steel, freight and equipment take cash out; customer payments put it back in. Shared by everyone in your company."));R-=CashW+26;Rect(R,Y+16,1,42,Line);R-=18;
  for(int Rate:{8,4,1}){R-=50;Button(FString::Printf(TEXT("Speed%d"),Rate),FString::Printf(TEXT("%dx"),Rate),R,Y+16,46,40,EStyle::Secondary,!PC->bOperationsPending,FString::Printf(TEXT("Run time at %dx speed. Production, forklifts and trucks all use the same clock."),Rate),S.Speed==Rate);}
  R-=150;Button(TEXT("Run"),S.Running?TEXT("Pause time"):TEXT("Resume time"),R,Y+16,144,40,S.Running?EStyle::Secondary:EStyle::Primary,!PC->bOperationsPending,S.Running?TEXT("Stop the clock. Nothing moves or gets made while time is paused."):TEXT("Start the clock so people, machines and trucks get to work."));
  R-=12;const FString State=S.Running?TEXT("Running"):TEXT("Paused");const float SWd=Width(State,13,true)+30;R-=SWd;Rect(R,Y+24,SWd,26,S.Running?SoftGreen:SoftOrange);Rect(R+10,Y+33,8,8,S.Running?Green:Orange);Text(State,R+22,Y+28,13,true,S.Running?Green:Amber);}

 // Section rail. The dot marks where the next step happens.
 Rect(X,Y+HeadH,NavW,H-HeadH-FootH,Rail);Rect(X+NavW-1,Y+HeadH,1,H-HeadH-FootH,Line);
 struct FNav {int Page;const TCHAR* Label;const TCHAR* Sub;int Icon;int Badge;FLinearColor BadgeCol;const TCHAR* Help;};
 const int Missing=(Guide.bOperator?0:1)+(Guide.bForkliftDriver?0:1);
 const FNav Nav[]={{9,TEXT("Home"),TEXT("What to do next"),0,0,Muted,TEXT("Your next step and a summary of the whole business.")},
  {0,TEXT("Orders"),TEXT("Customer work"),1,Guide.OffersWaiting,Blue,TEXT("Find customer orders, accept them and follow their progress.")},
  {3,TEXT("Buy steel"),TEXT("Sheets to cut"),2,0,Muted,TEXT("Order steel sheets. A truck delivers them to your loading bay.")},
  {10,TEXT("Production"),TEXT("Machines"),3,0,Muted,TEXT("See what each machine is making, program the plasma cutter and buy equipment.")},
  {4,TEXT("Stock & trucks"),TEXT("Pallets and freight"),4,Guide.FinishedAtBay,Green,TEXT("Trucks coming and going, and every pallet you own.")},
  {7,TEXT("Team"),TEXT("People and jobs"),5,Missing,Red,TEXT("Hire people and give them jobs on machines and forklifts.")},
  {5,TEXT("Money"),TEXT("Cash & payments"),6,0,Muted,TEXT("Cash, spending and customer payments.")}};
 const int GuidePage=!C.Enabled?-1:Guide.Step==EFoundrylineGuideStep::AcceptOrder?0:Guide.Step==EFoundrylineGuideStep::BuySteel?3:Guide.Step==EFoundrylineGuideStep::StaffWork?7:Guide.Step==EFoundrylineGuideStep::ProgramPlasma?10:4;
 for(int I=0;I<7;++I){const auto& N=Nav[I];const bool On=Page==N.Page||(N.Page==0&&Page==2)||(N.Page==10&&Page==6)||(N.Page==4&&Page==11);const float At=Y+HeadH+18+I*66;const float V=Hit(FString::Printf(TEXT("Page%d"),N.Page),X+10,At,NavW-20,60,true,N.Help);
  if(On||V>.01f)Rect(X+10,At,NavW-20,60,On?Paper:FMath::Lerp(Rail,Hex(TEXT("ECEFF1")),V));if(On){Border(X+10,At,NavW-20,60,Line);Rect(X+10,At,4,60,Orange);}else Hover(X+10,At,NavW-20,60,V);
  Icon(N.Icon,X+26,At+17,26,On?Orange:Graphite);Text(N.Label,X+66,At+10,16,true,Ink,NavW-84);Text(N.Sub,X+66,At+33,12,false,Muted,NavW-84);
  if(N.Badge>0){const FString B=FString::FromInt(N.Badge);const float D=FMath::Max(20.f,Width(B,11,true)+10);Rect(X+42,At+8,D,20,Paper);Rect(X+43,At+9,D-2,18,N.BadgeCol);Text(B,X+42+(D-Width(B,11,true))/2,At+10,11,true,Paper);}
  else if(N.Page==GuidePage&&Page==9){Rect(X+NavW-36,At+25,10,10,Orange);}}
 {const float B=Y+H-FootH-178;Text(TEXT("OTHER SCREENS"),X+22,B,11,true,Faint);
  Button(TEXT("Tablet"),TEXT("Loading tablet"),X+16,B+22,NavW-32,40,EStyle::Secondary,true,TEXT("Choose finished pallets and book a pickup truck for an order. Same screen as the tablet by the loading bay."));
  Button(TEXT("Plasma"),TEXT("Plasma controls"),X+16,B+70,NavW-32,40,EStyle::Secondary,true,TEXT("Open the plasma cutter's own screen to choose a part and batch size."));
  const bool Prop=PC->bPropertyViewAccess;Button(Prop?TEXT("ReturnOffice"):TEXT("PropertyView"),Prop?TEXT("Return to office"):TEXT("Build & layout view"),X+16,B+118,NavW-32,40,EStyle::Secondary,Prop||PC->IsNearOfficeComputer(),TEXT("Top-down view of your property for building and arranging the shop. Only available from this office computer."));}

 // Footer: last error from the server, otherwise a hint.
 Rect(X,Y+H-FootH,W,FootH,Hex(TEXT("E8EBEE")));Rect(X,Y+H-FootH,W,1,Line);
 if(!PC->OperationsStatus.IsEmpty()){Rect(X,Y+H-FootH,W,FootH,SoftRed);Rect(X,Y+H-FootH,5,FootH,Red);Text(TEXT("Could not do that: ")+PC->OperationsStatus,X+20,Y+H-FootH+11,14,true,Red,W-40);}
 else {Text(TEXT("Tip: point at any number or button to see what it means."),X+20,Y+H-FootH+12,13,false,Muted);RightText(PC->bOperationsPending?TEXT("Sending..."):TEXT("Co-op: your teammates see the same company data live."),X+W-20,Y+H-FootH+12,13,false,Muted);}

 if(!C.Enabled&&(Plant->bCutPartsStarter||Page!=7)){Text(Plant->bCutPartsStarter?TEXT("Connecting to your company..."):TEXT("Business finance is not enabled in this workshop. Team & jobs are available."),CX,CY+20,18,true,Muted,CW);return true;}
 const auto* Selected=C.Orders.FindByPredicate([&](const auto& O){return O.Id==PC->OperationsOrder;});
 const auto* Booking=C.Bookings.FindByPredicate([&](const auto& B){return B.Id==PC->OperationsBooking;});
 const auto Roster=FFoundrylineEmployeeRoster::Build(S);

 if(Page==9){
  PageHeader(Guide.CompletedOrders?TEXT("Welcome back"):TEXT("Home"),TEXT("Follow the next step below. Everything else on this computer is there when you need it."));
  const float Top=CY+80,LW=CW*.58f,RX=CX+LW+24,RW=CW-LW-24;
  // What to do next.
  const int StepNo=int(Guide.Step)+1;const float NH=372;Card(CX,Top,LW,NH,FString(),Orange);
  Text(TEXT("WHAT TO DO NEXT"),CX+26,Top+24,13,true,Orange);RightText(FString::Printf(TEXT("Step %d of 6"),StepNo),CX+LW-26,Top+24,13,true,Muted);
  float At=Top+54;At+=Wrap(Guide.Title,CX+26,At,LW-52,26,true,Ink,2,34)+6;At+=Wrap(Guide.Detail,CX+26,At,LW-52,16,false,Muted,3,23)+12;
  if(Guide.OrderId)HelpArea(TEXT("Order"),CX+20,Top+18,300,24,FString::Printf(TEXT("This step is for order %s from %s: %d %s still to deliver."),*Guide.OrderReference,*Guide.OrderCustomer,Guide.OrderRemaining,*Guide.OrderPartName));
  for(int I=0;I<FMath::Min(3,Guide.Problems.Num());++I){const auto& P=Guide.Problems[I];const bool Soft=P.Action==TEXT("Run");Rect(CX+26,At,LW-52,38,Soft?SoftOrange:SoftRed);Rect(CX+26,At,4,38,Soft?Orange:Red);Text(P.Text,CX+42,At+10,14,true,Soft?Amber:Red,LW-200);Button(P.Action,Soft?TEXT("Resume"):TEXT("Fix this >"),CX+LW-136,At+4,104,30,EStyle::Secondary,!PC->bOperationsPending,TEXT("Go straight to the screen that fixes this."));At+=44;}
  Button(Guide.Action,Guide.ActionLabel+TEXT("  >"),CX+26,Top+NH-74,FMath::Min(LW-52,360.f),52,EStyle::Primary,!PC->bOperationsPending,TEXT("Do the next step. You can always check and change things before anything is spent."));
  if(Guide.OrderId)Text(FString::Printf(TEXT("Order %s  /  %s  /  %d %s"),*Guide.OrderReference,*Guide.OrderCustomer,Guide.OrderRemaining,*Guide.OrderPartName),CX+FMath::Min(LW-52,360.f)+46,Top+NH-56,13,false,Muted,LW-FMath::Min(LW-52,360.f)-70);
  // The six-step loop.
  const float LTop=Top+NH+18,LH=CB-LTop;Card(CX,LTop,LW,LH,Guide.CompletedOrders?TEXT("Every order, step by step"):TEXT("Your first order, step by step"));
  const TCHAR* Steps[]={TEXT("Accept an order"),TEXT("Buy steel"),TEXT("Staff the plasma & forklift"),TEXT("Program the plasma"),TEXT("Book a pickup truck"),TEXT("Get paid")};
  const TCHAR* StepPages[]={TEXT("Page0"),TEXT("Page3"),TEXT("Page7"),TEXT("Page10"),TEXT("Page4"),TEXT("Page5")};
  const FString Notes[]={Guide.OrderId?(Guide.bAssignedToBay?FString::Printf(TEXT("%s, at the loading bay"),*Guide.OrderReference):FString::Printf(TEXT("%s accepted, not at the loading bay"),*Guide.OrderReference)):FString::Printf(TEXT("%d offers waiting"),Guide.OffersWaiting),
   Guide.SheetsToBuy?FString::Printf(TEXT("%d more sheet%s needed"),Guide.SheetsToBuy,Guide.SheetsToBuy==1?TEXT(""):TEXT("s")):FString::Printf(TEXT("%s ready, %s on the way"),*Lb(Guide.SteelOnHandLb),*Lb(Guide.SteelIncomingLb)),
   FString::Printf(TEXT("Plasma: %s  /  Forklift: %s"),Guide.bOperator?*Guide.OperatorName:TEXT("nobody"),Guide.bForkliftDriver?*Guide.DriverName:TEXT("nobody")),
   Guide.OrderId?FString::Printf(TEXT("%d to make, %d cutting, %d made"),Guide.PartsToMake,Guide.PartsCutting,Guide.PartsReady):FString(TEXT("Waits for an order")),
   Guide.OrderId?FString::Printf(TEXT("%d at the bay, %d on trucks"),Guide.PartsAtBay,Guide.PartsOnTrucks):FString(TEXT("Waits for finished parts")),
   FString::Printf(TEXT("%s owed to you on accepted orders"),*Cash(Guide.OwedToYouCents))};
  const float RowH=FMath::Min(56.f,(LH-58)/6);
  for(int I=0;I<6;++I){const float RY=LTop+48+I*RowH;const bool Done=Guide.bStepDone[I],Now=int(Guide.Step)==I;const float V=Hit(StepPages[I],CX+12,RY,LW-24,RowH-4,true,FString::Printf(TEXT("Open the screen for step %d: %s."),I+1,Steps[I]));
   if(Now)Rect(CX+12,RY,LW-24,RowH-4,SoftOrange);else if(V>.01f)Rect(CX+12,RY,LW-24,RowH-4,FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Hover(CX+12,RY,LW-24,RowH-4,V);
   const float CC=RY+(RowH-4)/2-15;Rect(CX+26,CC,30,30,Done?Green:Now?Orange:Surface);if(!Done&&!Now)Border(CX+26,CC,30,30,Line);Text(Done?TEXT("OK"):FString::FromInt(I+1),CX+26+(30-Width(Done?TEXT("OK"):FString::FromInt(I+1),12,true))/2,CC+7,12,true,Done||Now?Paper:Muted);
   Text(Steps[I],CX+72,RY+(RowH-4)/2-11,16,true,Done?Muted:Ink,LW*.42f);RightText(Notes[I],CX+LW-28,RY+(RowH-4)/2-9,13,false,Now?Amber:Muted);}
  // At a glance: each tile opens its section; hover explains the number.
  Text(TEXT("AT A GLANCE"),RX,Top+2,13,true,Muted);
  const float TW=(RW-14)/2,TH=104;
  struct FTile {FString Key,Label,Value,Sub,Help;FLinearColor Col;};
  const FString PlasmaState=!Plasma?FString(TEXT("No plasma installed")):!S.Running&&Guide.PartsInProgress?FString(TEXT("Paused: time stopped")):Plasma->Remaining>0?FString(TEXT("Plasma is cutting")):Guide.PartsInProgress?FString(TEXT("Waiting for steel")):FString(TEXT("Plasma has no job"));
  const FTile Tiles[]={
   {TEXT("Page5"),TEXT("Cash"),Cash(C.CashCents),TEXT("Ready to spend"),TEXT("Money your company can spend now. Buying steel, freight and equipment uses it; customer payments add to it."),Orange},
   {Guide.AcceptedOrders?TEXT("Page2"):TEXT("Page0"),TEXT("Orders"),FString::Printf(TEXT("%d accepted"),Guide.AcceptedOrders),FString::Printf(TEXT("%d offers waiting"),Guide.OffersWaiting),TEXT("Accepted orders are jobs you've agreed to make and deliver (up to 6 at a time). Offers are new jobs you can still accept."),Blue},
   {TEXT("Page11"),TEXT("Steel"),Lb(Guide.SteelOnHandLb),FString::Printf(TEXT("+%s on the way"),*Lb(Guide.SteelIncomingLb)),FString::Printf(TEXT("Steel you have in the shop (racks, receiving truck and plasma), and steel still on a delivery truck. One sheet is about %.0f lb."),SheetLb),Graphite},
   {TEXT("Page10"),TEXT("Being cut"),FString::Printf(TEXT("%d parts"),Guide.PartsInProgress),PlasmaState,TEXT("Parts left in the plasma cutter's current batch, including the one being cut now."),Orange},
   {TEXT("Page11"),TEXT("Finished parts"),FString::Printf(TEXT("%d parts"),Guide.FinishedParts),FString::Printf(TEXT("%d at the loading bay"),Guide.FinishedAtBay),TEXT("Cut parts not yet on a truck. Parts at the loading bay can be booked onto a pickup truck from the loading tablet."),Green},
   {TEXT("Page4"),TEXT("Trucks"),FString::Printf(TEXT("%d pickup%s"),Guide.PickupsBooked,Guide.PickupsBooked==1?TEXT(""):TEXT("s")),FString::Printf(TEXT("%d steel deliver%s due"),Guide.SteelDeliveriesDue,Guide.SteelDeliveriesDue==1?TEXT("y"):TEXT("ies")),TEXT("Customer pickups you've booked and steel deliveries that haven't finished unloading."),Blue},
   {TEXT("Page2"),TEXT("Owed to you"),Cash(Guide.OwedToYouCents),TEXT("On accepted orders"),TEXT("What customers will still pay for your accepted orders, according to each order's payment terms."),Green},
   {TEXT("Page5"),TEXT("Received"),Cash(Guide.ReceivedCents),TEXT("From customers"),TEXT("Payments customers have made on your recent orders."),Green}};
  for(int I=0;I<8;++I){const auto& T=Tiles[I];const float A=RX+(I%2)*(TW+14),B=Top+26+(I/2)*(TH+12);const float V=Hit(T.Key,A,B,TW,TH,true,T.Help);Rect(A,B,TW,TH,FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Border(A,B,TW,TH,Line);Rect(A,B,4,TH,T.Col);Hover(A,B,TW,TH,V);if(V>.3f)RightText(TEXT("Open  >"),A+TW-16,B+16,12,true,Blue);
   Text(T.Label.ToUpper(),A+20,B+16,12,true,Muted,TW-36);Text(T.Value,A+20,B+38,25,true,Ink,TW-36);Text(T.Sub,A+20,B+74,13,false,Muted,TW-36);}
  // Coming up: next truck movements.
  const float UTop=Top+26+4*(TH+12)+6,UH=CB-UTop;Card(RX,UTop,RW,UH,TEXT("Trucks coming up"));
  TArray<const FFoundrylineFreightBooking*> Next;for(const auto& B:C.Bookings)if(B.Status!=EFoundrylineBookingState::Delivered)Next.Add(&B);Next.Sort([](const auto& A,const auto& B){return A.ArrivalAt<B.ArrivalAt;});
  const int Rows=FMath::Max(1,int((UH-56)/62));
  for(int I=0;I<FMath::Min(Rows,Next.Num());++I){const auto& B=*Next[I];const float RY=UTop+46+I*62;const float V=Hit(FString::Printf(TEXT("CalendarBooking%d"),B.Id),RX+10,RY,RW-20,56,true,TEXT("Open this truck in Stock & trucks."));if(V>.01f)Rect(RX+10,RY,RW-20,56,FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Hover(RX+10,RY,RW-20,56,V);
   Chip(B.Inbound?(B.Part?TEXT("PARTS IN"):TEXT("STEEL IN")):TEXT("PICKUP"),RX+20,RY+16,B.Inbound?Blue:Green,B.Inbound?SoftBlue:SoftGreen);Text(B.Inbound?FFoundrylineCommerceCatalog::InboundLabel(B):B.Customer,RX+118,RY+8,15,true,Ink,RW-150);Text(BookingStatus(B)+TEXT("  /  ")+FFoundrylineCommerceCatalog::Date(B.ArrivalAt),RX+118,RY+31,13,false,Muted,RW-150);}
  if(Next.IsEmpty())Wrap(TEXT("No trucks booked. Steel deliveries and customer pickups you book will show here."),RX+20,UTop+52,RW-40,14,false,Muted,3);
 }
 else if(Page==0||Page==2){
  const bool Mine=Page==2;
  PageHeader(TEXT("Orders"),Mine?TEXT("Jobs you've agreed to make. Follow each one from steel to payment."):TEXT("Customers asking for parts. Pick one you can make with the plasma cutter, then accept it."));
  TArray<const FFoundrylineCustomerOrder*> Rows;int OfferCount=0,MineCount=0;for(const auto& O:C.Orders){OfferCount+=O.Status==EFoundrylineOrderState::Offered;MineCount+=O.Status!=EFoundrylineOrderState::Offered;if((O.Status!=EFoundrylineOrderState::Offered)==Mine)Rows.Add(&O);}
  if(Mine)Rows.Sort([](const auto& A,const auto& B){return (A.Status==EFoundrylineOrderState::Completed)!=(B.Status==EFoundrylineOrderState::Completed)?A.Status!=EFoundrylineOrderState::Completed:A.Id>B.Id;});
  Tabs({MakeTuple(FString(TEXT("Page0")),FString::Printf(TEXT("New offers (%d)"),OfferCount),!Mine,FString(TEXT("Orders customers are offering. Nothing is promised until you accept."))),MakeTuple(FString(TEXT("Page2")),FString::Printf(TEXT("Your orders (%d)"),MineCount),Mine,FString(TEXT("Orders you've accepted, plus recently completed ones.")))},CX,CY+74);
  const float Top=CY+134,LW=CW*.47f,RX=CX+LW+22,RW=CW-LW-22,RowH=96;const int Per=FMath::Max(1,int((CB-Top-50)/RowH)),F=First(Rows.Num(),Per);
  for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& O=*Rows[I];const float RY=Top+(I-F)*RowH;const bool On=O.Id==PC->OperationsOrder;const float V=Hit(FString::Printf(TEXT("Order%d"),O.Id),CX,RY,LW,RowH-8,true);
   Rect(CX,RY,LW,RowH-8,On?SoftOrange:FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Border(CX,RY,LW,RowH-8,On?Orange:Line);if(On)Rect(CX,RY,4,RowH-8,Orange);else Hover(CX,RY,LW,RowH-8,V);Mark(O.Customer,CX+16,RY+16,44);
   Text(O.Customer,CX+74,RY+12,16,true,Ink,LW-250);Text(FString::Printf(TEXT("%d x %s"),O.Quantity,*PartName(O.Part)),CX+74,RY+36,14,false,Ink,LW-250);RightText(Cash(O.Quantity*O.UnitPriceCents),CX+LW-18,RY+12,18,true,Ink);
   float CA=CX+74;const FString St=O.Status==EFoundrylineOrderState::Completed?TEXT("COMPLETED"):O.Status==EFoundrylineOrderState::Accepted?(O.Late?TEXT("LATE"):C.PickupOrder==O.Id?TEXT("IN PROGRESS / AT BAY"):TEXT("IN PROGRESS")):O.Email?TEXT("EMAIL OFFER"):TEXT("OFFER");
   CA+=Chip(St,CA,RY+60,O.Late?Red:O.Status==EFoundrylineOrderState::Completed?Green:O.Status==EFoundrylineOrderState::Accepted?Orange:Blue,O.Late?SoftRed:O.Status==EFoundrylineOrderState::Completed?SoftGreen:O.Status==EFoundrylineOrderState::Accepted?SoftOrange:SoftBlue)+8;
   if(O.Rush&&O.Status!=EFoundrylineOrderState::Completed)CA+=Chip(TEXT("RUSH"),CA,RY+60,Amber,SoftOrange)+8;Text(O.Status==EFoundrylineOrderState::Accepted?FString::Printf(TEXT("%d of %d delivered"),O.Delivered,O.Quantity):Terms(O.Payment),CA+4,RY+63,13,false,Muted,CX+LW-CA-20);}
  if(Rows.IsEmpty()){Card(CX,Top,LW,170);Text(Mine?TEXT("No orders yet"):TEXT("No offers right now"),CX+24,Top+28,20,true,Ink,LW-48);Wrap(Mine?TEXT("Accept an offer and it will show up here so you can follow it to payment."):TEXT("New customer offers arrive over time. Check back soon."),CX+24,Top+64,LW-48,15,false,Muted,3);if(Mine)Button(TEXT("Page0"),TEXT("See new offers  >"),CX+24,Top+112,220,42,EStyle::Primary,true,TEXT("Browse offers you can accept."));}
  Pager(Rows.Num(),Per,CX,CB-38,LW);
  // Selected order.
  const auto* O=Selected&&Rows.Contains(Selected)?Selected:nullptr;Card(RX,Top,RW,CB-Top);
  if(!O){Text(TEXT("Select an order on the left"),RX+28,Top+30,20,true,Muted,RW-56);}
  else {
   const auto& Spec=O->Part;const double NeedLb=(O->Quantity-O->Delivered)*([&](){const auto* P=S.Programs.FindByPredicate([&](const auto& V){return V.Machine==1&&int(V.Part)==Spec;});return P?double(P->SteelLb):0.;}());
   Mark(O->Customer,RX+28,Top+26,52);Text(O->Customer,RX+94,Top+28,18,true,Ink,RW-122);Text((O->Email?TEXT("Email enquiry  /  "):TEXT("Order  /  "))+O->Reference,RX+94,Top+54,13,false,Muted,RW-122);
   Text(FString::Printf(TEXT("%d x %s"),O->Quantity,*PartName(O->Part)),RX+28,Top+100,26,true,Ink,RW-56);
   struct FFact {FString Label,Value,Help;FLinearColor Col;};
   const FString Due=O->Deadline>0?FFoundrylineCommerceCatalog::Date(O->Deadline):O->AllowedMinutes>0?FString::Printf(TEXT("%.0f game hours after accepting"),O->AllowedMinutes/60):FString(TEXT("No deadline"));
   const FFact Facts[]={{TEXT("You earn"),Cash(O->Quantity*O->UnitPriceCents),TEXT("The full order value: quantity times the price per part."),Green},{TEXT("Price per part"),Cash(O->UnitPriceCents),TEXT("What the customer pays for each part. Rush orders usually pay more."),Ink},
    {TEXT("You get paid"),Terms(O->Payment),TEXT("Upfront: paid when you accept. Per delivery: paid for each truckload that arrives. On completion: paid once every part has arrived."),Ink},{O->Deadline>0?TEXT("Deadline"):TEXT("Time allowed"),Due,TEXT("The clock starts when you accept. Late orders are marked late."),O->Late?Red:Ink},
    {TEXT("Steel needed"),FString::Printf(TEXT("about %s (%d sheet%s)"),*Lb(NeedLb),FMath::Max(1,FMath::CeilToInt(NeedLb/SheetLb-.001)),FMath::CeilToInt(NeedLb/SheetLb-.001)>1?TEXT("s"):TEXT("")),TEXT("Steel still needed to cut the parts that haven't been delivered, before counting parts already made."),Ink},{TEXT("Made on"),TEXT("Plasma cutter"),TEXT("The plasma cutter cuts these parts from steel sheet. It needs an operator."),Ink}};
   for(int I=0;I<6;++I){const float A=RX+28+(I%2)*(RW-56)/2,B=Top+152+(I/2)*66;Text(Facts[I].Label.ToUpper(),A,B,12,true,Muted,(RW-56)/2-16);Text(Facts[I].Value,A,B+20,17,true,Facts[I].Col,(RW-56)/2-16);HelpArea(FString::Printf(TEXT("Fact%d"),I),A,B-4,(RW-56)/2-16,52,Facts[I].Help);}
   float At=Top+152+3*66+8;Rect(RX+28,At,RW-56,1,Line);At+=18;
   if(O->Status==EFoundrylineOrderState::Offered){
    Text(TEXT("ARE YOU READY?"),RX+28,At,12,true,Muted);At+=26;
    auto Ready=[&](bool Ok,const FString& T){Rect(RX+28,At+3,16,16,Ok?Green:Orange);Text(T,RX+54,At,15,false,Ok?Ink:Amber,RW-90);At+=30;};
    const double Have=Guide.SteelOnHandLb+Guide.SteelIncomingLb-Guide.SteelNeededLb;Ready(Have>=NeedLb,Have>=NeedLb?FString(TEXT("You have enough steel for this order")):FString::Printf(TEXT("You'll need to buy about %d sheet%s (%s each + delivery)"),FMath::CeilToInt((NeedLb-FMath::Max(0.,Have))/SheetLb-.001),FMath::CeilToInt((NeedLb-FMath::Max(0.,Have))/SheetLb-.001)>1?TEXT("s"):TEXT(""),*Cash(FMath::RoundToInt64(SheetLb*100))));
    Ready(Guide.bOperator,Guide.bOperator?FString::Printf(TEXT("Plasma operator: %s"),*Guide.OperatorName):FString(TEXT("Nobody runs the plasma yet (assign in Team)")));Ready(Guide.bForkliftDriver,Guide.bForkliftDriver?FString::Printf(TEXT("Forklift driver: %s"),*Guide.DriverName):FString(TEXT("No forklift driver yet (assign in Team)")));
    Ready(Guide.AcceptedOrders<6,FString::Printf(TEXT("%d of 6 order slots in use"),Guide.AcceptedOrders));
    Button(TEXT("Accept"),TEXT("Accept this order"),RX+28,CB-76,RW-56,54,EStyle::Primary,!PC->bOperationsPending,TEXT("Agree to make and deliver this order. Its deadline starts now. Nothing is charged for accepting."));
   } else {
    int Free=0,Bay=0;for(const auto& P:S.Pallets)if(int(P.Part)==O->Part&&!P.BookingId&&P.Place!=EFoundrylinePalletPlace::InTransit){Free+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Pickup)Bay+=P.Quantity;}
    int Trucks=0;for(const auto& B:C.Bookings)if(!B.Inbound&&B.OrderId==O->Id&&B.Status!=EFoundrylineBookingState::Delivered)for(const auto& Item:B.Manifest)Trucks+=Item.Quantity;
    Text(TEXT("PROGRESS"),RX+28,At,12,true,Muted);At+=24;const float BW=RW-56;const float Q=FMath::Max(1,O->Quantity);
    Rect(RX+28,At,BW,14,Surface);float BX=RX+28;auto Seg=[&](int N,FLinearColor Col){const float D=BW*FMath::Clamp(N/Q,0.f,1.f);Rect(BX,At,D,14,Col);BX+=D;};Seg(O->Delivered,Green);Seg(FMath::Min(Trucks,O->Quantity-O->Delivered),Blue);Seg(FMath::Min(Free,FMath::Max(0,O->Quantity-O->Delivered-Trucks)),Hex(TEXT("9FB7C6")));At+=22;
    Text(FString::Printf(TEXT("Delivered %d   /   On trucks %d   /   In stock %d (%d at the bay)   of %d"),O->Delivered,Trucks,Free,Bay,O->Quantity),RX+28,At,13,false,Muted,BW);HelpArea(TEXT("Progress"),RX+28,At-26,BW,46,TEXT("Green: delivered to the customer. Blue: loaded on booked trucks. Grey: finished parts in your shop that match this order."));At+=28;
    Text(FString::Printf(TEXT("Paid so far: %s of %s"),*Cash(O->PaidCents),*Cash(O->Quantity*O->UnitPriceCents)),RX+28,At,15,true,O->PaidCents>0?Green:Ink,BW);At+=34;
    if(O->Status==EFoundrylineOrderState::Accepted){
     if(Guide.OrderId==O->Id){Rect(RX+28,At,BW,40,SoftOrange);Text(TEXT("Next: ")+Guide.Title,RX+42,At+11,14,true,Amber,BW-28);At+=48;}
     const float Half=(BW-12)/2;
     if(C.PickupOrder==O->Id)Button(TEXT("HelpAtBay"),TEXT("At the loading bay"),RX+28,CB-76,Half,54,EStyle::Ghost,false,TEXT("Finished parts for this order are stacked at the loading bay so a pickup truck can collect them. Only one order uses the bay at a time."));
     else Button(TEXT("Assign"),TEXT("Send to loading bay"),RX+28,CB-76,Half,54,EStyle::Primary,!PC->bOperationsPending,TEXT("Make this the order whose finished parts the forklift stacks at the loading bay. Only one order uses the bay at a time."));
     Button(FString::Printf(TEXT("ProgramOrder%d"),O->Id),TEXT("Program plasma"),RX+40+Half,CB-76,Half,54,C.PickupOrder==O->Id?EStyle::Primary:EStyle::Secondary,!PC->bOperationsPending,TEXT("Open the plasma screen with this part and the quantity still needed filled in. You press Start there."));
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
   Text(TEXT("Steel sheet"),CX+28,Top+104,26,true,Ink);Text(FString::Printf(TEXT("60 x 120 in, 1/8 in thick  /  about %.0f lb per sheet  /  $1.00 per lb"),SheetLb),CX+28,Top+142,15,false,Muted,LW-56);
   float At=Top+184;
   {const bool Need=Guide.SheetsToBuy>0,Any=Guide.AcceptedOrders>0;Rect(CX+28,At,LW-56,72,Need?SoftOrange:Any?SoftGreen:Surface);Rect(CX+28,At,4,72,Need?Orange:Any?Green:Line);
    Text(Need?FString::Printf(TEXT("Your accepted orders need %d more sheet%s"),Guide.SheetsToBuy,Guide.SheetsToBuy==1?TEXT(""):TEXT("s")):Any?FString(TEXT("You have enough steel for your accepted orders")):FString(TEXT("Accept an order first to see how much steel you need")),CX+46,At+14,16,true,Need?Amber:Any?Green:Muted,LW-260);
    Text(FString::Printf(TEXT("Needed %s  /  In the shop %s  /  On the way %s"),*Lb(Guide.SteelNeededLb),*Lb(Guide.SteelOnHandLb),*Lb(Guide.SteelIncomingLb)),CX+46,At+42,13,false,Muted,LW-260);
    if(Need)Button(FString::Printf(TEXT("BuySheets%d"),FMath::Clamp(Guide.SheetsToBuy,1,10)),FString::Printf(TEXT("Use %d"),FMath::Clamp(Guide.SheetsToBuy,1,10)),CX+LW-196,At+16,150,40,EStyle::Secondary,true,TEXT("Set the quantity to exactly what your accepted orders still need."));}
   At+=100;Text(TEXT("HOW MANY SHEETS"),CX+28,At,12,true,Muted);HelpArea(TEXT("Qty"),CX+28,At-4,200,22,TEXT("You can buy 1 to 10 whole sheets per delivery. Leftover steel stays on your rack for the next order."));
   Button(TEXT("SheetsMinus"),TEXT("-"),CX+28,At+24,60,56,EStyle::Secondary,N>1,TEXT("One sheet fewer."));Text(FString::FromInt(N),CX+98+(70-Width(FString::FromInt(N),34,true))/2,At+30,34,true,Ink);Button(TEXT("SheetsPlus"),TEXT("+"),CX+178,At+24,60,56,EStyle::Secondary,N<10,TEXT("One sheet more (up to 10 per delivery)."));
   Text(FString::Printf(TEXT("= %s of steel"),*Lb(Pounds)),CX+260,At+40,16,false,Muted);
   At+=110;Text(TEXT("DELIVERY ARRIVES"),CX+28,At,12,true,Muted);Text(FFoundrylineCommerceCatalog::Date(C.Clock+PC->OperationsDelay),CX+28,At+22,22,true,Ink);HelpArea(TEXT("Arrival"),CX+28,At-4,260,56,TEXT("When the supplier's truck reaches your loading bay. It may wait if another truck is using the bay."));
   Button(TEXT("TimeMinus"),TEXT("30 min sooner"),CX+300,At+14,150,42,EStyle::Secondary,PC->OperationsDelay>30,TEXT("Ask for the delivery 30 minutes earlier."));Button(TEXT("TimePlus"),TEXT("30 min later"),CX+460,At+14,150,42,EStyle::Secondary,true,TEXT("Ask for the delivery 30 minutes later."));
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
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& B=*Rows[I];const float RY=Top+52+(I-F)*64;const float V=Hit(FString::Printf(TEXT("CalendarBooking%d"),B.Id),CX+8,RY,CW-16,58,true,TEXT("Open this delivery in Stock & trucks."));if(V>.01f||(I-F)%2)Rect(CX+8,RY,CW-16,58,V>.01f?Hex(TEXT("EEF4F8")):Rail);Hover(CX+8,RY,CW-16,58,V);
    Text(FString::Printf(TEXT("PO-%04d"),B.Id),CX+24,RY+18,15,true,Ink);Text(B.Part?FFoundrylineCommerceCatalog::InboundLabel(B):FString::Printf(TEXT("%d sheet%s  /  %s"),B.Sheets,B.Sheets==1?TEXT(""):TEXT("s"),*Lb(B.Sheets*SheetLb)),
CX+CW*.2f,RY+18,15,false,Ink,CW*.2f);Text(FFoundrylineCommerceCatalog::Date(B.ArrivalAt),CX+CW*.42f,RY+18,15,false,Ink,CW*.22f);
    Chip(BookingStatus(B).ToUpper(),CX+CW*.66f,RY+17,B.Status==EFoundrylineBookingState::Delivered?Green:Blue,B.Status==EFoundrylineBookingState::Delivered?SoftGreen:SoftBlue);}
   if(Rows.IsEmpty()){Text(TEXT("No steel ordered yet"),CX+24,Top+70,20,true,Ink);Button(TEXT("Workspace0"),TEXT("Buy steel  >"),CX+24,Top+112,200,44,EStyle::Primary,true,TEXT("Order your first steel sheets."));}
   Pager(Rows.Num(),Per,CX+20,CB-52,CW-40);
  }
 }
 else if(Page==10||Page==6){
  PageHeader(TEXT("Production"),Page==10?TEXT("What each machine is doing. The plasma cutter turns steel sheet into parts."):TEXT("Add machines and forklifts as your business grows. New equipment needs a person assigned in Team."));
  Tabs({MakeTuple(FString(TEXT("Page10")),FString(TEXT("Machines")),Page==10,FString(TEXT("Live status of your machines."))),MakeTuple(FString(TEXT("Page6")),FString(TEXT("Buy equipment")),Page==6,FString(TEXT("Buy a press brake or another forklift.")))},CX,CY+74);
  const float Top=CY+134;
  if(Page==10){
   int Row=0;for(const auto& M:S.Machines){const float RY=Top+Row++*196;if(RY+184>CB)break;const auto* Op=Roster.FindByPredicate([&](const auto& E){return E.Role==EFoundrylineEmployeeRole::Operator&&E.Assignment==M.Id;});
    const FString Name=M.Id==1?TEXT("Plasma cutter"):M.Id==2?TEXT("Press brake"):TEXT("Sheet roller");const int Left=M.Auto?0:M.BatchRemaining+(M.Remaining>0?1:0);
    const bool Cutting=S.Running&&M.Enabled&&M.Remaining>0;FString State;FLinearColor Col;
    if(!S.Running&&Left){State=TEXT("TIME PAUSED");Col=Amber;}else if(!M.Enabled){State=TEXT("MACHINE PAUSED");Col=Amber;}else if(Cutting){State=M.Id==1?TEXT("CUTTING"):TEXT("WORKING");Col=Green;}else if(Left&&!Op){State=TEXT("NEEDS AN OPERATOR");Col=Red;}else if(Left){State=M.Id==1?TEXT("WAITING FOR STEEL"):TEXT("WAITING FOR PARTS");Col=Amber;}else {State=TEXT("NO JOB");Col=Muted;}
    Card(CX,RY,CW,184);Rect(CX,RY,4,184,Col);Icon(3,CX+26,RY+26,30,Graphite);Text(Name,CX+72,RY+22,22,true,Ink,CW*.3f);Chip(State,CX+72,RY+58,Col,Col==Green?SoftGreen:Col==Red?SoftRed:Col==Muted?Surface:SoftOrange);
    HelpArea(FString::Printf(TEXT("MachineState%d"),M.Id),CX+70,RY+54,220,32,TEXT("Cutting: working now. Waiting for steel: the forklift still has to bring a sheet. Needs an operator: assign someone in Team. No job: program it with a part and quantity."));
    const float IX=CX+CW*.36f;Text(TEXT("OPERATOR"),IX,RY+24,12,true,Muted);Text(Op?Op->Label:FString(TEXT("Nobody assigned")),IX,RY+44,17,true,Op?Ink:Red,CW*.26f);
    Text(TEXT("MAKING"),IX,RY+90,12,true,Muted);Text(Left?FString::Printf(TEXT("%s  /  %d left"),*PartName(int(M.Making)),Left):FString(TEXT("Nothing programmed")),IX,RY+110,17,true,Ink,CW*.3f);
    HelpArea(FString::Printf(TEXT("MachineMaking%d"),M.Id),IX-4,RY+86,CW*.3f,50,TEXT("The part this machine is making and how many are left in the batch, including the one in progress."));
    if(M.Id==1){Text(TEXT("STEEL LOADED"),IX+CW*.32f,RY+24,12,true,Muted);Text(Lb(M.BedLb),IX+CW*.32f,RY+44,17,true,Ink);HelpArea(TEXT("Bed"),IX+CW*.32f-4,RY+20,170,50,TEXT("Steel already on the cutting table. Each part uses a fixed weight; leftovers carry over to the next part."));}
    const float Progress=M.Duration>0?1-M.Remaining/M.Duration:0;if(M.Remaining>0){Text(FString::Printf(TEXT("Current part %d%%"),FMath::RoundToInt(Progress*100)),IX+CW*.32f,RY+90,12,true,Muted);Bar(IX+CW*.32f,RY+114,CW*.18f,Progress,Orange);}
    const float BX=CX+CW-266;
    if(M.Id==1&&Guide.Step==EFoundrylineGuideStep::ProgramPlasma)Button(FString::Printf(TEXT("ProgramOrder%d"),Guide.OrderId),FString::Printf(TEXT("Program for %s"),*Guide.OrderReference),BX,RY+28,240,50,EStyle::Primary,!PC->bOperationsPending,TEXT("Open the plasma screen with this order's part and remaining quantity filled in."));
    if(M.Id<=2)Button(M.Id==1?TEXT("Plasma"):TEXT("BrakeComputer"),M.Id==1?TEXT("Open plasma screen"):TEXT("Open brake screen"),BX,RY+(M.Id==1&&Guide.Step==EFoundrylineGuideStep::ProgramPlasma?94:28),240,50,EStyle::Secondary,true,TEXT("Open this machine's own control screen."));
    if(!Op)Button(TEXT("Page7"),TEXT("Assign operator"),BX,RY+112,240,44,EStyle::Secondary,true,TEXT("Open Team to put someone on this machine."));
   }
   if(S.Machines.IsEmpty())Text(TEXT("No machines installed."),CX,Top+20,20,true,Muted);
   // How parts move through the shop, with live counts.
   {const float FT=Top+Row*196+8;if(FT+200<CB){Card(CX,FT,CW,CB-FT>260?250:CB-FT,TEXT("How parts move through your shop"));
    struct FStage {const TCHAR* Name;FString Value;const TCHAR* Who;int Icon;};
    const FStage Flow[]={{TEXT("Steel rack"),Lb(Guide.SteelOnHandLb),TEXT("Truck delivers, forklift stores"),2},{TEXT("Plasma cutter"),FString::Printf(TEXT("%d cutting"),Guide.PartsInProgress),TEXT("Operator cuts parts"),3},{TEXT("Finished parts"),FString::Printf(TEXT("%d parts"),Guide.FinishedParts-Guide.FinishedAtBay),TEXT("Forklift racks them"),1},{TEXT("Loading bay"),FString::Printf(TEXT("%d parts"),Guide.FinishedAtBay),TEXT("Staged for your order"),4},{TEXT("Customer"),FString::Printf(TEXT("%d pickup%s booked"),Guide.PickupsBooked,Guide.PickupsBooked==1?TEXT(""):TEXT("s")),TEXT("Truck delivers, you get paid"),6}};
    const float SW5=(CW-48-4*36)/5;for(int I=0;I<5;++I){const float A=CX+24+I*(SW5+36),B=FT+56;Rect(A,B,SW5,140,Rail);Border(A,B,SW5,140,Line);Icon(Flow[I].Icon,A+18,B+18,26,Graphite);Text(Flow[I].Name,A+56,B+20,15,true,Ink,SW5-70);Text(Flow[I].Value,A+18,B+60,22,true,Ink,SW5-36);Wrap(Flow[I].Who,A+18,B+96,SW5-36,12,false,Muted,2,17);
     if(I<4){Rect(A+SW5+8,B+68,18,4,Orange);Rect(A+SW5+20,B+62,4,16,Orange);Rect(A+SW5+24,B+65,3,10,Orange);}}}}
  } else {
   const float EW=(CW-22)/2;
   auto Offer=[&](float A,const FString& Name,const FString& What,const FString& Note,int64 Price,const FString& Key,const FString& Label,bool Can,const FString& Help){Card(A,Top,EW,CB-Top);Icon(Key==TEXT("BuyBrake")?3:4,A+28,Top+30,38,Graphite);Text(Name,A+84,Top+30,26,true,Ink,EW-112);
    float At=Top+100;At+=Wrap(What,A+28,At,EW-56,16,false,Ink,3)+10;Wrap(Note,A+28,At,EW-56,14,false,Muted,3);Text(TEXT("PRICE"),A+28,CB-176,12,true,Muted);Text(Cash(Price),A+28,CB-154,30,true,Ink);
    Button(Key,Label,A+28,CB-90,EW-56,56,EStyle::Primary,Can&&!PC->bOperationsPending,Help);};
   Offer(CX,TEXT("Press brake"),TEXT("Bends flat parts into bent brackets and support channels, which sell for more."),TEXT("Comes with its own input storage. You also need an operator: hire someone and assign them in Team."),450000,TEXT("BuyBrake"),C.BrakeOwned?TEXT("Already installed"):TEXT("Buy & install"),!C.BrakeOwned,TEXT("Pay now and install the press brake in your shop."));
   Offer(CX+EW+22,TEXT("Extra forklift"),TEXT("A second forklift moves steel and parts faster when the shop gets busy."),FString::Printf(TEXT("You have %d of 2 forklift parking spots filled. Each forklift needs its own driver."),S.Drivers.Num()),120000,TEXT("BuyForklift"),S.Drivers.Num()>=2?TEXT("Both spots filled"):TEXT("Buy forklift"),S.Drivers.Num()<2,TEXT("Pay now for another forklift. Assign a driver in Team."));
  }
 }
 else if(Page==4||Page==11){
  const bool Trucks=Page==4;
  PageHeader(TEXT("Stock & trucks"),Trucks?TEXT("Steel deliveries coming in and customer pickups going out, all through your one loading bay."):TEXT("Every pallet you own: steel sheets and finished parts, where they are and whether they're spoken for."));
  int Open=0;for(const auto& B:C.Bookings)Open+=B.Status!=EFoundrylineBookingState::Delivered;
  Tabs({MakeTuple(FString(TEXT("Page4")),FString::Printf(TEXT("Trucks (%d)"),Open),Trucks,FString(TEXT("Booked deliveries and pickups."))),MakeTuple(FString(TEXT("Page11")),FString::Printf(TEXT("Stock (%d pallets)"),S.Pallets.Num()),!Trucks,FString(TEXT("Everything on your pallets.")))},CX,CY+74);
  const float Top=CY+134;
  if(Trucks){
   const float BannerH=74;Card(CX,Top,CW,BannerH);Rect(CX,Top,4,BannerH,Guide.FinishedAtBay?Green:Line);
   Text(Guide.FinishedAtBay?FString::Printf(TEXT("%d finished parts are waiting at the loading bay"),Guide.FinishedAtBay):FString(TEXT("No finished parts at the loading bay yet")),CX+24,Top+14,18,true,Guide.FinishedAtBay?Green:Ink,CW-380);
   Text(TEXT("Book a pickup on the loading tablet: tick the pallets, choose a truck that fits, confirm."),CX+24,Top+44,13,false,Muted,CW-380);
   Button(TEXT("Tablet"),TEXT("Book a pickup  >"),CX+CW-300,Top+12,276,50,Guide.FinishedAtBay?EStyle::Primary:EStyle::Secondary,true,TEXT("Open the loading tablet to choose pallets and book a pickup truck."));
   const float LTop=Top+BannerH+16,LW=CW*.56f,RX=CX+LW+22,RW=CW-LW-22;
   TArray<const FFoundrylineFreightBooking*> Rows;for(const auto& B:C.Bookings)Rows.Add(&B);Rows.Sort([](const auto& A,const auto& B){const bool AD=A.Status==EFoundrylineBookingState::Delivered,BD=B.Status==EFoundrylineBookingState::Delivered;return AD!=BD?!AD:AD?A.ArrivalAt>B.ArrivalAt:A.ArrivalAt<B.ArrivalAt;});
   const int Per=FMath::Max(1,int((CB-LTop-50)/80)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& B=*Rows[I];const float RY=LTop+(I-F)*80;const bool On=Booking&&B.Id==Booking->Id;const float V=Hit(FString::Printf(TEXT("Booking%d"),B.Id),CX,RY,LW,72,true);
    Rect(CX,RY,LW,72,On?SoftOrange:FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Border(CX,RY,LW,72,On?Orange:Line);if(On)Rect(CX,RY,4,72,Orange);else Hover(CX,RY,LW,72,V);
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
     Button(TEXT("TimeMinus"),TEXT("Sooner"),RX+24,At+50,(RW-60)/2,40,EStyle::Secondary,true,TEXT("Move the new time 30 minutes earlier."));Button(TEXT("TimePlus"),TEXT("Later"),RX+36+(RW-60)/2,At+50,(RW-60)/2,40,EStyle::Secondary,true,TEXT("Move the new time 30 minutes later."));
     Button(TEXT("Rebook"),TEXT("Move truck to this time"),RX+24,CB-74,RW-48,52,EStyle::Primary,!PC->bOperationsPending,TEXT("Rebook this truck for the new time. Costs half of the original fee."));}}
  } else {
   // Summary by material, then every pallet.
   TMap<int32,FIntVector> ByPart;for(const auto& P:S.Pallets){if(P.Place==EFoundrylinePalletPlace::InTransit||P.Part==EFoundrylinePart::Sheet)continue;auto& V=ByPart.FindOrAdd(int(P.Part));if(P.BookingId)V.Z+=P.Quantity;else {V.X+=P.Quantity;if(P.Place==EFoundrylinePalletPlace::Pickup)V.Y+=P.Quantity;}}
   const float SH2=104;float A=CX;const float SWd=(CW-3*16)/4;
   Card(A,Top,SWd,SH2);Rect(A,Top,4,SH2,Graphite);Text(TEXT("STEEL SHEET"),A+20,Top+16,12,true,Muted);Text(Lb(Guide.SteelOnHandLb),A+20,Top+38,25,true,Ink);Text(FString::Printf(TEXT("about %.1f sheets"),Guide.SteelOnHandLb/SheetLb),A+20,Top+74,13,false,Muted);HelpArea(TEXT("StockSteel"),A,Top,SWd,SH2,TEXT("All steel in the shop, including what's already on the plasma table."));A+=SWd+16;
   int Shown=0;for(const auto& P:ByPart){if(Shown++>=3)break;Card(A,Top,SWd,SH2);Rect(A,Top,4,SH2,Green);Text(PartName(P.Key).ToUpper(),A+20,Top+16,12,true,Muted,SWd-36);Text(FString::Printf(TEXT("%d free"),P.Value.X),A+20,Top+38,25,true,Ink);Text(FString::Printf(TEXT("%d at the bay  /  %d on trucks"),P.Value.Y,P.Value.Z),A+20,Top+74,13,false,Muted,SWd-36);HelpArea(FString::Printf(TEXT("StockPart%d"),P.Key),A,Top,SWd,SH2,TEXT("Free: finished and not yet booked on a truck. At the bay: ready for a pickup. On trucks: booked and leaving or gone."));A+=SWd+16;}
   if(ByPart.IsEmpty()){Card(A,Top,CW-(A-CX),SH2);Text(TEXT("No finished parts yet"),A+20,Top+22,18,true,Ink);Text(TEXT("Parts appear here after the plasma cuts them."),A+20,Top+54,14,false,Muted);}
   const float TTop=Top+SH2+18;Card(CX,TTop,CW,CB-TTop);const float Col[]={0,.13f,.38f,.5f,.72f};const TCHAR* Names[]={TEXT("PALLET"),TEXT("CONTAINS"),TEXT("QUANTITY"),TEXT("WHERE"),TEXT("STATUS")};
   for(int I=0;I<5;++I)Text(Names[I],CX+24+CW*Col[I],TTop+18,12,true,Muted);Rect(CX+16,TTop+44,CW-32,1,Line);
   TArray<const FFoundrylineFactoryPallet*> Rows;for(const auto& P:S.Pallets)Rows.Add(&P);Rows.Sort([](const auto& A,const auto& B){return A.Part!=B.Part?A.Part<B.Part:A.Id<B.Id;});
   const int Per=FMath::Max(1,int((CB-TTop-110)/50)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& P=*Rows[I];const float RY=TTop+52+(I-F)*50;if((I-F)%2)Rect(CX+8,RY,CW-16,48,Rail);
    Text(FString::Printf(TEXT("PLT-%04d"),P.Id),CX+24,RY+14,14,true,Ink);Text(P.Part==EFoundrylinePart::Sheet?FString(TEXT("Steel sheet")):PartName(int(P.Part)),CX+24+CW*.13f,RY+14,15,false,Ink,CW*.24f);
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
  auto AssignmentName=[&](const FFoundrylineEmployeeRecord& E){if(!E.Assignment)return FString(TEXT("No job yet"));if(E.Role==EFoundrylineEmployeeRole::Operator)return FString(E.Assignment==1?TEXT("Plasma cutter"):E.Assignment==2?TEXT("Press brake"):TEXT("Sheet roller"));if(E.Role==EFoundrylineEmployeeRole::Welder){if(const auto* St=S.WeldingStations.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;}))return FFoundrylineWeldingStationLayout::Name(St->Number)+(E.StationSlot?TEXT(" / Welder B"):TEXT(" / Welder A"));return FString::Printf(TEXT("Welding zone %d"),E.Assignment);}const int Index=S.Drivers.IndexOfByPredicate([&](const auto& D){return D.Id==E.VehicleId;});return Index>=0?FString::Printf(TEXT("Forklift %d"),Index+1):FString(TEXT("No job yet"));};
  PageHeader(TEXT("Team"),TEXT("Hiring adds a person. Then give them a job: run a machine or drive a forklift. Everyone in your co-op company shares this team."));
  // Who the starter shop needs.
  {const float RY=CY+70;float A=CX;auto Need=[&](const FString& Job,bool Ok,const FString& Who){const FString T=Job+TEXT(": ")+(Ok?Who:FString(TEXT("nobody")));const float D=Width(T,14,true)+52;Rect(A,RY,D,34,Ok?SoftGreen:SoftRed);Rect(A+12,RY+12,10,10,Ok?Green:Red);Text(T,A+32,RY+8,14,true,Ok?Green:Red);HelpArea(TEXT("Need")+Job,A,RY,D,34,Ok?FString(TEXT("Covered. This job is filled.")):FString(TEXT("Missing. Without this job filled, the starter loop stops. Pick an employee below and assign the job.")));A+=D+12;};
   Need(TEXT("Plasma operator"),Guide.bOperator,Guide.OperatorName);Need(TEXT("Forklift driver"),Guide.bForkliftDriver,Guide.DriverName);}
  const auto* EmployeeSelected=Staff.FindByPredicate([&](const auto& E){return E.Id==PC->OperationsEmployeeChoice;});
  Tabs({MakeTuple(FString(TEXT("Workspace0")),FString::Printf(TEXT("People & jobs (%d)"),Staff.Num()),PC->OperationsWorkspaceTab==0,FString(TEXT("Everyone you've hired and what they're doing."))),MakeTuple(FString(TEXT("Workspace2")),FString(TEXT("Hire")),PC->OperationsWorkspaceTab==2,FString(TEXT("Add a new person to your team."))),MakeTuple(FString(TEXT("Workspace1")),FString(TEXT("Forklift priorities")),PC->OperationsWorkspaceTab==1,FString(TEXT("Choose which forklift jobs come first.")))},CX,CY+116);
  const float Top=CY+176;
  if(PC->OperationsWorkspaceTab==0&&EmployeeSelected){
   const auto& E=*EmployeeSelected;const float LW=CW*.64f,RX=CX+LW+22,RW=CW-LW-22;const int JobRoleId=FMath::Clamp(PC->OperationsJobRole,0,2);const bool Returning=E.bMoving||(!E.Assignment&&FVector::Dist2D(E.Position,E.Home)>3);const bool CanAssign=!E.Assignment&&!Returning;
   Card(CX,Top,LW,CB-Top);Text(TEXT("Give ")+E.Label+TEXT(" a job"),CX+24,Top+22,22,true,Ink,LW-250);Button(TEXT("TeamRoster"),TEXT("< Back to team"),CX+LW-204,Top+18,180,40,EStyle::Secondary,true,TEXT("Back to the list of people."));
   Text(TEXT("1   CHOOSE THE KIND OF JOB"),CX+24,Top+76,12,true,Muted);const TCHAR* JobTitles[]={TEXT("Machine operator"),TEXT("Forklift driver"),TEXT("Welder")};const int JobValues[]={0,2,1};
   const TCHAR* JobHelp[]={TEXT("Runs a machine such as the plasma cutter. Must walk to the machine before it can work."),TEXT("Drives a forklift: unloads steel, feeds the plasma and stacks finished parts."),TEXT("Welds at a welding station (Welder A side) or a product work zone.")};
   for(int J=0;J<3;++J)Button(FString::Printf(TEXT("JobRole%d"),JobValues[J]),JobTitles[J],CX+24+J*(LW-48+12)/3,Top+98,(LW-48-24)/3,46,EStyle::Secondary,true,JobHelp[J],JobRoleId==JobValues[J]);
   Text(TEXT("2   CHOOSE WHERE THEY WORK"),CX+24,Top+170,12,true,Muted);
   struct FJobOption {int Id;FString Title,Detail;bool Occupied=false,Ready=true;int Slot=0;};TArray<FJobOption> Options;
   if(JobRoleId==0){for(const auto& M:S.Machines){const auto* Assigned=Staff.FindByPredicate([&](const auto& Other){return Other.Id!=E.Id&&int(Other.Role)==JobRoleId&&Other.Assignment==M.Id;});Options.Add({M.Id,M.Id==1?TEXT("Plasma cutter"):M.Id==2?TEXT("Press brake"):TEXT("Sheet roller"),Assigned?FString(TEXT("Already run by "))+Assigned->Label:FString(TEXT("Free: no operator yet")),Assigned!=nullptr,true});}}
   else if(JobRoleId==2){for(int V=0;V<S.Drivers.Num();++V){const auto& D=S.Drivers[V];const bool Parked=!D.Job&&FVector::Dist2D(D.Position,D.Parking)<20;Options.Add({D.Id,FString::Printf(TEXT("Forklift %d"),V+1),D.EmployeeId?TEXT("Already has a driver"):Parked?TEXT("Free and parked"):TEXT("Driving back to its parking spot"),D.EmployeeId!=0&&D.EmployeeId!=E.Id,Parked});}}
   else{for(const auto& St:S.WeldingStations)for(int Slot=0;Slot<2;++Slot){const auto* Assigned=Staff.FindByPredicate([&](const auto& Other){return Other.Id!=E.Id&&Other.Role==EFoundrylineEmployeeRole::Welder&&Other.Assignment==St.Id&&Other.StationSlot==Slot;});
     Options.Add({St.Id,FFoundrylineWeldingStationLayout::Name(St.Number)+(Slot?TEXT(" / Welder B"):TEXT(" / Welder A")),Assigned?FString(TEXT("Already worked by "))+Assigned->Label:FString(Slot?TEXT("Second welder: 170% with both at work"):TEXT("Headache-rack assembly at the bench")),Assigned!=nullptr,true,Slot});}
    for(const auto& Z:S.Zones){const bool Occupied=Staff.ContainsByPredicate([&](const auto& Other){return Other.Id!=E.Id&&Other.Role==EFoundrylineEmployeeRole::Welder&&Other.Assignment==Z.Id;});Options.Add({Z.Id,FString::Printf(TEXT("Welding zone %d"),Z.Id),TEXT("Product assembly station"),Occupied,true});}}
   const int Per=FMath::Max(1,int((CB-Top-260)/80)),F=FMath::Clamp(PC->OperationsJobPage,0,FMath::Max(0,(Options.Num()-1)/Per))*Per;
   for(int I=F;I<FMath::Min(F+Per,Options.Num());++I){const auto& Option=Options[I];const float RY=Top+194+(I-F)*80;Rect(CX+24,RY,LW-48,72,(I-F)%2?Rail:Paper);Border(CX+24,RY,LW-48,72,Line);Text(Option.Title,CX+42,RY+12,18,true,Ink,LW-280);Text(Option.Detail,CX+42,RY+42,13,false,Option.Occupied?Amber:Muted,LW-280);
    const bool Current=int(E.Role)==JobRoleId&&E.Assignment==Option.Id&&E.StationSlot==Option.Slot;const FString Label=Current?TEXT("Current job"):Option.Occupied?TEXT("Taken"):!Option.Ready?TEXT("Not ready"):TEXT("Assign here");
    Button(Option.Slot?FString::Printf(TEXT("AssignEmployee%d_%d_%d_%d"),E.Id,JobRoleId,Option.Id,Option.Slot):FString::Printf(TEXT("AssignEmployee%d_%d_%d"),E.Id,JobRoleId,Option.Id),Label,CX+LW-210,RY+14,170,44,EStyle::Primary,CanAssign&&!Option.Occupied&&Option.Ready&&!PC->bOperationsPending,Current?TEXT("This is their job now."):!CanAssign?TEXT("Release their current job first (on the right), and let them reach the waiting area."):Option.Occupied?TEXT("Someone else already works here."):TEXT("Give them this job. They'll walk over and start."));}
   if(Options.IsEmpty()){Text(JobRoleId==1?TEXT("No welding stations placed"):TEXT("No equipment installed"),CX+42,Top+210,20,true,Ink,LW-84);Wrap(JobRoleId==1?TEXT("Place a welding station first (Production > Welding station)."):JobRoleId==2?TEXT("Buy a forklift first (Production > Buy equipment). Hiring gives you a person only."):TEXT("Install a machine first."),CX+42,Top+246,LW-84,14,false,Muted,2);}
   if(Options.Num()>Per){Button(TEXT("JobPrevious"),TEXT("< Back"),CX+LW-250,CB-56,100,36,EStyle::Secondary,F>0);Button(TEXT("JobNext"),TEXT("More >"),CX+LW-140,CB-56,100,36,EStyle::Secondary,F+Per<Options.Num());}
   Card(RX,Top,RW,CB-Top,TEXT("This person"));Text(E.Label,RX+24,Top+50,21,true,Ink,RW-48);Text(AssignmentName(E),RX+24,Top+84,16,true,E.Assignment?Ink:Orange,RW-48);Chip(FFoundrylineEmployeeRoster::ActivityName(E.Activity),RX+24,Top+118,E.bWorking?Green:E.bMoving?Blue:Muted,E.bWorking?SoftGreen:E.bMoving?SoftBlue:Surface);Wrap(E.CurrentJob,RX+24,Top+156,RW-48,13,false,Muted,2);Rect(RX+24,Top+206,RW-48,1,Line);
   if(E.Assignment){Wrap(TEXT("To change jobs, release this one first. They keep their name and come back to the waiting area."),RX+24,Top+224,RW-48,14,false,Ink,3);bool Safe=true;
    if(E.Role==EFoundrylineEmployeeRole::ForkliftDriver){const auto* D=S.Drivers.FindByPredicate([&](const auto& V){return V.Id==E.VehicleId;});Safe=D&&!D->Job&&FVector::Dist2D(D->Position,D->Parking)<20;}
    if(E.Role==EFoundrylineEmployeeRole::Operator){const auto* M=S.Machines.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;});Safe=M&&(M->Remaining<=0||!M->Enabled);}
    // A station keeps its unit's progress when its welder leaves, so release is always safe there.
    if(E.Role==EFoundrylineEmployeeRole::Welder){const auto* Z=S.Zones.FindByPredicate([&](const auto& V){return V.Id==E.Assignment;});Safe=S.WeldingStations.ContainsByPredicate([&](const auto& V){return V.Id==E.Assignment;})||Z&&(!S.Running||Z->ActiveStep<0);}
    Button(FString::Printf(TEXT("AssignEmployee%d_3_0"),E.Id),TEXT("Release from this job"),RX+24,CB-130,RW-48,48,EStyle::Danger,Safe&&!PC->bOperationsPending,Safe?TEXT("Take them off this job. Work there stops until someone else is assigned."):TEXT("Wait until the current part is finished (or pause the machine) and the forklift is parked."));
    if(!Safe)Wrap(TEXT("Finish or pause the current work and park the forklift first."),RX+24,CB-72,RW-48,13,false,Amber,2);}
   else{Text(Returning?TEXT("Walking to the waiting area"):TEXT("Ready for a job"),RX+24,Top+224,17,true,Returning?Muted:Green,RW-48);Wrap(Returning?TEXT("You can assign a new job once they arrive."):TEXT("Choose a kind of job, then where they work."),RX+24,Top+256,RW-48,14,false,Muted,3);}
  }else if(PC->OperationsWorkspaceTab==0){
   Card(CX,Top,CW,CB-Top);const float C1=CX+CW*.28f,C2=CX+CW*.52f,C3=CX+CW*.80f;Text(TEXT("PERSON"),CX+24,Top+18,12,true,Muted);Text(TEXT("JOB"),C1,Top+18,12,true,Muted);Text(TEXT("DOING NOW"),C2,Top+18,12,true,Muted);Rect(CX+16,Top+44,CW-32,1,Line);
   const int Per=FMath::Max(1,int((CB-Top-110)/70)),F=First(Staff.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Staff.Num());++I){const auto& E=Staff[I];const float RY=Top+52+(I-F)*70;if((I-F)%2)Rect(CX+8,RY,CW-16,64,Rail);
    Icon(5,CX+24,RY+18,26,E.Assignment?Graphite:Orange);Text(E.Label,CX+62,RY+12,16,true,Ink,CW*.28f-70);Text(E.Assignment?FFoundrylineEmployeeRoster::RoleName(E.Role):TEXT("Not working yet"),CX+62,RY+38,13,false,Muted,CW*.28f-70);
    Text(AssignmentName(E),C1,RY+12,16,true,E.Assignment?Ink:Orange,CW*.24f-14);Text(E.CurrentJob,C1,RY+38,12,false,Muted,CW*.24f-14);
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
   Card(CX,Top,CW,CB-Top);Wrap(TEXT("Drivers do the most important job first. Click a job to change its priority (1 is first). Drivers pick available work automatically."),CX+24,Top+20,CW-48,15,false,Muted,2);
   const TCHAR* Tasks[]={TEXT("Trucks: unload & load"),TEXT("Feed the machines"),TEXT("Put away finished parts")};
   for(int I=0;I<S.Drivers.Num()&&I<4;++I){const auto& D=S.Drivers[I];const float RY=Top+76+I*134;Rect(CX+20,RY,CW-40,120,Rail);Border(CX+20,RY,CW-40,120,Line);Icon(4,CX+40,RY+20,28,Graphite);Text(FString::Printf(TEXT("Forklift %d"),I+1),CX+80,RY+20,18,true,Ink);Text(D.EmployeeId?TEXT("Driver: ")+D.DisplayName:FString(TEXT("No driver")),CX+220,RY+22,15,false,D.EmployeeId?Muted:Red,CW*.4f);
    for(int J=0;J<3;++J){const int P=D.Priorities.IsValidIndex(J)?D.Priorities[J]+1:J+1;Button(FString::Printf(TEXT("Priority%d_%d"),D.Id,J),FString::Printf(TEXT("%d   %s"),P,Tasks[J]),CX+40+(CW-100)/3*J+J*10,RY+60,(CW-100)/3-10,46,EStyle::Secondary,!PC->bOperationsPending,TEXT("Click to cycle this job's priority: 1 (first), 2, 3."),P==1);}}
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
    {TEXT("Owed to you"),Guide.OwedToYouCents,TEXT("What customers will still pay on accepted orders, according to their terms."),Blue},{TEXT("Paid upfront, parts owed"),VM.Summary.CustomerAdvancesCents,TEXT("Customers who paid in advance and are still waiting for their parts."),Amber}};
   const float TW=(CW-5*14)/6;for(int I=0;I<6;++I){const float A=CX+I*(TW+14);Card(A,Top,TW,104);Rect(A,Top,4,104,Sums[I].Col);Text(FString(Sums[I].Label).ToUpper(),A+18,Top+16,11,true,Muted,TW-30);Text(Cash(Sums[I].Value),A+18,Top+40,21,true,I==3&&Sums[I].Value?Red:Ink,TW-30);HelpArea(FString::Printf(TEXT("Sum%d"),I),A,Top,TW,104,Sums[I].Help);}
   const float LTop=Top+122;Card(CX,LTop,CW,CB-LTop,TEXT("Money in and out, newest first"));
   TArray<const FFoundrylineERPLedgerEntry*> Rows;for(const auto& V:VM.Ledger)Rows.Add(&V);Rows.Sort([](const auto& A,const auto& B){return A.At==B.At?A.SourceIndex>B.SourceIndex:A.At>B.At;});
   Text(TEXT("WHEN"),CX+24,LTop+48,12,true,Muted);Text(TEXT("WHAT"),CX+CW*.2f,LTop+48,12,true,Muted);RightText(TEXT("AMOUNT"),CX+CW-24,LTop+48,12,true,Muted);Rect(CX+16,LTop+72,CW-32,1,Line);
   const int Per=FMath::Max(1,int((CB-LTop-140)/46)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& V=*Rows[I];const float RY=LTop+80+(I-F)*46;if((I-F)%2)Rect(CX+8,RY,CW-16,44,Rail);Text(FFoundrylineCommerceCatalog::Date(V.At),CX+24,RY+13,14,false,Muted);Text(V.Label,CX+CW*.2f,RY+12,15,true,Ink,CW*.55f);RightText((V.MovementCents>0?TEXT("+"):TEXT(""))+Cash(V.MovementCents),CX+CW-24,RY+11,16,true,V.MovementCents>=0?Green:Ink);}
   if(Rows.IsEmpty())Text(TEXT("No money movements yet."),CX+24,LTop+90,16,false,Muted);
   Pager(Rows.Num(),Per,CX+20,CB-52,CW-40);
  } else {
   const float LW=CW*.5f,RX=CX+LW+22,RW=CW-LW-22;TArray<const FFoundrylineCustomerOrder*> Rows;for(const auto& O:C.Orders)if(O.Status!=EFoundrylineOrderState::Offered)Rows.Add(&O);Rows.Sort([](const auto& A,const auto& B){return A.Id>B.Id;});
   const int Per=FMath::Max(1,int((CB-Top-50)/88)),F=First(Rows.Num(),Per);
   for(int I=F;I<FMath::Min(F+Per,Rows.Num());++I){const auto& O=*Rows[I];const float RY=Top+(I-F)*88;const bool On=O.Id==PC->OperationsOrder;const float V=Hit(FString::Printf(TEXT("Order%d"),O.Id),CX,RY,LW,80,true);Rect(CX,RY,LW,80,On?SoftOrange:FMath::Lerp(Paper,Hex(TEXT("EEF4F8")),V));Border(CX,RY,LW,80,On?Orange:Line);if(On)Rect(CX,RY,4,80,Orange);else Hover(CX,RY,LW,80,V);
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

 // Confirmations. Factory actions (hire, assign, time) reply only on failure, so success is read from the replicated snapshot.
 const double Now=GetWorld()->GetRealTimeSeconds();
 if(PC->OperationsWatch){
  if(Now-PC->OperationsWatchAt>10)PC->OperationsWatch=0;
  else if(PC->OperationsWatch==1&&PC->OperationsHireRequest.IsValid()&&S.HireRequests.Contains(PC->OperationsHireRequest))PC->ConfirmOperations(TEXT("New team member hired. Give them a job in People & jobs."));
  else if(PC->OperationsWatch==2){if(const auto* E=Roster.FindByPredicate([&](const auto& V){return V.Id==PC->OperationsWatchId;}))if(PC->OperationsWatchTarget?E->Assignment!=0:E->Assignment==0)PC->ConfirmOperations(PC->OperationsWatchTarget?FString::Printf(TEXT("%s has a new job: %s."),*E->Label,*E->CurrentJob):FString::Printf(TEXT("%s was released and is free for a new job."),*E->Label));}
  else if(PC->OperationsWatch==3&&S.Running==(PC->OperationsWatchTarget!=0))PC->ConfirmOperations(S.Running?TEXT("Time is running. People, machines and trucks are working."):TEXT("Time paused. Nothing moves until you resume."));
  else if(PC->OperationsWatch==4&&FMath::RoundToInt(S.Speed)==PC->OperationsWatchTarget)PC->ConfirmOperations(FString::Printf(TEXT("Speed set to %dx."),PC->OperationsWatchTarget));
 }
 auto Toast=[&](const FString& Head,const FString& Msg,FLinearColor Col,double Age,double Life){if(Age<0||Age>Life)return;const float In=FMath::Clamp(float(Age)*7,0.f,1.f),Out=FMath::Clamp(float(Life-Age)*2,0.f,1.f),Alpha=FMath::Min(In,Out);
  const float D=FMath::Min(CW*.7f,Width(Msg,16,true)+Width(Head,12,true)+76),A=CX+CW-D,B=Y+HeadH+12-(1-In)*14;auto Fade=[&](FLinearColor V){V.A*=Alpha;return V;};
  Rect(A+3,B+4,D,52,Fade(FLinearColor(0,0,0,.2f)));Rect(A,B,D,52,Fade(Col));Rect(A,B+48,D*FMath::Clamp(1-float(Age/Life),0.f,1.f),4,Fade(FLinearColor(1,1,1,.45f)));
  const float HW=Width(Head,12,true)+18;Rect(A+14,B+14,HW,24,Fade(FLinearColor(1,1,1,.22f)));Text(Head,A+23,B+18,12,true,Fade(Paper));Text(Msg,A+30+HW,B+15,16,true,Fade(Paper),D-HW-48);};
 if(PC->bOperationsPending)Toast(TEXT("SENDING"),TEXT("Waiting for the server to confirm..."),Graphite,.5,2);
 else if(!PC->OperationsStatus.IsEmpty()&&Now-PC->OperationsStatusAt<7)Toast(TEXT("NOT DONE"),PC->OperationsStatus,Red,Now-PC->OperationsStatusAt,7);
 else if(!PC->OperationsConfirm.IsEmpty())Toast(TEXT("DONE"),PC->OperationsConfirm,Green,Now-PC->OperationsConfirmAt,6);
 PC->CurrentMouseCursor=bClickableHover?EMouseCursor::Hand:EMouseCursor::Default;
 // Hover help popup, drawn last so it sits above everything.
 if(const FString* Help=HelpTexts.Find(HoveredBox)){float MX=0,MY=0;PC->GetMousePosition(MX,MY);MX/=Scale;MY/=Scale;const float TW=380;
  TArray<FString> Words;Help->ParseIntoArray(Words,TEXT(" "));int Lines=1;FString Current;for(const auto& Wd:Words){const FString Next=Current.IsEmpty()?Wd:Current+TEXT(" ")+Wd;if(!Current.IsEmpty()&&Width(Next,14,false)>TW-28){++Lines;Current=Wd;}else Current=Next;}
  const float TH=FMath::Min(6,Lines)*20+24;float A=MX+18,B=MY+24;if(A+TW>SW-8)A=MX-TW-12;if(B+TH>SH-8)B=MY-TH-12;Rect(A+3,B+3,TW,TH,FLinearColor(0,0,0,.18f));Rect(A,B,TW,TH,Dark);Rect(A,B,3,TH,Orange);Wrap(*Help,A+14,B+12,TW-28,14,false,Paper,6,20);}
 return true;
}
