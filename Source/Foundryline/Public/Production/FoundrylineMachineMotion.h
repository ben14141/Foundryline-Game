#pragma once
#include "CoreMinimal.h"
struct FFoundrylineMachineMotion {
 static float Phase(float Remaining,float Duration,bool Running,float Speed,double Age){return Duration>0?FMath::Clamp(1.f-FMath::Max(0.f,Remaining-(Running?float(FMath::Clamp(Age,0.,.5))*Speed:0.f))/Duration,0.f,1.f):0.f;}
 static float BrakeStroke(float P){const auto Ease=[](float X){X=FMath::Clamp(X,0.f,1.f);return X*X*(3-2*X);};return P<.38f?-31.5f*Ease((P-.12f)/.26f):P<.55f?-31.5f:-31.5f*(1-Ease((P-.55f)/.3f));}
};
