#pragma once

#include "CoreMinimal.h"
#include "Construction/FoundrylinePlacementTypes.h"

// Physical centimetres: change bay count/spacing, never stretch a window or pilaster.
struct FFoundrylineFacadeLayout
{
 static int32 BayCount(int32 Length, int32 PreferredBay)
 {
  return FMath::Clamp(FMath::RoundToInt(double(Length)/FMath::Max(400,PreferredBay)),1,FMath::Max(1,Length/400));
 }
 static TArray<int32> Windows(int32 Length,int32 PreferredBay)
 {
  TArray<int32> Result;const int32 Count=BayCount(Length,PreferredBay);
  for(int32 I=0;I<Count;++I) Result.Add(FMath::RoundToInt((I+.5)*double(Length)/Count-100));
  return Result;
 }
 static TArray<float> Columns(int32 Length,int32 PreferredBay)
 {
  TArray<float> Result;const int32 Count=BayCount(Length,PreferredBay);
  for(int32 I=0;I<=Count;++I) Result.Add(I==0?30.f:I==Count?Length-30.f:float(double(I)*Length/Count));
  return Result;
 }
 static void Reflow(FFoundrylinePlacedObject& Object,int32 Width,int32 Depth,int32 Height)
 {
  if(Object.bOfficeAnnex) return;
  const auto Previous=Object.Openings;
  Object.Openings.RemoveAll([](const auto& V){return V.bAutomatic && V.Kind==EFoundrylineOpeningKind::ShopWindow;});
  for(uint8 Edge=0;Edge<4;++Edge) for(int32 Offset:Windows(Edge%2==0?Width:Depth,Object.ColumnBayCm))
  {
   if(Object.Openings.ContainsByPredicate([&](const auto& V){return V.Edge==Edge && Offset<V.OffsetCm+V.WidthCm()+30 && Offset+230>V.OffsetCm;})) continue;
   FFoundrylineOpening Window;Window.Edge=Edge;Window.OffsetCm=Offset;Window.Kind=EFoundrylineOpeningKind::ShopWindow;Window.bAutomatic=true;Window.WindowSillCm=Height>=1220?900:100;
   const auto* Old=Previous.FindByPredicate([&](const auto& V){return V.bAutomatic && V.Kind==Window.Kind && V.Edge==Edge && V.OffsetCm==Offset;});
   Window.OpeningId=Old?Old->OpeningId:FGuid(Object.ObjectId.A^0xFAC00000u^Edge,Object.ObjectId.B,Object.ObjectId.C^uint32(Offset),Object.ObjectId.D^uint32(Window.WindowSillCm));
   Object.Openings.Add(Window);
  }
 }
};
