#include "Construction/FoundrylineBuildSite.h"
#include "Pathing/FoundrylineDrivingLayout.h"
bool AFoundrylineBuildSite::EditTruckRouting(FIntVector Start,FIntVector End,bool Erase)
{
 if(!bReady || !FFoundrylineGrid::IsSafeCoordinate(Start) || !FFoundrylineGrid::IsSafeCoordinate(End)) return false;
 const FIntVector Low=Start.ComponentMin(End),High=Start.ComponentMax(End);
 if(High.X-Low.X<100 || High.Y-Low.Y<100 || High.X-Low.X>12800 || High.Y-Low.Y>12800) {Status=TEXT("Drag at least 1 m of paved area");DraftBlocked(Status,true);return false;}
 TArray<FFoundrylinePlacementRequest> Requests;
 for(const auto& O:Placement->GetObjects()) {
  const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D || !FFoundrylineDrivingLayout::IsPaved(D->FunctionId) || O.CompanyId!=Context.CompanyId || O.PlantId!=Context.PlantId) continue;
  const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));
  FVector2D Min(MAX_flt,MAX_flt),Max(-MAX_flt,-MAX_flt);
  for(int32 X:{Low.X,High.X}) for(int32 Y:{Low.Y,High.Y}) {const auto P=T.InverseTransformPosition(FVector(X,Y,0));Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);}
  FFoundrylineTruckZone Clip;Clip.Min={FMath::Max(0,FMath::RoundToInt32(Min.X/100)*100),FMath::Max(0,FMath::RoundToInt32(Min.Y/100)*100)};Clip.Max={FMath::Min(D->Footprint.Max.X,FMath::RoundToInt32(Max.X/100)*100),FMath::Min(D->Footprint.Max.Y,FMath::RoundToInt32(Max.Y/100)*100)};
  if(Clip.Max.X<=Clip.Min.X || Clip.Max.Y<=Clip.Min.Y) continue;
  auto Zones=O.TruckZones;
  if(Erase) {
   Zones.Reset();
   for(const auto& Z:O.TruckZones) {
    const FIntPoint A(FMath::Max(Z.Min.X,Clip.Min.X),FMath::Max(Z.Min.Y,Clip.Min.Y)),B(FMath::Min(Z.Max.X,Clip.Max.X),FMath::Min(Z.Max.Y,Clip.Max.Y));
    if(B.X<=A.X || B.Y<=A.Y) {Zones.Add(Z);continue;}
    auto Add=[&](FIntPoint Min,FIntPoint Max){if(Max.X>Min.X && Max.Y>Min.Y) {FFoundrylineTruckZone R;R.Min=Min;R.Max=Max;Zones.Add(R);}};
    Add(Z.Min,{A.X,Z.Max.Y});Add({B.X,Z.Min.Y},Z.Max);Add({A.X,Z.Min.Y},{B.X,A.Y});Add({A.X,B.Y},{B.X,Z.Max.Y});
   }
  } else {
   if(Zones.ContainsByPredicate([&](const auto& Z){return Z.Min.X<=Clip.Min.X && Z.Min.Y<=Clip.Min.Y && Z.Max.X>=Clip.Max.X && Z.Max.Y>=Clip.Max.Y;})) continue;
   Zones.RemoveAll([&](const auto& Z){return Clip.Min.X<=Z.Min.X && Clip.Min.Y<=Z.Min.Y && Clip.Max.X>=Z.Max.X && Clip.Max.Y>=Z.Max.Y;});Zones.Add(Clip);
  }
  if(Zones.Num()>64) {Status=TEXT("Too many zone fragments. Erase a larger area first");DraftBlocked(Status,true);return false;}
  FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::SetTruckZones;R.ObjectId=O.ObjectId;R.ExpectedRevision=Placement->GetRevision();R.TruckZones=Zones;Requests.Add(R);
 }
 if(Requests.IsEmpty()) {Status=TEXT("No changed, owned paved area in this rectangle");return false;}
 const auto Result=ExecuteConstructionBatch(Requests);
 if(!Result.IsSuccess() && Result.Error!=EFoundrylinePlacementError::Pending) {Status=PlacementErrorText(Result.Error);return false;}
 Status=Erase?TEXT("Truck access removed. Use Check Semi Route to verify connections"):TEXT("Truck access drawn. Use Check Semi Route to verify turning space");return true;
}
