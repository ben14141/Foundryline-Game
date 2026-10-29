#include "Employees/FoundrylineHandymanAssetTools.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalMeshMerge.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif
FString UFoundrylineHandymanAssetTools::InspectMesh(USkeletalMesh* Mesh){
 if(!Mesh)return TEXT("{}");auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("mesh"),Mesh->GetPathName());auto Vec=[](FVector V){TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(V.X));A.Add(MakeShared<FJsonValueNumber>(V.Y));A.Add(MakeShared<FJsonValueNumber>(V.Z));return A;};const auto B=Mesh->GetBounds();J->SetArrayField(TEXT("boundsMinCm"),Vec(B.Origin-B.BoxExtent));J->SetArrayField(TEXT("boundsMaxCm"),Vec(B.Origin+B.BoxExtent));const auto& Ref=Mesh->GetRefSkeleton();TArray<FTransform> Pose;TArray<TSharedPtr<FJsonValue>> Bones;for(int I=0;I<Ref.GetNum();++I){FTransform T=Ref.GetRefBonePose()[I];int P=Ref.GetParentIndex(I);if(P>=0)T*=Pose[P];Pose.Add(T);auto Bone=MakeShared<FJsonObject>();Bone->SetStringField(TEXT("name"),Ref.GetBoneName(I).ToString());Bone->SetNumberField(TEXT("parent"),P);Bone->SetArrayField(TEXT("position"),Vec(T.GetLocation()));TArray<TSharedPtr<FJsonValue>> Q;for(double V:{T.GetRotation().X,T.GetRotation().Y,T.GetRotation().Z,T.GetRotation().W})Q.Add(MakeShared<FJsonValueNumber>(V));Bone->SetArrayField(TEXT("rotation"),Q);Bones.Add(MakeShared<FJsonValueObject>(Bone));}J->SetArrayField(TEXT("bones"),Bones);FString Out;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Out));return Out;
}
USkeletalMesh* UFoundrylineHandymanAssetTools::BuildRuntimeVariant(int32 Index,const TArray<USkeletalMesh*>& Parts){
 if(Parts.IsEmpty())return nullptr;for(auto* P:Parts)if(!P)return nullptr;auto* Mesh=NewObject<USkeletalMesh>(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),USkeletalMesh::StaticClass(),FName(*FString::Printf(TEXT("FoundrylineWorker_%d"),Index))),RF_Transient);TArray<FSkelMeshMergeSectionMapping> Mapping;FSkeletalMeshMerge Merge(Mesh,Parts,Mapping,0,EMeshBufferAccess::Default);if(!Merge.DoMerge())return nullptr;Mesh->SetSkeleton(Parts[0]->GetSkeleton());Mesh->SetPhysicsAsset(nullptr);return Mesh;
}
