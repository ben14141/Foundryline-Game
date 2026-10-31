#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "SaveSystem/FoundrylineFactorySave.h"
#include "Kismet/GameplayStatics.h"
#include "Employees/FoundrylineEmployeeVisualComponent.h"
#include "Engine/World.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;
FFoundrylineFactorySimulation Fixture(){
 FFoundrylineFactorySimulation M;M.State.FloorMin={-2000,-2000};M.State.FloorMax={2500,2500};
 FFoundrylineWeldingStationRecord Z;Z.Id=M.State.NextId++;Z.Center={500,500,20};Z.Recipe=1;Z.BatchRemaining=10;Z.Reserved.Init(0,17);M.State.WeldingStations.Add(Z);
 int32 Slot=100;for(Part P:{Part::BentBracket,Part::MountingPlate,Part::LightKit}){FFoundrylineFactorySlot S;S.Id=Slot++;S.Part=P;S.Position={-900.,-900.+(Slot-100)*200.,20};S.Dock=S.Position;M.State.Slots.Add(S);FFoundrylineFactoryPallet A;A.Id=M.State.NextId++;A.Part=P;A.Quantity=20;A.UnitLb=P==Part::BentBracket?5:P==Part::MountingPlate?2:4.5f;A.Place=Place::Rack;A.Owner=S.Id;A.Position=S.Position;M.State.Pallets.Add(A);}

 return M;
}
int32 Person(FFoundrylineFactorySimulation& M,FVector Home){FFoundrylineStandbyDriver P;P.Id=M.State.NextId++;P.DisplayName=FString::Printf(TEXT("Collector %d"),P.Id);P.Home=P.Position=Home;M.State.UnassignedEmployees.Add(P);return P.Id;}
void Run(FFoundrylineFactorySimulation& M,int32 Ticks=1600){for(int32 I=0;I<Ticks;++I)M.UpdateCollectors(.1f);}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyCapacity,"Foundryline.AssemblySupply.CapacityAndUpgrade",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyCapacity::RunTest(const FString&){
 TestEqual(TEXT("100 lb compartment / 5 lb bracket"),FFoundrylineAssemblySupplyCatalog::Fit(100,5,50),20);
 TestEqual(TEXT("Physical count wins"),FFoundrylineAssemblySupplyCatalog::Fit(1000,5,4),4);
 TestEqual(TEXT("No invalid zero-weight infinite capacity"),FFoundrylineAssemblySupplyCatalog::Fit(100,0,50),0);
 auto M=Fixture();FString E;const int32 Area=M.State.WeldingStations[0].Id;
 if(!TestTrue(TEXT("Create 4 compartments"),M.ConfigureSupplyBin(Area,4,E))){AddError(E);return false;}TestTrue(TEXT("Upgrade to 6"),M.ConfigureSupplyBin(Area,6,E));TestTrue(TEXT("Upgrade to 8"),M.ConfigureSupplyBin(Area,8,E));
 TestFalse(TEXT("Reject unknown count"),M.ConfigureSupplyBin(Area,5,E));TestFalse(TEXT("Reject downgrade"),M.ConfigureSupplyBin(Area,4,E));
 TestTrue(TEXT("Physical model expands"),FFoundrylineAssemblySupplyCatalog::BinSize(8).X>FFoundrylineAssemblySupplyCatalog::BinSize(4).X);
 M.State.Running=true;TestFalse(TEXT("Reject live geometry change"),M.ConfigureSupplyBin(Area,6,E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyTrip,"Foundryline.AssemblySupply.MixedTripReservationsAndDelivery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyTrip::RunTest(const FString&){
 auto M=Fixture();FString E;const int32 Area=M.State.WeldingStations[0].Id,Id=Person(M,{-1200,-1200,20});
 if(!TestTrue(TEXT("Assign existing person to area"),M.AssignEmployee(Id,EFoundrylineEmployeeRole::PartsCollector,Area,E))){AddError(E);return false;}
 M.State.Running=true;const float Mass=M.SteelMass();M.UpdateCollectors(.1);
 TestEqual(TEXT("One trip carries multiple ingredient types"),M.State.Collectors[0].Load.Num(),3);
 TestEqual(TEXT("Bracket trip reserved"),M.CollectorIncoming(Area,int32(Part::BentBracket)),8);
 const int32 Second=Person(M,{-1400,-1200,20});TestTrue(TEXT("Additional collector assigned same area"),M.AssignCollector(Second,Area,E));M.UpdateCollectors(.1);
 TestEqual(TEXT("Second collector does not duplicate demand"),M.CollectorIncoming(Area,int32(Part::BentBracket)),8);
 TestTrue(TEXT("Reserved state validates: ")+E,M.SupplyValid(E));
 Run(M);TestEqual(TEXT("Brackets actually arrive"),M.Available(Part::BentBracket,Place::Buffer,Area),8);TestEqual(TEXT("Purchased fittings arrive too"),M.Available(Part::LightKit,Place::Buffer,Area),4);
 TestTrue(TEXT("Transport preserves steel mass"),FMath::IsNearlyEqual(M.SteelMass(),Mass,.01f));TestTrue(TEXT("Delivered state validates: ")+E,M.SupplyValid(E));
 TestTrue(TEXT("Release requested without teleport"),M.AssignEmployee(Id,EFoundrylineEmployeeRole::Unassigned,0,E));Run(M);
 TestFalse(TEXT("Collector removed after returning"),M.State.Collectors.ContainsByPredicate([&](const auto& C){return C.Id==Id;}));TestTrue(TEXT("Same employee becomes unassigned"),M.State.UnassignedEmployees.ContainsByPredicate([&](const auto& P){return P.Id==Id;}));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyPersistence,"Foundryline.AssemblySupply.SaveRestoreInTransit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyPersistence::RunTest(const FString&){
 auto M=Fixture();FString E;const int32 Area=M.State.WeldingStations[0].Id;if(!M.AssignCollector(Person(M,{-1200,-1200,20}),Area,E)){AddError(E);return false;}M.State.Running=true;
 bool Picked=false;for(int32 I=0;I<500;++I){M.UpdateCollectors(.1);if(M.State.Collectors[0].Load.ContainsByPredicate([](const auto& L){return L.Picked;})){Picked=true;break;}}
 TestTrue(TEXT("Actual pickup before save"),Picked);
 auto* Save=NewObject<UFoundrylineFactorySave>();Save->Factory=M.State;TArray<uint8> Bytes;TestTrue(TEXT("Serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 auto* Loaded=Cast<UFoundrylineFactorySave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Load"),Loaded))return false;
 FFoundrylineFactorySimulation Restored;Restored.State=Loaded->Factory;Run(M);Run(Restored);
 TestEqual(TEXT("Restored delivery matches uninterrupted"),Restored.Available(Part::BentBracket,Place::Buffer,Area),M.Available(Part::BentBracket,Place::Buffer,Area));TestTrue(TEXT("Restored supply valid: ")+E,Restored.SupplyValid(E));TestTrue(TEXT("Restored material mass matches"),FMath::IsNearlyEqual(M.SteelMass(),Restored.SteelMass(),.01f));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyFailures,"Foundryline.AssemblySupply.MissingStockOwnershipAndMigration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyFailures::RunTest(const FString&){
 auto M=Fixture();FString E;const int Area=M.State.WeldingStations[0].Id,Id=Person(M,{-1200,-1200,20});
 TestFalse(TEXT("Cannot assign employee from another business"),M.AssignCollector(99999,Area,E));TestFalse(TEXT("Unknown area rejected"),M.AssignCollector(Id,99999,E));
 if(!TestTrue(TEXT("Assign valid area"),M.AssignCollector(Id,Area,E))){AddError(E);return false;}M.State.Running=true;for(auto& P:M.State.Pallets)P.OrderId=10;
 Run(M,30);TestTrue(TEXT("Order-reserved stock is not stolen"),M.State.Collectors[0].Load.IsEmpty());for(auto& P:M.State.Pallets)P.OrderId=0;
 Run(M);TestEqual(TEXT("Newly available stock resumes supply"),M.Available(Part::BentBracket,Place::Buffer,Area),8);
 auto Legacy=Fixture();Legacy.State.Version=14;const int Next=Legacy.State.NextId;Legacy.MigrateAssemblySupply();Legacy.MigrateAssemblySupply();TestEqual(TEXT("Schema migrated once"),Legacy.State.Version,15);TestEqual(TEXT("Migration preserves IDs"),Legacy.State.NextId,Next);
 auto Other=M.State;Other.CompanyId=FGuid::NewGuid();Other.PlantId=FGuid::NewGuid();const auto A=FFoundrylineEmployeeRoster::Build(M.State),B=FFoundrylineEmployeeRoster::Build(Other);TestTrue(TEXT("Company-scoped collector identity"),A[0].Key!=B[0].Key);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyGrip,"Foundryline.AssemblySupply.AnimatedCartGrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyGrip::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Actor=World->SpawnActor<AActor>();auto* Visual=NewObject<UFoundrylineEmployeeVisualComponent>(Actor);Actor->SetRootComponent(Visual);Visual->RegisterComponent();Actor->AddInstanceComponent(Visual);
 FFoundrylineEmployeeRecord Employee;Employee.Id=99;Employee.Key=TEXT("collector-animation-test");Employee.Role=EFoundrylineEmployeeRole::PartsCollector;Employee.Activity=EFoundrylineEmployeeActivity::Walking;Employee.bMoving=true;Employee.Speed=90;
 for(int32 Frame=0;Frame<12;++Frame){Employee.Distance=Frame*9;Visual->Present(Employee,.1f);Visual->PresentCollectorGrip(FVector(25,-22,99),FVector(25,22,99));TSharedPtr<FJsonObject> Report;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Visual->GetPresentationReport()),Report);
  if(!TestTrue(TEXT("Actual humanoid collector"),Report&&Report->GetBoolField(TEXT("skeletal"))))break;
  for(int32 Hand=0;Hand<2;++Hand){const auto& P=Report->GetArrayField(FString::Printf(TEXT("palm%dWorld"),Hand));const FVector Palm(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber());TestTrue(TEXT("Palm remains on real handle while legs walk"),Palm.Equals(FVector(25,Hand?22:-22,99),.2));}
 }
 Employee.bMoving=false;Employee.bWorking=true;Employee.Activity=EFoundrylineEmployeeActivity::Handling;Employee.WorkPhase=.5f;Visual->Present(Employee,.1f);Visual->PresentCollectorGrip(FVector(25,-22,99),FVector(45,10,84));TSharedPtr<FJsonObject> LoadingReport;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Visual->GetPresentationReport()),LoadingReport);if(LoadingReport){const auto& Palm=LoadingReport->GetArrayField(TEXT("palm1World"));TestTrue(TEXT("Loading reach stays on tray without stretching"),FVector(Palm[0]->AsNumber(),Palm[1]->AsNumber(),Palm[2]->AsNumber()).Equals(FVector(45,10,84),.2));}
 World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlanningPurchases,"Foundryline.AssemblySupply.PlanningPurchasesAreAtomic",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlanningPurchases::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);M.State.Running=false;M.State.FloorMin={-5000,-5000};M.State.FloorMax={5000,5000};M.Solids.Reset();M.State.Lanes.Reset();FString E;
 const int64 Before=M.State.Commerce.CashCents;TestFalse(TEXT("Outside-shop placement rejected"),M.BuyPlanningEquipment(1,{10000,10000,20},0,2,E));TestEqual(TEXT("Failed placement costs nothing"),M.State.Commerce.CashCents,Before);
 if(!TestTrue(TEXT("Purchase and place parts rack: ")+E,M.BuyPlanningEquipment(1,{1000,1000,20},0,2,E)))return false;
 TestEqual(TEXT("Two levels have four real slots"),M.State.StorageRacks[0].Slots.Num(),4);TestEqual(TEXT("Charge exact purchase price"),M.State.Commerce.CashCents,Before-FFoundrylinePlanningEquipment::Price(1,2));
 const int64 After=M.State.Commerce.CashCents;TestFalse(TEXT("Overlapping purchase rejected"),M.BuyPlanningEquipment(2,{1000,1000,20},0,1,E));TestEqual(TEXT("Rejected repeat does not charge"),M.State.Commerce.CashCents,After);
 TestTrue(TEXT("Purchase wide finished-goods rack: ")+E,M.BuyPlanningEquipment(2,{-2000,2000,20},1,1,E));TestTrue(TEXT("Rack role saved"),M.State.StorageRacks.Last().FinishedGoods);
 M.State.Commerce.CashCents=0;TestFalse(TEXT("Insufficient cash rejected"),M.BuyPlanningEquipment(3,{2500,-1500,20},0,1,E));TestTrue(TEXT("Purchase state validates: ")+E,M.Valid(E));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCollectorCrossingEscape,"Foundryline.AssemblySupply.CrossingAllowsRetreat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCollectorCrossingEscape::RunTest(const FString&){
 auto M=Fixture();FString E;
 M.PaintLane(EFoundrylineFloorLane::Forklift,{-500,0,20},{500,0,20},200,0,E);
 M.PaintLane(EFoundrylineFloorLane::Pedestrian,{0,-500,20},{0,500,20},200,0,E);
 FFoundrylineFactoryLane Crossing;Crossing.Id=M.State.NextId++;Crossing.Kind=EFoundrylineFloorLane::Crossing;Crossing.Min={-100,-100};Crossing.Max={100,100};M.State.Lanes.Add(Crossing);
 const auto Crossings=M.FloorCrossings();if(!TestTrue(TEXT("A real painted crossing exists"),!Crossings.IsEmpty()))return false;
 const auto& Bounds=Crossings[0].Bounds;FFoundrylineFactoryDriver D;D.Id=M.State.NextId++;D.Position=FVector(Bounds.GetCenter(),20)+FVector(95,0,0);D.Parking=D.Position;M.State.Drivers.Add(D);
 const FVector From(Bounds.Min.X-1,Bounds.GetCenter().Y,20);
 TestTrue(TEXT("Cart/person can retreat from occupied crossing approach"),M.CanEmployeeAdvance(From,From-FVector(10,0,0),0));
 TestFalse(TEXT("Approaching still yields to forklift"),M.CanEmployeeAdvance(From,From+FVector(10,0,0),0));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyLegacyStack,"Foundryline.AssemblySupply.LegacyFiveStackPreserved",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSupplyLegacyStack::RunTest(const FString&){
 FFoundrylineFactorySimulation M;M.SeedCutPartsShop(true);M.State.Version=14;auto* Slot=M.State.Slots.FindByPredicate([](const auto& S){return S.Id==109;});if(!TestNotNull(TEXT("Existing top storage slot"),Slot))return false;
 Slot->Part=Part::HeadacheRack;FFoundrylineFactoryPallet P;P.Id=M.State.NextId++;P.Part=Part::HeadacheRack;P.Quantity=5;P.UnitLb=129;P.Place=Place::Rack;P.Owner=Slot->Id;P.Position=Slot->Position;M.State.Pallets.Add(P);M.MigrateWeldingStations();FString Error;
 const auto* Restored=M.State.Pallets.FindByPredicate([&](const auto& A){return A.Id==P.Id;});TestTrue(TEXT("Legacy five-product stack retained"),Restored&&Restored->Quantity==5&&Restored->bLegacyFiveStack);TestEqual(TEXT("New pallet capacity is four"),FFoundrylineFactoryCatalog::Part(Part::HeadacheRack).Capacity,4);TestTrue(TEXT("Migrated stock validates: ")+Error,M.Valid(Error));return true;
}
#endif
