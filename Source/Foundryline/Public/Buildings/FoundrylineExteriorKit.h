#pragma once
#include "Construction/FoundrylinePlacementTypes.h"

// Game-owned fixed-gauge exterior detail kit. Positions and sizes are real centimetres.
// Presentation derives from saved building/opening data; no per-part actors or replicated state.
struct FOUNDRYLINE_API FFoundrylineExteriorKit {
 struct FPart {FVector Center,Size;FName Finish,Role;uint8 Edge;float Along;};
 static void ClipJoinedFaces(TArray<FVector2D> (&Spans)[4],float Width,float Depth,FVector2D NeighborMin,FVector2D NeighborMax);
 static TArray<FPart> Generate(const FFoundrylinePlacedObject& Building,float Width,float Depth,float RoofZ,const TArray<FVector2D> (&Exposed)[4]);
};
