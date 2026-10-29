#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Logistics/FoundrylineTrafficSubsystem.h"
#include "Construction/FoundrylineBuildSite.h"
#include "Engine/World.h"
namespace {
 FGuid Add(FFoundrylineRouteNetwork& G,FVector2D P){FFoundrylineWorldRouteNode N;N.Data.Id=FGuid::NewGuid();N.Position=P;G.Nodes.Add(N);return N.Data.Id;}
 void Link(FFoundrylineRouteNetwork& G,FGuid A,FGuid B,bool Reverse=false){auto* N=G.Nodes.FindByPredicate([&](const auto& X){return X.Data.Id==A;});FFoundrylineRouteLink L;L.To=B;L.Reverse=Reverse;N->Data.Links.Add(L);}
 FFoundrylineDrivingMap Yard(){FFoundrylineDrivingMap M;FFoundrylineDriveSurface S;S.Id=FGuid::NewGuid();S.Function=TEXT("FreightYard");S.Size={20000,20000};S.Transform=FTransform::Identity;S.TruckZones.Add({{0,0},S.Size});M.Surfaces.Add(S);return M;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineNodeGeometryTest,"Foundryline.Driving.NodeNetwork",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineNodeGeometryTest::RunTest(const FString&) {
 auto M=Yard();FFoundrylineRouteNetwork G;const auto A=Add(G,{4000,4000}),B=Add(G,{9000,4000}),C=Add(G,{9000,10000});Link(G,A,B);Link(G,B,C);
 const auto V=FFoundrylineVehicleProfile::Truck();const auto R=G.Check(M,V,A,C);
 TestTrue(TEXT("Rounded semi polyline fits"),R.Success);TestTrue(TEXT("Full trailer sweep sampled"),R.Route.Poses.Num()>300);
 TestTrue(TEXT("Ends at the actual final node"),R.Success && FVector2D::Distance(R.Route.Poses.Last().Position,G.Find(C)->Position)<2);
 for(int32 I=1;I<R.Route.Poses.Num();++I) {const auto& P=R.Route.Poses[I];const auto& Q=R.Route.Poses[I-1];const double D=FVector2D::Distance(P.Position,Q.Position);TestTrue(TEXT("No sample jumps or below-minimum curvature"),D<=25.1 && FMath::Abs(FMath::FindDeltaAngleRadians(Q.Yaw,P.Yaw))<=D/V.TurnRadius+.0001);}
 TestTrue(TEXT("One-way links cannot be driven backwards"),G.Path(C,A).IsEmpty());
 G.Nodes[1].Position={4400,4000};TestFalse(TEXT("Impossible radius is rejected rather than changing scale"),G.Check(M,V,A,C).Success);
 G.Nodes[1].Position={9000,4000};M.Surfaces[0].TruckZones.Reset();TestFalse(TEXT("Nodes never bypass truck permissions"),G.Check(M,V,A,C).Success);
 M=Yard();G.Nodes.Reset();const auto X=Add(G,{8000,8000}),Y=Add(G,{4000,8000});Link(G,X,Y,true);const auto Back=G.Check(M,V,X,Y);TestTrue(TEXT("Straight articulated backing route works"),Back.Success);
 if(Back.Success) TestTrue(TEXT("Backing reaches final node without teleport"),FVector2D::Distance(Back.Route.Poses.Last().Position,G.Find(Y)->Position)<2);
 // Alternative branch receives a congestion penalty and the directed graph selects the clear branch.
 G.Nodes.Reset();const auto S=Add(G,{3000,3000}),P=Add(G,{6000,3000}),Q=Add(G,{6000,5000}),E=Add(G,{9000,3000});Link(G,S,P);Link(G,P,E);Link(G,S,Q);Link(G,Q,E);
 TMap<FGuid,float> Wait;Wait.Add(P,100);const auto Alternate=G.Path(S,E,Wait);TestTrue(TEXT("Congestion selects an available branch"),Alternate.Num()==3 && Alternate[1]==Q);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineTrafficRulesTest,"Foundryline.Driving.TrafficRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineTrafficRulesTest::RunTest(const FString&) {
 FFoundrylineRouteNetwork G;const auto A=Add(G,{0,0}),B=Add(G,{5000,0}),C=Add(G,{10000,0}),D=Add(G,{5000,5000});Link(G,A,B);Link(G,B,C);Link(G,C,B);Link(G,B,A);Link(G,D,B);
 FFoundrylineTrafficLedger L;FString Why;const FGuid V1=FGuid::NewGuid(),V2=FGuid::NewGuid(),V3=FGuid::NewGuid();const auto V=FFoundrylineVehicleProfile::Truck();
 TestFalse(TEXT("No entering when the downstream link cannot hold a semi"),L.Enter(G,V1,A,B,C,V,1200,0,Why));TestEqual(TEXT("Blocked exit holds no partial junction lock"),L.Active.Num(),0);L.CancelWaiting(V1);
 TestTrue(TEXT("Clear movement gets atomic reservation"),L.Enter(G,V1,A,B,C,V,5000,1,Why));
 TestFalse(TEXT("Opposing traffic yields"),L.Enter(G,V2,C,B,A,V,5000,2,Why));TestFalse(TEXT("Merging vehicle yields"),L.Enter(G,V3,D,B,C,V,5000,3,Why));
 L.ExpireWaiting(100);TestEqual(TEXT("Active lock never expires under a stationary trailer"),L.Active.Num(),1);
 TestFalse(TEXT("Same vehicle cannot hold and wait for a different movement"),L.Enter(G,V1,C,B,A,V,5000,101,Why));
 L.Enter(G,V2,C,B,A,V,5000,102,Why);L.Enter(G,V3,D,B,C,V,5000,103,Why);L.RearCleared(V1);
 TestFalse(TEXT("Newer merge cannot jump an earlier opposing arrival"),L.Enter(G,V3,D,B,C,V,5000,104,Why));TestTrue(TEXT("Earlier waiter goes after the trailer clears"),L.Enter(G,V2,C,B,A,V,5000,105,Why));
 L.RearCleared(V2);TestTrue(TEXT("Queue makes progress without partial lock deadlock"),L.Enter(G,V3,D,B,C,V,5000,106,Why));
 FFoundrylineTrafficClaim H,I;H.A={-5000,0};H.B={0,0};H.C={5000,0};H.Width=300;H.From=FGuid::NewGuid();H.Via=FGuid::NewGuid();H.To=FGuid::NewGuid();I.A={0,-5000};I.B={0,-1000};I.C={0,5000};I.Width=300;I.From=FGuid::NewGuid();I.Via=FGuid::NewGuid();I.To=FGuid::NewGuid();TestTrue(TEXT("Crossing links conflict even without a shared junction node"),FFoundrylineTrafficLedger::Conflicts(H,I));
 L.RearCleared(V3);L.Waiting.Reset();
 TestFalse(TEXT("Blocked exit remains a waiting request"),L.Enter(G,V1,A,B,C,V,500,200,Why));
 TestTrue(TEXT("An exit-blocked waiter does not starve another ready movement"),L.Enter(G,V2,C,B,A,V,5000,201,Why));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoundrylineRouteAuthorityTest,"Foundryline.Driving.NodePersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFoundrylineRouteAuthorityTest::RunTest(const FString&) {
 auto* W=UWorld::CreateWorld(EWorldType::Game,false);auto* Site=W->SpawnActor<AFoundrylineBuildSite>();Site->bStarterProperty=true;Site->DispatchBeginPlay();auto* P=W->GetSubsystem<UFoundrylinePlacementSubsystem>();FGuid A,B,C;
 TestTrue(TEXT("Create a shared paved node"),Site->EditRouteNetwork(TEXT("NetDraw"),{},{3000,800},A));TestTrue(TEXT("Connect another paved node"),Site->EditRouteNetwork(TEXT("NetDraw"),A,{3000,2600},B,true));
 auto G=FFoundrylineRouteNetwork::Read(*P);TestEqual(TEXT("Two-way connections share actual node IDs"),G.Path(B,A).Num(),2);
 const auto* Save=P->CreateSnapshot();int32 Saved=0;for(const auto& O:Save->Objects) Saved+=O.RouteNodes.Num();TestEqual(TEXT("Nodes are included in property snapshots"),Saved,2);
 TestFalse(TEXT("Public street is not editable"),Site->EditRouteNetwork(TEXT("NetDraw"),{},{0,-3000},C));TestFalse(TEXT("Grass cannot host a node"),Site->EditRouteNetwork(TEXT("NetDraw"),{},{15000,15000},C));
 TestTrue(TEXT("Delete removes inbound and outbound connections atomically"),Site->EditRouteNetwork(TEXT("NetDelete"),B,{},C));G=FFoundrylineRouteNetwork::Read(*P);TestEqual(TEXT("No dangling links after node delete"),G.BrokenLinks,0);TestEqual(TEXT("Only other node remains"),G.Nodes.Num(),1);
 TestEqual(TEXT("Restore persisted nodes"),P->RestoreSnapshot(Save,true),EFoundrylinePlacementError::None);G=FFoundrylineRouteNetwork::Read(*P);TestEqual(TEXT("IDs and links survive restore"),G.Path(B,A).Num(),2);
 TestTrue(TEXT("Connect across separate paving records"),Site->EditRouteNetwork(TEXT("NetDraw"),A,{2000,-1000},C));G=FFoundrylineRouteNetwork::Read(*P);
 if(const auto* Node=G.Find(C)) {FFoundrylinePlacementContext Context;Context.CompanyId=FGuid(0xF001,1,1,1);Context.PlantId=FGuid(0xF001,2,1,1);Context.bCanConstruct=true;FFoundrylinePlacementRequest Remove;Remove.Action=EFoundrylinePlacementAction::Remove;Remove.ObjectId=Node->Surface;Remove.ExpectedRevision=P->GetRevision();TestTrue(TEXT("Paving demolition commits"),P->Execute(Remove,Context).IsSuccess());G=FFoundrylineRouteNetwork::Read(*P);TestEqual(TEXT("Paving demolition prunes incoming links on other records"),G.BrokenLinks,0);}
 W->DestroyWorld(false);return true;
}
#endif
