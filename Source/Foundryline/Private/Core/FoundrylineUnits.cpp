#include "Core/FoundrylineUnits.h"
#include "Internationalization/Regex.h"
FString FFoundrylineUnits::Length(double Cm){if(!FMath::IsFinite(Cm))return TEXT("--");int64 Eighths=FMath::RoundToInt64(FMath::Abs(Cm)/2.54*8);const int64 Feet=Eighths/96;Eighths%=96;const int Inches=Eighths/8,Rest=Eighths%8;FString V=Cm<0?TEXT("-"):TEXT("");if(Feet)V+=FString::Printf(TEXT("%lld' "),Feet);if(Inches||!Rest)V+=FString::FromInt(Inches);if(Rest){int Top=Rest,Bottom=8;while(Top%2==0){Top/=2;Bottom/=2;}if(Inches)V+=TEXT(" ");V+=FString::Printf(TEXT("%d/%d"),Top,Bottom);}return V+TEXT("\"");}
FString FFoundrylineUnits::Display(const FString& Text){
 if(!Text.Contains(TEXT("m")))return Text;
 static const FRegexPattern Measurements(TEXT("(\\d+(?:\\.\\d+)?(?:\\s*(?:[xX,/-]|or|and)\\s*\\d+(?:\\.\\d+)?)*)\\s*(km/h|m/s|cm|mm|m2|m²|m)\\b"));
 static const FRegexPattern Numbers(TEXT("\\d+(?:\\.\\d+)?"));FRegexMatcher Match(Measurements,Text);FString Result;int Cursor=0;
 while(Match.FindNext()){Result+=Text.Mid(Cursor,Match.GetMatchBeginning()-Cursor);const FString Values=Match.GetCaptureGroup(1),Unit=Match.GetCaptureGroup(2);FRegexMatcher Number(Numbers,Values);FString Converted;int At=0;
  while(Number.FindNext()){Converted+=Values.Mid(At,Number.GetMatchBeginning()-At);double V=FCString::Atod(*Number.GetCaptureGroup(0));if(Unit==TEXT("km/h"))Converted+=FString::Printf(TEXT("%.1f mph"),V*.6213711922);else if(Unit==TEXT("m/s"))Converted+=FString::Printf(TEXT("%.1f mph"),V*2.236936292);else if(Unit==TEXT("m2")||Unit==TEXT("m²"))Converted+=FString::Printf(TEXT("%.1f sq ft"),V*10.76391042);else Converted+=Length(V*(Unit==TEXT("cm")?1:Unit==TEXT("mm")?.1:100));At=Number.GetMatchEnding();}
  Result+=Converted+Values.Mid(At);Cursor=Match.GetMatchEnding();
 }return Result+Text.Mid(Cursor);
}
