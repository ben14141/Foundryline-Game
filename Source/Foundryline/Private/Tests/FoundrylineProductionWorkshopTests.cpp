#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineWorkshopTest,"Foundryline.Construction.ProductionWorkshopLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineWorkshopTest::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=World->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->bProductionWorkshop=true;Site->DispatchBeginPlay();
 auto* Placement=World->GetSubsystem<UFoundrylinePlacementSubsystem>();TestTrue(TEXT("Workshop property initializes"),Site->IsReady());
 int Shops=0,Offices=0,Bays=0;const FFoundrylinePlacedObject* Shop=nullptr;
 for(const auto& O:Placement->GetObjects()){
  const auto* D=Placement->FindDefinition(O.DefinitionId);if(!D)continue;
  if(D->FunctionId==TEXT("BuildingShell")){
   if(O.bOfficeAnnex){++Offices;TestTrue(TEXT("Office has a usable room"),O.OfficeRooms.Num()>0);int Doors=0;for(const auto& Opening:O.Openings)if(Opening.Kind==EFoundrylineOpeningKind::OfficeDoor)++Doors;TestTrue(TEXT("Office connects outside and into shop"),Doors>=2);}
   else{++Shops;Shop=&O;TestEqual(TEXT("Shop has a 52 metre testing frontage"),D->Footprint.Max.X,5200);TestEqual(TEXT("Shop has a 50 metre testing depth"),D->Footprint.Max.Y,5000);TestEqual(TEXT("Tall shop ceiling"),D->Footprint.Max.Z,1220);}
  }
  for(const auto& Opening:O.Openings)if(Opening.Kind==EFoundrylineOpeningKind::TruckGarage){++Bays;TestEqual(TEXT("Bays have five metre clearance"),Opening.HeightCm(),500);}
 }
 TestEqual(TEXT("One new shop"),Shops,1);TestEqual(TEXT("One attached office"),Offices,1);TestEqual(TEXT("Exactly two loading bays"),Bays,2);
 TestTrue(TEXT("Parts area fits shop without touching exterior walls"),Shop&&Shop->Anchor.X<-2200&&Shop->Anchor.Y<3300);
 bool PickupAligned=false,SecondAligned=false,ReceivingAligned=false;int ForkliftDoors=0,PersonalDoors=0;if(Shop){const auto Corridor=FFoundrylineWorkshopLayout::PickupClearance();const auto Steel=FFoundrylineWorkshopLayout::SteelArrival();for(const auto& Opening:Shop->Openings)if(Opening.Edge==1){const double Low=Shop->Anchor.Y+Opening.OffsetCm,High=Low+Opening.WidthCm();if(Opening.Kind==EFoundrylineOpeningKind::TruckGarage){PickupAligned|=Corridor.Min.Y>=Low&&Corridor.Max.Y<=High;SecondAligned|=4150>=Low&&4550<=High;}if(Opening.Kind==EFoundrylineOpeningKind::Garage){++ForkliftDoors;ReceivingAligned|=Steel.Y-152.4>=Low&&Steel.Y+152.4<=High;}if(Opening.Kind==EFoundrylineOpeningKind::Personnel)++PersonalDoors;}}
 TestTrue(TEXT("Second tanker fits its own loading door"),SecondAligned);TestEqual(TEXT("Loading wall has one smaller central forklift door"),ForkliftDoors,1);TestEqual(TEXT("Loading wall has one personnel door"),PersonalDoors,1);
 TestTrue(TEXT("Full tanker exit clearance fits the pickup door"),PickupAligned);TestTrue(TEXT("Steel sheet receiving aligns with the delivery door"),ReceivingAligned);
 FHitResult Hit;
 TestTrue(TEXT("Parts storage sits on the shop's real slab"),World->LineTraceSingleByChannel(Hit,{-1500,3700,40},{-1500,3700,0},ECC_Visibility));
 for(const auto& Rack:FFoundrylineWorkshopLayout::Racks())TestTrue(TEXT("Every machine rack stands on the actual shop slab"),World->LineTraceSingleByChannel(Hit,Rack.Center+FVector(0,0,20),Rack.Center-FVector(0,0,20),ECC_Visibility));
 auto* Plant=World->SpawnActor<AFoundrylineFactoryPlant>();Plant->DispatchBeginPlay();
 Plant->Tick(0);TestEqual(TEXT("Practice scene has two matching tanker zones"),Plant->Snapshot.Zones.Num(),2);for(const auto& Zone:Plant->Snapshot.Zones){const FVector Center=FFoundrylineWorkZoneLayout::WelderPosition(Zone)+FVector(0,0,80);const bool Blocked=World->LineTraceSingleByChannel(Hit,Center-FVector(200,0,0),Center,ECC_Visibility);TestTrue(TEXT("Every zone includes a physical standard welding machine"),Blocked&&Hit.Component.IsValid()&&Hit.Component->ComponentHasTag(TEXT("FoundrylineZoneWelder")));}
 TArray<UFoundrylineWeldingEffectComponent*> Effects;Plant->GetComponents(Effects);TestEqual(TEXT("Each work zone has its own gun-tip effect"),Effects.Num(),2);for(const auto* Effect:Effects)TestTrue(TEXT("Arc is attached at the actual gun tip beside its assigned product"),Plant->Snapshot.Zones.ContainsByPredicate([&](const auto& Z){return Effect->GetComponentLocation().Equals(FFoundrylineWorkZoneLayout::WeldPoint(Z),.1);}));
 for(const auto& Rack:FFoundrylineWorkshopLayout::Racks()){const FVector End=Rack.Center+FVector(0,0,130),Start=End+FRotator(0,Rack.Yaw,0).RotateVector(FVector(-200,0,0));const bool Blocked=World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility);TestTrue(TEXT("Physical rack blocks a player rather than acting as scenery"),Blocked&&Hit.GetActor()==Plant);}
 World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineForkParkingTest,"Foundryline.Production.ForkliftParkingClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineForkParkingTest::RunTest(const FString&){
 FFoundrylineFactorySimulation Model;Model.Seed(true);FString Error;while(Model.State.Drivers.Num()<4)if(!Model.AddDriver(1,Error))return false;
 for(int I=0;I<4;++I){const auto Bay=FFoundrylineWorkshopLayout::DriverParkingBounds(I);const auto Home=FFoundrylineWorkshopLayout::DriverHome(I);
  TestTrue(TEXT("Full parking stall remains on the shop slab"),FBox2D(Model.State.FloorMin,Model.State.FloorMax).IsInside(Bay.Min)&&FBox2D(Model.State.FloorMin,Model.State.FloorMax).IsInside(Bay.Max));
  for(int J=I+1;J<4;++J)TestFalse(TEXT("Parking stalls cannot overlap"),Bay.Intersect(FFoundrylineWorkshopLayout::DriverParkingBounds(J)));
  for(const auto& L:Model.State.Lanes)TestFalse(TEXT("Parking is clear of all travel lanes and pedestrian crossings"),Bay.Intersect(FBox2D(L.Min,L.Max)));
  for(const auto& Solid:Model.Solids)TestFalse(TEXT("Parking avoids machines and racks"),Bay.Intersect(Solid));
  for(const auto& Z:Model.State.Zones)TestFalse(TEXT("Parking avoids tanker work and supply zones"),Bay.Intersect(FFoundrylineWorkZoneLayout::Bounds(Z)));
  TestTrue(TEXT("Every parked forklift can reach the main aisle with four drivers present"),Model.Path(Home,FVector(-300,Home.Y,20)).Num()>1);
 }
 return true;
}
#endif
