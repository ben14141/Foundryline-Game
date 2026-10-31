#pragma once
#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "RenderUtils.h"

// Local Canvas presentation only. Geometry is bounded and does not alter HUD hit boxes.
namespace FoundrylineUISurface
{
inline void Round(UCanvas* Canvas,float X,float Y,float W,float H,float Radius,FLinearColor Top,FLinearColor Bottom)
{
 if(!Canvas||W<=0||H<=0)return;
 const float R=FMath::Clamp(Radius,0.f,FMath::Min(W,H)*.5f);
 TArray<FVector2D,TInlineAllocator<28>> Ring;
 const FVector2D Centers[]={{X+R,Y+R},{X+W-R,Y+R},{X+W-R,Y+H-R},{X+R,Y+H-R}};
 for(int C=0;C<4;++C)for(int I=0;I<=5;++I){const float Angle=PI+C*HALF_PI+I*HALF_PI/5;Ring.Add(Centers[C]+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*R);}
 const FVector2D Center(X+W*.5f,Y+H*.5f);
 auto Color=[&](const FVector2D& P){return FMath::Lerp(Top,Bottom,FMath::Clamp(float((P.Y-Y)/H),0.f,1.f));};
 TArray<FCanvasUVTri> Tris;Tris.Reserve(Ring.Num()*3);
 auto Triangle=[&](FVector2D A,FVector2D B,FVector2D C,FLinearColor CA,FLinearColor CB,FLinearColor CC){FCanvasUVTri T;T.V0_Pos=A;T.V1_Pos=B;T.V2_Pos=C;T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;T.V0_Color=CA;T.V1_Color=CB;T.V2_Color=CC;Tris.Add(T);};
 for(int I=0;I<Ring.Num();++I){const auto A=Ring[I],B=Ring[(I+1)%Ring.Num()];Triangle(Center,A,B,Color(Center),Color(A),Color(B));const auto OA=A+(A-Center).GetSafeNormal()*.8,OB=B+(B-Center).GetSafeNormal()*.8;auto ClearA=Color(A),ClearB=Color(B);ClearA.A=ClearB.A=0;Triangle(A,OA,OB,Color(A),ClearA,ClearB);Triangle(A,OB,B,Color(A),ClearB,Color(B));}
 FCanvasTriangleItem Item(Tris,GWhiteTexture);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
}
inline void Panel(UCanvas* Canvas,float X,float Y,float W,float H,float Scale,FLinearColor Top,FLinearColor Bottom,FLinearColor Edge,float Elevation=1,float Radius=6)
{
 const float S=FMath::Max(.1f,Scale),E=FMath::Max(0.f,Elevation);
 if(E>.01f){Round(Canvas,X-S,Y+(E+1)*S,W+2*S,H+S,(Radius+1)*S,FLinearColor(0,0,0,.045f),FLinearColor(0,0,0,.045f));Round(Canvas,X,Y+E*S,W,H,Radius*S,FLinearColor(0,0,0,.09f),FLinearColor(0,0,0,.09f));}
 Round(Canvas,X,Y,W,H,Radius*S,Edge,Edge);
 Round(Canvas,X+S,Y+S,W-2*S,H-2*S,FMath::Max(0.f,Radius-1)*S,Top,Bottom);
}
}
