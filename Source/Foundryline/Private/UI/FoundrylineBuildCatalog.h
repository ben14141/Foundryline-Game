#pragma once
#include "CoreMinimal.h"
class AFoundrylineBuildHUD;class AFoundrylinePlayerController;class UCanvas;struct FInputKeyEventArgs;
void DrawFoundrylineBuildCatalog(AFoundrylineBuildHUD*,AFoundrylinePlayerController*,UCanvas*);
bool HandleFoundrylineBuildCatalogInput(AFoundrylineBuildHUD*,AFoundrylinePlayerController*,const FInputKeyEventArgs&);
bool HandleFoundrylineBuildCatalogAction(AFoundrylineBuildHUD*,AFoundrylinePlayerController*,FName);
bool FoundrylineBuildCatalogOverUI(AFoundrylineBuildHUD*,const AFoundrylinePlayerController*);
bool FoundrylineBuildCatalogOpen(AFoundrylineBuildHUD*);

FString FoundrylineBuildCatalogReport(AFoundrylineBuildHUD*,const AFoundrylinePlayerController*);
