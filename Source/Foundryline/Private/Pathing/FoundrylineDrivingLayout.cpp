#include "Pathing/FoundrylineDrivingLayout.h"
bool FFoundrylineDrivingLayout::IsPaved(FName F) {return F==TEXT("Road2") || F==TEXT("Road4") || F==TEXT("Service") || F==TEXT("Driveway") || F==TEXT("TruckAccess") || F==TEXT("Parking") || F==TEXT("FreightYard");}
TArray<FFoundrylineParkingBay> FFoundrylineDrivingLayout::Parking(float W,float D,const TArray<FFoundrylineDriveRect>& Entrances)
{
 TArray<FFoundrylineParkingBay> Out;
 auto Row=[&](float Low,float High,float Rear,float Yaw,bool Central) {
  for(float X=50;X+250<=W-50;X+=250) {
   FFoundrylineDriveRect R{{X,Low},{X+250,High}};
   if(Central && (X<800 || X+250>W-800)) continue;
   if(Entrances.ContainsByPredicate([&](const auto& E){return R.Overlaps(E);})) continue;
   Out.Add({R,{X+125,Rear},Yaw});
  }
 };
 Row(0,550,425,-90,false);
 if(D>=1700) Row(D-550,D,D-425,90,false);
 if(D>=3700 && W>=2400) {Row(D/2-650,D/2-100,D/2-500,90,true);Row(D/2+100,D/2+650,D/2+500,-90,true);}
 return Out;
}
TArray<FFoundrylineDriveRect> FFoundrylineDrivingLayout::ParkingAisles(float W,float D,const TArray<FFoundrylineDriveRect>& Entrances)
{
 TArray<FFoundrylineDriveRect> Out=Entrances;
 if(D>=3700 && W>=2400) {
  Out.Add({{0,550},{W,D/2-700}});Out.Add({{0,D/2+700},{W,D-550}});
  Out.Add({{0,550},{800,D-550}});Out.Add({{W-800,550},{W,D-550}});
 } else Out.Add({{0,550},{W,D>=1700?D-550:D}});
 return Out;
}
TArray<FFoundrylineDriveRect> FFoundrylineDrivingLayout::Islands(float W,float D,bool Green)
{
 TArray<FFoundrylineDriveRect> Out;if(Green && D>=3700 && W>=2400 && W>1600) Out.Add({{800,D/2-100},{W-800,D/2+100}});return Out;
}
