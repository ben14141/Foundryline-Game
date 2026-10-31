"""Original fictional-company vector marks, with matching antialiased PNGs for Canvas UI."""
from PIL import Image,ImageDraw
from pathlib import Path
import math,json
out=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/UI/Companies');out.mkdir(parents=True,exist_ok=True)
companies=[('Hartwell','Hartwell Fabrication','#27424E','#F28A40'),('Northfield','Northfield Repairs','#52634A','#DBB75E'),('RedCedar','Red Cedar Equipment','#923F32','#F2BD86'),('Mason','Mason Agricultural','#354F3B','#E4BA5C'),('Alder','Alder Workshop','#25675F','#E3B580'),('Union','Union Conveyor Co.','#334B64','#EF8950'),('FoundrySteel','Foundry Steel Supply','#3F4851','#F07A2F'),('Swift','Swift Van Couriers','#156A77','#FFD282'),('FoundryLane','Foundry Lane Haulage','#48424F','#ED8A47'),('County','County Freight','#466368','#D7B879'),('Interstate','Interstate Pallet Lines','#2D4D72','#F39A4D')]
companies += [('Ridgeline','Ridgeline Truck Outfitters','#303D43','#F28B43'),('Crossroads','Crossroads Fleet Service','#294D65','#DDAB69'),('Prairie','Prairie Pickup Accessories','#67513B','#F1C26E')]
white='#FCF8EE';S=4;manifest=[]
for k,(key,name,bg,accent) in enumerate(companies):
 im=Image.new('RGBA',(256*S,256*S));d=ImageDraw.Draw(im);svg=[]
 def poly(p,c):d.polygon([(x*S,y*S) for x,y in p],fill=c);svg.append('<polygon points="'+' '.join(f'{x},{y}' for x,y in p)+'" fill="'+c+'"/>')
 def line(p,c,w=9):d.line([(x*S,y*S) for x,y in p],fill=c,width=int(w*S),joint='curve');svg.append('<polyline points="'+' '.join(f'{x},{y}' for x,y in p)+'" fill="none" stroke="'+c+'" stroke-width="'+str(w)+'" stroke-linejoin="round"/>')
 def rect(b,c,rr=0):d.rounded_rectangle(tuple(v*S for v in b),radius=rr*S,fill=c);svg.append(f'<rect x="{b[0]}" y="{b[1]}" width="{b[2]-b[0]}" height="{b[3]-b[1]}" rx="{rr}" fill="{c}"/>')
 def circle(x,y,r,c):d.ellipse(((x-r)*S,(y-r)*S,(x+r)*S,(y+r)*S),fill=c);svg.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{c}"/>')
 rect((8,8,248,248),bg,40)
 if k==0:
  line([(128,30),(211,78),(211,177),(128,225),(45,177),(45,78),(128,30)],accent,7);rect((75,74,96,182),white,2);rect((160,74,181,182),white,2);poly([(94,126),(162,105),(162,133),(94,154)],accent)
 elif k==1:
  pts=[(128+89*(1 if i%4 in(0,1) else .84)*math.cos(i*math.pi/16),128+89*(1 if i%4 in(0,1) else .84)*math.sin(i*math.pi/16)) for i in range(32)];poly(pts,accent);circle(128,128,64,bg);line([(91,166),(159,98)],white,21);circle(163,93,30,white);poly([(156,58),(177,91),(159,111),(137,79)],bg);circle(88,169,12,bg)
 elif k==2:
  poly([(128,27),(218,65),(205,171),(128,226),(51,171),(38,65)],accent);poly([(128,42),(204,75),(192,163),(128,209),(64,163),(52,75)],bg)
  for x,y,z in [(88,105,32),(166,111,27),(128,83,43)]:poly([(x,y-z),(x-z,y+z),(x+z,y+z)],white);rect((x-5,y+z,x+5,y+z+22),accent)
 elif k==3:
  circle(128,128,91,accent);circle(128,128,78,bg);line([(128,194),(128,69)],white,9)
  for y in [88,115,142]:poly([(123,y+14),(92,y-1),(83,y-25),(112,y-12)],accent);poly([(133,y+14),(164,y-1),(173,y-25),(144,y-12)],accent)
  line([(75,193),(181,193)],white,8)
 elif k==4:
  line([(53,181),(128,57),(203,181)],white,19);line([(88,148),(168,148)],accent,12);poly([(124,122),(112,92),(128,74),(144,92),(132,122)],accent);line([(63,205),(193,205)],accent,6)
 elif k==5:
  for x in [64,128,192]:circle(x,173,16,white);circle(x,173,7,bg)
  line([(44,147),(212,147)],accent,10);line([(66,57),(66,107),(98,126),(128,107),(128,57)],white,17);line([(128,57),(128,107),(160,126),(190,107),(190,57)],white,17)
 elif k==6:
  for y,w in [(68,140),(110,108),(152,76)]:poly([(58,y),(58+w,y-18),(58+w,y+10),(58,y+28)],white)
  rect((58,169,81,203),accent);poly([(116,180),(140,156),(164,180),(140,203)],accent)
 elif k==7:
  poly([(41,75),(210,75),(166,121),(97,121)],white);poly([(38,140),(168,140),(133,176),(74,176)],accent);line([(133,192),(185,192),(211,147)],white,11);line([(45,197),(99,197)],accent,8)
 elif k==8:
  poly([(43,209),(103,47),(121,47),(94,209)],white);poly([(162,209),(135,47),(153,47),(213,209)],white)
  for y,w in [(77,6),(119,10),(168,15)]:poly([(128-w,y),(128+w,y),(128+w+2,y+22),(128-w-2,y+22)],accent)
 elif k==9:
  line([(43,161),(72,112),(99,88),(128,80),(157,88),(184,112),(213,161)],white,12);line([(43,165),(213,165)],accent,11)
  for x in [65,96,128,160,191]:line([(x,166),(x,126 if x in(65,191) else 104)],white,9)
  for y in [193,212]:line([(54,y),(202,y)],accent,6)
 elif k==11:
  line([(39,116),(85,61),(119,94),(153,44),(217,116)],accent,11);poly([(59,153),(75,122),(144,122),(165,151),(207,151),(207,181),(49,181),(49,153)],white);rect((82,134,135,153),bg,2);circle(86,185,17,accent);circle(176,185,17,accent)
 elif k==12:
  circle(128,128,94,accent);circle(128,128,81,bg);line([(75,75),(181,181)],white,17);line([(75,181),(181,75)],white,17)
  for x,y in [(75,75),(181,75),(75,181),(181,181)]:circle(x,y,20,white);circle(x,y,9,bg)
  rect((114,111,142,145),accent,4)
 elif k==13:
  circle(128,94,47,accent);rect((46,118,210,150),bg);line([(42,137),(214,137)],white,7);poly([(58,170),(84,145),(142,145),(165,169),(207,169),(207,193),(47,193),(47,171)],white);rect((91,155,133,172),bg,2);circle(82,197,13,accent);circle(178,197,13,accent);line([(53,218),(202,218)],accent,6)
 else:
  poly([(128,27),(213,63),(204,170),(128,229),(52,170),(43,63)],white);poly([(128,42),(197,74),(189,162),(128,210),(67,162),(59,74)],bg);rect((74,69,182,91),accent,3);poly([(88,180),(113,108),(123,108),(113,180)],white);poly([(143,180),(133,108),(143,108),(168,180)],white);line([(128,126),(128,144)],accent,6);line([(128,161),(128,181)],accent,6)
 stem='T_Company_'+key;im.resize((256,256),Image.Resampling.LANCZOS).save(out/(stem+'.png'));(out/(stem+'.svg')).write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">'+''.join(svg)+'</svg>');manifest.append({'company':name,'asset':stem,'palette':[bg,accent]})
(out/'manifest.json').write_text(json.dumps(manifest,indent=2));print('Original company marks:',len(manifest))
