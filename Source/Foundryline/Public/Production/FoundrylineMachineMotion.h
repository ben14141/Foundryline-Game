#pragma once
#include "CoreMinimal.h"
struct FFoundrylineMachineMotion {
 static float Phase(float Remaining,float Duration,bool Running,float Speed,double Age){return Duration>0?FMath::Clamp(1.f-FMath::Max(0.f,Remaining-(Running?float(FMath::Clamp(Age,0.,.5))*Speed:0.f))/Duration,0.f,1.f):0.f;}
 static float Ease(float X){X=FMath::Clamp(X,0.f,1.f);return X*X*X*(X*(X*6-15)+10);}
 template<class StateType,class MachineType> static bool Active(const StateType& State,const MachineType& Machine){return State.Running&&Machine.Enabled&&Machine.Remaining>0&&State.Operators.ContainsByPredicate([&](const auto& O){return O.Machine==Machine.Id&&FVector::Dist2D(O.Position,O.WorkPoint)<=1;});}
 static float BrakeStroke(float P){return P<.12f?0:P<.30f?-26.f*Ease((P-.12f)/.18f):P<.45f?-26.f-5.5f*Ease((P-.30f)/.15f):P<.58f?-31.5f:P<.85f?-31.5f*(1-Ease((P-.58f)/.27f)):0;}
};
