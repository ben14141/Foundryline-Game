#include "FoundrylineNetworkTestTools.h"
#include "Production/FoundrylineFactoryPlant.h"
#include "Production/FoundrylineWorkshopLayout.h"
#include "Inventory/FoundrylineSheetStock.h"
#include "Logistics/FoundrylineForkliftBeacon.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
bool UFoundrylineNetworkTestTools::StartForkliftLiftReview(APlayerController* Host){
#if WITH_EDITOR
 if(!Host||!Host->HasAuthority()||Host->GetWorld()->WorldType!=EWorldType::PIE)return false;
 for(TActorIterator<AFoundrylineFactoryPlant> It(Host->GetWorld());It;++It){auto& M=It->Model;
  // Four sheets leave the raw-steel rack so it has room, then the real steel purchase books a truck; the forklift unloads it
  // from the truck and lifts it into the rack.
  if(auto* Rack=M.State.Pallets.FindByPredicate([](const auto& P){return P.Place==EFoundrylinePalletPlace::Rack&&P.Part==EFoundrylinePart::Sheet&&P.Quantity-P.Reserved>=4;})){Rack->Quantity-=4;M.State.ReceivedSteelLb-=4*Rack->UnitLb;}
  FFoundrylineCommerceCommand Buy;Buy.Action=EFoundrylineCommerceAction::BuySteel;Buy.Quantity=4;FString E;if(!M.CommerceCommand(Buy,E))return false;M.State.Running=true;++M.State.Revision;return true;}
#endif
 return false;
}
FString UFoundrylineNetworkTestTools::ForkliftBeaconReport(APlayerController* Player){
#if WITH_EDITOR
 if(!Player)return TEXT("{}");FString Items;
 for(TActorIterator<AFoundrylineFactoryPlant> It(Player->GetWorld());It;++It){TArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
  for(auto* Body:Meshes){if(!Body->GetStaticMesh()||Body->GetStaticMesh()->GetName()!=TEXT("SM_Forklift_Body"))continue;USceneComponent* Root=Body->GetAttachParent();if(!Root)continue;
   const UPointLightComponent* Light=nullptr;for(USceneComponent* Child:Root->GetAttachChildren())if(auto* L=Cast<UPointLightComponent>(Child))Light=L;
   float Lift=0;for(auto* M:Meshes)if(M->GetStaticMesh()&&M->GetStaticMesh()->GetName()==TEXT("SM_Forklift_Carriage")&&M->GetAttachParent()&&M->GetAttachParent()->GetAttachParent()==Root)Lift=M->GetAttachParent()->GetRelativeLocation().Z-28;
   const FVector At=Light?Light->GetComponentLocation():FVector::ZeroVector,Want=Root->GetComponentTransform().TransformPosition(FFoundrylineForkliftBeacon::BulbCm());
   Items+=FString::Printf(TEXT("%s{\"shown\":%s,\"light\":%s,\"light_on\":%s,\"lumens\":%.1f,\"radius\":%.1f,\"blue\":%s,\"at_bulb\":%s,\"lift\":%.1f}"),Items.IsEmpty()?TEXT(""):TEXT(","),Root->IsVisible()?TEXT("true"):TEXT("false"),Light?TEXT("true"):TEXT("false"),
    Light&&Light->IsVisible()?TEXT("true"):TEXT("false"),Light?Light->Intensity:0.f,Light?Light->AttenuationRadius:0.f,Light&&Light->GetLightColor().B>.9f&&Light->GetLightColor().R<.1f?TEXT("true"):TEXT("false"),Light&&At.Equals(Want,.5)?TEXT("true"):TEXT("false"),Lift);}}
 return FString::Printf(TEXT("{\"authority\":%s,\"forklifts\":[%s]}"),Player->HasAuthority()?TEXT("true"):TEXT("false"),*Items);
#else
 return TEXT("{}");
#endif
}
