#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Economy/FoundrylineEconomyTuning.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketPriceAgreementLock,"Foundryline.Commerce.MarketPricing.AcceptedTermsSurviveBalanceRefresh",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarketPriceAgreementLock::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop();FString Error;auto& Old=M.State.Commerce.Orders[0];const int Id=Old.Id;Old.UnitPriceCents=1234;
 FFoundrylineCommerceCommand Accept;Accept.Action=EFoundrylineCommerceAction::AcceptOrder;Accept.Id=Id;
 if(!TestTrue(TEXT("Accept the quoted historical price"),M.CommerceCommand(Accept,Error)))return false;
 for(auto& Offer:M.State.Commerce.Orders)if(Offer.Status==EFoundrylineOrderState::Offered)Offer.UnitPriceCents=1;
 M.RefreshCommerceOffers();
 TestEqual(TEXT("Signed contract retains its price"),M.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;})->UnitPriceCents,int64(1234));
 for(const auto& Offer:M.State.Commerce.Orders)if(Offer.Status==EFoundrylineOrderState::Offered){const int Modifier=Offer.Rush?140:Offer.Payment==EFoundrylinePayment::Upfront?90:100;TestEqual(TEXT("Unsigned market offer uses current price and its original terms"),Offer.UnitPriceCents,FFoundrylineCommerceCatalog::SalePrice(Offer.Part)*Modifier/100);}
 auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TArray<uint8> Bytes;
 if(!TestTrue(TEXT("Serialize signed contract"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
 auto* Restored=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Restore signed contract"),Restored))return false;
 FFoundrylineFactorySimulation Loaded;Loaded.State=Restored->Factory;Loaded.MigrateCurrentParts();
 TestEqual(TEXT("Save migration retains the signed agreement"),Loaded.State.Commerce.Orders.FindByPredicate([&](const auto& O){return O.Id==Id;})->UnitPriceCents,int64(1234));return true;
}
#endif
