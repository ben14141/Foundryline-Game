#include "Management/FoundrylineOfficeAssignments.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylinePlanningEquipment.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "UI/FoundrylineOperationsTerminal.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

namespace {
using Spec=FFoundrylineOfficeRoomSpec;
bool Same(const FFoundrylineOfficeAssignment& A,FGuid B,FGuid R,int F){return A.BuildingId==B&&A.RoomId==R&&A.Floor==F;}
FBox2D LocalBox(const Spec& S,FVector P,int Q,FVector Size){FBox2D B(ForceInit);const FRotator Rotation(0,Q*90,0);for(double X:{-.5,.5})for(double Y:{-.5,.5})B+=FVector2D(S.Frame.InverseTransformPosition(P+Rotation.RotateVector({X*Size.X,Y*Size.Y,0})));return B;}
bool Fits(const Spec& S,const FBox2D& B,bool Wall=false){if(!S.Bounds.ExpandBy(Wall?-8:-12).IsInsideOrOn(B.Min)||!S.Bounds.ExpandBy(Wall?-8:-12).IsInsideOrOn(B.Max))return false;for(const auto& K:S.KeepClear)if(K.Intersect(B))return false;if(Wall)for(const auto& K:S.WallOpenings)if(K.Intersect(B))return false;return true;}
void Openings(Spec& S,const TArray<FFoundrylineOpening>& Open,const FBox2D& B){
 for(const auto& V:Open){if(V.OfficeLevel!=S.Floor)continue;const double Off=V.OffsetCm,Width=V.WidthCm();FBox2D Wall,Door;
  switch(V.Edge){case 0:Wall={{B.Min.X+Off-15,B.Min.Y-20},{B.Min.X+Off+Width+15,B.Min.Y+35}};Door={{Wall.Min.X-25,B.Min.Y},{Wall.Max.X+25,B.Min.Y+145}};break;
  case 1:Wall={{B.Max.X-35,B.Min.Y+Off-15},{B.Max.X+20,B.Min.Y+Off+Width+15}};Door={{B.Max.X-145,Wall.Min.Y-25},{B.Max.X,Wall.Max.Y+25}};break;
  case 2:Wall={{B.Min.X+Off-15,B.Max.Y-35},{B.Min.X+Off+Width+15,B.Max.Y+20}};Door={{Wall.Min.X-25,B.Max.Y-145},{Wall.Max.X+25,B.Max.Y}};break;
  default:Wall={{B.Min.X-20,B.Min.Y+Off-15},{B.Min.X+35,B.Min.Y+Off+Width+15}};Door={{B.Min.X,Wall.Min.Y-25},{B.Min.X+145,Wall.Max.Y+25}};break;}
  S.WallOpenings.Add(Wall);if(!V.IsGlazing()){S.KeepClear.Add(Door);S.DoorIds.AddUnique(V.OpeningId);}
 }
}
TArray<Spec> MakeRooms(const FFoundrylinePlacedObject& O,const UFoundrylinePlaceableDefinition& D,const FTransform& Plant){
 TArray<Spec> Result;const FTransform Frame=FTransform(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor)).GetRelativeTransform(Plant);
 for(int Floor=0;Floor<(O.bOfficeSecondFloor?2:1);++Floor){TArray<FFoundrylineOfficeSpace> Rooms=O.OfficeRooms.FilterByPredicate([&](const auto& R){return R.OfficeLevel==Floor;});if(Rooms.IsEmpty()){FFoundrylineOfficeSpace R;R.SpaceId.Invalidate();R.Max={D.Footprint.Max.X,D.Footprint.Max.Y};R.OfficeLevel=Floor;Rooms.Add(R);}
  for(const auto& R:Rooms){Spec S;S.BuildingId=O.ObjectId;S.RoomId=R.SpaceId;S.Floor=Floor;S.Frame=Frame;S.Bounds={FVector2D(R.Min),FVector2D(R.Max)};S.Label=FString::Printf(TEXT("%s / %s"),Floor?TEXT("Second floor"):TEXT("Ground floor"),R.SpaceId.IsValid()?*FString::Printf(TEXT("Room %s"),*R.SpaceId.ToString(EGuidFormats::Digits).Left(4)):TEXT("Open office"));TArray<FFoundrylineOpening> Effective=R.SpaceId.IsValid()?R.Openings:O.Openings;
   if(R.SpaceId.IsValid())for(auto V:O.Openings)if(V.OfficeLevel==Floor&&!Effective.ContainsByPredicate([&](const auto& E){return E.OpeningId==V.OpeningId;})){const bool Touch=V.Edge==0?R.Min.Y==0:V.Edge==1?R.Max.X==D.Footprint.Max.X:V.Edge==2?R.Max.Y==D.Footprint.Max.Y:R.Min.X==0;const int Start=V.Edge%2?R.Min.Y:R.Min.X,End=V.Edge%2?R.Max.Y:R.Max.X;if(Touch&&V.OffsetCm>=Start&&V.OffsetCm+V.WidthCm()<=End){V.OffsetCm-=Start;Effective.Add(V);}}
   for(int Edge=0;Edge<4;++Edge){const bool Exterior=Edge==0?R.Min.Y==0:Edge==1?R.Max.X==D.Footprint.Max.X:Edge==2?R.Max.Y==D.Footprint.Max.Y:R.Min.X==0;S.WallInset[Edge]=Exterior?30:7.5;}Openings(S,Effective,S.Bounds);if(O.bOfficeSecondFloor)S.KeepClear.Add(FFoundrylineOfficeStairLayout::FullBounds(O.OfficeStairOrigin,O.OfficeStairQuarter,O.OfficeStairStyle).ExpandBy(5));Result.Add(S);}
 }
 return Result;
}
void AddWallDetails(const Spec& S,FFoundrylineOfficeAssignment& A){
 using Offices=FFoundrylineOffices;
 const TArray<uint8> Kinds=A.EmployeeId?TArray<uint8>{12,15,16}:TArray<uint8>{12,13,14};
 for(const uint8 Kind:Kinds){if(A.Decorations.ContainsByPredicate([&](const auto& D){return D.Kind==Kind;}))continue;
  const FVector Size=Offices::Size(Kind);const double Height=Kind==12?190:110;bool Added=false;
  for(int Edge=0;Edge<4&&!Added;++Edge)for(const double Fraction:{.5,.25,.75,.125,.875}){
   const double BackDepth=Kind==12||Kind==13?2:Kind==15?3:2.5;const double Inset=S.WallInset[Edge]+BackDepth+.5;FVector2D At;
   const auto B=S.Bounds;const double U=Edge%2?FMath::Lerp(B.Min.Y,B.Max.Y,Fraction):FMath::Lerp(B.Min.X,B.Max.X,Fraction);
   At=Edge==0?FVector2D(U,B.Min.Y+Inset):Edge==1?FVector2D(B.Max.X-Inset,U):Edge==2?FVector2D(U,B.Max.Y-Inset):FVector2D(B.Min.X+Inset,U);
   const int LocalQ=Edge==0?0:Edge==1?1:Edge==2?2:3;const int Q=(FMath::RoundToInt(S.Frame.Rotator().Yaw/90)+LocalQ+4)%4;
   const FVector P=S.Frame.TransformPosition(FVector(At,20+A.Floor*320+Height));const auto Bounds=LocalBox(S,P,Q,FVector(Size.X,BackDepth*2,Size.Z));
   if(!Fits(S,Bounds,true))continue;bool Overlap=false;
   for(const auto& D:A.Decorations)if(Offices::IsWallDecor(D.Kind)||(Offices::Collides(D.Kind)&&Offices::Size(D.Kind).Z>Height-5))if(LocalBox(S,D.Position,D.Quarter,Offices::Size(D.Kind)).ExpandBy(18).Intersect(Bounds)){Overlap=true;break;}
   if(Overlap)continue;FFoundrylineOfficeDecoration D;D.Kind=Kind;D.Position=P;D.Quarter=Q;A.Decorations.Add(D);Added=true;break;
  }
 }
 A.FurnishingVersion=2;
}
}
TArray<FFoundrylineOfficeRoomSpec> FFoundrylineOffices::Rooms(UWorld* W,const FFoundrylineFactoryState& State,const FTransform& Plant){
 if(!W)return {};auto* Placement=W->GetSubsystem<UFoundrylinePlacementSubsystem>();if(!Placement)return {};
 struct Cache{TWeakObjectPtr<UWorld> World;FGuid Company,Plant;FTransform Frame;int64 Revision=-1;TArray<Spec> Specs;};static TArray<Cache> Caches;
 Caches.RemoveAll([](const auto& C){return !C.World.IsValid();});const int64 Rev=Placement->GetRevision();
 if(const auto* C=Caches.FindByPredicate([&](const auto& V){return V.World.Get()==W&&V.Company==State.CompanyId&&V.Plant==State.PlantId&&V.Revision==Rev&&V.Frame.Equals(Plant);}))return C->Specs;
 if(Caches.Num()>=16)Caches.RemoveAt(0);Cache C;C.World=W;C.Company=State.CompanyId;C.Plant=State.PlantId;C.Frame=Plant;C.Revision=Rev;
 for(const auto& O:Placement->GetObjects())if(O.bOfficeAnnex&&O.CompanyId==State.CompanyId&&O.PlantId==State.PlantId)if(const auto* D=Placement->FindDefinition(O.DefinitionId)){C.Specs.Append(MakeRooms(O,*D,Plant));if(C.Specs.Num()>=128){C.Specs.SetNum(128);break;}}
 Caches.Add(C);return C.Specs;
}
const TCHAR* FFoundrylineOffices::Mesh(uint8 Kind){static const TCHAR* Names[]={TEXT(""),TEXT("ExecutiveCredenza"),TEXT("EngineeringCabinet"),TEXT("OfficePlant"),TEXT("GuestChair"),TEXT("OfficeRug"),TEXT("PresidentDeskSet"),TEXT("EngineerDeskSet"),TEXT("PresidentArtwork"),TEXT("BlueprintBoard"),TEXT("DraftingTable"),TEXT("OfficeBookshelf"),TEXT("WallClock"),TEXT("FramedCertificate"),TEXT("IndustrialWallArt"),TEXT("EngineeringWhiteboard"),TEXT("OfficeNoticeboard")};return Kind<UE_ARRAY_COUNT(Names)?Names[Kind]:Names[0];}
FVector FFoundrylineOffices::Size(uint8 K){switch(K){case 1:return {160,45,90};case 2:return {140,55,90};case 3:return {38,38,125};case 4:return {65,68,95};case 5:return {190,170,.6};case 6:return {44,25,25};case 7:return {50,32,18};case 8:return {100,4,70};case 9:return {150,4,100};case 10:return {110,70,95};case 11:return {90,35,180};case 12:return {36,7,36};case 13:return {62,5,78};case 14:return {180,7,72};case 15:return {160,20,95};case 16:return {100,7,75};default:return FVector::ZeroVector;}}
bool FFoundrylineOffices::IsWallDecor(uint8 K){return K==8||K==9||(K>=12&&K<=16);}
bool FFoundrylineOffices::Collides(uint8 K){return K==1||K==2||K==3||K==4||K==10||K==11;}
bool FFoundrylineOffices::Assign(UWorld* W,FFoundrylineFactorySimulation& Model,const FTransform& Plant,FGuid Building,FGuid Room,int Floor,int Person,FString& Error){
 if(Person<0||Floor<0||Floor>1||!Building.IsValid()){Error=TEXT("Choose a person and valid office");return false;}
 if(Person&&!Model.State.OfficeEmployees.ContainsByPredicate([&](const auto& P){return P.Id==Person;})){Error=TEXT("Hire an engineer in this business first");return false;}
 const auto Choices=Rooms(W,Model.State,Plant);const auto* Found=Choices.FindByPredicate([&](const auto& S){return S.BuildingId==Building&&S.RoomId==Room&&S.Floor==Floor;});if(!Found){Error=TEXT("That office is no longer available or belongs to another business");return false;}const auto& S=*Found;if(S.DoorIds.IsEmpty()){Error=TEXT("Add a personnel door to this office before assigning it");return false;}
 const auto* Occupant=Model.State.OfficeAssignments.FindByPredicate([&](const auto& A){return Same(A,Building,Room,Floor);});if(Occupant){Error=Occupant->EmployeeId==Person?TEXT("This person already has this office"):TEXT("That office already belongs to someone else");return false;}
 auto Candidate=Model;Candidate.State.OfficeAssignments.RemoveAll([&](const auto& A){return A.EmployeeId==Person;});if(Candidate.State.OfficeAssignments.Num()>=13){Error=TEXT("Office capacity reached");return false;}
 FFoundrylineOfficeAssignment A;A.BuildingId=Building;A.RoomId=Room;A.Floor=Floor;A.EmployeeId=Person;A.Label=S.Label;A.OccupantName=Person?Model.State.OfficeEmployees.FindByPredicate([&](const auto& P){return P.Id==Person;})->DisplayName:AFoundrylineBusinessRegistry::Find(W)?AFoundrylineBusinessRegistry::Find(W)->OwnerDisplayName(Model.State.CompanyId):FString(TEXT("Business owner"));bool DeskFound=false;
 auto TryDesk=[&](int Id,FVector At,int Q){if(Candidate.State.OfficeAssignments.ContainsByPredicate([&](const auto& Other){return Other.DeskId==Id;}))return false;if(FMath::Abs(S.Frame.InverseTransformPosition(At).Z-(20+Floor*320))>3)return false;const auto B=LocalBox(S,At+FRotator(0,Q*90,0).RotateVector({0,40,0}),Q,FFoundrylinePlanningEquipment::Size(7));if(!Fits(S,B))return false;A.DeskId=Id;A.DeskPosition=At;A.DeskQuarter=Q;return true;};
 for(const auto& D:Candidate.State.OfficeDesks)if(TryDesk(D.Id,D.Center,D.QuarterTurns)){DeskFound=true;break;}
 if(!DeskFound)for(TActorIterator<AFoundrylineOperationsTerminal> It(W);It;++It)if(It->Terminal==1&&It->PlanningDeskId==0&&It->PlantId==Model.State.PlantId){const FVector P=Plant.InverseTransformPosition(It->GetActorLocation());const int Q=(FMath::RoundToInt((It->GetActorRotation().Yaw-Plant.Rotator().Yaw)/90)+4)%4;if(TryDesk(0,P,Q)){DeskFound=true;break;}}
 if(!DeskFound){const auto Center=S.Bounds.GetCenter();TArray<FVector2D> Points={Center};for(int Y=0;Y<7;++Y)for(int X=0;X<7;++X)Points.Add(S.Bounds.Min+FVector2D((X+.5)*S.Bounds.GetSize().X/7,(Y+.5)*S.Bounds.GetSize().Y/7));
  for(const auto P:Points){if(DeskFound)break;for(int Quarter=0;Quarter<4;++Quarter){const FVector At=S.Frame.TransformPosition({FMath::GridSnap(P.X,25.),FMath::GridSnap(P.Y,25.),double(20+Floor*320)});const int Q=(FMath::RoundToInt(S.Frame.Rotator().Yaw/90)+Quarter+4)%4;if(!TryDesk(-1,At,Q))continue;FString Why;auto Check=Candidate;Check.State.Running=false;Check.State.Commerce.CashCents=FMath::Max<int64>(Check.State.Commerce.CashCents,50000);Check.State.Commerce.UnpaidCents=0;Check.State.FloorMin=Check.State.FloorMin.ComponentMin(FVector2D(At)-FVector2D(200));Check.State.FloorMax=Check.State.FloorMax.ComponentMax(FVector2D(At)+FVector2D(200));if(!Check.CanBuyPlanningEquipment(7,At,Q,1,Why))continue;bool PawnHere=false;const auto B=FFoundrylinePlanningEquipment::Bounds(At+FRotator(0,Q*90,0).RotateVector({0,40,0}),Q,FFoundrylinePlanningEquipment::Size(7));for(TActorIterator<APawn> It(W);It;++It)if(It->GetController()&&FMath::Abs(Plant.InverseTransformPosition(It->GetActorLocation()).Z-At.Z)<170&&B.ExpandBy(35).IsInsideOrOn(FVector2D(Plant.InverseTransformPosition(It->GetActorLocation()))))PawnHere=true;if(PawnHere)continue;
    FFoundrylinePlacedOfficeDesk D;D.Id=Candidate.State.NextId++;D.Center=At;D.QuarterTurns=Q;Candidate.State.OfficeDesks.Add(D);A.DeskId=D.Id;Candidate.State.FloorMin=Check.State.FloorMin;Candidate.State.FloorMax=Check.State.FloorMax;DeskFound=true;break;}}
 }
 if(!DeskFound){Error=TEXT("No clear computer-and-chair position fits. Leave doors and stairs clear or enlarge the office");return false;}
 A.Id=Candidate.State.NextId++;const auto DeskB=LocalBox(S,A.DeskPosition+FRotator(0,A.DeskQuarter*90,0).RotateVector({0,40,0}),A.DeskQuarter,FFoundrylinePlanningEquipment::Size(7));TArray<FBox2D> Taken={DeskB.ExpandBy(12)};
 for(const auto& D:Candidate.State.OfficeDesks)if(D.Id!=A.DeskId&&FMath::Abs(D.Center.Z-A.DeskPosition.Z)<50)Taken.Add(LocalBox(S,D.Center+FRotator(0,D.QuarterTurns*90,0).RotateVector({0,40,0}),D.QuarterTurns,FFoundrylinePlanningEquipment::Size(7)).ExpandBy(12));
 auto Place=[&](uint8 K,FVector2D P,int LocalQ,bool Wall=false){const FVector At=S.Frame.TransformPosition({P.X,P.Y,double(20+Floor*320)+(Wall?110:0)});const int Q=(FMath::RoundToInt(S.Frame.Rotator().Yaw/90)+LocalQ+4)%4;const auto B=LocalBox(S,At,Q,Size(K));if(!Fits(S,B,Wall))return false;if(Collides(K)){for(const auto& T:Taken)if(T.Intersect(B.ExpandBy(15)))return false;for(TActorIterator<APawn> It(W);It;++It)if(It->GetController()){const auto Local=S.Frame.InverseTransformPosition(Plant.InverseTransformPosition(It->GetActorLocation()));if(FMath::Abs(Local.Z-(20+Floor*320))<170&&B.ExpandBy(35).IsInsideOrOn(FVector2D(Local)))return false;}Taken.Add(B.ExpandBy(15));}FFoundrylineOfficeDecoration D;D.Kind=K;D.Position=At;D.Quarter=Q;A.Decorations.Add(D);return true;};
 const auto Min=S.Bounds.Min,Max=S.Bounds.Max,Mid=S.Bounds.GetCenter();
 auto WallObject=[&](uint8 K,bool Wall=false){const auto Z=Size(K);double Margin[4];for(int Edge=0;Edge<4;++Edge)Margin[Edge]=S.WallInset[Edge]+(Wall?.5:10);const TArray<TPair<FVector2D,int>> Spots={TPair<FVector2D,int>(FVector2D(Mid.X,Min.Y+Z.Y/2+Margin[0]),0),TPair<FVector2D,int>(FVector2D(Mid.X,Max.Y-Z.Y/2-Margin[2]),2),TPair<FVector2D,int>(FVector2D(Min.X+Z.Y/2+Margin[3],Mid.Y),3),TPair<FVector2D,int>(FVector2D(Max.X-Z.Y/2-Margin[1],Mid.Y),1),TPair<FVector2D,int>(FVector2D(Min.X+Z.X/2+25,Max.Y-Z.Y/2-Margin[2]),2)};for(const auto& Spot:Spots)if(Place(K,Spot.Key,Spot.Value,Wall))return true;return false;};
 WallObject(Person?2:1);WallObject(Person?9:8,true);if(!Person)WallObject(11);
 for(const auto P:TArray<FVector2D>{{Min.X+60,Min.Y+60},{Max.X-60,Max.Y-60},{Max.X-60,Min.Y+60},{Min.X+60,Max.Y-60}})if(Place(3,P,0))break;
 if(Person)WallObject(10);else{const FVector Root=S.Frame.InverseTransformPosition(A.DeskPosition);const FRotator R(0,A.DeskQuarter*90-S.Frame.Rotator().Yaw,0);for(double Side:{-55.,55.}){const FVector P=Root+R.RotateVector({Side,-165,0});Place(4,FVector2D(P),(FMath::RoundToInt(R.Yaw/90)+1+4)%4);}}
 auto Accessory=[&](uint8 K,FVector Offset){FFoundrylineOfficeDecoration D;D.Kind=K;D.Position=A.DeskPosition+FRotator(0,A.DeskQuarter*90,0).RotateVector(Offset);D.Quarter=A.DeskQuarter;A.Decorations.Add(D);};Accessory(Person?7:6,{-45,-4,76.3});
 // A flat rug may share the desk/chair footprint without adding a physical obstacle.
 const FVector Rug=A.DeskPosition+FRotator(0,A.DeskQuarter*90,0).RotateVector({0,40,0});const auto RugB=LocalBox(S,Rug,A.DeskQuarter,Size(5));if(Fits(S,RugB)){FFoundrylineOfficeDecoration D;D.Kind=5;D.Position=Rug;D.Position.Z=S.Frame.TransformPosition({0,0,double(22.1+Floor*320)}).Z;D.Quarter=A.DeskQuarter;A.Decorations.Add(D);}
 AddWallDetails(S,A);Candidate.State.OfficeAssignments.Add(A);if(!Candidate.Valid(Error))return false;++Candidate.State.Revision;Candidate.Solids=FFoundrylineWorkshopLayout::PlantSolids(Candidate.State);Model=MoveTemp(Candidate);Error.Reset();return true;
}
bool FFoundrylineOffices::Release(FFoundrylineFactorySimulation& Model,int Person,FString& E){const int Removed=Model.State.OfficeAssignments.RemoveAll([&](const auto& A){return A.EmployeeId==Person;});if(!Removed){E=TEXT("This person has no office assigned");return false;}++Model.State.Revision;Model.Solids=FFoundrylineWorkshopLayout::PlantSolids(Model.State);return true;}
bool FFoundrylineOffices::DeskInUse(const FFoundrylineFactoryState& S,int Desk){return S.OfficeAssignments.ContainsByPredicate([&](const auto& A){return A.DeskId==Desk;});}
bool FFoundrylineOffices::Valid(const FFoundrylineFactoryState& S,FString& E){
 auto Fail=[&](){E=TEXT("Invalid office ownership, occupant, workstation or furnishing record");return false;};if(S.OfficeAssignments.Num()>13||S.OfficeEmployees.Num()>12)return Fail();TSet<int> Ids,People,Desks;TSet<FString> Rooms;
 for(const auto& A:S.OfficeAssignments){const FString Key=A.BuildingId.ToString()+A.RoomId.ToString()+FString::FromInt(A.Floor);if(A.Id<1||A.Id>=S.NextId||Ids.Contains(A.Id)||!A.BuildingId.IsValid()||A.Floor>1||A.EmployeeId<0||People.Contains(A.EmployeeId)||A.EmployeeId&&!S.OfficeEmployees.ContainsByPredicate([&](const auto& P){return P.Id==A.EmployeeId;})||A.DeskId<0||Desks.Contains(A.DeskId)||A.DeskPosition.ContainsNaN()||A.DeskQuarter>3||A.OccupantName.IsEmpty()||A.OccupantName.Len()>64||A.Label.IsEmpty()||A.Label.Len()>120||A.Decorations.Num()>24||A.FurnishingVersion>2||Rooms.Contains(Key))return Fail();if(A.DeskId){const auto* D=S.OfficeDesks.FindByPredicate([&](const auto& D){return D.Id==A.DeskId;});if(!D||!D->Center.Equals(A.DeskPosition,.1)||D->QuarterTurns!=A.DeskQuarter)return Fail();}Ids.Add(A.Id);People.Add(A.EmployeeId);Desks.Add(A.DeskId);Rooms.Add(Key);for(const auto& D:A.Decorations)if(D.Kind<1||D.Kind>16||D.Position.ContainsNaN()||D.Quarter>3)return Fail();}
 return true;
}
bool FFoundrylineOffices::ProtectConstruction(UWorld* W,const FFoundrylinePlacedObject& O,FString& E){
 if(!W)return true;auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();const auto* Old=P?P->GetObjects().FindByPredicate([&](const auto& A){return A.ObjectId==O.ObjectId;}):nullptr;if(!Old)return true;
 for(TActorIterator<AFoundrylineFactoryPlant> It(W);It;++It){const auto& S=It->HasAuthority()?It->Model.State:It->Snapshot;for(const auto& A:S.OfficeAssignments)if(A.BuildingId==O.ObjectId){bool Safe=O.bOfficeAnnex&&!O.DefinitionId.IsNone()&&O.Anchor==Old->Anchor&&O.QuarterTurns==Old->QuarterTurns&&O.DefinitionId==Old->DefinitionId&&O.bOfficeSecondFloor==Old->bOfficeSecondFloor&&O.OfficeStairOrigin==Old->OfficeStairOrigin&&O.OfficeStairQuarter==Old->OfficeStairQuarter&&O.OfficeStairStyle==Old->OfficeStairStyle;if(A.RoomId.IsValid()){const auto* Before=Old->OfficeRooms.FindByPredicate([&](const auto& R){return R.SpaceId==A.RoomId;});const auto* After=O.OfficeRooms.FindByPredicate([&](const auto& R){return R.SpaceId==A.RoomId;});Safe&=Before&&After&&Before->Min==After->Min&&Before->Max==After->Max&&Before->OfficeLevel==After->OfficeLevel;}else Safe&=!O.OfficeRooms.ContainsByPredicate([&](const auto& R){return R.OfficeLevel==A.Floor;});if(Safe){const auto* D=P->FindDefinition(O.DefinitionId);const auto Specs=D?MakeRooms(O,*D,It->GetActorTransform()):TArray<Spec>();const auto* Spec=Specs.FindByPredicate([&](const auto& R){return R.RoomId==A.RoomId&&R.Floor==A.Floor;});Safe=Spec&&!Spec->DoorIds.IsEmpty();}if(!Safe){E=TEXT("Unassign the affected offices before changing their rooms, building or staircase");return false;}}}
 return true;
}

bool FFoundrylineOffices::ValidateProperty(const FFoundrylineFactoryState& State,const TArray<FFoundrylinePlacedObject>& Objects,const UFoundrylinePlacementSubsystem* Placement,const FTransform& Plant,FString& E){
 for(const auto& A:State.OfficeAssignments){const auto* O=Objects.FindByPredicate([&](const auto& O){return O.ObjectId==A.BuildingId&&O.bOfficeAnnex&&O.CompanyId==State.CompanyId&&O.PlantId==State.PlantId;});const auto* D=O&&Placement?Placement->FindDefinition(O->DefinitionId):nullptr;if(!D){E=TEXT("An assigned office building is missing or belongs to another business");return false;}const auto Specs=MakeRooms(*O,*D,Plant);const auto* S=Specs.FindByPredicate([&](const auto& S){return S.RoomId==A.RoomId&&S.Floor==A.Floor;});if(!S||S->DoorIds.IsEmpty()||!Fits(*S,LocalBox(*S,A.DeskPosition+FRotator(0,A.DeskQuarter*90,0).RotateVector({0,40,0}),A.DeskQuarter,FFoundrylinePlanningEquipment::Size(7)))){E=TEXT("A saved office room, access door or workstation is invalid");return false;}}
 return true;
}

void FFoundrylineOffices::ReconcileWallDecor(UWorld* World,FFoundrylineFactorySimulation& Model,const FTransform& Frame){
 const auto Specs=Rooms(World,Model.State,Frame);bool Changed=false;
 for(auto& A:Model.State.OfficeAssignments)if(const auto* S=Specs.FindByPredicate([&](const auto& R){return R.BuildingId==A.BuildingId&&R.RoomId==A.RoomId&&R.Floor==A.Floor;})){if(A.FurnishingVersion<2){AddWallDetails(*S,A);Changed=true;}for(auto& P:A.Decorations)if(IsWallDecor(P.Kind)){const auto Box=LocalBox(*S,P.Position,P.Quarter,Size(P.Kind));const bool Hidden=S->WallOpenings.ContainsByPredicate([&](const auto& O){return O.Intersect(Box);});if(P.HiddenByGlazing!=Hidden){P.HiddenByGlazing=Hidden;Changed=true;}}
 }if(Changed)++Model.State.Revision;
}
