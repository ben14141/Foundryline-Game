#pragma once
#include "CoreMinimal.h"
#include "Management/FoundrylineOfficeTypes.h"
class UWorld;struct FFoundrylineFactoryState;struct FFoundrylineFactorySimulation;struct FFoundrylinePlacedObject;
struct FFoundrylineOfficeRoomSpec {
 FGuid BuildingId,RoomId;uint8 Floor=0;FString Label;FTransform Frame;
 FBox2D Bounds;TArray<FGuid> DoorIds;TArray<FBox2D> KeepClear;TArray<FBox2D> WallOpenings;
 double WallInset[4]={30,30,30,30};
};
struct FOUNDRYLINE_API FFoundrylineOffices {
 static TArray<FFoundrylineOfficeRoomSpec> Rooms(UWorld*,const FFoundrylineFactoryState&,const FTransform& PlantFrame);
 static bool Assign(UWorld*,FFoundrylineFactorySimulation&,const FTransform&,FGuid Building,FGuid Room,int Floor,int Person,FString& Error);
 static void ReconcileWallDecor(UWorld*,FFoundrylineFactorySimulation&,const FTransform&);
 static bool Release(FFoundrylineFactorySimulation&,int Person,FString& Error);
 static bool ValidateProperty(const FFoundrylineFactoryState&,const TArray<FFoundrylinePlacedObject>&,const class UFoundrylinePlacementSubsystem*,const FTransform&,FString& Error);
 static bool Valid(const FFoundrylineFactoryState&,FString& Error);
 static bool ProtectConstruction(UWorld*,const FFoundrylinePlacedObject&,FString& Error);
 static bool DeskInUse(const FFoundrylineFactoryState&,int Desk);
 static const TCHAR* Mesh(uint8 Kind);
 static FVector Size(uint8 Kind);
 static bool Collides(uint8 Kind);
 static bool IsWallDecor(uint8 Kind);
};
