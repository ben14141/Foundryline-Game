#pragma once

#include "CoreMinimal.h"
#include "Construction/FoundrylinePlacementSubsystem.h"

// Local draft, deliberately separate from authoritative world state and saved objects.
class FOUNDRYLINE_API FFoundrylineConstructionPlan
{
public:
	bool Begin(const FFoundrylinePlacementContext& InContext, int64 WorldRevision);
	bool Add(FFoundrylinePlacementRequest Request);
	bool RemoveLast();
	void Cancel();
	FFoundrylinePlanResult Preview(const UFoundrylinePlacementSubsystem& Placement) const;
	FFoundrylinePlanResult Commit(UFoundrylinePlacementSubsystem& Placement,
		const FFoundrylinePlacementContext& CurrentContext, int64 ConfirmedCostCents);
	bool IsActive() const { return bActive; }
	const TArray<FFoundrylinePlacementRequest>& GetRequests() const { return Requests; }
private:
	bool bActive = false;
	int64 BaseRevision = 0;
	FFoundrylinePlacementContext Context;
	TArray<FFoundrylinePlacementRequest> Requests;
};
