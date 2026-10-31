#include "Logistics/FoundrylineCarrierSceneActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Production/FoundrylineWeldingStationLayout.h"

void AFoundrylineCarrierSceneActor::RebuildFlatbed()
{
 auto Frame=[&](FName Name,FVector At){auto* C=NewObject<USceneComponent>(this,Name);C->SetupAttachment(Visual);C->SetRelativeLocationAndRotation(At,FRotator(0,90,0));C->RegisterComponent();AddInstanceComponent(C);FlatbedParts.Add(C);return C;};
 FlatbedTractor=Frame(TEXT("FlatbedTractor"),{0,1358,0});FlatbedTrailer=Frame(TEXT("FlatbedTrailer"),{0,1400,0});
 auto Mesh=[&](USceneComponent* Parent,const FString& Folder,const FString& Name,FVector At){auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/%s/SM_%s.SM_%s"),*Folder,*Name,*Name)));C->SetRelativeLocation(At);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);FlatbedParts.Add(C);return C;};
 Mesh(FlatbedTractor,TEXT("Vehicles/Semi/Megapoly"),TEXT("Tractor_Body"),FVector::ZeroVector);
 Mesh(FlatbedTrailer,TEXT("Vehicles/FlatbedTrailer"),TEXT("Trailer_Body"),FVector::ZeroVector);
 const FVector TractorWheels[]={{.00926,95.22259,45.53076},{.00921,-95.59264,45.53089},{367.83292,114.05562,45.51796},{367.83297,-114.08374,45.5181}};
 const FVector TrailerWheels[]={{-1095,-98,47.352},{-1095,98,47.352},{-1400,-98,47.352},{-1400,98,47.352}};
 for(int I=0;I<4;++I)Wheels.Add(Mesh(FlatbedTractor,TEXT("Vehicles/Semi/Megapoly"),FString::Printf(TEXT("Tractor_Wheel_%d"),I),TractorWheels[I]));
 for(int I=0;I<4;++I)Wheels.Add(Mesh(FlatbedTrailer,TEXT("Vehicles/FlatbedTrailer"),FString::Printf(TEXT("Trailer_Wheel_%d"),I),TrailerWheels[I]));
 for(int I=0;I<2;++I)Mesh(FlatbedTrailer,TEXT("Vehicles/FlatbedTrailer"),FString::Printf(TEXT("Trailer_Leg_%d"),I),{-360,I?58.:-58.,60});
 auto Block=[&](USceneComponent* Parent,FVector Center,FVector Half){auto* C=NewObject<UBoxComponent>(this);C->SetupAttachment(Parent);C->SetRelativeLocation(Center);C->SetBoxExtent(Half);C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionObjectType(ECC_WorldDynamic);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->RegisterComponent();AddInstanceComponent(C);FlatbedParts.Add(C);};
 Block(FlatbedTractor,{210,0,210},{300,130,185});Block(FlatbedTrailer,{-716.3,0,139},{807.7,129.5,8});
 auto* PalletMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Inventory/Storage/Pallets/SM_Pallet_Wood.SM_Pallet_Wood"));
 auto Instances=[&](UStaticMesh* M){auto* C=NewObject<UInstancedStaticMeshComponent>(this);C->SetupAttachment(FlatbedTrailer);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();AddInstanceComponent(C);FlatbedParts.Add(C);return C;};
 auto* DeckPallets=Instances(PalletMesh);int Slot=0;
 // Wide products occupy a full row. Ordinary pallets fill two columns, thirteen rows.
 TArray<FFoundrylineFreightItem> Packed=Cargo;Packed.StableSort([](const auto& A,const auto& B){return (A.Part==uint8(EFoundrylinePart::HeadacheRack))>(B.Part==uint8(EFoundrylinePart::HeadacheRack));});
 for(const auto& P:Packed){Slot=P.DeckSlot;const bool Wide=P.Part==uint8(EFoundrylinePart::HeadacheRack);if(Wide&&Slot%2)++Slot;if(Slot+(Wide?2:1)>26)break;
  const FVector At(-1460.4+(Slot/2)*122,Wide?0:Slot%2?64:-64,147);Slot+=Wide?2:1;DeckPallets->AddInstance(FTransform(At));
  if(P.Part>=FFoundrylineFactoryCatalog::PartCount)continue;
  auto* PartMesh=LoadObject<UStaticMesh>(nullptr,*FFoundrylineFactoryCatalog::PartMesh(EFoundrylinePart(P.Part)));if(!PartMesh)continue;auto* Parts=Instances(PartMesh);
  if(Wide){for(int Level=0;Level<FMath::Min(P.Quantity,4);++Level)Parts->AddInstance(FFoundrylineWeldingStationLayout::RackOnPallet(Level)*FTransform(FVector(-FFoundrylineWeldingStationLayout::RackStackX(),0,0))*FTransform(FRotator(0,90,0),At));continue;}
  const FBox B=PartMesh->GetBoundingBox();const FVector Size=B.GetSize();const int Columns=FMath::Clamp(FMath::FloorToInt(100./FMath::Max(1.,Size.X+4)),1,3),Rows=FMath::Clamp(FMath::FloorToInt(100./FMath::Max(1.,Size.Y+4)),1,3);
  for(int I=0;I<FMath::Min(P.Quantity,10);++I){const int Cell=I%(Columns*Rows);const FVector Offset((Cell%Columns-(Columns-1)*.5)*(Size.X+4),(Cell/Columns-(Rows-1)*.5)*(Size.Y+4),17.7-B.Min.Z+I/(Columns*Rows)*(Size.Z+.2));Parts->AddInstance(FTransform(At+Offset));}
 }
}

void AFoundrylineCarrierSceneActor::TickFlatbed(float Dt,float Distance)
{
 if(!FlatbedTractor||!FlatbedTrailer)return;
 const FVector Hitch=FlatbedTractor->GetComponentTransform().TransformPosition({42,0,0});const float TractorYaw=FlatbedTractor->GetComponentRotation().Yaw;
 if(!bFlatbedReady||FVector::DistSquared(Hitch,PreviousHitch)>FMath::Square(500.f)){TrailerYaw=TractorYaw;bFlatbedReady=true;}
 else {const FVector Delta=Hitch-PreviousHitch;const FVector Right=FRotator(0,TrailerYaw,0).RotateVector(FVector::RightVector);TrailerYaw+=FMath::RadiansToDegrees(float(FVector::DotProduct(Delta,Right))/1247.5f);TrailerYaw=TractorYaw+FMath::Clamp(FMath::FindDeltaAngleDegrees(TractorYaw,TrailerYaw),-70.f,70.f);}
 if(bDocked)TrailerYaw=FMath::FInterpConstantTo(TrailerYaw,TractorYaw,Dt,35.f);
 PreviousHitch=Hitch;FlatbedTrailer->SetWorldLocationAndRotation(Hitch,FRotator(0,TrailerYaw,0));
 const float Turn=Distance>.5f?FMath::Clamp(FMath::FindDeltaAngleDegrees(TrailerYaw,TractorYaw)*.6f,-35.f,35.f):0;SteeringAngle=FMath::FInterpTo(SteeringAngle,Turn,Dt,8);
 for(int I=0;I<Wheels.Num();++I)if(Wheels[I])Wheels[I]->SetRelativeRotation(FQuat(FRotator(0,I==2||I==3?SteeringAngle:0,0))*FQuat(FVector::RightVector,-WheelAngle));
}

bool AFoundrylineCarrierSceneActor::ConfigureFlatbedReview(bool Parked)
{
#if WITH_EDITOR
 if(!GetWorld()||(GetWorld()->WorldType!=EWorldType::Editor&&GetWorld()->WorldType!=EWorldType::PIE)||!HasAuthority())return false;
 Kind=4;bDocked=Parked;Cargo={{90001,uint8(EFoundrylinePart::Bracket),40,5,0}};Rebuild();ForceNetUpdate();return true;
#else
 return false;
#endif
}
