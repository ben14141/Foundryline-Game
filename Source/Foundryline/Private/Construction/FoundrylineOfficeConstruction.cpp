#include "Construction/FoundrylineBuildSite.h"

const FFoundrylinePlacedObject* AFoundrylineBuildSite::PickOffice(FIntVector Cursor) const
{
	if(!bReady) return nullptr;
	return Placement->GetObjects().FindByPredicate([&](const auto& O)
	{
		if(!O.bOfficeAnnex || O.CompanyId!=Context.CompanyId || O.PlantId!=Context.PlantId) return false;
		const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D) return false;
		const auto P=FRotator(0,O.QuarterTurns*90,0).UnrotateVector(FVector(Cursor-O.Anchor));
		return P.X>=-100 && P.Y>=-100 && P.X<=D->Footprint.Max.X+100 && P.Y<=D->Footprint.Max.Y+100;
	});
}

bool AFoundrylineBuildSite::OfficeRectangle(FName Tool,FIntVector Start,FIntVector End,bool bPreview)
{
	if(Tool==TEXT("OfficeArea"))
	{
		if(bPreview) {PreviewRectangle(TEXT("Office"),Start,End);return true;}
		const bool Built=BuildRectangle(TEXT("Office"),Start,End);Status=Built?TEXT("Office built. Draw rooms inside it."):TEXT("Office must be at least 4 x 4 m, on your property and clear of construction.");return Built;
	}
	const auto* Host=PickOffice(Start);
	if(!Host || !Host->bOfficeAnnex) {Status=TEXT("Draw rooms inside an exterior office annex");if(bPreview) HidePreview();DraftBlocked(Status,!bPreview);return false;}
	const FRotator Rot(0,Host->QuarterTurns*90,0);
	const FVector A=Rot.UnrotateVector(FVector(Start-Host->Anchor)),B=Rot.UnrotateVector(FVector(End-Host->Anchor));
	FFoundrylineOfficeSpace Space;Space.SpaceId=FGuid::NewGuid();
	Space.Min={FMath::RoundToInt32(FMath::Min(A.X,B.X)/100)*100,FMath::RoundToInt32(FMath::Min(A.Y,B.Y)/100)*100};
	Space.Max={FMath::RoundToInt32(FMath::Max(A.X,B.X)/100)*100,FMath::RoundToInt32(FMath::Max(A.Y,B.Y)/100)*100};
	const auto* Def=Placement->FindDefinition(Host->DefinitionId);
	if(FMath::Abs(Space.Min.X)<=100) Space.Min.X=0;if(FMath::Abs(Space.Min.Y)<=100) Space.Min.Y=0;
	if(FMath::Abs(Space.Max.X-Def->Footprint.Max.X)<=100) Space.Max.X=Def->Footprint.Max.X;
	if(FMath::Abs(Space.Max.Y-Def->Footprint.Max.Y)<=100) Space.Max.Y=Def->Footprint.Max.Y;

	FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Host->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*Host;
	if(Tool==TEXT("OfficeArea")) R.Replacement.OfficeAreas.Add(Space);else R.Replacement.OfficeRooms.Add(Space);
	const auto Valid=ValidateConstruction(R);
	if(bPreview)
	{
		auto Display=R.Replacement;Display.OfficeAreas.Reset();Display.OfficeRooms.Reset();
		if(Tool==TEXT("OfficeArea")) Display.OfficeAreas={Space};else Display.OfficeRooms={Space};
		ShowPreview(Display,Valid.IsSuccess(),FString::Printf(TEXT("Office_%s_%s_%s_%lld"),*Tool.ToString(),*Start.ToString(),*End.ToString(),Placement->GetRevision()));
		Status=Valid.IsSuccess()?(Tool==TEXT("OfficeArea")?TEXT("Release to convert to open office floor. Then draw rooms."):TEXT("Release to create a 3 m high room. Then add doors and windows.")):TEXT("Rooms: at least 4 m, inside the office. Snap to the perimeter to reuse outside walls.");return Valid.IsSuccess();
	}
	FFoundrylinePlacementRequest Inverse=R;Inverse.Replacement=*Host;
	const auto Result=ExecuteConstruction(R);if(!Result.IsSuccess()) {Status=TEXT("Office/room blocked: check bounds, size and overlaps");return false;}
	RecordUndo(Inverse);HidePreview();Status=Tool==TEXT("OfficeArea")?TEXT("Open office created. Draw rooms within it."):TEXT("Room created. Add an office door and interior windows.");return true;
}

bool AFoundrylineBuildSite::OfficeOpening(FName Tool,FIntVector Cursor,bool bPreview)
{
	const auto* Host=PickOffice(Cursor);if(!Host) {if(bPreview) HidePreview();DraftBlocked(Status,!bPreview);return false;}
	const FVector P=FRotator(0,Host->QuarterTurns*90,0).UnrotateVector(FVector(Cursor-Host->Anchor));
	int32 RoomIndex=INDEX_NONE;FFoundrylineOpening Opening;double Best=151;
	Opening.OpeningId=FGuid::NewGuid();Opening.Kind=Tool==TEXT("RoomDoor")?EFoundrylineOpeningKind::OfficeDoor:EFoundrylineOpeningKind::ShopWindow;
	for(int32 I=0;I<Host->OfficeRooms.Num();++I)
	{
		const auto& Room=Host->OfficeRooms[I];
		if(P.X<Room.Min.X-150 || P.Y<Room.Min.Y-150 || P.X>Room.Max.X+150 || P.Y>Room.Max.Y+150) continue;
		const double Dist[]={FMath::Abs(P.Y-Room.Min.Y),FMath::Abs(P.X-Room.Max.X),FMath::Abs(P.Y-Room.Max.Y),FMath::Abs(P.X-Room.Min.X)};
		for(uint8 Edge=0;Edge<4;++Edge) if(Dist[Edge]<Best)
		{
			Best=Dist[Edge];RoomIndex=I;Opening.Edge=Edge;
			Opening.OffsetCm=FMath::RoundToInt32(((Edge%2==0?P.X-Room.Min.X:P.Y-Room.Min.Y)-Opening.WidthCm()/2.)/100)*100;
		}
	}
	if(RoomIndex==INDEX_NONE && Tool==TEXT("RoomWindow"))
 {
  const auto* D=Placement->FindDefinition(Host->DefinitionId);
  const double Dist[]={FMath::Abs(P.Y),FMath::Abs(P.X-D->Footprint.Max.X),FMath::Abs(P.Y-D->Footprint.Max.Y),FMath::Abs(P.X)};
  for(uint8 E=0;E<4;++E) if(Dist[E]<Best) {Best=Dist[E];Opening.Edge=E;Opening.OffsetCm=FMath::RoundToInt32(((E%2==0?P.X:P.Y)-100)/100)*100;}
 }
 if(RoomIndex==INDEX_NONE && Best>=151) {Status=TEXT("Point at an office wall to add a manual window");if(bPreview) HidePreview();DraftBlocked(Status,!bPreview);return false;}
	FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Host->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*Host;if(RoomIndex!=INDEX_NONE) R.Replacement.OfficeRooms[RoomIndex].Openings.Add(Opening);
	FFoundrylineOfficeSpace Source;if(RoomIndex!=INDEX_NONE) Source=Host->OfficeRooms[RoomIndex];else {Source.Min={0,0};const auto* D=Placement->FindDefinition(Host->DefinitionId);Source.Max={D->Footprint.Max.X,D->Footprint.Max.Y};}
	const auto* Def=Placement->FindDefinition(Host->DefinitionId);
	const bool Exterior=Opening.Edge==0?Source.Min.Y==0:Opening.Edge==1?Source.Max.X==Def->Footprint.Max.X:Opening.Edge==2?Source.Max.Y==Def->Footprint.Max.Y:Source.Min.X==0;
	if(Exterior)
	{
		auto Outer=Opening;Outer.OffsetCm+=Opening.Edge%2==0?Source.Min.X:Source.Min.Y;
		R.Replacement.Openings.RemoveAll([&](const auto& V){return V.bAutomatic && V.Edge==Outer.Edge && V.OffsetCm<Outer.OffsetCm+Outer.WidthCm()+30 && V.OffsetCm+V.WidthCm()+30>Outer.OffsetCm;});
		R.Replacement.Openings.Add(Outer);
	}

	for(int32 I=0;I<R.Replacement.OfficeRooms.Num();++I)
	{
		if(I==RoomIndex) continue;auto& Neighbour=R.Replacement.OfficeRooms[I];
		const bool Touch=Opening.Edge==0?Source.Min.Y==Neighbour.Max.Y:Opening.Edge==1?Source.Max.X==Neighbour.Min.X:Opening.Edge==2?Source.Max.Y==Neighbour.Min.Y:Source.Min.X==Neighbour.Max.X;
		const int32 Along=(Opening.Edge%2==0?Source.Min.X:Source.Min.Y)+Opening.OffsetCm;
		const int32 Low=Opening.Edge%2==0?Neighbour.Min.X:Neighbour.Min.Y,High=Opening.Edge%2==0?Neighbour.Max.X:Neighbour.Max.Y;
		if(Touch && Along<High && Along+Opening.WidthCm()>Low)
		{
			auto Mirror=Opening;Mirror.OpeningId=FGuid::NewGuid();Mirror.Edge=(Opening.Edge+2)%4;Mirror.OffsetCm=Along-Low;Mirror.bAutomatic=true;Neighbour.Openings.Add(Mirror);
		}
	}
	const auto Valid=ValidateConstruction(R);
	if(bPreview)
	{
		auto Display=R.Replacement;Display.OfficeAreas.Reset();if(RoomIndex!=INDEX_NONE) Display.OfficeRooms={R.Replacement.OfficeRooms[RoomIndex]};else Display.OfficeRooms.Reset();
  if(Exterior) {Display.Openings={R.Replacement.Openings.Last()};ShowPreview(Display,Valid.IsSuccess(),FString::Printf(TEXT("Door_OfficeWindow_%s_%d_%d_%lld"),*Host->ObjectId.ToString(),Opening.Edge,Opening.OffsetCm,Placement->GetRevision()));return Valid.IsSuccess();}
		ShowPreview(Display,Valid.IsSuccess(),FString::Printf(TEXT("OfficeOpening_%s_%s_%d_%d_%d_%lld"),*Tool.ToString(),*Host->ObjectId.ToString(),RoomIndex,Opening.Edge,Opening.OffsetCm,Placement->GetRevision()));
		Status=Valid.IsSuccess()?TEXT("Click to install office opening"):TEXT("Opening blocked: leave 1 m from corners and space between openings");return Valid.IsSuccess();
	}
	FFoundrylinePlacementRequest Inverse=R;Inverse.Replacement=*Host;const auto Result=ExecuteConstruction(R);
	if(!Result.IsSuccess()) {Status=TEXT("Office opening blocked or unavailable");return false;}RecordUndo(Inverse);HidePreview();Status=TEXT("Office opening installed. Z undoes.");return true;
}
