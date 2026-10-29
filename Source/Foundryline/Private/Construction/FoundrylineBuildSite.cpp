#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylineEquipmentCollision.h"
#include "Companies/FoundrylineBusinessRegistry.h"
#include "Construction/FoundrylineFacadeLayout.h"
#include "Construction/FoundrylineSizing.h"
#include "Construction/FoundrylineBuildingConnections.h"
#include "Companies/FoundrylinePropertyLot.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Buildings/FoundrylinePersonnelDoorComponent.h"
#include "UI/FoundrylinePlayerController.h"
#include "SaveSystem/FoundrylineSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "Pathing/FoundrylineDrivingLayout.h"
#include "Pathing/FoundrylineVehiclePathSubsystem.h"

namespace
{
	FFoundrylineGridBounds Bounds(FIntVector Max) { FFoundrylineGridBounds B; B.Max=Max; return B; }
	FString ErrorText(EFoundrylinePlacementError E)
	{
		return AFoundrylineBuildSite::PlacementErrorText(E);
	}
	bool IsBuilding(FName Tool) { return Tool==TEXT("Building") || Tool==TEXT("Shop2") || Tool==TEXT("Office"); }
	bool IsRoad(FName Tool) { return Tool==TEXT("Service") || Tool==TEXT("Driveway") || Tool==TEXT("Road2") || Tool==TEXT("Road4") || Tool==TEXT("TruckAccess"); }
}

AFoundrylineBuildSite::AFoundrylineBuildSite()
{
	PrimaryActorTick.bCanEverTick=false;
 bReplicates=true;bAlwaysRelevant=true;SetNetUpdateFrequency(10);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SiteRoot")));
}

void AFoundrylineBuildSite::BeginPlay()
{
	Super::BeginPlay();
		Placement=GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>();
	Context.CompanyId=FGuid(0xF001,1,1,1); Context.PlantId=FGuid(0xF001,2,1,1); Context.bCanConstruct=true;
	const TCHAR* Names[]={TEXT("Asphalt"),TEXT("Concrete"),TEXT("Wall"),TEXT("White"),TEXT("Yellow"),TEXT("Metal"),TEXT("Door"),TEXT("Glass"),TEXT("Preview")};
	for(const auto* Name:Names)
	{
		const FString Path=FString::Printf(TEXT("/Game/Foundryline/Construction/Materials/M_%s.M_%s"),Name,Name);
		Materials.Add(FName(Name),LoadObject<UMaterialInterface>(nullptr,*Path));
	}
	Materials.Add(TEXT("Glass"),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_WindowGlass.M_WindowGlass")));
	Materials.Add(TEXT("OfficeFloor"),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_OfficeFloor.M_OfficeFloor")));
	Materials.Add(TEXT("Landscape"),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/World/Materials/M_FlatForestGround.M_FlatForestGround")));
	ChangeHandle=Placement->OnPlacementChanged.AddUObject(this,&AFoundrylineBuildSite::Changed);
 if(!HasAuthority()) {if(NetworkState.Revision>0) ReceiveNetworkState();return;}
	FFoundrylineBuildRegion Region;
	Region.RegionId=FGuid(0xF001,3,1,1); Region.CompanyId=Context.CompanyId; Region.PlantId=Context.PlantId;
	Region.SurfaceId=TEXT("StarterLand"); Region.Bounds.Min=bLargeShowcase?FIntVector(-12000,-10000,0):FIntVector(-6500,-5500,0); Region.Bounds.Max=bLargeShowcase?FIntVector(12000,10000,5):FIntVector(6500,5500,5);
	if(bStarterProperty)
 {
  Region=FFoundrylinePropertyLot::Starter(Context.CompanyId,Context.PlantId).Region;
  auto Street=Region;Street.RegionId=FGuid(0xF001,3,2,1);Street.CompanyId=FGuid(0xF001,99,1,1);Street.PlantId=FGuid(0xF001,99,2,1);Street.bHasBuildingSetback=false;Street.Bounds.Min={-10000,-3700,0};Street.Bounds.Max={10000,-2400,5};
  bReady=Placement->InitializeRegions({Region,Street})==EFoundrylinePlacementError::None;
 }
 else bReady=Placement->InitializeRegions({Region})==EFoundrylinePlacementError::None;
	if(!bReady) { Status=TEXT("Property setup failed"); return; }
	SeedSite();
 PublishNetworkState();
 if(auto* GI=GetGameInstance()) if(auto* Saves=GI->GetSubsystem<UFoundrylineSaveSubsystem>()) SaveHandle=Saves->OnCompleted.AddUObject(this,&AFoundrylineBuildSite::SaveCompleted);
 for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It) if(It->IsLocalController()) {SetRoofsVisible(It->AreRoofsShown());break;}
	ClearConstructionFeedback();
 Status=bStarterProperty?TEXT("LOT 01 / 100 x 80 m / 3 m building setback / public street protected"):TEXT("Drag a rectangle for a building. Add personnel and garage doors afterward.");
 if(auto* R=AFoundrylineBusinessRegistry::Find(GetWorld());R&&!R->IsWorldBound())R->BindWorld();
}

void AFoundrylineBuildSite::EndPlay(const EEndPlayReason::Type Reason)
{
	if(Placement) Placement->OnPlacementChanged.Remove(ChangeHandle);
 if(auto* GI=GetGameInstance()) if(auto* Saves=GI->GetSubsystem<UFoundrylineSaveSubsystem>()) Saves->OnCompleted.Remove(SaveHandle);
	HidePreview();
 for(const auto& Pair:Visuals) if(IsValid(Pair.Value)) Pair.Value->Destroy();
 if(IsValid(BoundaryActor)) BoundaryActor->Destroy();
 Super::EndPlay(Reason);
}

FFoundrylinePlacementContext AFoundrylineBuildSite::LocalContext() const
{
 // Previews, picking and magnets follow the local player's real company once
 // the registry has bound the world. Bootstrap seeding keeps the site context.
 const auto* Registry=AFoundrylineBusinessRegistry::Find(GetWorld());
 if(!Registry || (HasAuthority() && !Registry->IsWorldBound())) return Context;
 for(TActorIterator<AFoundrylinePlayerController> It(GetWorld());It;++It) if(It->IsLocalController())
 {
  FFoundrylinePlacementContext Out;
  if(Registry->ConstructionContext(*It,Out)) return Out;
  break;
 }
 return Context;
}

TArray<FFoundrylineGridBounds> AFoundrylineBuildSite::SnapTargets(bool bBuilding,FGuid Exclude) const
{
 // Buildings join the same plant's buildings. Pavement meets any pavement or
 // wall flush, including the town street, because touching edges never overlap.
 TArray<FFoundrylineGridBounds> Out;const auto Local=LocalContext();
 for(const auto& Existing:Placement->GetObjects())
 {
  const auto* ED=Placement->FindDefinition(Existing.DefinitionId);
  if(!ED || Existing.ObjectId==Exclude) continue;
  const bool Shell=ED->FunctionId==TEXT("BuildingShell");
  if(bBuilding?(!Shell || Existing.CompanyId!=Local.CompanyId || Existing.PlantId!=Local.PlantId):(!Shell && !FFoundrylineDrivingLayout::IsPaved(ED->FunctionId))) continue;
  Out.Add(FFoundrylineGrid::RotateBounds(ED->Footprint,Existing.Anchor,Existing.QuarterTurns));
 }
 return Out;
}

bool AFoundrylineBuildSite::RectangleRequest(FName Tool,FIntVector Start,FIntVector End,FFoundrylinePlacementRequest& Out,uint8 SnapEdges,bool bExtend)
{
	bConnected=false;
	if(!bReady || (!IsBuilding(Tool) && Tool!=TEXT("Parking") && Tool!=TEXT("FreightYard") && Tool!=TEXT("Walkway") && !IsRoad(Tool))) return false;
	if(!FFoundrylineGrid::SnapWorld(FVector(Start),EFoundrylineSnap::Structural100cm,Start) || !FFoundrylineGrid::SnapWorld(FVector(End),EFoundrylineSnap::Structural100cm,End)) {Status=TEXT("Outside the supported grid range");return false;}
	int32 W=0,D=0;
	if(IsBuilding(Tool) || Tool==TEXT("Parking") || Tool==TEXT("FreightYard") || Tool==TEXT("Walkway"))
	{
        // Rectangles are allowed in either world orientation; their local layout
        // keeps its real car/truck dimensions and is never stretched to fit.
        const auto Minimum=FFoundrylineSizing::Minimum(Tool);
		FIntVector Min(FMath::Min(Start.X,End.X),FMath::Min(Start.Y,End.Y),0),Max(FMath::Max(Start.X,End.X),FMath::Max(Start.Y,End.Y),0);
		const auto Targets=SnapTargets(IsBuilding(Tool),SelectedObject);
		// A drag that never leaves the building stays a plain rectangle, so the
		// player sees the usual red overlap ghost and its reason.
		if(IsBuilding(Tool) && bExtend) for(const auto& Host:Targets)
		{
			if(Start.X<Host.Min.X || Start.Y<Host.Min.Y || Start.X>Host.Max.X || Start.Y>Host.Max.Y) continue;
			if(FFoundrylineBuildingConnections::ExtendFrom(Host,Start,End,Min,Max)) break;
		}
		const FIntVector RawMin=Min,RawMax=Max;
		auto Fits=[&](bool& Rotate,int64& LocalW,int64& LocalD)
		{
			const int64 DX=Max.X-Min.X,DY=Max.Y-Min.Y;
			Rotate=FFoundrylineSizing::Surface(Tool) && (DraftAreaRotation%2 || (Tool==TEXT("Parking") && DY<Minimum.Y && DX>=Minimum.Y && DY>=Minimum.X));
			LocalW=Rotate?DY:DX;LocalD=Rotate?DX:DY;
			return LocalW>=Minimum.X && LocalD>=Minimum.Y && LocalW<=6400 && LocalD<=6400;
		};
		bool Rotate=false;int64 LocalW=0,LocalD=0;
		if(SnapEdges && bExactDraftPlacement && !IsBuilding(Tool))
		{
			// A stamped size never changes; the magnet slides the whole footprint instead.
			FFoundrylineGridBounds Stamp;Stamp.Min=Min;Stamp.Max=Max;
			const auto Offset=FFoundrylineBuildingConnections::SnapOffset(Stamp,Targets);Min+=Offset;Max+=Offset;
		}
		else if(SnapEdges)
		{
			const bool Joined=FFoundrylineBuildingConnections::Snap(Min,Max,Targets,FMath::Min(Minimum.X,Minimum.Y),SnapEdges);
			if(!Fits(Rotate,LocalW,LocalD)) {Min=RawMin;Max=RawMax;}
			else if(IsBuilding(Tool)) bConnected=Joined;
		}
		if(!Fits(Rotate,LocalW,LocalD))
		{
			Status=FFoundrylineSizing::Limits(Tool)+TEXT(". Drag to draw the area");
			return false;
		}
		Out.Anchor={Min.X,Min.Y,0};
		W=Max.X-Min.X; D=Max.Y-Min.Y;
        if(Rotate) {Swap(W,D);Out.Anchor={Max.X,Min.Y,0};Out.QuarterTurns=1;}
		if(!FFoundrylineSizing::Valid(Tool,W,D)) {Status=FFoundrylineSizing::Limits(Tool);return false;}
	}
	else
	{
		const int64 DX=FMath::Abs(int64(End.X)-Start.X), DY=FMath::Abs(int64(End.Y)-Start.Y);
		const bool Horizontal=DX>DY;
		FIntVector A=Start,B=End;
		W=Tool==TEXT("Service")?400:Tool==TEXT("Driveway")?600:Tool==TEXT("TruckAccess")?DraftTruckWidthCm:Tool==TEXT("Road2")?800:1400;
		for(const auto& Existing:Placement->GetObjects())
		{
            if(bExactDraftPlacement) break;
			const auto* ED=Placement->FindDefinition(Existing.DefinitionId);
			if(!ED || (!IsRoad(ED->FunctionId) && ED->FunctionId!=TEXT("Parking"))) continue;
			const auto EB=FFoundrylineGrid::RotateBounds(ED->Footprint,Existing.Anchor,Existing.QuarterTurns);
			auto Inside=[&](FIntVector P){return P.X>=EB.Min.X && P.X<=EB.Max.X && P.Y>=EB.Min.Y && P.Y<=EB.Max.Y;};
			if(Inside(A)!=Inside(B) && IsRoad(ED->FunctionId))
			{
				// At an approach end, align the side of the new run with the end cap.
				// This makes a complete square L corner instead of half a T intersection.
				if(Horizontal && EB.Max.Y-EB.Min.Y>=W) Start.Y=FMath::Clamp(Start.Y,EB.Min.Y+W/2,EB.Max.Y-W/2);
				else if(!Horizontal && EB.Max.X-EB.Min.X>=W) Start.X=FMath::Clamp(Start.X,EB.Min.X+W/2,EB.Max.X-W/2);
			}
			if(Inside(A) && !Inside(B))
			{
				if(Horizontal) A.X=B.X>A.X?EB.Max.X:EB.Min.X;
				else A.Y=B.Y>A.Y?EB.Max.Y:EB.Min.Y;
			}
			else if(Inside(B) && !Inside(A))
			{
				if(Horizontal) B.X=A.X>B.X?EB.Max.X:EB.Min.X;
				else B.Y=A.Y>B.Y?EB.Max.Y:EB.Min.Y;
			}
		}
		if(!bExactDraftPlacement) SnapRoadEnds(Horizontal,W,Start,A,B);
		const int64 Length=Horizontal?FMath::Abs(int64(B.X)-A.X):FMath::Abs(int64(B.Y)-A.Y);
		if(Length<400 || Length>6400) { Status=TEXT("Road: drag a straight 4-64 m run"); return false; }
		D=int32(Length);
		if(Horizontal) { Out.Anchor={FMath::Max(A.X,B.X),Start.Y-W/2,0}; Out.QuarterTurns=1; }
		else Out.Anchor={Start.X-W/2,FMath::Min(A.Y,B.Y),0};
	}
	Out.DefinitionId=FName(*FString::Printf(TEXT("%s_%d_%d"),*Tool.ToString(),W,D));
	if(!FFoundrylineSizing::Valid(Tool,W,D)) {Status=FFoundrylineSizing::Limits(Tool);return false;}
 if(!Placement->FindDefinition(Out.DefinitionId))
	{
		auto* Def=NewObject<UFoundrylinePlaceableDefinition>(this);
		Def->DefinitionId=Out.DefinitionId; Def->FunctionId=IsBuilding(Tool)?FName(TEXT("BuildingShell")):Tool;
		// Odd metre road widths have half-metre edges about their centre line.
		Def->Snap=Tool==TEXT("TruckAccess") || (IsRoad(Tool) && W%200)?EFoundrylineSnap::Fine25cm:EFoundrylineSnap::Structural100cm;
		Def->Footprint=Def->WorkingClearance=Bounds({W,D,IsBuilding(Tool)?(SlabHeightCm+(Tool==TEXT("Office")?300:Tool==TEXT("Shop2")?HighBayWallHeightCm:WallHeightCm)):20});
		Def->AllowedSurfaces.Add(TEXT("StarterLand"));
		Def->Mesh=TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
		if(!Placement->RegisterDefinition(Def)) return false;
	}
	if(IsBuilding(Tool) && Tool!=TEXT("Office"))
		for(uint8 Edge=0;Edge<4;++Edge)
			for(int32 Offset:FFoundrylineFacadeLayout::Windows(Edge%2==0?W:D,600))
			{
				FFoundrylineOpening Window; Window.OpeningId=FGuid::NewGuid();
				Window.Edge=Edge; Window.OffsetCm=Offset; Window.Kind=EFoundrylineOpeningKind::ShopWindow; Window.bAutomatic=true; Window.WindowSillCm=Tool==TEXT("Shop2")?900:100;
				Out.Openings.Add(Window);
			}
	if(Tool==TEXT("Office"))
	{
		Out.bOfficeAnnex=true;FFoundrylineOfficeSpace Zone;Zone.SpaceId=FGuid::NewGuid();Zone.Min={0,0};Zone.Max={W,D};Out.OfficeAreas={Zone};
		// An office always starts with a shop connection and an exterior entrance.
		bool bEntrances=false;const auto Local=LocalContext();
		for(const auto& Other:Placement->GetObjects())
		{
			if(bEntrances) break;
			const auto* OD=Placement->FindDefinition(Other.DefinitionId);if(!OD || OD->FunctionId!=TEXT("BuildingShell") || Other.bOfficeAnnex || Other.CompanyId!=Local.CompanyId || Other.PlantId!=Local.PlantId) continue;
			const auto B=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);
			const FIntVector Min=B.Min-Out.Anchor,Max=B.Max-Out.Anchor;
			for(uint8 Edge=0;Edge<4;++Edge)
			{
				const bool Touch=Edge==0?Max.Y==0:Edge==1?Min.X==W:Edge==2?Min.Y==D:Max.X==0;
				const int32 Length=Edge%2==0?W:D,Low=FMath::Max(0,Edge%2==0?Min.X:Min.Y),High=FMath::Min(Length,Edge%2==0?Max.X:Max.Y);
				if(!Touch || High-Low<400) continue;
				Out.Openings.RemoveAll([&](const auto& V){return V.Edge==Edge;});
				FFoundrylineOpening Door;Door.OpeningId=FGuid::NewGuid();Door.Edge=Edge;Door.Kind=EFoundrylineOpeningKind::OfficeDoor;Door.OffsetCm=FMath::Clamp(((Low+High)/2-50)/100*100,100,Length-200);Out.Openings.Add(Door);
				Door.OpeningId=FGuid::NewGuid();Door.Edge=(Edge+2)%4;Door.OffsetCm=(Length/2-50)/100*100;
				Out.Openings.RemoveAll([&](const auto& V){return V.Edge==Door.Edge && V.OffsetCm<Door.OffsetCm+130 && V.OffsetCm+V.WidthCm()+30>Door.OffsetCm;});Out.Openings.Add(Door);
				bEntrances=true;break;
			}
		}

  if(!bEntrances) {
   // A standalone office, or one joined only to another office, gets its
   // entrance on the first wall that still faces outside.
   FFoundrylineGridBounds Self;Self.Max={W,D,0};Self=FFoundrylineGrid::RotateBounds(Self,Out.Anchor,Out.QuarterTurns);
   const auto Neighbors=SnapTargets(true,SelectedObject);
   uint8 Outside=0;
   for(uint8 Edge=0;Edge<4;++Edge) {
    const bool Covered=Neighbors.ContainsByPredicate([&](const auto& N){
     const bool Touch=Edge==0?N.Max.Y==Self.Min.Y:Edge==1?N.Min.X==Self.Max.X:Edge==2?N.Min.Y==Self.Max.Y:N.Max.X==Self.Min.X;
     return Touch && FFoundrylineBuildingConnections::SharedLength(Self,N)>0;});
    if(!Covered) {Outside=Edge;break;}
   }
   FFoundrylineOpening Door;Door.OpeningId=FGuid::NewGuid();Door.Edge=Outside;
   Door.Kind=EFoundrylineOpeningKind::OfficeDoor;Door.OffsetCm=((Outside%2==0?W:D)/2-50)/100*100;
   Out.Openings.Add(Door);
  }
	}
	Out.ExpectedRevision=Placement->GetRevision(); return true;
}

void AFoundrylineBuildSite::SnapRoadEnds(bool Horizontal,int32 Width,FIntVector& Center,FIntVector& A,FIntVector& B) const
{
 // Road ends reach for whatever they nearly touch: another road, parking, a
 // yard or a building wall within 3 m becomes a flush, connected edge. A run
 // continuing a parallel road also lines up with its centre.
 const int32 Axis=Horizontal?0:1,Cross=1-Axis,Range=300,Grid=Width%200?25:100;
 FIntVector* Ends[2]={&A,&B};
 for(int32 I=0;I<2;++I)
 {
  auto& P=*Ends[I];const auto& Q=*Ends[1-I];
  const int32 Dir=P[Axis]>=Q[Axis]?1:-1,Low=Center[Cross]-Width/2,High=Low+Width;
  int32 Best=P[Axis],BestDistance=Range+1,ParallelCenter=MAX_int32;
  for(const auto& Existing:Placement->GetObjects())
  {
   const auto* ED=Placement->FindDefinition(Existing.DefinitionId);
   if(!ED || (ED->FunctionId!=TEXT("BuildingShell") && !FFoundrylineDrivingLayout::IsPaved(ED->FunctionId))) continue;
   const auto EB=FFoundrylineGrid::RotateBounds(ED->Footprint,Existing.Anchor,Existing.QuarterTurns);
   if(EB.Max[Cross]<=Low || EB.Min[Cross]>=High) continue;
   // An end already past the far side is a crossing, not an approach.
   if(Dir>0?P[Axis]>=EB.Max[Axis]:P[Axis]<=EB.Min[Axis]) continue;
   const int32 Face=Dir>0?EB.Min[Axis]:EB.Max[Axis],Gap=(Face-P[Axis])*Dir;
   if(Gap<-Range || Gap>Range || Face%Grid || FMath::Abs(Gap)>=BestDistance) continue;
   Best=Face;BestDistance=FMath::Abs(Gap);
   const bool Parallel=IsRoad(ED->FunctionId) && Existing.QuarterTurns%2==(Horizontal?1:0);
   ParallelCenter=Parallel?(EB.Min[Cross]+EB.Max[Cross])/2:MAX_int32;
  }
  P[Axis]=Best;
  if(ParallelCenter!=MAX_int32 && FMath::Abs(ParallelCenter-Center[Cross])<=Range && (ParallelCenter-Width/2)%Grid==0) Center[Cross]=ParallelCenter;
 }
}

bool AFoundrylineBuildSite::BuildRectangle(FName Tool,FIntVector Start,FIntVector End)
{
	FFoundrylinePlacementRequest R;
	if(!RectangleRequest(Tool,Start,End,R)) {DraftBlocked(Status,true);return false;}
	const auto Result=ExecuteConstruction(R);
	if(SentToHost(Result.Error)) return true;
	if(!Result.IsSuccess()) { Status=ErrorText(Result.Error); return false; }
	R.Action=EFoundrylinePlacementAction::Remove; R.ObjectId=Result.ObjectId; RecordUndo(R);
	Status=Tool==TEXT("Office")?(bConnected?TEXT("Office built with a door into the joined building. Draw rooms in the Office tab."):TEXT("Office built. Draw rooms in the Office tab.")):IsBuilding(Tool)?(bConnected?TEXT("Building expanded. Shared walls removed."):TEXT("Building placed. Add personnel and garage doors.")):Tool==TEXT("Parking")?TEXT("Parking lot placed"):TEXT("Road placed");
	HidePreview(); return true;
}

void AFoundrylineBuildSite::PreviewRectangle(FName Tool,FIntVector Start,FIntVector End)
{
	FFoundrylinePlacementRequest R;
	if(!RectangleRequest(Tool,Start,End,R)) {HidePreview();DraftBlocked(Status);return;}
	const auto Result=ValidateConstruction(R);
	FFoundrylinePlacedObject O; O.DefinitionId=R.DefinitionId; O.Anchor=R.Anchor; O.QuarterTurns=R.QuarterTurns; O.Openings=R.Openings;O.bOfficeAnnex=R.bOfficeAnnex;O.OfficeAreas=R.OfficeAreas;
	const auto Local=LocalContext();O.CompanyId=Local.CompanyId; O.PlantId=Local.PlantId;
	const auto* Def=Placement->FindDefinition(O.DefinitionId);
	Status=FString::Printf(TEXT("%d x %d m  |  %d m high  |  %s%s"),Def->Footprint.Max.X/100,Def->Footprint.Max.Y/100,(Def->Footprint.Max.Z-SlabHeightCm)/100,*ErrorText(Result.Error),bConnected?TEXT("  |  Connected expansion"):TEXT(""));
 if(FFoundrylineSizing::Surface(Tool)) Status=FString::Printf(TEXT("%.0f m %s x %.0f m %s / %s"),Def->Footprint.Max.X/100.f,IsRoad(Tool)?TEXT("wide"):TEXT("width"),Def->Footprint.Max.Y/100.f,IsRoad(Tool)?TEXT("long"):TEXT("depth"),Result.IsSuccess() && bExactDraftPlacement?TEXT("Click to place"):*ErrorText(Result.Error));
	ShowPreview(O,Result.IsSuccess(),FString::Printf(TEXT("%s_%s_%d_%lld"),*O.DefinitionId.ToString(),*O.Anchor.ToString(),O.QuarterTurns,Placement->GetRevision()));
 PreviewJoinLines.Reset();
 if(!IsRoad(Tool) && Result.IsSuccess()) {
  // Outline exactly what will connect: open shop walls, an office door, or a paved entrance.
  const auto A=FFoundrylineGrid::RotateBounds(Def->Footprint,O.Anchor,O.QuarterTurns);
  int32 Joined=0;
  for(const auto& B:SnapTargets(IsBuilding(Tool),SelectedObject)) {
   const int32 Length=FFoundrylineBuildingConnections::SharedLength(A,B);if(Length<=0) continue;
   Joined+=Length;
   if(A.Min.X==B.Max.X || A.Max.X==B.Min.X) {
    const int32 X=A.Min.X==B.Max.X?A.Min.X:A.Max.X;
    PreviewJoinLines.Append({FVector(X,FMath::Max(A.Min.Y,B.Min.Y),25),FVector(X,FMath::Min(A.Max.Y,B.Max.Y),25)});
   } else {
    const int32 Y=A.Min.Y==B.Max.Y?A.Min.Y:A.Max.Y;
    PreviewJoinLines.Append({FVector(FMath::Max(A.Min.X,B.Min.X),Y,25),FVector(FMath::Min(A.Max.X,B.Max.X),Y,25)});
   }
  }
  if(Joined) Status+=FString::Printf(TEXT(" / %.0f m join: %s"),Joined/100.f,!IsBuilding(Tool)?TEXT("paving connects"):O.bOfficeAnnex?TEXT("connecting door added"):TEXT("shared wall opens"));
 }
}

bool AFoundrylineBuildSite::DoorRequest(FName Tool,FIntVector Cursor,FFoundrylinePlacementRequest& Out) const
{
	if(!bReady || (Tool!=TEXT("Personnel") && Tool!=TEXT("Garage") && Tool!=TEXT("TruckDoor") && Tool!=TEXT("LoadingDock"))) return false;
	double Closest=151;
	for(const auto& O:Placement->GetObjects())
	{
		const auto* Def=Placement->FindDefinition(O.DefinitionId);
		if(!Def || Def->FunctionId!=TEXT("BuildingShell") || (O.bOfficeAnnex && Tool==TEXT("LoadingDock"))) continue;
		const FVector P=FRotator(0,O.QuarterTurns*90,0).UnrotateVector(FVector(Cursor-O.Anchor));
		const int32 W=Def->Footprint.Max.X,D=Def->Footprint.Max.Y;
		if(P.X < -150 || P.Y < -150 || P.X>W+150 || P.Y>D+150) continue;
		const double Distances[]={FMath::Abs(P.Y),FMath::Abs(P.X-W),FMath::Abs(P.Y-D),FMath::Abs(P.X)};
		for(uint8 Edge=0;Edge<4;++Edge)
		{
			if(Distances[Edge]>=Closest) continue;
			Closest=Distances[Edge]; Out.ObjectId=O.ObjectId;
			Out.Opening.Edge=Edge; Out.Opening.Kind=Tool==TEXT("LoadingDock")?EFoundrylineOpeningKind::LoadingDock:Tool==TEXT("TruckDoor")?EFoundrylineOpeningKind::TruckGarage:Tool==TEXT("Garage")?EFoundrylineOpeningKind::Garage:O.bOfficeAnnex?EFoundrylineOpeningKind::OfficeDoor:EFoundrylineOpeningKind::Personnel;
			const double U=Edge%2==0?P.X:P.Y;
			Out.Opening.OffsetCm=FMath::RoundToInt((U-Out.Opening.WidthCm()/2)/100)*100;
		}
	}
	if(!Out.ObjectId.IsValid()) return false;
	Out.Action=EFoundrylinePlacementAction::AddOpening; Out.ExpectedRevision=Placement->GetRevision();
	Out.Opening.OpeningId=FGuid::NewGuid(); return true;
}

bool AFoundrylineBuildSite::PlaceDoor(FName Tool,FIntVector Cursor)
{
	FFoundrylinePlacementRequest R;
	if(!DoorRequest(Tool,Cursor,R)) { Status=TEXT("Point at an existing building wall");DraftBlocked(Status,true); return false; }
	const auto* Existing=Placement->GetObjects().FindByPredicate([&](const auto& O){return O.ObjectId==R.ObjectId;});
	const auto PreviousOpenings=Existing->Openings;
	const auto Result=ExecuteConstruction(R);
	if(SentToHost(Result.Error)) return true;
	if(!Result.IsSuccess()) { Status=ErrorText(Result.Error); return false; }
	R.Action=EFoundrylinePlacementAction::ReplaceOpenings; R.Openings=PreviousOpenings; RecordUndo(R);
	Status=TEXT("Door opening installed"); HidePreview(); return true;
}

void AFoundrylineBuildSite::PreviewDoor(FName Tool,FIntVector Cursor)
{
	FFoundrylinePlacementRequest R;
	if(!DoorRequest(Tool,Cursor,R)) { HidePreview(); Status=TEXT("Point at a building wall to install the door");DraftBlocked(Status); return; }
	const auto Validation=ValidateConstruction(R);const bool Valid=Validation.IsSuccess();
	const auto* Existing=Placement->GetObjects().FindByPredicate([&](const FFoundrylinePlacedObject& O){return O.ObjectId==R.ObjectId;});
	if(!Existing) return;
	FFoundrylinePlacedObject O=*Existing; O.Openings={R.Opening};
	Status=Valid?TEXT("Click to install door"):*PlacementErrorText(Validation.Error);
	ShowPreview(O,Valid,FString::Printf(TEXT("Door_%s_%d_%d_%d_%lld"),*R.ObjectId.ToString(),R.Opening.Edge,R.Opening.OffsetCm,int32(R.Opening.Kind),Placement->GetRevision()));
}

void AFoundrylineBuildSite::PreviewColumns(FIntVector Cursor)
{
	if(!bReady) {HidePreview();return;}
	for(const auto& Object:Placement->GetObjects())
	{
		const auto* Def=Placement->FindDefinition(Object.DefinitionId);
		if(!Def || Def->FunctionId!=TEXT("BuildingShell") || Def->Footprint.Max.Z<620 || Object.bOfficeAnnex) continue;
		const auto Bounds=FFoundrylineGrid::RotateBounds(Def->Footprint,Object.Anchor,Object.QuarterTurns);
		if(Cursor.X<Bounds.Min.X || Cursor.Y<Bounds.Min.Y || Cursor.X>Bounds.Max.X || Cursor.Y>Bounds.Max.Y) continue;
		FFoundrylinePlacedObject Draft=Object;
		Draft.ColumnBayCm=Object.ColumnBayCm==400?600:Object.ColumnBayCm==600?800:400;
        FFoundrylineFacadeLayout::Reflow(Draft,Def->Footprint.Max.X,Def->Footprint.Max.Y,Def->Footprint.Max.Z);
		FFoundrylinePlacementRequest Request;Request.Action=EFoundrylinePlacementAction::SetColumnBay;
		Request.ObjectId=Object.ObjectId;Request.ExpectedRevision=Placement->GetRevision();Request.ColumnBayCm=Draft.ColumnBayCm;
		const bool Valid=ValidateConstruction(Request).IsSuccess();
		Status=FString::Printf(TEXT("Hologram: %d m column bays. Click to apply."),Draft.ColumnBayCm/100);
		ShowPreview(Draft,Valid,FString::Printf(TEXT("Columns_%s_%d_%lld"),*Object.ObjectId.ToString(),Draft.ColumnBayCm,Placement->GetRevision()));return;
	}
	HidePreview();Status=TEXT("Point inside a Tier 2 shop to preview columns");
}

bool AFoundrylineBuildSite::AdjustColumns(FIntVector Cursor,int32 SpacingCm)
{
	if(!bReady) return false;
	for(const auto& Object:Placement->GetObjects())
	{
		const auto* Def=Placement->FindDefinition(Object.DefinitionId);
		if(!Def || Def->FunctionId!=TEXT("BuildingShell") || Def->Footprint.Max.Z<620 || Object.bOfficeAnnex) continue;
		const auto B=FFoundrylineGrid::RotateBounds(Def->Footprint,Object.Anchor,Object.QuarterTurns);
		if(Cursor.X<B.Min.X || Cursor.Y<B.Min.Y || Cursor.X>B.Max.X || Cursor.Y>B.Max.Y) continue;
		FFoundrylinePlacementRequest R; R.Action=EFoundrylinePlacementAction::SetColumnBay;
		R.ObjectId=Object.ObjectId; R.ExpectedRevision=Placement->GetRevision();
		R.ColumnBayCm=SpacingCm?SpacingCm:(Object.ColumnBayCm==400?600:Object.ColumnBayCm==600?800:400);
		const int32 Previous=Object.ColumnBayCm;
		const auto Result=ExecuteConstruction(R);
		if(SentToHost(Result.Error)) return true;
		if(!Result.IsSuccess()) {Status=ErrorText(Result.Error); return false;}
		Status=FString::Printf(TEXT("Shop column bays: %d m. Click again to adjust; Z undoes."),R.ColumnBayCm/100);
		R.ColumnBayCm=Previous; RecordUndo(R); HidePreview(); return true;
	}
	Status=TEXT("Click inside a Tier 2 shop to adjust columns"); return false;
}

void AFoundrylineBuildSite::ShowPreview(const FFoundrylinePlacedObject& O,bool Valid,const FString& Key)
{
	const FString StateKey=Key+(Valid?TEXT("_Ready"):TEXT("_Blocked"));
 if(PreviewActor && PreviewKey==StateKey) return;
	const bool HadFeedback=bHasPreviewFeedback;const auto Error=PreviewError;const FString Reason=DraftReason;
 HidePreview();bHasPreviewFeedback=HadFeedback;PreviewError=Error;DraftReason=Reason; PreviewActor=Render(O,true,Valid,Key.StartsWith(TEXT("Door_")),Key.StartsWith(TEXT("Columns_")),Key.StartsWith(TEXT("Office_Office")) || Key.StartsWith(TEXT("OfficeOpening_"))); PreviewKey=StateKey;
 if(const auto* D=Placement->FindDefinition(O.DefinitionId)) {PreviewBounds=FFoundrylineGrid::RotateBounds(D->Footprint,O.Anchor,O.QuarterTurns);bHasPreviewBounds=true;}
}

void AFoundrylineBuildSite::HidePreview()
{
	PreviewJoinLines.Reset();
	if(IsValid(PreviewActor)) PreviewActor->Destroy();
	bHasPreviewBounds=false;PreviewActor=nullptr; PreviewKey.Reset();bHasPreviewFeedback=false;DraftReason.Empty();
}

AActor* AFoundrylineBuildSite::Render(const FFoundrylinePlacedObject& O,bool bPreview,bool bValid,bool bOpeningOnly,bool bColumnsOnly,bool bOfficeOnly)
{
	const auto* Def=Placement->FindDefinition(O.DefinitionId);
	if(!Def) return nullptr;
	auto* Actor=GetWorld()->SpawnActor<AActor>();
	Actor->Tags.Add(TEXT("FoundrylinePropertyVisual"));
	if(bPreview) Actor->Tags.Add(TEXT("FoundrylinePlacementPreview"));
	auto* Root=NewObject<USceneComponent>(Actor); Actor->SetRootComponent(Root); Root->RegisterComponent();
	Actor->SetActorLocationAndRotation(FVector(O.Anchor),FRotator(0,O.QuarterTurns*90,0));
 // Committed construction never moves in place. Static bases use world-space
 // character corrections instead of references to client-local visual components.
 if(!bPreview) Root->SetMobility(EComponentMobility::Static);
	const float W=Def->Footprint.Max.X,D=Def->Footprint.Max.Y;
	auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInstanceDynamic* GhostMat=nullptr;
	if(bPreview && Materials.FindRef(TEXT("Preview")))
	{
		GhostMat=UMaterialInstanceDynamic::Create(Materials.FindRef(TEXT("Preview")),Actor);
		GhostMat->SetVectorParameterValue(TEXT("Tint"),bValid?FLinearColor(0.03f,0.45f,1.f):FLinearColor(1.f,0.03f,0.01f));
	}
 TMap<FName,UMaterialInterface*> Painted;
 if(!bPreview) for(const auto Key:{FName(TEXT("Wall")),FName(TEXT("Interior")),FName(TEXT("Metal"))})
 {
  auto* Base=Materials.FindRef(Key==TEXT("Interior")?FName(TEXT("Wall")):Key).Get();
  auto* Mat=UMaterialInstanceDynamic::Create(Base,Actor);
  Mat->SetVectorParameterValue(TEXT("Tint"),Key==TEXT("Wall")?O.Colors.Exterior:Key==TEXT("Interior")?O.Colors.Interior:O.Colors.Trim);
  Painted.Add(Key,Mat);
 }
	TMap<FName,UInstancedStaticMeshComponent*> Groups;
	auto Box=[&](FVector Center,FVector Size,FName Material,bool Collision=true,float Yaw=0.f)
	{
		if(Size.GetMin()<=0) return;
		if(bColumnsOnly && !(Material==TEXT("Metal") && ((Size.X==36 && Size.Y==58) || (Size.Y==36 && Size.X==58) || Size.Z==24 || Size.Z==30))) return;
		const FName Key(*FString::Printf(TEXT("%s_%d"),*Material.ToString(),Collision));
		auto*& Comp=Groups.FindOrAdd(Key);
		if(!Comp)
		{
			Comp=NewObject<UInstancedStaticMeshComponent>(Actor);Comp->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static); Actor->AddInstanceComponent(Comp);
			Comp->SetupAttachment(Root); Comp->SetStaticMesh(Cube);
			Comp->SetMaterial(0,bPreview?static_cast<UMaterialInterface*>(GhostMat):(Painted.Contains(Material)?Painted[Material]:Materials.FindRef(Material).Get()));
			Comp->SetCollisionEnabled(!bPreview && Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
			Comp->SetCollisionResponseToAllChannels(ECR_Block); Comp->SetCastShadow(!bPreview && Collision && Material!=TEXT("Glass"));
   if(Material==TEXT("Glass")) Comp->SetAffectDistanceFieldLighting(false);
			Comp->RegisterComponent();
		}
		Comp->AddInstance(FTransform(FRotator(0,Yaw,0).Quaternion(),Center,Size/100.f));
	};
	auto Module=[&](const TCHAR* Path,FVector Position,float Yaw)
	{
		auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path); if(!Mesh) return;
		const FName Key=Mesh->GetFName(); auto*& Comp=Groups.FindOrAdd(Key);
		if(!Comp)
		{
			Comp=NewObject<UInstancedStaticMeshComponent>(Actor);Comp->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static); Actor->AddInstanceComponent(Comp);
			Comp->SetupAttachment(Root); Comp->SetStaticMesh(Mesh); Comp->SetMaterial(0,bPreview?static_cast<UMaterialInterface*>(GhostMat):Materials.FindRef(TEXT("Concrete")).Get());
			Comp->SetCollisionEnabled(bPreview?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics); Comp->SetCollisionResponseToAllChannels(ECR_Block); Comp->SetCastShadow(!bPreview); Comp->RegisterComponent();
		}
		Comp->AddInstance(FTransform(FRotator(0,Yaw,0).Quaternion(),Position));
	};
auto Grass=[&](FVector Position)
	{
		const FName Key(TEXT("LandscapeGrass")); auto*& Comp=Groups.FindOrAdd(Key);
		if(!Comp)
		{
			auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Tree/SM_Grass_02.SM_Grass_02")); if(!Mesh) return;
			Comp=NewObject<UInstancedStaticMeshComponent>(Actor);Comp->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static); Actor->AddInstanceComponent(Comp);
			Comp->SetupAttachment(Root); Comp->SetStaticMesh(Mesh);
			if(bPreview) for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot) Comp->SetMaterial(Slot,GhostMat);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCullDistances(6000,10000); Comp->SetCastShadow(false); Comp->RegisterComponent();
		}
		Comp->AddInstance(FTransform(FRotator::ZeroRotator,Position));
	};
	auto DoorLeaf=[&](uint8 Edge,float A,float B,FVector2D Min,FVector2D Max,bool Office,FGuid OpeningId,bool Inward=false)
	{
		const TCHAR* Path=Office?TEXT("/Game/Factory/Meshes/Wall_Outer(Door)/SM_Door_B2_Right.SM_Door_B2_Right"):TEXT("/Game/Factory/Meshes/Wall_Outer(Door)/SM_Door_A1.SM_Door_A1");
		auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);if(!Mesh) return;
		const float U=Office?A+5:B-5;
		const FVector Hinge=Edge==0?FVector(Min.X+U,Min.Y,20):Edge==1?FVector(Max.X,Min.Y+U,20):Edge==2?FVector(Min.X+U,Max.Y,20):FVector(Min.X,Min.Y+U,20);
		const float BaseYaw=Edge%2==0?0:90;
		const float Swing=(Edge==0 || Edge==1?1.f:-1.f)*(Office?-90.f:90.f)*(Inward?-1.f:1.f);
		auto* C=NewObject<UFoundrylinePersonnelDoorComponent>(Actor);Actor->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);
		if(bPreview) for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot) C->SetMaterial(Slot,GhostMat);
		C->SetCollisionEnabled(bPreview?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetCastShadow(!bPreview);C->RegisterComponent();
		const float Bottom=Mesh->GetBoundingBox().Min.Z;
		C->SetRelativeLocation(Hinge-FVector(0,0,Bottom));C->BuildingObjectId=O.ObjectId;C->InitializeSwing(BaseYaw,Swing,OpeningId,DoorStates.Contains(OpeningId)?DoorStates[OpeningId]:true);
  if(bPreview) C->SetIsReplicated(false);
	};
	auto OccupiedOutside=[&](FVector Local,bool RoadOnly=false)
	{
		const FVector World=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector(Local);
		for(const auto& Other:Placement->GetObjects())
		{
			if(Other.ObjectId==O.ObjectId) continue;
			const auto* OD=Placement->FindDefinition(Other.DefinitionId); if(!OD) continue;
			if(RoadOnly && !IsRoad(OD->FunctionId) && OD->FunctionId!=TEXT("Parking") && OD->FunctionId!=TEXT("FreightYard")) continue;
			const FVector P=FRotator(0,Other.QuarterTurns*90,0).UnrotateVector(World-FVector(Other.Anchor));
			if(P.X>=0 && P.Y>=0 && P.X<OD->Footprint.Max.X && P.Y<OD->Footprint.Max.Y) return true;
		}
		return false;
	};
	const bool Building=Def->FunctionId==TEXT("BuildingShell");
	if(!Building && IsRoad(Def->FunctionId) && !bPreview)
	{
		for(float Y=0;Y<D;Y+=100)
		{
			bool Overlap=false;
			for(float X=50;X<W;X+=100) Overlap|=OccupiedOutside({X,Y+50,0},true);
			if(!Overlap) {Box({W/2,Y+50,10},{W,100,20},TEXT("Asphalt"));continue;}
			for(float X=0;X<W;X+=100)
			{
				const FVector World=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector({X+50,Y+50,0});bool Covered=false;
				for(const auto& Other:Placement->GetObjects())
				{
					if(Other.ObjectId==O.ObjectId || Other.ObjectId.ToString()>O.ObjectId.ToString()) continue;
					const auto* OD=Placement->FindDefinition(Other.DefinitionId);if(!OD || !IsRoad(OD->FunctionId)) continue;
					const FVector P=FRotator(0,Other.QuarterTurns*90,0).UnrotateVector(World-FVector(Other.Anchor));
					Covered|=P.X>=0 && P.Y>=0 && P.X<OD->Footprint.Max.X && P.Y<OD->Footprint.Max.Y;
				}
				if(!Covered) Box({X+50,Y+50,10},{100,100,20},TEXT("Asphalt"));
			}
		}
	}
	else if(!bOfficeOnly && (!bPreview || !bOpeningOnly)) Box({W/2,D/2,10},{W,D,20},(Building || Def->FunctionId==TEXT("Walkway"))?TEXT("Concrete"):TEXT("Asphalt"));
    if(Def->FunctionId==TEXT("Walkway")) return Actor;
    if(bStarterProperty && Def->FunctionId==TEXT("Road2"))
    {
     auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Road/Ground/SM_Ground_400.SM_Ground_400"));
     if(Mesh)
     {
      auto* C=NewObject<UInstancedStaticMeshComponent>(Actor);C->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static);Actor->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);
      if(bPreview) C->SetMaterial(0,GhostMat);
      C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
      for(int32 X=0;X+400<=W;X+=400) for(int32 Y=0;Y+400<=D;Y+=400) C->AddInstance(FTransform(FVector(X,Y,20.05)));
     }
    }
	if(!Building)
	{
        if(Def->FunctionId==TEXT("FreightYard"))
        {
         // 4m x 22m trailer bays face the shop; retain a minimum 30m manoeuvring apron.
         for(float Y=400;W>=5200 && Y+400<=D-200;Y+=500)
         {
          Box({1100,Y,20.2},{2200,12,.4},TEXT("White"),false);
          Box({1100,Y+400,20.2},{2200,12,.4},TEXT("White"),false);
          Box({2200,Y+200,20.2},{12,400,.4},TEXT("Yellow"),false);
         }
         for(float Y=100;W>=5200 && Y<D;Y+=200) Box({2400,Y,20.2},{12,100,.4},TEXT("Yellow"),false);
         return Actor;
        }
		if(Def->FunctionId==TEXT("Parking"))
		{
   const auto Entrances=UFoundrylineVehiclePathSubsystem::Entrances(*Placement,O);
   const auto Bays=FFoundrylineDrivingLayout::Parking(W,D,Entrances);
   for(const auto& Bay:Bays) {
    const auto& B=Bay.Bounds;const float Mid=(B.Min.Y+B.Max.Y)/2;
    Box({B.Min.X,Mid,20.2},{10,B.Max.Y-B.Min.Y,.4},TEXT("White"),false);
    Box({B.Max.X,Mid,20.2},{10,B.Max.Y-B.Min.Y,.4},TEXT("White"),false);
    const float End=Bay.Heading<0?B.Min.Y+25:B.Max.Y-25;
    Box({(B.Min.X+B.Max.X)/2,End,20.2},{B.Max.X-B.Min.X,10,.4},TEXT("White"),false);
   }
   TArray<float> Aisles;
   for(const auto& A:FFoundrylineDrivingLayout::ParkingAisles(W,D,{})) if(A.Max.X-A.Min.X==W) Aisles.Add((A.Min.Y+A.Max.Y)/2);
   if(D>=3700 && W>=2400)
   {
				// Raised grass island between back-to-back rows, with 8 m end access.
				if(O.bLandscaping) for(float X=800;X+100<=W-800;X+=100)
				{
					Box({X+50,D/2,26},{100,160,12},TEXT("Landscape"),false);
					Grass({X+50,D/2,32});
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{X,D/2-100,20},90);
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{X+100,D/2+100,20},-90);
				}
				if(O.bLandscaping) for(float Y=D/2-100;Y<D/2+100;Y+=100)
				{
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{800,Y+100,20},0);
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{W-800,Y,20},180);
				}
				Aisles={(550+D/2-700)/2,(D/2+700+D-550)/2};
			}
			for(float AY:Aisles)
			for(float X=400;X<W-200;X+=800)
			{
				Box({X,AY,20.2},{180,15,0.4},TEXT("White"),false);
				Box({X+70,AY-20,20.2},{70,15,0.4},TEXT("White"),false,45);
				Box({X+70,AY+20,20.2},{70,15,0.4},TEXT("White"),false,-45);
			}
			for(float Y=0;Y<D;Y+=100)
			{
				if(!OccupiedOutside({-50,Y+50,0},true)) Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{20,Y+100,20},0);
				if(!OccupiedOutside({W+50,Y+50,0},true)) Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{W-20,Y,20},180);
			}
			for(float X=0;X<W;X+=100)
			{
				if(!OccupiedOutside({X+50,-50,0})) Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{X,20,20},90);
				if(!OccupiedOutside({X+50,D+50,0})) Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{X+100,D-20,20},-90);
			}
			// Grass-filled edging on free outer edges; roads/driveways remain open.
			if(O.bLandscaping) for(int32 Side=0;Side<2;++Side)
			{
				for(float Y=0;Y<D;Y+=100)
				{
					const float X=Side?W+60.f:-60.f;
					if(OccupiedOutside({X,Y+50,0})) continue;
					Box({X,Y+50,18},{120,100,36},TEXT("Landscape"),false);
					Grass({X,Y+50,36});
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{Side?W+120.f:-120.f,Side?Y:Y+100,20},Side?180:0);
				}
				for(float X=0;X<W;X+=100)
				{
					const float Y=Side?D+60.f:-60.f;
					if(OccupiedOutside({X+50,Y,0})) continue;
					Box({X+50,Y,18},{100,120,36},TEXT("Landscape"),false);
					Grass({X+50,Y,36});
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{Side?X+100:X,Side?D+120.f:-120.f,20},Side?-90:90);
				}
			}
            if(O.bLandscaping)
            {
             auto Plant=[&](const TCHAR* Path,FVector Position,float Height,bool Tree)
             {
              if(OccupiedOutside({Position.X,Position.Y,0})) return;
              auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);if(!Mesh) return;
              const FName Key(*FString::Printf(TEXT("ParkingPlant_%s"),*Mesh->GetName()));auto*& C=Groups.FindOrAdd(Key);
              if(!C) {C=NewObject<UInstancedStaticMeshComponent>(Actor);C->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static);Actor->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);C->ComponentTags.Add(TEXT("FoundrylineParkingPlant"));if(bPreview) for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot) C->SetMaterial(Slot,GhostMat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(!bPreview && Tree);C->SetCullDistances(Tree?20000:4500,Tree?30000:6500);C->RegisterComponent();}
              const auto B=Mesh->GetBoundingBox();const float Scale=Height/FMath::Max(1.f,float(B.GetSize().Z));
              C->AddInstance(FTransform(FQuat::Identity,Position-FVector(B.GetCenter().X,B.GetCenter().Y,B.Min.Z)*Scale,FVector(Scale)));
             };
             const TCHAR* Tree=TEXT("/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/Trees/SM_Tree_Birch_1.SM_Tree_Birch_1");
             const TCHAR* Flowers=TEXT("/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/Foliage/SM_Fol_Flower_Clump_1.SM_Fol_Flower_Clump_1");
             for(float Side:{-70.f,W+70.f})
             {
              for(float Y=400;Y<D-200;Y+=600) Plant(Tree,{Side,Y,36},300,true);
              for(float Y=150;Y<D-100;Y+=180) Plant(Flowers,{Side,Y,36},35,false);
             }
             for(float Y:{-70.f,D+70.f}) for(float X=200;X<W-100;X+=180) Plant(Flowers,{X,Y,36},35,false);
            }
			return Actor;
		}
		for(float Y=0;Y<D;Y+=100)
		{
			if(!OccupiedOutside({36,Y+50,0},true) && !OccupiedOutside({-50,Y+50,0},true)) Box({36,Y+50,20.15},{12,100,0.3},TEXT("White"),false);
			if(!OccupiedOutside({W-36,Y+50,0},true) && !OccupiedOutside({W+50,Y+50,0},true)) Box({W-36,Y+50,20.15},{12,100,0.3},TEXT("White"),false);
		}
		if(Def->FunctionId!=TEXT("Service") && Def->FunctionId!=TEXT("Driveway") && Def->FunctionId!=TEXT("TruckAccess"))
		{
			for(float Y=0;Y<D;Y+=100)
				if(!OccupiedOutside({W/2,Y+50,0},true) && !OccupiedOutside({-50,Y+50,0},true) && !OccupiedOutside({W+50,Y+50,0},true) && !O.Crosswalks.ContainsByPredicate([&](int32 C){return FMath::Abs(Y+50-C)<250;}))
					for(float X:{W/2-12,W/2+12}) Box({X,Y+50,20.15},{8,100,0.3},TEXT("Yellow"),false);
			if(Def->FunctionId==TEXT("Road4"))
				for(float Y=50;Y<D;Y+=600)
					if(!OccupiedOutside({W/2,Y+150,0},true) && !OccupiedOutside({-50,Y+150,0},true) && !OccupiedOutside({W+50,Y+150,0},true) && !O.Crosswalks.ContainsByPredicate([&](int32 C){return FMath::Abs(Y+150-C)<400;}))
						for(float X:{375.f,W-375.f}) Box({X,Y+FMath::Min(300.f,D-Y)/2,20.15},{12,FMath::Min(300.f,D-Y),0.3},TEXT("White"),false);
			for(float Y=0;Y<D;Y+=100)
				for(int32 Side=0;Side<2;++Side)
				{
					if(OccupiedOutside({Side?W+110.f:-110.f,Y+50,0})) continue;
					if(O.Crosswalks.ContainsByPredicate([&](int32 C){return FMath::Abs(Y+50-C)<200;}))
					{
						// 18 cm rise over 2 m gives a continuous, step-free curb ramp.
						const float X=Side?W+120.f:-120.f;
						Box({X,Y+50,10},{200,100,20},TEXT("Concrete"));
						auto* Ramp=NewObject<UInstancedStaticMeshComponent>(Actor);Ramp->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static);Actor->AddInstanceComponent(Ramp);Ramp->SetupAttachment(Root);
						Ramp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
						Ramp->SetMaterial(0,bPreview?static_cast<UMaterialInterface*>(GhostMat):Materials.FindRef(TEXT("Concrete")).Get());
						Ramp->SetCollisionEnabled(bPreview?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);Ramp->SetCollisionResponseToAllChannels(ECR_Block);Ramp->SetCastShadow(!bPreview);Ramp->RegisterComponent();
						Ramp->AddInstance(FTransform(FRotator(Side?-5.14f:5.14f,0,0).Quaternion(),{X,Y+50,28},{2.f,1.f,.02f}));
						continue;
					}
					const float X=Side?W+120.f:-120.f;
					Box({X,Y+50,14},{200,100,28},TEXT("Concrete"));
					Box({Side?W+10.f:-10.f,Y+50,10},{20,100,20},TEXT("Concrete"));
					Module(TEXT("/Game/Factory/Meshes/Road/SM_Lane_100.SM_Lane_100"),{Side?W+20.f:-20.f,Side?Y:Y+100,28},Side?180:0);
					Module(TEXT("/Game/Factory/Meshes/Road/Border/SM_Border_100.SM_Border_100"),{Side?W:0,Side?Y:Y+100,20},Side?180:0);
				}
		}
		for(int32 Y:O.Crosswalks)
			for(float X=80;X<W-60;X+=100) if(!OccupiedOutside({X,float(Y),0},true)) Box({X,float(Y),20.3},{50,300,.4},TEXT("White"),false);
		return Actor;
	}
	TArray<FVector2D> Exposed[4];
	for(uint8 Edge=0;Edge<4;++Edge) Exposed[Edge].Add({0,Edge%2==0?W:D});
	for(const auto& Other:Placement->GetObjects())
	{
		if(Other.ObjectId==O.ObjectId || Other.CompanyId!=O.CompanyId || Other.PlantId!=O.PlantId) continue;
		// Shops open into shops and offices into offices. Against a shop, an
		// office keeps its own wall with the connecting door; the shop wall opens.
		if(O.bOfficeAnnex && !Other.bOfficeAnnex) continue;
		const auto* OD=Placement->FindDefinition(Other.DefinitionId);
		if(!OD || OD->FunctionId!=TEXT("BuildingShell")) continue;
		const auto WB=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);
		const FRotator Rot(0,O.QuarterTurns*90,0);
		const FVector A=Rot.UnrotateVector(FVector(WB.Min)-FVector(O.Anchor)), B=Rot.UnrotateVector(FVector(WB.Max)-FVector(O.Anchor));
		const FVector Min=A.ComponentMin(B),Max=A.ComponentMax(B);
		for(uint8 Edge=0;Edge<4;++Edge)
		{
			const bool Touch=Edge==0?FMath::Abs(Max.Y)<0.1:Edge==1?FMath::Abs(Min.X-W)<0.1:Edge==2?FMath::Abs(Min.Y-D)<0.1:FMath::Abs(Max.X)<0.1;
			if(!Touch) continue;
			const float CutA=Edge%2==0?Min.X:Min.Y,CutB=Edge%2==0?Max.X:Max.Y;
			TArray<FVector2D> Remaining;
			for(const auto& Span:Exposed[Edge])
			{
				if(CutB<=Span.X || CutA>=Span.Y) {Remaining.Add(Span); continue;}
				if(CutA>Span.X) Remaining.Add({Span.X,FMath::Min(double(CutA),Span.Y)});
				if(CutB<Span.Y) Remaining.Add({FMath::Max(double(CutB),Span.X),Span.Y});
			}
			Exposed[Edge]=MoveTemp(Remaining);
		}
	}
	for(const auto& Zone:O.OfficeAreas)
	{
		Box({(Zone.Min.X+Zone.Max.X)/2.,(Zone.Min.Y+Zone.Max.Y)/2.,21},{double(Zone.Max.X-Zone.Min.X),double(Zone.Max.Y-Zone.Min.Y),2},TEXT("OfficeFloor"),false);
	}
	auto Furniture=[&](const TCHAR* Path,FVector Center,float Yaw=0.f)
	{
		auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);if(!Mesh) return;
		auto* C=NewObject<UStaticMeshComponent>(Actor);Actor->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);
		if(bPreview) for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot) C->SetMaterial(Slot,GhostMat);
		C->SetCollisionEnabled(bPreview?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetCastShadow(!bPreview);C->RegisterComponent();
		const auto B=Mesh->GetBoundingBox();const FVector Offset(B.GetCenter().X,B.GetCenter().Y,B.Min.Z);
		C->SetRelativeLocationAndRotation(Center-FRotator(0,Yaw,0).RotateVector(Offset),FRotator(0,Yaw,0));
        if(!bPreview)FFoundrylineEquipmentCollision::AddPawnBody(C);
	};
	for(int32 RoomIndex=0;RoomIndex<O.OfficeRooms.Num();++RoomIndex)
	{
		const auto& Room=O.OfficeRooms[RoomIndex];
		const float RW=Room.Max.X-Room.Min.X,RD=Room.Max.Y-Room.Min.Y;
		if(O.bAutoFurnished && RW>=700 && RD>=800)
		{
			const float X=(Room.Min.X+Room.Max.X)/2.f,Y=(Room.Min.Y+Room.Max.Y)/2.f-50;
			Furniture(TEXT("/Game/Factory/Meshes/Office/SM_Table_01.SM_Table_01"),{X,Y,22});
			Furniture(TEXT("/Game/Factory/Meshes/Office/SM_Chairs_01.SM_Chairs_01"),{X,Y+194,22},180);
			Furniture(TEXT("/Game/Factory/Meshes/Office/SM_Flower_01.SM_Flower_01"),{Room.Max.X-150.f,Room.Max.Y-150.f,22});
		}

		TArray<FVector2D> Spans[4];
		for(uint8 Edge=0;Edge<4;++Edge)
		{
			const bool Exterior=Edge==0?Room.Min.Y==0:Edge==1?Room.Max.X==W:Edge==2?Room.Max.Y==D:Room.Min.X==0;
			Spans[Edge].Add({0,Edge%2==0?RW:RD});
			// The host shell owns exterior walls, glazing and doors. Where a joined
			// office has opened that shell, the room still needs its own partition.
			if(Exterior && !bPreview)
			{
				const double Shift=Edge%2==0?Room.Min.X:Room.Min.Y;
				for(const auto& Shell:Exposed[Edge])
				{
					TArray<FVector2D> Remaining;
					for(const auto& Span:Spans[Edge]) {if(Shell.Y-Shift<=Span.X || Shell.X-Shift>=Span.Y) Remaining.Add(Span);else {if(Shell.X-Shift>Span.X) Remaining.Add({Span.X,Shell.X-Shift});if(Shell.Y-Shift<Span.Y) Remaining.Add({Shell.Y-Shift,Span.Y});}}
					Spans[Edge]=MoveTemp(Remaining);
				}
			}
			for(int32 J=0;J<RoomIndex;++J)
			{
				const auto& Other=O.OfficeRooms[J];
				const bool Touch=Edge==0?Room.Min.Y==Other.Max.Y:Edge==1?Room.Max.X==Other.Min.X:Edge==2?Room.Max.Y==Other.Min.Y:Room.Min.X==Other.Max.X;
				if(!Touch) continue;
				const double Low=Edge%2==0?Other.Min.X-Room.Min.X:Other.Min.Y-Room.Min.Y,High=Edge%2==0?Other.Max.X-Room.Min.X:Other.Max.Y-Room.Min.Y;
				TArray<FVector2D> Remaining;for(const auto& Span:Spans[Edge]) {if(High<=Span.X || Low>=Span.Y) Remaining.Add(Span);else {if(Low>Span.X) Remaining.Add({Span.X,Low});if(High<Span.Y) Remaining.Add({High,Span.Y});}}Spans[Edge]=MoveTemp(Remaining);
			}
		}
		auto Partition=[&](uint8 Edge,float A,float B,float Bottom,float Top,FName Mat=TEXT("Wall"),float Thickness=15.f,bool Collision=true)
		{
			if(B<=A || Top<=Bottom) return;
			for(const auto& Span:Spans[Edge])
			{
				const float L=FMath::Max(A,float(Span.X)),R=FMath::Min(B,float(Span.Y));if(R<=L) continue;
				const float U=(L+R)/2,Z=20+(Bottom+Top)/2;
    if(Mat==TEXT("Wall")) Mat=TEXT("Interior");
				if(Edge%2==0) Box({Room.Min.X+U,double(Edge==0?Room.Min.Y:Room.Max.Y),Z},{R-L,Thickness,Top-Bottom},Mat,Collision);
				else Box({double(Edge==3?Room.Min.X:Room.Max.X),Room.Min.Y+U,Z},{Thickness,R-L,Top-Bottom},Mat,Collision);
			}
		};
		for(uint8 Edge=0;Edge<4;++Edge)
		{
			auto Openings=Room.Openings.FilterByPredicate([&](const auto& V){return V.Edge==Edge;});Openings.Sort([](const auto& A,const auto& B){return A.OffsetCm<B.OffsetCm;});float Cursor=0;const float Length=Edge%2==0?RW:RD;
			for(const auto& V:Openings)
			{
				const float A=V.OffsetCm,B=A+V.WidthCm(),Bottom=V.BottomCm(),Top=Bottom+V.HeightCm();
				Partition(Edge,Cursor,A,0,300);Partition(Edge,A,B,0,Bottom);Partition(Edge,A,B,Top,300);Cursor=B;
				Partition(Edge,A-6,A,Bottom,Top,TEXT("Metal"),20);Partition(Edge,B,B+6,Bottom,Top,TEXT("Metal"),20);Partition(Edge,A-6,B+6,Top,Top+6,TEXT("Metal"),20);
				if(V.Kind==EFoundrylineOpeningKind::ShopWindow)
				{
					Partition(Edge,A,B,Bottom,Bottom+6,TEXT("Metal"),20);Partition(Edge,A,B,Bottom+6,Top-6,TEXT("Glass"),2,false);Partition(Edge,(A+B)/2-2,(A+B)/2+2,Bottom,Top,TEXT("Metal"),18);
				}
				else if(Spans[Edge].ContainsByPredicate([&](const auto& Span){return A>=Span.X && B<=Span.Y;})) DoorLeaf(Edge,A,B,FVector2D(Room.Min),FVector2D(Room.Max),true,V.OpeningId,true);
			}
			Partition(Edge,Cursor,Length,0,300);
			float Base=0;
			for(const auto& V:Openings) if(V.Kind!=EFoundrylineOpeningKind::ShopWindow) {Partition(Edge,Base,V.OffsetCm,0,10,TEXT("Metal"),17);Base=V.OffsetCm+V.WidthCm();}
			Partition(Edge,Base,Length,0,10,TEXT("Metal"),17);
			Partition(Edge,0,Length,294,300,TEXT("Metal"),18);
		}
	}
	if(bOfficeOnly) return Actor;
	const float H=Def->Footprint.Max.Z-SlabHeightCm, T=WallThicknessCm;
	auto EdgeBox=[&](uint8 Edge,float A,float B,float Bottom,float Top,FName Mat=TEXT("Wall"),float Depth=30.f,bool Collision=true)
	{
		for(const auto& Span:Exposed[Edge])
		{
			const float Left=FMath::Max(A,float(Span.X)),Right=FMath::Min(B,float(Span.Y));
			if(Right<=Left) continue;
			const float U=(Left+Right)/2,Z=(Bottom+Top)/2+20;
   const FVector Center=Edge%2==0?FVector(U,Edge==0?T/2:D-T/2,Z):FVector(Edge==3?T/2:W-T/2,U,Z);
   const FVector Size=Edge%2==0?FVector(Right-Left,Depth,Top-Bottom):FVector(Depth,Right-Left,Top-Bottom);
   if(Mat==TEXT("Wall") && !bPreview)
   {
    const FVector Inward=Edge==0?FVector(0,1,0):Edge==1?FVector(-1,0,0):Edge==2?FVector(0,-1,0):FVector(1,0,0);
    auto Body=Size;auto Finish=Size;if(Edge%2==0) {Body.Y-=1;Finish.Y=1;}else {Body.X-=1;Finish.X=1;}
    FName BodyFinish=Mat;
    if(O.bOfficeAnnex)
    {
     const FVector Probe=FVector(O.Anchor)+FRotator(0,O.QuarterTurns*90,0).RotateVector(Center-Inward*(Depth/2+50));
     for(const auto& Neighbor:Placement->GetObjects()) if(Neighbor.ObjectId!=O.ObjectId)
     {
      const auto* ND=Placement->FindDefinition(Neighbor.DefinitionId);if(!ND || ND->FunctionId!=TEXT("BuildingShell")) continue;
      const auto NB=FFoundrylineGrid::RotateBounds(ND->Footprint,Neighbor.Anchor,Neighbor.QuarterTurns);
      if(Probe.X>NB.Min.X && Probe.X<NB.Max.X && Probe.Y>NB.Min.Y && Probe.Y<NB.Max.Y) {BodyFinish=TEXT("Interior");break;}
     }
    }
    Box(Center-Inward*.5,Body,BodyFinish,Collision);Box(Center+Inward*(Depth/2-.5),Finish,TEXT("Interior"),Collision);
   }
   else Box(Center,Size,Mat,Collision);
		}
	};
	for(uint8 Edge=0;Edge<4;++Edge)
	{
		TArray<FFoundrylineOpening> Openings;
		for(const auto& Opening:O.Openings)
			if(Opening.Edge==Edge && Exposed[Edge].ContainsByPredicate([&](const auto& Span){return Opening.OffsetCm>=Span.X && Opening.OffsetCm+Opening.WidthCm()<=Span.Y;})) Openings.Add(Opening);
		Openings.Sort([](const auto& A,const auto& B){return A.OffsetCm<B.OffsetCm;});
		float Cursor=Edge%2==0?0:T,End=Edge%2==0?W:D-T;
		for(const auto& Opening:Openings)
		{
			const float A=Opening.OffsetCm,B=A+Opening.WidthCm(),Bottom=Opening.BottomCm(),Top=Bottom+Opening.HeightCm();
			if(bPreview && bOpeningOnly) EdgeBox(Edge,A,B,Bottom,Top,TEXT("Preview"),50,false);
			if(!bOpeningOnly) {EdgeBox(Edge,Cursor,A,0,H); EdgeBox(Edge,A,B,0,Bottom); EdgeBox(Edge,A,B,Top,H);}
			Cursor=B;
			const bool IsWindow=Opening.Kind==EFoundrylineOpeningKind::ShopWindow;
			const float Frame=IsWindow || O.bOfficeAnnex?6:12,FrameDepth=O.bOfficeAnnex?36:42;
			EdgeBox(Edge,A-Frame,A+(IsWindow?Frame:0),Bottom,Top,TEXT("Metal"),FrameDepth);
			EdgeBox(Edge,B-(IsWindow?Frame:0),B+Frame,Bottom,Top,TEXT("Metal"),FrameDepth);
			EdgeBox(Edge,A-Frame,B+Frame,Top-(IsWindow?Frame:0),Top+Frame,TEXT("Metal"),FrameDepth);
			if(Opening.Kind==EFoundrylineOpeningKind::ShopWindow)
			{
				EdgeBox(Edge,A-6,B+6,Bottom-6,Bottom+6,TEXT("Metal"),42);
				EdgeBox(Edge,A+6,B-6,Bottom+6,Top-6,TEXT("Glass"),2);
				EdgeBox(Edge,(A+B)/2-2,(A+B)/2+2,Bottom,Top,TEXT("Metal"),34);
			}
			else if(Opening.Kind==EFoundrylineOpeningKind::Garage || Opening.Kind==EFoundrylineOpeningKind::TruckGarage || Opening.Kind==EFoundrylineOpeningKind::LoadingDock)
			{
				EdgeBox(Edge,A-20,B+20,Top+12,Top+65,TEXT("Metal"),70);
				for(float Z=Top+18;Z<Top+65;Z+=10) EdgeBox(Edge,A-15,B+15,Z,Z+3,TEXT("White"),72,false);
			}
			else DoorLeaf(Edge,A,B,{0,0},{W,D},O.bOfficeAnnex,Opening.OpeningId);
            if(Opening.Kind==EFoundrylineOpeningKind::LoadingDock)
            {
             const float Along=(A+B)/2;
             const FVector Outward=Edge==0?FVector(0,-1,0):Edge==1?FVector(1,0,0):Edge==2?FVector(0,1,0):FVector(-1,0,0);
             const FVector WallPoint=Edge%2==0?FVector(Along,Edge==0?0:D,0):FVector(Edge==3?0:W,Along,0);
             auto DockBox=[&](float Across,float Distance,float Z,float Width,float Depth,float Height,FName Finish,bool Collision=true)
             {
              FVector P=WallPoint+Outward*Distance+(Edge%2==0?FVector(Across,0,Z):FVector(0,Across,Z));
              Box(P,Edge%2==0?FVector(Width,Depth,Height):FVector(Depth,Width,Height),Finish,Collision);
             };
             // Grade-level loading bay: no raised platform or internal ramp.
             for(float Side:{-1.f,1.f})
             {
              DockBox(Side*190,10,100,25,20,160,TEXT("Metal"));
              DockBox(Side*245,80,100,15,15,160,TEXT("Yellow"));
             }
             for(float Across=-225;Across<=225;Across+=50) DockBox(Across,295,20.5,25,10,1,TEXT("Yellow"),false);
             DockBox(0,70,410,500,140,15,TEXT("Metal"));
            }
		}
		if(!(bPreview && bOpeningOnly))
		{
			EdgeBox(Edge,Cursor,End,0,H);
			// The cap stands 2 cm proud of the wall so its top never z-fights the wall's top face.
			EdgeBox(Edge,0,Edge%2==0?W:D,H-(O.bOfficeAnnex?10:18),H+2,TEXT("Metal"),O.bOfficeAnnex?38:46);
			float BaseCursor=0;
			const float BaseHeight=O.bOfficeAnnex?12:60,BaseDepth=O.bOfficeAnnex?34:44;
			const FName BaseFinish=O.bOfficeAnnex?FName(TEXT("Metal")):FName(TEXT("Concrete"));
			for(const auto& Opening:Openings)
			{
				if(Opening.Kind==EFoundrylineOpeningKind::ShopWindow) continue;
				EdgeBox(Edge,BaseCursor,Opening.OffsetCm,0,BaseHeight,BaseFinish,BaseDepth);
				BaseCursor=Opening.OffsetCm+Opening.WidthCm();
			}
			EdgeBox(Edge,BaseCursor,Edge%2==0?W:D,0,BaseHeight,BaseFinish,BaseDepth);
			if(O.bOfficeAnnex) {
				const float Length=Edge%2==0?W:D;
				// Slim corner casings and a continuous fascia suit a 3 m office;
				// shop-sized concrete skirts and pilasters overwhelm its walls.
				EdgeBox(Edge,0,8,0,H,TEXT("Metal"),34);
				EdgeBox(Edge,Length-8,Length,0,H,TEXT("Metal"),34);
			}
			if(!O.bOfficeAnnex && H>=WallHeightCm)
			{
				const float Length=Edge%2==0?W:D;
				for(float Bay:FFoundrylineFacadeLayout::Columns(Length,O.ColumnBayCm))
				{
                    float U=Bay;
                    for(const auto& V:Openings) if(V.Kind==EFoundrylineOpeningKind::ShopWindow && U+28>V.OffsetCm-6 && U-28<V.OffsetCm+V.WidthCm()+6) U=V.OffsetCm-50;
					const bool DoorGap=Openings.ContainsByPredicate([&](const auto& Opening){return Opening.Kind!=EFoundrylineOpeningKind::ShopWindow && U+30>Opening.OffsetCm && U-30<Opening.OffsetCm+Opening.WidthCm();});
					if(DoorGap) continue;
					// Full-height pilasters sit beside glazing rather than ending at the sill.
					EdgeBox(Edge,U-18,U+18,0,H,TEXT("Metal"),58);
					EdgeBox(Edge,U-28,U+28,0,24,TEXT("Metal"),70);
					EdgeBox(Edge,U-28,U+28,H-30,H,TEXT("Metal"),70);
				}
			}

		}
	}
 if(Building)
 {
  const float RoofZ=Def->Footprint.Max.Z;
  // Height-change walls are structural walls, not roof panels: they remain
  // visible and collidable when the local player hides roofs. The shared shop floor stays open.
  for(const auto& Other:Placement->GetObjects())
  {
   if(Other.ObjectId==O.ObjectId || Other.CompanyId!=O.CompanyId || Other.PlantId!=O.PlantId) continue;
   const auto* OD=Placement->FindDefinition(Other.DefinitionId);if(!OD || OD->FunctionId!=TEXT("BuildingShell")) continue;
   const float Bottom=Other.Anchor.Z+OD->Footprint.Max.Z-O.Anchor.Z;if(Bottom>=RoofZ) continue;
   const auto StepBounds=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);const FRotator Rotation(0,O.QuarterTurns*90,0);
   const auto StepMin=Rotation.UnrotateVector(FVector(StepBounds.Min)-FVector(O.Anchor)),StepMax=Rotation.UnrotateVector(FVector(StepBounds.Max)-FVector(O.Anchor));const auto Min=StepMin.ComponentMin(StepMax),Max=StepMin.ComponentMax(StepMax);
   for(int32 E=0;E<4;++E)
   {
    const bool Touch=E==0?FMath::Abs(Max.Y)<.1:E==1?FMath::Abs(Min.X-W)<.1:E==2?FMath::Abs(Min.Y-D)<.1:FMath::Abs(Max.X)<.1;if(!Touch) continue;
    const float L=FMath::Max(0.f,float(E%2==0?Min.X:Min.Y)),R=FMath::Min(E%2==0?W:D,float(E%2==0?Max.X:Max.Y));if(R<=L) continue;
    auto StepBox=[&](float A,float B,float Low,float High,FName Finish,float Depth=30.f)
    {
     A=FMath::Max(A,L);B=FMath::Min(B,R);Low=FMath::Max(Low,Bottom);High=FMath::Min(High,RoofZ);if(B<=A || High<=Low) return;
     const float U=(A+B)/2,Mid=(Low+High)/2;
     const FVector Center=E%2==0?FVector(U,E==0?15:D-15,Mid):FVector(E==3?15:W-15,U,Mid);
     const FVector Inward=E==0?FVector(0,1,0):E==1?FVector(-1,0,0):E==2?FVector(0,-1,0):FVector(1,0,0);
     auto Size=[&](float Thickness){return E%2==0?FVector((B-A)/100,Thickness/100,(High-Low)/100):FVector(Thickness/100,(B-A)/100,(High-Low)/100);};
     if(Finish==TEXT("Wall"))
     {
      Box(Center-Inward*.5,Size(29)*100,TEXT("Wall"));
      Box(Center+Inward*14.5,Size(1)*100,TEXT("Interior"));
     }
     else Box(Center,Size(Depth)*100,Finish);
    };
    auto Windows=O.Openings.FilterByPredicate([&](const auto& V){return V.Edge==E && V.Kind==EFoundrylineOpeningKind::ShopWindow && V.OffsetCm>=L && V.OffsetCm+V.WidthCm()<=R && V.BottomCm()+20>Bottom;});
    Windows.Sort([](const auto& A,const auto& B){return A.OffsetCm<B.OffsetCm;});
    float Cursor=L;
    for(const auto& V:Windows)
    {
     const float A=V.OffsetCm,B=A+V.WidthCm(),Low=V.BottomCm()+20,High=Low+V.HeightCm();
     StepBox(Cursor,A,Bottom,RoofZ,TEXT("Wall"));StepBox(A,B,Bottom,Low,TEXT("Wall"));StepBox(A,B,High,RoofZ,TEXT("Wall"));Cursor=B;
     StepBox(A-6,A+6,Low,High,TEXT("Metal"),42);StepBox(B-6,B+6,Low,High,TEXT("Metal"),42);
     StepBox(A-6,B+6,Low-6,Low+6,TEXT("Metal"),42);StepBox(A-6,B+6,High-6,High+12,TEXT("Metal"),42);
     StepBox(A+6,B-6,Low+6,High-6,TEXT("Glass"),2);StepBox((A+B)/2-2,(A+B)/2+2,Low,High,TEXT("Metal"),34);
    }
    StepBox(Cursor,R,Bottom,RoofZ,TEXT("Wall"));
    StepBox(L,R,Bottom,Bottom+16,TEXT("Metal"),46);StepBox(L,R,RoofZ-18,RoofZ,TEXT("Metal"),46);
    if(!O.bOfficeAnnex && H>=WallHeightCm) for(float Bay:FFoundrylineFacadeLayout::Columns(E%2==0?W:D,O.ColumnBayCm))
    {
     float U=Bay;
     for(const auto& V:Windows) if(U+28>V.OffsetCm-6 && U-28<V.OffsetCm+V.WidthCm()+6) U=V.OffsetCm-50;
     StepBox(U-18,U+18,Bottom,RoofZ,TEXT("Metal"),58);
     StepBox(U-28,U+28,RoofZ-30,RoofZ,TEXT("Metal"),70);
    }
   }
  }
 }
 if(!bPreview && Building)
 {
  const float RoofZ=Def->Footprint.Max.Z;
  auto* RoofParent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Buildings/Roofs/M_RoofFinish.M_RoofFinish"));
  auto* RoofFinish=RoofParent?UMaterialInstanceDynamic::Create(RoofParent,Actor):nullptr;
  if(RoofFinish) {RoofFinish->SetVectorParameterValue(TEXT("ExteriorTint"),O.Colors.Trim);RoofFinish->SetVectorParameterValue(TEXT("InteriorTint"),O.Colors.Interior);}
  auto AddRoof=[&](UStaticMesh* Mesh,FVector Position,FVector Scale,UMaterialInterface* Material=nullptr)
  {
   if(!Mesh) return;
   const FName Key(*FString::Printf(TEXT("Roof_%s_%s"),*Mesh->GetName(),*GetNameSafe(Material)));auto*& C=Groups.FindOrAdd(Key);
   if(!C)
   {
    C=NewObject<UInstancedStaticMeshComponent>(Actor);C->SetMobility(bPreview?EComponentMobility::Movable:EComponentMobility::Static);Actor->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);
    if(Material) C->SetMaterial(0,Material);
    if(RoofFinish && (Mesh->GetName().StartsWith(TEXT("SM_Roof_")) || Mesh->GetName()==TEXT("SM_Skylight_400"))) for(int32 Slot=0;Slot<FMath::Min(2,Mesh->GetStaticMaterials().Num());++Slot) C->SetMaterial(Slot,RoofFinish);
    if(Mesh->GetName()==TEXT("SM_Skylight_400")) {C->SetAffectDistanceFieldLighting(false);C->SetVisibleInRayTracing(false);}
    C->ComponentTags.Add(TEXT("FoundrylineRoof"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
    C->SetCastShadow(Material!=Materials.FindRef(TEXT("Glass")).Get());C->SetVisibility(bRoofsVisible);C->RegisterComponent();
   }
   C->AddInstance(FTransform(FQuat::Identity,Position,Scale));
  };
  auto* Panel=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Factory/Meshes/Roof/SM_Roof_400.SM_Roof_400"));
  auto* Skylight=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Foundryline/Buildings/Roofs/SM_Skylight_400.SM_Skylight_400"));
  for(int32 X=0;X<W;X+=400) for(int32 Y=0;Y<D;Y+=400)
  {
   const float Width=FMath::Min(400.f,W-X),Depth=FMath::Min(400.f,D-Y);
   if(Width==400 && Depth==400)
   {
    const bool Window=(X/400)%3==1 && (Y/400)%3==1;
    // Vendor modules pivot at the maximum XY corner and keep native 1:1 scale.
    AddRoof(Window?Skylight:Panel,FVector(X+400,Y+400,RoofZ),FVector::OneVector,Window?nullptr:Painted[TEXT("Metal")]);
   }
   else AddRoof(Cube,FVector(X+Width/2,Y+Depth/2,RoofZ+10),FVector(Width/100,Depth/100,.2),Painted[TEXT("Metal")]);
  }
  AddCeilingLights(Actor,O,FVector(W,D,RoofZ));
 }
	return Actor;
}

void AFoundrylineBuildSite::SetRoofsVisible(bool Visible)
{
 bRoofsVisible=Visible;
 for(const auto& Pair:Visuals) if(IsValid(Pair.Value))
 {
  TInlineComponentArray<USceneComponent*> Components;Pair.Value->GetComponents(Components);
  for(auto* C:Components) if(C->ComponentHasTag(TEXT("FoundrylineRoof"))) C->SetVisibility(Visible);
 }
}

void AFoundrylineBuildSite::Changed(const FFoundrylinePlacementChange& Change)
{
 if(Change.bHadPrevious && Change.bHasCurrent) {
  auto WithoutZones=Change.Current;WithoutZones.TruckZones=Change.Previous.TruckZones;WithoutZones.RouteNodes=Change.Previous.RouteNodes;
  if(FFoundrylinePlacedObject::StaticStruct()->CompareScriptStruct(&WithoutZones,&Change.Previous,0)) {PublishNetworkState();return;}
 }

	if(auto* Existing=Visuals.Find(Change.ObjectId)) {if(IsValid(*Existing)) {TInlineComponentArray<UFoundrylinePersonnelDoorComponent*> Doors;(*Existing)->GetComponents(Doors);if(HasAuthority()) for(auto* Door:Doors) DoorStates.Add(Door->OpeningId,Door->IsOpen());(*Existing)->Destroy();} Visuals.Remove(Change.ObjectId);}
	if(Change.bHasCurrent)
	{
		Visuals.Add(Change.ObjectId,Render(Change.Current));
		if(!Change.bHadPrevious || Change.Previous.Anchor!=Change.Current.Anchor || Change.Previous.DefinitionId!=Change.Current.DefinitionId)
		{
			const auto* Def=Placement->FindDefinition(Change.Current.DefinitionId);
			const bool HasSidewalk=Def && (Def->FunctionId==TEXT("Road2") || Def->FunctionId==TEXT("Road4"));
			ClearGrass(Change.CurrentBounds,HasSidewalk?250:(Def && Def->FunctionId==TEXT("Parking"))?150:30);
		}
	}
	if(!Change.bAffectsNavigation) {PublishNetworkState();return;}
	// Adjoining sections share an exposed perimeter. Rebuild only nearby shells,
	// including on undo so the original exterior and windows return.
	auto Affected=Change.bHasCurrent?Change.CurrentBounds:Change.PreviousBounds;
	if(Change.bHasCurrent && Change.bHadPrevious) {Affected.Min=Affected.Min.ComponentMin(Change.PreviousBounds.Min);Affected.Max=Affected.Max.ComponentMax(Change.PreviousBounds.Max);}
	for(const auto& Other:Placement->GetObjects())
	{
		if(Other.ObjectId==Change.ObjectId) continue;
		const auto* OD=Placement->FindDefinition(Other.DefinitionId);
		if(!OD) continue;
		const auto B=FFoundrylineGrid::RotateBounds(OD->Footprint,Other.Anchor,Other.QuarterTurns);
		if(B.Max.X<Affected.Min.X-250 || B.Min.X>Affected.Max.X+250 || B.Max.Y<Affected.Min.Y-250 || B.Min.Y>Affected.Max.Y+250) continue;
		if(auto* Visual=Visuals.Find(Other.ObjectId)) {if(IsValid(*Visual)) {TInlineComponentArray<UFoundrylinePersonnelDoorComponent*> Doors;(*Visual)->GetComponents(Doors);if(HasAuthority()) for(auto* Door:Doors) DoorStates.Add(Door->OpeningId,Door->IsOpen());(*Visual)->Destroy();}}
		Visuals.Add(Other.ObjectId,Render(Other));
	}
 PublishNetworkState();
}

void AFoundrylineBuildSite::ClearGrass(const FFoundrylineGridBounds& B,int32 MarginCm)
{
	const FBox Area(FVector(B.Min.X-MarginCm,B.Min.Y-MarginCm,-100),FVector(B.Max.X+MarginCm,B.Max.Y+MarginCm,200));
	for(TActorIterator<AActor> It(GetWorld());It;++It)
	{
		TInlineComponentArray<UFoliageInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
		for(auto* Comp:Components)
			if(Comp->GetStaticMesh())
				Comp->RemoveInstances(Comp->GetInstancesOverlappingBox(Area,true));
	}
}

void AFoundrylineBuildSite::SeedSite()
{
	if(bStarterProperty) {SeedStarterProperty();return;}
	if(bLargeShowcase)
	{
		BuildRectangle(TEXT("Road2"),{-400,-8000,0},{-400,-1600,0});
		BuildRectangle(TEXT("Road2"),{-400,-1600,0},{-400,4800,0});
		BuildRectangle(TEXT("Building"),{400,-2400,0},{5200,0,0});
		BuildRectangle(TEXT("Shop2"),{400,0,0},{5200,3600,0});
		BuildRectangle(TEXT("Parking"),{-7200,-2400,0},{-800,2400,0});
		BuildRectangle(TEXT("Service"),{0,-800,0},{400,-800,0});
		BuildRectangle(TEXT("Road2"),{0,2200,0},{400,2200,0});
		PlaceDoor(TEXT("Personnel"),{400,-1800,0});
		PlaceDoor(TEXT("Garage"),{400,-800,0});
		PlaceDoor(TEXT("TruckDoor"),{400,2200,0});
		OfficeRectangle(TEXT("OfficeArea"),{5200,-2400,0},{8000,0,0});
		OfficeRectangle(TEXT("OfficeRoom"),{5200,-2400,0},{6100,-1400,0});
		OfficeRectangle(TEXT("OfficeRoom"),{6300,-2400,0},{7000,-1400,0});
		OfficeRectangle(TEXT("OfficeRoom"),{5200,-1200,0},{7000,0,0});
		OfficeOpening(TEXT("RoomDoor"),{6100,-1800,0});
		OfficeOpening(TEXT("RoomDoor"),{7000,-1800,0});
		OfficeOpening(TEXT("RoomDoor"),{7000,-700,0});
		if(const auto* Office=Placement->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeAnnex;}))
		{
			FFoundrylinePlacementRequest Furnish;Furnish.Action=EFoundrylinePlacementAction::ReplaceObject;Furnish.ObjectId=Office->ObjectId;Furnish.ExpectedRevision=Placement->GetRevision();Furnish.Replacement=*Office;Furnish.Replacement.bAutoFurnished=true;ExecuteConstruction(Furnish);
		}
		BuildRectangle(TEXT("Road2"),{-400,-5600,0},{-5200,-5600,0});
		ApplyGroundTool(TEXT("Crosswalk"),{-400,-4600,0});
		UndoStack.Reset(); return;
	}
	BuildRectangle(TEXT("Road2"),{-400,-5200,0},{-400,1200,0});
	BuildRectangle(TEXT("Building"),{400,-1200,0},{2800,400,0});
	BuildRectangle(TEXT("Parking"),{-3600,-1200,0},{-800,800,0});
	BuildRectangle(TEXT("Service"),{0,-200,0},{400,-200,0});
	PlaceDoor(TEXT("Personnel"),{400,-950,0});
	PlaceDoor(TEXT("Garage"),{400,-200,0});
	BuildRectangle(TEXT("Shop2"),{800,400,0},{2400,1600,0});
	UndoStack.Reset();
}
