#include "Grid/FoundrylineFloorGuide.h"
#include "Components/DecalComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
AFoundrylineFloorGuide::AFoundrylineFloorGuide(){SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Guide")));SetActorEnableCollision(false);}
void AFoundrylineFloorGuide::BeginPlay(){Super::BeginPlay();if(!FApp::CanEverRender())return;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Foundryline/Production/Materials/M_FactoryFloorDecal.M_FactoryFloorDecal"));if(!Base)return;
 auto Texture=[&](bool Grid){const int N=Grid?256:1024;auto* T=UTexture2D::CreateTransient(N,N,PF_B8G8R8A8);T->SRGB=false;T->Filter=TF_Bilinear;T->AddressX=T->AddressY=Grid?TA_Wrap:TA_Clamp;
  auto* Pixels=static_cast<FColor*>(T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
  for(int Y=0;Y<N;++Y)for(int X=0;X<N;++X){uint8 A;if(Grid){const bool Major=X%128<2||Y%128<2,Minor=X%32<1||Y%32<1;A=Major?110:Minor?45:0;}else A=X<2||Y<2||X>N-3||Y>N-3?245:45;Pixels[Y*N+X]=FColor(255,255,255,A);}
  T->GetPlatformData()->Mips[0].BulkData.Unlock();T->UpdateResource();return T;};
 GridTexture=Texture(true);FootprintTexture=Texture(false);
 auto Decal=[&](const TCHAR* Name,UTexture2D* T,UMaterialInstanceDynamic*& M,int Order){auto* D=NewObject<UDecalComponent>(this,Name);D->SetupAttachment(RootComponent);D->SetRelativeRotation(FRotator(-90,0,0));D->SetFadeScreenSize(0);D->SetSortOrder(Order);M=UMaterialInstanceDynamic::Create(Base,this);M->SetTextureParameterValue(TEXT("PaintAtlas"),T);D->SetDecalMaterial(M);D->RegisterComponent();AddInstanceComponent(D);D->SetVisibility(false);return D;};
 UMaterialInstanceDynamic* GM=nullptr,*FM=nullptr;GridDecal=Decal(TEXT("FloorGrid"),GridTexture,GM,20);FootprintDecal=Decal(TEXT("PaintFootprint"),FootprintTexture,FM,21);GridMaterial=GM;FootprintMaterial=FM;
}
void AFoundrylineFloorGuide::Show(const FBox2D& Floor,const FBox2D& Footprint,double Step,bool Grid,bool Valid,bool Visible){if(!GridDecal||!FootprintDecal)return;
 GridDecal->SetVisibility(Visible&&Grid);FootprintDecal->SetVisibility(Visible);if(!Visible)return;
 auto Place=[](UDecalComponent* D,const FBox2D& B){const auto S=B.GetSize();const FVector P(B.GetCenter(),20),Size(1,S.Y/2,S.X/2);if(D->GetComponentLocation().Equals(P,.001)&&D->DecalSize.Equals(Size,.001))return;D->SetWorldLocation(P);D->DecalSize=Size;D->MarkRenderStateDirty();};
 Place(GridDecal,Floor);Place(FootprintDecal,Footprint);const double Period=Step*8;
 GridMaterial->SetVectorParameterValue(TEXT("UVScale"),FLinearColor(Floor.GetSize().X/Period,Floor.GetSize().Y/Period,0,0));
 GridMaterial->SetVectorParameterValue(TEXT("UVOffset"),FLinearColor(Floor.Min.X/Period+.5/256,Floor.Min.Y/Period+.5/256,0,0));
 GridMaterial->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.3,.65,.85,1));
 FootprintMaterial->SetVectorParameterValue(TEXT("Tint"),Valid?FLinearColor(.04,.65,1,1):FLinearColor(1,.035,.01,1));
}
