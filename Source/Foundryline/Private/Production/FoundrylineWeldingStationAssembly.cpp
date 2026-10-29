// Building one unit at a time on the bench: welders at work, the shared work rate, the stages, the lift onto OUT, and the station status line.
#include "Production/FoundrylineFactorySimulation.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Grid/FoundrylineFloorPlacement.h"
#include "FoundrylineWeldingStationInternal.h"
namespace {using Layout=FFoundrylineWeldingStationLayout;using Place=EFoundrylinePalletPlace;using Part=EFoundrylinePart;using Catalog=FFoundrylineFactoryCatalog;
using FoundrylineWeldingStationInternal::Overlap;using FoundrylineWeldingStationInternal::Access;}
int32 FFoundrylineFactorySimulation::StationWelders(const FFoundrylineWeldingStationRecord& S,bool AtWork)const{
 int32 N=0;for(const auto& W:State.Walkers)if(W.Station==S.Id&&(!AtWork||FVector::Dist2D(W.Position,Layout::WorkPoint(S,W.StationSide))<=1))++N;return N;
}
float FFoundrylineFactorySimulation::StationRate(const FFoundrylineWeldingStationRecord& S,int32 AtWork)const{
 FFoundrylineAssemblyRecipe R;if(!Catalog::Recipe(S.Stage?S.UnitRecipe:S.Recipe,R))Catalog::Recipe(1,R);const int32 N=AtWork<0?StationWelders(S,true):AtWork;return N>=2?R.TwoWelderRate:N==1?1.f:0.f;
}
// One unit at a time on the bench. The kit is claimed when the unit starts, and only if an OUT pallet has room; progress is shared
// by the welders at their work points (100% for one, the recipe's two-welder rate for both) and only advances while the
// plant runs. It is kept through pauses, staff changes and saves.
void FFoundrylineFactorySimulation::AssembleWeldingStations(float Dt){
 for(auto& S:State.WeldingStations){S.bAdvancing=false;FFoundrylineAssemblyRecipe R;
  int32 Slot,Level;const bool OutRoom=Layout::NextOutSpot(S,State.Pallets,Slot,Level);
  if(!S.Stage){
   if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0||!OutRoom||!StationWelderReady(S))continue;
   bool Kit=true;for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]&&Available(Part(P),Place::Buffer,S.Id)<R.Bill[P])Kit=false;if(!Kit)continue;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;
   for(int P=0;P<R.Bill.Num();++P)if(R.Bill[P]){float Lb=0;Consume(Part(P),R.Bill[P],Place::Buffer,S.Id,&Lb);S.Reserved[P]=R.Bill[P];S.ReservedLb+=Lb;}
   S.UnitRecipe=S.Recipe;--S.BatchRemaining;if(!S.BatchRemaining)S.Recipe=0;S.Stage=1;S.Work=0;continue;
  }
  // The finished rack is carried from the fixture onto the first OUT pallet with room; it joins that stack exactly once, on
  // arrival. Racks on one pallet are one stock record (mass kept as the average per rack).
  if(S.Stage==4){S.Work=FMath::Min(S.Work+Dt,Layout::StageSeconds(4));if(S.Work<Layout::StageSeconds(4)||!OutRoom||!Catalog::Recipe(S.UnitRecipe,R))continue;
   auto* Stack=State.Pallets.FindByPredicate([&](const auto& A){return A.Owner==S.Id&&A.Place==Place::StationOut&&Layout::OutSlot(S,A.Position)==Slot;});
   if(Stack){Stack->UnitLb=(Stack->UnitLb*Stack->Quantity+S.ReservedLb)/(Stack->Quantity+1);++Stack->Quantity;}
   else if(State.Pallets.Num()>=64)continue;else AddPallet(R.Output,1,Place::StationOut,S.Id,Layout::OutPalletPoint(S,Slot),S.ReservedLb);
   ++S.UnitsCompleted;
   S.Reserved.Init(0,Catalog::PartCount);S.ReservedLb=0;S.UnitRecipe=0;S.Stage=0;S.Work=0;continue;}
  const float Rate=StationRate(S);if(Rate<=0)continue;
  for(auto& W:State.Walkers)if(W.Station==S.Id&&FVector::Dist2D(W.Position,Layout::WorkPoint(S,W.StationSide))<=1)W.Yaw=Layout::WorkYaw(S,W.StationSide);
  S.Work=FMath::Min(S.Work+Dt*Rate,Layout::StageSeconds(S.Stage));S.bAdvancing=true;if(S.Work<Layout::StageSeconds(S.Stage))continue;S.Work=0;++S.Stage;
 }
}
FString FFoundrylineFactorySimulation::WeldingStationStatus(const FFoundrylineWeldingStationRecord& S)const{
 FFoundrylineAssemblyRecipe R;const bool HasWelder=State.Walkers.ContainsByPredicate([&](const auto& W){return W.Station==S.Id;});
 if(S.Stage&&Catalog::Recipe(S.UnitRecipe,R)){
  static const TCHAR* Stages[]={TEXT(""),TEXT("Fit-up"),TEXT("Welding"),TEXT("Installing lights"),TEXT("Moving to OUT")};const int Percent=FMath::FloorToInt(100*S.Work/Layout::StageSeconds(S.Stage));
  if(S.Stage<4&&!HasWelder)return FString::Printf(TEXT("%s paused at %d%% / assign a welder"),Stages[S.Stage],Percent);
  if(S.Stage<4&&!StationWelderReady(S))return FString::Printf(TEXT("%s paused at %d%% / welder walking"),Stages[S.Stage],Percent);
  return FString::Printf(TEXT("%s %s %d%%%s"),Stages[S.Stage],*FString(R.Name).ToLower(),Percent,S.Stage<4&&StationWelders(S,true)>1?TEXT(" / 2 welders"):TEXT(""));}
 if(!Catalog::Recipe(S.Recipe,R)||S.BatchRemaining<=0){const FString Out=StationOutStatus(S,FString::Printf(TEXT("%d finished on OUT"),OutTotal(S)));return Out.IsEmpty()?FString(TEXT("Choose a product and quantity")):Out;}
 // Both OUT pallets full is what holds the next unit back, whatever the IN stock still needs.
 {int32 Slot,Level;if(!Layout::NextOutSpot(S,State.Pallets,Slot,Level))return StationOutStatus(S,FString::Printf(TEXT("OUT full (%d on %d pallets)"),OutTotal(S),Layout::OutPallets));}
 for(Part P:Catalog::AssemblyParts())if(StationShortfall(S,P)>0){const FString Name=FString(Catalog::AssemblyName(P)).ToLower();
  if(StationRackSupply(P)>0)return TEXT("Requesting ")+Name;
  return Catalog::Bought(P)?TEXT("Missing ")+Name+TEXT(" / buy parts"):TEXT("Missing ")+Name+TEXT(" / cut more on the plasma table");}
 for(Part P:Catalog::AssemblyParts())if(StationStock(S,P,false)<StationTarget(S,P))return TEXT("Receiving ")+FString(Catalog::AssemblyName(P)).ToLower();
 const int Kits=FMath::Min(S.BatchRemaining,2);const FString Ready=FString::Printf(TEXT("Supplies ready for %d %s%s"),Kits,*FString(R.Name).ToLower(),Kits==1?TEXT(""):TEXT("s"));
 return Ready+(!HasWelder?TEXT(" / assign a welder"):!StationWelderReady(S)?TEXT(" / welder walking"):TEXT(""));
}
