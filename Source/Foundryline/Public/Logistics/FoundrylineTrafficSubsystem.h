#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Pathing/FoundrylineRouteNetwork.h"
#include "FoundrylineTrafficSubsystem.generated.h"

struct FFoundrylineTrafficClaim {
 FGuid Vehicle,From,Via,To;FVector2D A,B,C;float Width=0;int64 Revision=0;
};
struct FFoundrylineTrafficWait {FFoundrylineTrafficClaim Claim;uint64 Ticket=0;double LastSeen=0;bool ExitReady=false;};
// No ticking, construction changes or actor access. Called by a bounded server fleet scheduler.
struct FOUNDRYLINE_API FFoundrylineTrafficLedger {
 TArray<FFoundrylineTrafficClaim> Active;TArray<FFoundrylineTrafficWait> Waiting;
 uint64 NextTicket=0;
 bool Enter(const FFoundrylineRouteNetwork& Graph,FGuid Vehicle,FGuid From,FGuid Via,FGuid To,const FFoundrylineVehicleProfile& Profile,float ClearExitCm,double Now,FString& Reason);
 // Caller reports rear-of-trailer clearance, never just tractor-centre arrival.
 void RearCleared(FGuid Vehicle);
 void CancelWaiting(FGuid Vehicle);
 void ExpireWaiting(double Now);
 TMap<FGuid,float> Delays() const;
 static bool Conflicts(const FFoundrylineTrafficClaim& A,const FFoundrylineTrafficClaim& B);
};
UCLASS()
class FOUNDRYLINE_API UFoundrylineTrafficSubsystem : public UWorldSubsystem {
 GENERATED_BODY()
public:
 // Native server scheduler only: use a checked route and measured downstream free space.
 bool Enter(FGuid Vehicle,FGuid From,FGuid Via,FGuid To,const FFoundrylineVehicleProfile& Profile,float ClearExitCm,FString& Reason);
 bool RearCleared(FGuid Vehicle);
 const FFoundrylineTrafficLedger& GetLedger() const {return Ledger;}
private:
 FFoundrylineTrafficLedger Ledger;
};
