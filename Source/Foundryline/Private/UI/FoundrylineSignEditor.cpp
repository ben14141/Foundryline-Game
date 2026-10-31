#include "UI/FoundrylinePlayerController.h"
#include "UI/FoundrylineBuildHUD.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"

void AFoundrylinePlayerController::FinishSignName(const FString& Name,bool Apply){
 if(Apply)for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)if(const auto* O=It->GetSelectedObject())It->SetSelectedSign(Name.Left(40),O->SignFacingDegrees,O->Colors);
 if(SignNameWidget.IsValid()&&GetWorld()->GetGameViewport())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(SignNameWidget.ToSharedRef());SignNameWidget.Reset();
 FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
}
void AFoundrylinePlayerController::SignAction(FName Action){
 if(IsWalking()||!bPropertyViewAccess||SignNameWidget.IsValid())return;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){const auto* O=It->GetSelectedObject();if(!O||O->DefinitionId!=TEXT("ManufacturingSign"))continue;
  const FString Key=Action.ToString();auto Colors=O->Colors;float Facing=O->SignFacingDegrees;
  if(Action==TEXT("Name")){
   TSharedPtr<SEditableTextBox> Entry;
   SignNameWidget=SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center).BorderBackgroundColor(FLinearColor(0,0,0,.75f))[
    SNew(SBox).WidthOverride(620)[SNew(SBorder).Padding(28).BorderBackgroundColor(FLinearColor(.95f,.94f,.91f))[
     SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Text(FText::FromString(TEXT("MANUFACTURING SIGN / Company name"))).ColorAndOpacity(FLinearColor(.1f,.12f,.14f))]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[SAssignNew(Entry,SEditableTextBox).Text(FText::FromString(O->SignName)).SelectAllTextWhenFocused(true)
      .OnVerifyTextChanged_Lambda([](const FText& V,FText& Error){const FString S=V.ToString();if(S.Len()>40||S.Contains(TEXT("<"))||S.Contains(TEXT(">"))){Error=FText::FromString(TEXT("Use up to 40 characters."));return false;}return true;})
      .OnTextCommitted_Lambda([this](const FText& V,ETextCommit::Type Type){if(Type==ETextCommit::OnEnter)FinishSignName(V.ToString(),true);})]
     +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(TEXT("SAVE NAME"))).OnClicked_Lambda([this,Entry](){FinishSignName(Entry->GetText().ToString(),true);return FReply::Handled();})]
      +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(TEXT("CANCEL"))).OnClicked_Lambda([this](){FinishSignName(TEXT(""),false);return FReply::Handled();})]]]]];
   GetWorld()->GetGameViewport()->AddViewportWidgetContent(SignNameWidget.ToSharedRef(),100);FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Entry);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);FlushPressedKeys();return;
  }
  if(Action==TEXT("Left"))Facing-=15;else if(Action==TEXT("Right"))Facing+=15;
  else if(Key.StartsWith(TEXT("Panel_")))Colors.Exterior=FLinearColor(FColor::FromHex(Key.RightChop(6)));
  else if(Key.StartsWith(TEXT("Accent_")))Colors.Trim=FLinearColor(FColor::FromHex(Key.RightChop(7)));
  else if(Key.StartsWith(TEXT("Text_")))Colors.Interior=FLinearColor(FColor::FromHex(Key.RightChop(5)));
  else return;
  It->SetSelectedSign(O->SignName,Facing,Colors);return;
 }
}
bool AFoundrylineBuildHUD::DrawSignInspector(AFoundrylinePlayerController* PC){
 const FFoundrylinePlacedObject* O=nullptr;for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It)if(const auto* V=It->GetSelectedObject();V&&V->DefinitionId==TEXT("ManufacturingSign")){O=V;break;}if(!O)return false;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),X=Canvas->ClipX/S-400,Y=280,W=376;
 const FLinearColor Ink(FColor::FromHex(TEXT("242B31"))),Orange(FColor::FromHex(TEXT("ED7B2D")));
 auto R=[&](float A,float B,float C,float D,FLinearColor Color){DrawRect(Color,A*S,B*S,C*S,D*S);};
 auto T=[&](FString V,float A,float B,float Size=1.1f){DrawText(V,Ink,A*S,B*S,GEngine->GetMediumFont(),Size*S);};
 auto B=[&](FString Id,FString Label,float A,float B,float Width){const FName Key(*(TEXT("Sign")+Id));R(A,B,Width,42,HoveredBox==Key?FLinearColor(1,.78f,.55f):FLinearColor(.87f,.88f,.87f));T(Label,A+10,B+13,1);AddHitBox({A*S,B*S},{Width*S,42*S},Key,true);};
 R(X+4,Y+4,W,440,FLinearColor(0,0,0,.2f));R(X,Y,W,440,FLinearColor(.97f,.96f,.94f));R(X,Y,4,440,Orange);
 T(TEXT("MANUFACTURING SIGN"),X+18,Y+20,1.25f);T(O->SignName.Left(30),X+18,Y+53);T(TEXT("12' panel / double-sided / 9' overall"),X+18,Y+79,.95f);
 B(TEXT("Name"),TEXT("EDIT COMPANY NAME"),X+18,Y+109,W-36);B(TEXT("Left"),TEXT("TURN -15"),X+18,Y+161,164);B(TEXT("Right"),TEXT("TURN +15"),X+194,Y+161,164);
 const TCHAR* Colors[]={TEXT("30373D"),TEXT("ED7B2D"),TEXT("F0EEE7"),TEXT("33657A"),TEXT("55775A"),TEXT("8F3234")};
 const TCHAR* Channels[]={TEXT("Panel"),TEXT("Accent"),TEXT("Text")};
 for(int Row=0;Row<3;++Row){const float Z=Y+220+Row*63;T(FString(Channels[Row])+TEXT(" color"),X+18,Z,.98f);for(int I=0;I<6;++I){const float A=X+18+I*56;const FName Id(*FString::Printf(TEXT("Sign%s_%s"),Channels[Row],Colors[I]));R(A,Z+22,46,28,FLinearColor(FColor::FromHex(Colors[I])));if(HoveredBox==Id)R(A,Z+49,46,3,Orange);AddHitBox({A*S,(Z+22)*S},{46*S,30*S},Id,true);}}
 return true;
}
