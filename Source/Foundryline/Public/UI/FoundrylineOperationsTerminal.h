#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoundrylineOperationsTerminal.generated.h"
UCLASS()
class FOUNDRYLINE_API AFoundrylineOperationsTerminal:public AActor {
 GENERATED_BODY()
public:
 AFoundrylineOperationsTerminal();
 virtual void BeginPlay()override;
 virtual void EndPlay(const EEndPlayReason::Type Reason)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 UPROPERTY(Replicated) int32 Terminal=1;
 UPROPERTY(Replicated) FGuid CompanyId=FGuid(0xF001,1,1,1);
 UPROPERTY(Replicated) FGuid PlantId=FGuid(0xF001,2,1,1);
 FVector InteractionPoint()const{return GetActorLocation()+FVector(0,0,Terminal==1?111:113);}
private:
 void RefreshDesktop();
 UFUNCTION() void DrawDesktop(class UCanvas* Canvas,int32 Width,int32 Height);
 UFUNCTION() void DrawLeftDesktop(class UCanvas* Canvas,int32 Width,int32 Height);
 UFUNCTION() void DrawRightDesktop(class UCanvas* Canvas,int32 Width,int32 Height);
 void DrawScreen(class UCanvas* Canvas,int32 Width,int32 Height,int32 View);
 FTimerHandle DesktopTimer;
 UPROPERTY() TObjectPtr<class UCanvasRenderTarget2D> Desktop;
 UPROPERTY() TObjectPtr<class UCanvasRenderTarget2D> LeftDesktop;
 UPROPERTY() TObjectPtr<class UCanvasRenderTarget2D> RightDesktop;
 UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> SideMaterials;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> DesktopMaterial;
 UPROPERTY() TObjectPtr<class UFont> DesktopFont;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
};
