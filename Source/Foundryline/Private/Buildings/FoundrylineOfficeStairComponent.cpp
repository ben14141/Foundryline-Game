#include "Buildings/FoundrylineOfficeStairComponent.h"
#include "Buildings/FoundrylineOfficeStairLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"

UFoundrylineOfficeStairComponent::UFoundrylineOfficeStairComponent(){PrimaryComponentTick.bCanEverTick=false;SetCanEverAffectNavigation(false);}
void UFoundrylineOfficeStairComponent::Build(FIntPoint Origin,int32 Quarter,UMaterialInterface* Step,UMaterialInterface* Rail,bool Preview,UMaterialInterface* Ghost,uint8 Style){
 for(auto C:GeneratedParts)if(IsValid(C))C->DestroyComponent();GeneratedParts.Reset();
 if(Quarter<0||Quarter>3||Style>1||!GetOwner())return;
 SetMobility(EComponentMobility::Movable);SetRelativeLocationAndRotation({double(Origin.X),double(Origin.Y),0},FRotator(0,Quarter*90,0));SetMobility(Preview?EComponentMobility::Movable:EComponentMobility::Static);
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Material=[&](const TCHAR* Name,UMaterialInterface* Fallback){const FString Path=FString(TEXT("/Game/Foundryline/Construction/Materials/"))+Name+TEXT(".")+Name;auto* M=LoadObject<UMaterialInterface>(nullptr,*Path);return M?M:Fallback;};
 Step=Material(TEXT("M_OfficeStairOak"),Step);Rail=Material(TEXT("M_OfficeStairRail"),Rail);
 auto* Paint=Material(TEXT("M_OfficeStairPaint"),Step);auto* Glass=Material(TEXT("M_OfficeStairGlass"),Step);
 auto Register=[&](USceneComponent* C,FName Tag){C->SetupAttachment(this);C->SetMobility(Preview?EComponentMobility::Movable:EComponentMobility::Static);C->ComponentTags.Add(TEXT("FoundrylineOfficeStair"));C->ComponentTags.Add(Tag);C->UpdateComponentToWorld();C->RegisterComponent();GetOwner()->AddInstanceComponent(C);GeneratedParts.Add(C);};
 auto Box=[&](FVector At,FVector Size,UMaterialInterface* Mat,bool Collision,FName Tag,FRotator Rotation=FRotator::ZeroRotator){auto* C=NewObject<UStaticMeshComponent>(GetOwner());C->SetupAttachment(this);C->SetRelativeLocationAndRotation(At,Rotation);C->SetRelativeScale3D(Size/100);C->SetStaticMesh(Cube);C->SetMaterial(0,Preview?Ghost:Mat);C->SetCollisionEnabled(!Preview&&Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetCanEverAffectNavigation(false);C->SetCastShadow(!Preview);if(Tag==TEXT("FoundrylineOfficeStairGuard")&&Size.Z<10)C->SetVisibility(Preview);Register(C,Tag);return C;};
 auto Beam=[&](FVector A,FVector B,float Thick,UMaterialInterface* Mat,bool Collision,FName Tag){const FVector D=B-A;auto* C=Box((A+B)/2,{D.Size(),Thick,Thick},Mat,Collision,Tag,D.Rotation());if(Thick==2&&!Preview)C->SetVisibility(false);return C;};
 auto Panel=[&](FVector A,FVector B,double Bottom,double Top,UMaterialInterface* Mat){auto* C=NewObject<UProceduralMeshComponent>(GetOwner());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(Mat!=Glass&&!Preview);C->SetMaterial(0,Preview?Ghost:Mat);Register(C,TEXT("FoundrylineOfficeStairFinish"));const TArray<FVector> V={A+FVector(0,0,Bottom),B+FVector(0,0,Bottom),B+FVector(0,0,Top),A+FVector(0,0,Top)};const FVector N=FVector::CrossProduct(B-A,FVector::UpVector).GetSafeNormal();C->CreateMeshSection_LinearColor(0,V,{0,1,2,0,2,3},{N,N,N,N},{{0,0},{1,0},{1,1},{0,1}},TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);};
 auto Post=[&](FVector Base,double Height){Box(Base+FVector(0,0,Height/2),{3,3,Height},Rail,true,TEXT("FoundrylineOfficeStairGuard"));Box(FVector(FMath::Clamp(Base.X,3.5,Style?132.5:296.5),FMath::Clamp(Base.Y,3.5,Style?760.5:508.5),Base.Z+.5),{7,7,1},Rail,false,TEXT("FoundrylineOfficeStairHardware"));};
 if(Style==1){
  constexpr double Run=504,LowY=130,HighY=634;
  for(int I=0;I<18;++I){const double Y=LowY+(I+.5)*28,Top=20+(I+1)*320./18;
   Box({68,Y,Top-2},{130,28,4},Step,false,TEXT("FoundrylineOfficeStairTread"));
   Box({68,Y-14,Top-160./18},{130,2,320./18},Paint,false,TEXT("FoundrylineOfficeStairRiser"));
   Box({68,Y-12.5,Top+.15},{130,3,.3},Rail,false,TEXT("FoundrylineOfficeStairNosing"));
  }
  for(double X:{17.,119.})Beam({X,LowY,8},{X,HighY,328},12,Paint,false,TEXT("FoundrylineOfficeStairStringer"));
  const FVector UnderA(68,LowY,0),UnderB(68,HighY,320);Box((UnderA+UnderB)/2,{FVector::Distance(UnderA,UnderB),130,4},Paint,false,TEXT("FoundrylineOfficeStairSoffit"),(UnderB-UnderA).Rotation());
  for(double X:{1.5,134.5}){
   Panel({X,LowY,20},{X,HighY,340},15,100,Glass);Panel({X,LowY,20},{X,HighY,340},-25,2,Paint);
   Panel({X,131.5,340},{X,632.5,340},15,100,Glass);
   Beam({X,LowY,130},{X,HighY,450},3,Rail,true,TEXT("FoundrylineOfficeStairHandrail"));
   Beam({X,LowY,75},{X,HighY,395},2,Rail,true,TEXT("FoundrylineOfficeStairGuard"));
   for(int I=0;I<=6;++I)Post({X,LowY+Run*I/6.,20+320.*I/6},110);
   Beam({X,131.5,450},{X,632.5,450},3,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));
   Beam({X,131.5,395},{X,632.5,395},2,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));
   for(double Y:{131.5,258.,384.,510.,632.5})Post({X,Y,340},110);
  }
  Panel({1.5,131.5,340},{134.5,131.5,340},15,100,Glass);
  Beam({1.5,131.5,450},{134.5,131.5,450},3,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));
  Beam({1.5,131.5,395},{134.5,131.5,395},2,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));
  if(!Preview){const FVector A(68,LowY,20),B(68,HighY,340),D=B-A;const FRotator Rotation=D.Rotation();auto* C=NewObject<UBoxComponent>(GetOwner());C->SetupAttachment(this);C->SetRelativeLocationAndRotation((A+B)/2-Rotation.RotateVector(FVector::UpVector)*5,Rotation);C->SetBoxExtent({(D.Size()+1)/2,65,5});C->SetCollisionProfileName(TEXT("BlockAll"));C->SetCanEverAffectNavigation(false);Register(C,TEXT("FoundrylineOfficeStairRamp"));C->SetVisibility(false);C->SetHiddenInGame(true);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));}
  return;
 }
 // Three centimetre rails lie outside each 130cm clear flight, within the 300cm frame.
 const double Left0=3,Left1=133,Right0=167,Right1=297,Run=FFoundrylineOfficeStairLayout::FlightRun;
 const double StepRun=Run/9,Riser=160./9;
 for(int Flight=0;Flight<2;++Flight){const double X0=Flight?Right0:Left0,X1=Flight?Right1:Left1;
  for(int I=0;I<9;++I){const double Y=Flight?382-(I+.5)*StepRun:130+(I+.5)*StepRun,Top=20+Flight*160+(I+1)*Riser;
   Box({(X0+X1)/2,Y,Top-2},{130,StepRun,4},Step,false,TEXT("FoundrylineOfficeStairTread"));
   const double Front=Flight?Y+StepRun/2:Y-StepRun/2;
   Box({(X0+X1)/2,Front,Top-Riser/2},{130,2,Riser},Paint,false,TEXT("FoundrylineOfficeStairRiser"));
   Box({(X0+X1)/2,Front+(Flight?-1.5:1.5),Top+.15},{130,3,.3},Rail,false,TEXT("FoundrylineOfficeStairNosing"));
  }
  const double LowY=Flight?382:130,HighY=Flight?130:382,LowZ=20+Flight*160,HighZ=LowZ+160;
  for(double X:{X0+14,X1-14})Beam({X,LowY,LowZ-12},{X,HighY,HighZ-12},12,Paint,false,TEXT("FoundrylineOfficeStairStringer"));
  const FVector UnderA((X0+X1)/2,LowY,LowZ-20),UnderB((X0+X1)/2,HighY,HighZ-20);Box((UnderA+UnderB)/2,{FVector::Distance(UnderA,UnderB),130,4},Paint,false,TEXT("FoundrylineOfficeStairSoffit"),(UnderB-UnderA).Rotation());
  for(double X:{X0-1.5,X1+1.5}){
   Panel({X,LowY,LowZ},{X,HighY,HighZ},15,100,Glass);Panel({X,LowY,LowZ},{X,HighY,HighZ},-25,2,Paint);
   Beam({X,LowY,LowZ+110},{X,HighY,HighZ+110},3,Rail,true,TEXT("FoundrylineOfficeStairHandrail"));
   Beam({X,LowY,LowZ+55},{X,HighY,HighZ+55},2,Rail,true,TEXT("FoundrylineOfficeStairGuard"));
   for(int I=0;I<=3;++I){const double T=I/3.;Post({X,FMath::Lerp(LowY,HighY,T),FMath::Lerp(LowZ,HighZ,T)},110);}
  }
  if(!Preview){
   // Explicit shape collision stays independent of the visible mesh and its collision data.
   const FVector A((X0+X1)/2,LowY,LowZ),B((X0+X1)/2,HighY,HighZ),D=B-A;
   const FRotator Rotation=D.Rotation();const FVector Normal=Rotation.RotateVector(FVector::UpVector);
   auto* C=NewObject<UBoxComponent>(GetOwner());C->SetupAttachment(this);C->SetRelativeLocationAndRotation((A+B)/2-Normal*5,Rotation);C->SetBoxExtent({(D.Size()+1.)/2,65,5});C->SetCollisionProfileName(TEXT("BlockAll"));C->SetCanEverAffectNavigation(false);Register(C,TEXT("FoundrylineOfficeStairRamp"));
   C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCastShadow(false);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));
  }
 }
 // Flush midlanding connects both flights; there is no collision across either entrance.
 Box({150,447,174},{300,130,12},Step,true,TEXT("FoundrylineOfficeStairLanding"));
 for(double X:{1.5,298.5}){Panel({X,382,180},{X,510.5,180},15,100,Glass);Beam({X,382,290},{X,510.5,290},3,Rail,true,TEXT("FoundrylineOfficeStairHandrail"));Post({X,510.5,180},110);}
 Panel({1.5,510.5,180},{298.5,510.5,180},15,100,Glass);
 Beam({1.5,510.5,290},{298.5,510.5,290},3,Rail,true,TEXT("FoundrylineOfficeStairHandrail"));Post({150,510.5,180},110);
 // Upper-hole perimeter. The front's return-flight exit X167..297 stays open.
 for(double X:{1.5,298.5}){Panel({X,382,180},{X,510.5,180},15,100,Glass);Beam({X,131.5,450},{X,510.5,450},3,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));Beam({X,131.5,395},{X,510.5,395},2,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));for(double Y:{131.5,258.,384.,510.5})Post({X,Y,340},110);}
 for(const auto& Ends:TArray<TPair<FVector,FVector>>{TPair<FVector,FVector>(FVector(1.5,510.5,450),FVector(298.5,510.5,450)),TPair<FVector,FVector>(FVector(1.5,131.5,450),FVector(165.5,131.5,450))}){Panel(Ends.Key-FVector(0,0,110),Ends.Value-FVector(0,0,110),15,100,Glass);Beam(Ends.Key,Ends.Value,3,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));Beam(Ends.Key-FVector(0,0,55),Ends.Value-FVector(0,0,55),2,Rail,true,TEXT("FoundrylineOfficeStairUpperGuard"));}
 Post({150,510.5,340},110);Post({165.5,131.5,340},110);
}
