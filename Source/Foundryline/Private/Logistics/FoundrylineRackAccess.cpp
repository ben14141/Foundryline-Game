#include "Logistics/FoundrylineRackAccess.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Construction/FoundrylinePlannerItems.h"
#include "Construction/FoundrylinePlanningEquipment.h"
bool FFoundrylineRackAccess::FinishedSlot(const FFoundrylineFactoryState& S,int32 Id){return S.StorageRacks.ContainsByPredicate([&](const auto& R){return R.FinishedGoods&&R.Slots.Contains(Id);});}
bool FFoundrylineRackAccess::Ready(const FFoundrylineFactoryState& S,const FFoundrylineFactoryPallet& P){return P.Place==EFoundrylinePalletPlace::Rack&&FinishedSlot(S,P.Owner);}
TArray<FVector> FFoundrylineRackAccess::Docks(const FFoundrylineFactoryState& S,int32 Id){
 const auto* Slot=S.Slots.FindByPredicate([&](const auto& V){return V.Id==Id;});if(!Slot)return {};
 TArray<FVector> Out={Slot->Dock};bool Rack=S.StorageRacks.ContainsByPredicate([&](const auto& R){return R.Slots.Contains(Id);});
 if(!Rack)Rack=FFoundrylinePlannerItems::FixedRacks(S).ContainsByPredicate([&](const auto& R){return R.Slots.Contains(Id);});
 if(Rack){auto Other=Slot->Position*2-Slot->Dock;Other.Z=Slot->Dock.Z;Out.AddUnique(Other);}return Out;
}
bool FFoundrylineRackAccess::Route(const FFoundrylineFactorySimulation& M,FFoundrylineFactoryJob& J,FVector From,TArray<FVector>& Path){
 const auto* Source=M.State.Pallets.FindByPredicate([&](const auto& P){return P.Id==J.SourcePallet;});
 auto Pick=Source&&Source->Place==EFoundrylinePalletPlace::Rack?Docks(M.State,Source->Owner):TArray<FVector>{J.Pickup};
 auto Drop=J.DestinationPlace==EFoundrylinePalletPlace::Rack?Docks(M.State,J.Destination):TArray<FVector>{J.Drop};
 auto Length=[](const TArray<FVector>& P){double L=0;for(int I=1;I<P.Num();++I)L+=FVector::Dist(P[I-1],P[I]);return L;};
 double Best=MAX_dbl;FVector Pickup=J.Pickup,Destination=J.Drop;
 for(auto A:Pick){auto First=M.Path(From,A);if(First.IsEmpty())continue;for(auto B:Drop){auto Second=M.Path(A,B);if(Second.IsEmpty())continue;const double Cost=Length(First)+Length(Second);if(Cost<Best){Best=Cost;Path=First;Pickup=A;Destination=B;}}}
 if(Best==MAX_dbl)return false;J.Pickup=Pickup;J.Drop=Destination;return true;
}
FBox2D FFoundrylineRackAccess::Service(const FFoundrylinePlannerItem& I,int Side){
 const FRotator R(0,I.Quarter*90,0);
 if(I.Kind==EFoundrylinePlannerItem::FixedRack&&!I.CompactRack)return FFoundrylinePlanningEquipment::Bounds(I.Center+R.RotateVector({Side*242.88,0,0}),I.Quarter,{365.76,416,0});
 return FFoundrylinePlanningEquipment::Bounds(I.Center+R.RotateVector({0,Side*(75+182.88),0}),I.Quarter,{I.Kind==EFoundrylinePlannerItem::StorageRack?FMath::Max(I.Bounds.GetSize().X,I.Bounds.GetSize().Y):310,365.76,0});
}
bool FFoundrylineRackAccess::Clear(const FFoundrylineFactorySimulation& M,const FBox2D& B,const FBox2D& Extra){
 const FBox2D Floor(M.State.FloorMin,M.State.FloorMax);if(!Floor.IsInsideOrOn(B.Min)||!Floor.IsInsideOrOn(B.Max))return false;
 const auto Inner=B.ExpandBy(-.5);if(Extra.bIsValid&&Inner.Intersect(Extra))return false;
 for(const auto& S:M.Solids)if(Inner.Intersect(S))return false;
 if(M.OverlapsWeldingStation(Inner,false))return false;
 for(const auto& Z:M.State.Zones)if(FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)).Intersect(Inner))return false;
 for(const auto& D:M.State.Drivers)for(auto At:{D.Position,D.Parking})if(FFoundrylinePlanningEquipment::Bounds(At,0,{360,180,0}).Intersect(Inner))return false;
 return true;
}
