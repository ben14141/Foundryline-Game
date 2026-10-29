#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineKitMaterialTest,"Foundryline.Construction.KitMaterialsSupportInstancing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineKitMaterialTest::RunTest(const FString& Parameters)
{
	for(const auto* Name:{TEXT("Asphalt"),TEXT("Concrete"),TEXT("Wall"),TEXT("White"),TEXT("Yellow"),TEXT("Metal"),TEXT("Door"),TEXT("Glass"),TEXT("Preview")})
	{
		const FString Path=FString::Printf(TEXT("/Game/Foundryline/Construction/Materials/M_%s.M_%s"),Name,Name);
		auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path);
		TestTrue(FString::Printf(TEXT("%s must render on instanced construction, not fall back"),Name),Material && Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineBuildSiteTest,"Foundryline.Construction.RectangleRoadsAndOpenings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineBuildSiteTest::RunTest(const FString& Parameters)
{
	auto* World=UWorld::CreateWorld(EWorldType::Game,false);
	auto* Site=World->SpawnActor<AFoundrylineBuildSite>(); Site->DispatchBeginPlay();
	auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
	auto CountKind=[](const auto& Openings,EFoundrylineOpeningKind Kind) {return Openings.FilterByPredicate([&](const auto& O){return O.Kind==Kind;}).Num();};
	TestTrue(TEXT("Site ready"),Site->IsReady());
	TestEqual(TEXT("Starter infrastructure, T1 and connected T2"),Placement->GetObjects().Num(),5);
	const auto* Starter=Placement->GetObjects().FindByPredicate([&](const auto& O){return Placement->FindDefinition(O.DefinitionId)->FunctionId==TEXT("BuildingShell");});
	if(!TestNotNull(TEXT("Starter building"),Starter)) {World->DestroyWorld(false); return false;}
	TestEqual(TEXT("Personnel opening record"),CountKind(Starter->Openings,EFoundrylineOpeningKind::Personnel),1);
	TestEqual(TEXT("Garage opening record"),CountKind(Starter->Openings,EFoundrylineOpeningKind::Garage),1);
	TestTrue(TEXT("Automatic exterior window bays created"),CountKind(Starter->Openings,EFoundrylineOpeningKind::ShopWindow)>0);
	const FGuid StarterId=Starter->ObjectId;
	const auto BaseRevision=Placement->GetRevision();
	Site->PreviewRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0});
	TestEqual(TEXT("Preview cannot mutate construction records"),Placement->GetRevision(),BaseRevision);
	TestFalse(TEXT("Undersized rectangle rejected"),Site->BuildRectangle(TEXT("Building"),{3200,1000,0},{3400,1200,0}));
	TestFalse(TEXT("Oversized drag rejected"),Site->BuildRectangle(TEXT("Building"),{-6000,1000,0},{6000,2000,0}));
	TestFalse(TEXT("Outside property rejected"),Site->BuildRectangle(TEXT("Building"),{6000,1000,0},{7000,2000,0}));
	TestTrue(TEXT("Reverse drag creates full building atomically"),Site->BuildRectangle(TEXT("Building"),{4400,2000,0},{3200,1000,0}));
	TestEqual(TEXT("One record for entire new building"),Placement->GetObjects().Num(),6);
	TestFalse(TEXT("Overlapping building blocked"),Site->BuildRectangle(TEXT("Building"),{3200,1000,0},{4400,2000,0}));
	TestTrue(TEXT("Personnel door after creation"),Site->PlaceDoor(TEXT("Personnel"),{3200,1200,0}));
	TestTrue(TEXT("Garage door after creation"),Site->PlaceDoor(TEXT("Garage"),{3200,1600,0}));
	TestFalse(TEXT("Duplicate opening blocked"),Site->PlaceDoor(TEXT("Garage"),{3200,1600,0}));
	TestFalse(TEXT("Door needs real building wall"),Site->PlaceDoor(TEXT("Personnel"),{-5000,-4000,0}));
	const auto NewId=Placement->GetObjects().Last().ObjectId;
	Site->Undo();
	TestEqual(TEXT("Undo garage keeps personnel opening"),CountKind(Placement->GetObjects().Last().Openings,EFoundrylineOpeningKind::Personnel),1);
	TestEqual(TEXT("Undo removes garage opening"),CountKind(Placement->GetObjects().Last().Openings,EFoundrylineOpeningKind::Garage),0);
	Site->Undo();
	TestEqual(TEXT("Undo personnel restores facade window layout"),CountKind(Placement->GetObjects().Last().Openings,EFoundrylineOpeningKind::ShopWindow),8);
	Site->Undo();
	TestEqual(TEXT("Whole-building undo leaves starter site"),Placement->GetObjects().Num(),5);
	TestTrue(TEXT("Addition magnetically snaps to existing east wall"),Site->BuildRectangle(TEXT("Building"),{2900,-1000,0},{4000,0,0}));
	TestEqual(TEXT("Addition snapped anchor"),Placement->GetObjects().Last().Anchor.X,2800);
	FHitResult SharedHit;
	TestFalse(TEXT("Both shared walls removed for continuous interior"),World->LineTraceSingleByChannel(SharedHit,{2700,-800,350},{3000,-800,350},ECC_Visibility));
	TestFalse(TEXT("Door cannot be installed on removed internal seam"),Site->PlaceDoor(TEXT("Personnel"),{2800,-600,0}));
	Site->Undo();
	TestTrue(TEXT("Undo restores the original exterior wall"),World->LineTraceSingleByChannel(SharedHit,{2700,-800,350},{3000,-800,350},ECC_Visibility));
	TestTrue(TEXT("Service option"),Site->BuildRectangle(TEXT("Service"),{-5000,3000,0},{-5000,4000,0}));
	TestTrue(TEXT("Two-lane option"),Site->BuildRectangle(TEXT("Road2"),{-4000,3000,0},{-4000,4000,0}));
	TestTrue(TEXT("Four-lane option"),Site->BuildRectangle(TEXT("Road4"),{-2200,3000,0},{-2200,4000,0}));
	TestTrue(TEXT("Road branch trims and joins existing road edge"),Site->BuildRectangle(TEXT("Service"),{-2200,3500,0},{-500,3500,0}));
	TestFalse(TEXT("Curb and sidewalk are cut at branch connection"),World->LineTraceSingleByChannel(SharedHit,{-1600,3500,30},{-1400,3500,30},ECC_Visibility));
	TestTrue(TEXT("Parking rectangle"),Site->BuildRectangle(TEXT("Parking"),{1000,2500,0},{3500,4500,0}));
	for(const auto& O:Placement->GetObjects())
	{
		const auto* Def=Placement->FindDefinition(O.DefinitionId);
		if(Def->FunctionId!=TEXT("BuildingShell")) TestEqual(TEXT("Every road is a 20 cm slab"),Def->Footprint.Max.Z,20);
	}
	// Rendering data must reserve real depth and leave usable openings, not paper faces.
	bool FoundFoundation=false,FoundRoad=false,FoundWall=false,FoundWhite=false,FoundYellow=false;
	for(TActorIterator<AActor> It(World);It;++It)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
		for(auto* C:Components)
		{
			if(C->GetCollisionEnabled()==ECollisionEnabled::NoCollision) continue;
			for(int32 I=0;I<C->GetInstanceCount();++I)
			{
				FTransform T; C->GetInstanceTransform(I,T,true);
				const FVector Size=T.GetScale3D()*100;
				if(FMath::IsNearlyEqual(Size.Z,20.,0.01))
				{
					TestTrue(TEXT("Slab bottom touches ground and top is 20 cm"),FMath::IsNearlyEqual(T.GetLocation().Z,10.,0.01));
					FoundRoad|=C->GetMaterial(0)->GetName()==TEXT("M_Asphalt");
					FoundFoundation|=C->GetMaterial(0)->GetName()==TEXT("M_Concrete");
				}
				if(FMath::IsNearlyEqual(Size.Z,600.,0.01) && (FMath::IsNearlyEqual(Size.X,29.,0.01) || FMath::IsNearlyEqual(Size.Y,29.,0.01))) FoundWall=true;
			}
		}
		for(auto* C:Components)
		{
			if(!C->GetMaterial(0)) continue;
			FoundWhite|=C->GetMaterial(0)->GetName()==TEXT("M_White") && C->GetInstanceCount()>0;
			FoundYellow|=C->GetMaterial(0)->GetName()==TEXT("M_Yellow") && C->GetInstanceCount()>0;
		}
	}
	TestTrue(TEXT("Foundation slab rendered"),FoundFoundation); TestTrue(TEXT("Road slab rendered"),FoundRoad);
	TestTrue(TEXT("29 cm wall body rendered (plus 1 cm interior finish)"),FoundWall); TestTrue(TEXT("White road paint rendered"),FoundWhite); TestTrue(TEXT("Yellow road paint rendered"),FoundYellow);
	Site->HidePreview();
	FHitResult Hit;
	TestFalse(TEXT("Personnel opening is clear through concrete"),World->LineTraceSingleByChannel(Hit,{200,-950,120},{600,-950,120},ECC_Visibility));
	TestTrue(TEXT("Concrete next to opening blocks"),World->LineTraceSingleByChannel(Hit,{200,-1100,120},{600,-1100,120},ECC_Visibility));
	TestFalse(TEXT("Garage opening is clear through concrete"),World->LineTraceSingleByChannel(Hit,{200,-200,200},{600,-200,200},ECC_Visibility));
	TestFalse(TEXT("No roof across building interior"),World->LineTraceSingleByChannel(Hit,{1000,0,1000},{1000,0,100},ECC_Visibility));
	FFoundrylinePlacementContext Context;
	Context.CompanyId=FGuid(0xF001,1,1,1); Context.PlantId=FGuid(0xF001,2,1,1); Context.bCanConstruct=true;
	FFoundrylinePlacementRequest Bad; Bad.Action=EFoundrylinePlacementAction::AddOpening;
	Bad.ExpectedRevision=Placement->GetRevision(); Bad.ObjectId=StarterId;
	Bad.Opening.OpeningId=FGuid::NewGuid(); Bad.Opening.Edge=4;
	TestFalse(TEXT("Forged wall edge rejected"),Placement->Execute(Bad,Context).IsSuccess());
	Bad.Opening.Edge=0; Bad.Opening.OffsetCm=0;
	TestFalse(TEXT("Corner opening rejected"),Placement->Execute(Bad,Context).IsSuccess());
	Bad.Opening.OffsetCm=500; Context.CompanyId=FGuid::NewGuid();
	TestFalse(TEXT("Foreign company cannot edit building"),Placement->Execute(Bad,Context).IsSuccess());
	TArray<uint8> Bytes;
	TestTrue(TEXT("Opening records serialize"),UGameplayStatics::SaveGameToMemory(Placement->CreateSnapshot(),Bytes));
	auto* Save=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("Snapshot deserializes"),Save);
	auto* Restored=NewObject<UFoundrylinePlacementSubsystem>(World);
	for(const auto& O:Placement->GetObjects()) if(!Restored->FindDefinition(O.DefinitionId)) Restored->RegisterDefinition(const_cast<UFoundrylinePlaceableDefinition*>(Placement->FindDefinition(O.DefinitionId)));
	TestTrue(TEXT("Opening snapshot restores with validation"),Restored->RestoreSnapshot(Save)==EFoundrylinePlacementError::None);
	const auto* SavedBuilding=Restored->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==StarterId;});
	TestTrue(TEXT("Both doors survive serialization and restore"),SavedBuilding && CountKind(SavedBuilding->Openings,EFoundrylineOpeningKind::Personnel)==1 && CountKind(SavedBuilding->Openings,EFoundrylineOpeningKind::Garage)==1);
	World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineBuildingTierTest,"Foundryline.Construction.BuildingTiersAndHeightTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineBuildingTierTest::RunTest(const FString& Parameters)
{
	auto* World=UWorld::CreateWorld(EWorldType::Game,false);
	auto* Site=World->SpawnActor<AFoundrylineBuildSite>(); Site->DispatchBeginPlay();
	auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
	TestTrue(TEXT("High-bay addition can join starter T1"),Site->BuildRectangle(TEXT("Shop2"),{2900,-1000,0},{4400,0,0}));
	const auto HighBay=Placement->GetObjects().Last();
	TestEqual(TEXT("T2 snaps to T1 edge"),HighBay.Anchor.X,2800);
	for(const auto& Opening:HighBay.Openings)
	{
		TestEqual(TEXT("T2 window sill is 9 m above finished floor"),Opening.BottomCm(),900);
		TestEqual(TEXT("T2 glazing ends near the 12 m wall top"),Opening.BottomCm()+Opening.HeightCm(),1080);
	}
	TArray<uint8> TierBytes;
	TestTrue(TEXT("Tier window elevations serialize"),UGameplayStatics::SaveGameToMemory(Placement->CreateSnapshot(),TierBytes));
	auto* TierSave=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(TierBytes));
	TestNotNull(TEXT("Tier snapshot deserializes"),TierSave);
	if(TierSave)
	{
		const auto* Saved=TierSave->Objects.FindByPredicate([&](const auto& Object){return Object.ObjectId==HighBay.ObjectId;});
		TestTrue(TEXT("High windows survive save/load"),Saved && !Saved->Openings.IsEmpty() && Saved->Openings[0].WindowSillCm==900);
	}
	TestEqual(TEXT("T2 reserves real 12 m walls plus slab"),Placement->FindDefinition(HighBay.DefinitionId)->Footprint.Max.Z,1220);
	FHitResult Hit;
	TestFalse(TEXT("Mixed-height seam open below lower wall top"),World->LineTraceSingleByChannel(Hit,{2700,-800,350},{3000,-800,350},ECC_Visibility));
	TestTrue(TEXT("Upper height-transition wall remains with roofs hidden"),World->LineTraceSingleByChannel(Hit,{2700,-800,900},{3000,-800,900},ECC_Visibility));
	TestTrue(TEXT("Structural transition closes up to the tall ceiling"),World->LineTraceSingleByChannel(Hit,{2700,-800,1210},{3000,-800,1210},ECC_Visibility));
	TestTrue(TEXT("High bay exterior blocks above T1 height"),World->LineTraceSingleByChannel(Hit,{4300,-800,1100},{4500,-800,1100},ECC_Visibility));
	TestFalse(TEXT("High bay wall ends at 12 m above slab"),World->LineTraceSingleByChannel(Hit,{4300,-800,1250},{4500,-800,1250},ECC_Visibility));
	TestFalse(TEXT("High bay retains roof-free gameplay interior"),World->LineTraceSingleByChannel(Hit,{3600,-500,1500},{3600,-500,100},ECC_Visibility));
	TestTrue(TEXT("Standard garage installs on T2"),Site->PlaceDoor(TEXT("Garage"),{4400,-600,0}));
	Site->Undo(); Site->Undo();
	TestTrue(TEXT("Undo high bay restores original low wall"),World->LineTraceSingleByChannel(Hit,{2700,-800,350},{3000,-800,350},ECC_Visibility));
	TestFalse(TEXT("Undo removes upper transition"),World->LineTraceSingleByChannel(Hit,{2700,-800,900},{3000,-800,900},ECC_Visibility));
	const auto BeforePreview=Placement->GetRevision();
	Site->PreviewRectangle(TEXT("Building"),{2900,-1100,0},{4400,300,0});
	TestTrue(TEXT("Connection preview aligns both nearby exterior corners"),Site->PreviewBounds.Min==FIntVector(2800,-1200,0) && Site->PreviewBounds.Max==FIntVector(4400,400,620));
	TestTrue(TEXT("Preview marks the real shared edge"),Site->PreviewJoinLines.Num()==2 && FMath::IsNearlyEqual(FVector::Distance(Site->PreviewJoinLines[0],Site->PreviewJoinLines[1]),1600.));
	TestEqual(TEXT("Join preview cannot mutate neighbors"),Placement->GetRevision(),BeforePreview);
	TestTrue(TEXT("Placement uses the exact same connection as the preview"),Site->BuildRectangle(TEXT("Building"),{2900,-1100,0},{4400,300,0}));
	TestFalse(TEXT("Connected equal-height sections remain open inside"),World->LineTraceSingleByChannel(Hit,{2700,-800,350},{3000,-800,350},ECC_Visibility));
	Site->Undo();
	World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineDetailedShowcaseTest,"Foundryline.Construction.DetailedLargeSiteAndTruckClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineDetailedShowcaseTest::RunTest(const FString& Parameters)
{
	auto* World=UWorld::CreateWorld(EWorldType::Game,false);
	auto* Site=World->SpawnActor<AFoundrylineBuildSite>(); Site->bLargeShowcase=true; Site->DispatchBeginPlay();
	auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
	TestTrue(TEXT("Large site ready"),Site->IsReady());
	TestEqual(TEXT("Three road runs, two shops, office annex, parking and two driveways"),Placement->GetObjects().Num(),9);
	const auto* Tall=Placement->GetObjects().FindByPredicate([&](const auto& Object){return Placement->FindDefinition(Object.DefinitionId)->Footprint.Max.Z==1220;});
	if(!TestNotNull(TEXT("Large high bay exists"),Tall)) {World->DestroyWorld(false); return false;}
	const auto TallId=Tall->ObjectId;
	bool HasGrass=false;
	for(TActorIterator<AActor> It(World);It;++It)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
		for(auto* Component:Components)
			if(Component->GetStaticMesh() && Component->GetStaticMesh()->GetName()==TEXT("SM_Grass_02"))
			{
				HasGrass|=Component->GetInstanceCount()>0;
				TestTrue(TEXT("Parking grass is decorative"),Component->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
			}
	}
	TestTrue(TEXT("Grass-filled curb borders are present"),HasGrass);
	auto* Green=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/World/Materials/M_FlatForestGround.M_FlatForestGround"));
	TestTrue(TEXT("Green edging material supports instanced rendering"),Green && Green->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	TestEqual(TEXT("Shop is 48 m wide"),Placement->FindDefinition(Tall->DefinitionId)->Footprint.Max.X,4800);
	TestEqual(TEXT("Shop is 36 m deep"),Placement->FindDefinition(Tall->DefinitionId)->Footprint.Max.Y,3600);
	TestTrue(TEXT("Semi entrance has its own saved opening kind"),Tall->Openings.ContainsByPredicate([](const auto& Opening){return Opening.Kind==EFoundrylineOpeningKind::TruckGarage && Opening.WidthCm()==500 && Opening.HeightCm()==500;}));
	FHitResult Hit;
	TestFalse(TEXT("4.9 m wide and high clearance box passes through 5 x 5 m semi doorway"),World->SweepSingleByChannel(Hit,{200,2250,270},{600,2250,270},FQuat::Identity,ECC_Visibility,FCollisionShape::MakeBox(FVector(20,245,245))));
	TestTrue(TEXT("Wall above semi entrance remains solid"),World->LineTraceSingleByChannel(Hit,{200,2250,600},{600,2250,600},ECC_Visibility));
	TestFalse(TEXT("Joined factory has open lower seam"),World->LineTraceSingleByChannel(Hit,{2000,-100,300},{2000,100,300},ECC_Visibility));
	auto ColumnCount=[&]()
	{
		int32 Count=0;
		for(TActorIterator<AActor> It(World);It;++It)
		{
			TInlineComponentArray<UInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
			for(auto* Component:Components)
				for(int32 I=0;I<Component->GetInstanceCount();++I)
				{
					FTransform Transform; Component->GetInstanceTransform(I,Transform);
					if(FMath::IsNearlyEqual(Transform.GetScale3D().Z*100,1200.,0.01)) ++Count;
				}
		}
		return Count;
	};
	const int32 DefaultColumns=ColumnCount();
	TestTrue(TEXT("Column bay adjustment to 4 m"),Site->AdjustColumns({2000,2000,0},400));
	TestTrue(TEXT("Narrower bays visibly increase column count"),ColumnCount()>DefaultColumns);
	auto FindTall=[&](){return Placement->GetObjects().FindByPredicate([&](const auto& Object){return Object.ObjectId==TallId;});};
	TestEqual(TEXT("Column spacing is authoritative state"),FindTall()->ColumnBayCm,400);
	TestFalse(TEXT("Unsupported spacing cannot be committed"),Site->AdjustColumns({2000,2000,0},300));
	Site->Undo(); TestEqual(TEXT("Undo restores 6 m bays"),FindTall()->ColumnBayCm,600);
	TestTrue(TEXT("Click cycle selects 8 m"),Site->AdjustColumns({2000,2000,0}));
	TestEqual(TEXT("Cycle updates per-building spacing"),FindTall()->ColumnBayCm,800);
	TArray<uint8> Bytes;
	TestTrue(TEXT("Bay settings serialize"),UGameplayStatics::SaveGameToMemory(Placement->CreateSnapshot(),Bytes));
	auto* Save=Cast<UFoundrylinePlacementSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	const auto* Saved=Save?Save->Objects.FindByPredicate([&](const auto& Object){return Object.ObjectId==TallId;}):nullptr;
	TestTrue(TEXT("Column spacing survives serialization"),Saved && Saved->ColumnBayCm==800);
	FFoundrylinePlacementContext Foreign; Foreign.CompanyId=FGuid::NewGuid(); Foreign.PlantId=FGuid(0xF001,2,1,1); Foreign.bCanConstruct=true;
	FFoundrylinePlacementRequest R; R.Action=EFoundrylinePlacementAction::SetColumnBay; R.ObjectId=TallId; R.ExpectedRevision=Placement->GetRevision(); R.ColumnBayCm=400;
	TestFalse(TEXT("Other companies cannot change columns"),Placement->Execute(R,Foreign).IsSuccess());
	World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineAllHologramsTest,"Foundryline.Construction.AllPlacementPartsAreHolograms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineAllHologramsTest::RunTest(const FString& Parameters)
{
	auto* World=UWorld::CreateWorld(EWorldType::Game,false);
	auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();
	auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();
	const auto Revision=Placement->GetRevision();const int32 ObjectCount=Placement->GetObjects().Num();
	auto Check=[&](const FString& Tool)
	{
		int32 Instances=0;
		for(TActorIterator<AActor> It(World);It;++It)
		{
			if(!It->ActorHasTag(TEXT("FoundrylinePlacementPreview")) || It->IsActorBeingDestroyed()) continue;
			TInlineComponentArray<UInstancedStaticMeshComponent*> Components;It->GetComponents(Components);
			for(auto* C:Components)
			{
				Instances+=C->GetInstanceCount();
				TestTrue(Tool+TEXT(" ghost cannot block placement or picking"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
				for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
					TestTrue(Tool+TEXT(" every part uses a translucent hologram material"),C->GetMaterial(Slot) && C->GetMaterial(Slot)->GetBlendMode()==BLEND_Translucent);
			}
		}
		TestTrue(Tool+TEXT(" preview exists"),Instances>0);
		TestEqual(TEXT("Draft does not change revision"),Placement->GetRevision(),Revision);
		TestEqual(TEXT("Draft does not add placed objects"),Placement->GetObjects().Num(),ObjectCount);
		Site->HidePreview();
	};
	for(const auto* Tool:{TEXT("Building"),TEXT("Shop2"),TEXT("Service"),TEXT("Road2"),TEXT("Road4"),TEXT("Parking")})
	{Site->PreviewRectangle(Tool,{3200,2500,0},{5000,4500,0});Check(Tool);}
 Site->PreviewRectangle(TEXT("Office"),{2900,-1100,0},{4400,300,0});
 bool OfficeWalls=false;
 for(TActorIterator<AActor> It(World);It;++It) if(It->ActorHasTag(TEXT("FoundrylinePlacementPreview")) && !It->IsActorBeingDestroyed()) {
  TInlineComponentArray<UInstancedStaticMeshComponent*> Cs;It->GetComponents(Cs);
  for(auto* C:Cs) for(int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;C->GetInstanceTransform(I,T);OfficeWalls|=T.GetScale3D().Z>=2.8;}
 }
 TestTrue(TEXT("Exterior office hologram includes full-height walls, not only its floor"),OfficeWalls);
 Check(TEXT("Office annex"));
	for(const auto* Tool:{TEXT("Personnel"),TEXT("Garage"),TEXT("TruckDoor")})
	{Site->PreviewDoor(Tool,{400,-900,0});Check(Tool);}
	Site->PreviewColumns({1200,800,0});Check(TEXT("Columns"));
	Site->PreviewRectangle(TEXT("Building"),{400,-1200,0},{2800,400,0});Check(TEXT("Blocked building"));
	World->DestroyWorld(false);return true;
}
#endif
