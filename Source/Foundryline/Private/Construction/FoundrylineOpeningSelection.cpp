#include "Construction/FoundrylineLoadingBayLayout.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylineFacadeLayout.h"

const FFoundrylineOpening* AFoundrylineBuildSite::GetSelectedOpening() const
{
 const auto* O=GetSelectedObject();if(!O||!SelectedOpening.OpeningId.IsValid())return nullptr;
 const auto* Room=O->OfficeRooms.FindByPredicate([&](const auto& R){return R.SpaceId==SelectedRoom;});
 const auto& Openings=SelectedRoom.IsValid()&&Room?Room->Openings:O->Openings;
 return Openings.FindByPredicate([&](const auto& V){return V.OpeningId==SelectedOpening.OpeningId;});
}

FString AFoundrylineBuildSite::GetSelectedItemLabel() const
{
 const auto* V=GetSelectedOpening();if(!V)return FString();
 switch(V->Kind){case EFoundrylineOpeningKind::OfficeGlassWall:return TEXT("OFFICE GLASS WALL");case EFoundrylineOpeningKind::Personnel:return TEXT("PERSONNEL DOOR");case EFoundrylineOpeningKind::OfficeDoor:return TEXT("OFFICE DOOR");case EFoundrylineOpeningKind::Garage:return TEXT("GARAGE OPENING");case EFoundrylineOpeningKind::TruckGarage:return TEXT("TRUCK DOOR");case EFoundrylineOpeningKind::LoadingDock:return FFoundrylineLoadingBayLayout::Spec(V->LoadingTier).Name;default:return TEXT("WINDOW");}
}

const FFoundrylinePlacedObject* AFoundrylineBuildSite::PickOpening(FIntVector Cursor,FFoundrylineOpening& Opening,FGuid& RoomId) const
{
 if(!Placement||!bReady)return nullptr;const auto ContextNow=LocalContext();const FFoundrylinePlacedObject* BestObject=nullptr;double Best=85;
 for(const auto& O:Placement->GetObjects())
 {
  if(O.CompanyId!=ContextNow.CompanyId||O.PlantId!=ContextNow.PlantId)continue;
  const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D||D->FunctionId!=TEXT("BuildingShell"))continue;
  const FRotator Rotation(0,O.QuarterTurns*90,0);const FVector P=Rotation.UnrotateVector(FVector(Cursor-O.Anchor));
  auto Check=[&](const TArray<FFoundrylineOpening>& Values,FVector2D Min,FVector2D Max,FGuid Room)
  {
   for(const auto& V:Values)
   {
    const double Along=V.Edge%2==0?P.X-Min.X:P.Y-Min.Y;
    const double Across=V.Edge==0?P.Y-Min.Y:V.Edge==1?P.X-Max.X:V.Edge==2?P.Y-Max.Y:P.X-Min.X;
    double Distance=FMath::Abs(Across)+FMath::Max(0.,FMath::Abs(Along-V.OffsetCm-V.WidthCm()*.5)-V.WidthCm()*.5);
    if(V.LoadingTier&&!Room.IsValid()){const auto Bounds=FFoundrylineLoadingBayLayout::Bounds(*D,O,V);if(Cursor.X>=Bounds.Min.X&&Cursor.X<=Bounds.Max.X&&Cursor.Y>=Bounds.Min.Y&&Cursor.Y<=Bounds.Max.Y)Distance=FMath::Min(Distance,30.);}
    if(Distance>=Best)continue;
    if(!Room.IsValid())
    {
     const FVector Mid=V.Edge%2==0?FVector(V.OffsetCm+V.WidthCm()*.5,V.Edge==0?0:Max.Y,0):FVector(V.Edge==3?0:Max.X,V.OffsetCm+V.WidthCm()*.5,0);
     const FVector Out=V.Edge==0?FVector(0,-1,0):V.Edge==1?FVector(1,0,0):V.Edge==2?FVector(0,1,0):FVector(-1,0,0);
     const FVector Probe=FVector(O.Anchor)+Rotation.RotateVector(Mid+Out*40);bool Hidden=false;
     for(const auto& Other:Placement->GetObjects())
     {
      if(Other.ObjectId==O.ObjectId||Other.CompanyId!=O.CompanyId||Other.PlantId!=O.PlantId||(O.bOfficeAnnex&&!Other.bOfficeAnnex))continue;
      const auto* OD=Placement->FindDefinition(Other.DefinitionId);if(!OD||OD->FunctionId!=TEXT("BuildingShell")||Other.Anchor.Z+OD->Footprint.Max.Z<=O.Anchor.Z+20+V.BottomCm())continue;
      const auto B=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);
      if(Probe.X>B.Min.X&&Probe.X<B.Max.X&&Probe.Y>B.Min.Y&&Probe.Y<B.Max.Y){Hidden=true;break;}
     }
     if(Hidden)continue;
    }
    Best=Distance;BestObject=&O;Opening=V;RoomId=Room;
   }
  };
  Check(O.Openings,{0,0},{double(D->Footprint.Max.X),double(D->Footprint.Max.Y)},FGuid());
  for(const auto& Room:O.OfficeRooms)Check(Room.Openings,FVector2D(Room.Min),FVector2D(Room.Max),Room.SpaceId);
 }
 return BestObject;
}

bool AFoundrylineBuildSite::OpeningEditRequest(FIntVector Cursor,bool Remove,FFoundrylinePlacementRequest& R) const
{
 const auto* O=GetSelectedObject();const auto* Original=GetSelectedOpening();if(!O||!Original)return false;
 R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=O->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*O;
 auto* Room=R.Replacement.OfficeRooms.FindByPredicate([&](const auto& V){return V.SpaceId==SelectedRoom;});
 auto& Values=Room?Room->Openings:R.Replacement.Openings;
 if(Remove){Values.RemoveAll([&](const auto& V){return V.OpeningId==Original->OpeningId;});return true;}
 const auto* D=Placement->FindDefinition(O->DefinitionId);const FVector2D Min=Room?FVector2D(Room->Min):FVector2D::ZeroVector,Max=Room?FVector2D(Room->Max):FVector2D(D->Footprint.Max.X,D->Footprint.Max.Y);
 const FVector P=FRotator(0,O->QuarterTurns*90,0).UnrotateVector(FVector(Cursor-O->Anchor))-FVector(Min,0);
 const FVector2D Size=Max-Min;const double Distances[]={FMath::Abs(P.Y),FMath::Abs(P.X-Size.X),FMath::Abs(P.Y-Size.Y),FMath::Abs(P.X)};
 uint8 Edge=0;for(uint8 E=1;E<4;++E)if(Distances[E]<Distances[Edge])Edge=E;
 if(Distances[Edge]>200)return false;
 auto* V=Values.FindByPredicate([&](const auto& Item){return Item.OpeningId==Original->OpeningId;});if(!V)return false;
 const int Snap=V->Kind==EFoundrylineOpeningKind::OfficeGlassWall?25:100;V->Edge=Edge;V->OffsetCm=FMath::RoundToInt(((Edge%2==0?P.X:P.Y)-V->WidthCm()*.5)/Snap)*Snap;V->bAutomatic=false;
 if(!Room&&Original->Kind!=EFoundrylineOpeningKind::ShopWindow)FFoundrylineFacadeLayout::Reflow(R.Replacement,D->Footprint.Max.X,D->Footprint.Max.Y,D->Footprint.Max.Z);
 return true;
}

void AFoundrylineBuildSite::RecordOpeningOutline(const FFoundrylinePlacedObject& O,const FFoundrylineOpening& V,FGuid RoomId,bool Hover,bool Valid)
{
 const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D)return;
 if(V.LoadingTier){const auto Bay=FFoundrylineLoadingBayLayout::Bounds(*D,O,V);SelectionOutlines.Add({FVector2D(Bay.Min.X,Bay.Min.Y),FVector2D(Bay.Max.X,Bay.Max.Y),22,Hover,0,!Valid});return;}
 const auto* Room=O.OfficeRooms.FindByPredicate([&](const auto& R){return R.SpaceId==RoomId;});
 const FVector2D Min=Room?FVector2D(Room->Min):FVector2D::ZeroVector,Max=Room?FVector2D(Room->Max):FVector2D(D->Footprint.Max.X,D->Footprint.Max.Y);
 const double Along=(V.Edge%2==0?Min.X:Min.Y)+V.OffsetCm+V.WidthCm()*.5;
 const FVector Center=V.Edge%2==0?FVector(Along,V.Edge==0?Min.Y:Max.Y,0):FVector(V.Edge==3?Min.X:Max.X,Along,0);
 const FVector Half=V.Edge%2==0?FVector(V.WidthCm()*.5+10,28,0):FVector(28,V.WidthCm()*.5+10,0);
 FBox2D Bounds(ForceInit);const FRotator Rotation(0,O.QuarterTurns*90,0);
 for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})Bounds+=FVector2D(FVector(O.Anchor)+Rotation.RotateVector(Center+FVector(Half.X*X,Half.Y*Y,0)));
 SelectionOutlines.Add({Bounds.Min,Bounds.Max,float(O.Anchor.Z+20+V.BottomCm()-5),Hover,float(V.HeightCm()+10),!Valid});
}
