#include "UI/FoundrylineOperationsTerminal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineERPViewModel.h"
#include "Employees/FoundrylineEmployeeTypes.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
AFoundrylineOperationsTerminal::AFoundrylineOperationsTerminal(){bReplicates=true;bAlwaysRelevant=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Terminal"));SetRootComponent(Mesh);Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->SetCanEverAffectNavigation(false);}
void AFoundrylineOperationsTerminal::BeginPlay(){Super::BeginPlay();Mesh->SetRelativeRotation(FRotator(0,180,0));Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Terminal==1?TEXT("/Game/Foundryline/UI/Operations/SM_OfficeWorkstation.SM_OfficeWorkstation"):TEXT("/Game/Foundryline/UI/Operations/SM_LoadingTablet.SM_LoadingTablet")));
 if(Terminal==1&&GetNetMode()!=NM_DedicatedServer){
  auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/UI/Operations/M_OfficeDesktopScreen.M_OfficeDesktopScreen"));
  if(Base){Desktop=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),1536,864);Desktop->ClearColor=FLinearColor::Black;Desktop->OnCanvasRenderTargetUpdate.AddDynamic(this,&AFoundrylineOperationsTerminal::DrawDesktop);DesktopMaterial=UMaterialInstanceDynamic::Create(Base,this);DesktopMaterial->SetTextureParameterValue(TEXT("DesktopTexture"),Desktop);LeftDesktop=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),1536,864);RightDesktop=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),1536,864);LeftDesktop->OnCanvasRenderTargetUpdate.AddDynamic(this,&AFoundrylineOperationsTerminal::DrawLeftDesktop);RightDesktop->OnCanvasRenderTargetUpdate.AddDynamic(this,&AFoundrylineOperationsTerminal::DrawRightDesktop);const auto Slots=Mesh->GetMaterialSlotNames();for(int32 I=0;I<Slots.Num();++I){const FString Slot=Slots[I].ToString();if(!Slot.Contains(TEXT("OfficeDesktopScreen")))continue;auto* Material=DesktopMaterial.Get();if(Slot.Contains(TEXT("Left"))||Slot.Contains(TEXT("Right"))){Material=UMaterialInstanceDynamic::Create(Base,this);Material->SetTextureParameterValue(TEXT("DesktopTexture"),Slot.Contains(TEXT("Left"))?LeftDesktop.Get():RightDesktop.Get());SideMaterials.Add(Material);}Mesh->SetMaterial(I,Material);}RefreshDesktop();GetWorldTimerManager().SetTimer(DesktopTimer,this,&AFoundrylineOperationsTerminal::RefreshDesktop,1.f,true);}
 }
}
void AFoundrylineOperationsTerminal::EndPlay(const EEndPlayReason::Type Reason){GetWorldTimerManager().ClearTimer(DesktopTimer);Super::EndPlay(Reason);}
void AFoundrylineOperationsTerminal::RefreshDesktop(){if(Desktop)Desktop->UpdateResource();if(LeftDesktop)LeftDesktop->UpdateResource();if(RightDesktop)RightDesktop->UpdateResource();}
void AFoundrylineOperationsTerminal::DrawDesktop(UCanvas* Canvas,int32 Width,int32 Height){DrawScreen(Canvas,Width,Height,0);}
void AFoundrylineOperationsTerminal::DrawLeftDesktop(UCanvas* Canvas,int32 Width,int32 Height){DrawScreen(Canvas,Width,Height,1);}
void AFoundrylineOperationsTerminal::DrawRightDesktop(UCanvas* Canvas,int32 Width,int32 Height){DrawScreen(Canvas,Width,Height,2);}
void AFoundrylineOperationsTerminal::DrawScreen(UCanvas* Canvas,int32 Width,int32 Height,int32 View){
 if(!Canvas)return;
 auto Col=[](const TCHAR* H){return FLinearColor(FColor::FromHex(H));};
 const auto Ink=Col(TEXT("1F2327")),Muted=Col(TEXT("4B5258")),Orange=Col(TEXT("F07A2F")),White=FLinearColor::White,Surface=Col(TEXT("F7F5F1"));
 auto Box=[&](float X,float Y,float W,float H,FLinearColor Color){FCanvasTileItem Tile(FVector2D(X,Y),FVector2D(W,H),Color);Tile.BlendMode=SE_BLEND_Opaque;Canvas->DrawItem(Tile);};
 auto Text=[&](const FString& S,float X,float Y,float Size,FLinearColor Color){FCanvasTextItem Item(FVector2D(X,Y),FText::FromString(S),GEngine->GetMediumFont(),Color);Item.Scale=FVector2D(Size/20.f);Item.bCentreX=false;Canvas->DrawItem(Item);};
 Box(0,0,Width,Height,Surface);Box(0,0,Width,48,Ink);Box(0,0,Width,5,Orange);Text(TEXT("FOUNDRYLINE  /  WORKSHOP"),28,15,19,White);Text(TEXT("WORKSTATION 01"),1270,15,17,White);
 Box(0,48,Width,124,White);Text(TEXT("YOUR WORKSHOP"),32,70,21,Orange);
 auto* PC=Cast<AFoundrylinePlayerController>(UGameplayStatics::GetPlayerController(this,0));auto* Plant=PC?PC->FactoryPlant():nullptr;
 const bool Access=Plant&&Plant->BusinessCompanyId==CompanyId&&Plant->BusinessPlantId==PlantId&&Plant->Snapshot.Commerce.Enabled;
 if(!Access){Text(TEXT("Company workstation"),32,108,34,Ink);Text(TEXT("Authorized business session required"),80,285,28,Ink);Text(TEXT("Press E to open your business applications"),80,345,23,Muted);return;}
 const auto& State=Plant->Snapshot;const auto VM=FFoundrylineERPViewModel::Build(State.Commerce,FFoundrylineERPViewModel::CompanyName(GetWorld(),CompanyId));
 Text(VM.CompanyName.Left(36),32,106,34,Ink);const int32 Minute=FMath::FloorToInt(State.Commerce.Clock);Text(FString::Printf(TEXT("DAY %d  /  %02d:%02d"),Minute/1440+1,(Minute%1440)/60,Minute%60),1190,85,22,Muted);Text(State.Running?TEXT("WORKSHOP RUNNING"):TEXT("SIMULATION PAUSED"),1190,120,18,State.Running?Col(TEXT("277452")):Muted);
 if(View!=0){
  Text(View==1?TEXT("Production & sales"):TEXT("Dispatch & people"),40,202,36,Ink);Text(TEXT("YOUR BUSINESS AT A GLANCE"),40,256,18,Muted);
  Box(40,304,880,460,White);Box(948,304,548,460,White);
  if(View==1){Text(TEXT("INSTALLED WORK CENTRES"),64,330,21,Muted);int32 Row=0;for(const auto& Machine:State.Machines){if(Row>=4)break;const float Y=386+Row++*83;Text(FString::Printf(TEXT("WORK CENTRE %02d"),Machine.Id),64,Y,25,Ink);Text(!State.Running?TEXT("PAUSED"):Machine.Remaining>0?TEXT("PROCESSING"):Machine.BatchRemaining>0?TEXT("QUEUED"):TEXT("IDLE / READY"),470,Y,23,Muted);Text(FString::Printf(TEXT("%d parts queued"),Machine.BatchRemaining),64,Y+32,20,Muted);}
   Text(TEXT("CUSTOMER ORDERS"),972,330,21,Muted);Text(FString::Printf(TEXT("%d active orders"),VM.Summary.ActiveOrders),972,386,30,Ink);Text(FString::Printf(TEXT("%d available offers"),VM.Summary.AvailableOffers),972,440,25,Muted);Text(FString::Printf(TEXT("$%.2f delivered"),VM.Summary.EarnedRevenueCents/100.0),972,500,27,Ink);Text(TEXT("Select recipes and quantities"),972,620,21,Muted);Text(TEXT("in the Production menu."),972,653,21,Muted);
  }else{Text(TEXT("FREIGHT SCHEDULE"),64,330,21,Muted);int32 Row=0;for(const auto& Booking:State.Commerce.Bookings){if(Booking.Status==EFoundrylineBookingState::Delivered)continue;if(Row>=4)break;const float Y=386+Row++*82;const int32 Arrival=FMath::FloorToInt(Booking.ArrivalAt);Text(FString::Printf(TEXT("FREIGHT %04d  /  %s"),Booking.Id,Booking.Inbound?TEXT("RECEIVING"):TEXT("SHIPPING")),64,Y,25,Ink);Text(FString::Printf(TEXT("Scheduled day %d  /  %02d:%02d"),Arrival/1440+1,(Arrival%1440)/60,Arrival%60),64,Y+33,21,Muted);}if(Row==0){Text(TEXT("No scheduled freight"),64,400,30,Ink);Text(TEXT("Book a pickup or order steel at this computer."),64,458,23,Muted);}
   Text(TEXT("YOUR NAMED TEAM"),972,330,21,Muted);int32 PersonRow=0;for(const auto& Employee:FFoundrylineEmployeeRoster::Build(State)){if(PersonRow>=5)break;const float Y=386+PersonRow++*66;Text(Employee.Label.Left(28),972,Y,25,Ink);Text(Employee.Assignment?TEXT("Assigned"):TEXT("Available for assignment"),972,Y+30,18,Muted);}
  }
  Box(0,816,Width,48,Col(TEXT("D7DFE5")));Text(TEXT("FL   /   LIVE WORKSHOP DISPLAY"),28,832,20,Ink);Text(TEXT("[ E ]  OPEN BUSINESS APPLICATIONS"),1040,832,18,Ink);return;
 }
 Box(16,190,224,602,Ink);const TCHAR* Names[]={TEXT("Home"),TEXT("Orders"),TEXT("Buy steel"),TEXT("Production"),TEXT("Stock & trucks"),TEXT("Team"),TEXT("Money")};for(int32 I=0;I<7;++I){if(I==0)Box(28,210,200,54,Orange);Text(Names[I],46,228+I*72,22,I==0?Ink:White);}
 Text(TEXT("Your workshop"),278,204,36,Ink);Text(TEXT("Pick your next move."),278,254,22,Muted);
 Box(278,306,1214,218,White);Box(278,306,6,218,Orange);Text(TEXT("MAKE SOMETHING. GROW YOUR BUSINESS."),308,336,25,Ink);Text(TEXT("Orders, production and people. One clear menu."),308,386,22,Muted);Box(308,446,420,52,Orange);Text(TEXT("[ E ]  OPEN YOUR WORKSHOP"),330,463,22,Ink);
 const FString Values[]={FString::Printf(TEXT("$%.2f"),VM.Summary.CashCents/100.0),FString::FromInt(VM.Summary.ActiveOrders),FString::FromInt(FFoundrylineEmployeeRoster::Build(State).Num())};const TCHAR* Labels[]={TEXT("COMPANY CASH"),TEXT("ACTIVE ORDERS"),TEXT("YOUR TEAM")};for(int32 I=0;I<3;++I){const float X=278+I*412;Box(X,552,390,210,White);Text(Labels[I],X+24,578,19,Muted);Text(Values[I],X+24,624,40,Ink);Box(X,758,390,4,Orange);}
 Box(0,816,Width,48,Ink);Text(TEXT("FOUNDRYLINE  /  YOUR COMPANY"),28,832,20,White);Text(TEXT("[ E ]  OPEN WORKSHOP"),1190,832,18,White);

}
void AFoundrylineOperationsTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineOperationsTerminal,Terminal);DOREPLIFETIME(AFoundrylineOperationsTerminal,CompanyId);DOREPLIFETIME(AFoundrylineOperationsTerminal,PlantId);}
