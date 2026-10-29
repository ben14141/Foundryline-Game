#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWeldingStationLayout.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SplineMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Production/FoundrylineWeldingEffectComponent.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
// Bench and fixture kit are the original models from main (Docs/Architecture/ORIGINAL_MODEL_ASSETS.md), both on floor
// datums; a checkout without Content falls back to a plain bench block.
void AFoundrylineFactoryPlant::RebuildWeldingStations(){
 using Layout=FFoundrylineWeldingStationLayout;
 FString Key;for(const auto& S:Snapshot.WeldingStations)Key+=FString::Printf(TEXT("%d:%s:%d:%d;"),S.Id,*S.Center.ToString(),S.QuarterTurns,S.bSwapSides?1:0);
 if(Key==StationLayoutKey)return;StationLayoutKey=Key;
 for(UStaticMeshComponent* C:StationMeshes)if(C)C->DestroyComponent();StationMeshes.Reset();for(UBoxComponent* C:StationCollision)if(C)C->DestroyComponent();StationCollision.Reset();
 for(USceneComponent* C:StationUnitParts)if(C)C->DestroyComponent();StationUnitParts.Reset();StationStoredFixtures.Reset();StationJigParts.Reset();StationRackGhosts.Reset();StationRackBuilds.Reset();StationRacks.Reset();
 StationHolsteredGuns.Reset();StationGuns.Reset();StationLeads.Reset();StationWeldEffects.Reset();StationShownStage.Reset();
 if(Snapshot.WeldingStations.IsEmpty())return;
 const bool Render=GetNetMode()!=NM_DedicatedServer;
 auto* Bench=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_StandardWeldingBench.SM_StandardWeldingBench")):nullptr;
 auto* Fixture=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureKit.SM_WeldingFixtureKit")):nullptr;
 auto* Cube=Render&&!Bench?LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")):nullptr;
 auto* Paint=Cube?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_FoundryOrange.M_MH_FoundryOrange")):nullptr;
 for(const auto& S:Snapshot.WeldingStations){const FTransform T=Layout::BenchTransform(S);
  // Model-aligned body over the 8 x 4 ft top; worker clearances and both ports stay open.
  auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(FVector(4*Layout::Foot,2*Layout::Foot,Layout::BenchHeightCm/2));Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);
  Box->SetRelativeLocationAndRotation(T.GetLocation()+FVector(0,0,Layout::BenchHeightCm/2),T.Rotator());StationCollision.Add(Box);
  if(!Render)continue;
  auto Mesh=[&](UStaticMesh* M,const FTransform& At)->UStaticMeshComponent*{auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetRelativeTransform(At);StationMeshes.Add(C);return C;};
  if(Bench)Mesh(Bench,T);else if(Cube){auto* C=Mesh(Cube,FTransform(T.Rotator(),T.GetLocation()+FVector(0,0,Layout::BenchHeightCm/2),FVector(8*Layout::Foot,4*Layout::Foot,Layout::BenchHeightCm)/100));if(Paint)C->SetMaterial(0,Paint);}
  // The fixture kit is authored in its stored pose on the bench's lower shelf, so it shares the bench transform.
  StationStoredFixtures.Add(Fixture?Mesh(Fixture,T):nullptr);
 }
 // One welding machine per welder side, each with its own holstered gun and coiled leads (same offsets as zone welders).
 auto* Machine=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Machine.SM_Welder_Machine")):nullptr;
 auto* Gun=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_MIGGun.SM_Welder_MIGGun")):nullptr;
 auto* Leads=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/Welder/SM_Welder_Leads.SM_Welder_Leads")):nullptr;
 for(const auto& S:Snapshot.WeldingStations)for(int Side=0;Side<2;++Side){const FTransform T=Layout::MachineTransform(S,Side);const FVector Size=Layout::MachineSizeCm();
  auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(Size/2);Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);
  Box->SetRelativeLocationAndRotation(T.GetLocation()+FVector(0,0,Size.Z/2),T.Rotator());StationCollision.Add(Box);
  if(!Render)continue;
  auto Mesh=[&](UStaticMesh* M,const FTransform& At)->UStaticMeshComponent*{if(!M)return nullptr;auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetRelativeTransform(At);StationMeshes.Add(C);return C;};
  Mesh(Machine,T);Mesh(Leads,T);auto* Holstered=Mesh(Gun,FTransform(T.Rotator(),T.TransformPosition(FVector(17,-38,87))));if(!Side)StationHolsteredGuns.Add(Holstered);
 }
 // Parts bay rack beside IN (frame cradle below, three bins on the shelf) and the steel OUT cradle, built from painted blocks.
 // SM_PalletRack_1Level is a 10 ft x 4 ft 10 in floor pallet bay with no shelf, so it cannot stand in the 4.5 x 2.5 ft parts bay.
 auto* Block=Render?LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")):nullptr;
 auto* Orange=Render?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_FoundryOrange.M_MH_FoundryOrange")):nullptr;
 auto* Steel=Render?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_Metal.M_Metal")):nullptr;
 for(const auto& S:Snapshot.WeldingStations){const FRotator Yaw=Layout::Rotation(S);
  auto Collide=[&](FVector Center,FVector Extent,FRotator Rot){auto* Box=NewObject<UBoxComponent>(this);Box->SetupAttachment(RootComponent);Box->SetBoxExtent(Extent);Box->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();AddInstanceComponent(Box);Box->SetRelativeLocationAndRotation(Center,Rot);StationCollision.Add(Box);};
  const FBox2D Bay=Layout::PartsBay(S);const FVector BayCenter(Bay.GetCenter(),20);

  Collide(BayCenter+FVector(0,0,50),FVector(Bay.GetSize()/2,50),FRotator::ZeroRotator);
  const FTransform Cradle=Layout::CradleTransform(S);const FVector C=Layout::CradleSizeCm();Collide(Cradle.GetLocation()+FVector(0,0,C.Z/2),C/2,Cradle.Rotator());
  if(!Block)continue;
  // Box in a local frame: centre and size in cm, turned with that frame.
  auto Part=[&](const FTransform& Frame,FVector Center,FVector Size,UMaterialInterface* M){auto* Mc=NewObject<UStaticMeshComponent>(this);Mc->SetupAttachment(RootComponent);Mc->SetStaticMesh(Block);Mc->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mc->SetCanEverAffectNavigation(false);Mc->RegisterComponent();AddInstanceComponent(Mc);Mc->SetRelativeTransform(FTransform(Frame.Rotator(),Frame.TransformPosition(Center),Size/100));if(M)Mc->SetMaterial(0,M);StationMeshes.Add(Mc);};
  const FTransform BayFrame(Yaw,BayCenter);const double HX=Layout::Foot*4.5/2-4,HY=Layout::Foot*2.5/2-4,Shelf=Layout::BinShelfCm;
  for(int Corner=0;Corner<4;++Corner)Part(BayFrame,FVector(Corner&1?HX:-HX,Corner&2?HY:-HY,52),FVector(5,5,104),Orange);
  Part(BayFrame,FVector(0,0,Shelf-1.5),FVector(2*HX+5,2*HY+5,3),Steel);
  for(double X:{-21.,21.})Part(BayFrame,FVector(X,0,Shelf+6),FVector(1,2*HY,12),Steel);
  for(double X:{-45.,45.})Part(BayFrame,FVector(X,0,6),FVector(8,2*HY,12),Orange);
  for(double Y:{-40.,40.})Part(Cradle,FVector(0,Y,C.Z/2),FVector(C.X,10,C.Z),Orange);
  for(double X:{-100.,0.,100.})Part(Cradle,FVector(X,0,5),FVector(10,C.Y,10),Steel);
 }
 if(!Render)return;
 // The unit on the bench, Welder A's gun in hand with its lead back to their own machine, and the sparks at the tip.
 auto* Rack=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Products/HeadacheRack/SM_HeadacheRack.SM_HeadacheRack"));
 auto* Riser=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureRiser.SM_WeldingFixtureRiser"));
 auto* HoldDown=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureHoldDown.SM_WeldingFixtureHoldDown"));
 auto* Hologram=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryHologram.M_FactoryHologram"));
 auto* Cable=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));auto* Rubber=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_Rubber.M_MH_Rubber"));
 for(const auto& S:Snapshot.WeldingStations){const FTransform BenchAt=Layout::BenchTransform(S),On=Layout::ProductTransform(S);
  auto Add=[&](UStaticMesh* M,const FTransform& At)->UStaticMeshComponent*{if(!M)return nullptr;auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();AddInstanceComponent(C);C->SetRelativeTransform(At);C->SetVisibility(false);StationUnitParts.Add(C);return C;};
  // The kit's risers and hold-downs set out as the rack jig; the stored kit on the shelf hides while they are in use.
  for(const auto& At:Layout::JigRisers())StationJigParts.Add(Add(Riser,At*BenchAt));for(const auto& At:Layout::JigHoldDowns())StationJigParts.Add(Add(HoldDown,At*BenchAt));
  // Ghost and weld-progress copies sweep along the rack's length, the way the welder works along it.
  auto Progress=[&](UStaticMeshComponent* C,bool Ghost){if(!C)return C;for(int Slot=0;Slot<C->GetNumMaterials();++Slot){
    auto* Base=Ghost?Hologram:LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Foundryline/Production/Materials/M_FactoryBuild"))+RackBuildSuffix(C->GetMaterial(Slot))));if(!Base)continue;
    C->SetMaterial(Slot,RackProgressMaterial(Base,On));}return C;};
  StationRackGhosts.Add(Progress(Add(Rack,On),true));StationRackBuilds.Add(Progress(Add(Rack,On),false));StationRacks.Add(Add(Rack,On));
  const FRotator GunRotation(-30,Layout::WorkYaw(S,0),0);auto* InHand=Add(Gun,FTransform(GunRotation,On.GetLocation()));StationGuns.Add(InHand);
  if(InHand){auto* Effect=NewObject<UFoundrylineWeldingEffectComponent>(this);Effect->SetIsReplicated(false);Effect->SetLocalActivity(false);Effect->SetupAttachment(InHand,TEXT("WeldTip"));Effect->RegisterComponent();AddInstanceComponent(Effect);StationUnitParts.Add(Effect);StationWeldEffects.Add(Effect);}else StationWeldEffects.Add(nullptr);
  // Lead: machine A's output terminal, down to the floor, along to the welder's feet, up to the gun (the end follows the gun).
  const FTransform MachineA=Layout::MachineTransform(S,0);const FVector Work=Layout::WorkPoint(S,0),ToMachine=(MachineA.GetLocation()-Work).GetSafeNormal2D();
  const FVector Points[]={MachineA.TransformPosition({54,-12,53}),MachineA.TransformPosition({80,-12,4}),Work+ToMachine*25+FVector(0,0,4),Work+FVector(0,0,85),On.GetLocation()};
  for(int I=0;I<4;++I){auto* Lead=NewObject<USplineMeshComponent>(this);Lead->SetMobility(EComponentMobility::Movable);Lead->SetupAttachment(RootComponent);Lead->SetStaticMesh(Cable);Lead->SetForwardAxis(ESplineMeshAxis::Z,false);Lead->SetStartScale(FVector2D(.024f));Lead->SetEndScale(FVector2D(.024f));
   Lead->SetStartAndEnd(Points[I],Points[I+1]-Points[I],Points[I+1],Points[I+1]-Points[I]);Lead->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lead->SetCanEverAffectNavigation(false);if(Rubber)Lead->SetMaterial(0,Rubber);Lead->RegisterComponent();AddInstanceComponent(Lead);Lead->SetVisibility(false);StationUnitParts.Add(Lead);StationLeads.Add(Lead);}
  StationShownStage.Add(-1);
 }
}
// Build-progress material for one rack slot: lights in white or red, steel in metal, sweeping along the rack's length.
FString AFoundrylineFactoryPlant::RackBuildSuffix(const UMaterialInterface* Material){
 const FString N=Material?Material->GetName():FString();return N.Contains(TEXT("Amber"))?TEXT("Red"):N.Contains(TEXT("LED"))||N.Contains(TEXT("Lens"))?TEXT("White"):TEXT("Metal");
}
bool AFoundrylineFactoryPlant::IsRackLight(const UMaterialInterface* Material){return RackBuildSuffix(Material)!=TEXT("Metal");}
UMaterialInstanceDynamic* AFoundrylineFactoryPlant::RackProgressMaterial(UMaterialInterface* Base,const FTransform& On){
 auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("BuildOrigin"),FLinearColor(On.GetLocation()));M->SetVectorParameterValue(TEXT("BuildAxis"),FLinearColor(On.GetUnitAxis(EAxis::X)));
 M->SetScalarParameterValue(TEXT("BuildLow"),-100);M->SetScalarParameterValue(TEXT("BuildHigh"),100);return M;
}
// Presentation follows the replicated unit stage and progress, extrapolated between snapshots only while work advances.
void AFoundrylineFactoryPlant::AnimateWeldingStations(float Dt,double Age){
 using Layout=FFoundrylineWeldingStationLayout;
 for(int I=0;I<Snapshot.WeldingStations.Num()&&I<StationRacks.Num()&&I<StationGuns.Num();++I){const auto& S=Snapshot.WeldingStations[I];const int Stage=FMath::Clamp(S.Stage,0,4);
  const bool Advancing=Snapshot.Running&&(S.bAdvancing||Stage==4);const float Seconds=Layout::StageSeconds(Stage);
  const float Work=S.Work+(Advancing?float(FMath::Clamp(Age,0.,.5))*Snapshot.Speed:0.f),F=Seconds>0?FMath::Clamp(Work/Seconds,0.f,1.f):0.f;
  const float Shown=F<=0?-1:F>=1?2:F;
  auto Show=[](UStaticMeshComponent* C,bool V){if(C&&C->IsVisible()!=V)C->SetVisibility(V);};
  auto Fraction=[](UStaticMeshComponent* C,float V){if(!C)return;for(int Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(Slot)))M->SetScalarParameterValue(TEXT("BuildFraction"),V);};
  const bool OnJig=Stage>=1&&Stage<=3,HasJig=StationJigParts.IsValidIndex(I*8)&&StationJigParts[I*8];
  if(StationStoredFixtures.IsValidIndex(I))Show(StationStoredFixtures[I],!OnJig||!HasJig);for(int J=I*8;J<I*8+8&&J<StationJigParts.Num();++J)Show(StationJigParts[J],OnJig);
  // Fit-up: the real kit parts are laid out first (RebuildVisuals), then the rack takes shape on the fixture.
  Show(StationRackGhosts[I],Stage==1&&F>=.6f||Stage==2);Show(StationRackBuilds[I],Stage==2);Show(StationRacks[I],Stage>=3);
  if(Stage==1)Fraction(StationRackGhosts[I],-1);if(Stage==2){Fraction(StationRackGhosts[I],Shown);Fraction(StationRackBuilds[I],Shown);}
  if(auto* R=StationRacks[I].Get()){const FTransform A=Layout::ProductTransform(S),B=Layout::ProductOnCradle()*Layout::CradleTransform(S);
   // Welded steel in its own materials; the lights appear as they are installed.
   if(StationShownStage[I]!=Stage){R->EmptyOverrideMaterials();if(Stage==3)for(int Slot=0;Slot<R->GetNumMaterials();++Slot)if(IsRackLight(R->GetMaterial(Slot)))
     if(auto* Base=LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Foundryline/Production/Materials/M_FactoryBuild"))+RackBuildSuffix(R->GetMaterial(Slot)))))R->SetMaterial(Slot,RackProgressMaterial(Base,A));}
   if(Stage==3)Fraction(R,Shown);
   // A controlled lift to the OUT cradle: straight up clear of welding machine B, across while turning, then down onto the
   // runners. The OUT product replaces it when it lands.
   if(Stage==4){const float Up=FMath::SmoothStep(0.f,.25f,F),Across=FMath::SmoothStep(.25f,.75f,F),Down=FMath::SmoothStep(.75f,1.f,F);
    const double Top=A.GetLocation().Z+60;const FVector XY=FMath::Lerp(A.GetLocation(),B.GetLocation(),Across);
    R->SetRelativeLocationAndRotation(FVector(XY.X,XY.Y,Down>0?FMath::Lerp(Top,B.GetLocation().Z,Down):FMath::Lerp(A.GetLocation().Z,Top,Up)),FQuat::Slerp(A.GetRotation(),B.GetRotation(),Across));}
   else if(StationShownStage[I]!=Stage)R->SetRelativeTransform(A);}
  StationShownStage[I]=Stage;
  // Welder A holds their own gun only while welding at the work point; sparks only while the weld advances.
  const FVector WorkAt=Layout::WorkPoint(S,0);const bool AtWork=Snapshot.Walkers.ContainsByPredicate([&](const auto& W){return W.Station==S.Id&&FVector::Dist2D(W.Position,WorkAt)<=1;});const bool InHand=Stage==2&&AtWork;
  if(StationHolsteredGuns.IsValidIndex(I))Show(StationHolsteredGuns[I],!InHand);auto* G=StationGuns[I].Get();Show(G,InHand);
  for(int J=I*4;J<I*4+4&&J<StationLeads.Num();++J)if(StationLeads[J]&&StationLeads[J]->IsVisible()!=InHand)StationLeads[J]->SetVisibility(InHand);
  if(StationWeldEffects.IsValidIndex(I)&&StationWeldEffects[I])StationWeldEffects[I]->SetLocalActivity(InHand&&Snapshot.Running&&S.bAdvancing);
  if(!G||!InHand)continue;
  const FTransform Bench=Layout::BenchTransform(S);const FVector Along=Bench.GetUnitAxis(EAxis::X),Toward=(WorkAt-Bench.GetLocation()).GetSafeNormal2D();
  const FVector Seam=Bench.GetLocation()+FVector(0,0,Layout::RackOnBenchCm().Max.Z-8)+Toward*30+Along*(55*FMath::Sin(Work*.18f))+FVector(0,0,1.5f*FMath::Sin(Work*2.6f));
  const FVector Socket=G->GetSocketTransform(TEXT("WeldTip"),RTS_Component).GetLocation();const FVector Target=Seam-G->GetRelativeRotation().RotateVector(Socket);
  const FVector Position=FMath::VInterpTo(G->GetRelativeLocation(),Target,Dt,18);G->SetRelativeLocation(Position);if(StationLeads.IsValidIndex(I*4+3))StationLeads[I*4+3]->SetEndPosition(Position);
  for(const auto& Employee:CachedEmployees)if(Employee.Role==EFoundrylineEmployeeRole::Welder&&Employee.Assignment==S.Id)if(auto* Visual=EmployeeVisuals.FindRef(Employee.Key).Get())Visual->PresentWeldingGrip(G->GetComponentLocation(),G->GetComponentTransform().TransformPosition(FVector(3,-3,8)));
 }
}
// Fit-up's first part: the kit's frame sections, plates and brackets laid out on the worktop beside the fixture, one by one.
void AFoundrylineFactoryPlant::AddStationFitUpParts(){
 using Layout=FFoundrylineWeldingStationLayout;using Part=EFoundrylinePart;
 for(const auto& S:Snapshot.WeldingStations){if(S.Stage!=1)continue;const float F=S.Work/Layout::StageSeconds(1);if(F>=.6f)continue;
  const int Shown=FMath::Clamp(FMath::CeilToInt(F/.6f*12),1,12);int N=0;const FTransform Bench=Layout::BenchTransform(S);
  auto Lay=[&](Part P,FVector Local,double MaxLength){if(N++>=Shown||!PartMeshes.IsValidIndex(int(P)))return;auto* Ism=PartMeshes[int(P)].Get();if(!Ism||!Ism->GetStaticMesh())return;
   const FBox B=Ism->GetStaticMesh()->GetBoundingBox();const bool Turn=B.GetSize().Y>B.GetSize().X;const float Scale=FMath::Min(1.f,float(MaxLength/FMath::Max(1.,B.GetSize().GetMax())));
   const FRotator Rot(0,Bench.Rotator().Yaw+(Turn?90:0),0);Ism->AddInstance(FTransform(Rot,Bench.TransformPosition(Local)-FVector(0,0,B.Min.Z*Scale),FVector(Scale)));};
  for(int I=0;I<6;++I)Lay(Part::FrameSection,FVector(0,-26-I*6.5,Layout::BenchHeightCm),200);
  // Clear of the jig risers and hold-downs.
  for(int I=0;I<2;++I)Lay(Part::MountingPlate,FVector(-40,18,Layout::BenchHeightCm+I*1.5),40);
  for(int I=0;I<4;++I)Lay(Part::Bracket,FVector(12+(I%2)*24,10+(I/2)*20,Layout::BenchHeightCm),22);
 }
}
