#include "UI/FoundrylinePlacementPreview.h"

FFoundrylinePlacementPreview FFoundrylinePlacementPreview::Build(const UFoundrylinePlacementSubsystem& Placement,
	const FFoundrylinePlacementContext& Context, FName DefinitionId, FVector GroundCm, uint8 QuarterTurns)
{
	FFoundrylinePlacementPreview Preview;
	Preview.Request.DefinitionId = DefinitionId;
	Preview.Request.ExpectedRevision = Placement.GetRevision();
	Preview.Request.QuarterTurns = QuarterTurns;
	Preview.Validation.Revision = Placement.GetRevision();
	const auto* Def = Placement.FindDefinition(DefinitionId);
	if (!Def) { Preview.Validation.Error = EFoundrylinePlacementError::UnknownDefinition; return Preview; }
	if (!Def->IsDefinitionValid()) { Preview.Validation.Error = EFoundrylinePlacementError::InvalidDefinition; return Preview; }
	if (GroundCm.ContainsNaN()) { Preview.Validation.Error = EFoundrylinePlacementError::InvalidTransform; return Preview; }
	GroundCm.Z = 0.0;
	if (QuarterTurns > 3 || !FFoundrylineGrid::SnapWorld(GroundCm, Def->Snap, Preview.Request.Anchor))
	{ Preview.Validation.Error = EFoundrylinePlacementError::InvalidTransform; return Preview; }
	Preview.Footprint = FFoundrylineGrid::RotateBounds(Def->Footprint, Preview.Request.Anchor, QuarterTurns);
	Preview.WorkingClearance = FFoundrylineGrid::RotateBounds(Def->WorkingClearance, Preview.Request.Anchor, QuarterTurns);
	Preview.Mesh = Def->Mesh;
	Preview.PriceCents = Def->PriceCents;
	const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(90.0 * QuarterTurns));
	Preview.MeshTransform = FTransform(Rotation,
		FFoundrylineGrid::ToWorld(Preview.Request.Anchor) + Rotation.RotateVector(Def->MeshOffsetCm));
	Preview.bHasGeometry = Preview.Footprint.IsValid() && Preview.WorkingClearance.IsValid();
	Preview.Validation = Placement.Validate(Preview.Request, Context);
	return Preview;
}

FText FFoundrylinePlacementPreview::ErrorText(EFoundrylinePlacementError Error)
{
	switch (Error)
	{
	case EFoundrylinePlacementError::None: return NSLOCTEXT("Foundryline", "PlacementReady", "Ready to place");
	case EFoundrylinePlacementError::NotAuthority: return NSLOCTEXT("Foundryline", "PlacementServer", "Waiting for placement validation");
	case EFoundrylinePlacementError::StaleRevision: return NSLOCTEXT("Foundryline", "PlacementStale", "The site changed. Review the plan again.");
	case EFoundrylinePlacementError::AccessDenied: return NSLOCTEXT("Foundryline", "PlacementAccess", "Requires permission, owned space and a compatible surface.");
	case EFoundrylinePlacementError::Occupied: return NSLOCTEXT("Foundryline", "PlacementOccupied", "Blocked by an object or required working clearance.");
	case EFoundrylinePlacementError::InvalidTransform: return NSLOCTEXT("Foundryline", "PlacementTransform", "Invalid position, snapping or rotation.");
	case EFoundrylinePlacementError::UnknownObject: return NSLOCTEXT("Foundryline", "PlacementMissingObject", "This object is no longer available.");
	case EFoundrylinePlacementError::CostLimitExceeded: return NSLOCTEXT("Foundryline", "PlacementCost", "Plan cost exceeds the amount approved.");
	case EFoundrylinePlacementError::TooManyActions: return NSLOCTEXT("Foundryline", "PlacementPlanSize", "The plan must contain 1 to 128 changes.");
	case EFoundrylinePlacementError::Busy: return NSLOCTEXT("Foundryline", "PlacementBusy", "Another site change is being applied.");
	default: return NSLOCTEXT("Foundryline", "PlacementInvalid", "This placement or its definition is invalid.");
	}
}
