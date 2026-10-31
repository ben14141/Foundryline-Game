#include "Construction/FoundrylineBuildSite.h"
#include "Construction/FoundrylinePavementGeometry.h"
#include "Pathing/FoundrylineDrivingLayout.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FoliageInstancedStaticMeshComponent.h"

namespace {
using G=FFoundrylinePavementGeometry;
struct MeshData {
 TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FProcMeshTangent> Tangents;
 void Triangle(FVector A,FVector B,FVector C){const int I=V.Num();const FVector Normal=FVector::CrossProduct(B-A,C-A).GetSafeNormal();V.Append({A,B,C});N.Append({Normal,Normal,Normal});for(const auto P:{A,B,C}){UV.Add({P.X/100,P.Y/100});Tangents.Add(FProcMeshTangent(1,0,0));}T.Append({I,I+2,I+1});}
 void Quad(FVector A,FVector B,FVector C,FVector D){Triangle(A,B,C);Triangle(A,C,D);}
 void Surface(const TArray<G::FPolygon>& Shape,double Z){for(const auto& P:Shape){TArray<FVector2D> Vertices;TArray<int32> Tri;if(!G::Triangulate(P,Vertices,Tri))continue;for(int I=0;I<Tri.Num();I+=3)Triangle(FVector(Vertices[Tri[I]],Z),FVector(Vertices[Tri[I+1]],Z),FVector(Vertices[Tri[I+2]],Z));}}
};
}
AActor* AFoundrylineBuildSite::RenderPavement(const FFoundrylinePlacedObject& O,bool Preview,bool Valid,bool PaintOnly){
 auto* Actor=GetWorld()->SpawnActor<AActor>();Actor->Tags.Add(TEXT("FoundrylinePropertyVisual"));Actor->Tags.Add(TEXT("FoundrylinePavementVisual"));if(Preview)Actor->Tags.Add(TEXT("FoundrylinePlacementPreview"));
 auto* Root=NewObject<USceneComponent>(Actor);Actor->SetRootComponent(Root);Root->RegisterComponent();Actor->SetActorLocationAndRotation(FVector(O.Anchor),FRotator(0,O.QuarterTurns*90,0));if(!Preview)Root->SetMobility(EComponentMobility::Static);
 auto Material=[&](const TCHAR* Name,const TCHAR* Fallback){const FString Path=FString::Printf(TEXT("/Game/Foundryline/Construction/Materials/%s.%s"),Name,Name);auto* M=LoadObject<UMaterialInterface>(nullptr,*Path);return M?M:Materials.FindRef(FName(Fallback)).Get();};
 auto* Asphalt=Material(TEXT("M_PavementAsphalt"),TEXT("Asphalt"));auto* Concrete=Material(TEXT("M_CurbConcrete"),TEXT("Concrete"));auto* White=Material(TEXT("M_RoadPaintWhite"),TEXT("White"));auto* Yellow=Material(TEXT("M_RoadPaintYellow"),TEXT("Yellow"));
 UMaterialInstanceDynamic* Ghost=nullptr;if(Preview){Ghost=UMaterialInstanceDynamic::Create(Materials.FindRef(TEXT("Preview")),Actor);Ghost->SetVectorParameterValue(TEXT("Tint"),Valid?FLinearColor(.03f,.45f,1):FLinearColor(1,.03f,.01f));}
 auto Mesh=[&](MeshData& D,UMaterialInterface* M,bool Collision){if(D.T.IsEmpty())return;auto* C=NewObject<UProceduralMeshComponent>(Actor);C->SetupAttachment(Root);C->SetMobility(Preview?EComponentMobility::Movable:EComponentMobility::Static);C->bUseAsyncCooking=false;C->bUseComplexAsSimpleCollision=true;C->SetCollisionEnabled(!Preview&&Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetCanEverAffectNavigation(false);C->SetCastShadow(!Preview&&Collision);C->RegisterComponent();Actor->AddInstanceComponent(C);C->CreateMeshSection_LinearColor(0,D.V,D.T,D.N,D.UV,TArray<FLinearColor>(),D.Tangents,!Preview&&Collision);C->SetMaterial(0,Preview?static_cast<UMaterialInterface*>(Ghost):M);};
 const FTransform World(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));
 auto OtherPaved=[&](FVector2D Local){const auto W=World.TransformPosition(FVector(Local,20));for(const auto& Other:Placement->GetObjects()){if(Other.ObjectId==O.ObjectId)continue;const auto* Def=Placement->FindDefinition(Other.DefinitionId);if(!Def)continue;const FVector P=FRotator(0,Other.QuarterTurns*90,0).UnrotateVector(W-FVector(Other.Anchor));if(!Other.Pavement.Polygons.IsEmpty()){if(G::Contains(Other.Pavement.Polygons,FVector2D(P),.2))return true;}else if((FFoundrylineDrivingLayout::IsPaved(Def->FunctionId)||Def->FunctionId==TEXT("Walkway")||Def->FunctionId==TEXT("BuildingShell"))&&P.X>=-.2&&P.Y>=-.2&&P.X<=Def->Footprint.Max.X+.2&&P.Y<=Def->Footprint.Max.Y+.2)return true;}return false;};
 auto Surface=O.Pavement.Polygons;
 // Stable ownership of overlap removes coplanar triangles at intersections.
 if(!Preview)for(const auto& Other:Placement->GetObjects())if(Other.ObjectId!=O.ObjectId&&Other.ObjectId.ToString()<O.ObjectId.ToString()&&!Other.Pavement.Polygons.IsEmpty()){
  auto Shape=G::Transform(Other.Pavement.Polygons,FTransform(FRotator(0,Other.QuarterTurns*90,0),FVector(Other.Anchor)));Shape=G::Transform(Shape,World.Inverse());if(!G::Bounds(Surface).Intersect(G::Bounds(Shape)))continue;TArray<G::FPolygon> Cut;if(G::Boolean(Surface,Shape,true,Cut))Surface=MoveTemp(Cut);
 }
 MeshData Slab;Slab.Surface(Surface,20);
 for(const auto& P:Surface){auto Side=[&](const auto& Loop){for(int I=0;I<Loop.Num();++I){const auto A=Loop[I],B=Loop[(I+1)%Loop.Num()];if(!OtherPaved((A+B)*.5))Slab.Quad(FVector(A,0),FVector(B,0),FVector(B,20),FVector(A,20));}};Side(P.Outer);for(const auto& H:P.Holes)Side(H.Points);}
 if(!PaintOnly)Mesh(Slab,Asphalt,true);
 if(O.Pavement.Curbs&&!PaintOnly){MeshData Kerb;for(const auto& P:O.Pavement.Polygons){auto Loop=[&](const auto& Points){for(int I=0;I<Points.Num();++I){const auto A=Points[I],B=Points[(I+1)%Points.Num()],Delta=B-A;const double Length=Delta.Size();if(Length<.1)continue;const auto Along=Delta/Length,Inside=FVector2D(-Along.Y,Along.X);const int N=FMath::Max(1,FMath::CeilToInt(Length/25));
   auto Miter=[&](FVector2D Before,FVector2D After){const auto U=(After-Before).GetSafeNormal();const FVector2D N2(-U.Y,U.X);const auto Sum=(Inside+N2).GetSafeNormal();const double Dot=FVector2D::DotProduct(Sum,Inside);return Dot>.4?Sum/Dot:Inside;};
   const auto StartMiter=Miter(Points[(I+Points.Num()-1)%Points.Num()],A),EndMiter=Miter(B,Points[(I+2)%Points.Num()]);
   // Test all sides at 10-inch resolution; connecting pavement or a building opens the curb.
   int Run=-1;auto Emit=[&](double From,double To){const int Pieces=FMath::Max(1,FMath::CeilToInt((To-From)/100));for(int J=0;J<Pieces;++J){const double Low=From+(To-From)*J/Pieces+(J==0&&From<.01?0:.35),High=From+(To-From)*(J+1)/Pieces-(J==Pieces-1&&To>Length-.01?0:.35);if(High<=Low)continue;const FVector2D Start=A+Along*Low,End=A+Along*High,StartNormal=Low<.01?StartMiter:Inside,EndNormal=High>Length-.01?EndMiter:Inside;const FVector2D Profile[]={{0,20},{0,32},{4,35.24},{14,35.24},{18,31},{18,20}};
    for(int K=0;K<5;++K){const auto U=Profile[K],V=Profile[K+1];Kerb.Quad(FVector(Start+StartNormal*U.X,U.Y),FVector(End+EndNormal*U.X,U.Y),FVector(End+EndNormal*V.X,V.Y),FVector(Start+StartNormal*V.X,V.Y));}
    for(int Side:{0,1}){const auto Point=Side?End:Start,Normal=Side?EndNormal:StartNormal;for(int K=1;K<5;++K){const FVector A0(Point+Normal*Profile[0].X,Profile[0].Y),B0(Point+Normal*Profile[K].X,Profile[K].Y),C0(Point+Normal*Profile[K+1].X,Profile[K+1].Y);if(Side)Kerb.Triangle(A0,C0,B0);else Kerb.Triangle(A0,B0,C0);}}
   }};
   for(int J=0;J<=N;++J){const bool Covered=J==N||OtherPaved(A+Along*(Length*(J+.5)/N)-Inside*3);if(!Covered&&Run<0)Run=J;if(Covered&&Run>=0){Emit(Length*Run/N,Length*J/N);Run=-1;}}
  }};Loop(P.Outer);for(const auto& H:P.Holes)Loop(H.Points);}Mesh(Kerb,Concrete,true);}
 MeshData WhitePaint,YellowPaint;
 for(const auto& Mark:O.Pavement.Markings){const bool IsYellow=Mark.Kind==EFoundrylinePavementMark::YellowLine||Mark.Kind==EFoundrylinePavementMark::KeepClear||Mark.Kind==EFoundrylinePavementMark::Loading;auto& Paint=IsYellow?YellowPaint:WhitePaint;const double Width=Mark.Kind==EFoundrylinePavementMark::Stop?40:10.16;TArray<G::FPolygon> Strokes;double AlongMark=0;
  for(const auto& Line:G::PaintLines(Mark)){const double Length=FVector2D::Distance(Line.Key,Line.Value);const auto F=(Line.Value-Line.Key).GetSafeNormal(),R=FVector2D(-F.Y,F.X)*Width/2;
   auto Add=[&](double Low,double High){const int Count=Mark.RoadGenerated?FMath::Max(1,FMath::CeilToInt((High-Low)/50)):1;for(int Part=0;Part<Count;++Part){const double From=FMath::Lerp(Low,High,double(Part)/Count),To=FMath::Lerp(Low,High,double(Part+1)/Count);if(Mark.RoadGenerated&&OtherPaved(Line.Key+F*((From+To)/2)))continue;G::FPolygon P;const auto A=Line.Key+F*From,B=Line.Key+F*To;P.Outer={A-R,B-R,B+R,A+R};Strokes.Add(P);}};
   if(Mark.Kind==EFoundrylinePavementMark::DashedLine){for(double X=-FMath::Fmod(AlongMark,600.);X<Length;X+=600){const double Low=FMath::Max(0.,X),High=FMath::Min(X+300,Length);if(High>Low)Add(Low,High);}}else Add(0,Length);AlongMark+=Length;
  }
  TArray<G::FPolygon> Trimmed;if(PaintOnly||Preview)Trimmed=Strokes;else for(const auto& Stroke:Strokes){TArray<G::FPolygon> Outside;if(!G::Boolean({Stroke},O.Pavement.Polygons,true,Outside))continue;TArray<G::FPolygon> Clipped;if(G::Boolean({Stroke},Outside,true,Clipped))Trimmed.Append(Clipped);}Paint.Surface(Trimmed,20.12);
 }
 Mesh(WhitePaint,White,false);Mesh(YellowPaint,Yellow,false);return Actor;
}
void AFoundrylineBuildSite::ClearPavementGrass(const FFoundrylinePlacedObject& O){
 const auto B=G::Bounds(G::Transform(O.Pavement.Polygons,FTransform(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor))));const FBox Box(FVector(B.Min,-100),FVector(B.Max,200));
 for(TActorIterator<AActor> It(GetWorld());It;++It){TInlineComponentArray<UFoliageInstancedStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto* C:Cs){TArray<int32> Remove;for(const int Id:C->GetInstancesOverlappingBox(Box,true)){FTransform T;C->GetInstanceTransform(Id,T,true);const auto Local=FRotator(0,O.QuarterTurns*90,0).UnrotateVector(T.GetLocation()-FVector(O.Anchor));if(G::Contains(O.Pavement.Polygons,FVector2D(Local),25))Remove.Add(Id);}if(!Remove.IsEmpty())C->RemoveInstances(Remove);}}
}

bool AFoundrylineBuildSite::RenderRoadsideJoins(AActor* Actor,const FFoundrylinePlacedObject& O){
 const auto* Def=Placement->FindDefinition(O.DefinitionId);if(!Def||!Actor)return false;const double W=Def->Footprint.Max.X,D=Def->Footprint.Max.Y;const FTransform T(FRotator(0,O.QuarterTurns*90,0),FVector(O.Anchor));
 TArray<TArray<G::FPolygon>> Cuts;bool HasCurve=false;
 for(const auto& Other:Placement->GetObjects()){if(Other.ObjectId==O.ObjectId)continue;const auto* OD=Placement->FindDefinition(Other.DefinitionId);if(!OD||!FFoundrylineDrivingLayout::IsPaved(OD->FunctionId))continue;
  auto Shape=Other.Pavement.Polygons.IsEmpty()?G::Rectangle({0,0},{double(OD->Footprint.Max.X),double(OD->Footprint.Max.Y)}):Other.Pavement.Polygons;Shape=G::Transform(Shape,FTransform(FRotator(0,Other.QuarterTurns*90,0),FVector(Other.Anchor)));Shape=G::Transform(Shape,T.Inverse());if(!G::Bounds(Shape).Intersect(FBox2D({-250,0},{W+250,D})))continue;
  HasCurve|=Other.Pavement.EntranceRadiusCm>0&&Other.Pavement.EntranceEnds!=0;Cuts.Add(MoveTemp(Shape));
 }
 if(!HasCurve)return false;
 MeshData Mesh;for(int Side=0;Side<2;++Side){auto Shape=G::Rectangle({Side?W+20:-220,0},{Side?W+220:-20,D});
  for(const auto& Cut:Cuts){TArray<G::FPolygon> Result;if(G::Boolean(Shape,Cut,true,Result))Shape=MoveTemp(Result);}
  for(const int Y:O.Crosswalks){TArray<G::FPolygon> Result;if(G::Boolean(Shape,G::Rectangle({-250,double(Y)-200},{W+250,double(Y)+200}),true,Result))Shape=MoveTemp(Result);}
  Mesh.Surface(Shape,28);for(const auto& P:Shape){auto Sides=[&](const auto& Points){for(int I=0;I<Points.Num();++I){const auto A=Points[I],B=Points[(I+1)%Points.Num()];Mesh.Quad(FVector(A,0),FVector(B,0),FVector(B,28),FVector(A,28));}};Sides(P.Outer);for(const auto& H:P.Holes)Sides(H.Points);}
 }
 if(Mesh.T.IsEmpty())return true;auto* C=NewObject<UProceduralMeshComponent>(Actor);C->SetupAttachment(Actor->GetRootComponent());C->SetMobility(EComponentMobility::Static);C->bUseComplexAsSimpleCollision=true;C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("FoundrylineCurvedRoadside"));C->RegisterComponent();Actor->AddInstanceComponent(C);C->CreateMeshSection_LinearColor(0,Mesh.V,Mesh.T,Mesh.N,Mesh.UV,TArray<FLinearColor>(),Mesh.Tangents,true);auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Construction/Materials/M_CurbConcrete.M_CurbConcrete"));C->SetMaterial(0,M?M:Materials.FindRef(TEXT("Concrete")).Get());return true;
}
