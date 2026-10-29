#include "Production/FoundrylineFactoryTypes.h"
FFoundrylinePartSpec FFoundrylineFactoryCatalog::Part(EFoundrylinePart Id){
 static const FFoundrylinePartSpec Specs[]={{TEXT("Steel sheets"),10},{TEXT("Bracket blanks"),50},{TEXT("Rail blanks"),12},{TEXT("Frame rails"),12},{TEXT("Shell blanks"),3},{TEXT("Rolled shells"),3},{TEXT("Dished tank heads"),2},{TEXT("Running gear kits"),2},{TEXT("Fittings kits"),10},{TEXT("Bent brackets"),50},{TEXT("Mounting plates"),100},{TEXT("Gusset plates"),75},{TEXT("Reinforcement plates"),30},{TEXT("Support channels"),20},{TEXT("Frame sections"),12},{TEXT("Light kits"),8},{TEXT("Headache racks"),1}};
 return Specs[FMath::Clamp(int32(Id),0,PartCount-1)];
}
// Bought parts reuse the closest existing component art: channel lengths for frame sections, a kit crate for lights.
FString FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart Id){
 static const TCHAR* Names[]={TEXT("SteelSheet_60x120"),TEXT("BracketBlank"),TEXT("RailBlank"),TEXT("FrameRail"),TEXT("ShellBlank"),TEXT("ShellSection"),TEXT("TankHead_ASME_FD"),TEXT("RunningGearKit"),TEXT("FittingsKit"),TEXT("BentBracket"),TEXT("MountingPlate"),TEXT("GussetPlate"),TEXT("ReinforcementPlate"),TEXT("SupportChannel"),TEXT("SupportChannel"),TEXT("FittingsKit"),TEXT("HeadacheRack")};
 const int32 I=FMath::Clamp(int32(Id),0,PartCount-1);
 const TCHAR* Folder=I==0?TEXT("Inventory/Materials/Steel"):I==6?TEXT("Products/Components/DishedHeads"):Id==EFoundrylinePart::HeadacheRack?TEXT("Products/HeadacheRack"):TEXT("Products/Components/Fabrication");
 return FString(TEXT("/Game/Foundryline/"))+Folder+TEXT("/SM_")+Names[I];
}
bool FFoundrylineFactoryCatalog::Bought(EFoundrylinePart Id,FFoundrylineBoughtPart* Out){
 FFoundrylineBoughtPart P;
 // Frame section: one ready-cut 2 in square tube member (~4 ft average). Light kit: one lamp with its wiring and hardware.
 if(Id==EFoundrylinePart::FrameSection){P.UnitLb=16;P.PriceCents=2400;}
 else if(Id==EFoundrylinePart::LightKit){P.UnitLb=4.5f;P.PriceCents=3800;}
 else return false;
 if(Out)*Out=P;return true;
}
const TCHAR* FFoundrylineFactoryCatalog::AssemblyName(EFoundrylinePart Id){switch(Id){case EFoundrylinePart::MountingPlate:return TEXT("Steel Plates");case EFoundrylinePart::Bracket:return TEXT("Steel Brackets");case EFoundrylinePart::FrameSection:return TEXT("Frame Sections");case EFoundrylinePart::LightKit:return TEXT("Light Kits");default:return Part(Id).Name;}}
// Initial balancing bill from the welding workstation plan; visual slats, gussets and bolts are not extra SKUs.
bool FFoundrylineFactoryCatalog::Recipe(int32 Id,FFoundrylineAssemblyRecipe& Out){
 if(Id!=1)return false;
 Out.Id=1;Out.Name=TEXT("Headache rack");Out.Output=EFoundrylinePart::HeadacheRack;Out.Bill.Init(0,PartCount);
 Out.Bill[int32(EFoundrylinePart::MountingPlate)]=2;Out.Bill[int32(EFoundrylinePart::Bracket)]=4;Out.Bill[int32(EFoundrylinePart::FrameSection)]=6;Out.Bill[int32(EFoundrylinePart::LightKit)]=2;
 // Two welders share one rack: 170% together, less than double because they work around each other at one fixture.
 Out.TwoWelderRate=1.7f;
 return true;
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
