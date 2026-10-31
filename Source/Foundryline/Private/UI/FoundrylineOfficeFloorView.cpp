#include "UI/FoundrylinePlayerController.h"
#include "Construction/FoundrylineBuildSite.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
void AFoundrylinePlayerController::SetOfficePlanningLevel(int32 Level){
 if(IsWalking()||!bPropertyViewAccess)return;OfficePlanningLevel=FMath::Clamp(Level,0,1);PlannerKind=PlannerId=0;bPlannerMoving=false;bFactoryMenu=false;FactoryTool=NAME_None;BuildTool=NAME_None;
 for(TActorIterator<AFoundrylineBuildSite> It(GetWorld());It;++It){It->OfficeViewLevel=OfficePlanningLevel;It->SetRoofsVisible(AreRoofsShown());}
 PlannerStatus=OfficePlanningLevel?TEXT("SECOND OFFICE FLOOR / desks and rooms / select ground floor to edit below"):TEXT("GROUND FLOOR");
}
