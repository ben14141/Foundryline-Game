#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylineFacadeLayout.h"
#include "Engine/World.h"
#include "Pathing/FoundrylineDrivingLayout.h"

namespace
{
	void InsertOpening(TArray<FFoundrylineOpening>& Openings, const FFoundrylineOpening& New)
	{
		// Doors replace automatic facade bays; manual openings always require validation.
		if (New.Kind != EFoundrylineOpeningKind::ShopWindow)
			Openings.RemoveAll([&](const auto& O) { return O.bAutomatic && O.Kind == EFoundrylineOpeningKind::ShopWindow
				&& O.Edge == New.Edge && New.OffsetCm < O.OffsetCm+O.WidthCm()+30 && New.OffsetCm+New.WidthCm()+30 > O.OffsetCm; });
		Openings.Add(New);
	}
    FFoundrylineGridBounds DockBounds(const UFoundrylinePlaceableDefinition& Def,const FFoundrylinePlacedObject& Object,const FFoundrylineOpening& Opening)
    {
     const int32 Along=Opening.OffsetCm+Opening.WidthCm()/2,Width=Def.Footprint.Max.X,Depth=Def.Footprint.Max.Y;
     FFoundrylineGridBounds Bounds;
     Bounds.Min=Opening.Edge%2==0?FIntVector(Along-250,Opening.Edge==0?-330:Depth,0):FIntVector(Opening.Edge==3?-330:Width,Along-250,0);
     Bounds.Max=Bounds.Min+(Opening.Edge%2==0?FIntVector(500,330,600):FIntVector(330,500,600));
     return FFoundrylineGrid::RotateBounds(Bounds,Object.Anchor,Object.QuarterTurns);
    }
	bool Road(FName F) {return F==TEXT("Service") || F==TEXT("Driveway") || F==TEXT("Road2") || F==TEXT("Road4") || F==TEXT("TruckAccess");}
	bool ContainsXY(const FFoundrylineGridBounds& Outer, const FFoundrylineGridBounds& Inner)
	{
		return Outer.Min.X <= Inner.Min.X && Outer.Min.Y <= Inner.Min.Y
			&& Outer.Max.X >= Inner.Max.X && Outer.Max.Y >= Inner.Max.Y;
	}
}

bool UFoundrylinePlacementSubsystem::IsAuthority() const
{
	return GetWorld() && GetWorld()->GetNetMode() != NM_Client;
}

bool UFoundrylinePlacementSubsystem::RegisterDefinition(UFoundrylinePlaceableDefinition* Definition)
{
	if (!IsValid(Definition) || !Definition->IsDefinitionValid()) return false;
	// Replacing a definition could invalidate placed objects; require an explicit migration instead.
	if (Definitions.Contains(Definition->DefinitionId)) return false;
	Definitions.Add(Definition->DefinitionId, Definition);
	return true;
}

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::ValidateRegions(
	const TArray<FFoundrylineBuildRegion>& InRegions)
{
	TSet<FGuid> Seen;
	for (int32 Index = 0; Index < InRegions.Num(); ++Index)
	{
		const auto& Region = InRegions[Index];
		if (!Region.RegionId.IsValid() || !Region.CompanyId.IsValid() || !Region.PlantId.IsValid()
			|| Region.SurfaceId.IsNone() || !Region.Bounds.IsValid()) return EFoundrylinePlacementError::InvalidRegion;
		if(Region.bHasBuildingSetback && (!Region.BuildingBounds.IsValid() || !ContainsXY(Region.Bounds,Region.BuildingBounds))) return EFoundrylinePlacementError::InvalidRegion;
        if (Seen.Contains(Region.RegionId)) return EFoundrylinePlacementError::DuplicateId;
		Seen.Add(Region.RegionId);
		for (int32 Other = 0; Other < Index; ++Other)
		{
			const auto& B = InRegions[Other].Bounds;
			if (Region.Bounds.Min.X < B.Max.X && Region.Bounds.Max.X > B.Min.X
				&& Region.Bounds.Min.Y < B.Max.Y && Region.Bounds.Max.Y > B.Min.Y)
				return EFoundrylinePlacementError::InvalidRegion;
		}
	}
	return EFoundrylinePlacementError::None;
}

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::InitializeRegions(
	const TArray<FFoundrylineBuildRegion>& InRegions)
{
	if (!IsAuthority()) return EFoundrylinePlacementError::NotAuthority;
	if (!Regions.IsEmpty() || !Objects.IsEmpty() || Revision != 0) return EFoundrylinePlacementError::InvalidRegion;
	const auto Error = ValidateRegions(InRegions);
	if (Error != EFoundrylinePlacementError::None) return Error;
	Regions = InRegions;
	++Revision;
	return EFoundrylinePlacementError::None;
}

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::ValidateObject(
	const FFoundrylinePlacedObject& Candidate, const TArray<FFoundrylinePlacedObject>& Against,
	const TArray<FFoundrylineBuildRegion>& InRegions) const
{
	if(!Candidate.Colors.IsValid()) return EFoundrylinePlacementError::InvalidTransform;
	if (!Candidate.CompanyId.IsValid() || !Candidate.PlantId.IsValid()) return EFoundrylinePlacementError::InvalidIdentity;
	const auto* Entry = Definitions.Find(Candidate.DefinitionId);
	if (!Entry || !IsValid(Entry->Get())) return EFoundrylinePlacementError::UnknownDefinition;
	const auto& Def = **Entry;
	if (!Def.IsDefinitionValid()) return EFoundrylinePlacementError::InvalidDefinition;
	if (!Candidate.Openings.IsEmpty() && Def.FunctionId != TEXT("BuildingShell")) return EFoundrylinePlacementError::InvalidDefinition;
	if(Def.DefinitionId.ToString().StartsWith(TEXT("Office_"))!=Candidate.bOfficeAnnex) return EFoundrylinePlacementError::InvalidDefinition;
 if(Candidate.RouteNodes.Num()>64 || (!Candidate.RouteNodes.IsEmpty() && !FFoundrylineDrivingLayout::IsPaved(Def.FunctionId))) return EFoundrylinePlacementError::InvalidTransform;
 int32 TotalNodes=Candidate.RouteNodes.Num();for(const auto& O:Against) if(O.ObjectId!=Candidate.ObjectId) TotalNodes+=O.RouteNodes.Num();
 if(TotalNodes>4096) return EFoundrylinePlacementError::TooManyActions;
 TSet<FGuid> NodeIds;
 for(const auto& N:Candidate.RouteNodes) {
  if(!N.Id.IsValid() || NodeIds.Contains(N.Id) || N.Links.Num()>8 || N.RadiusCm<400 || N.RadiusCm>3000 || N.RadiusCm%100 || N.Position.X<0 || N.Position.Y<0 || N.Position.X>Def.Footprint.Max.X || N.Position.Y>Def.Footprint.Max.Y) return EFoundrylinePlacementError::InvalidTransform;
  const auto WorldNode=Candidate.Anchor+FFoundrylineGrid::RotateOffset({N.Position.X,N.Position.Y,0},Candidate.QuarterTurns);
  // New nodes follow the world metre grid even on odd-width roads. Retain
  // legacy local-metre nodes so existing property saves remain loadable.
  if((N.Position.X%100 || N.Position.Y%100) && !FFoundrylineGrid::IsOnSnap(WorldNode,EFoundrylineSnap::Structural100cm)) return EFoundrylinePlacementError::InvalidTransform;
  NodeIds.Add(N.Id);TSet<FGuid> Links;for(const auto& E:N.Links) {if(!E.To.IsValid() || E.To==N.Id || Links.Contains(E.To)) return EFoundrylinePlacementError::InvalidIdentity;Links.Add(E.To);}
  for(const auto& O:Against) if(O.ObjectId!=Candidate.ObjectId) for(const auto& Other:O.RouteNodes) {
   if(Other.Id==N.Id) return EFoundrylinePlacementError::DuplicateId;
   if((O.CompanyId!=Candidate.CompanyId || O.PlantId!=Candidate.PlantId) && N.Links.ContainsByPredicate([&](const auto& E){return E.To==Other.Id;})) return EFoundrylinePlacementError::AccessDenied;
  }
 }
 if(Candidate.TruckZones.Num()>64 || (!Candidate.TruckZones.IsEmpty() && !FFoundrylineDrivingLayout::IsPaved(Def.FunctionId))) return EFoundrylinePlacementError::InvalidTransform;
 for(const auto& Zone:Candidate.TruckZones) if(Zone.Min.X<0 || Zone.Min.Y<0 || Zone.Max.X>Def.Footprint.Max.X || Zone.Max.Y>Def.Footprint.Max.Y || Zone.Max.X<=Zone.Min.X || Zone.Max.Y<=Zone.Min.Y || Zone.Min.X%100 || Zone.Min.Y%100 || Zone.Max.X%100 || Zone.Max.Y%100) return EFoundrylinePlacementError::InvalidTransform;
	if (Candidate.ColumnBayCm!=400 && Candidate.ColumnBayCm!=600 && Candidate.ColumnBayCm!=800) return EFoundrylinePlacementError::InvalidTransform;
	if (Candidate.Crosswalks.Num()>16 || (!Candidate.Crosswalks.IsEmpty() && (!Road(Def.FunctionId) || (Def.FunctionId==TEXT("Service") || Def.FunctionId==TEXT("Driveway"))))) return EFoundrylinePlacementError::InvalidTransform;
	for(int32 I=0;I<Candidate.Crosswalks.Num();++I)
	{
		const int32 Y=Candidate.Crosswalks[I];
		if(Y<200 || Y>Def.Footprint.Max.Y-200 || Y%100) return EFoundrylinePlacementError::InvalidTransform;
		for(int32 J=0;J<I;++J) if(FMath::Abs(Y-Candidate.Crosswalks[J])<600) return EFoundrylinePlacementError::Occupied;
	}
	if((!Candidate.OfficeAreas.IsEmpty() || !Candidate.OfficeRooms.IsEmpty()) && (!Candidate.bOfficeAnnex || Def.FunctionId!=TEXT("BuildingShell"))) return EFoundrylinePlacementError::InvalidDefinition;
	if(Candidate.OfficeAreas.Num()>32 || Candidate.OfficeRooms.Num()>64) return EFoundrylinePlacementError::TooManyActions;
	TSet<FGuid> SpaceIds;
	auto Inside=[](const FFoundrylineOfficeSpace& A,const FFoundrylineOfficeSpace& B){return A.Min.X>=B.Min.X && A.Min.Y>=B.Min.Y && A.Max.X<=B.Max.X && A.Max.Y<=B.Max.Y;};
	auto Overlap=[](const auto& A,const auto& B){return A.Min.X<B.Max.X && A.Max.X>B.Min.X && A.Min.Y<B.Max.Y && A.Max.Y>B.Min.Y;};
	for(int32 Kind=0;Kind<2;++Kind)
	{
		const auto& Spaces=Kind?Candidate.OfficeRooms:Candidate.OfficeAreas;
		for(int32 I=0;I<Spaces.Num();++I)
		{
			const auto& A=Spaces[I];
			if(!A.SpaceId.IsValid() || SpaceIds.Contains(A.SpaceId) || A.Min.X<0 || A.Min.Y<0 || A.Max.X>Def.Footprint.Max.X || A.Max.Y>Def.Footprint.Max.Y
				|| A.Max.X<=A.Min.X || A.Max.Y<=A.Min.Y || A.Max.X-A.Min.X<400 || A.Max.Y-A.Min.Y<400
				|| A.Min.X%100 || A.Min.Y%100 || A.Max.X%100 || A.Max.Y%100) return EFoundrylinePlacementError::InvalidTransform;
			SpaceIds.Add(A.SpaceId);
			if(Kind && !Candidate.OfficeAreas.ContainsByPredicate([&](const auto& Zone){return Inside(A,Zone);})) return EFoundrylinePlacementError::InvalidTransform;
			for(int32 J=0;J<I;++J) if(Overlap(A,Spaces[J])) return EFoundrylinePlacementError::Occupied;
			if((!Kind && !A.Openings.IsEmpty()) || A.Openings.Num()>16) return EFoundrylinePlacementError::InvalidTransform;
			TSet<FGuid> Ids;
			for(int32 J=0;J<A.Openings.Num();++J)
			{
				const auto& V=A.Openings[J];const int32 Length=V.Edge%2==0?A.Max.X-A.Min.X:A.Max.Y-A.Min.Y;
				if(!V.OpeningId.IsValid() || Ids.Contains(V.OpeningId) || V.Edge>3 || (V.Kind!=EFoundrylineOpeningKind::OfficeDoor && V.Kind!=EFoundrylineOpeningKind::ShopWindow)
					|| V.OffsetCm%100 || V.OffsetCm<100 || V.OffsetCm>Length-100 || V.OffsetCm+V.WidthCm()>Length-100 || V.BottomCm()<0 || V.BottomCm()+V.HeightCm()>280) return EFoundrylinePlacementError::InvalidTransform;
				Ids.Add(V.OpeningId);
				for(int32 K=0;K<J;++K) if(A.Openings[K].Edge==V.Edge && V.OffsetCm<A.Openings[K].OffsetCm+A.Openings[K].WidthCm()+30 && V.OffsetCm+V.WidthCm()+30>A.Openings[K].OffsetCm) return EFoundrylinePlacementError::Occupied;
			}
		}
	}
	if (Candidate.Openings.Num() > 64) return EFoundrylinePlacementError::TooManyActions;
	TSet<FGuid> OpeningIds;
	for (int32 I=0; I<Candidate.Openings.Num(); ++I)
	{
		const auto& O = Candidate.Openings[I];
		const int32 Length = (O.Edge % 2 == 0 ? Def.Footprint.Max.X : Def.Footprint.Max.Y);
		if (!O.OpeningId.IsValid() || OpeningIds.Contains(O.OpeningId) || O.Edge > 3
			|| uint8(O.Kind) > uint8(EFoundrylineOpeningKind::LoadingDock) || (!(O.bAutomatic && O.Kind==EFoundrylineOpeningKind::ShopWindow) && O.OffsetCm % 100 != 0)
			|| (O.Kind==EFoundrylineOpeningKind::ShopWindow && (O.WindowSillCm<0 || O.WindowSillCm>Def.Footprint.Max.Z))
			|| O.OffsetCm < 100 || O.OffsetCm > Length - 100 || O.OffsetCm + O.WidthCm() > Length - 100
			|| O.BottomCm() + O.HeightCm() + 20 >= Def.Footprint.Max.Z) return EFoundrylinePlacementError::InvalidTransform;
		OpeningIds.Add(O.OpeningId);
		for (int32 J=0; J<I; ++J)
		{
			const auto& B = Candidate.Openings[J];
			if (O.Edge == B.Edge && O.OffsetCm < B.OffsetCm+B.WidthCm()+30 && O.OffsetCm+O.WidthCm()+30 > B.OffsetCm)
				return EFoundrylinePlacementError::Occupied;
		}
	}
	if (Candidate.QuarterTurns > 3 || Candidate.Anchor.Z != 0
		|| !FFoundrylineGrid::IsOnSnap(Candidate.Anchor, Def.Snap)) return EFoundrylinePlacementError::InvalidTransform;
	const auto Solid = FFoundrylineGrid::RotateBounds(Def.Footprint, Candidate.Anchor, Candidate.QuarterTurns);
	const auto Clearance = FFoundrylineGrid::RotateBounds(Def.WorkingClearance, Candidate.Anchor, Candidate.QuarterTurns);
	if (!Solid.IsValid() || !Clearance.IsValid()) return EFoundrylinePlacementError::InvalidTransform;
	const bool bHasRegion = InRegions.ContainsByPredicate([&](const FFoundrylineBuildRegion& Region)
	{
		return Region.CompanyId == Candidate.CompanyId && Region.PlantId == Candidate.PlantId
			&& Def.AllowedSurfaces.Contains(Region.SurfaceId) && ContainsXY(Region.Bounds, Clearance)
            && (Def.FunctionId!=TEXT("BuildingShell") || !Region.bHasBuildingSetback || ContainsXY(Region.BuildingBounds,Clearance));
	});
	if (!bHasRegion) return EFoundrylinePlacementError::AccessDenied;
    for(const auto& Opening:Candidate.Openings) if(Opening.Kind==EFoundrylineOpeningKind::LoadingDock)
    {
     if(Candidate.bOfficeAnnex) return EFoundrylinePlacementError::InvalidDefinition;
     const int32 Along=Opening.OffsetCm+Opening.WidthCm()/2;
     const int32 Width=Def.Footprint.Max.X,Depth=Def.Footprint.Max.Y;
     FFoundrylineGridBounds Dock;
     Dock.Min=Opening.Edge%2==0?FIntVector(Along-250,Opening.Edge==0?-330:Depth,0):FIntVector(Opening.Edge==3?-330:Width,Along-250,0);
     Dock.Max=Dock.Min+(Opening.Edge%2==0?FIntVector(500,330,600):FIntVector(330,500,600));
     const auto WorldDock=FFoundrylineGrid::RotateBounds(Dock,Candidate.Anchor,Candidate.QuarterTurns);
     if(!InRegions.ContainsByPredicate([&](const auto& R){return R.CompanyId==Candidate.CompanyId && R.PlantId==Candidate.PlantId && ContainsXY(R.Bounds,WorldDock);})) return EFoundrylinePlacementError::AccessDenied;
     for(const auto& Other:Against)
     {
      if(Other.ObjectId==Candidate.ObjectId) continue;
      const auto* OtherDef=FindDefinition(Other.DefinitionId);
      if(OtherDef && OtherDef->FunctionId==TEXT("BuildingShell") && WorldDock.Overlaps(FFoundrylineGrid::RotateBounds(OtherDef->Footprint,Other.Anchor,Other.QuarterTurns))) return EFoundrylinePlacementError::Occupied;
     }
    }
	if(Candidate.bOfficeAnnex)
	{
		if(Def.FunctionId!=TEXT("BuildingShell") || Def.Footprint.Max.Z!=320 || Def.Footprint.Max.X<400 || Def.Footprint.Max.Y<400) return EFoundrylinePlacementError::InvalidDefinition;
		bool Attached=false,DoorToShop=false;TSet<uint8> SharedEdges;
		for(const auto& Other:Against)
		{
			if(Other.ObjectId==Candidate.ObjectId || Other.bOfficeAnnex || Other.CompanyId!=Candidate.CompanyId || Other.PlantId!=Candidate.PlantId) continue;
			const auto* OD=FindDefinition(Other.DefinitionId);if(!OD || OD->FunctionId!=TEXT("BuildingShell")) continue;
			const auto B=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);
			const FRotator Rot(0,Candidate.QuarterTurns*90,0);const FVector A=Rot.UnrotateVector(FVector(B.Min-Candidate.Anchor)),C=Rot.UnrotateVector(FVector(B.Max-Candidate.Anchor));
			const auto Min=A.ComponentMin(C),Max=A.ComponentMax(C);
			for(uint8 Edge=0;Edge<4;++Edge)
			{
				const bool Touch=Edge==0?FMath::Abs(Max.Y)<.1:Edge==1?FMath::Abs(Min.X-Def.Footprint.Max.X)<.1:Edge==2?FMath::Abs(Min.Y-Def.Footprint.Max.Y)<.1:FMath::Abs(Max.X)<.1;
				const double Low=FMath::Max(0.,Edge%2==0?Min.X:Min.Y),High=FMath::Min(double(Edge%2==0?Def.Footprint.Max.X:Def.Footprint.Max.Y),Edge%2==0?Max.X:Max.Y);
				if(!Touch || High-Low<400) continue;
				Attached=true;SharedEdges.Add(Edge);
				DoorToShop|=Candidate.Openings.ContainsByPredicate([&](const auto& V){return (V.Kind==EFoundrylineOpeningKind::OfficeDoor || V.Kind==EFoundrylineOpeningKind::Personnel) && V.Edge==Edge && V.OffsetCm>=Low && V.OffsetCm+V.WidthCm()<=High;});
			}
		}
		const bool DoorOutside=Candidate.Openings.ContainsByPredicate([&](const auto& V){return (V.Kind==EFoundrylineOpeningKind::OfficeDoor || V.Kind==EFoundrylineOpeningKind::Personnel) && !SharedEdges.Contains(V.Edge);});
		if((Attached && !DoorToShop) || !DoorOutside) return EFoundrylinePlacementError::InvalidTransform;
	}
	if (Def.CollisionClass == EFoundrylineCollisionClass::Cosmetic) return EFoundrylinePlacementError::None;
	for (const auto& Other : Against)
	{
		if (Candidate.ObjectId.IsValid() && Other.ObjectId == Candidate.ObjectId) continue;
		const auto* OtherEntry = Definitions.Find(Other.DefinitionId);
		if (!OtherEntry || !IsValid(OtherEntry->Get()) || !(*OtherEntry)->IsDefinitionValid())
			return EFoundrylinePlacementError::InvalidDefinition;
		const auto& OtherDef = **OtherEntry;
		if (OtherDef.CollisionClass == EFoundrylineCollisionClass::Cosmetic) continue;
		// A connected road surface may overlap another road owned by the same plant.
		// Parking, buildings and other companies still retain their full exclusion bounds.
		if(Road(Def.FunctionId) && Road(OtherDef.FunctionId) && Candidate.CompanyId==Other.CompanyId && Candidate.PlantId==Other.PlantId && Candidate.QuarterTurns%2!=Other.QuarterTurns%2) continue;
		const auto OtherSolid = FFoundrylineGrid::RotateBounds(OtherDef.Footprint, Other.Anchor, Other.QuarterTurns);
        if(Def.FunctionId==TEXT("BuildingShell")) for(const auto& Opening:Other.Openings)
         if(Opening.Kind==EFoundrylineOpeningKind::LoadingDock && Solid.Overlaps(DockBounds(OtherDef,Other,Opening))) return EFoundrylinePlacementError::Occupied;
		const auto OtherClearance = FFoundrylineGrid::RotateBounds(OtherDef.WorkingClearance, Other.Anchor, Other.QuarterTurns);
		if (Solid.Overlaps(OtherClearance) || Clearance.Overlaps(OtherSolid)) return EFoundrylinePlacementError::Occupied;
	}
	return EFoundrylinePlacementError::None;
}

FFoundrylinePlacementResult UFoundrylinePlacementSubsystem::Validate(
	const FFoundrylinePlacementRequest& Request, const FFoundrylinePlacementContext& Context) const
{
	const auto Plan=PreviewBatch({Request},Context);
	FFoundrylinePlacementResult Result;Result.Error=Plan.Error;Result.Revision=Revision;Result.ObjectId=Request.ObjectId;return Result;
}

FFoundrylinePlacementResult UFoundrylinePlacementSubsystem::ValidateAgainst(
	const FFoundrylinePlacementRequest& Request, const FFoundrylinePlacementContext& Context,
	const TArray<FFoundrylinePlacedObject>& Against,bool bLocalPreview) const
{
	FFoundrylinePlacementResult Result;
	Result.ObjectId = Request.ObjectId;
	Result.Revision = Revision;
	if (!IsAuthority() && !bLocalPreview) { Result.Error = EFoundrylinePlacementError::NotAuthority; return Result; }
	if (Request.ExpectedRevision != Revision || Revision == MAX_int64)
	{ Result.Error = EFoundrylinePlacementError::StaleRevision; return Result; }
	if (!Context.bCanConstruct || !Context.CompanyId.IsValid() || !Context.PlantId.IsValid())
	{ Result.Error = EFoundrylinePlacementError::AccessDenied; return Result; }
	FFoundrylinePlacedObject Candidate;
	Candidate.CompanyId = Context.CompanyId;
	Candidate.PlantId = Context.PlantId;
	Candidate.DefinitionId = Request.DefinitionId;
	Candidate.Anchor = Request.Anchor;
	Candidate.QuarterTurns = Request.QuarterTurns;
	Candidate.Openings = Request.Openings;
	Candidate.ColumnBayCm = Request.ColumnBayCm;
	Candidate.bOfficeAnnex=Request.bOfficeAnnex;Candidate.OfficeAreas=Request.OfficeAreas;
	if (Request.Action == EFoundrylinePlacementAction::Build)
	{
		if (Request.ObjectId.IsValid()) { Result.Error = EFoundrylinePlacementError::InvalidIdentity; return Result; }
	}
	else if(Request.Action==EFoundrylinePlacementAction::RestoreRemoved)
	{
		const auto* Removed=RemovedObjects.Find(Request.ObjectId);
		if(!Removed || Against.ContainsByPredicate([&](const auto& O){return O.ObjectId==Request.ObjectId;})) {Result.Error=EFoundrylinePlacementError::UnknownObject;return Result;}
		if(Removed->CompanyId!=Context.CompanyId || Removed->PlantId!=Context.PlantId) {Result.Error=EFoundrylinePlacementError::AccessDenied;return Result;}
		Candidate=*Removed;
	}
	else
	{
		const auto* Existing = Against.FindByPredicate([&](const FFoundrylinePlacedObject& Object)
		{ return Object.ObjectId == Request.ObjectId; });
		if (!Existing) { Result.Error = EFoundrylinePlacementError::UnknownObject; return Result; }
		if (Existing->CompanyId != Context.CompanyId || Existing->PlantId != Context.PlantId)
		{ Result.Error = EFoundrylinePlacementError::AccessDenied; return Result; }
		Candidate = *Existing;
		switch (Request.Action)
		{
		case EFoundrylinePlacementAction::ReplaceObject:
		{
			const auto* Old=FindDefinition(Existing->DefinitionId); const auto* New=FindDefinition(Request.Replacement.DefinitionId);
			if(!Old || !New || Old->FunctionId!=New->FunctionId || Old->Footprint.Max.Z!=New->Footprint.Max.Z
				|| Request.Replacement.ObjectId!=Existing->ObjectId || Request.Replacement.CompanyId!=Existing->CompanyId || Request.Replacement.PlantId!=Existing->PlantId)
			{Result.Error=EFoundrylinePlacementError::InvalidIdentity;return Result;}
			Candidate=Request.Replacement; break;
		}
		case EFoundrylinePlacementAction::SetRouteNodes: Candidate.RouteNodes=Request.RouteNodes;break;
  case EFoundrylinePlacementAction::SetTruckZones: Candidate.TruckZones=Request.TruckZones;break;
  case EFoundrylinePlacementAction::SetBuildingColors:
   if(const auto* D=FindDefinition(Candidate.DefinitionId);!D || D->FunctionId!=TEXT("BuildingShell")) {Result.Error=EFoundrylinePlacementError::InvalidDefinition;return Result;}
   Candidate.Colors=Request.Colors;break;
		case EFoundrylinePlacementAction::SetLandscaping:
			if(const auto* D=FindDefinition(Candidate.DefinitionId);!D || D->FunctionId!=TEXT("Parking")) {Result.Error=EFoundrylinePlacementError::InvalidDefinition;return Result;}
			Candidate.bLandscaping=Request.bLandscaping;break;
		case EFoundrylinePlacementAction::SetCrosswalks: Candidate.Crosswalks=Request.Crosswalks;break;
		case EFoundrylinePlacementAction::SetColumnBay:
			if (const auto* Def=FindDefinition(Candidate.DefinitionId); !Def || Def->FunctionId!=TEXT("BuildingShell") || Def->Footprint.Max.Z<620 || Candidate.bOfficeAnnex)
			{ Result.Error=EFoundrylinePlacementError::InvalidDefinition; return Result; }
			Candidate.ColumnBayCm=Request.ColumnBayCm;
            if(const auto* D=FindDefinition(Candidate.DefinitionId)) FFoundrylineFacadeLayout::Reflow(Candidate,D->Footprint.Max.X,D->Footprint.Max.Y,D->Footprint.Max.Z);
            break;
		case EFoundrylinePlacementAction::Duplicate: Candidate.RouteNodes.Reset(); Candidate.ObjectId.Invalidate(); Candidate.Anchor=Request.Anchor; break;
		case EFoundrylinePlacementAction::Move: Candidate.Anchor = Request.Anchor; break;
		case EFoundrylinePlacementAction::Rotate: Candidate.QuarterTurns = Request.QuarterTurns; break;
		case EFoundrylinePlacementAction::Remove: return Result;
		case EFoundrylinePlacementAction::AddOpening:
			if (Request.Opening.OffsetCm < 0 || Request.Opening.OffsetCm > FFoundrylineGrid::MaxCoordinate)
			{ Result.Error = EFoundrylinePlacementError::InvalidTransform; return Result; }
			if (const auto* Def=FindDefinition(Candidate.DefinitionId); Def && Def->FunctionId==TEXT("BuildingShell") && !Candidate.bOfficeAnnex)
			{
				for (const auto& Other:Against)
				{
					if (Other.ObjectId==Candidate.ObjectId || Other.CompanyId!=Candidate.CompanyId || Other.PlantId!=Candidate.PlantId) continue;
					const auto* OtherDef=FindDefinition(Other.DefinitionId);
					if (!OtherDef || OtherDef->FunctionId!=TEXT("BuildingShell")) continue;
					const auto WorldB=FFoundrylineGrid::RotateBounds(OtherDef->Footprint,Other.Anchor,Other.QuarterTurns);
					const FRotator Rot(0,Candidate.QuarterTurns*90,0);
					const FVector A=Rot.UnrotateVector(FVector(WorldB.Min)-FVector(Candidate.Anchor));
					const FVector B=Rot.UnrotateVector(FVector(WorldB.Max)-FVector(Candidate.Anchor));
					const FVector Min=A.ComponentMin(B),Max=A.ComponentMax(B);
					const auto& O=Request.Opening;
					const bool Touch=O.Edge==0?FMath::Abs(Max.Y)<0.1:O.Edge==1?FMath::Abs(Min.X-Def->Footprint.Max.X)<0.1:O.Edge==2?FMath::Abs(Min.Y-Def->Footprint.Max.Y)<0.1:O.Edge==3 && FMath::Abs(Max.X)<0.1;
					const double Low=O.Edge%2==0?Min.X:Min.Y,High=O.Edge%2==0?Max.X:Max.Y;
					if (Touch && O.OffsetCm<High && O.OffsetCm+O.WidthCm()>Low)
					{ Result.Error=EFoundrylinePlacementError::InvalidTransform; return Result; }
				}
			}
			InsertOpening(Candidate.Openings, Request.Opening); break;
		case EFoundrylinePlacementAction::ReplaceOpenings: Candidate.Openings = Request.Openings; break;
		case EFoundrylinePlacementAction::RemoveOpening:
			if (!Candidate.Openings.RemoveAll([&](const FFoundrylineOpening& O) { return O.OpeningId == Request.Opening.OpeningId; }))
			{ Result.Error = EFoundrylinePlacementError::UnknownObject; return Result; }
			break;
		default: Result.Error = EFoundrylinePlacementError::InvalidAction; return Result;
		}
	}
	Result.Error = ValidateObject(Candidate, Against, Regions);
	return Result;
}

FFoundrylinePlacementResult UFoundrylinePlacementSubsystem::Execute(
	const FFoundrylinePlacementRequest& Request, const FFoundrylinePlacementContext& Context)
{
	const auto Batch = ExecuteBatch({Request}, Context, MAX_int64);
	if (Batch.IsSuccess()) return Batch.Actions[0];
	FFoundrylinePlacementResult Result;
	Result.Error = Batch.Error;
	Result.ObjectId = Batch.Actions.IsEmpty()?Request.ObjectId:Batch.Actions[0].ObjectId;
	Result.Revision = Revision;
	return Result;
}

FFoundrylinePlanResult UFoundrylinePlacementSubsystem::StageBatch(
	const TArray<FFoundrylinePlacementRequest>& Requests, const FFoundrylinePlacementContext& Context,
	TArray<FFoundrylinePlacedObject>& Staged, TArray<FFoundrylinePlacementChange>& Changes,bool bLocalPreview) const
{
	FFoundrylinePlanResult Result;
	Result.Revision = Revision;
	if (!IsAuthority() && !bLocalPreview) { Result.Error = EFoundrylinePlacementError::NotAuthority; return Result; }
	if (Requests.IsEmpty() || Requests.Num() > 128)
	{ Result.Error = EFoundrylinePlacementError::TooManyActions; return Result; }
	Staged = Objects;
	for (int32 Index = 0; Index < Requests.Num(); ++Index)
	{
		const auto& Request = Requests[Index];
		auto Action = ValidateAgainst(Request, Context, Staged,bLocalPreview);
		if (!Action.IsSuccess())
		{ Result.Error = Action.Error; Result.FailedAction = Index; return Result; }
		FFoundrylinePlacementChange Change;
		if (Request.Action == EFoundrylinePlacementAction::Build)
		{
			const auto& Def = *Definitions.FindChecked(Request.DefinitionId);
			if (Result.CostCents > MAX_int64 - Def.PriceCents)
			{ Result.Error = EFoundrylinePlacementError::CostOverflow; Result.FailedAction = Index; return Result; }
			Result.CostCents += Def.PriceCents;
			FFoundrylinePlacedObject Object;
			Object.ObjectId = FGuid::NewGuid();
			Object.CompanyId = Context.CompanyId;
			Object.PlantId = Context.PlantId;
			Object.DefinitionId = Request.DefinitionId;
			Object.Anchor = Request.Anchor;
			Object.QuarterTurns = Request.QuarterTurns;
			Object.Openings = Request.Openings;
			Object.ColumnBayCm = Request.ColumnBayCm;
			Object.bOfficeAnnex=Request.bOfficeAnnex;Object.OfficeAreas=Request.OfficeAreas;
			Staged.Add(Object);
			Change.bHasCurrent = true;
			Change.Current = Object;
			Action.ObjectId = Object.ObjectId;
		}
		else if(Request.Action==EFoundrylinePlacementAction::Duplicate)
        {
            auto Copy=*Staged.FindByPredicate([&](const auto& O){return O.ObjectId==Request.ObjectId;});
            Copy.ObjectId=FGuid::NewGuid();Copy.Anchor=Request.Anchor;Copy.RouteNodes.Reset();
            for(auto& O:Copy.Openings) O.OpeningId=FGuid::NewGuid();
            for(auto& Area:Copy.OfficeAreas) {Area.SpaceId=FGuid::NewGuid();for(auto& O:Area.Openings) O.OpeningId=FGuid::NewGuid();}
            for(auto& Room:Copy.OfficeRooms) {Room.SpaceId=FGuid::NewGuid();for(auto& O:Room.Openings) O.OpeningId=FGuid::NewGuid();}
            Staged.Add(Copy);Change.Current=Copy;Change.bHasCurrent=true;Action.ObjectId=Copy.ObjectId;
        }
        else if(Request.Action==EFoundrylinePlacementAction::RestoreRemoved)
        {
            Change.Current=RemovedObjects.FindChecked(Request.ObjectId); Change.bHasCurrent=true; Staged.Add(Change.Current);
		}
		else
		{
			const int32 ObjectIndex = Staged.IndexOfByPredicate([&](const FFoundrylinePlacedObject& Object)
			{ return Object.ObjectId == Request.ObjectId; });
			Change.bHadPrevious = true;
			Change.Previous = Staged[ObjectIndex];
			if (Request.Action == EFoundrylinePlacementAction::Remove) Staged.RemoveAt(ObjectIndex);
			else
			{
				if(Request.Action==EFoundrylinePlacementAction::ReplaceObject) Staged[ObjectIndex]=Request.Replacement;
    else if(Request.Action==EFoundrylinePlacementAction::SetBuildingColors) Staged[ObjectIndex].Colors=Request.Colors;
				else if(Request.Action==EFoundrylinePlacementAction::SetLandscaping) Staged[ObjectIndex].bLandscaping=Request.bLandscaping;
				else if(Request.Action==EFoundrylinePlacementAction::SetCrosswalks) Staged[ObjectIndex].Crosswalks=Request.Crosswalks;
				else if (Request.Action == EFoundrylinePlacementAction::Move) Staged[ObjectIndex].Anchor = Request.Anchor;
				else if (Request.Action == EFoundrylinePlacementAction::Rotate) Staged[ObjectIndex].QuarterTurns = Request.QuarterTurns;
				else if (Request.Action == EFoundrylinePlacementAction::AddOpening) InsertOpening(Staged[ObjectIndex].Openings, Request.Opening);
				else if (Request.Action == EFoundrylinePlacementAction::SetTruckZones) Staged[ObjectIndex].TruckZones=Request.TruckZones;
    else if (Request.Action == EFoundrylinePlacementAction::SetRouteNodes) Staged[ObjectIndex].RouteNodes=Request.RouteNodes;
				else if (Request.Action == EFoundrylinePlacementAction::SetColumnBay) {auto& O=Staged[ObjectIndex];O.ColumnBayCm=Request.ColumnBayCm;if(const auto* D=FindDefinition(O.DefinitionId)) FFoundrylineFacadeLayout::Reflow(O,D->Footprint.Max.X,D->Footprint.Max.Y,D->Footprint.Max.Z);}
				else if (Request.Action == EFoundrylinePlacementAction::ReplaceOpenings) Staged[ObjectIndex].Openings = Request.Openings;
				else if (Request.Action == EFoundrylinePlacementAction::RemoveOpening)
					Staged[ObjectIndex].Openings.RemoveAll([&](const FFoundrylineOpening& O) { return O.OpeningId == Request.Opening.OpeningId; });
				Change.bHasCurrent = true;
				Change.Current = Staged[ObjectIndex];
			}
		}
		Change.ObjectId = Action.ObjectId;
		const FName DefinitionId = Change.bHasCurrent ? Change.Current.DefinitionId : Change.Previous.DefinitionId;
		const auto* Definition = Definitions.Find(DefinitionId);
		if (Definition && IsValid(Definition->Get()))
		{
			Change.bAffectsNavigation = Request.Action!=EFoundrylinePlacementAction::SetBuildingColors && (*Definition)->CollisionClass != EFoundrylineCollisionClass::Cosmetic;
			if (Change.bHadPrevious) Change.PreviousBounds = FFoundrylineGrid::RotateBounds(
				FindDefinition(Change.Previous.DefinitionId)->WorkingClearance, Change.Previous.Anchor, Change.Previous.QuarterTurns);
			if (Change.bHasCurrent) Change.CurrentBounds = FFoundrylineGrid::RotateBounds(
				(*Definition)->WorkingClearance, Change.Current.Anchor, Change.Current.QuarterTurns);
		}
		Changes.Add(Change);
		Result.Actions.Add(Action);
	}
	// Removing paving also removes incoming links to its deleted nodes. Publish these
 // metadata changes atomically with demolition, never leave an invisible dead-end edge.
 TSet<FGuid> PresentNodes;for(const auto& O:Staged) for(const auto& N:O.RouteNodes) PresentNodes.Add(N.Id);
 TSet<FGuid> DeletedNodes;
 for(const auto& C:Changes) if(C.bHadPrevious) for(const auto& N:C.Previous.RouteNodes)
  if(!PresentNodes.Contains(N.Id)) DeletedNodes.Add(N.Id);
 if(!DeletedNodes.IsEmpty()) for(auto& O:Staged) {
  const auto Previous=O;bool Changed=false;
  for(auto& N:O.RouteNodes) Changed|=N.Links.RemoveAll([&](const auto& E){return DeletedNodes.Contains(E.To);})>0;
  if(!Changed) continue;int32 Existing=INDEX_NONE;for(int32 I=Changes.Num()-1;I>=0;--I) if(Changes[I].ObjectId==O.ObjectId) {Existing=I;break;}
  if(Existing!=INDEX_NONE) Changes[Existing].Current=O;
  else {FFoundrylinePlacementChange C;C.ObjectId=O.ObjectId;C.bHadPrevious=C.bHasCurrent=true;C.Previous=Previous;C.Current=O;C.bAffectsNavigation=true;
   C.CurrentBounds=C.PreviousBounds=FFoundrylineGrid::RotateBounds(FindDefinition(O.DefinitionId)->WorkingClearance,O.Anchor,O.QuarterTurns);Changes.Add(C);}
 }
 // Validate cross-surface references after the atomic batch, so new linked nodes can arrive together.
 if(Requests.ContainsByPredicate([](const auto& R){return R.Action==EFoundrylinePlacementAction::SetRouteNodes;})) {
  TMap<FGuid,const FFoundrylinePlacedObject*> Hosts;for(const auto& O:Staged) for(const auto& N:O.RouteNodes) Hosts.Add(N.Id,&O);
  for(const auto& R:Requests) if(R.Action==EFoundrylinePlacementAction::SetRouteNodes) {
   const auto* O=Staged.FindByPredicate([&](const auto& V){return V.ObjectId==R.ObjectId;});
   for(const auto& N:O->RouteNodes) for(const auto& E:N.Links) {
    const auto* B=Hosts.Find(E.To);if(!B || (*B)->CompanyId!=O->CompanyId || (*B)->PlantId!=O->PlantId) {Result.Error=EFoundrylinePlacementError::AccessDenied;return Result;}
   }
  }
 }
 // A newly built shop may attach to an existing office. Derive office
 // doorway changes on the authority, in the same transaction as the shop.
 TSet<FGuid> NewShops;
 for(const auto& C:Changes) if(!C.bHadPrevious && C.bHasCurrent && !C.Current.bOfficeAnnex) {
  const auto* D=FindDefinition(C.Current.DefinitionId);
  if(D && D->FunctionId==TEXT("BuildingShell")) NewShops.Add(C.ObjectId);
 }
 for(auto& Office:Staged) if(Office.bOfficeAnnex && !NewShops.IsEmpty()) {
  const auto Previous=Office;const auto* OD=FindDefinition(Office.DefinitionId);if(!OD) continue;
  const int32 W=OD->Footprint.Max.X,H=OD->Footprint.Max.Y;
  TSet<uint8> Shared;bool Updated=false;
  auto EnsureDoor=[&](uint8 Edge,int32 Low,int32 High) {
   if(Office.Openings.ContainsByPredicate([&](const auto& V){return V.Edge==Edge && (V.Kind==EFoundrylineOpeningKind::OfficeDoor || V.Kind==EFoundrylineOpeningKind::Personnel) && V.OffsetCm>=Low && V.OffsetCm+V.WidthCm()<=High;})) return;
   const int32 Length=Edge%2==0?W:H;
   FFoundrylineOpening Door;Door.OpeningId=FGuid::NewGuid();Door.Edge=Edge;Door.Kind=EFoundrylineOpeningKind::OfficeDoor;
   Door.OffsetCm=FMath::Clamp(((Low+High)/2-50)/100*100,100,Length-200);
   InsertOpening(Office.Openings,Door);Updated=true;
  };
  bool NewConnection=false;
  for(const auto& Shop:Staged) {
   if(Shop.bOfficeAnnex || Shop.CompanyId!=Office.CompanyId || Shop.PlantId!=Office.PlantId) continue;
   const auto* SD=FindDefinition(Shop.DefinitionId);if(!SD || SD->FunctionId!=TEXT("BuildingShell")) continue;
   const auto B=FFoundrylineGrid::RotateBounds(SD->Footprint,Shop.Anchor,Shop.QuarterTurns);
   const FRotator Rot(0,Office.QuarterTurns*90,0);
   const FVector A=Rot.UnrotateVector(FVector(B.Min-Office.Anchor)),C=Rot.UnrotateVector(FVector(B.Max-Office.Anchor));
   const auto Min=A.ComponentMin(C),Max=A.ComponentMax(C);
   for(uint8 Edge=0;Edge<4;++Edge) {
    const bool Touch=Edge==0?FMath::Abs(Max.Y)<.1:Edge==1?FMath::Abs(Min.X-W)<.1:Edge==2?FMath::Abs(Min.Y-H)<.1:FMath::Abs(Max.X)<.1;
    const int32 Low=FMath::Max(0,FMath::RoundToInt(Edge%2==0?Min.X:Min.Y)),High=FMath::Min(Edge%2==0?W:H,FMath::RoundToInt(Edge%2==0?Max.X:Max.Y));
    if(!Touch || High-Low<400) continue;
    Shared.Add(Edge);
    if(NewShops.Contains(Shop.ObjectId)) {EnsureDoor(Edge,Low,High);NewConnection=true;}
   }
  }
  if(!NewConnection) continue;
  if(!Office.Openings.ContainsByPredicate([&](const auto& V){return !Shared.Contains(V.Edge) && (V.Kind==EFoundrylineOpeningKind::OfficeDoor || V.Kind==EFoundrylineOpeningKind::Personnel);}))
   for(uint8 Edge=0;Edge<4;++Edge) if(!Shared.Contains(Edge)) {EnsureDoor(Edge,0,Edge%2==0?W:H);break;}
  if(!Updated) continue;
  auto* Existing=Changes.FindByPredicate([&](const auto& C){return C.ObjectId==Office.ObjectId;});
  if(Existing) Existing->Current=Office;
  else {
   FFoundrylinePlacementChange C;C.ObjectId=Office.ObjectId;C.bHadPrevious=C.bHasCurrent=C.bAffectsNavigation=true;
   C.Previous=Previous;C.Current=Office;C.PreviousBounds=C.CurrentBounds=FFoundrylineGrid::RotateBounds(OD->WorkingClearance,Office.Anchor,Office.QuarterTurns);Changes.Add(C);
  }
 }
 for(const auto& O:Staged) if(O.bOfficeAnnex)
	{
		const auto Error=ValidateObject(O,Staged,Regions);
		if(Error!=EFoundrylinePlacementError::None) {Result.Error=Error;return Result;}
	}

	return Result;
}

FFoundrylinePlanResult UFoundrylinePlacementSubsystem::PreviewBatch(
	const TArray<FFoundrylinePlacementRequest>& Requests, const FFoundrylinePlacementContext& Context,bool bLocalPreview) const
{
	TArray<FFoundrylinePlacedObject> Staged;
	TArray<FFoundrylinePlacementChange> Changes;
	return StageBatch(Requests, Context, Staged, Changes,bLocalPreview);
}

FFoundrylinePlanResult UFoundrylinePlacementSubsystem::ExecuteBatch(
	const TArray<FFoundrylinePlacementRequest>& Requests, const FFoundrylinePlacementContext& Context,
	int64 MaxAuthorizedCostCents)
{
	if (bApplyingChanges)
	{
		FFoundrylinePlanResult Result;
		Result.Error = EFoundrylinePlacementError::Busy;
		Result.Revision = Revision;
		return Result;
	}
	TGuardValue<bool> Guard(bApplyingChanges, true);
	TArray<FFoundrylinePlacedObject> Staged;
	TArray<FFoundrylinePlacementChange> Changes;
	auto Result = StageBatch(Requests, Context, Staged, Changes);
	if (!Result.IsSuccess()) return Result;
	if (MaxAuthorizedCostCents < 0 || Result.CostCents > MaxAuthorizedCostCents)
	{ Result.Error = EFoundrylinePlacementError::CostLimitExceeded; return Result; }
	for(const auto& Change:Changes)
	{
		if(Change.bHadPrevious && !Change.bHasCurrent) RemovedObjects.Add(Change.ObjectId,Change.Previous);
		else if(Change.bHasCurrent) RemovedObjects.Remove(Change.ObjectId);
	}
	Objects = MoveTemp(Staged);
	Result.Revision = ++Revision;
	for (auto& Action : Result.Actions) Action.Revision = Revision;
	// Listeners see the complete committed state; recursive mutations are rejected.
	for (const auto& Change : Changes) OnPlacementChanged.Broadcast(Change);
	return Result;
}

UFoundrylinePlacementSave* UFoundrylinePlacementSubsystem::CreateSnapshot() const
{
	if (!IsAuthority()) return nullptr;
	auto* Save = NewObject<UFoundrylinePlacementSave>();
	Save->Revision = Revision;
	Save->Regions = Regions;
	Save->Objects = Objects;
	return Save;
}

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::RestoreSnapshot(const UFoundrylinePlacementSave* Snapshot, bool bReplaceExisting)
{
	if (!IsAuthority()) return EFoundrylinePlacementError::NotAuthority;
	if (bApplyingChanges) return EFoundrylinePlacementError::Busy;
	// Restore is bootstrap-only, so a rewind cannot make an old action revision valid again.
	if (!bReplaceExisting && (!Objects.IsEmpty() || !Regions.IsEmpty() || Revision != 0)) return EFoundrylinePlacementError::StaleRevision;
	if (!IsValid(Snapshot) || Snapshot->SchemaVersion != UFoundrylinePlacementSave::CurrentSchemaVersion
		|| Snapshot->Revision < 0 || Snapshot->Revision == MAX_int64) return EFoundrylinePlacementError::UnsupportedSave;
	auto Error = ValidateRegions(Snapshot->Regions);
	if (Error != EFoundrylinePlacementError::None) return Error;
	TSet<FGuid> Seen;
	TArray<FFoundrylinePlacedObject> Validated;
	for (const auto& Object : Snapshot->Objects)
	{
		if (!Object.ObjectId.IsValid()) return EFoundrylinePlacementError::InvalidIdentity;
		if (Seen.Contains(Object.ObjectId)) return EFoundrylinePlacementError::DuplicateId;
		Seen.Add(Object.ObjectId);
		Error = ValidateObject(Object, Snapshot->Objects, Snapshot->Regions);
		if (Error != EFoundrylinePlacementError::None) return Error;
		Validated.Add(Object);
	}
    if(bReplaceExisting && Revision==MAX_int64) return EFoundrylinePlacementError::StaleRevision;
    const auto Previous=Objects;
    Objects = MoveTemp(Validated);
    Regions = Snapshot->Regions;
    Revision = bReplaceExisting?FMath::Max(Revision,Snapshot->Revision)+1:Snapshot->Revision;
    RemovedObjects.Reset();
    if(bReplaceExisting)
    {
        TGuardValue<bool> Guard(bApplyingChanges,true);
        for(const auto& Old:Previous)
        {
            FFoundrylinePlacementChange C;C.ObjectId=Old.ObjectId;C.bHadPrevious=true;C.Previous=Old;
            C.PreviousBounds=FFoundrylineGrid::RotateBounds(FindDefinition(Old.DefinitionId)->Footprint,Old.Anchor,Old.QuarterTurns);
            C.bAffectsNavigation=true;OnPlacementChanged.Broadcast(C);
        }
        for(const auto& O:Objects)
        {
            FFoundrylinePlacementChange C;C.ObjectId=O.ObjectId;C.bHasCurrent=true;C.Current=O;
            C.CurrentBounds=FFoundrylineGrid::RotateBounds(FindDefinition(O.DefinitionId)->Footprint,O.Anchor,O.QuarterTurns);
            C.bAffectsNavigation=true;OnPlacementChanged.Broadcast(C);
        }
    }
	return EFoundrylinePlacementError::None;
}

EFoundrylinePlacementError UFoundrylinePlacementSubsystem::RegisterBusinessRegions(const TArray<FFoundrylineBuildRegion>& Additional){
 if(!IsAuthority())return EFoundrylinePlacementError::NotAuthority;if(bApplyingChanges)return EFoundrylinePlacementError::Busy;
 auto Candidate=Regions;for(const auto& R:Additional){if(const auto* Existing=Candidate.FindByPredicate([&](const auto& V){return V.RegionId==R.RegionId;})){if(!FFoundrylineBuildRegion::StaticStruct()->CompareScriptStruct(Existing,&R,0))return EFoundrylinePlacementError::AccessDenied;}else Candidate.Add(R);}
 const auto Error=ValidateRegions(Candidate);if(Error!=EFoundrylinePlacementError::None)return Error;if(Candidate.Num()!=Regions.Num()){Regions=MoveTemp(Candidate);++Revision;}return EFoundrylinePlacementError::None;
}
