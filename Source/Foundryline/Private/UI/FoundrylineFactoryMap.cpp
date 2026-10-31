#include "UI/FoundrylineBuildHUD.h"
#include "UI/FoundrylinePlayerController.h"
#include "Core/FoundrylineUnits.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Production/FoundrylineWorkZoneLayout.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Production/FoundrylineMachineCellLayout.h"
void AFoundrylineBuildHUD::DrawFactoryMap(AFoundrylinePlayerController* PC){if(PC->IsWalking()||(!PC->bFactoryOverlay&&PC->FactorySelectedPart<0))return;auto* Plant=PC->FactoryPlant();if(!Plant)return;
 const float S=FMath::Min(1.5f,Canvas->ClipY/1320.f),W=Canvas->ClipX/S;const FLinearColor Ink(.055,.065,.075,.94),Paper(.96,.95,.93),Orange(1,.4,.08),Blue(.2,.58,1),Yellow(1,.8,.12),Green(.18,.9,.38);
 auto T=[&](const FString& V,float X,float Y,FLinearColor C,float Size=1.f){DrawText(V,C,X*S,Y*S,GEngine->GetMediumFont(),Size*S);};
 TArray<FBox2D> Occupied;auto Label=[&](FVector World,const FString& Value,FLinearColor C){FVector2D P;if(!PC->ProjectWorldLocationToScreen(World+FVector(0,0,40),P,false))return;int VW,VH;PC->GetViewportSize(VW,VH);if(VW<=0||VH<=0)return;P.X*=Canvas->ClipX/VW;P.Y*=Canvas->ClipY/VH;float TW,TH;GetTextSize(Value,TW,TH,GEngine->GetMediumFont(),1.05*S);FBox2D Box(P-FVector2D(8*S,8*S),P+FVector2D(TW+8*S,TH+8*S));if(Box.Min.X<430*S||Box.Max.X>Canvas->ClipX-300*S||Box.Min.Y<15*S||Box.Max.Y>Canvas->ClipY-45*S)return;for(const auto& B:Occupied)if(B.Intersect(Box))return;Occupied.Add(Box);DrawRect(Ink,Box.Min.X,Box.Min.Y,Box.GetSize().X,Box.GetSize().Y);DrawRect(C,Box.Min.X,Box.Min.Y,4*S,Box.GetSize().Y);DrawText(Value,Paper,P.X,P.Y,GEngine->GetMediumFont(),1.05*S);};
 FFoundrylineFactorySimulation View;View.State=Plant->Snapshot;const EFoundrylinePart Parts[]={EFoundrylinePart::BentBracket,EFoundrylinePart::FrameRail,EFoundrylinePart::ShellSection,EFoundrylinePart::EndCap,EFoundrylinePart::RunningGear,EFoundrylinePart::Fittings};
 if(PC->bFactoryOverlay){
 for(int I=0;I<Plant->Snapshot.Zones.Num();++I){const auto& Z=Plant->Snapshot.Zones[I];Label(Z.Min+FVector(80,80,0),FString::Printf(TEXT("WORK ZONE %02d / TANKER ASSEMBLY"),I+1),Orange);Label(FFoundrylineWorkZoneLayout::WelderPosition(Z)+FVector(0,0,170),TEXT("STANDARD MIG WELDER"),Orange);Label(Z.DropMin+FVector(30,-100,0),FString::Printf(TEXT("ZONE %02d / SUPPLIES"),I+1),Blue);for(auto P:Parts){const FVector Pos=View.BufferPosition(Z,P);Label(Pos,FString(FFoundrylineFactoryCatalog::Part(P).Name),Blue);}

 }
 // Sits below the PRODUCTION / OVERLAY buttons in the top-right corner.
  DrawRect(Ink,(W-282)*S,246*S,258*S,175*S);T(TEXT("FLOOR LEGEND"),W-266,264,Paper,1.15);const TCHAR* Names[]={TEXT("WORK / MACHINE CELLS"),TEXT("PARTS INSIDE ZONE"),TEXT("FORKLIFT LANES")};const FLinearColor Colors[]={Orange,Blue,Yellow};for(int I=0;I<UE_ARRAY_COUNT(Names);++I){DrawRect(Colors[I],(W-266)*S,(304+I*35)*S,22*S,8*S);T(Names[I],W-232,299+I*35,Paper,.9);}
 }
 if(PC->FactorySelectedPart>=0){const auto Part=EFoundrylinePart(PC->FactorySelectedPart);const auto Size=FFoundrylineFactoryCatalog::DropSizeCm(Part);DrawRect(Ink,(W-330)*S,510*S,306*S,190*S);T(FFoundrylineFactoryCatalog::Part(Part).Name,W-312,529,Blue,1.2);T(TEXT("PART-SIZED DELIVERY BAY"),W-312,568,Paper,.95);T(FFoundrylineUnits::Length(Size.X)+TEXT(" x ")+FFoundrylineUnits::Length(Size.Y),W-312,607,Paper,1.05);T(FString::Printf(TEXT("Pallet capacity: %d"),FFoundrylineFactoryCatalog::Part(Part).Capacity),W-312,648,Paper,1);}
}
