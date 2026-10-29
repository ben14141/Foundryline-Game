#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
#include "Construction/FoundrylineConstructionPlan.h"
#include "UI/FoundrylinePlacementPreview.h"
#include "SaveSystem/FoundrylineSaveSubsystem.h"

namespace
{
	FFoundrylineGridBounds Bounds(FIntVector Min, FIntVector Max)
	{
		FFoundrylineGridBounds Result;
		Result.Min = Min * 5;
		Result.Max = Max * 5;
		return Result;
	}

	struct FPlacementFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		UFoundrylinePlacementSubsystem* Placement = NewObject<UFoundrylinePlacementSubsystem>(World);
		UFoundrylinePlaceableDefinition* Definition = NewObject<UFoundrylinePlaceableDefinition>(World);
		FFoundrylinePlacementContext Context;
		FPlacementFixture()
		{
			Context.CompanyId = FGuid::NewGuid();
			Context.PlantId = FGuid::NewGuid();
			Context.bCanConstruct = true;
			Definition->DefinitionId = TEXT("TestRack");
			Definition->Snap = EFoundrylineSnap::Precision5cm;
			Definition->Footprint = Bounds(FIntVector(0, 0, 0), FIntVector(10, 20, 20));
			Definition->WorkingClearance = Bounds(FIntVector(-5, -5, 0), FIntVector(15, 25, 20));
			Definition->AllowedSurfaces.Add(TEXT("Concrete"));
			Placement->RegisterDefinition(Definition);
			FFoundrylineBuildRegion Region;
			Region.RegionId = FGuid::NewGuid();
			Region.CompanyId = Context.CompanyId;
			Region.PlantId = Context.PlantId;
			Region.SurfaceId = TEXT("Concrete");
			Region.Bounds = Bounds(FIntVector(-100, -100, 0), FIntVector(100, 100, 1));
			Placement->InitializeRegions({Region});
		}
		~FPlacementFixture() { World->DestroyWorld(false); }
		FFoundrylinePlacementRequest BuildAt(FIntVector Anchor) const
		{
			FFoundrylinePlacementRequest Request;
			Request.ExpectedRevision = Placement->GetRevision();
			Request.DefinitionId = Definition->DefinitionId;
			Request.Anchor = Anchor;
			return Request;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineGridTest, "Foundryline.Grid.ScaleSnapRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineGridTest::RunTest(const FString& Parameters)
{
	FIntVector Anchor;
	TestTrue(TEXT("Fine snap accepts negative world coordinates"),
		FFoundrylineGrid::SnapWorld(FVector(-38, 38, 0), EFoundrylineSnap::Fine25cm, Anchor));
	TestTrue(TEXT("25 cm snapping rounds in both directions"), Anchor == FIntVector(-50, 50, 0));
	TestTrue(TEXT("One metre is 100 UU"), FFoundrylineGrid::ToWorld(FIntVector(100, 0, 0)) == FVector(100, 0, 0));
	TestFalse(TEXT("Invalid snap enum fails closed"),
		FFoundrylineGrid::SnapWorld(FVector::ZeroVector, static_cast<EFoundrylineSnap>(255), Anchor));
	TestFalse(TEXT("Unsafe coordinates rejected"), FFoundrylineGrid::SnapWorld(
		FVector(1.e15, 0, 0), EFoundrylineSnap::Precision5cm, Anchor));
	const auto Local = Bounds(FIntVector(0, 0, 0), FIntVector(10, 20, 30));
	const auto Rotated = FFoundrylineGrid::RotateBounds(Local, FIntVector(25, 25, 0), 1);
	TestTrue(TEXT("Quarter turn preserves rectangular dimensions and anchor pivot"),
		Rotated.Min == FIntVector(-75, 25, 0) && Rotated.Max == FIntVector(25, 75, 150));
	TestFalse(TEXT("Touching faces permit adjacency"), Local.Overlaps(
		Bounds(FIntVector(10, 0, 0), FIntVector(20, 20, 30))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineActionTest, "Foundryline.Placement.AuthorityAndTransactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineActionTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	const auto Request = F.BuildAt(FIntVector::ZeroValue);
	const auto Built = F.Placement->Execute(Request, F.Context);
	TestTrue(TEXT("Build generates stable ID"), Built.IsSuccess() && Built.ObjectId.IsValid());
	if (!Built.IsSuccess()) return false;
	TestTrue(TEXT("Replayed stale action rejected"),
		F.Placement->Execute(Request, F.Context).Error == EFoundrylinePlacementError::StaleRevision);
	TestEqual(TEXT("Rejected action does not add another object"), F.Placement->GetObjects().Num(), 1);
	TestTrue(TEXT("Solid entering working clearance is blocked"),
		F.Placement->Validate(F.BuildAt(FIntVector(70, 0, 0)), F.Context).Error == EFoundrylinePlacementError::Occupied);
	TestTrue(TEXT("Overlapping empty working clearances are allowed"),
		F.Placement->Validate(F.BuildAt(FIntVector(75, 0, 0)), F.Context).IsSuccess());
	auto Foreign = F.Context;
	Foreign.CompanyId = FGuid::NewGuid();
	FFoundrylinePlacementRequest Remove;
	Remove.Action = EFoundrylinePlacementAction::Remove;
	Remove.ObjectId = Built.ObjectId;
	Remove.ExpectedRevision = F.Placement->GetRevision();
	TestTrue(TEXT("Foreign company cannot delete object"),
		F.Placement->Execute(Remove, Foreign).Error == EFoundrylinePlacementError::AccessDenied);
	auto Move = Remove;
	Move.Action = EFoundrylinePlacementAction::Move;
	Move.Anchor = FIntVector(475, 0, 0);
	TestTrue(TEXT("Clearance outside owned region is rejected"),
		F.Placement->Execute(Move, F.Context).Error == EFoundrylinePlacementError::AccessDenied);
	TestTrue(TEXT("Failed move leaves original position"), F.Placement->GetObjects()[0].Anchor == FIntVector::ZeroValue);
	Move.Anchor = FIntVector(150, 0, 0);
	TestTrue(TEXT("Valid move succeeds"), F.Placement->Execute(Move, F.Context).IsSuccess());
	auto Rotate = Move;
	Rotate.Action = EFoundrylinePlacementAction::Rotate;
	Rotate.ExpectedRevision = F.Placement->GetRevision();
	Rotate.QuarterTurns = 1;
	TestTrue(TEXT("Rotate succeeds"), F.Placement->Execute(Rotate, F.Context).IsSuccess());
	Remove.ExpectedRevision = F.Placement->GetRevision();
	TestTrue(TEXT("Owner can remove"), F.Placement->Execute(Remove, F.Context).IsSuccess());
	TestEqual(TEXT("Object removed"), F.Placement->GetObjects().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineSaveTest, "Foundryline.Placement.SaveRestoreAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineSaveTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	const auto Built = F.Placement->Execute(F.BuildAt(FIntVector::ZeroValue), F.Context);
	if (!TestTrue(TEXT("Fixture build succeeds"), Built.IsSuccess())) return false;
	auto* Snapshot = F.Placement->CreateSnapshot();
	auto* Restored = NewObject<UFoundrylinePlacementSubsystem>(F.World);
	Restored->RegisterDefinition(F.Definition);
	if (!TestTrue(TEXT("Restore valid snapshot"),
		Restored->RestoreSnapshot(Snapshot) == EFoundrylinePlacementError::None)) return false;
	TestTrue(TEXT("Object ID survives restore"), Restored->GetObjects()[0].ObjectId == Built.ObjectId);
	TestEqual(TEXT("Revision survives restore"), Restored->GetRevision(), Snapshot->Revision);
	auto* InvalidTarget = NewObject<UFoundrylinePlacementSubsystem>(F.World);
	InvalidTarget->RegisterDefinition(F.Definition);
	const auto DuplicateObject = Snapshot->Objects[0];
	Snapshot->Objects.Add(DuplicateObject);
	TestTrue(TEXT("Duplicate ID rejects entire save"),
		InvalidTarget->RestoreSnapshot(Snapshot) == EFoundrylinePlacementError::DuplicateId);
	TestEqual(TEXT("No partial restore"), InvalidTarget->GetObjects().Num(), 0);
	TestEqual(TEXT("Failed restore leaves revision untouched"), InvalidTarget->GetRevision(), int64(0));
	Snapshot->SchemaVersion = 999;
	TestTrue(TEXT("Unknown save schema rejected"),
		InvalidTarget->RestoreSnapshot(Snapshot) == EFoundrylinePlacementError::UnsupportedSave);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineDefinitionTest, "Foundryline.Placement.DefinitionAndCosmetics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineDefinitionTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	auto* Cosmetic = NewObject<UFoundrylinePlaceableDefinition>(F.World);
	Cosmetic->DefinitionId = TEXT("TestDecal");
	Cosmetic->Footprint = F.Definition->Footprint;
	Cosmetic->WorkingClearance = F.Definition->WorkingClearance;
	Cosmetic->AllowedSurfaces = F.Definition->AllowedSurfaces;
	Cosmetic->CollisionClass = EFoundrylineCollisionClass::Cosmetic;
	TestTrue(TEXT("Cosmetic definition registers"), F.Placement->RegisterDefinition(Cosmetic));
	const auto Built = F.Placement->Execute(F.BuildAt(FIntVector::ZeroValue), F.Context);
	if (!TestTrue(TEXT("Blocking rack built"), Built.IsSuccess())) return false;
	auto Request = F.BuildAt(FIntVector::ZeroValue);
	Request.DefinitionId = Cosmetic->DefinitionId;
	TestTrue(TEXT("Cosmetic may overlap physical object"), F.Placement->Execute(Request, F.Context).IsSuccess());
	Cosmetic->FunctionId = TEXT("Production");
	TestFalse(TEXT("Cosmetic cannot declare simulation function"), Cosmetic->IsDefinitionValid());
	Cosmetic->FunctionId = NAME_None;
	Cosmetic->AllowedSurfaces = {FName(TEXT("Asphalt"))};
	Request.ExpectedRevision = F.Placement->GetRevision();
	TestTrue(TEXT("Wrong surface rejected"),
		F.Placement->Validate(Request, F.Context).Error == EFoundrylinePlacementError::AccessDenied);
	auto Denied = F.Context;
	Denied.bCanConstruct = false;
	TestTrue(TEXT("Owner still needs construction permission"),
		F.Placement->Validate(F.BuildAt(FIntVector(200, 0, 0)), Denied).Error == EFoundrylinePlacementError::AccessDenied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineBatchTest, "Foundryline.Placement.PlanAtomicityAndCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineBatchTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	F.Definition->PriceCents = 12500;
	int32 Changes = 0;
	F.Placement->OnPlacementChanged.AddLambda([&](const FFoundrylinePlacementChange&) { ++Changes; });
	const int64 OriginalRevision = F.Placement->GetRevision();
	const TArray<FFoundrylinePlacementRequest> Invalid = {F.BuildAt(FIntVector::ZeroValue), F.BuildAt(FIntVector::ZeroValue)};
	const auto Rejected = F.Placement->ExecuteBatch(Invalid, F.Context, 100000);
	TestTrue(TEXT("Second overlapping operation rejects whole plan"),
		Rejected.Error == EFoundrylinePlacementError::Occupied && Rejected.FailedAction == 1);
	TestEqual(TEXT("No partial object commit"), F.Placement->GetObjects().Num(), 0);
	TestEqual(TEXT("Failed batch has no events"), Changes, 0);
	TestEqual(TEXT("Failed batch leaves revision unchanged"), F.Placement->GetRevision(), OriginalRevision);
	FFoundrylineConstructionPlan Plan;
	TestTrue(TEXT("Begin draft"), Plan.Begin(F.Context, OriginalRevision));
	Plan.Add(F.BuildAt(FIntVector::ZeroValue));
	Plan.Add(F.BuildAt(FIntVector(150, 0, 0)));
	const auto Quote = Plan.Preview(*F.Placement);
	TestTrue(TEXT("Quote prices whole plan"), Quote.IsSuccess() && Quote.CostCents == 25000);
	TestEqual(TEXT("Preview does not place objects"), F.Placement->GetObjects().Num(), 0);
	TestTrue(TEXT("Cost cap rejects commit"),
		Plan.Commit(*F.Placement, F.Context, 24999).Error == EFoundrylinePlacementError::CostLimitExceeded);
	TestTrue(TEXT("Failed commit retains draft"), Plan.IsActive());
	auto Revoked = F.Context;
	Revoked.bCanConstruct = false;
	TestTrue(TEXT("Revoked permission rejects old draft"),
		Plan.Commit(*F.Placement, Revoked, 25000).Error == EFoundrylinePlacementError::AccessDenied);
	const auto Committed = Plan.Commit(*F.Placement, F.Context, 25000);
	TestTrue(TEXT("Commit succeeds and closes draft"), Committed.IsSuccess() && !Plan.IsActive());
	TestEqual(TEXT("Two objects placed"), F.Placement->GetObjects().Num(), 2);
	TestEqual(TEXT("Batch has one revision"), F.Placement->GetRevision(), OriginalRevision + 1);
	TestEqual(TEXT("One event per change"), Changes, 2);
	FPlacementFixture Overflow;
	Overflow.Definition->PriceCents = MAX_int64;
	const auto Overflowed = Overflow.Placement->ExecuteBatch(
		{Overflow.BuildAt(FIntVector::ZeroValue), Overflow.BuildAt(FIntVector(150, 0, 0))}, Overflow.Context, MAX_int64);
	TestTrue(TEXT("Cost overflow fails closed"), Overflowed.Error == EFoundrylinePlacementError::CostOverflow);
	TestEqual(TEXT("Overflow never commits objects"), Overflow.Placement->GetObjects().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineChangeTest, "Foundryline.Placement.LocalChangeNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineChangeTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	bool bBoundsValid = false;
	bool bReentryRejected = false;
	F.Placement->OnPlacementChanged.AddLambda([&](const FFoundrylinePlacementChange& Change)
	{
		bBoundsValid = Change.bHasCurrent && !Change.bHadPrevious && Change.bAffectsNavigation && Change.CurrentBounds.IsValid();
		bReentryRejected = F.Placement->Execute(F.BuildAt(FIntVector(250, 0, 0)), F.Context).Error == EFoundrylinePlacementError::Busy;
	});
	const auto Built = F.Placement->Execute(F.BuildAt(FIntVector::ZeroValue), F.Context);
	TestTrue(TEXT("Event includes local bounds for a physical object"), Built.IsSuccess() && bBoundsValid);
	TestTrue(TEXT("Callbacks cannot recursively modify placement"), bReentryRejected);
	TestEqual(TEXT("Only the original action committed"), F.Placement->GetObjects().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylinePreviewTest, "Foundryline.Placement.PreviewAndSlotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylinePreviewTest::RunTest(const FString& Parameters)
{
	FPlacementFixture F;
	F.Definition->MeshOffsetCm = FVector(10, 0, 0);
	const auto Preview = FFoundrylinePlacementPreview::Build(*F.Placement, F.Context,
		F.Definition->DefinitionId, FVector(100, 0, 0), 1);
	TestTrue(TEXT("Preview validates without creating an object"), Preview.Validation.IsSuccess() && Preview.bHasGeometry);
	TestTrue(TEXT("Mesh pivot offset rotates with object"), Preview.MeshTransform.GetLocation().Equals(FVector(100, 10, 0), 0.001));
	TestEqual(TEXT("No preview mutations"), F.Placement->GetObjects().Num(), 0);
	TestTrue(TEXT("Valid slot name"), UFoundrylineSaveSubsystem::IsValidSlotName(TEXT("Starter_Shop-01")));
	TestFalse(TEXT("Paths cannot be used as slots"), UFoundrylineSaveSubsystem::IsValidSlotName(TEXT("../other")));
	TestFalse(TEXT("Empty slot rejected"), UFoundrylineSaveSubsystem::IsValidSlotName(TEXT("")));
	return true;
}

#endif
