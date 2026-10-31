#include "Construction/FoundrylinePavementGeometry.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

bool AFoundrylineBuildSite::SetSelectedSign(const FString& Name,float Facing,const FFoundrylineBuildingColors& Colors){
 const auto* O=GetSelectedObject();const auto* D=O?Placement->FindDefinition(O->DefinitionId):nullptr;if(!D||D->FunctionId!=TEXT("ManufacturingSign"))return false;
 FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=O->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*O;
 FFoundrylinePlacementRequest Inverse=R;R.Replacement.SignName=Name.TrimStartAndEnd();R.Replacement.SignFacingDegrees=FMath::UnwindDegrees(Facing);R.Replacement.Colors=Colors;
 const auto Result=ExecuteConstruction(R);if(SentToHost(Result.Error))return true;if(!Result.IsSuccess()){Status=PlacementErrorText(Result.Error);return false;}RecordUndo(Inverse);return true;
}

AActor* AFoundrylineBuildSite::RenderManufacturingSign(const FFoundrylinePlacedObject& O,bool Preview,bool Valid){
 auto* Actor=GetWorld()->SpawnActor<AActor>();Actor->Tags.Add(TEXT("FoundrylinePropertyVisual"));Actor->Tags.Add(TEXT("FoundrylineManufacturingSign"));if(Preview)Actor->Tags.Add(TEXT("FoundrylinePlacementPreview"));
 auto* Root=NewObject<USceneComponent>(Actor);Actor->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();Actor->SetActorLocationAndRotation(FVector(O.Anchor),FRotator(0,O.QuarterTurns*90,0));if(!Preview)Root->SetMobility(EComponentMobility::Static);
 float SurfaceZ=0;const FVector Probe=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector({200,200,0});
 for(const auto& Other:Placement->GetObjects()){if(Other.ObjectId==O.ObjectId)continue;const auto* D=Placement->FindDefinition(Other.DefinitionId);if(!D||D->Footprint.Max.Z!=20)continue;const auto B=FFoundrylineGrid::RotateBounds(D->Footprint,Other.Anchor,Other.QuarterTurns);if(Probe.X>B.Min.X&&Probe.X<B.Max.X&&Probe.Y>B.Min.Y&&Probe.Y<B.Max.Y&&(Other.Pavement.Polygons.IsEmpty()||FFoundrylinePavementGeometry::Contains(Other.Pavement.Polygons,FVector2D(FRotator(0,Other.QuarterTurns*90,0).UnrotateVector(Probe-FVector(Other.Anchor))))))SurfaceZ=20;}
 auto* Frame=NewObject<USceneComponent>(Actor);Frame->SetupAttachment(Root);Frame->SetMobility(Preview?EComponentMobility::Movable:EComponentMobility::Static);Frame->SetRelativeLocationAndRotation({200,200,SurfaceZ},FRotator(0,O.SignFacingDegrees,0));Frame->RegisterComponent();Actor->AddInstanceComponent(Frame);
 auto Finish=[&](FLinearColor Color,bool Metal){auto* Base=Preview?Materials.FindRef(TEXT("Preview")).Get():LoadObject<UMaterialInterface>(nullptr,Metal?TEXT("/Game/Foundryline/Construction/Materials/M_ArchitecturalTrim.M_ArchitecturalTrim"):TEXT("/Game/Foundryline/Construction/Materials/M_InteriorFinish.M_InteriorFinish"));if(!Base)Base=Materials.FindRef(TEXT("Metal")).Get();auto* M=UMaterialInstanceDynamic::Create(Base,Actor);M->SetVectorParameterValue(TEXT("Tint"),Preview?(Valid?FLinearColor(.03f,.45f,1):FLinearColor(1,.03f,.01f)):Color);return M;};
 const TCHAR* Names[]={TEXT("Frame"),TEXT("Panel"),TEXT("Accent"),TEXT("Hardware")};const FLinearColor Colors[]={FLinearColor(.035f,.045f,.055f),O.Colors.Exterior,O.Colors.Trim,FLinearColor(.45f,.49f,.52f)};
 for(int I=0;I<4;++I){const FString Name=FString(TEXT("SM_ManufacturingSign_"))+Names[I];auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Foundryline/Buildings/Signs/Manufacturing/%s.%s"),*Name,*Name));if(!Mesh)continue;
  auto* C=NewObject<UStaticMeshComponent>(Actor);C->SetupAttachment(Frame);C->SetMobility(Preview?EComponentMobility::Movable:EComponentMobility::Static);C->SetStaticMesh(Mesh);auto* M=Finish(Colors[I],I!=1);for(int J=0;J<Mesh->GetStaticMaterials().Num();++J)C->SetMaterial(J,M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(!Preview);C->RegisterComponent();Actor->AddInstanceComponent(C);
 }
 auto Text=[&](const FString& Value,int Side,float Z,float Size,float MaxWidth){auto* C=NewObject<UTextRenderComponent>(Actor);C->SetupAttachment(Frame);C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);C->SetRelativeLocationAndRotation({0,Side*14.4,Z},FRotator(0,Side<0?-90:90,0));C->SetText(FText::FromString(Value));C->SetTextRenderColor((Preview?FLinearColor(.1f,.7f,1):O.Colors.Interior).ToFColor(true));C->SetWorldSize(Size);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();Actor->AddInstanceComponent(C);const FVector Extent=C->GetTextLocalSize();const float Width=FMath::Max(Extent.X,Extent.Y);if(Width>MaxWidth)C->SetWorldSize(Size*MaxWidth/Width);};
 for(int Side:{-1,1}){Text(O.SignName.IsEmpty()?TEXT("YOUR COMPANY"):O.SignName,Side,225,52,326);Text(TEXT("MANUFACTURING"),Side,180,18,290);}
 if(!Preview){auto Block=[&](FVector At,FVector Half){auto* C=NewObject<UBoxComponent>(Actor);C->SetupAttachment(Frame);C->SetMobility(EComponentMobility::Static);C->SetRelativeLocation(At);C->SetBoxExtent(Half);C->SetCollisionProfileName(TEXT("BlockAll"));C->ComponentTags.Add(TEXT("FoundrylinePhysicalCollision"));C->RegisterComponent();Actor->AddInstanceComponent(C);};Block({0,0,213.36},{182.88,13,61});for(int Side:{-1,1}){Block({Side*137.16,0,132},{8,8,132});Block({Side*137.16,0,1.4},{21,20,1.4});}}
 return Actor;
}
