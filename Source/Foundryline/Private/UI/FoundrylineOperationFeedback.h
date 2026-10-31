#pragma once
#include "CoreMinimal.h"
class AFoundrylineBuildHUD;
class AFoundrylinePlayerController;
class UCanvas;
struct FFoundrylineFactoryCommand;
struct FFoundrylineFactoryState;
struct FFoundrylineCommerceCommand;
namespace FoundrylineOperationFeedback {
// Receipts are built only after authoritative commands commit successfully.
bool FactoryHasReceipt(const FFoundrylineFactoryCommand& Command);
FString FactoryReceipt(const FFoundrylineFactoryCommand& Command,const FFoundrylineFactoryState& State);
FString CommerceReceipt(const FFoundrylineCommerceCommand& Command,const FFoundrylineFactoryState& State);
void BeginFactory(AFoundrylinePlayerController* Player,const FFoundrylineFactoryCommand& Command);
void FinishFactory(AFoundrylinePlayerController* Player);
void Draw(AFoundrylineBuildHUD* HUD,AFoundrylinePlayerController* Player,UCanvas* Canvas);
}
