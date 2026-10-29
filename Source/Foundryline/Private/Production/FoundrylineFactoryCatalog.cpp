#include "Production/FoundrylineFactoryTypes.h"
FFoundrylinePartSpec FFoundrylineFactoryCatalog::Part(EFoundrylinePart Id){
 static const FFoundrylinePartSpec Specs[]={{TEXT("Steel sheets"),10},{TEXT("Bracket blanks"),50},{TEXT("Rail blanks"),12},{TEXT("Frame rails"),12},{TEXT("Shell blanks"),3},{TEXT("Rolled shells"),3},{TEXT("Dished tank heads"),2},{TEXT("Running gear kits"),2},{TEXT("Fittings kits"),10},{TEXT("Bent brackets"),50},{TEXT("Mounting plates"),100},{TEXT("Gusset plates"),75},{TEXT("Reinforcement plates"),30},{TEXT("Support channels"),20}};
 return Specs[FMath::Clamp(int32(Id),0,PartCount-1)];
}
TArray<FFoundrylineAssemblyStep> FFoundrylineFactoryCatalog::TankerSteps(){
 auto Need=[](std::initializer_list<TPair<EFoundrylinePart,int32>> Items){TArray<int32> N;N.Init(0,PartCount);for(auto I:Items)N[int32(I.Key)]=I.Value;return N;};
 return {{TEXT("Weld chassis"),0,60,Need({{EFoundrylinePart::FrameRail,12},{EFoundrylinePart::BentBracket,24}})},
 {TEXT("Install running gear"),1,20,Need({{EFoundrylinePart::RunningGear,1}})},
 {TEXT("Weld tank sections"),1,192,Need({{EFoundrylinePart::ShellSection,48}})},
 {TEXT("Close tank ends"),4,40,Need({{EFoundrylinePart::EndCap,2}})},
 {TEXT("Fit valves and equipment"),14,20,Need({{EFoundrylinePart::Fittings,1}})},
 {TEXT("Final inspection"),31,10,Need({})}};
}
TArray<int32> FFoundrylineFactoryCatalog::TankerBill(){TArray<int32> N;N.Init(0,PartCount);for(const auto& S:TankerSteps())for(int I=0;I<PartCount;++I)N[I]+=S.Parts[I];return N;}
TArray<FFoundrylineFactoryProgram> FFoundrylineFactoryCatalog::DefaultPrograms(){TArray<FFoundrylineFactoryProgram> Rows;
 auto Add=[&](int Machine,EFoundrylinePart Part,float Seconds,float Lb){FFoundrylineFactoryProgram R;R.Machine=Machine;R.Part=Part;R.Seconds=Seconds;R.SteelLb=Lb;Rows.Add(R);};
 Add(1,EFoundrylinePart::Bracket,8,5);Add(1,EFoundrylinePart::RailBlank,15,52);Add(1,EFoundrylinePart::ShellBlank,12,96);
 Add(2,EFoundrylinePart::BentBracket,8,0);Add(2,EFoundrylinePart::FrameRail,16,0);Add(3,EFoundrylinePart::ShellSection,18,0);Add(1,EFoundrylinePart::MountingPlate,4,2);Add(1,EFoundrylinePart::GussetPlate,6,3);Add(1,EFoundrylinePart::ReinforcementPlate,12,8);Add(2,EFoundrylinePart::SupportChannel,12,0);return Rows;
}
FVector2D FFoundrylineFactoryCatalog::DropSizeCm(EFoundrylinePart P){switch(P){case EFoundrylinePart::BentBracket:return {180,180};case EFoundrylinePart::FrameRail:return {180,360};case EFoundrylinePart::ShellSection:return {180,210};case EFoundrylinePart::EndCap:return {285,285};case EFoundrylinePart::RunningGear:return {160,160};case EFoundrylinePart::Fittings:return {140,140};default:return {180,360};}}
