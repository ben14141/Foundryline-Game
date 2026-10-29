#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FoundrylineBuildHUD.generated.h"

UCLASS()
class FOUNDRYLINE_API AFoundrylineBuildHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;
private:
 void DrawText(const FString& Text,FLinearColor Color,float X,float Y,class UFont* Font=nullptr,float Scale=1,bool ScalePosition=false);
 void GetTextSize(const FString& Text,float& Width,float& Height,class UFont* Font=nullptr,float Scale=1)const;
 bool DrawOperations(class AFoundrylinePlayerController* PC);
 bool DrawBusinessDesktop(class AFoundrylinePlayerController* PC);
 void DrawFactoryMap(class AFoundrylinePlayerController* PC);
 bool DrawFactory(class AFoundrylinePlayerController* PC);
 void DrawMaterialHandling(class AFoundrylinePlayerController* PC);
 void DrawFreightControls(class AFoundrylinePlayerController* PC);
 void DrawSurfaceSizing(class AFoundrylinePlayerController* PC);
 void DrawPlacementGuide(class AFoundrylinePlayerController* PC);
 void DrawNetworkPanel(class AFoundrylinePlayerController* PC);
 void DrawLogisticsMap(class AFoundrylinePlayerController* PC);
	bool bEditTab=false;
 bool bConfirmLoad=false;
	bool bOfficeTab=false;
 bool bPaintTab=false;
	FName BuildCategory=TEXT("Structures");
 FName SiteCategory=TEXT("Streets");
	FName HoveredBox;
	TMap<FName,float> HoverAmounts;
	TMap<FName,float> PressAmounts;
 // Office desktop hover help, rebuilt every frame from the drawn hit boxes. Local presentation only.
 TMap<FName,FString> HelpTexts;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> Logo;
 UPROPERTY(Transient) TObjectPtr<class UFont> ERPFont;
};
