"""Generate Foundryline plasma CNC screen artwork."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
out=Path(__file__).resolve().parents[2]/'SourceAssets/Production/Equipment/PlasmaTable';out.mkdir(parents=True,exist_ok=True)
im=Image.new('RGB',(1600,900),'#101a24');d=ImageDraw.Draw(im)
white='#edf3f6';orange='#f09139';muted='#9aafbd';green='#7ee1ad';line='#344958'
def text(x,y,s,size=25,color=white,bold=False):d.text((x,y),s,font=ImageFont.truetype('C:/Windows/Fonts/'+('segoeuib.ttf' if bold else 'segoeui.ttf'),size),fill=color)
d.rectangle((0,0,1600,100),fill='#243544');d.rectangle((0,0,12,100),fill=orange)
text(35,18,'FOUNDRYLINE',40,white,True);text(420,29,'PT-1410  /  PLASMA CNC',29,muted);text(1360,28,'READY',30,green,True)
for x,s in [(35,'CUT'),(185,'NEST'),(355,'MATERIAL'),(590,'SETUP')]:text(x,123,s,27,orange if s=='CUT' else muted,True)
d.line((35,172,105,172),fill=orange,width=5);text(1010,126,'JOB 015  /  CHASSIS BRACKETS',24)
d.rounded_rectangle((30,200,1100,755),14,fill='#1b2a38',outline=line,width=2)
text(55,220,'NEST PREVIEW  /  60 × 120 IN SHEET',25,muted,True)
for x in range(65,1070,30):d.line((x,280,x,720),fill='#253b4c')
for y in range(280,721,30):d.line((65,y,1070,y),fill='#253b4c')
d.rectangle((90,305,1035,700),outline=muted,width=2)
for row in range(3):
 for col in range(6):
  x=120+col*148;y=333+row*116
  pts=[(x,y),(x+112,y),(x+112,y+70),(x+85,y+70),(x+85,y+90),(x+22,y+90),(x+22,y+70),(x,y+70),(x,y)]
  d.line(pts,fill=orange,width=4)
  for cx in [x+25,x+85]:d.ellipse((cx-7,y+25,cx+7,y+39),outline=green,width=3)
d.line((90,305,132,333),fill=green,width=3);d.line((275,280,275,715),fill='#5e8e9f',width=2);d.line((70,465,1060,465),fill='#5e8e9f',width=2)
d.ellipse((265,455,285,475),outline=green,width=3)
d.rounded_rectangle((1120,200,1570,755),14,fill='#1b2a38',outline=line,width=2)
text(1148,223,'CUT PARAMETERS',25,muted,True)
for y,k,v in [(285,'MATERIAL','MILD STEEL'),(375,'THICKNESS','1/8 in'),(465,'CUT CURRENT','65 A'),(555,'FEED RATE','94.5 in/min'),(645,'TORCH HEIGHT','0.059 in')]:
 text(1148,y,k,21,muted);text(1148,y+29,v,31,white,True)
d.rectangle((0,790,1600,900),fill='#243544');text(35,812,'SHEET 01 / 10',27,orange,True);text(35,850,'TABLE  10 ft x 14 ft',20,muted)
text(465,812,'PARTS  00 / 18',27,white,True);text(465,850,'NEST UTILIZATION  78%',20,muted)
for x,s in [(885,'SIMULATE'),(1120,'DRY RUN'),(1350,'START CUT')]:
 d.rounded_rectangle((x,815,x+210,870),8,fill='#294b43' if x==1350 else line,outline=green if x==1350 else line,width=2);text(x+20,830,s,22,green if x==1350 else white,True)
im.save(out/'T_PlasmaTable_CNC.png')
