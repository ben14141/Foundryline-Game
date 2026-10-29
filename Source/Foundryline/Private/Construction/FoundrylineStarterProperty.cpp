#include "Construction/FoundrylineBuildSite.h"
#include "Companies/FoundrylinePropertyLot.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

void AFoundrylineBuildSite::SeedStarterProperty()
{
 if(bCutPartsStarter){SeedCutPartsShop();return;}
 if(bProductionWorkshop){SeedProductionWorkshop();return;}
 const auto PlantContext=Context;
 // Town-owned frontage cannot be moved, painted or demolished by the plant owner.
 Context.CompanyId=FGuid(0xF001,99,1,1);Context.PlantId=FGuid(0xF001,99,2,1);
 BuildRectangle(TEXT("Road2"),{-8000,-2800,0},{-1600,-2800,0});
 BuildRectangle(TEXT("Road2"),{-1600,-2800,0},{4800,-2800,0});
 Context=PlantContext;
 BuildRectangle(TEXT("Building"),{-2800,1700,0},{-400,3500,0});
 BuildRectangle(TEXT("Parking"),{-3700,-400,0},{-1500,1300,0});
 BuildRectangle(TEXT("Driveway"),{-2400,-2400,0},{-2400,-400,0});
 DraftTruckWidthCm=1200;
 BuildRectangle(TEXT("TruckAccess"),{2000,-2400,0},{2000,0,0});
 BuildRectangle(TEXT("FreightYard"),{0,0,0},{4000,3600,0});
 // Two loading positions need a short apron along the facade, not a full
 // trailer staging yard. Every surface remains ordinary editable construction.
 BuildRectangle(TEXT("FreightYard"),{-400,1700,0},{0,3500,0});
 DraftTruckWidthCm=2400;
 PlaceDoor(TEXT("Personnel"),{-2800,2200,0});
 PlaceDoor(TEXT("Garage"),{-1600,1700,0});
 PlaceDoor(TEXT("LoadingDock"),{-400,2600,0});
 PlaceDoor(TEXT("LoadingDock"),{-400,3200,0});
 OfficeRectangle(TEXT("OfficeArea"),{-2800,3500,0},{-1200,4700,0});
 OfficeRectangle(TEXT("OfficeRoom"),{-2000,3500,0},{-1200,4300,0});
 OfficeOpening(TEXT("RoomDoor"),{-2000,3900,0});
 if(const auto* Office=Placement->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeAnnex;}))
 {
  FFoundrylinePlacementRequest Furnish;Furnish.Action=EFoundrylinePlacementAction::ReplaceObject;Furnish.ObjectId=Office->ObjectId;Furnish.ExpectedRevision=Placement->GetRevision();Furnish.Replacement=*Office;Furnish.Replacement.bAutoFurnished=true;Placement->Execute(Furnish,Context);
 }

 DrawLotBoundary();
 // Ordinary saved construction: snap, move/remove and undo like other paved areas.
 BuildRectangle(TEXT("Walkway"),{-3000,1300,0},{-2800,2200,0});
 UndoStack.Reset();
}

void AFoundrylineBuildSite::SeedProductionWorkshop()
{
 const auto PlantContext=Context;
 Context.CompanyId=FGuid(0xF001,99,1,1);Context.PlantId=FGuid(0xF001,99,2,1);
 BuildRectangle(TEXT("Road2"),{-8000,-2800,0},{-1600,-2800,0});
 BuildRectangle(TEXT("Road2"),{-1600,-2800,0},{4800,-2800,0});Context=PlantContext;
 BuildRectangle(TEXT("Shop2"),{-2400,0,0},{2800,5000,0});
 PlaceDoor(TEXT("Personnel"),{-1600,0,0});
 // Two 5 x 5 m ground-level loading doors, sharing road/slab height. No ramp.
 PlaceDoor(TEXT("TruckDoor"),{2800,1600,0});
 PlaceDoor(TEXT("TruckDoor"),{2800,4300,0});
 PlaceDoor(TEXT("Garage"),{2800,3000,0});
 PlaceDoor(TEXT("Personnel"),{2800,1000,0});
 OfficeRectangle(TEXT("OfficeArea"),{-3600,2400,0},{-2400,4000,0});
 OfficeRectangle(TEXT("OfficeRoom"),{-3200,3200,0},{-2400,4000,0});
 OfficeOpening(TEXT("RoomDoor"),{-3200,3500,0});
 OfficeOpening(TEXT("RoomWindow"),{-3600,2800,0});
 OfficeOpening(TEXT("RoomWindow"),{-2700,4000,0});
 if(const auto* Office=Placement->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeAnnex;})){
  FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Office->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*Office;R.Replacement.bAutoFurnished=true;Placement->Execute(R,Context);
 }
 BuildRectangle(TEXT("Parking"),{-3700,-1900,0},{-1300,-200,0});
 BuildRectangle(TEXT("Driveway"),{-2400,-2400,0},{-2400,-1900,0});
 DraftTruckWidthCm=1200;BuildRectangle(TEXT("TruckAccess"),{4200,-2400,0},{4200,0,0});DraftTruckWidthCm=2400;
 BuildRectangle(TEXT("FreightYard"),{2800,0,0},{5600,5100,0});
 BuildRectangle(TEXT("Walkway"),{-3900,-200,0},{-3600,3300,0});
 BuildRectangle(TEXT("Walkway"),{-3600,-200,0},{-1400,0,0});
 DrawLotBoundary();UndoStack.Reset();
}


void AFoundrylineBuildSite::DrawLotBoundary()
{
 // Ground-level survey markings are presentation only and do not block expansion.
 if(IsValid(BoundaryActor)) return;
 auto* Markers=GetWorld()->SpawnActor<AActor>();BoundaryActor=Markers;Markers->Tags.Add(TEXT("FoundrylineLotBoundary"));
 auto* C=NewObject<UInstancedStaticMeshComponent>(Markers);Markers->AddInstanceComponent(C);Markers->SetRootComponent(C);
 C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetMaterial(0,Materials.FindRef(TEXT("Yellow")));
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->RegisterComponent();
 auto Mark=[&](FVector P,FVector Size){C->AddInstance(FTransform(FQuat::Identity,P,Size/100.f));};
 const auto Lot=FFoundrylinePropertyLot::Starter(Context.CompanyId,Context.PlantId);
 const auto B=Lot.Region.Bounds;
 for(int32 X=B.Min.X;X<B.Max.X;X+=200)
 {
  const bool Gate=(X+100>-2700 && X<-2100) || (bProductionWorkshop?(X+100>3600 && X<4800):(X+100>1400 && X<2600));
  if(!Gate) Mark({double(X+50),double(B.Min.Y),2},{100,8,4});
  Mark({double(X+50),double(B.Max.Y),2},{100,8,4});
 }
 for(int32 Y=B.Min.Y;Y<B.Max.Y;Y+=200) for(int32 X:{B.Min.X,B.Max.X}) Mark({double(X),double(Y+50),2},{8,100,4});
}

void AFoundrylineBuildSite::SeedCutPartsShop(){
 const auto Owned=Context;Context.CompanyId=FGuid(0xF001,99,1,1);Context.PlantId=FGuid(0xF001,99,2,1);
 BuildRectangle(TEXT("Road2"),{-8000,-2800,0},{-1600,-2800,0});BuildRectangle(TEXT("Road2"),{-1600,-2800,0},{4800,-2800,0});BuildRectangle(TEXT("Road2"),{4800,-2800,0},{10000,-2800,0});Context=Owned;
 BuildRectangle(TEXT("Building"),{-2400,0,0},{400,2200,0});
 PlaceDoor(TEXT("Personnel"),{-2100,0,0});PlaceDoor(TEXT("TruckDoor"),{400,1700,0});
 OfficeRectangle(TEXT("OfficeArea"),{-3200,400,0},{-2400,1400,0});
 OfficeRectangle(TEXT("OfficeRoom"),{-3200,400,0},{-2600,800,0});
 OfficeRectangle(TEXT("OfficeRoom"),{-3200,800,0},{-2600,1200,0});
 OfficeOpening(TEXT("RoomDoor"),{-2600,600,0});OfficeOpening(TEXT("RoomDoor"),{-2600,1000,0});
 OfficeOpening(TEXT("RoomWindow"),{-3200,600,0});OfficeOpening(TEXT("RoomWindow"),{-3200,1000,0});
 if(const auto* Office=Placement->GetObjects().FindByPredicate([](const auto& O){return O.bOfficeAnnex;})){
  FFoundrylinePlacementRequest R;R.Action=EFoundrylinePlacementAction::ReplaceObject;R.ObjectId=Office->ObjectId;R.ExpectedRevision=Placement->GetRevision();R.Replacement=*Office;R.Replacement.bAutoFurnished=true;Placement->Execute(R,Context);
 }
 BuildRectangle(TEXT("Parking"),{-3700,-1800,0},{-1600,-300,0});
 BuildRectangle(TEXT("Driveway"),{-2400,-2400,0},{-2400,-1800,0});
 BuildRectangle(TEXT("FreightYard"),{400,-200,0},{5600,3600,0});
 DraftTruckWidthCm=1400;BuildRectangle(TEXT("TruckAccess"),{4200,-2400,0},{4200,-200,0});DraftTruckWidthCm=2400;
 BuildRectangle(TEXT("Walkway"),{-3500,-300,0},{-3200,1400,0});BuildRectangle(TEXT("Walkway"),{-3200,-300,0},{-1900,0,0});
 DrawLotBoundary();UndoStack.Reset();
}
