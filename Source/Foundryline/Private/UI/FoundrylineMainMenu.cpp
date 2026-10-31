#include "UI/FoundrylineMainMenu.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderUtils.h"
#include "UObject/StrongObjectPtr.h"

AFoundrylineMainMenuMode::AFoundrylineMainMenuMode()
{
 DefaultPawnClass=nullptr;
 PlayerControllerClass=AFoundrylineMainMenuController::StaticClass();
 HUDClass=AFoundrylineMainMenuHUD::StaticClass();
}

void AFoundrylineMainMenuController::BeginPlay()
{
 Super::BeginPlay();
 bShowMouseCursor=true;
 bEnableClickEvents=true;
 bEnableMouseOverEvents=true;
 FInputModeGameAndUI Mode;
 Mode.SetHideCursorDuringCapture(false);
 Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 SetInputMode(Mode);
}

void AFoundrylineMainMenuHUD::DrawHUD()
{
 Super::DrawHUD();
 if(!Canvas || !PlayerOwner)return;
 if(!Artwork)Artwork=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Foundryline/UI/MainMenu/T_MainMenuReference.T_MainMenuReference"));
 if(!Logo)Logo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Foundryline/UI/MainMenu/T_MainMenuLogo.T_MainMenuLogo"));
 const float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Min(W/1267.f,H/712.f);
 const float X=72*S,Y=260*S;
 const FLinearColor Orange(FColor::FromHex(TEXT("D88C48"))),White(FColor::FromHex(TEXT("F0F1ED")));
 DrawRect(FLinearColor(.035f,.05f,.061f),0,0,W,H);
 // Preserve the supplied background's proportions on every display.
 if(Artwork){const float ViewAspect=W/H,ArtAspect=float(Artwork->GetSizeX())/Artwork->GetSizeY();const float U=FMath::Min(1.f,ViewAspect/ArtAspect),V=FMath::Min(1.f,ArtAspect/ViewAspect);DrawTexture(Artwork,0,0,W,H,(1-U)*.5f,(1-V)*.5f,U,V);}
 auto Gradient=[&](float A,float B,float CW,float CH,FLinearColor TL,FLinearColor TR,FLinearColor BL,FLinearColor BR){TArray<FCanvasUVTri> Tri;FCanvasUVTri T;T.V0_Pos={A,B};T.V1_Pos={A+CW,B};T.V2_Pos={A,B+CH};T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;T.V0_Color=TL;T.V1_Color=TR;T.V2_Color=BL;Tri.Add(T);T.V0_Pos={A+CW,B};T.V1_Pos={A+CW,B+CH};T.V2_Pos={A,B+CH};T.V0_Color=TR;T.V1_Color=BR;T.V2_Color=BL;Tri.Add(T);FCanvasTriangleItem Item(Tri,GWhiteTexture);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);};
 Gradient(0,0,W,H,FLinearColor(.01f,.018f,.025f,.82f),FLinearColor(0,0,0,0),FLinearColor(.01f,.018f,.025f,.94f),FLinearColor(0,0,0,.08f));
 Gradient(0,H*.5f,W,H*.5f,FLinearColor(0,0,0,0),FLinearColor(0,0,0,0),FLinearColor(.02f,.03f,.04f,.45f),FLinearColor(.02f,.03f,.04f,.45f));
 if(Logo)DrawTexture(Logo,X,48*S,425*S,425*S*Logo->GetSizeY()/Logo->GetSizeX(),0,0,1,1);
 else DrawText(TEXT("FOUNDRYLINE"),White,X,90*S,GEngine->GetLargeFont(),1.7f*S);
 float MX=0,MY=0;PlayerOwner->GetMousePosition(MX,MY);
 static TStrongObjectPtr<UFont> MenuFont;
 if(!MenuFont.IsValid()){MenuFont.Reset(NewObject<UFont>());MenuFont->FontCacheType=EFontCacheType::Runtime;}
 auto MenuText=[&](const FString& Label,FLinearColor Color,float A,float B){FSlateFontInfo Font(FPaths::ProjectContentDir()/TEXT("Foundryline/UI/Fonts/BebasNeue-Regular.ttf"),FMath::RoundToInt(22*S));Font.LetterSpacing=125;FCanvasTextItem Item(FVector2D(A,B),FText::FromString(Label),Font,Color);Item.Font=MenuFont.Get();Canvas->DrawItem(Item);};
 auto Button=[&](FName Id,const FString& Label,int Row,bool Enabled=true)
 {
  const float BY=Y+Row*57*S,BW=340*S,BH=48*S;
  const bool Hover=Enabled&&MX>=X&&MX<X+BW&&MY>=BY&&MY<BY+BH;
  float& A=HoverAnimations.FindOrAdd(Id);A=FMath::FInterpTo(A,Hover?1.f:0.f,FMath::Min(GetWorld()->GetDeltaSeconds(),.05f),22.f);
  if(A>.005f){
   const float GX=X-3*S*(1-A),GW=BW*(.98f+.02f*A);
   static TStrongObjectPtr<UMaterialInstanceDynamic> Frost;
   if(!Frost.IsValid())if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/UI/MainMenu/M_MenuFrost.M_MenuFrost")))Frost.Reset(UMaterialInstanceDynamic::Create(Base,GetTransientPackage()));
   if(Frost.IsValid()&&Artwork){const float VA=W/H,AA=float(Artwork->GetSizeX())/Artwork->GetSizeY(),U=FMath::Min(1.f,VA/AA),V=FMath::Min(1.f,AA/VA);Frost->SetVectorParameterValue(TEXT("UVOrigin"),FLinearColor((1-U)*.5f+GX/W*U,(1-V)*.5f+BY/H*V,0,0));Frost->SetVectorParameterValue(TEXT("UVScale"),FLinearColor(GW/W*U,BH/H*V,0,0));Frost->SetScalarParameterValue(TEXT("Reveal"),A);DrawMaterialSimple(Frost.Get(),GX,BY,GW,BH);}
   Gradient(GX,BY,GW,BH,FLinearColor(.1f,.085f,.07f,.3f*A),FLinearColor(.1f,.085f,.07f,0),FLinearColor(.07f,.065f,.06f,.35f*A),FLinearColor(.07f,.065f,.06f,0));
   Gradient(GX,BY,GW,S,FLinearColor(1,1,1,.2f*A),FLinearColor(1,1,1,0),FLinearColor(1,1,1,.2f*A),FLinearColor(1,1,1,0));
   DrawRect(FLinearColor(0,0,0,.18f*A),GX,BY+BH-S,GW,S);
   auto Accent=Orange;Accent.A=A;DrawRect(Accent,X,BY+7*S,2*S,(BH-14*S)*A);
  }
  MenuText(Label,Enabled?FMath::Lerp(White,Orange,A):FLinearColor(.36f,.4f,.42f),X+(28+3*A)*S,BY+9*S);
  if(Enabled&&!bLoading)AddHitBox(FVector2D(X,BY),FVector2D(BW,BH),Id,true);
 };
 if(bSettings)
 {
  Button(TEXT("Windowed"),TEXT("WINDOWED  1280 x 720"),0);
  Button(TEXT("Fullscreen"),TEXT("BORDERLESS FULLSCREEN"),1);
  Button(TEXT("Back"),TEXT("BACK"),3);
 }
 else
 {
  Button(TEXT("Continue"),TEXT("CONTINUE"),0,false);
  Button(TEXT("NewGame"),TEXT("NEW GAME"),1);
  Button(TEXT("Load"),TEXT("LOAD GAME"),2,false);
  Button(TEXT("Host"),TEXT("MULTIPLAYER"),3);
  Button(TEXT("Settings"),TEXT("SETTINGS"),4);
  Button(TEXT("Quit"),TEXT("QUIT"),5);
 }
 const FString Help=bLoading?TEXT("Opening your first shop..."):Message;
 DrawText(Help,FLinearColor(.65f,.71f,.73f),X,H-42*S,GEngine->GetSmallFont(),S);
}

void AFoundrylineMainMenuHUD::StartShop(bool bHost)
{
 const FString Map=TEXT("/Game/Foundryline/Maps/L_FirstShopStarter");
 if(!FPackageName::DoesPackageExist(Map)){Message=TEXT("First shop map is missing. Run the first-shop setup before starting.");return;}
 bLoading=true;
 UGameplayStatics::OpenLevel(this,FName(*Map),true,bHost?TEXT("game=/Script/Foundryline.FoundrylineGameMode?listen"):TEXT("game=/Script/Foundryline.FoundrylineGameMode"));
}

void AFoundrylineMainMenuHUD::NotifyHitBoxClick(FName Name)
{
 Super::NotifyHitBoxClick(Name);
 if(bLoading)return;
 if(Name==TEXT("NewGame"))StartShop(false);
 else if(Name==TEXT("Host"))StartShop(true);
 else if(Name==TEXT("Settings"))bSettings=true;
 else if(Name==TEXT("Back"))bSettings=false;
 else if(Name==TEXT("Quit"))UKismetSystemLibrary::QuitGame(this,PlayerOwner,EQuitPreference::Quit,false);
 else if(Name==TEXT("Windowed")||Name==TEXT("Fullscreen"))
 {
  if(auto* Settings=GEngine->GetGameUserSettings())
  {
   Settings->SetFullscreenMode(Name==TEXT("Windowed")?EWindowMode::Windowed:EWindowMode::WindowedFullscreen);
   if(Name==TEXT("Windowed"))Settings->SetScreenResolution(FIntPoint(1280,720));
   Settings->ApplySettings(false);
  }
 }
}
