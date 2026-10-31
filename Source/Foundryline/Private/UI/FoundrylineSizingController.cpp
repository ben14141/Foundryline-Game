#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
#include "Construction/FoundrylineSizing.h"
#include "Construction/FoundrylineBuildSite.h"
#include "EngineUtils.h"
FIntPoint AFoundrylinePlayerController::GetConstructionSizeCm() const {
 const auto* Size=SurfaceSizes.Find(BuildTool);return Size?*Size:FFoundrylineSizing::Default(BuildTool);
}
void AFoundrylinePlayerController::ApplySurfaceDraft() {
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) {It->DraftTruckWidthCm=BuildTool==TEXT("TruckAccess")?GetConstructionSizeCm().X:2400;It->DraftAreaRotation=bRotateSurface?1:0;It->bExactDraftPlacement=bExactSurface;}
}
void AFoundrylinePlayerController::SetConstructionSize(int32 W,int32 D,bool Exact) {
 if(!IsLocalController() || !FFoundrylineSizing::Surface(BuildTool) || IsConstructionPending()) return;
 const auto Min=FFoundrylineSizing::Minimum(BuildTool);
 W=FMath::Clamp(W,Min.X/100,64);D=FMath::Clamp(D,Min.Y/100,64);
 if(const int32 Fixed=FFoundrylineSizing::FixedWidth(BuildTool)) W=Fixed/100;
 SurfaceSizes.Add(BuildTool,{W*100,D*100});bExactSurface=Exact;bDraggingBuild=false;ApplySurfaceDraft();
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It) It->Status=Exact?TEXT("Click once to place. R rotates. Right-click cancels."):TEXT("Hold left mouse and drag. Release to build. Right-click cancels.");
}
void AFoundrylinePlayerController::RotateSurfaceDraft() {if(BuildTool==TEXT("Sign")){for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->DraftAreaRotation=(It->DraftAreaRotation+1)%4;break;}return;}SurfaceSizingAction(TEXT("SizeRotate"));}
bool AFoundrylinePlayerController::SurfaceSizingAction(FName Action) {
 if(!Action.ToString().StartsWith(TEXT("Size"))) return false;
 if(!FFoundrylineSizing::Surface(BuildTool) || IsWalking() || IsConstructionPending()) return true;
 auto V=GetConstructionSizeCm();int32 W=V.X/100,D=V.Y/100;
 // Only lane width and layout orientation remain adjustable. Legacy preset
 // actions cannot silently switch normal map input into stamp placement.
 bExactSurface=false;
 if(Action==TEXT("SizeRotate")) bRotateSurface=!bRotateSurface;
 if(Action==TEXT("SizeWidthLess")) --W;
 if(Action==TEXT("SizeWidthMore")) ++W;
 SetConstructionSize(W,D,bExactSurface);return true;
}
FIntVector AFoundrylinePlayerController::ExactSurfaceEnd(FIntVector Start) const {
 const auto Size=GetConstructionSizeCm();
 if(FFoundrylineSizing::Road(BuildTool)) return Start+(bRotateSurface?FIntVector(Size.Y,0,0):FIntVector(0,Size.Y,0));
 return Start+(bRotateSurface?FIntVector(Size.Y,Size.X,0):FIntVector(Size.X,Size.Y,0));
}
