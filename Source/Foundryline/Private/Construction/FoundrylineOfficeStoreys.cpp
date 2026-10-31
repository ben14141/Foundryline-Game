#include "Construction/FoundrylineBuildSite.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

bool AFoundrylineBuildSite::OfficeStoreyRequestsClear(const TArray<FFoundrylinePlacementRequest>& Requests,FString& Error)const{
 for(const auto& R:Requests){
  if(R.Action==EFoundrylinePlacementAction::ReplaceObject){if(!OfficeStoreyWorldClear(R.Replacement,Error))return false;continue;}
  if(R.Action!=EFoundrylinePlacementAction::Remove&&R.Action!=EFoundrylinePlacementAction::Move&&R.Action!=EFoundrylinePlacementAction::Rotate)continue;
  const auto* Old=Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==R.ObjectId;});
  if(!Old||!Old->bOfficeSecondFloor)continue;
  auto Candidate=*Old;
  if(R.Action==EFoundrylinePlacementAction::Remove)Candidate.bOfficeSecondFloor=false;
  else if(R.Action==EFoundrylinePlacementAction::Move)Candidate.Anchor=R.Anchor;
  else Candidate.QuarterTurns=R.QuarterTurns;
  if(!OfficeStoreyWorldClear(Candidate,Error))return false;
 }
 return true;
}

bool AFoundrylineBuildSite::OfficeStoreyWorldClear(const FFoundrylinePlacedObject& O,FString& Error)const{
 if(!O.bOfficeAnnex)return true;
 const auto* Old=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==O.ObjectId;});
 if(Old&&Old->bOfficeSecondFloor&&(Old->Anchor!=O.Anchor||Old->QuarterTurns!=O.QuarterTurns||Old->DefinitionId!=O.DefinitionId)){auto Before=*Old;Before.bOfficeSecondFloor=false;if(!OfficeStoreyWorldClear(Before,Error))return false;}
 if(Old&&Old->Anchor==O.Anchor&&Old->QuarterTurns==O.QuarterTurns&&Old->DefinitionId==O.DefinitionId&&Old->bOfficeSecondFloor==O.bOfficeSecondFloor&&(!O.bOfficeSecondFloor||(Old->OfficeStairOrigin==O.OfficeStairOrigin&&Old->OfficeStairQuarter==O.OfficeStairQuarter&&Old->OfficeStairStyle==O.OfficeStairStyle)))return true;
 const auto* Def=Placement->FindDefinition(O.DefinitionId);if(!Def)return false;
 const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));const FBox2D Stair=FFoundrylineOfficeStairLayout::FullBounds(O.OfficeStairOrigin,O.OfficeStairQuarter,O.OfficeStairStyle);
 auto Inside=[&](FVector P){return P.X>=0&&P.Y>=0&&P.X<=Def->Footprint.Max.X&&P.Y<=Def->Footprint.Max.Y;};
 for(TActorIterator<APawn> It(GetWorld());It;++It)if(It->GetController()&&It->GetActorLocation().Z<800){const auto P=T.InverseTransformPosition(It->GetActorLocation());if(Inside(P)&&((Old&&Old->bOfficeSecondFloor&&P.Z>250)||(O.bOfficeSecondFloor&&Stair.ExpandBy(40).IsInsideOrOn(FVector2D(P))))){Error=TEXT("Move people off the stairs and upper floor before changing the staircase");return false;}}
 for(TActorIterator<AFoundrylineOperationsTerminal> It(GetWorld());It;++It)if(It->Terminal==1){const FVector P=T.InverseTransformPosition(It->GetActorLocation());if(!Inside(P))continue;const int Q=(FMath::RoundToInt((It->GetActorRotation().Yaw-O.QuarterTurns*90)/90)+4)%4;const auto Desk=FFoundrylinePlanningEquipment::Bounds(P+FRotator(0,Q*90,0).RotateVector({0,40,0}),Q,FFoundrylinePlanningEquipment::Size(7));if((!O.bOfficeSecondFloor&&P.Z>250)||(O.bOfficeSecondFloor&&Desk.Intersect(Stair))){Error=TEXT("Move desks clear of the staircase and its landings first");return false;}}
 for(TActorIterator<AFoundrylineFactoryPlant> It(GetWorld());It;++It)if(It->BusinessCompanyId==O.CompanyId&&It->BusinessPlantId==O.PlantId){const auto& State=It->HasAuthority()?It->Model.State:It->Snapshot;for(const auto& I:FFoundrylinePlannerItems::List(State)){if(I.Stored)continue;FBox2D B(ForceInit);double Top=-MAX_dbl;for(int X:{0,1})for(int Y:{0,1}){const auto P=T.InverseTransformPosition(It->GetActorTransform().TransformPosition({X?I.Bounds.Max.X:I.Bounds.Min.X,Y?I.Bounds.Max.Y:I.Bounds.Min.Y,I.Bounds.Max.Z}));B+=FVector2D(P);Top=FMath::Max(Top,P.Z);}if(!O.bOfficeSecondFloor&&I.Kind==EFoundrylinePlannerItem::OfficeDesk&&I.Center.Z>300&&B.Intersect(FBox2D({0,0},{double(Def->Footprint.Max.X),double(Def->Footprint.Max.Y)}))){Error=TEXT("Move upstairs desks before removing their floor");return false;}if(O.bOfficeSecondFloor&&(B.Intersect(Stair)||(Top>315&&B.Intersect(FBox2D({0,0},{double(Def->Footprint.Max.X),double(Def->Footprint.Max.Y)}))))){if(I.Kind==EFoundrylinePlannerItem::OfficeDesk&&I.Center.Z>300&&!B.Intersect(Stair))continue;Error=TEXT("The stairs or new office floor overlap equipment; move it clear first");return false;}}}
 return true;
}

bool AFoundrylineBuildSite::PlaceOfficeStairs(FIntVector Cursor,int32 Quarter,bool Preview,uint8 Style){
 const auto* Host=PickOffice(Cursor);if(!Host){Status=TEXT("Place the staircase inside your ground-floor office");if(Preview)HidePreview();return false;}
 if(Cursor.Z>=320){Status=TEXT("Place or move the staircase from the ground floor");return false;}
 const FVector P=FRotator(0,Host->QuarterTurns*90,0).UnrotateVector(FVector(Cursor-Host->Anchor));
 FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Host->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*Host;R.Replacement.bOfficeSecondFloor=true;R.Replacement.OfficeStairStyle=Style;R.Replacement.OfficeStairOrigin={FMath::RoundToInt32(P.X/25)*25,FMath::RoundToInt32(P.Y/25)*25};R.Replacement.OfficeStairQuarter=(Quarter-Host->QuarterTurns+4)%4;
 FString Error;const bool Clear=OfficeStoreyWorldClear(R.Replacement,Error);const auto Valid=Clear?ValidateConstruction(R):FFoundrylinePlacementResult{EFoundrylinePlacementError::Occupied};
 if(Preview){ShowPreview(R.Replacement,Clear&&Valid.IsSuccess(),FString::Printf(TEXT("OfficeStairs_%s_%d_%d_%d_%d_%lld"),*Host->ObjectId.ToString(),R.Replacement.OfficeStairOrigin.X,R.Replacement.OfficeStairOrigin.Y,R.Replacement.OfficeStairQuarter,Style,Placement->GetRevision()));Status=!Clear?Error:Valid.IsSuccess()?TEXT("Click to add stairs and second office floor / R rotates"):TEXT("Fit the full staircase and landings inside an open office area");return Clear&&Valid.IsSuccess();}
 if(!Clear){Status=Error;return false;}
 FFoundrylinePlacementRequest Back;Back.Action=EFoundrylinePlacementAction::ReplaceObject;Back.ObjectId=Host->ObjectId;Back.Replacement=*Host;
 const auto Result=ExecuteConstruction(R);if(SentToHost(Result.Error))return true;if(!Result.IsSuccess()){Status=PlacementErrorText(Result.Error);return false;}RecordUndo(Back);HidePreview();Status=TEXT("Second office floor and staircase built. Choose upstairs in the office catalogue to furnish it.");return true;
}
