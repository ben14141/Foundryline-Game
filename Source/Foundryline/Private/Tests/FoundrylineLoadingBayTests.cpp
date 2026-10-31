#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylineLoadingBayLayout.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadingBayLifecycle,"Foundryline.Construction.LoadingBay.PresetLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLoadingBayLifecycle::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();auto* Driving=W->GetSubsystem<UFoundrylineVehiclePathSubsystem>();
 TestTrue(TEXT("Test shop"),Site->BuildRectangle(TEXT("Building"),{-5000,1800,0},{-2600,4200,0}));if(!Site->PlaceDoor(TEXT("LoadingBay"),{-2600,3000,0})){AddError(TEXT("Cannot place starter bay: ")+Site->Status);W->DestroyWorld(false);return false;}
 Site->BeginEdit(TEXT("Select"),{-2200,3000,0});auto Bay=Site->GetSelectedOpening();if(!Bay){AddError(TEXT("Bay must be selectable from its apron"));W->DestroyWorld(false);return false;}const FGuid Id=Bay->OpeningId;const FGuid Object=Site->SelectedObject;TestEqual(TEXT("Starter tier"),int(Bay->LoadingTier),1);
 auto Target=[&]()->FFoundrylineVehicleTarget {const auto M=Driving->GetMap();const auto* T=M->Targets.FindByPredicate([&](const auto& V){return V.Id==Id;});return T?*T:FFoundrylineVehicleTarget();};
 TestTrue(TEXT("One stable loading target and driving apron"),Target().Id==Id&&Driving->GetMap()->Surfaces.ContainsByPredicate([&](const auto& V){return V.Id==FFoundrylineLoadingBayLayout::SurfaceId(Id);}));TestTrue(TEXT("Actual starter hauler profile fits"),Target().SmallTruckClear);if(!Target().SmallTruckClear){const auto T=Target();AddInfo(FString::Printf(TEXT("Pose %s yaw %f offset %f tier %d"),*T.Pose.Position.ToString(),T.Pose.Yaw,T.LoadingOffset,T.LoadingTier));for(const auto& Surface:Driving->GetMap()->Surfaces)if(Surface.Id==FFoundrylineLoadingBayLayout::SurfaceId(Id))AddInfo(FString::Printf(TEXT("Apron %s %s size %s"),*Surface.Transform.GetLocation().ToString(),*Surface.Transform.Rotator().ToString(),*Surface.Size.ToString()));for(float X:{-223.f,870.f})for(float Y:{-158.f,158.f})AddInfo(FString::Printf(TEXT("Corner %f,%f = %d"),X,Y,Driving->GetMap()->PointAllowed(T.Pose.Position+FVector2D(X,Y),true,false,Id)));}
 const auto OldRevision=P->GetRevision();Site->PreviewEdit(TEXT("UpgradeBay"),{},{ });TestEqual(TEXT("Upgrade preview does not mutate records"),P->GetRevision(),OldRevision);TestTrue(TEXT("Upgrade to box truck"),Site->CommitEdit(TEXT("UpgradeBay"),{},{}));TestTrue(TEXT("Upgrade keeps target identity"),Target().Id==Id&&Target().LoadingTier==2);TestTrue(TEXT("Upgrade to semi"),Site->CommitEdit(TEXT("UpgradeBay"),{},{}));TestTrue(TEXT("Semi keeps target identity"),Target().Id==Id&&Target().LoadingTier==3);TestFalse(TEXT("No tier beyond semi"),Site->CommitEdit(TEXT("UpgradeBay"),{},{}));
 auto* Save=P->CreateSnapshot();TArray<uint8> Bytes;FMemoryWriter Writer(Bytes);FObjectAndNameAsStringProxyArchive Out(Writer,false);Save->Serialize(Out);auto* Loaded=NewObject<UFoundrylinePlacementSave>();FMemoryReader Reader(Bytes);FObjectAndNameAsStringProxyArchive In(Reader,true);Loaded->Serialize(In);TestTrue(TEXT("Tier and identity survive real save archive"),Loaded->Objects.ContainsByPredicate([&](const auto& O){return O.ObjectId==Object&&O.Openings.ContainsByPredicate([&](const auto& V){return V.OpeningId==Id&&V.LoadingTier==3;});}));
 TestTrue(TEXT("Move complete bay"),Site->CommitEdit(TEXT("MoveDoor"),{-2600,3000,0},{-3800,1800,0}));TestTrue(TEXT("Move retains tier and target"),Target().Id==Id&&Target().LoadingTier==3);
 TestTrue(TEXT("Delete complete bay"),Site->CommitEdit(TEXT("Demolish"),{},{}));TestFalse(TEXT("Deleted target removed"),Target().Id.IsValid());TestFalse(TEXT("Deleted apron removed"),Driving->GetMap()->Surfaces.ContainsByPredicate([&](const auto& V){return V.Id==FFoundrylineLoadingBayLayout::SurfaceId(Id);}));Site->Undo();TestTrue(TEXT("Undo restores whole bay"),Target().Id==Id&&Target().LoadingTier==3);
 // Invalid tiers cannot enter via saved/network construction data.
 auto Bad=*P->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==Object;});Bad.Openings.FindByPredicate([&](const auto& V){return V.OpeningId==Id;})->LoadingTier=4;FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Object;R.Replacement=Bad;R.ExpectedRevision=P->GetRevision();FFoundrylinePlacementContext C;C.CompanyId=Bad.CompanyId;C.PlantId=Bad.PlantId;C.bCanConstruct=true;TestEqual(TEXT("Malformed tier rejected"),P->Validate(R,C).Error,EFoundrylinePlacementError::InvalidTransform);
 W->DestroyWorld(false);return true;
}
#endif
