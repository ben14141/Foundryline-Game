#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Construction/FoundrylineSizing.h"

const UFoundrylinePlaceableDefinition* UFoundrylinePlacementSubsystem::FindDefinition(FName DefinitionId) const
{
	const auto* Entry = Definitions.Find(DefinitionId);
	return Entry && IsValid(Entry->Get()) ? Entry->Get() : nullptr;
}

// Reconstruct only game-owned procedural kit definitions; save files cannot supply assets or rules.
bool UFoundrylinePlacementSubsystem::EnsureGeneratedDefinition(FName Id)
{
 if(FindDefinition(Id)) return true;
 if(Id==TEXT("ManufacturingSign")){auto* D=NewObject<UFoundrylinePlaceableDefinition>(this);D->DefinitionId=Id;D->FunctionId=Id;D->Snap=EFoundrylineSnap::Fine25cm;D->Footprint.Max={400,400,300};D->WorkingClearance=D->Footprint;D->AllowedSurfaces.Add(TEXT("StarterLand"));D->Mesh=TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Foundryline/Buildings/Signs/Manufacturing/SM_ManufacturingSign_Frame.SM_ManufacturingSign_Frame")));return RegisterDefinition(D);}

 TArray<FString> Parts;Id.ToString().ParseIntoArray(Parts,TEXT("_"));
 if(Parts.Num()==3&&Parts[0]==TEXT("Pavement")&&Parts[1].IsNumeric()&&Parts[2].IsNumeric()){const int W=FCString::Atoi(*Parts[1]),H=FCString::Atoi(*Parts[2]);if(W<100||H<100||W>20000||H>20000||W%25||H%25||Id.ToString()!=FString::Printf(TEXT("Pavement_%d_%d"),W,H))return false;auto* D=NewObject<UFoundrylinePlaceableDefinition>(this);D->DefinitionId=Id;D->FunctionId=TEXT("Pavement");D->Snap=EFoundrylineSnap::Fine25cm;D->Footprint.Max={W,H,20};D->WorkingClearance=D->Footprint;D->AllowedSurfaces.Add(TEXT("StarterLand"));return RegisterDefinition(D);}

 if(Parts.Num()!=3 || !Parts[1].IsNumeric() || !Parts[2].IsNumeric()) return false;
 const FString T=Parts[0];const int32 W=FCString::Atoi(*Parts[1]),H=FCString::Atoi(*Parts[2]);
 if(W<=0 || H<=0 || W>6400 || H>6400 || W%100 || H%100 || Id.ToString()!=FString::Printf(TEXT("%s_%d_%d"),*T,W,H)) return false;
 const bool Building=T==TEXT("Building") || T==TEXT("Shop2") || T==TEXT("Office");
 const bool Road=T==TEXT("Road2") || T==TEXT("Road4") || T==TEXT("Service") || T==TEXT("Driveway") || T==TEXT("TruckAccess");
 if(!Building && !Road && T!=TEXT("Parking") && T!=TEXT("FreightYard") && T!=TEXT("Walkway")) return false;
 if(!FFoundrylineSizing::Valid(FName(*T),W,H)) return false;
 auto* D=NewObject<UFoundrylinePlaceableDefinition>(this);D->DefinitionId=Id;
 D->FunctionId=Building?FName(TEXT("BuildingShell")):FName(*T);D->Snap=T==TEXT("TruckAccess") || (Road && W%200)?EFoundrylineSnap::Fine25cm:EFoundrylineSnap::Structural100cm;
 D->Footprint.Max={W,H,Building?(T==TEXT("Office")?320:T==TEXT("Shop2")?1220:620):20};
 D->WorkingClearance=D->Footprint;D->AllowedSurfaces.Add(TEXT("StarterLand"));
 D->Mesh=TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
 return RegisterDefinition(D);
}
