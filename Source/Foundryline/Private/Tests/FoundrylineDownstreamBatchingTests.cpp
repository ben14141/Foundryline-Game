#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"

namespace DownstreamBatchingTest {
using Part=EFoundrylinePart;using Place=EFoundrylinePalletPlace;
struct FFixture {
 FFoundrylineFactorySimulation M;int32 Source=0,Input=110;bool Ready=false;
 FFixture(int32 SourceCount,int32 Wanted,FString& Error){
  M.SeedCutPartsShop(true);M.State.Running=false;M.State.Commerce.CashCents=10000000;
  if(!M.InstallBrake(Error))return;M.State.Commerce.BrakeInstallMinutes=0;M.AddMachineOperator(2,true);
  auto* Press=M.State.Machines.FindByPredicate([](const auto& V){return V.Id==2;});if(!Press)return;
  Press->Making=Part::BentBracket;Press->BatchRemaining=Wanted;Press->Enabled=true;Press->Auto=false;
  auto& Plasma=M.State.Machines[0];Plasma.Making=Part::Bracket;Plasma.BatchRemaining=50;Plasma.Remaining=0;Plasma.Enabled=true;Plasma.Auto=false;
  const auto* Slot=M.State.Slots.FindByPredicate([](const auto& S){return S.Id==107;});if(!Slot)return;
  FFoundrylineFactoryPallet Stock;Stock.Id=M.State.NextId++;Stock.Part=Part::Bracket;Stock.Quantity=SourceCount;Stock.Place=Place::Rack;Stock.Owner=Slot->Id;Stock.Position=Slot->Position;Stock.UnitLb=5;Source=Stock.Id;M.State.Pallets.Add(Stock);M.State.ReceivedSteelLb=M.SteelMass();M.State.Running=true;Ready=true;
 }
 void Advance(){M.Step(.1f);}
 bool RestoreReservedDelivery(int32 Count){
  auto* Stock=M.State.Pallets.FindByPredicate([&](const auto& P){return P.Id==Source;});const auto* Destination=M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==Input;});const auto* SourceSlot=Stock?M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==Stock->Owner;}):nullptr;if(!Stock||!Destination||!SourceSlot||Count>Stock->Quantity)return false;
  FFoundrylineFactoryJob Job;Job.Id=M.State.NextId++;Job.SourcePallet=Stock->Id;Job.SourcePlace=Place::Rack;Job.Part=Stock->Part;Job.Quantity=Count;Job.Destination=Input;Job.DestinationPlace=Place::Rack;Job.Pickup=SourceSlot->Dock;Job.Drop=Destination->Dock;Job.StoredAt=Destination->Position;Job.Priority=75;Stock->Reserved=Count;M.State.Jobs.Add(Job);return true;
 }
 int ToInput()const{int Q=0;for(const auto& J:M.State.Jobs)if(J.Destination==Input&&J.DestinationPlace==Place::Rack)Q+=J.Quantity;return Q;}
 int Reserved()const{const auto* P=M.State.Pallets.FindByPredicate([&](const auto& V){return V.Id==Source;});return P?P->Reserved:0;}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownstreamOutputBatchingTest,"Foundryline.Factory.DownstreamSupply.ProducerPalletTripBatching",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDownstreamOutputBatchingTest::RunTest(const FString&){
 using namespace DownstreamBatchingTest;FString Error;
 auto CheckMass=[&](const FFixture& F,float Before){TestTrue(TEXT("Planning transports neither creates nor loses steel"),FMath::IsNearlyEqual(F.M.SteelMass(),Before,.001f));};
 {FFixture F(9,50,Error);if(!TestTrue(TEXT("Fixture installs a real press and assigned operator: ")+Error,F.Ready))return false;const float Mass=F.M.SteelMass();F.Advance();TestEqual(TEXT("A growing nine-part output waits for a useful trip"),F.ToInput(),0);TestEqual(TEXT("Waiting does not reserve the producing pallet"),F.Reserved(),0);CheckMass(F,Mass);}
 {FFixture F(10,50,Error);const float Mass=F.M.SteelMass();F.Advance();TestEqual(TEXT("Ten available parts trigger a supply trip"),F.ToInput(),10);TestEqual(TEXT("The trip reserves its exact count"),F.Reserved(),10);F.Advance();TestEqual(TEXT("Repeated planning cannot double-book the pallet"),F.ToInput(),10);CheckMass(F,Mass);}
 {FFixture F(1,1,Error);const float Mass=F.M.SteelMass();F.Advance();TestEqual(TEXT("A one-part order never waits for ten or a full pallet"),F.ToInput(),1);CheckMass(F,Mass);}
 {FFixture F(3,3,Error);F.Advance();TestEqual(TEXT("The final three-part demand moves while upstream keeps producing"),F.ToInput(),3);}
 {FFixture F(4,50,Error);F.M.State.Machines[0].BatchRemaining=0;F.Advance();TestEqual(TEXT("An upstream completed batch flushes its partial pallet"),F.ToInput(),4);}
 {FFixture F(4,50,Error);F.M.State.Machines[0].Enabled=false;F.Advance();TestEqual(TEXT("A disabled producer cannot strand a partial pallet"),F.ToInput(),4);}
 {FFixture F(4,50,Error);F.M.State.Machines[0].Making=Part::MountingPlate;F.Advance();TestEqual(TEXT("A recipe change releases the prior recipe's partial pallet"),F.ToInput(),4);}
 {FFixture F(15,50,Error);const auto* Input=F.M.State.Slots.FindByPredicate([&](const auto& S){return S.Id==F.Input;});if(!TestNotNull(TEXT("Press input rack exists"),Input))return false;const float Mass=F.M.SteelMass();TestTrue(TEXT("Restored delivery fixture reserves eight parts"),F.RestoreReservedDelivery(8));F.Advance();TestEqual(TEXT("Existing reservation remains eight rather than being counted as free stock"),F.Reserved(),8);TestEqual(TEXT("No second trip is added for the reserved source"),F.ToInput(),8);CheckMass(F,Mass);}
 return true;
}
#endif
