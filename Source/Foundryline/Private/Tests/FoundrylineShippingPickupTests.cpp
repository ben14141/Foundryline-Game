#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylineLoadingBayLayout.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShippingPickupGeometryTest,"Foundryline.Commerce.ShippingPickup.RotationAndCommittedFreight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShippingPickupGeometryTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);
 TestTrue(TEXT("Legacy saves retain original staging coordinates"),M.ShippingPickupPosition(0).Equals(FFoundrylineFactorySimulation::PickupPosition(0)));
 M.State.bPresetPickup=true;M.State.PickupOpeningId=FGuid::NewGuid();M.State.PickupOrigin={100,1500,20};
 for(int Q=0;Q<4;++Q){M.State.PickupYaw=Q*90;const FVector Out=FRotator(0,Q*90,0).RotateVector(FVector::ForwardVector);
  TestTrue(TEXT("Two pallets stay 250cm apart at every loading-door orientation"),FMath::IsNearlyEqual(FVector::Dist2D(M.ShippingPickupPosition(0),M.ShippingPickupPosition(1)),250.,.001));
  for(int Slot=0;Slot<2;++Slot)TestTrue(TEXT("Forklift docks on the interior side of each pallet"),(M.ShippingPickupPosition(Slot)-M.ShippingPickupDock(Slot)).Equals(Out*136,.001));
 }
 TestTrue(TEXT("An idle loading bay may follow a door edit"),M.CanRelocateShippingPickup());
 FFoundrylineFactoryPallet Pallet;Pallet.Id=M.State.NextId++;Pallet.Place=EFoundrylinePalletPlace::Pickup;M.State.Pallets.Add(Pallet);TestFalse(TEXT("Staged cargo freezes the loading bay position"),M.CanRelocateShippingPickup());M.State.Pallets.Pop();
 FFoundrylineFactoryJob Job;Job.DestinationPlace=EFoundrylinePalletPlace::Pickup;M.State.Jobs.Add(Job);TestFalse(TEXT("A queued forklift delivery freezes the loading bay position"),M.CanRelocateShippingPickup());M.State.Jobs.Reset();
 FFoundrylineFreightBooking Booking;Booking.Status=EFoundrylineBookingState::Booked;M.State.Commerce.Bookings.Add(Booking);TestFalse(TEXT("A booked carrier freezes the loading bay position"),M.CanRelocateShippingPickup());M.State.Commerce.Bookings[0].Status=EFoundrylineBookingState::Delivered;TestTrue(TEXT("Finished freight releases the bay for relocation"),M.CanRelocateShippingPickup());return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShippingPickupOwnedDoorTest,"Foundryline.Commerce.ShippingPickup.FollowsActualOwnedLoadingDoor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShippingPickupOwnedDoorTest::RunTest(const FString&){
 auto* W=UWorld::CreateWorld(EWorldType::Game,false,TEXT("L_FirstShopStarter"));auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->bCutPartsStarter=true;Site->bFirstShopStarter=true;Site->DispatchBeginPlay();
 auto* Plant=W->SpawnActor<AFoundrylineFactoryPlant>();Plant->bCutPartsStarter=true;Plant->DispatchBeginPlay();
 TestTrue(TEXT("The starter's real preset loading door supplies the shipping positions"),Plant->Model.State.bPresetPickup);
 TestFalse(TEXT("Starter loading-door interior has clear floor"),Plant->Model.State.bPickupLayoutPending);
 bool Found=false;auto* Placement=W->GetSubsystem<UFoundrylinePlacementSubsystem>();for(const auto& Object:Placement->GetObjects())for(const auto& Opening:Object.Openings)if(Opening.OpeningId==Plant->Model.State.PickupOpeningId){
  Found=true;const auto* D=Placement->FindDefinition(Object.DefinitionId);const FTransform B(FRotator(0,Object.QuarterTurns*90,0),FVector(Object.Anchor));const FVector Wall=B.TransformPosition(FFoundrylineLoadingBayLayout::Point(*D,Opening,0,0,20));const FVector Outside=B.TransformPosition(FFoundrylineLoadingBayLayout::Point(*D,Opening,0,1,20))-Wall;
  const FVector Origin=Plant->GetActorTransform().TransformPosition(Plant->Model.State.PickupOrigin);
  TestTrue(TEXT("Preparation sits eight feet inside the actual door"),FMath::IsNearlyEqual(FVector::DotProduct(Origin-Wall,Outside),-243.84,.1));
  TestTrue(TEXT("Both real pallet docks remain reachable by the starter forklift"),!Plant->Model.Path(Plant->Model.State.Drivers[0].Parking,Plant->Model.ShippingPickupDock(0)).IsEmpty()&&!Plant->Model.Path(Plant->Model.State.Drivers[0].Parking,Plant->Model.ShippingPickupDock(1)).IsEmpty());
 }
 TestTrue(TEXT("Shipping origin references an owned opening record"),Found);W->DestroyWorld(false);return true;
}
#endif
