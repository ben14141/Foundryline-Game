#include "Misc/AutomationTest.h"
#include "Construction/FoundrylineFacadeLayout.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineFacadeTest,"Foundryline.Construction.AdaptiveFacadeBays",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoundrylineFacadeTest::RunTest(const FString&)
{
 for(int32 Length=400;Length<=6400;Length+=100) for(int32 Bay:{400,600,800})
 {
  const auto Windows=FFoundrylineFacadeLayout::Windows(Length,Bay);
  const auto Columns=FFoundrylineFacadeLayout::Columns(Length,Bay);
  TestEqual(TEXT("Each window has two bay boundaries"),Columns.Num(),Windows.Num()+1);
  for(int32 I=0;I<Windows.Num();++I)
  {
   TestTrue(TEXT("Native 2 m window has corner clearance"),Windows[I]>=100 && Windows[I]+200<=Length-100);
   TestTrue(TEXT("Window centered in evenly distributed bay"),FMath::Abs(Windows[I]+100-(I+.5)*Length/Windows.Num())<=.51);
   for(float Column:Columns) TestTrue(TEXT("Column and cap clear window frames"),Column+28<=Windows[I]-6 || Column-28>=Windows[I]+206);
  }
 }
 FFoundrylinePlacedObject Object;Object.ObjectId=FGuid::NewGuid();
 FFoundrylineOpening Door;Door.OpeningId=FGuid::NewGuid();Door.OffsetCm=400;Door.Kind=EFoundrylineOpeningKind::Garage;Object.Openings.Add(Door);
 FFoundrylineFacadeLayout::Reflow(Object,1700,2300,1220);
 const auto First=Object.Openings;
 FFoundrylineFacadeLayout::Reflow(Object,1700,2300,1220);
 TestEqual(TEXT("Reflow is stable"),Object.Openings.Num(),First.Num());
 for(int32 I=0;I<First.Num();++I) TestTrue(TEXT("Unchanged opening retains identity"),Object.Openings[I].OpeningId==First[I].OpeningId);
 TestTrue(TEXT("Manual garage retained"),Object.Openings.ContainsByPredicate([&](const auto& V){return V.OpeningId==Door.OpeningId;}));
 Object.bOfficeAnnex=true;const int32 Count=Object.Openings.Num();FFoundrylineFacadeLayout::Reflow(Object,3000,3000,320);
 TestEqual(TEXT("Office windows remain manual"),Object.Openings.Num(),Count);
 // Partial expansions leave real exterior returns; caps and posts must fit wholly inside each return.
 FFoundrylineOpening Opening;Opening.OffsetCm=800;Opening.Kind=EFoundrylineOpeningKind::ShopWindow;
 const auto Partial=FFoundrylineFacadeLayout::ColumnsForSpan(2400,600,450,1850,{Opening});
 TestTrue(TEXT("Partial expansion retains framing"),Partial.Num()>0);
 for(float P:Partial){TestTrue(TEXT("No half-width post at an exposed span end"),P-28>=450 && P+28<=1850);TestTrue(TEXT("Post clears glazing and frame"),P+34<=Opening.OffsetCm-6 || P-34>=Opening.OffsetCm+Opening.WidthCm()+6);}
 TestTrue(TEXT("Thin return cannot generate a clipped pilaster"),FFoundrylineFacadeLayout::ColumnsForSpan(2400,600,500,580,{}).IsEmpty());
 for(int32 Size=400;Size<=6400;Size+=100){const auto Posts=FFoundrylineFacadeLayout::ColumnsForSpan(Size,600,0,Size,{});for(int I=1;I<Posts.Num();++I)TestTrue(TEXT("No doubled posts on odd sized shells"),Posts[I]-Posts[I-1]>=60);}

 return true;
}
