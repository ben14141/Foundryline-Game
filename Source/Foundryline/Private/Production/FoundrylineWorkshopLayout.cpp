#include "Production/FoundrylineWorkshopLayout.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Construction/FoundrylinePlannerItems.h"

TArray<FFoundrylineWorkshopRack> FFoundrylineWorkshopLayout::Racks(){
 TArray<FFoundrylineWorkshopRack> Out;
 const TCHAR* Names[]={TEXT("PLASMA"),TEXT("PRESS BRAKE"),TEXT("SHEET ROLLER")};
 for(int Id=1;Id<=3;++Id){Out.Add({InputRack(Id),0,FString(Names[Id-1])+TEXT(" / INPUT")});Out.Add({OutputRack(Id),0,FString(Names[Id-1])+TEXT(" / OUTPUT")});}
 Out.Add({FVector(1600,3000,20),90,TEXT("PURCHASED KITS")});return Out;
}
TArray<FBox2D> FFoundrylineWorkshopLayout::Solids(){
 TArray<FBox2D> Out;const FVector2D Min[]={{-280,-260},{-125,-175},{-110,-270}},Max[]={{285,260},{225,175},{200,270}};
 for(int Id=1;Id<=3;++Id){const FVector2D P(Machine(Id));Out.Emplace(P+Min[Id-1],P+Max[Id-1]);}
 for(const auto& Rack:Racks()){const FVector2D Half=Rack.Yaw==0?FVector2D(60,208):FVector2D(208,60);Out.Emplace(FVector2D(Rack.Center)-Half,FVector2D(Rack.Center)+Half);}
 return Out;
}
TArray<FFoundrylineFactorySlot> FFoundrylineWorkshopLayout::Slots(){
 TArray<FFoundrylineFactorySlot> Out;
 auto Add=[&](int Id,EFoundrylinePart Part,FVector Center,int Level,bool Across=false){FFoundrylineFactorySlot S;S.Id=Id;S.Part=Part;S.Position=Center+FVector(0,0,SlotHeight(Level));S.Dock=Center+(Across?FVector(0,136,0):FVector(136,0,0));S.MachineId=Id==100||Id>=107&&Id<=109?1:Id==101||Id==102||Id==110||Id==111?2:Id==103||Id==112?3:0;S.bInputSupply=Id==100||Id>=110;Out.Add(S);};
 Add(100,EFoundrylinePart::Sheet,InputRack(1),1);
 Add(101,EFoundrylinePart::BentBracket,OutputRack(2),0);Add(102,EFoundrylinePart::FrameRail,OutputRack(2),1);Add(103,EFoundrylinePart::ShellSection,OutputRack(3),0);
 FFoundrylineFactorySlot Head;Head.Id=104;Head.Part=EFoundrylinePart::EndCap;Head.Position={800,3000,20};Head.Dock={800,3136,20};Out.Add(Head);
 Add(105,EFoundrylinePart::RunningGear,{1600,3000,20},0,true);Add(106,EFoundrylinePart::Fittings,{1600,3000,20},1,true);
 Add(107,EFoundrylinePart::Bracket,OutputRack(1),0);Add(108,EFoundrylinePart::RailBlank,OutputRack(1),1);Add(109,EFoundrylinePart::ShellBlank,OutputRack(1),2);
 Add(110,EFoundrylinePart::Bracket,InputRack(2),1);Add(111,EFoundrylinePart::RailBlank,InputRack(2),2);Add(112,EFoundrylinePart::ShellBlank,InputRack(3),1);
 return Out;
}
const TCHAR* FFoundrylineWorkshopLayout::SlotName(int Id){switch(Id){case 100:return TEXT("Plasma / sheet supply");case 101:case 102:return TEXT("Brake / finished parts");case 103:return TEXT("Roller / finished shells");case 104:return TEXT("Whole-head storage");case 105:case 106:return TEXT("Purchased kits");case 107:case 108:case 109:return TEXT("Plasma / cut blanks");case 110:case 111:return TEXT("Brake / blank supply");case 112:return TEXT("Roller / blank supply");default:return TEXT("Assigned storage");}}

TArray<FBox2D> FFoundrylineWorkshopLayout::PlantSolids(const FFoundrylineFactoryState& State){
 TArray<FBox2D> Out;for(const auto& Item:FFoundrylinePlannerItems::List(State))if(!Item.Stored&&(Item.Kind==EFoundrylinePlannerItem::Machine||Item.Kind==EFoundrylinePlannerItem::StorageRack||Item.Kind==EFoundrylinePlannerItem::FixedRack))Out.Emplace(FVector2D(Item.Bounds.Min),FVector2D(Item.Bounds.Max));return Out;
}
TArray<FBox2D> FFoundrylineWorkshopLayout::CutShopSolids(){
 TArray<FBox2D> Out;const FVector2D P(Machine(1));Out.Emplace(P+FVector2D(-280,-260),P+FVector2D(285,260));
 for(const auto& R:CutShopRacks())Out.Emplace(FVector2D(R.Center)-FVector2D(60,208),FVector2D(R.Center)+FVector2D(60,208));return Out;
}
