#include "Economy/FoundrylineEconomyTuning.h"

int64 FFoundrylineEconomyTuning::SalePriceCents(EFoundrylinePart P){
 switch(P){
  case EFoundrylinePart::MountingPlate:return 1350;
  case EFoundrylinePart::GussetPlate:return 1200;
  case EFoundrylinePart::Bracket:return 3000;
  case EFoundrylinePart::ReinforcementPlate:return 2800;
  case EFoundrylinePart::BentBracket:return 5400;
  case EFoundrylinePart::SupportChannel:return 4800;
  case EFoundrylinePart::HeadacheRack:return 100000;
  default:return 0;
 }
}
FFoundrylineCarrierSpec FFoundrylineEconomyTuning::Carrier(int32 I){
 static const FFoundrylineCarrierSpec S[]={
  {TEXT("Swift Van Couriers"),TEXT("Pallet van"),1,900,2500,15,60,160},
  {TEXT("Foundry Lane Haulage"),TEXT("Two-pallet flatbed"),2,2000,4000,20,90,244},
  {TEXT("County Freight"),TEXT("Rigid delivery truck"),6,6000,9000,40,120,240},
  {TEXT("Interstate Pallet Lines"),TEXT("Box semi"),20,24000,18000,60,240,248},
  {TEXT("Ironline Flatbed Freight"),TEXT("53 ft flatbed semi"),26,48000,22500,90,240,259}};
 return S[FMath::Clamp(I,0,UE_ARRAY_COUNT(S)-1)];
}
TArray<FFoundrylineFactoryProgram> FFoundrylineEconomyTuning::MachinePrograms(){
 TArray<FFoundrylineFactoryProgram> Rows;
 auto Add=[&](int Machine,EFoundrylinePart Part,float Minutes,float Lb){FFoundrylineFactoryProgram R;R.Machine=Machine;R.Part=Part;R.Seconds=Minutes;R.SteelLb=Lb;Rows.Add(R);};
 // Plasma (1): minutes per part and pounds of sheet.
 Add(1,EFoundrylinePart::Bracket,8,5);Add(1,EFoundrylinePart::RailBlank,15,52);Add(1,EFoundrylinePart::ShellBlank,12,96);
 // Press brake (2) and roller (3) form a blank; no extra steel.
 Add(2,EFoundrylinePart::BentBracket,8,0);Add(2,EFoundrylinePart::FrameRail,16,0);Add(3,EFoundrylinePart::ShellSection,18,0);
 Add(1,EFoundrylinePart::MountingPlate,4,2);Add(1,EFoundrylinePart::GussetPlate,6,3);Add(1,EFoundrylinePart::ReinforcementPlate,12,8);Add(2,EFoundrylinePart::SupportChannel,12,0);
 return Rows;
}
