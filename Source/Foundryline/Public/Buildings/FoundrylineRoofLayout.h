#pragma once
#include "Construction/FoundrylinePlacementTypes.h"
struct FOUNDRYLINE_API FFoundrylineRoofLayout {
 struct FWindow {FVector2D Center,Size;bool Office=false;FBox2D Bounds()const{return {Center-Size/2,Center+Size/2};}};
 struct FFixture {FVector2D Center,Size;};
 static TArray<FBox2D> OfficeZones(const FFoundrylinePlacedObject& Object,float Width,float Depth);
 static TArray<FWindow> Windows(const FFoundrylinePlacedObject& Object,float Width,float Depth);
 static TArray<FBox2D> SolidPanels(float Width,float Depth,const TArray<FWindow>& Windows);
 static TArray<FFixture> OfficeFixtures(const FFoundrylinePlacedObject& Object,float Width,float Depth);
};
