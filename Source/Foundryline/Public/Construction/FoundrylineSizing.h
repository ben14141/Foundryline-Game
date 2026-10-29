#pragma once
#include "CoreMinimal.h"

// One contract for drawing, resizing, saved definitions and the size inspector.
struct FFoundrylineSizing {
 static bool Road(FName T){return T==TEXT("Road2") || T==TEXT("Road4") || T==TEXT("Service") || T==TEXT("Driveway") || T==TEXT("TruckAccess");}
 static bool Surface(FName T){return Road(T) || T==TEXT("Parking") || T==TEXT("FreightYard") || T==TEXT("Walkway");}
 static int32 FixedWidth(FName T){return T==TEXT("Road2")?800:T==TEXT("Road4")?1400:T==TEXT("Service")?400:T==TEXT("Driveway")?600:0;}
 static FIntPoint Minimum(FName T){return {FixedWidth(T)?FixedWidth(T):T==TEXT("Parking")?600:T==TEXT("Walkway")?200:400,T==TEXT("Parking")?1200:T==TEXT("Walkway")?200:400};}
 static FIntPoint Default(FName T){return {FixedWidth(T)?FixedWidth(T):T==TEXT("TruckAccess")?1200:T==TEXT("Parking")?1200:T==TEXT("Walkway")?200:2000,T==TEXT("Parking")?1800:2000};}
 static bool Valid(FName T,int32 W,int32 D){const auto Min=Minimum(T);return W>=Min.X && D>=Min.Y && W<=6400 && D<=6400 && W%100==0 && D%100==0 && (!FixedWidth(T) || W==FixedWidth(T));}
 static FString Limits(FName T){const auto M=Minimum(T);return FixedWidth(T)?FString::Printf(TEXT("Width fixed %.0f m / length 4-64 m"),M.X/100.f):FString::Printf(TEXT("Min %.0f x %.0f m / max 64 x 64 m"),M.X/100.f,M.Y/100.f);}
};
