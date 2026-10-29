#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"
#include "Engine/StaticMesh.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineRackPalletsSeated,"Foundryline.Inventory.PalletsRestOnShelfBeams",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineRackPalletsSeated::RunTest(const FString&){
 using Layout=FFoundrylineWorkshopLayout;
 // The shelf art: beams carry the pallet below the thin edge lips; the pallet's underside is its origin.
 if(auto* Shelf=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Shelf400.SM_Rack_Shelf400"))){const FBox B=Shelf->GetBoundingBox();
  TestTrue(TEXT("Shelf origin is its underside and its lips stand above the beam tops"),FMath::IsNearlyZero(B.Min.Z,.01)&&B.Max.Z>Layout::ShelfBearingCm);}
 else AddWarning(TEXT("Shelf art not in this checkout; layout checks only"));
 if(auto* Pallet=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood.SM_Pallet_Wood")))TestTrue(TEXT("Pallet origin is its underside"),FMath::IsNearlyZero(Pallet->GetBoundingBox().Min.Z,.01));
 for(int L=0;L<3;++L)TestEqual(TEXT("Each shelf's beam tops sit at its slot height"),Layout::ShelfOrigin(L)+Layout::ShelfBearingCm,Layout::SlotHeight(L));
 // Every rack slot in both shops rests on a shelf of the rack it stands in, or on the floor.
 auto Check=[&](const TArray<FFoundrylineWorkshopRack>& Racks,const TArray<FFoundrylineFactorySlot>& Slots,const TCHAR* Shop){
  for(const auto& S:Slots){const auto* Rack=Racks.FindByPredicate([&](const auto& R){return FVector::Dist2D(R.Center,S.Position)<1;});
   if(!Rack){TestTrue(FString::Printf(TEXT("%s slot %d off the racks stands on the floor"),Shop,S.Id),FMath::IsNearlyEqual(S.Position.Z,20,.01));continue;}
   bool OnBeam=false;for(int L=0;L<3;++L)OnBeam|=FMath::IsNearlyEqual(S.Position.Z,Rack->Center.Z+Layout::ShelfOrigin(L)+Layout::ShelfBearingCm,.01);
   TestTrue(FString::Printf(TEXT("%s slot %d pallet underside meets a shelf's beam tops"),Shop,S.Id),OnBeam);}};
 Check(Layout::Racks(),Layout::Slots(),TEXT("Workshop"));
 FFoundrylineFactorySimulation Cut;Cut.SeedCutPartsShop();Check(Layout::CutShopRacks(),Cut.State.Slots,TEXT("Cut-parts shop"));
 // The press-brake supply rack (bought in the cut-parts shop): shelves at 20 cm + ShelfOrigin, slots 110/111 at 190/320 cm.
 for(int L=1;L<3;++L)TestEqual(TEXT("Press-brake supply rack slot meets its shelf beams"),20+Layout::ShelfOrigin(L)+Layout::ShelfBearingCm,190.+(L-1)*130);
 // Material-handling cell rack: sheet pallets stand at the rack position; the shelf beam tops are placed there.
 for(int L=0;L<3;++L)TestEqual(TEXT("Cell rack pallet height is a whole shelf step"),AFoundrylineMaterialHandlingCell::RackPosition(L).Z-AFoundrylineMaterialHandlingCell::RackPosition(0).Z,L*130.);
 return true;
}
#endif
