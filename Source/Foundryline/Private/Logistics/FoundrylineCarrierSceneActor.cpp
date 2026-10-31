#include "Logistics/FoundrylineCarrierSceneActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "Production/FoundrylineWeldingStationLayout.h"
namespace {
// Presentation follows bounded, rounded routes; the server owns bay allocation and cargo.
TArray<FVector> Rounded(const TArray<FVector>& Corners,const TArray<float>& RadiusHints={}){
 TArray<FVector> Result;if(Corners.IsEmpty())return Result;Result.Add(Corners[0]);
 for(int I=1;I+1<Corners.Num();++I){const FVector In=(Corners[I]-Corners[I-1]).GetSafeNormal(),Out=(Corners[I+1]-Corners[I]).GetSafeNormal();const float Fraction=Corners.Num()==3?1.f:.4f;const float Hint=RadiusHints.IsValidIndex(I-1)?RadiusHints[I-1]:600.f;const float Radius=FMath::Min3(Hint,float(FVector::Distance(Corners[I],Corners[I-1]))*Fraction,float(FVector::Distance(Corners[I],Corners[I+1]))*Fraction);const FVector A=Corners[I]-In*Radius,B=Corners[I]+Out*Radius;Result.Add(A);for(int J=1;J<=12;++J){const float T=J/12.f;Result.Add(FMath::Lerp(FMath::Lerp(A,Corners[I],T),FMath::Lerp(Corners[I],B,T),T));}}
 Result.Add(Corners.Last());return Result;
}
FTransform Sample(const TArray<FVector>& P,double Alpha){
 float Length=0;for(int I=1;I<P.Num();++I)Length+=FVector::Distance(P[I-1],P[I]);float Left=FMath::Clamp(float(Alpha),0.f,1.f)*Length;
 for(int I=1;I<P.Num();++I){const float D=FVector::Distance(P[I-1],P[I]);if(Left<=D||I==P.Num()-1){const FVector Direction=P[I]-P[I-1];return FTransform(FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(Direction.Y,Direction.X))-90,0),FMath::Lerp(P[I-1],P[I],D>0?FMath::Clamp(Left/D,0.f,1.f):0));}Left-=D;}
 return FTransform(FVector(1260,1700,20));
}
const TArray<FVector>& Approach(){static const auto P=Rounded({{-6500,-3000,20},{4200,-3000,20},{4200,-1100,20}},{1250});return P;}
const TArray<FVector>& Docking(){static const auto P=Rounded({{4200,-1100,20},{4200,400,20},{1260,400,20},{1260,1700,20}});return P;}
const TArray<FVector>& Departure(){static const auto P=Rounded({{1260,1700,20},{1260,2800,20},{4200,2800,20},{4200,-2800,20},{9300,-2800,20}},{600,600,1700});return P;}
}
AFoundrylineCarrierSceneActor::AFoundrylineCarrierSceneActor(){bReplicates=true;SetNetCullDistanceSquared(FMath::Square(30000.f));SetReplicateMovement(true);SetNetUpdateFrequency(10);PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Carrier")));auto AddCollision=[&](FName Name,FVector At,FVector Extent){auto* C=CreateDefaultSubobject<UBoxComponent>(Name);C->SetupAttachment(RootComponent);C->SetRelativeLocation(At);C->SetBoxExtent(Extent);C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionObjectType(ECC_WorldDynamic);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));PawnCollision.Add(C);};AddCollision(TEXT("BedPawnCollision"),{0,0,110.41},{138.1,393.5,2.54});AddCollision(TEXT("ChassisPawnCollision"),{0,50,54},{90,360,35});AddCollision(TEXT("CabPawnCollision"),{0,536,205},{124,123,195});Visual=CreateDefaultSubobject<USceneComponent>(TEXT("Display"));Visual->SetupAttachment(RootComponent);for(auto C:PawnCollision)C->SetupAttachment(Visual);Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(Visual);Bed=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bed"));Bed->SetupAttachment(Visual);Gate=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LoadingSideGate"));Gate->SetupAttachment(Visual);Gate->SetCollisionEnabled(ECollisionEnabled::NoCollision);Gate->SetMobility(EComponentMobility::Movable);Proxy=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TemporaryCarrier"));Proxy->SetupAttachment(Visual);Pallets=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Pallets"));Pallets->SetupAttachment(Visual);Steel=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("VisibleParts"));Steel->SetupAttachment(Visual);for(auto* M:{Body.Get(),Bed.Get(),static_cast<UStaticMeshComponent*>(Proxy.Get()),static_cast<UStaticMeshComponent*>(Pallets.Get()),static_cast<UStaticMeshComponent*>(Steel.Get())}){M->SetMobility(EComponentMobility::Movable);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);}}
void AFoundrylineCarrierSceneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AFoundrylineCarrierSceneActor,Kind);DOREPLIFETIME(AFoundrylineCarrierSceneActor,Cargo);DOREPLIFETIME(AFoundrylineCarrierSceneActor,bDocked);DOREPLIFETIME(AFoundrylineCarrierSceneActor,BookingId);}
void AFoundrylineCarrierSceneActor::Rebuild(){
 for(auto Part:FlatbedParts)if(Part)Part->DestroyComponent();FlatbedParts.Reset();FlatbedTractor=nullptr;FlatbedTrailer=nullptr;bFlatbedReady=false;
 for(int I=0;I<PawnCollision.Num();++I){auto C=PawnCollision[I];if(Kind==1){const FVector P[]={{0,0,110.41},{0,50,54},{0,536,205}};const FVector E[]={{138.1,393.5,2.54},{90,360,35},{124,123,195}};C->SetRelativeLocation(P[I]);C->SetBoxExtent(E[I]);}else if(I<2){C->SetRelativeLocation(I?FVector(0,260,170):FVector(0,0,120));C->SetBoxExtent(I?FVector(120,90,135):FVector(120,210,10));}C->SetCollisionEnabled(Kind==1||I<2?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 for(auto W:Wheels)if(W)W->DestroyComponent();Wheels.Reset();for(auto P:CargoParts)if(P)P->DestroyComponent();CargoParts.Reset();
 if(Kind==4){Body->SetStaticMesh(nullptr);Bed->SetStaticMesh(nullptr);Gate->SetStaticMesh(nullptr);Proxy->ClearInstances();Pallets->ClearInstances();Steel->ClearInstances();for(auto C:PawnCollision)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);RebuildFlatbed();return;}
 auto* Library=LoadObject<UFoundrylineFreightVehicleLibrary>(nullptr,TEXT("/Game/Foundryline/Logistics/DA_FreightVehicles.DA_FreightVehicles"));const int Index=Library&&!Library->Bodies.IsEmpty()?FMath::Clamp(Kind,0,Library->Bodies.Num()-1):-1;auto* Mesh=Library&&Library->Bodies.IsValidIndex(Index)?Library->Bodies[Index].Get():nullptr;Body->SetStaticMesh(Mesh);Bed->SetStaticMesh(Library&&Library->Beds.IsValidIndex(Index)?Library->Beds[Index].Get():nullptr);if(Library){const FVector Offset=Library->BodyOffsets.IsValidIndex(Index)?Library->BodyOffsets[Index]:FVector::ZeroVector;const FRotator R(0,Library->YawOffsets.IsValidIndex(Index)?Library->YawOffsets[Index]:0,0);Body->SetRelativeLocationAndRotation(Offset,R);Bed->SetRelativeLocationAndRotation(Offset,R);}
 Proxy->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Proxy->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_Metal.M_Metal")));Proxy->ClearInstances();Proxy->SetVisibility(!Mesh);if(!Mesh){Proxy->AddInstance(FTransform(FQuat::Identity,FVector(0,0,120),FVector(2.4,4.2,.2)));Proxy->AddInstance(FTransform(FQuat::Identity,FVector(0,260,170),FVector(2.4,1.8,2.7)));for(float X:{-110.f,110.f})for(float Y:{-140.f,220.f})Proxy->AddInstance(FTransform(FQuat::Identity,FVector(X,Y,55),FVector(.35,.75,.75)));}
 Gate->SetStaticMesh(Kind==1?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Logistics/StarterHauler/SM_LoadingSideGate.SM_LoadingSideGate")):nullptr);Gate->SetRelativeLocation({-128,0,112.9513});Gate->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Logistics/StarterHauler/M_TruckCabPaint.M_TruckCabPaint")));
 if(Kind==1&&Library){for(int I=0;I<Library->StarterWheels.Num();++I){auto* W=NewObject<UStaticMeshComponent>(this);W->SetupAttachment(Visual);W->SetStaticMesh(Library->StarterWheels[I]);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetRelativeLocation(Library->StarterWheelPositions.IsValidIndex(I)?Library->StarterWheelPositions[I]:FVector::ZeroVector);W->RegisterComponent();AddInstanceComponent(W);Wheels.Add(W);}}
 Pallets->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood.SM_Pallet_Wood")));Pallets->ClearInstances();Steel->ClearInstances();
 int I=0;for(const auto& P:Cargo){if(I>=2)break;
 // A pallet of headache racks fills both deck positions: one pallet in the middle of the deck, the flat stack centred on it
 // and lying across the deck (6 ft 6 in of the 8 ft deck width).
 if(P.Part==uint8(EFoundrylinePart::HeadacheRack)){I=2;const FVector Mid(-60,0,113);Pallets->AddInstance(FTransform(FRotator(0,90,0),Mid));auto* C=NewObject<UInstancedStaticMeshComponent>(this);C->SetupAttachment(Visual);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart::HeadacheRack)));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);CargoParts.Add(C);
  for(int Level=0;Level<P.Quantity;++Level)C->AddInstance(FFoundrylineWeldingStationLayout::RackOnPallet(Level)*FTransform(FVector(-FFoundrylineWeldingStationLayout::RackStackX(),0,0))*FTransform(FRotator(0,90,0),Mid));continue;}
 const bool Sheet=P.Part==0;const FVector At(-60,I?65:-65,113);++I;if(Sheet){Pallets->AddInstance(FTransform(At+FVector(0,-76.2,0)));Pallets->AddInstance(FTransform(At+FVector(0,76.2,0)));}else Pallets->AddInstance(FTransform(At));if(P.Part>=FFoundrylineFactoryCatalog::PartCount)continue;auto* C=NewObject<UInstancedStaticMeshComponent>(this);C->SetupAttachment(Visual);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart(P.Part))));
C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);CargoParts.Add(C);const FBox Bounds=C->GetStaticMesh()?C->GetStaticMesh()->GetBoundingBox():FBox(FVector(-10,-10,0),FVector(10,10,1));const FVector Size=Bounds.GetSize();const int Columns=Sheet?1:FMath::Clamp(FMath::FloorToInt(100.f/FMath::Max(1.,Size.X+4)),1,3),Rows=Sheet?1:FMath::Clamp(FMath::FloorToInt(100.f/FMath::Max(1.,Size.Y+4)),1,3);const int PerLayer=Columns*Rows;for(int J=0;J<FMath::Min(Sheet?3:10,P.Quantity);++J){const int Cell=J%PerLayer;FVector Offset=Sheet?FVector::ZeroVector:FVector((Cell%Columns-(Columns-1)*.5)*(Size.X+4),(Cell/Columns-(Rows-1)*.5)*(Size.Y+4),0);Offset.Z=17.7-Bounds.Min.Z+(J/PerLayer)*(Size.Z+.2);C->AddInstance(FTransform(FRotator::ZeroRotator,At+Offset));}}

}
void AFoundrylineCarrierSceneActor::SetBooking(const FFoundrylineFreightBooking& B,const FFoundrylineCommerceState& S,const FTransform& Plant,const TArray<FFoundrylineFactoryPallet>& Stock){Kind=B.Carrier;TArray<FFoundrylineFreightItem> New;int DeckSlot=0;for(const auto& Item:B.Manifest){const auto* P=Stock.FindByPredicate([&](const auto& V){return V.Id==Item.PalletId&&V.BookingId==B.Id&&(V.Place==EFoundrylinePalletPlace::Inbound||V.Place==EFoundrylinePalletPlace::Outbound||V.Place==EFoundrylinePalletPlace::InTransit);});if(P)New.Add({P->Id,uint8(P->Part),P->Quantity,P->UnitLb,DeckSlot});DeckSlot+=Item.Part==uint8(EFoundrylinePart::HeadacheRack)?2:1;}FString Key=FString::FromInt(Kind);for(const auto& P:New)Key+=FString::Printf(TEXT("/%d:%d:%d"),P.PalletId,P.Part,P.Quantity);if(CargoKey!=Key){CargoKey=Key;Cargo=New;Rebuild();ForceNetUpdate();}
 BookingId=B.Id;bDocked=B.Status==EFoundrylineBookingState::Loading&&S.Clock>=B.DockedAt;
 FTransform Pose(FVector(1260,1700,20));
 int QueueRank=0;for(const auto& O:S.Bookings)if((O.Status==EFoundrylineBookingState::Approaching||O.Status==EFoundrylineBookingState::Waiting)&&(O.ArrivalAt<B.ArrivalAt||O.ArrivalAt==B.ArrivalAt&&O.Id<B.Id))++QueueRank;
 const FVector QueueAt=QueueRank<2?FVector(4200,-1100-QueueRank*1250,20):FVector(4200-(QueueRank-1)*1250,-3000,20);
 if(B.Status==EFoundrylineBookingState::Approaching){const double Alpha=(S.Clock-B.DispatchAt)/FMath::Max(1.,B.ArrivalAt-B.DispatchAt);if(!QueueRank)Pose=Sample(Approach(),Alpha);else if(QueueRank==1)Pose=Sample(Rounded({{-6500,-3000,20},{4200,-3000,20},QueueAt}),Alpha);else Pose=Sample({{-6500,-3000,20},QueueAt},Alpha);}
 else if(B.Status==EFoundrylineBookingState::Waiting)Pose=FTransform(FRotator(0,QueueRank<2?0:-90,0),QueueAt);
 else if(B.Status==EFoundrylineBookingState::Loading)Pose=Sample(Docking(),(S.Clock-B.BerthAt)/FMath::Max(1.,B.DockedAt-B.BerthAt));
 else if(B.Status==EFoundrylineBookingState::Departing){
  Pose=Sample(Departure(),(S.Clock-B.DepartedAt)/20.);FVector At=Pose.GetLocation();
  // A rigid truck uses the full junction while turning, then returns to its
  // eastbound lane once straight. This keeps front tyres off the south verge.
  if(At.X>=6500&&At.Y<=-2799){const double U=FMath::Clamp((At.X-6500.)/2200.,0.,1.);At.Y=-2800.-200.*U*U*(3.-2.*U);const double Slope=-1200.*U*(1.-U)/2200.;Pose.SetLocation(At);Pose.SetRotation(FQuat(FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(Slope,1.))-90.,0)));}
 }
 else if(B.Status==EFoundrylineBookingState::InTransit){SetActorHiddenInGame(true);return;}
 SetActorLocationAndRotation(Plant.TransformPosition(Pose.GetLocation()),Plant.GetRotation()*Pose.GetRotation());
}
void AFoundrylineCarrierSceneActor::Tick(float Dt){
 Super::Tick(Dt);SetActorEnableCollision(!IsHidden());if(LastKind!=Kind){LastKind=Kind;Rebuild();}
 if(LastBooking!=BookingId){LastBooking=BookingId;bDisplayReady=false;WheelAngle=0;GateAngle=0;SteeringAngle=0;}
 if(!bDisplayReady){Display=GetActorLocation();DisplayRotation=GetActorRotation();bDisplayReady=true;}
 const FVector Previous=Display;const FRotator PreviousRotation=DisplayRotation;Display=FMath::VInterpTo(Display,GetActorLocation(),Dt,6);const float Distance=float(FVector::Distance(Previous,Display));const FVector Forward=DisplayRotation.RotateVector(FVector::RightVector);const float Direction=FVector::DotProduct(Display-Previous,Forward)>=0?1.f:-1.f;WheelAngle+=Distance/47.35f*Direction;DisplayRotation=FMath::RInterpTo(DisplayRotation,GetActorRotation(),Dt,6);Visual->SetWorldLocationAndRotation(Display,DisplayRotation);
 if(Kind==4){TickFlatbed(Dt,Distance);return;}
 // The front axle steers from displayed curvature; rear axles remain fixed.
 // Authored front-to-rear axle midpoint is 555 cm. Visual steering is bounded
 // independently from authoritative booking/path state, as on the tanker rig.
 const float YawChange=FMath::DegreesToRadians(float(FMath::FindDeltaAngleDegrees(PreviousRotation.Yaw,DisplayRotation.Yaw)));
 const float TargetSteering=Distance>.5f?FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(555.f*YawChange/Distance*Direction)),-35.f,35.f):0;
 SteeringAngle=FMath::FInterpTo(SteeringAngle,TargetSteering,Dt,8);
 for(int I=0;I<Wheels.Num();++I)if(Wheels[I])Wheels[I]->SetRelativeRotation(FQuat(FRotator(0,90+(I<2?SteeringAngle:0),0))*FQuat(FVector::RightVector,WheelAngle));
 GateAngle=FMath::FInterpConstantTo(GateAngle,bDocked?PI:0,Dt,PI);Gate->SetRelativeRotation(FQuat(FRotator(0,90,0))*FQuat(FVector::ForwardVector,GateAngle));
}
