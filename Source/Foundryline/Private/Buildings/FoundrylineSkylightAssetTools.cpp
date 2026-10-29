#include "Buildings/FoundrylineSkylightAssetTools.h"
#if WITH_EDITOR
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif
bool UFoundrylineSkylightAssetTools::PrepareSkylight()
{
#if WITH_EDITOR
 auto* Source=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Roof/SM_Window_Roof_400.SM_Window_Roof_400"));
 auto* Glass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_WindowGlass.M_WindowGlass"));
 if(!Source || !Glass) return false;
 const FString Path=TEXT("/Game/Foundryline/Buildings/Roofs/SM_Skylight_400");
 if(LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".SM_Skylight_400")))) return true;
 auto* Package=CreatePackage(*Path);auto* Mesh=DuplicateObject<UStaticMesh>(Source,Package,TEXT("SM_Skylight_400"));
 Mesh->SetFlags(RF_Public|RF_Standalone);auto* Description=Mesh->GetMeshDescription(0);if(!Description) return false;
 FStaticMeshAttributes Attributes(*Description);auto Positions=Attributes.GetVertexPositions();auto Slots=Attributes.GetPolygonGroupMaterialSlotNames();
 TArray<FPolygonID> Panes;
 for(auto Polygon:Description->Polygons().GetElementIDs())
 {
  if(Slots[Description->GetPolygonPolygonGroup(Polygon)]!=FName(TEXT("Material #23"))) continue;
  double Area=0;bool Broad=false;
  for(auto Triangle:Description->GetPolygonTriangles(Polygon))
  {
   const auto Vs=Description->GetTriangleVertices(Triangle);const FVector A(Positions[Vs[0]]),B(Positions[Vs[1]]),C(Positions[Vs[2]]);
   const double Part=FVector::CrossProduct(B-A,C-A).Size()/2;Area+=Part;
   Broad|=Part>4000 && FMath::Min3(FVector::Distance(A,B),FVector::Distance(B,C),FVector::Distance(C,A))>80;
  }
  if(Broad && Area>4000) Panes.Add(Polygon);
 }
 if(Panes.IsEmpty()) {UE_LOG(LogTemp,Error,TEXT("Skylight has no verified broad glass panes"));return false;}
 Mesh->GetStaticMaterials().Add(FStaticMaterial(Glass,TEXT("FoundrylineGlass")));
 const auto Group=Description->CreatePolygonGroup();Slots[Group]=TEXT("FoundrylineGlass");
 for(auto Polygon:Panes) Description->SetPolygonPolygonGroup(Polygon,Group);
 Mesh->CommitMeshDescription(0);Mesh->Build(false);Mesh->MarkPackageDirty();FAssetRegistryModule::AssetCreated(Mesh);
 FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
 UE_LOG(LogTemp,Display,TEXT("FOUNDRYLINE_SKYLIGHT_GLASS_POLYGONS=%d"),Panes.Num());
 return UPackage::SavePackage(Package,Mesh,*FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension()),Args);
#else
 return false;
#endif
}
