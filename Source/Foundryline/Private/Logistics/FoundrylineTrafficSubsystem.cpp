#include "Logistics/FoundrylineTrafficSubsystem.h"
#include "Construction/FoundrylinePlacementSubsystem.h"
#include "Engine/World.h"
namespace {
 double TrafficCross(FVector2D A,FVector2D B){return A.X*B.Y-A.Y*B.X;}
 double TrafficPointSegment(FVector2D P,FVector2D A,FVector2D B){const auto D=B-A;return FVector2D::Distance(P,A+D*FMath::Clamp(FVector2D::DotProduct(P-A,D)/FMath::Max(1.,D.SizeSquared()),0.,1.));}
 double TrafficSegmentGap(FVector2D A,FVector2D B,FVector2D C,FVector2D D){const auto U=B-A,V=D-C;const double Det=TrafficCross(U,V);if(FMath::Abs(Det)>1.e-6) {const double T=TrafficCross(C-A,V)/Det,S=TrafficCross(C-A,U)/Det;if(T>=0 && T<=1 && S>=0 && S<=1) return 0;}return FMath::Min(FMath::Min(TrafficPointSegment(A,C,D),TrafficPointSegment(B,C,D)),FMath::Min(TrafficPointSegment(C,A,B),TrafficPointSegment(D,A,B)));}
}
bool FFoundrylineTrafficLedger::Conflicts(const FFoundrylineTrafficClaim& X,const FFoundrylineTrafficClaim& Y) {
 if(X.From==Y.From || X.From==Y.Via || X.From==Y.To || X.Via==Y.From || X.Via==Y.Via || X.Via==Y.To || X.To==Y.From || X.To==Y.Via || X.To==Y.To) return true;
 const double Gap=(X.Width+Y.Width)/2;
 return TrafficSegmentGap(X.A,X.B,Y.A,Y.B)<Gap || TrafficSegmentGap(X.A,X.B,Y.B,Y.C)<Gap || TrafficSegmentGap(X.B,X.C,Y.A,Y.B)<Gap || TrafficSegmentGap(X.B,X.C,Y.B,Y.C)<Gap;
}
bool FFoundrylineTrafficLedger::Enter(const FFoundrylineRouteNetwork& G,FGuid Vehicle,FGuid From,FGuid Via,FGuid To,const FFoundrylineVehicleProfile& V,float Clear,double Now,FString& Reason) {
 for(float Dimension:{V.Front,V.Rear,V.Width,V.Clearance,V.TurnRadius,V.Hitch,V.TrailerAxle,V.TrailerFront,V.TrailerRear,V.TrailerWidth}) if(!FMath::IsFinite(Dimension) || Dimension<0 || Dimension>10000) {Reason=TEXT("Invalid vehicle profile");return false;}
 ExpireWaiting(Now);const auto* A=G.Find(From);const auto* B=G.Find(Via);const auto* C=G.Find(To);
 if(!Vehicle.IsValid() || !A || !B || !C || From==Via || Via==To || From==To || !FMath::IsFinite(Clear) || !FMath::IsFinite(Now) || !FMath::IsFinite(V.Width) || !FMath::IsFinite(V.TurnRadius) || V.Width<=0 || V.TurnRadius<=0 || V.Width>1000 || V.TurnRadius>10000) {Reason=TEXT("Invalid movement");return false;}
 if(!A->Data.Links.ContainsByPredicate([&](const auto& L){return L.To==Via;}) || !B->Data.Links.ContainsByPredicate([&](const auto& L){return L.To==To;}) || A->Company!=B->Company || B->Company!=C->Company || A->Plant!=B->Plant || B->Plant!=C->Plant) {Reason=TEXT("Movement is not connected in this direction");return false;}
 if(const auto* Own=Active.FindByPredicate([&](const auto& X){return X.Vehicle==Vehicle;})) {const bool Same=Own->From==From && Own->Via==Via && Own->To==To && Own->Revision==G.Revision;Reason=Same?TEXT("Reserved until trailer clears"):TEXT("Clear the existing movement first");return Same;}
 if(Active.Num()>=512 || Waiting.Num()>=512) {Reason=TEXT("Traffic scheduler capacity reached");return false;}
 FFoundrylineTrafficClaim Claim;Claim.Vehicle=Vehicle;Claim.From=From;Claim.Via=Via;Claim.To=To;Claim.A=A->Position;Claim.B=B->Position;Claim.C=C->Position;Claim.Width=FMath::Max(V.Width+2*V.Clearance,V.Semi?2*V.TurnRadius:V.Width+2*V.Clearance);Claim.Revision=G.Revision;
 auto* W=Waiting.FindByPredicate([&](const auto& X){return X.Claim.Vehicle==Vehicle;});
 if(W && (W->Claim.From!=From || W->Claim.Via!=Via || W->Claim.To!=To || W->Claim.Revision!=G.Revision)) {CancelWaiting(Vehicle);W=nullptr;}
 if(!W) {FFoundrylineTrafficWait New;New.Claim=Claim;New.Ticket=++NextTicket;New.LastSeen=Now;Waiting.Add(New);W=&Waiting.Last();}W->LastSeen=Now;
 // Downstream capacity must hold a complete cab/trailer plus headway; no partial lock acquisition.
 const float Length=V.Semi?V.Front+V.Hitch+V.TrailerRear:V.Front+V.Rear;
 W->ExitReady=FMath::IsFinite(Length) && Length>0 && Clear>=Length+300;
 if(!W->ExitReady) {Reason=TEXT("WAIT / exit cannot hold the whole vehicle + 3 m gap");return false;}
 if(Active.ContainsByPredicate([&](const auto& X){return X.Revision!=G.Revision || Conflicts(X,Claim);})) {Reason=TEXT("YIELD / junction or opposing link occupied");return false;}
 const uint64 Ticket=W->Ticket;
 if(Waiting.ContainsByPredicate([&](const auto& X){return X.ExitReady && X.Ticket<Ticket && Conflicts(X.Claim,Claim);})) {Reason=TEXT("YIELD / earlier conflicting vehicle has priority");return false;}
 Active.Add(Claim);CancelWaiting(Vehicle);Reason=TEXT("GO / junction and downstream space reserved");return true;
}
void FFoundrylineTrafficLedger::RearCleared(FGuid Id){Active.RemoveAll([&](const auto& X){return X.Vehicle==Id;});}
void FFoundrylineTrafficLedger::CancelWaiting(FGuid Id){Waiting.RemoveAll([&](const auto& X){return X.Claim.Vehicle==Id;});}
void FFoundrylineTrafficLedger::ExpireWaiting(double Now){Waiting.RemoveAll([&](const auto& X){return Now-X.LastSeen>30;});}
TMap<FGuid,float> FFoundrylineTrafficLedger::Delays() const {TMap<FGuid,float> D;for(const auto& X:Active) D.FindOrAdd(X.Via)+=8;for(const auto& X:Waiting) D.FindOrAdd(X.Claim.Via)+=4;return D;}
bool UFoundrylineTrafficSubsystem::Enter(FGuid Vehicle,FGuid From,FGuid Via,FGuid To,const FFoundrylineVehicleProfile& V,float Clear,FString& Reason) {
 if(GetWorld()->GetNetMode()==NM_Client) {Reason=TEXT("Traffic is server-authoritative");return false;}
 const auto G=FFoundrylineRouteNetwork::Read(*GetWorld()->GetSubsystem<UFoundrylinePlacementSubsystem>());
 return Ledger.Enter(G,Vehicle,From,Via,To,V,Clear,GetWorld()->GetTimeSeconds(),Reason);
}
bool UFoundrylineTrafficSubsystem::RearCleared(FGuid Vehicle){if(GetWorld()->GetNetMode()==NM_Client) return false;Ledger.RearCleared(Vehicle);return true;}
