"""Reusable game-owned 1:1 triple-monitor office workstation geometry.
Executed with build_operations_assets.py box/cyl/mats/group helpers in scope.
"""
from mathutils import Matrix

def office_mat(key,color,rough=.5,metal=0):
 m=bpy.data.materials.new('ops_'+key.lower());m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;mats[key]=m;return m
for key,col,rough,metal in [('Case',(.045,.055,.065),.44,.16),('Desk',(.72,.73,.70),.64,.04),('Keycap',(.035,.042,.05),.65,0),('Legend',(.72,.76,.77),.65,0),('Bezel',(.012,.017,.023),.38,.1),('Orange',(.75,.17,.025),.4,.15),('Port',(.004,.007,.01),.7,.05),('Indicator',(.1,.55,.34),.35,0)]:office_mat(key,col,rough,metal)
p=mats['Indicator'].node_tree.nodes.get('Principled BSDF');p.inputs['Emission Color'].default_value=(.05,.45,.2,1);p.inputs['Emission Strength'].default_value=.5

def text3(name,value,at,size=.65,mat='Legend',rot=(0,0,180)):
 bpy.ops.object.text_add(location=Vector(at)/100);o=bpy.context.object;o.name=name;o.data.body=value;o.data.size=size/100;o.data.align_x='CENTER';o.data.align_y='CENTER';o.data.extrude=.00012;o.rotation_euler=[math.radians(v) for v in rot];o.data.materials.append(mats[mat]);group.append(o);return o

def cable(name,points,r=.22):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=8;cu.bevel_depth=r/100;cu.bevel_resolution=2;sp=cu.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
 for b,p in zip(sp.bezier_points,points):b.co=Vector(p)/100;b.handle_left_type='AUTO';b.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(o);o.data.materials.append(mats['Rubber']);group.append(o);return o

def bolt(name,at,axis='Z',radius=.36):return cyl(name,at,radius,.16,'MachinedSteel',rot=(90,0,0) if axis=='Y' else (0,90,0) if axis=='X' else None)

# A 60 x 30 inch desk: warm satin top, bevelled edge, structurally clear knee space.
box('DeskTop',(0,0,76),(152,76,4),'Desk',.75)
box('DeskFrontEdge',(0,37.95,75.6),(150,.14,1.1),'Case',.05)
for x in [-65,65]:
 for y in [-27,27]:
  box('DeskLeg',(x,y,36),(4.5,4.5,72),'Case',.28);box('AdjustableFoot',(x,y,1.25),(6,6,2.5),'Rubber',.3)
 box('LegUpperRail',(x,0,72),(5,62,3.5),'Case',.25)
box('RearBrace',(0,-28,30),(135,3,4),'Case',.3);box('ModestyPanel',(0,-29,56),(126,1.6,29),'Case',.35)
for x in [-53,53]:cyl('CableGrommet',(x,-27,78.15),3.0,.45,'Rubber');cyl('GrommetPassThrough',(x,-27,78.42),.85,.12,'Port')
box('UnderDeskCableTray',(0,-24,67),(94,12,2),'Case',.3)
for x in [-45,45]:box('TrayEnd',(x,-24,69),(2,12,5),'Case',.2)
box('TrayBackLip',(0,-29.5,69),(94,1,5),'Case',.2)

# Three identical 23.5-inch active panels, with a shared three-arm clamp mount.
box('MonitorClampTop',(0,-32,79),(13,10,2),'Case',.4);box('MonitorClampUnder',(0,-32,72),(11,9,2),'Case',.3);cyl('ClampScrew',(0,-32,75.5),.8,8,'MachinedSteel');cyl('ClampKnob',(0,-32,70),2,1.8,'Rubber');cyl('MonitorMast',(0,-29,96),1.6,33,'Case')
for x in [-27.5,27.5]:box('MonitorCrossArm',(x,-27,108),(55,4,4),'Case',.6);cyl('ArmHinge',(x*2,-27,108),2.6,5,'MachinedSteel')
for x in [-55.2,55.2]:box('SideMonitorReachArm',(x,-20,108),(4,14,4),'Case',.5)
for x in [-55.2,0,55.2]:box('VesaHead',(x,-24 if x==0 else -13,111),(9,5,9),'Case',.6)

screen_specs=[]
for label,cx,cy,yaw in [('Left',55.2,-8,25),('Center',0,-20,0),('Right',-55.2,-8,-25)]:
 start=len(group);slot='OfficeDesktopScreen_'+label;m=bpy.data.materials.new(slot);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.045,.09,.13,1);p.inputs['Emission Color'].default_value=(.025,.06,.08,1);p.inputs['Emission Strength'].default_value=.25
 # Thin manufactured shell and inset front bezel, not a floating screen plane.
 box(label+'_DisplayShell',(0,0,0),(56.6,3.4,33.6),'Case',.55)
 box(label+'_InnerBezel',(0,1.78,0),(54.2,.4,31.2),'Bezel',.2)
 box(label+'_BottomAccent',(0,2.02,-15.65),(46,.18,.24),'Orange',.06)
 text3(label+'_Brand','FOUNDRYLINE',(0,2.08,-15.5),.6,'Legend',(90,0,180))
 cyl(label+'_PowerLED',(-25.7,2.1,-15.4),.12,.12,'Indicator',(90,0,0))
 for vx in [-22,-18,-14,-10,-6,-2,2,6,10,14,18,22]:box(label+'_BackVent',(vx,-1.74,9),(2.0,.14,.25),'Port',.06)
 box(label+'_VesaPlate',(0,-2.5,0),(10,1.4,10),'Case',.3)
 for vx in [-3.75,3.75]:
  for vz in [-3.75,3.75]:bolt(label+'_VesaBolt',(vx,-3.25,vz),'Y')
 box(label+'_BackIO',(9,-1.82,-11),(13,.25,3),'Port',.2)
 for vx in [5,9,13]:box(label+'_DisplayPort',(vx,-2,-11),(2.5,.2,1.2),'MachinedSteel',.1)
 me=bpy.data.meshes.new(slot);me.from_pydata([( .26,.0265,-.145),(-.26,.0265,-.145),(-.26,.0265,.145),(.26,.0265,.145)],[],[(0,1,2,3)]);me.materials.append(m);uv=me.uv_layers.new(name='UVMap')
 for loop,coord in zip(me.loops,[(0,0),(1,0),(1,1),(0,1)]):uv.data[loop.index].uv=coord
 plane=bpy.data.objects.new(label+'_ActiveScreen',me);bpy.context.collection.objects.link(plane);group.append(plane)
 M=Matrix.Translation(Vector((cx,cy,111))/100)@Matrix.Rotation(math.radians(yaw),4,'Z')
 for o in group[start:]:o.matrix_world=M@o.matrix_world
 c=M@Vector((0,.0265,0));screen_specs.append({'slot':slot,'center_cm':[round(v*100,4) for v in c],'yaw_deg':yaw,'active_cm':[52,29]})
 cable(label+'_DisplayLead',[(cx,cy-3,102),(cx,cy-9,98),(cx*.6,-29,90),(30,-28,79),(53,-27,78),(47,-24,68)],.25)

# Full-size stepped keyboard: six rows, varied key widths, legends and cable.
keyboard_start=len(group)
box('KeyboardBase',(0,17,79.05),(44.5,15.5,1.7),'Case',.6)
box('KeyboardPalmEdge',(0,24.4,79.6),(42.5,.5,.5),'Rubber',.2)
def key(name,x,y,width=1.65,label=''):
 z=80.25+(17-y)*.035;box(name,(x,y,z),(width,1.55,.55),'Keycap',.16)
 if label:text3('KeyLegend_'+label,label,(x,y,z+.285),.55,'Legend')
rows=[list('1234567890-='),list('QWERTYUIOP[]'),list('ASDFGHJKL;'),list('ZXCVBNM,./')]
for row,letters in enumerate(rows):
 y=12.4+row*2.05
 for col,value in enumerate(letters):key('KeyboardKey',-17.2+col*1.95,y,1.65,value)
for col in range(12):key('FunctionKey',-18+col*2.0,10.1,1.55,'F'+str(col+1))
for x,y,w,label in [(-20,12.4,2.1,'ESC'),(-20,14.45,2.1,'TAB'),(-19.7,16.5,2.7,'CAPS'),(-19,18.55,4.1,'SHIFT'),(7.5,16.5,3.5,'ENTER'),(5.8,18.55,4.3,'SHIFT'),(-19.5,21,2.5,'CTRL'),(-15.8,21,2.5,'WIN'),(-12.3,21,2.5,'ALT'),(-4.1,21,12,'SPACE'),(4.2,21,2.5,'ALT'),(8,21,2.5,'CTRL')]:key('SpecialKey',x,y,w,label)
for row in range(5):
 for col in range(3):key('NumpadKey',12.9+col*2.2,11.3+row*2.2,1.8,str([7,8,9,4,5,6,1,2,3,0,0,0,0,0,0][row*3+col]))
for x in [12.8,16,19.2]:cyl('KeyboardLockLED',(x,23.4,80.1),.1,.12,'Indicator')
cable('KeyboardUSBLead',[(0,9.2,79),(0,4,78.5),(-20,-10,78.4),(-45,-23,78.4),(-53,-27,78.5),(-45,-25,68)],.17)

for o in group[keyboard_start:]:
 if o.type=='CURVE' and o.data.splines:continue
 o.location.x=-o.location.x

# Shaped mouse, split buttons, wheel and edged cloth pad.
mouse_start=len(group)
box('MousePad',(39,17,78.25),(26,22,.4),'Rubber',.8);box('PadOrangeEdge',(39,27.9,78.47),(23,.2,.06),'Orange',.04);text3('MousePadBrand','FOUNDRYLINE',(39,24.5,78.48),.8,'Legend')
box('MouseLowerShell',(39,17,79.5),(6.5,10.2,2.0),'Case',.9)
for x in [37.4,40.6]:box('MouseButton',(x,14.9,80.75),(2.9,5.6,1.1),'Keycap',.8,rot=(4,0,0))
cyl('MouseWheel',(39,13.7,81.3),.65,1.0,'Rubber',(0,90,0))
cable('MouseUSBLead',[(39,11.8,80),(39,7,78.6),(44,-9,78.5),(53,-27,78.5),(47,-25,68)],.16)

for o in group[mouse_start:]:
 if o.type=='CURVE' and o.data.splines:
  for sp in o.data.splines:
   for b in sp.bezier_points:b.co.x=-b.co.x;b.handle_left.x=-b.handle_left.x;b.handle_right.x=-b.handle_right.x
 else:o.location.x=-o.location.x

# Business tower, with inset panels, real-sized front I/O, fans, vents and plugs.
box('TowerChassis',(49,-3,25),(21,42,44),'Case',.7)
for x in [41,57]:
 for y in [-18,12]:box('TowerFoot',(x,y,2),(3.5,5,3),'Rubber',.5)
box('TowerFrontBezel',(49,18.25,25),(19.6,.65,42.5),'Bezel',.45)
box('TowerOrangeSpine',(58.1,18.65,25),(.55,.2,37),'Orange',.1)
box('TowerIOInset',(48,18.7,41),(14,.24,5),'Port',.25)
cyl('TowerPowerButton',(42.5,18.95,41),.8,.25,'MachinedSteel',(90,0,0));cyl('TowerPowerLED',(42.5,19.1,41),.28,.16,'Indicator',(90,0,0))
for x in [46,50]:box('FrontUSBMetal',(x,18.9,41),(2.2,.25,.9),'MachinedSteel',.12);box('FrontUSBVoid',(x,19.05,41),(1.6,.12,.55),'Port',.05)
cyl('HeadphoneSocket',(54.6,19,41),.45,.3,'Port',(90,0,0))
for z in range(9,33,2):
 for x in [43.5,47.2,50.9,54.6]:box('FrontVent',(x,18.75,z),(2.9,.18,.4),'Port',.13)
text3('TowerBrand','FOUNDRYLINE',(49,19,35.8),.85,'Legend',(90,0,180))
box('TowerSideInset',(59.65,-3,25),(.18,37,38),'Bezel',.2)
for y in range(-16,12,3):
 for z in [12,16,20,24,28]:box('SideVent',(59.8,y,z),(.16,1.7,.5),'Port',.08)
for y in [-18,12]:
 for z in [8,42]:bolt('SidePanelScrew',(59.85,y,z),'X')
box('RearMotherboardIO',(48,-24.2,34),(12,.3,15),'MachinedSteel',.25)
for x,z,w,h in [(44,38,2.2,1),(48,38,2.2,1),(44,34,2.2,1),(48,34,2.2,1),(44,30,3,1.2),(49,30,1.6,1.6)]:box('RearPort',(x,-24.45,z),(w,.2,h),'Port',.1)
box('RearPSU',(49,-24.25,10),(16,.35,9),'Bezel',.2);box('PSUPowerSocket',(44.5,-24.6,10),(3,.6,2.2),'Port',.2)
for z in [7,9,11,13]:box('PSUVent',(52,-24.5,z),(6,.2,.5),'Port',.12)
cable('TowerPowerCord',[(44.5,-25,10),(42,-30,12),(37,-28,38),(35,-25,65),(25,-24,68)],.32)
cable('TowerDisplayCable',[(45,-25,34),(40,-29,38),(36,-27,62),(30,-24,68)],.24)
