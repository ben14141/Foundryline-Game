#include "Grid/FoundrylineGrid.h"

bool FFoundrylineGridBounds::IsValid() const
{
	return FFoundrylineGrid::IsSafeCoordinate(Min) && FFoundrylineGrid::IsSafeCoordinate(Max)
		&& Min.X < Max.X && Min.Y < Max.Y && Min.Z < Max.Z;
}

bool FFoundrylineGridBounds::Overlaps(const FFoundrylineGridBounds& Other) const
{
	return Min.X < Other.Max.X && Max.X > Other.Min.X
		&& Min.Y < Other.Max.Y && Max.Y > Other.Min.Y
		&& Min.Z < Other.Max.Z && Max.Z > Other.Min.Z;
}

int32 FFoundrylineGrid::SnapStep(EFoundrylineSnap Snap)
{
	switch (Snap)
	{
	case EFoundrylineSnap::Structural100cm: return 100;
	case EFoundrylineSnap::Fine25cm: return 25;
	case EFoundrylineSnap::Precision10cm: return 10;
	case EFoundrylineSnap::Precision5cm: return 5;
	default: return 0;
	}
}

bool FFoundrylineGrid::IsSafeCoordinate(const FIntVector& Value)
{
	return Value.X >= -MaxCoordinate && Value.X <= MaxCoordinate
		&& Value.Y >= -MaxCoordinate && Value.Y <= MaxCoordinate
		&& Value.Z >= -MaxCoordinate && Value.Z <= MaxCoordinate;
}

bool FFoundrylineGrid::IsOnSnap(const FIntVector& Anchor, EFoundrylineSnap Snap)
{
	const int32 Step = SnapStep(Snap);
	return Step > 0 && IsSafeCoordinate(Anchor)
		&& Anchor.X % Step == 0 && Anchor.Y % Step == 0 && Anchor.Z % Step == 0;
}

bool FFoundrylineGrid::SnapWorld(const FVector& WorldCm, EFoundrylineSnap Snap, FIntVector& OutAnchor)
{
	const int32 Step = SnapStep(Snap);
	if (Step == 0 || !FMath::IsFinite(WorldCm.X) || !FMath::IsFinite(WorldCm.Y)
		|| !FMath::IsFinite(WorldCm.Z)) return false;
	const FVector Units = WorldCm / UnitCm;
	if (Units.GetAbsMax() > MaxCoordinate - Step) return false;
	OutAnchor = FIntVector(FMath::RoundToInt(Units.X / Step) * Step,
		FMath::RoundToInt(Units.Y / Step) * Step, FMath::RoundToInt(Units.Z / Step) * Step);
	return true;
}

FVector FFoundrylineGrid::ToWorld(const FIntVector& Anchor)
{
	return FVector(Anchor) * UnitCm;
}

FIntVector FFoundrylineGrid::RotateOffset(const FIntVector& P,uint8 QuarterTurns)
{
 switch(QuarterTurns%4) {
 case 1:return {-P.Y,P.X,P.Z};
 case 2:return {-P.X,-P.Y,P.Z};
 case 3:return {P.Y,-P.X,P.Z};
 default:return P;
 }
}

FFoundrylineGridBounds FFoundrylineGrid::RotateBounds(const FFoundrylineGridBounds& Local,
	const FIntVector& Anchor, uint8 QuarterTurns)
{
	FFoundrylineGridBounds Result;
	Result.Min = FIntVector(MAX_int32, MAX_int32, MAX_int32);
	Result.Max = FIntVector(MIN_int32, MIN_int32, MIN_int32);
	for (int32 X : {Local.Min.X, Local.Max.X})
	{
		for (int32 Y : {Local.Min.Y, Local.Max.Y})
		{
			FIntVector Corner=RotateOffset({X,Y,0},QuarterTurns);
			Corner += Anchor;
			Result.Min.X = FMath::Min(Result.Min.X, Corner.X);
			Result.Min.Y = FMath::Min(Result.Min.Y, Corner.Y);
			Result.Max.X = FMath::Max(Result.Max.X, Corner.X);
			Result.Max.Y = FMath::Max(Result.Max.Y, Corner.Y);
		}
	}
	Result.Min.Z = Local.Min.Z + Anchor.Z;
	Result.Max.Z = Local.Max.Z + Anchor.Z;
	return Result;
}
