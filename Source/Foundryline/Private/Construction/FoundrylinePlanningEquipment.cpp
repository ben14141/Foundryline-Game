#include "Construction/FoundrylinePlanningEquipment.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Economy/FoundrylineEconomyTuning.h"

int64 FFoundrylinePlanningEquipment::Price(int32 Item,int32 Levels){
 using T=FFoundrylineEconomyTuning;const int32 Extra=FMath::Clamp(Levels,1,3)-1;
 return Item==0?T::FabStationCents:Item==1?T::PartsRackBaseCents+Extra*T::PartsRackExtraLevelCents:Item==2?T::FinishedRackBaseCents+Extra*T::FinishedRackExtraLevelCents:Item==3?T::ForkliftCents:MAX_int64;
}
FVector FFoundrylinePlanningEquipment::Size(int32 Item,int32 Levels){return Item==0?FVector(594.36,396.24,304.8):Item==3?FVector(450,190,300):FVector(Item==2?470:310,150,152.4*Levels);}
FString FFoundrylinePlanningEquipment::Mesh(int32 Item,int32 Levels){
 if(Item==0)return TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_StandardWeldingBench");
 if(Item==3)return TEXT("/Game/Foundryline/Vehicles/Forklift/FL01/SM_Forklift_Body");
 return FString(Item==2?TEXT("/Game/Foundryline/Inventory/Storage/WideBayPalletRacking/SM_PalletRack_"):TEXT("/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_"))+(Levels==1?TEXT("1Level"):FString::FromInt(Levels)+TEXT("Levels"));
}
FBox2D FFoundrylinePlanningEquipment::Bounds(FVector Center,int32 Q,FVector Size){FBox2D B(ForceInit);const FRotator R(0,Q*90,0);for(float X:{-.5f,.5f})for(float Y:{-.5f,.5f})B+=FVector2D(Center+R.RotateVector(FVector(Size.X*X,Size.Y*Y,0)));return B;}
FBox2D FFoundrylinePlanningEquipment::Service(FVector Center,int32 Q,float Width){const FRotator R(0,Q*90,0);return Bounds(Center+R.RotateVector(FVector(0,-75-182.88,0)),Q,FVector(Width,365.76,0));}

bool FFoundrylineFactorySimulation::CanBuyPlanningEquipment(int32 Item,FVector At,int32 Q,int32 Levels,FString& E,bool Swap)const{
 if(Item<0||Item>3||Q<0||Q>3||Levels<1||Levels>3||At.ContainsNaN()||FMath::Abs(At.Z-20)>1){E=TEXT("Choose a valid equipment position, rotation and rack height");return false;}
 if(State.Running){E=TEXT("Pause production before purchasing and placing equipment");return false;}
 if(!State.Commerce.Enabled||State.Commerce.UnpaidCents||State.Commerce.CashCents<FFoundrylinePlanningEquipment::Price(Item,Levels)){E=TEXT("Not enough available business cash for this purchase");return false;}
 if(Item==0){FFoundrylineWeldingStationRecord S;S.Center=At;S.QuarterTurns=Q;S.bSwapSides=Swap;return CanPlaceWeldingStation(S,E);}
 if(Item==3&&State.Drivers.Num()>=4||Item!=3&&(State.StorageRacks.Num()>=8||State.Slots.Num()+Levels*2>64)){E=TEXT("Equipment or storage capacity reached");return false;}
 const auto B=FFoundrylinePlanningEquipment::Bounds(At,Q,FFoundrylinePlanningEquipment::Size(Item,Levels));const FBox2D Floor(State.FloorMin,State.FloorMax);
 if(!Floor.IsInsideOrOn(B.Min)||!Floor.IsInsideOrOn(B.Max)){E=TEXT("The entire equipment footprint must fit inside your shop");return false;}
 const auto Access=Item==3?B:FFoundrylinePlanningEquipment::Service(At,Q,FFoundrylinePlanningEquipment::Size(Item,Levels).X);
 if(!Floor.IsInsideOrOn(Access.Min)||!Floor.IsInsideOrOn(Access.Max)){E=TEXT("Keep the rack's 12 ft loading aisle inside your shop");return false;}
 auto SolidBlocks=[&](const FBox2D& Other){return Other.Intersect(B)||(Item!=3&&Other.Intersect(Access));};
 for(const auto& S:Solids)if(SolidBlocks(S)){E=TEXT("Equipment or rack loading access is blocked");return false;}
 if(OverlapsWeldingStation(B,true)||Item!=3&&OverlapsWeldingStation(Access,false)){E=TEXT("Leave the fabrication station and its access clear");return false;}
 for(const auto& Z:State.Zones)if(FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)).Intersect(B)||Item!=3&&FBox2D(FVector2D(Z.Min),FVector2D(Z.Max)).Intersect(Access)){E=TEXT("Equipment overlaps a work zone");return false;}
 for(const auto& Rack:State.StorageRacks){if(FFoundrylinePlanningEquipment::Service(Rack.Center,Rack.QuarterTurns,Rack.FinishedGoods?470:310).Intersect(B)){E=TEXT("Equipment blocks an existing rack's loading aisle");return false;}}
 for(const auto& D:State.Drivers)if(SolidBlocks(FFoundrylinePlanningEquipment::Bounds(D.Position,0,FVector(360,180,0)))||SolidBlocks(FFoundrylinePlanningEquipment::Bounds(D.Parking,0,FVector(360,180,0)))){E=TEXT("A forklift or parking position is in the way");return false;}
 for(const auto& L:State.Lanes)if(FBox2D(L.Min,L.Max).Intersect(B)){E=TEXT("Equipment cannot cover a painted lane");return false;}
 return true;
}

bool FFoundrylineFactorySimulation::BuyPlanningEquipment(int32 Item,FVector At,int32 Q,int32 Levels,FString& E,bool Swap){
 if(!CanBuyPlanningEquipment(Item,At,Q,Levels,E,Swap))return false;
 auto Backup=State;const auto OldSolids=Solids;
 if(Item==0){if(!AddWeldingStation(At,Q,Swap,E))return false;}
 else if(Item==3){if(State.Routes.IsEmpty()){E=TEXT("Create a forklift route before buying a vehicle");return false;}if(!AddDriver(State.Routes[0].Id,E,false)){State=Backup;return false;}auto& D=State.Drivers.Last();D.Parking=D.Position=At+FRotator(0,Q*90,0).RotateVector(FVector(65,0,0));D.Yaw=D.ParkingYaw=Q*90;}
 else{
  FFoundrylinePlacedStorageRack R;R.Id=State.NextId++;R.Center=At;R.QuarterTurns=Q;R.Levels=Levels;R.FinishedGoods=Item==2;const FRotator Rotation(0,Q*90,0);
  for(int32 Level=0;Level<Levels;++Level)for(float Side:{-1.f,1.f}){FFoundrylineFactorySlot S;S.Id=State.NextId++;S.Part=EFoundrylinePart::Bracket;S.Position=At+Rotation.RotateVector(FVector(Side*(Item==2?110:70),0,152.4*Level));S.Dock=At+Rotation.RotateVector(FVector(Side*(Item==2?110:70),-211,0));State.Slots.Add(S);R.Slots.Add(S.Id);for(auto& Route:State.Routes){Route.Sources.AddUnique(S.Id);Route.Destinations.AddUnique(S.Id);}}
  State.StorageRacks.Add(R);Solids=FFoundrylineWorkshopLayout::PlantSolids(State);
 }
 CommerceExpense(FFoundrylinePlanningEquipment::Price(Item,Levels),Item==0?TEXT("Fabrication station purchase"):Item==3?TEXT("Forklift purchase"):TEXT("Storage rack purchase"));
 if(!Valid(E)){State=MoveTemp(Backup);Solids=OldSolids;return false;}++State.Revision;return true;
}
