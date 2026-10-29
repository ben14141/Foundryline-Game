"""Author a readable CNC display texture without external artwork."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[2]
out=root/'SourceAssets/Production/Equipment/PressBrake'
out.mkdir(parents=True,exist_ok=True)
im=Image.new('RGB',(1600,900),'#101a24');d=ImageDraw.Draw(im)
orange='#f09139';white='#edf3f6';muted='#9aafbd';green='#7ee1ad';panel='#1b2a38';line='#344958'
def font(size,bold=False):return ImageFont.truetype('C:/Windows/Fonts/'+('segoeuib.ttf' if bold else 'segoeui.ttf'),size)
def text(x,y,s,size=24,color=white,bold=False):d.text((x,y),s,font=font(size,bold),fill=color)
def card(rect,title):
 d.rounded_rectangle(rect,14,fill=panel,outline=line,width=2);text(rect[0]+25,rect[1]+18,title,23,muted,True)
d.rectangle((0,0,1600,100),fill='#243544');d.rectangle((0,0,12,100),fill=orange)
text(35,18,'FOUNDRYLINE',40,white,True);text(400,30,'PB-10  /  CNC BENDING',27,muted)
d.ellipse((1320,38,1338,56),fill=green);text(1353,26,'READY',30,green,True)
for x,s in [(35,'RUN'),(205,'PROGRAM'),(450,'TOOLING'),(680,'SETUP')]:
 text(x,125,s,26,orange if s=='RUN' else muted,True)
d.line((35,171,135,171),fill=orange,width=5)
text(1050,124,'JOB 014  /  TANKER BRACKET',24,white)
card((30,200,565,495),'TARGET ANGLE')
text(65,245,'90.0',120,white,True);text(365,274,'DEG',28,orange,True)
text(65,416,'ACTUAL  90.0°',27,green);text(335,416,'±0.5°',27,muted)
card((30,515,565,765),'AXIS POSITIONS')
for y,key,value in [(575,'Y  RAM','12.402 in'),(636,'X  BACK GAUGE','7.087 in'),(697,'R  HEIGHT','1.575 in')]:
 text(57,y,key,24,muted);text(330,y-5,value,30,white,True)
card((590,200,1570,765),'BEND PREVIEW  /  SECTION A-A')
for x in range(635,1540,40):d.line((x,265,x,650),fill='#243745')
for y in range(265,651,40):d.line((635,y,1540,y),fill='#243745')
# Upper punch, formed sheet section and lower die.
d.polygon([(1000,265),(1140,265),(1140,380),(1070,470),(1000,380)],fill='#8da4b3',outline='#e2edf2',width=3)
d.line([(865,360),(1070,565),(1275,360)],fill=orange,width=13,joint='curve')
d.polygon([(850,650),(850,540),(970,540),(1070,635),(1170,540),(1290,540),(1290,650)],fill='#546d7f',outline='#bed0dc',width=3)
d.arc((1000,494,1140,634),200,340,fill=green,width=3)
text(1040,515,'90°',25,green,True)
d.line((1365,370,1365,560),fill=muted,width=2);d.line((1347,370,1383,370),fill=muted,width=2);d.line((1347,560,1383,560),fill=muted,width=2)
text(1390,447,'7.087',27,white);text(1390,480,'in',20,muted)
text(645,690,'MILD STEEL  1/8 in',26,white,True);text(1110,690,'PUNCH 01  /  V-DIE 02',24,muted)
d.rectangle((0,790,1600,900),fill='#243544')
text(35,809,'STEP 01 / 03',28,orange,True);text(35,850,'PROGRAM: BRACKET_90',20,muted)
text(420,809,'PARTS  00 / 10',28,white,True);text(420,850,'TOOL LENGTH  10 ft',20,muted)
for x,label,active in [(860,'SINGLE BEND',False),(1120,'DRY RUN',False),(1350,'CYCLE READY',True)]:
 d.rounded_rectangle((x,815,x+215,870),8,fill='#294b43' if active else '#344958',outline=green if active else line,width=2)
 text(x+16,830,label,21,green if active else white,True)
im.save(out/'T_PressBrake_CNC.png')
print('CNC_DISPLAY_TEXTURE_READY')
