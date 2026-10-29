#include "Construction/FoundrylineConstructionPlan.h"

bool FFoundrylineConstructionPlan::Begin(const FFoundrylinePlacementContext& InContext, int64 WorldRevision)
{
	if (bActive || !InContext.bCanConstruct || !InContext.CompanyId.IsValid()
		|| !InContext.PlantId.IsValid() || WorldRevision < 0) return false;
	Context = InContext;
	BaseRevision = WorldRevision;
	bActive = true;
	return true;
}

bool FFoundrylineConstructionPlan::Add(FFoundrylinePlacementRequest Request)
{
	if (!bActive || Requests.Num() >= 128) return false;
	Request.ExpectedRevision = BaseRevision;
	Requests.Add(Request);
	return true;
}

bool FFoundrylineConstructionPlan::RemoveLast()
{
	if (!bActive || Requests.IsEmpty()) return false;
	Requests.RemoveAt(Requests.Num() - 1);
	return true;
}

void FFoundrylineConstructionPlan::Cancel()
{
	Requests.Reset();
	Context = FFoundrylinePlacementContext();
	BaseRevision = 0;
	bActive = false;
}

FFoundrylinePlanResult FFoundrylineConstructionPlan::Preview(const UFoundrylinePlacementSubsystem& Placement) const
{
	if (bActive) return Placement.PreviewBatch(Requests, Context);
	FFoundrylinePlanResult Result;
	Result.Error = EFoundrylinePlacementError::InvalidAction;
	return Result;
}

FFoundrylinePlanResult FFoundrylineConstructionPlan::Commit(UFoundrylinePlacementSubsystem& Placement,
	const FFoundrylinePlacementContext& CurrentContext, int64 ConfirmedCostCents)
{
	if (!bActive) return Preview(Placement);
	if (CurrentContext.CompanyId != Context.CompanyId || CurrentContext.PlantId != Context.PlantId)
	{
		FFoundrylinePlanResult Result;
		Result.Error = EFoundrylinePlacementError::AccessDenied;
		return Result;
	}
	// Re-resolve permissions at commitment; opening a plan does not preserve old authority.
	auto Result = Placement.ExecuteBatch(Requests, CurrentContext, ConfirmedCostCents);
	if (Result.IsSuccess()) Cancel();
	return Result; // Failure retains the draft so the user can inspect or cancel it.
}
