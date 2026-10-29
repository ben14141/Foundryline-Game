#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FoundrylineGameMode.generated.h"

UCLASS()
class FOUNDRYLINE_API AFoundrylineGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AFoundrylineGameMode();
	virtual void BeginPlay() override;
 virtual void PostLogin(APlayerController* Player)override;
 virtual void Logout(AController* Player)override;
protected:
 bool bCutPartsStarter=false;
};

UCLASS()
class FOUNDRYLINE_API AFoundrylineCutPartsGameMode : public AFoundrylineGameMode {
 GENERATED_BODY()
public:
 AFoundrylineCutPartsGameMode(){bCutPartsStarter=true;}
};
