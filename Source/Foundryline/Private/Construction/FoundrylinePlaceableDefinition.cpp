#include "Construction/FoundrylinePlaceableDefinition.h"

bool UFoundrylinePlaceableDefinition::IsDefinitionValid() const
{
	const bool bContains = WorkingClearance.Min.X <= Footprint.Min.X
		&& WorkingClearance.Min.Y <= Footprint.Min.Y && WorkingClearance.Min.Z <= Footprint.Min.Z
		&& WorkingClearance.Max.X >= Footprint.Max.X && WorkingClearance.Max.Y >= Footprint.Max.Y
		&& WorkingClearance.Max.Z >= Footprint.Max.Z;
	return !DefinitionId.IsNone() && Footprint.IsValid() && WorkingClearance.IsValid() && bContains
		&& FFoundrylineGrid::SnapStep(Snap) > 0 && PriceCents >= 0 && AllowedSurfaces.Num() > 0
		&& !AllowedSurfaces.Contains(NAME_None) && !MeshOffsetCm.ContainsNaN()
		&& static_cast<uint8>(CollisionClass) <= static_cast<uint8>(EFoundrylineCollisionClass::Cosmetic)
		&& (CollisionClass != EFoundrylineCollisionClass::Cosmetic || FunctionId.IsNone());
}
