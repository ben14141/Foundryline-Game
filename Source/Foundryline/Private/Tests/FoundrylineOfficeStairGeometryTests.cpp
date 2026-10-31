#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "Buildings/FoundrylineOfficeStairComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeStairFloorGeometry,"Foundryline.Construction.OfficeStairs.FloorHoleAndRoute",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeStairFloorGeometry::RunTest(const FString&){
 using L=FFoundrylineOfficeStairLayout;
 TestTrue(TEXT("Ramp slope is walkable without changing character settings"),FMath::RadiansToDegrees(FMath::Atan2(L::HalfRise,L::FlightRun))<35);
 for(int Q=0;Q<4;++Q){const FIntPoint Origin=Q==0?FIntPoint(200,200):Q==1?FIntPoint(800,200):Q==2?FIntPoint(800,800):FIntPoint(200,800);const auto Full=L::FullBounds(Origin,Q),Hole=L::HoleBounds(Origin,Q);const auto Panels=L::FloorPanels(1200,1200,Origin,Q);
  TestTrue(TEXT("Landing stays outside the floor opening"),FMath::IsNearlyEqual(Full.GetArea()-Hole.GetArea(),300.*130.,.01));double Area=0;
  for(int I=0;I<Panels.Num();++I){const auto& A=Panels[I];Area+=A.GetArea();TestTrue(TEXT("Floor panel stays in office"),A.Min.X>=0&&A.Min.Y>=0&&A.Max.X<=1200&&A.Max.Y<=1200);const auto Min=A.Min.ComponentMax(Hole.Min),Max=A.Max.ComponentMin(Hole.Max);TestFalse(TEXT("No solid floor crosses the stair headroom opening"),Min.X<Max.X-.01&&Min.Y<Max.Y-.01);for(int J=0;J<I;++J){const auto Low=A.Min.ComponentMax(Panels[J].Min),High=A.Max.ComponentMin(Panels[J].Max);TestFalse(TEXT("Floor panels do not double-cover each other"),Low.X<High.X-.01&&Low.Y<High.Y-.01);}}
  TestTrue(TEXT("Floor area preserves precisely the opening"),FMath::IsNearlyEqual(Area,1200.*1200.-Hole.GetArea(),.01));
  const auto Route=L::WalkRoute(Origin,Q);TestEqual(TEXT("Six waypoints include both landings and the turn"),Route.Num(),6);TestEqual(TEXT("Starts on lower floor"),Route[0].Z,20.);TestEqual(TEXT("Middle landing height"),Route[2].Z,180.);TestEqual(TEXT("Ends on upper floor"),Route.Last().Z,340.);for(const auto& P:Route)TestTrue(TEXT("Walking route remains inside full footprint"),Full.IsInsideOrOn(FVector2D(P)));
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOfficeStairCollisionGeometry,"Foundryline.Construction.OfficeStairs.SmoothCollisionAndPreview",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOfficeStairCollisionGeometry::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AActor>();auto* C=NewObject<UFoundrylineOfficeStairComponent>(A);A->SetRootComponent(C);C->RegisterComponent();A->AddInstanceComponent(C);C->Build({0,0},0,nullptr,nullptr,false);
 TArray<UPrimitiveComponent*> Parts;A->GetComponents(Parts);int Ramps=0,Treads=0;for(auto* P:Parts){Ramps+=P->ComponentHasTag(TEXT("FoundrylineOfficeStairRamp"));if(P->ComponentHasTag(TEXT("FoundrylineOfficeStairTread"))){++Treads;TestEqual(TEXT("Visible treads do not introduce step collision jitter"),P->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}}
 TestEqual(TEXT("Two continuous collision flights"),Ramps,2);TestEqual(TEXT("Eighteen actual treads"),Treads,18);
 auto Surface=[&](FVector At,double Expected){FHitResult Hit;const bool Found=W->LineTraceSingleByChannel(Hit,At+FVector(0,0,700),At-FVector(0,0,50),ECC_Visibility);TestTrue(*FString::Printf(TEXT("Real collision supports route at %s"),*At.ToCompactString()),Found);if(Found)TestTrue(TEXT("Flight/landing surface matches its real elevation"),FMath::Abs(Hit.ImpactPoint.Z-Expected)<.2);};
 Surface({68,130,0},20);Surface({68,256,0},100);Surface({68,447,0},180);Surface({232,256,0},260);Surface({232,130,0},340);
 C->Build({0,0},0,nullptr,nullptr,true);Parts.Reset();A->GetComponents(Parts);for(auto* P:Parts)TestEqual(TEXT("Hologram cannot block players"),P->GetCollisionEnabled(),ECollisionEnabled::NoCollision);W->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStraightOfficeStair,"Foundryline.Construction.OfficeStairs.StraightLayoutAndCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStraightOfficeStair::RunTest(const FString&){
 using L=FFoundrylineOfficeStairLayout;
 for(int Q=0;Q<4;++Q){const FIntPoint Origin=Q==0?FIntPoint(100,100):Q==1?FIntPoint(1000,100):Q==2?FIntPoint(1000,1000):FIntPoint(100,1000);const auto Hole=L::HoleBounds(Origin,Q,1),Full=L::FullBounds(Origin,Q,1);const auto Route=L::WalkRoute(Origin,Q,1);double Area=0;for(const auto& P:L::FloorPanels(1200,1200,Origin,Q,1)){Area+=P.GetArea();TestFalse(TEXT("Straight opening remains clear of upper slab"),P.Intersect(Hole.ExpandBy(-.1)));}TestTrue(TEXT("Straight slab has exact opening area"),FMath::Abs(Area+Hole.GetArea()-1440000)<.01);TestEqual(TEXT("Straight route has no lateral turn"),Route.Num(),5);for(const auto P:Route)TestTrue(TEXT("Straight route and landings fit full reserved footprint"),Full.IsInsideOrOn(FVector2D(P)));
  auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* A=W->SpawnActor<AActor>();auto* C=NewObject<UFoundrylineOfficeStairComponent>(A);A->SetRootComponent(C);C->RegisterComponent();A->AddInstanceComponent(C);C->Build(Origin,Q,nullptr,nullptr,false,nullptr,1);
  for(const auto At:TArray<FVector>{{68,130,20},{68,382,180},{68,634,340}}){const auto Point=L::Point(Origin,Q,At);FHitResult H;TestTrue(TEXT("Straight flight collision exists in every rotation"),W->LineTraceSingleByChannel(H,Point+FVector(0,0,100),Point-FVector(0,0,25),ECC_Visibility));TestTrue(TEXT("Straight collision matches real floor and rise"),FMath::Abs(H.ImpactPoint.Z-At.Z)<.2);}
  C->Build(Origin,Q,nullptr,nullptr,true,nullptr,1);TArray<UPrimitiveComponent*> Parts;A->GetComponents(Parts);for(auto* P:Parts)TestEqual(TEXT("Straight hologram never blocks the player"),P->GetCollisionEnabled(),ECollisionEnabled::NoCollision);W->DestroyWorld(false);
 }
 return true;
}
#endif
