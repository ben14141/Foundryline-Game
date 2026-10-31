#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Inventory/FoundrylineSheetStock.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentPartsCatalog,"Foundryline.Commerce.CurrentParts.HeadacheRackOnly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCurrentPartsCatalog::RunTest(const FString&){
 using P=EFoundrylinePart;using C=FFoundrylineFactoryCatalog;FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString E;
 int Count=0;for(int I=0;I<C::PartCount;++I)Count+=C::AvailableNow(P(I));TestEqual(TEXT("Seven active IDs including material and blank intermediate"),Count,7);
 TestEqual(TEXT("Plasma offers just bracket blanks and steel plates"),M.State.Programs.Num(),2);
 for(const auto& O:M.State.Commerce.Orders)TestTrue(TEXT("Every generated order is an active product"),C::AvailableNow(P(O.Part)));
 TestFalse(TEXT("Disabled gusset cannot be programmed"),M.Program(1,P::GussetPlate,1,false,E));TestFalse(TEXT("Disabled reinforcement cannot be programmed"),M.Program(1,P::ReinforcementPlate,1,false,E));
 TestFalse(TEXT("Retired parts cannot be sold"),FFoundrylineCommerceCatalog::Saleable(uint8(P::SupportChannel)));
 M.State.Commerce.CashCents=1000000;TestTrue(TEXT("Press brake upgrade still installs"),M.InstallBrake(E));TestEqual(TEXT("Upgrade adds only formed brackets"),M.State.Programs.Num(),3);
 TestTrue(TEXT("Formed output explicitly names its process"),FString(C::Part(P::BentBracket).Name).StartsWith(TEXT("Formed")));
 FFoundrylineAssemblyRecipe R;C::Recipe(1,R);TestEqual(TEXT("Headache rack requires four formed brackets"),R.Bill[int(P::BentBracket)],4);TestEqual(TEXT("Raw blanks cannot satisfy assembly bill"),R.Bill[int(P::Bracket)],0);
 TestTrue(TEXT("Light kit has dedicated identifiable model"),C::PartMesh(P::LightKit).Contains(TEXT("LightKit/SM_LightKitBox")));
 TestTrue(TEXT("Current business validates"),M.Valid(E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPalletCargoWeights,"Foundryline.Commerce.CurrentParts.PalletCargoWeights",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPalletCargoWeights::RunTest(const FString&){using P=EFoundrylinePart;using C=FFoundrylineFactoryCatalog;
 for(auto Part:{P::Sheet,P::Bracket,P::BentBracket,P::MountingPlate,P::FrameSection,P::LightKit,P::HeadacheRack})TestTrue(TEXT("Every current item has a positive default weight"),C::Part(Part).UnitLb>0);
 TestTrue(TEXT("Sheet weight is derived from the actual stock dimensions"),FMath::IsNearlyEqual(double(C::Part(P::Sheet).UnitLb),FFoundrylineSteelSheet::PoundsEach(),.01));
 TestTrue(TEXT("Exactly 1500 lb fits a standard pallet"),C::FitsPalletWeight(P::Bracket,300,5));TestFalse(TEXT("Over 1500 lb is rejected"),C::FitsPalletWeight(P::Bracket,301,5));
 TestEqual(TEXT("Oversized sheet stock uses two supporting pallets"),C::PalletCargoLimitLb(P::Sheet),3000.);TestTrue(TEXT("Standard ten-sheet bundle fits the two pallets"),C::FitsPalletWeight(P::Sheet,10,C::Part(P::Sheet).UnitLb));TestFalse(TEXT("Overloaded sheet bundle is rejected"),C::FitsPalletWeight(P::Sheet,12,C::Part(P::Sheet).UnitLb));
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P::LightKit;A.Quantity=2;A.UnitLb=C::Part(A.Part).UnitLb;A.Place=EFoundrylinePalletPlace::Receiving;A.Owner=10;A.Position={1000,1600,20};M.State.Pallets.Add(A);FString E;TestTrue(TEXT("Normally weighted stock saves"),M.Valid(E));
 auto Bad=M;Bad.State.Pallets.Last().UnitLb=1000;TestFalse(TEXT("Overloaded save is rejected"),Bad.Valid(E));
 return true;
}
#endif
