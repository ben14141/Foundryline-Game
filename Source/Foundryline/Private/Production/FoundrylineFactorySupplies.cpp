#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
bool FFoundrylineFactorySimulation::ReceiveKits(FString& E){if(State.Commerce.Enabled){E=TEXT("Purchased tanker kits are unavailable in the cut-parts starter");return false;}
 if(State.Pallets.Num()>61||State.Pallets.ContainsByPredicate([](const auto& P){return P.Place==EFoundrylinePalletPlace::Receiving&&P.Owner==11||P.Part==EFoundrylinePart::EndCap&&P.Place==EFoundrylinePalletPlace::Rack;})){E=TEXT("Use stored heads and clear purchased-parts receiving first");return false;}
 AddPallet(EFoundrylinePart::EndCap,2,EFoundrylinePalletPlace::Receiving,11,FFoundrylineWorkshopLayout::PurchasedArrival(EFoundrylinePart::EndCap),270);
 AddPallet(EFoundrylinePart::RunningGear,1,EFoundrylinePalletPlace::Receiving,11,FFoundrylineWorkshopLayout::PurchasedArrival(EFoundrylinePart::RunningGear));
 AddPallet(EFoundrylinePart::Fittings,1,EFoundrylinePalletPlace::Receiving,11,FFoundrylineWorkshopLayout::PurchasedArrival(EFoundrylinePart::Fittings));
 State.ReceivedSteelLb+=540;++State.Revision;return true;
}
