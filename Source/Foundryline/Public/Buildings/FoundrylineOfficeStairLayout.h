#pragma once
#include "CoreMinimal.h"

// Real centimetres in the host office's local frame. Finished floors are Z20/Z340.
struct FOUNDRYLINE_API FFoundrylineOfficeStairLayout {
 static constexpr double Width=300;
 static constexpr double Depth=512;
 static constexpr double LandingDepth=130;
 static constexpr double FlightRun=252;
 static constexpr double Rise=320;
 static constexpr double HalfRise=160;
 static constexpr int32 RisersPerFlight=9;
 static FVector Point(FIntPoint Origin,int32 Quarter,FVector FramePoint);
 static FBox2D FullBounds(FIntPoint Origin,int32 Quarter,uint8 Style=0);
 static FBox2D HoleBounds(FIntPoint Origin,int32 Quarter,uint8 Style=0);
 static TArray<FBox2D> FloorPanels(double WidthCm,double DepthCm,FIntPoint Origin,int32 Quarter,uint8 Style=0);
 // Foot-root waypoints, excluding the capsule's standing half-height.
 static TArray<FVector> WalkRoute(FIntPoint Origin,int32 Quarter,uint8 Style=0);
};
