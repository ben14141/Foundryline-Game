#pragma once
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
// Helpers shared by the welding station simulation files (Placement, Logistics, Assembly, Validation).
namespace FoundrylineWeldingStationInternal {
inline bool Overlap(const FBox2D& A,const FBox2D& B){return FFoundrylineFloorPlacement::Overlap(A,B);}
// Port-end bays first, then the Welder A and B sides of the access ring.
inline TArray<FBox2D> Access(const FFoundrylineWeldingStationRecord& S){return FFoundrylineWeldingStationLayout::AccessRing(S);}
}
