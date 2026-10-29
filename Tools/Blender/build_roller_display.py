"""Roller screen using the same CNC palette, type and layout as the other machines."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import math
out=Path(__file__).resolve().parents[2]/'SourceAssets/Production/Equipment/SheetRoller';out.mkdir(parents=True,exist_ok=True)
im=Image.new('RGB',(1600,900),'#101a24');d=ImageDraw.Draw(im)
white='#edf3f6';orange='#f09139';muted='#9aafbd';green='#7ee1ad';line='#344958'
def text(x,y,s,size=25,color=white,bold=False):d.text((x,y),s,font=ImageFont.truetype('C:/Windows/Fonts/'+('segoeuib.ttf' if bold else 'segoeui.ttf'),size),fill=color)
d.rectangle((0,0,1600,100),fill='#243544');d.rectangle((0,0,12,100),fill=orange)
text(35,18,'FOUNDRYLINE',40,white,True);text(420,29,'SR-3520 / ROLL CONTROL',27,muted);text(1360,28,'READY',30,green,True)
for x,s in [(35,'ROLL'),(210,'PROFILE'),(440,'MATERIAL'),(680,'SETUP')]:text(x,123,s,26,orange if s=='ROLL' else muted,True)
d.line((35,172,145,172),fill=orange,width=5);text(1010,126,'JOB 016 / TANKER SHELL',25)
for rect,title in [((30,200,565,495),'TARGET RADIUS'),((30,515,565,765),'ROLL SETTINGS'),((590,200,1570,765),'FORMING PREVIEW / SECTION A-A')]:
 d.rounded_rectangle(rect,14,fill='#1b2a38',outline=line,width=2);text(rect[0]+25,rect[1]+18,title,23,muted,True)
text(65,260,'45.28',104,white,True);text(380,290,'in',30,orange,True);text(65,426,'MILD STEEL  1/8 in',27,green)
for y,k,v in [(575,'FEED SPEED','6.56 ft/min'),(636,'UPPER ROLL','0.945 in'),(697,'PASS','01 / 03')]:text(57,y,k,24,muted);text(330,y-5,v,30,white,True)
for x in range(630,1540,40):d.line((x,265,x,655),fill='#243745')
for y in range(265,656,40):d.line((630,y,1540,y),fill='#243745')
for x,y in [(945,570),(1210,570),(1077,390)]:
 d.ellipse((x-72,y-72,x+72,y+72),fill='#546d7f',outline='#bed0dc',width=3)
 d.ellipse((x-23,y-23,x+23,y+23),outline='#bed0dc',width=3)
d.arc((740,225,1430,800),190,350,fill=orange,width=12)
d.line((700,505,920,505),fill=orange,width=9);d.polygon([(895,485),(925,505),(895,525)],fill=green)
d.line((1077,490,1077,650),fill=muted,width=2);text(1100,645,'R 1150',27,green,True)
text(635,700,'3-ROLL PLATE FORMING',25,white,True);text(1150,700,'SHAFT  11 ft 6-5/8 in',24,muted)
d.rectangle((0,790,1600,900),fill='#243544');text(35,812,'PASS 01 / 03',27,orange,True);text(35,850,'PROGRAM: SHELL_1150',20,muted)
text(440,812,'PARTS 00 / 10',27,white,True);text(440,850,'STOCK 60 × 120 IN',20,muted)
for x,s in [(870,'JOG'),(1110,'REVERSE'),(1350,'START ROLL')]:
 d.rounded_rectangle((x,815,x+210,870),8,fill='#294b43' if x==1350 else line,outline=green if x==1350 else line,width=2);text(x+22,830,s,22,green if x==1350 else white,True)
im.save(out/'T_SheetRoller_CNC.png')
