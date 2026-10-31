#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "FoundrylineMainMenu.generated.h"

UCLASS()
class FOUNDRYLINE_API AFoundrylineMainMenuController : public APlayerController
{
 GENERATED_BODY()
protected:
 virtual void BeginPlay() override;
};

// A separate local front end: no company, production or construction simulation.
UCLASS()
class FOUNDRYLINE_API AFoundrylineMainMenuMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AFoundrylineMainMenuMode();
};

UCLASS()
class FOUNDRYLINE_API AFoundrylineMainMenuHUD : public AHUD
{
 GENERATED_BODY()
public:
 static bool HasStarterSave(const FString& Slot=TEXT("FoundrylineCutParts01"));
 virtual void BeginPlay() override;
 virtual void DrawHUD() override;
 virtual void NotifyHitBoxClick(FName Name) override;
private:
 UPROPERTY(Transient) TObjectPtr<class UTexture2D> Artwork;
 UPROPERTY(Transient) TObjectPtr<class UTexture2D> Logo;
 bool bLoading=false;
 bool bSettings=false;
 bool bCanContinue=false;
 FString Message;
 TMap<FName,float> HoverAnimations;
 void StartShop(bool bHost,bool bResume=false);
};
