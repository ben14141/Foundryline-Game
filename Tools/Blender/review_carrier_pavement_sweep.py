"""Static swept tyre-contact review against the authored starter pavement union.
Diagnostic geometry review; native motion/vehicle physics acceptance is separate.
"""
import math,json
from pathlib import Path
root=Path(__file__).resolve().parents[2];rects=[(-8000,-3200,10000,-2400),(3500,-2400,4900,-200),(400,-200,5600,3600)]
def rounded(c,hints):
 p=[c[0]]
 for i in range(1,len(c)-1):
  a,cen,b=c[i-1],c[i],c[i+1];li=math.dist(a,cen);lo=math.dist(cen,b);f=1 if len(c)==3 else .4;r=min(hints[i-1] if i-1<len(hints) else 600,li*f,lo*f);inside=((cen[0]-a[0])/li,(cen[1]-a[1])/li);out=((b[0]-cen[0])/lo,(b[1]-cen[1])/lo);a=(cen[0]-inside[0]*r,cen[1]-inside[1]*r);b=(cen[0]+out[0]*r,cen[1]+out[1]*r);p.append(a)
  for j in range(1,13):
   t=j/12;p.append(((1-t)**2*a[0]+2*(1-t)*t*cen[0]+t*t*b[0],(1-t)**2*a[1]+2*(1-t)*t*cen[1]+t*t*b[1]))
 p.append(c[-1]);return p
def covered(x,y):return any(a-.01<=x<=c+.01 and b-.01<=y<=d+.01 for a,b,c,d in rects)
wheels=json.loads((root/'SourceAssets/StarterHauler/manifest.json').read_text())['wheels'];report={}
for name,c,h in [('arrival',[(-6500,-3000),(4200,-3000),(4200,-1100)],[1250]),('docking',[(4200,-1100),(4200,400),(1260,400),(1260,1700)],[]),('departure',[(1260,1700),(1260,2800),(4200,2800),(4200,-2800),(9300,-2800)],[600,600,1700])]:
 p=rounded(c,h);bad=[];samples=0
 for a,b in zip(p,p[1:]):
  distance=math.dist(a,b);forward=((b[0]-a[0])/distance,(b[1]-a[1])/distance)
  for k in range(max(2,int(distance/5))):
   t=k/(max(2,int(distance/5))-1);x=a[0]+(b[0]-a[0])*t;y=a[1]+(b[1]-a[1])*t;dx,dy=forward
   if name=='departure' and x>=6500 and y<=-2799:
    u=max(0,min(1,(x-6500)/2200));y=-2800-200*u*u*(3-2*u);slope=-1200*u*(1-u)/2200;norm=math.hypot(1,slope);dx,dy=1/norm,slope/norm
   samples+=1
   for w in wheels:
    lateral=-w['centre_cm'][1];longitudinal=w['centre_cm'][0]+134.4;half=w['size_cm'][1]/2
    # Conservative tyre-contact rectangle; includes steering-independent width.
    for across in [-half,half]:
     for along in [-10,10]:
      xx=x+dy*(lateral+across)+dx*(longitudinal+along);yy=y-dx*(lateral+across)+dy*(longitudinal+along)
      if not covered(xx,yy) and len(bad)<8:bad.append([round(xx,2),round(yy,2)])
 report[name]={'sampled_poses':samples,'all_tyre_contacts_paved':not bad,'first_outside_points':bad}
report['method']='Static authored pavement and measured wheel pivots; not a native physics or FPS result.'
(root/'Saved/Logs/CarrierTyrePavementReview.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));assert all(v['all_tyre_contacts_paved'] for k,v in report.items() if isinstance(v,dict))
