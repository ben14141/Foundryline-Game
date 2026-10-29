#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylineFacadeLayout.h"
#include "Construction/FoundrylineSizing.h"
#include "Construction/FoundrylineBuildingConnections.h"

namespace {
 // Resizing changes the local origin, not the world position of authored routing.
 bool ResizePavementRouting(const FFoundrylinePlacedObject& Old,FFoundrylinePlacedObject& New,int32 Width,int32 Depth) {
  auto Local=[&](FIntPoint P){const auto World=Old.Anchor+FFoundrylineGrid::RotateOffset({P.X,P.Y,0},Old.QuarterTurns);const auto Q=FFoundrylineGrid::RotateOffset(World-New.Anchor,(4-New.QuarterTurns%4)%4);return FIntPoint(Q.X,Q.Y);};
  New.TruckZones.Reset();
  for(const auto& Zone:Old.TruckZones) {
   FIntPoint Min(MAX_int32,MAX_int32),Max(MIN_int32,MIN_int32);
   for(int32 X:{Zone.Min.X,Zone.Max.X}) for(int32 Y:{Zone.Min.Y,Zone.Max.Y}) {const auto P=Local({X,Y});Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);}
   FFoundrylineTruckZone Clipped;Clipped.Min={FMath::Clamp(Min.X,0,Width),FMath::Clamp(Min.Y,0,Depth)};Clipped.Max={FMath::Clamp(Max.X,0,Width),FMath::Clamp(Max.Y,0,Depth)};
   if(Clipped.Min.X<Clipped.Max.X && Clipped.Min.Y<Clipped.Max.Y) New.TruckZones.Add(Clipped);
  }
  for(auto& Node:New.RouteNodes) {Node.Position=Local(Node.Position);if(Node.Position.X<0 || Node.Position.Y<0 || Node.Position.X>Width || Node.Position.Y>Depth) return false;}
  New.Crosswalks.Reset();for(int32 Y:Old.Crosswalks) {const int32 Shifted=Local({0,Y}).Y;if(Shifted>=200 && Shifted<=Depth-200) New.Crosswalks.Add(Shifted);}
  return true;
 }
 // Growing from the south or west moves the local origin. Doors, windows and
 // office rooms keep their world positions, and a whole-office open floor grows with it.
 void RebaseInterior(FFoundrylinePlacedObject& O,FIntPoint Delta,FIntPoint OldSize,FIntPoint NewSize) {
  for(auto& V:O.Openings) V.OffsetCm-=V.Edge%2==0?Delta.X:Delta.Y;
  for(auto& Room:O.OfficeRooms) {Room.Min-=Delta;Room.Max-=Delta;}
  for(auto& Area:O.OfficeAreas) {
   if(Area.Min==FIntPoint::ZeroValue && Area.Max==OldSize) Area.Max=NewSize;
   else {Area.Min-=Delta;Area.Max-=Delta;}
  }
 }
}

const FFoundrylinePlacedObject* AFoundrylineBuildSite::Pick(FIntVector Cursor) const
{
	if(!bReady) return nullptr;
	const auto Local=LocalContext();
	for(const auto& O:Placement->GetObjects())
	{
		if(O.CompanyId!=Local.CompanyId || O.PlantId!=Local.PlantId) continue;
		const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D) continue;
		const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,O.Anchor,O.QuarterTurns);
		if(Cursor.X>=B.Min.X && Cursor.Y>=B.Min.Y && Cursor.X<=B.Max.X && Cursor.Y<=B.Max.Y) return &O;
	}
	return nullptr;
}

const FFoundrylinePlacedObject* AFoundrylineBuildSite::GetSelectedObject() const
{
 return Placement?Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==SelectedObject;}):nullptr;
}

const FFoundrylinePlacedObject* AFoundrylineBuildSite::GetSelectedBuilding() const
{
 if(!bReady) return nullptr;
 const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==SelectedObject;});
 return O && Placement->FindDefinition(O->DefinitionId)->FunctionId==TEXT("BuildingShell")?O:nullptr;
}
bool AFoundrylineBuildSite::PaintSelected(int32 Channel,FLinearColor Color)
{
 const auto* O=GetSelectedBuilding();if(!O || Channel<0 || Channel>2) {Status=TEXT("Select a shop or office in Edit, then choose wall colors.");return false;}
 FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::SetBuildingColors;R.ObjectId=O->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Colors=O->Colors;
 FFoundrylinePlacementRequest Inverse=R;
 if(Channel==0) R.Colors.Exterior=Color;else if(Channel==1) R.Colors.Interior=Color;else R.Colors.Trim=Color;
 const auto Result=ExecuteConstruction(R);if(SentToHost(Result.Error)) return true;
 if(!Result.IsSuccess()) {Status=PlacementErrorText(Result.Error);return false;}
 RecordUndo(Inverse);HidePreview();Status=TEXT("Building colors updated. Z undoes.");return true;
}

bool AFoundrylineBuildSite::BeginEdit(FName Tool,FIntVector Cursor)
{
	HidePreview(); SelectedObject.Invalidate(); SelectedOpening=FFoundrylineOpening();
	const auto* O=Pick(Cursor);
	if(!O) {Status=TEXT("Point inside a building, road or parking lot to select it");return false;}
	SelectedObject=O->ObjectId;
	if(Tool==TEXT("MoveDoor"))
	{
		const FVector P=FRotator(0,O->QuarterTurns*90,0).UnrotateVector(FVector(Cursor-O->Anchor));
		const auto* D=Placement->FindDefinition(O->DefinitionId); double Best=251;
		for(const auto& Opening:O->Openings)
		{
			if(Opening.Kind==EFoundrylineOpeningKind::ShopWindow) continue;
			const double Along=Opening.Edge%2==0?P.X:P.Y;
			const double Perp=Opening.Edge==0?P.Y:Opening.Edge==1?P.X-D->Footprint.Max.X:Opening.Edge==2?P.Y-D->Footprint.Max.Y:P.X;
			const double Distance=FMath::Abs(Perp)+FMath::Max(0.,FMath::Abs(Along-Opening.OffsetCm-Opening.WidthCm()/2.)-Opening.WidthCm()/2.);
			if(Distance<Best) {Best=Distance;SelectedOpening=Opening;}
		}
		if(!SelectedOpening.OpeningId.IsValid()) {Status=TEXT("Start the drag on a personnel or garage opening");return false;}
	}
	const auto* D=Placement->FindDefinition(O->DefinitionId);
	Status=FString::Printf(TEXT("Selected %s: %d x %d m. Drag to edit; Z undoes."),*D->FunctionId.ToString(),D->Footprint.Max.X/100,D->Footprint.Max.Y/100);
	if(Tool!=TEXT("Select")) PreviewEdit(Tool,Cursor,Cursor);return true;
}

bool AFoundrylineBuildSite::EditRequest(FName Tool,FIntVector Start,FIntVector End,FFoundrylinePlacementRequest& R)
{
	const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==SelectedObject;});
	if(!O) return false;
	R.ObjectId=O->ObjectId;R.ExpectedRevision=Placement->GetRevision();
	if(Tool==TEXT("Demolish")) {R.Action=EFoundrylinePlacementAction::Remove;return true;}
	if(Tool==TEXT("Rotate"))
    {
        R.Action=EFoundrylinePlacementAction::ReplaceObject;R.Replacement=*O;
        const auto* D=Placement->FindDefinition(O->DefinitionId);
        const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,O->Anchor,O->QuarterTurns);
        R.Replacement.QuarterTurns=(O->QuarterTurns+1)%4;
        const auto N=FFoundrylineGrid::RotateBounds(D->Footprint,FIntVector::ZeroValue,R.Replacement.QuarterTurns);
        R.Replacement.Anchor=B.Min-N.Min;return true;
    }
    if(Tool==TEXT("Duplicate") || Tool==TEXT("Move"))
    {
        // Moved sections magnetize like new drafts; a duplicate may sit against its source.
        const auto* D=Placement->FindDefinition(O->DefinitionId);
        const FIntVector Anchor=O->Anchor+End-Start;
        const auto Offset=FFoundrylineBuildingConnections::SnapOffset(FFoundrylineGrid::RotateBounds(D->Footprint,Anchor,O->QuarterTurns),
            SnapTargets(D->FunctionId==TEXT("BuildingShell"),Tool==TEXT("Move")?O->ObjectId:FGuid()),FFoundrylineBuildingConnections::MagnetCm,FFoundrylineGrid::SnapStep(D->Snap));
        R.Action=Tool==TEXT("Move")?EFoundrylinePlacementAction::Move:EFoundrylinePlacementAction::Duplicate;R.Anchor=Anchor+Offset;return true;
    }
	if(Tool==TEXT("Resize"))
	{
		const auto* D=Placement->FindDefinition(O->DefinitionId);
		const bool Road=D->FunctionId==TEXT("Road2") || D->FunctionId==TEXT("Road4") || D->FunctionId==TEXT("Service") || D->FunctionId==TEXT("Driveway") || D->FunctionId==TEXT("TruckAccess");
		const int32 W=D->Footprint.Max.X,H=D->Footprint.Max.Y;
		// Grab the middle half of a wall to move only that wall; anywhere else drags the nearest corner.
		const FVector Grab=FRotator(0,O->QuarterTurns*90,0).UnrotateVector(FVector(Start-O->Anchor));
		auto Middle=[](double U,double Length){return U>Length/4 && U<Length*3/4;};
		const bool OnlyX=Middle(Grab.Y,H) && !Middle(Grab.X,W),OnlyY=Middle(Grab.X,W) && !Middle(Grab.Y,H);
        if(O->QuarterTurns || Road || D->FunctionId==TEXT("Walkway") || D->FunctionId==TEXT("FreightYard"))
        {
            const FRotator Rotation(0,O->QuarterTurns*90,0);
            const FVector A=Grab;FVector B=Rotation.UnrotateVector(FVector(End-O->Anchor));
            const FIntVector Fixed(A.X<W/2?W:0,A.Y<H/2?H:0,0);
            if(OnlyX) B.Y=Fixed.Y?0:H;
            if(OnlyY) B.X=Fixed.X?0:W;
            FIntVector Low(FMath::RoundToInt(FMath::Min(double(Fixed.X),B.X)/100)*100,FMath::RoundToInt(FMath::Min(double(Fixed.Y),B.Y)/100)*100,0);
            int32 Width=FMath::RoundToInt(FMath::Abs(B.X-Fixed.X)/100)*100,Depth=FMath::RoundToInt(FMath::Abs(B.Y-Fixed.Y)/100)*100;
            // Marked streets retain their lane width. Property truck access
            // can grow across either axis while keeping the opposite corner.
            if(Road && D->FunctionId!=TEXT("TruckAccess")) {Low.X=0;Width=W;}
            const FString T=D->FunctionId==TEXT("BuildingShell")?(O->bOfficeAnnex?TEXT("Office"):D->Footprint.Max.Z>620?TEXT("Shop2"):TEXT("Building")):D->FunctionId.ToString();
            const FName Id(*FString::Printf(TEXT("%s_%d_%d"),*T,Width,Depth));
            if(!Placement->EnsureGeneratedDefinition(Id)) {Status=FFoundrylineSizing::Limits(FName(*T));DraftBlocked(Status);return false;}
            R.Action=EFoundrylinePlacementAction::ReplaceObject;R.Replacement=*O;R.Replacement.DefinitionId=Id;
            const auto Delta=Rotation.RotateVector(FVector(Low));R.Replacement.Anchor=O->Anchor+FIntVector(FMath::RoundToInt(Delta.X),FMath::RoundToInt(Delta.Y),0);
            if(D->FunctionId!=TEXT("BuildingShell") && !ResizePavementRouting(*O,R.Replacement,Width,Depth)) {Status=TEXT("Move or remove route points outside the new pavement before shrinking it");DraftBlocked(Status);return false;}
            R.Replacement.Openings.RemoveAll([](const auto& V){return V.bAutomatic;});
            if(D->FunctionId==TEXT("BuildingShell")) {RebaseInterior(R.Replacement,{Low.X,Low.Y},{W,H},{Width,Depth});FFoundrylineFacadeLayout::Reflow(R.Replacement,Width,Depth,D->Footprint.Max.Z);}
            return true;
        }
		// Drag the nearest corner (or one wall); the opposite side remains fixed.
		const FIntVector Fixed(O->Anchor.X+(Start.X<O->Anchor.X+W/2?W:0),O->Anchor.Y+(Start.Y<O->Anchor.Y+H/2?H:0),0);
		FIntVector Target=End;
		if(OnlyX) Target.Y=Fixed.Y==O->Anchor.Y?O->Anchor.Y+H:O->Anchor.Y;
		if(OnlyY) Target.X=Fixed.X==O->Anchor.X?O->Anchor.X+W:O->Anchor.X;
		// Only the edges being dragged magnetize, so the fixed side never shifts.
		const uint8 Moving=uint8((OnlyY?0:(Target.X>=Fixed.X?FFoundrylineBuildingConnections::MaxX:FFoundrylineBuildingConnections::MinX))|(OnlyX?0:(Target.Y>=Fixed.Y?FFoundrylineBuildingConnections::MaxY:FFoundrylineBuildingConnections::MinY)));
		const FName T=D->FunctionId==TEXT("Parking")?FName(TEXT("Parking")):O->bOfficeAnnex?FName(TEXT("Office")):D->Footprint.Max.Z>=1220?FName(TEXT("Shop2")):FName(TEXT("Building"));
		FFoundrylinePlacementRequest Draft;
		TGuardValue<uint8> DrawingRotation(DraftAreaRotation,0);
        if(!RectangleRequest(T,Fixed,Target,Draft,Moving,false)) return false;
		R.Action=EFoundrylinePlacementAction::ReplaceObject;R.Replacement=*O;
		R.Replacement.DefinitionId=Draft.DefinitionId;R.Replacement.Anchor=Draft.Anchor;R.Replacement.QuarterTurns=Draft.QuarterTurns;
		// Existing manual openings retain their identities and world positions; invalid reductions are blocked.
		R.Replacement.Openings=O->Openings.FilterByPredicate([](const auto& V){return !V.bAutomatic;});
        const auto* Resized=Placement->FindDefinition(Draft.DefinitionId);
        if(D->FunctionId!=TEXT("BuildingShell") && !ResizePavementRouting(*O,R.Replacement,Resized->Footprint.Max.X,Resized->Footprint.Max.Y)) {Status=TEXT("Move or remove route points outside the new pavement before shrinking it");DraftBlocked(Status);return false;}
        if(D->FunctionId==TEXT("BuildingShell"))
        {
            RebaseInterior(R.Replacement,{Draft.Anchor.X-O->Anchor.X,Draft.Anchor.Y-O->Anchor.Y},{W,H},{Resized->Footprint.Max.X,Resized->Footprint.Max.Y});
            FFoundrylineFacadeLayout::Reflow(R.Replacement,Resized->Footprint.Max.X,Resized->Footprint.Max.Y,Resized->Footprint.Max.Z);
        }
		return true;
	}
	return false;
}

void AFoundrylineBuildSite::PreviewEdit(FName Tool,FIntVector Start,FIntVector End)
{
	if(Tool==TEXT("Select")) return;
	if(Tool==TEXT("MoveDoor"))
	{
		FFoundrylinePlacementRequest Add;
		const FName T=(SelectedOpening.Kind==EFoundrylineOpeningKind::Personnel || SelectedOpening.Kind==EFoundrylineOpeningKind::OfficeDoor)?FName(TEXT("Personnel")):SelectedOpening.Kind==EFoundrylineOpeningKind::TruckGarage?FName(TEXT("TruckDoor")):FName(TEXT("Garage"));
		if(!DoorRequest(T,End,Add) || Add.ObjectId!=SelectedObject) {HidePreview();Status=TEXT("Drag the door to an exterior wall of the same section");return;}
		FFoundrylinePlacementRequest Remove=Add;Remove.Action=EFoundrylinePlacementAction::RemoveOpening;Remove.Opening=SelectedOpening;
		Add.Opening.OpeningId=SelectedOpening.OpeningId;
		const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==SelectedObject;});if(!O) return;
		const auto Result=Placement->PreviewBatch({Remove,Add},LocalContext(),true);RecordConstructionResult(Result.Error,false);
		auto Draft=*O;Draft.Openings={Add.Opening};
		ShowPreview(Draft,Result.IsSuccess(),FString::Printf(TEXT("Door_Move_%s_%d_%d_%lld"),*SelectedObject.ToString(),Add.Opening.Edge,Add.Opening.OffsetCm,Placement->GetRevision()));
		Status=Result.IsSuccess()?TEXT("Release to relocate door. Its old opening closes."):TEXT("Edit blocked: overlap, bounds or opening clearance");return;
	}
	FFoundrylinePlacementRequest R;if(!EditRequest(Tool,Start,End,R)) {HidePreview();return;}
	const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==SelectedObject;});if(!O) return;
	auto Draft=R.Action==EFoundrylinePlacementAction::ReplaceObject?R.Replacement:*O;
	if((R.Action==EFoundrylinePlacementAction::Move || R.Action==EFoundrylinePlacementAction::Duplicate)) Draft.Anchor=R.Anchor;
	const auto Result=ValidateConstruction(R);
	ShowPreview(Draft,Result.IsSuccess() && Tool!=TEXT("Demolish"),FString::Printf(TEXT("Edit_%s_%s_%s_%s_%lld"),*Tool.ToString(),*Draft.DefinitionId.ToString(),*Draft.Anchor.ToString(),*SelectedObject.ToString(),Placement->GetRevision()));
	Status=Result.IsSuccess()?(Tool==TEXT("Demolish")?TEXT("Release to remove this section. Z restores it."):TEXT("Release to apply edit. Z undoes.")):TEXT("Edit blocked: overlap, bounds or opening clearance");
}

bool AFoundrylineBuildSite::CommitEdit(FName Tool,FIntVector Start,FIntVector End)
{
	if(Tool==TEXT("Select")) return SelectedObject.IsValid();
	const auto* O=Placement->GetObjects().FindByPredicate([&](const auto& V){return V.ObjectId==SelectedObject;});if(!O) return false;
	const auto Previous=*O;
	FFoundrylinePlacementRequest Inverse;Inverse.ObjectId=SelectedObject;Inverse.Action=EFoundrylinePlacementAction::ReplaceObject;Inverse.Replacement=Previous;
	if(Tool==TEXT("MoveDoor"))
	{
		FFoundrylinePlacementRequest Add;
		const FName T=(SelectedOpening.Kind==EFoundrylineOpeningKind::Personnel || SelectedOpening.Kind==EFoundrylineOpeningKind::OfficeDoor)?FName(TEXT("Personnel")):SelectedOpening.Kind==EFoundrylineOpeningKind::TruckGarage?FName(TEXT("TruckDoor")):FName(TEXT("Garage"));
		if(!DoorRequest(T,End,Add) || Add.ObjectId!=SelectedObject) return false;
		FFoundrylinePlacementRequest Remove=Add;Remove.Action=EFoundrylinePlacementAction::RemoveOpening;Remove.Opening=SelectedOpening;Add.Opening.OpeningId=SelectedOpening.OpeningId;
		const auto Result=ExecuteConstructionBatch({Remove,Add});RecordConstructionResult(Result.Error,true);
		if(SentToHost(Result.Error)) return true;
		if(!Result.IsSuccess()) {Status=PlacementErrorText(Result.Error);return false;}
	}
	else
	{
		FFoundrylinePlacementRequest R;if(!EditRequest(Tool,Start,End,R)) return false;
		const auto Result=ExecuteConstruction(R);if(SentToHost(Result.Error)) {if(R.Action==EFoundrylinePlacementAction::Remove) SelectedObject.Invalidate();return true;}
		if(!Result.IsSuccess()) {Status=PlacementErrorText(Result.Error);return false;}
		if(R.Action==EFoundrylinePlacementAction::Remove) {Inverse.Action=EFoundrylinePlacementAction::RestoreRemoved;SelectedObject.Invalidate();}
        if(R.Action==EFoundrylinePlacementAction::Duplicate) {Inverse.Action=EFoundrylinePlacementAction::Remove;Inverse.ObjectId=Result.ObjectId;SelectedObject=Result.ObjectId;}
	}
	RecordUndo(Inverse);HidePreview();Status=TEXT("Edit applied. Z undoes.");return true;
}

bool AFoundrylineBuildSite::ApplyGroundTool(FName Tool,FIntVector Cursor,bool bPreview)
{
	const auto* O=Pick(Cursor);if(!O) {if(bPreview) HidePreview();return false;}
	const auto* D=Placement->FindDefinition(O->DefinitionId);
	FFoundrylinePlacementRequest R;R.ObjectId=O->ObjectId;R.ExpectedRevision=Placement->GetRevision();
	auto Draft=*O;
	if(Tool==TEXT("Landscape") && D->FunctionId==TEXT("Parking"))
	{
		R.Action=EFoundrylinePlacementAction::SetLandscaping;R.bLandscaping=!O->bLandscaping;Draft.bLandscaping=R.bLandscaping;
	}
	else if(Tool==TEXT("Crosswalk") && (D->FunctionId==TEXT("Road2") || D->FunctionId==TEXT("Road4")) && D->FunctionId!=TEXT("Service") && D->FunctionId!=TEXT("Driveway"))
	{
		const FVector P=FRotator(0,O->QuarterTurns*90,0).UnrotateVector(FVector(Cursor-O->Anchor));
		const int32 Y=FMath::RoundToInt(P.Y/100)*100;
		R.Action=EFoundrylinePlacementAction::SetCrosswalks;R.Crosswalks=O->Crosswalks;
		if(const auto* V=R.Crosswalks.FindByPredicate([&](int32 A){return FMath::Abs(A-Y)<300;})) R.Crosswalks.Remove(*V);else R.Crosswalks.Add(Y);
		Draft.Crosswalks=R.Crosswalks;
	}
	else {Status=Tool==TEXT("Landscape")?TEXT("Click a parking lot to toggle its grass islands and edging"):TEXT("Click a two- or four-lane road to add/remove a crosswalk");if(bPreview) HidePreview();return false;}
	const auto Valid=ValidateConstruction(R);
	if(bPreview)
	{
		ShowPreview(Draft,Valid.IsSuccess(),FString::Printf(TEXT("Ground_%s_%s_%s_%lld"),*Tool.ToString(),*O->ObjectId.ToString(),*Cursor.ToString(),Placement->GetRevision()));
		Status=Valid.IsSuccess()?TEXT("Click to apply the hologram. Z undoes."):TEXT("Grounds edit blocked");return Valid.IsSuccess();
	}
	FFoundrylinePlacementRequest Inverse;Inverse.Action=EFoundrylinePlacementAction::ReplaceObject;Inverse.ObjectId=O->ObjectId;Inverse.Replacement=*O;
	const auto Result=ExecuteConstruction(R);if(SentToHost(Result.Error)) return true;
	if(!Result.IsSuccess()) {Status=PlacementErrorText(Result.Error);return false;}
	RecordUndo(Inverse);HidePreview();Status=TEXT("Grounds updated. Z undoes.");return true;
}
