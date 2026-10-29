#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Engine/StaticMesh.h"
#include "Production/FoundrylineMachineCellLayout.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkshopAccessTest,"Foundryline.Factory.OrganizedWorkshopAccess",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorkshopAccessTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed();FString Error;TestTrue(TEXT("Organized workshop validates"),M.Valid(Error));TestEqual(TEXT("Input and output rack for each of three machines plus kit rack"),FFoundrylineWorkshopLayout::Racks().Num(),7);
 TestTrue(TEXT("Usable floor is more than twice the old shop"),(M.State.FloorMax-M.State.FloorMin).X*(M.State.FloorMax-M.State.FloorMin).Y>2*3540*2940);
 const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(M.State.Zones[0]);for(const auto& Solid:M.Solids)TestFalse(TEXT("Spacious assembly and attached supplies clear all fixed equipment"),Bounds.Intersect(Solid));
 TestTrue(TEXT("Tanker zone connects directly to the loading-wall threshold"),FMath::IsNearlyEqual(Bounds.Max.X,M.State.FloorMax.X,.01));TestTrue(TEXT("Tanker lines up with the centre of the pickup door"),FMath::IsNearlyEqual(FFoundrylineWorkZoneLayout::ProductCenter(M.State.Zones[0]).Y,1650.,.01));
 TestTrue(TEXT("Resized tanker preset is sixty by thirty feet including its supplies"),Bounds.GetSize().Equals(FVector2D(60,30)*30.48,.01));TestTrue(TEXT("Resizing the bay preserves the actual tanker position"),FFoundrylineWorkZoneLayout::ProductCenter(M.State.Zones[0]).Equals(FVector(2055,1650,20),.01));
 const FVector Welder=FFoundrylineWorkZoneLayout::WorkPoint(M.State.Zones[0]);TestTrue(TEXT("Welder fits inside the compact zone and outside the tank collision"),Bounds.IsInsideOrOn(FVector2D(Welder)-FVector2D(35))&&Bounds.IsInsideOrOn(FVector2D(Welder)+FVector2D(35))&&FMath::Abs(Welder.Y-1650)>165);
 FBox ProductBounds(ForceInit);const FVector ProductOrigin=FFoundrylineWorkZoneLayout::ProductCenter(M.State.Zones[0])+FVector(404,0,0);for(const TCHAR* Name:{TEXT("ProductFrame"),TEXT("ProductShell"),TEXT("ProductFittings")}){auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Products/Tankers/StraightBarrel/ProductionStages/SM_%s.SM_%s"),Name,Name));TestNotNull(TEXT("Product stage mesh exists"),Mesh);if(Mesh)ProductBounds+=Mesh->GetBoundingBox().ShiftBy(ProductOrigin);}TestTrue(TEXT("Actual tanker front is directly inside the loading door without entering the wall"),ProductBounds.IsValid&&ProductBounds.Max.X<M.State.FloorMax.X&&ProductBounds.Max.X>M.State.FloorMax.X-300);TestTrue(TEXT("Actual tanker stays within its work zone"),ProductBounds.Min.X>=Bounds.Min.X&&ProductBounds.Min.Y>=Bounds.Min.Y&&ProductBounds.Max.Y<=Bounds.Max.Y);
 for(const auto& Solid:M.Solids)TestFalse(TEXT("Four metre tanker exit corridor stays clear of all machines and racks"),FFoundrylineWorkshopLayout::PickupClearance().Intersect(Solid));
 const FVector Home=M.State.Drivers[0].Position;
 for(int Id=1;Id<=3;++Id){const auto Cell=FFoundrylineMachineCellLayout::Bounds(Id);const FVector2D Output(FFoundrylineWorkshopLayout::OutputStock(Id));TestTrue(TEXT("Machine output pallet pad is inside its machine cell"),Cell.IsInsideOrOn(Output-FVector2D(100,180))&&Cell.IsInsideOrOn(Output+FVector2D(100,180)));for(FVector Center:{FFoundrylineWorkshopLayout::InputRack(Id),FFoundrylineWorkshopLayout::OutputRack(Id)})TestTrue(TEXT("Machine input/output rack footprint is inside its cell"),Cell.IsInsideOrOn(FVector2D(Center)-FVector2D(75,220))&&Cell.IsInsideOrOn(FVector2D(Center)+FVector2D(75,220)));}
 for(const auto& Slot:M.State.Slots)TestTrue(*FString::Printf(TEXT("Forklift reaches rack slot %d without crossing a rack or machine"),Slot.Id),!M.Path(Home,Slot.Dock).IsEmpty());
 for(const auto& Machine:M.State.Machines){TestTrue(TEXT("Machine feed rack reachable"),!M.Path(Home,Machine.InputDock).IsEmpty());TestTrue(TEXT("Machine output pickup reachable"),!M.Path(Home,Machine.OutputDock).IsEmpty());}
 for(auto P:{EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings})TestTrue(TEXT("Attached product supply bay reachable"),!M.Path(Home,M.BufferPosition(M.State.Zones[0],P)+FVector(0,136,0)).IsEmpty());
 TestTrue(TEXT("Welder has personnel access to product"),!M.Path(M.State.Walkers[0].Position,FFoundrylineWorkZoneLayout::WorkPoint(M.State.Zones[0]),true).IsEmpty());
 for(int I=0;I<3;++I)M.AddDriver(1,Error);M.State.Drivers[0].Position={-430.735,700,20};M.State.Drivers[1].Position={-450,1191,20};M.State.Drivers[2].Position={-436,1700,20};M.State.Drivers[3].Position={-450,2181,20};TestTrue(TEXT("Output dock remains reachable when a parked forklift occupies the closest lane node"),!M.Path(M.State.Machines[0].OutputDock,M.State.Slots[7].Dock).IsEmpty());
 TestFalse(TEXT("Dedicated machine rack recipe assignment cannot be changed"),M.AssignSlot(107,EFoundrylinePart::Sheet,Error));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkshopRackFlowTest,"Foundryline.Factory.MachineRackProductionFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorkshopRackFlowTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed();FString Error;const float Initial=M.SteelMass();M.Order(M.State.Zones[0].Id,Error);M.State.Speed=8;TSet<int> Used;bool Fed[3]={false,false,false};
 for(int I=0;I<30000&&M.State.Zones[0].CompletedMask!=63;++I){
  if(M.Total(EFoundrylinePart::Sheet)<5&&M.Available(EFoundrylinePart::Sheet,EFoundrylinePalletPlace::Receiving,10)==0)M.ReceiveSteel(10,Error);M.Step(.1f);
  for(const auto& Pallet:M.State.Pallets){if(Pallet.Place==EFoundrylinePalletPlace::Rack&&Pallet.Quantity>0)Used.Add(Pallet.Owner);if(Pallet.Place==EFoundrylinePalletPlace::MachineInput&&Pallet.Quantity>0&&Pallet.Owner>=1&&Pallet.Owner<=3)Fed[Pallet.Owner-1]=true;}
  for(const auto& Machine:M.State.Machines)if(Machine.Remaining>0)Fed[Machine.Id-1]=true;
 }
 for(int Id=100;Id<=112;++Id)TestTrue(*FString::Printf(TEXT("Real stock used assigned rack %d"),Id),Used.Contains(Id));
 for(bool F:Fed)TestTrue(TEXT("Each machine consumed stock from its own feed rack"),F);
 TestEqual(TEXT("Racked cutting, bending and rolling flow completes the tanker"),M.State.Zones[0].CompletedMask,63);TestTrue(TEXT("No stock or steel mass lost through rack transfers"),FMath::Abs(M.SteelMass()-Initial-M.State.ReceivedSteelLb)<2);TestTrue(TEXT("Final reservations and ownership validate"),M.Valid(Error));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStockedWorkshopTest,"Foundryline.Factory.StockedPracticeInventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStockedWorkshopTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed(true);FString Error;
 TestTrue(TEXT("Stocked starter validates"),M.Valid(Error));TestFalse(TEXT("Practice stock starts paused"),M.State.Running);
 for(const auto& S:M.State.Slots){const auto* P=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Place==EFoundrylinePalletPlace::Rack&&A.Owner==S.Id;});TestTrue(TEXT("Assigned shelf has actual stock at the shelf position"),P&&P->Quantity>0&&P->Position.Equals(S.Position));}
 const auto& Z=M.State.Zones[0];
 for(auto Part:{EFoundrylinePart::FrameRail,EFoundrylinePart::BentBracket,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings}){const auto* P=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Place==EFoundrylinePalletPlace::Buffer&&A.Owner==Z.Id&&A.Part==Part;});TestTrue(TEXT("Parts sit in their own linked supply bay"),P&&P->Position.Equals(M.BufferPosition(Z,Part)));}
 for(const auto& Machine:M.State.Machines){const auto* P=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Place==EFoundrylinePalletPlace::MachineOutput&&A.Owner==Machine.Id;});TestTrue(TEXT("Output pallet has real parts at its marked pickup"),P&&P->Position.Equals(Machine.OutputStock));}
 const float Initial=M.SteelMass();M.Order(Z.Id,Error);M.State.Speed=8;
 for(int I=0;I<24000&&M.State.Zones[0].CompletedMask!=63;++I){if(M.Total(EFoundrylinePart::Sheet)<5&&!M.Available(EFoundrylinePart::Sheet,EFoundrylinePalletPlace::Receiving,10))M.ReceiveSteel(10,Error);M.Step(.1f);}
 TestEqual(TEXT("Starter parts participate in a complete production run"),M.State.Zones[0].CompletedMask,63);TestTrue(TEXT("Starter stock is consumed without losing or duplicating steel"),FMath::Abs(M.SteelMass()-Initial-M.State.ReceivedSteelLb)<2);TestTrue(TEXT("Finished stocked run validates"),M.Valid(Error));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTwinWorkshopTest,"Foundryline.Factory.TwinTankerZones",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTwinWorkshopTest::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.Seed(true);FString Error;TestEqual(TEXT("Two matching starter work zones"),M.State.Zones.Num(),2);if(M.State.Zones.Num()!=2)return false;
 TestFalse(TEXT("Bay footprints remain separate"),FFoundrylineWorkZoneLayout::Bounds(M.State.Zones[0]).Intersect(FFoundrylineWorkZoneLayout::Bounds(M.State.Zones[1])));
 for(int I=0;I<2;++I){const auto& Z=M.State.Zones[I];const auto Bounds=FFoundrylineWorkZoneLayout::Bounds(Z);TestTrue(TEXT("Both tanker bays keep the sixty by thirty foot footprint"),Bounds.GetSize().Equals(FVector2D(60,30)*30.48,.01));TestTrue(TEXT("Each tanker is centred on its own door"),FMath::IsNearlyEqual(FFoundrylineWorkZoneLayout::ProductCenter(Z).Y,I?4350.:1650.,.01));for(const auto& Solid:M.Solids)TestFalse(TEXT("Storage and machines clear both tanker bays"),Bounds.Intersect(Solid));TestTrue(TEXT("Welder reaches either bay"),!M.Path(M.State.Walkers[I].Position,FFoundrylineWorkZoneLayout::WorkPoint(Z),true).IsEmpty());for(auto Part:{EFoundrylinePart::FrameRail,EFoundrylinePart::BentBracket,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings}){const auto Pos=M.BufferPosition(Z,Part);TestTrue(TEXT("Each bay owns its staged parts"),M.Available(Part,EFoundrylinePalletPlace::Buffer,Z.Id)>0);TestTrue(TEXT("Forklift reaches every mirrored drop point"),!M.Path(M.State.Drivers[0].Position,Pos+FFoundrylineWorkZoneLayout::Rotation(Z).RotateVector(FVector(0,Z.SupplySide*136,0))).IsEmpty());}}
 const float Initial=M.SteelMass();M.Order(M.State.Zones[0].Id,Error);M.Order(M.State.Zones[1].Id,Error);M.State.Speed=8;
 for(int I=0;I<48000&&(M.State.Zones[0].CompletedMask!=63||M.State.Zones[1].CompletedMask!=63);++I){if(M.Total(EFoundrylinePart::Sheet)<5&&!M.Available(EFoundrylinePart::Sheet,EFoundrylinePalletPlace::Receiving,10))M.ReceiveSteel(10,Error);if(M.Total(EFoundrylinePart::EndCap)<2)M.ReceiveKits(Error);M.Step(.1f);}
 TestEqual(TEXT("First bay completes its tanker"),M.State.Zones[0].CompletedMask,63);TestEqual(TEXT("Second bay completes its tanker"),M.State.Zones[1].CompletedMask,63);TestTrue(TEXT("Two products conserve their actual steel inventory"),FMath::Abs(M.SteelMass()-Initial-M.State.ReceivedSteelLb)<2);TestTrue(TEXT("Twin-bay reservations and stock remain valid"),M.Valid(Error));return true;
}
#endif
