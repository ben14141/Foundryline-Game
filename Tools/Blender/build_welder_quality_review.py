"""Original MIG equipment rebuild, isolated model-only output while Unreal is in use."""
import bpy,bmesh,math,json,hashlib,sys
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29';O.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);S=bpy.context.scene;S.unit_settings.system='IMPERIAL';S.unit_settings.scale_length=1;groups={};parts=[];label_qa=[]
def group(n,p=(0,0,0)):
 o=bpy.data.objects.new(n,None);S.collection.objects.link(o);o.location=p;groups[n]=o;return o
cart=group('CartFrame');wheels=group('CartWheels');power=group('PowerUnit');bottle=group('GasCylinder');reg=group('GasRegulator');gun=group('MIGGun',(0,0,0));leads=group('Leads');clamp=group('WorkClamp')
def mat(n,c,metal=0,rough=.4,coat=0):
 m=bpy.data.materials.new(n);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;p.inputs['Coat Weight'].default_value=coat;return m
orange=mat('WQ_PowderOrange',(.75,.13,.018),.18,.27,.45);graphite=mat('WQ_Graphite',(.025,.035,.046),.55,.31);steel=mat('WQ_BrushedSteel',(.34,.40,.46),.9,.29);rubber=mat('WQ_Rubber',(.009,.013,.017),0,.7);copper=mat('WQ_Copper',(.50,.22,.075),.94,.23);brass=mat('WQ_Brass',(.52,.32,.085),.88,.27);blue=mat('WQ_GasBlue',(.025,.12,.24),.35,.29,.4);white=mat('WQ_LabelWhite',(.75,.83,.87),0,.4);black=mat('WQ_ScreenBlack',(.003,.012,.018),0,.22);display=mat('WQ_DisplayCyan',(.12,.65,.79),0,.25);p=display.node_tree.nodes.get('Principled BSDF');p.inputs['Emission Color'].default_value=(.08,.55,.75,1);p.inputs['Emission Strength'].default_value=.8
glass=mat('WQ_ClearWindow',(.38,.49,.55),0,.12);p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Transmission Weight'].default_value=.92;p.inputs['IOR'].default_value=1.45
def finish(o,n,m,g):
 o.name=n;o.data.materials.append(m);M=o.matrix_world.copy();o.parent=g;o.matrix_world=M;parts.append(o);return o
def box(n,p,d,m,g=power,bevel=.003,rot=(0,0,0)):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p,rotation=rot);o=bpy.context.object;o.dimensions=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);finish(o,n,m,g)
 if bevel:b=o.modifiers.new('Edge radius','BEVEL');b.width=bevel;b.segments=3;w=o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');w.keep_sharp=True
 return o
def cyl(n,p,r,d,m,g=power,axis='Z',N=48):
 rot=(math.pi/2,0,0) if axis=='Y' else (0,math.pi/2,0) if axis=='X' else (0,0,0);bpy.ops.mesh.primitive_cylinder_add(vertices=N,radius=r,depth=d,location=p,rotation=rot);o=finish(bpy.context.object,n,m,g)
 for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
 return o
def rod(n,a,b,r,m,g=cart):
 a,b=Vector(a),Vector(b);o=cyl(n,(a+b)/2,r,(b-a).length,m,g);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def tube(n,points,r,m,g=leads):
 c=bpy.data.curves.new(n,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=3;sp=c.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
 for p,co in zip(sp.bezier_points,points):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 o=bpy.data.objects.new(n,c);S.collection.objects.link(o);return finish(o,n,m,g)
def ring(n,p,major,minor,m,g,axis='Z'):
 rot=(math.pi/2,0,0) if axis=='Y' else (0,math.pi/2,0) if axis=='X' else (0,0,0);bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=48,minor_segments=8,location=p,rotation=rot);return finish(bpy.context.object,n,m,g)
def text(n,t,p,size,m,g=power,face='X'):
 bpy.ops.object.text_add(location=p,rotation=(math.pi/2,0,math.pi/2) if face=='X' else (math.pi/2,0,0));o=bpy.context.object;o.data.body=t;o.data.size=size;o.data.extrude=0;o.data.bevel_depth=0;o.data.resolution_u=8
 bpy.ops.object.convert(target='MESH');o=bpy.context.object;finish(o,n,m,g);bpy.context.view_layer.update();wrapped=n.startswith('Cylinder') or n=='TorchModel'
 if wrapped:
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.subdivide_edges(bm,edges=list(bm.edges),cuts=3,use_grid_fill=True);bm.to_mesh(o.data);bm.free()
 def base_distance(q):
  if n.startswith('Cylinder'):return math.sqrt((q.x+.42)**2+q.y*q.y)-.112
  if n=='TorchModel':return math.sqrt(q.y*q.y+(q.z-.079)**2)-.024
  return (q.x-.5035) if n in ['AmpReadout','VoltReadout','ReadoutUnits'] else (q.x-.485) if face=='X' else (-q.y-.1665) if n=='GaugeUnits' else (-q.y-.240)
 M=o.matrix_world.copy();inv=M.inverted();offset=.00015
 for v in o.data.vertices:
  q=M@v.co
  if n.startswith('Cylinder'):
   dx=q.x+.42;assert abs(dx)<.112,(n,q[:]);r=.112+offset;q.y=-math.sqrt(r*r-dx*dx)
  elif n=='TorchModel':dz=q.z-.079;r=.024+offset;assert abs(dz)<r;q.y=-math.sqrt(r*r-dz*dz)
  elif n in ['AmpReadout','VoltReadout','ReadoutUnits']:q.x=.5035+offset
  elif face=='X':q.x=.485+offset
  elif n=='GaugeUnits':q.y=-.1665-offset
  else:q.y=-.240-offset
  v.co=inv@q
 o.data.update();samples=[M@v.co for v in o.data.vertices]+[M@f.center for f in o.data.polygons];target=bpy.data.objects.get('DrawnGasCylinder' if n.startswith('Cylinder') else 'TorchBarrel') if wrapped else None
 def actual_gap(q):
  if not target:return base_distance(q)*1000
  ok,near,normal,_=target.closest_point_on_mesh(target.matrix_world.inverted()@q);assert ok
  delta=q-target.matrix_world@near;return delta.length*1000*(1 if delta.dot(target.matrix_world.to_3x3()@normal)>=0 else -1)
 gaps=[actual_gap(q) for q in samples];assert min(gaps)>=.02 and max(gaps)<.30,(n,min(gaps),max(gaps));normaldots=[]
 for f in o.data.polygons:
  q=M@f.center;expected=(Vector((1,0,0)) if face=='X' else Vector((0,-1,0))) if not target else target.matrix_world.to_3x3()@target.closest_point_on_mesh(target.matrix_world.inverted()@q)[2]
  normaldots.append((M.to_3x3()@f.normal).normalized().dot(expected.normalized()))
 assert min(normaldots)>.95,(n,min(normaldots))
 label_qa.append({'name':n,'surface':'cylinder_wrapped' if wrapped else 'panel_flush','min_gap_mm':min(gaps),'max_gap_mm':max(gaps),'min_surface_normal_dot':min(normaldots),'extrusion_mm':0,'vertices':len(o.data.vertices)})
 return o
def lathe(n,profile,m,g,xy=(0,0),N=64):
 v=[];f=[]
 for r,z in profile:
  for j in range(N):a=j*math.tau/N;v.append((xy[0]+r*math.cos(a),xy[1]+r*math.sin(a),z))
 for i in range(len(profile)-1):
  for j in range(N):a=i*N+j;b=i*N+(j+1)%N;f.append((a,b,b+N,a+N))
 mesh=bpy.data.meshes.new(n);mesh.from_pydata(v,[],f);mesh.update();o=bpy.data.objects.new(n,mesh);S.collection.objects.link(o);finish(o,n,m,g)
 for p in mesh.polygons:p.use_smooth=True
 return o
# Dedicated folded-steel cart; clear bottle bay, independent original axle/wheel groups.
box('FoldedMainTray',(0,0,.36),(1.05,.62,.04),graphite,cart,.009);box('LowerToolShelf',(.08,0,.205),(.76,.51,.022),graphite,cart,.006)
for y in [-.295,.295]:
 box('TrayUpstand',(0,y,.395),(1.04,.022,.06),graphite,cart)
 for x in [-.40,.39]:box('CartTubeLeg',(x,y*.84,.25),(.038,.038,.23),graphite,cart)
 rod('PushHandleUpright',(-.43,y*.79,.37),(-.53,y*.79,.89),.018,graphite);ring('CableHook',(.08,y*1.1,.47),.035,.006,steel,cart,'X')
rod('PushGrip',(-.53,-.235,.89),(-.53,.235,.89),.022,rubber)
for x in [-.40,.39]:
 rod('RealWheelAxle',(x,-.33,.125),(x,.33,.125),.018,steel)
 for side in [-1,1]:
  y=side*.30;cyl('Tyre',(x,y,.125),.12,.060,rubber,wheels,'Y');cyl('Rim',(x,y+side*.032,.125),.071,.008,steel,wheels,'Y');cyl('AxleCap',(x,y+side*.04,.125),.023,.012,graphite,wheels,'Y')
  for i in range(6):a=i*math.tau/6;cyl('WheelStud',(x+math.sin(a)*.045,y+side*.041,.125+math.cos(a)*.045),.005,.004,steel,wheels,'Y',6)
# Main enclosure formed around a protected recessed control panel and a visible feed bay.
box('MainCaseBottom',(.12,0,.4275),(.70,.45,.035),graphite,power,.012);box('MainCaseTop',(.12,0,.945),(.70,.45,.035),graphite,power,.012);box('RearCasePanel',(-.224,0,.68),(.02,.45,.50),graphite,power,.008)
for y in [-.232,.232]:
 # Panel strips leave a real opening; the window does not sit on an opaque backing.
 for x in [-.17,.38]:box('SideDoorEdge',(x,y,.71),(.055,.016,.35),orange)
 for z in [.52,.90]:box('SideDoorEdge',(.105,y,z),(.60,.016,.045),orange)
 box('ServiceDoorLower',(.105,y,.58),(.49,.018,.09),orange)
 for i in range(10):box('LowerVentLouver',(-.12+i*.05,y*1.04,.57),(.027,.009,.043),graphite,bevel=.001)
 for x in [-.17,.38]:
  for z in [.53,.89]:cyl('QuarterTurnPanelFastener',(x,y*1.06,z),.007,.008,steel,axis='Y',N=6)
for side in [-1,1]:box('FrontCaseCorner',(.438,side*.232,.70),(.060,.018,.45),graphite,power,.006)
box('DoorWindow',(.105,-.245,.76),(.46,.003,.22),glass,power,.008)
for y in [.247]:box('SolidFarDoor',(.105,y,.76),(.49,.015,.23),orange)
text('SideBrand','FOUNDRYLINE',(-.16,-.251,.895),.025,white,face='Y');text('SideModel','MIG / 250',(-.14,-.252,.515),.021,white,face='Y')
# Visible wire spool/feed train, genuinely inside the opened-side enclosure.
cyl('WireSpoolCore',(-.045,-.12,.75),.088,.11,copper,axis='Y')
for y in [-.183,-.057]:
 cyl('SpoolFlange',(-.045,y,.75),.11,.012,graphite,axis='Y');ring('SpoolFlangeRim',(-.045,y-.008,.75),.098,.006,steel,power,'Y')
for i in range(12):ring('CopperWireWinding',(-.045,-.174+i*.009,.75),.087,.0018,copper,power,'Y')
cyl('SpoolLockHub',(-.045,-.20,.75),.026,.026,steel,axis='Y',N=6)
for x in [.18,.25]:
 cyl('FeedRoller',(x,-.19,.69),.024,.032,steel,axis='Y');cyl('FeedRollerScrew',(x,-.213,.69),.008,.009,graphite,axis='Y',N=6)
tube('CopperWireGuide',[(-.045,-.14,.66),(.09,-.17,.67),(.20,-.19,.68),(.39,-.17,.68)],.001,copper,power)
rod('DriveRollerTensionLever',(.18,-.18,.73),(.25,-.18,.77),.007,graphite,power);cyl('DriveTensionKnob',(.26,-.18,.77),.018,.025,rubber,axis='Y')
for x in [-.12,.31]:box('CarryHandlePedestal',(x,0,.976),(.07,.08,.05),graphite)
rod('TopCarryHandle',(-.12,0,1.012),(.31,0,1.012),.021,rubber,power)
box('RecessedFrontFace',(.478,0,.70),(.014,.45,.43),graphite,power,.012)
box('DisplayRecess',(.491,0,.85),(.015,.322,.123),rubber,power,.009);box('GlassReadout',(.501,0,.85),(.005,.29,.092),black,power,.003)
text('AmpReadout','185',(.505,-.13,.84),.047,display);text('VoltReadout','24.5',(.505,.025,.84),.035,display);text('ReadoutUnits','A          V',(.505,-.128,.815),.014,white)
for y,label in [(-.12,'WIRE'),(.12,'VOLT')]:
 cyl('ControlEncoder',(.511,y,.704),.031,.038,rubber,axis='X');ring('EncoderRing',(.532,y,.704),.026,.002,steel,power,'X');box('KnobIndex',(.533,y,.728),(.006,.005,.010),white)
 text('EncoderLegend',label,(.503,y-.035,.752),.014,white)
box('PowerRocker',(.504,0,.705),(.023,.032,.046),orange,power,.004)
for y,label in [(-.13,'MIG'),(.13,'WORK')]:
 cyl('RecessedSocketFlange',(.505,y,.56),.035,.020,steel,axis='X');cyl('SocketInsulator',(.523,y,.56),.027,.024,rubber,axis='X');cyl('EuroCablePlug',(.55,y,.56),.020,.039,brass,axis='X',N=12)
 text('SocketLegend',label,(.504,y-.033,.613),.013,white)
for i in range(9):box('FrontExhaustSlot',(.493,-.16+i*.04,.45),(.009,.025,.055),rubber,bevel=.001)
# Proper smoothly drawn cylinder shoulders, dished base and open foot ring.
lathe('DrawnGasCylinder',[(0,.405),(.065,.405),(.10,.418),(.112,.45),(.112,1.33),(.109,1.38),(.095,1.415),(.066,1.445),(.027,1.464),(.025,1.485)],blue,bottle,(-.42,0))
lathe('OpenFootRing',[(.108,.38),(.121,.38),(.121,.43),(.117,.445),(.108,.445),(.108,.38)],graphite,bottle,(-.42,0))
for z in [.65,1.12]:
 ring('BottleRetainingBand',(-.42,0,z),.115,.007,steel,bottle)
 for side in [-1,1]:rod('RetainingChainTab',(-.42,side*.12,z),(-.46,side*.20,z),.008,graphite,cart)
for i in range(31):
 t=i/30;x=-.46+.175*math.sin(t*math.pi);y=-.20+.40*t;ring('SafetyChainLink',(x,y,1.14+(i%2)*.004),.0115,.002,steel,bottle,'Y' if i%2 else 'X')
text('CylinderGasLabel','WELD GAS',(-.50,-.113,1.01),.028,white,bottle,'Y');text('CylinderLabelSmall','SHIELDING GAS',(-.505,-.114,.97),.016,white,bottle,'Y')
cyl('BottleNeckValve',(-.42,0,1.523),.022,.085,brass,bottle);cyl('HandwheelStem',(-.42,0,1.586),.008,.035,steel,bottle)
ring('ValveHandwheel',(-.42,0,1.605),.044,.007,graphite,bottle)
for i in range(4):a=i*math.tau/4;rod('HandwheelSpoke',(-.42,0,1.605),(-.42+.04*math.cos(a),.04*math.sin(a),1.605),.004,graphite,bottle)
# Regulator on the rearward side, two gauges and hose barb, no fantasy flame nozzle.
box('RegulatorBody',(-.42,-.074,1.53),(.105,.10,.064),brass,reg,.009);cyl('PressureAdjustment',(-.42,-.142,1.51),.023,.038,graphite,reg,'Y');cyl('RegulatorUnion',(-.42,-.025,1.53),.024,.045,brass,reg,'Y',6)
for x,label,needle in [(-.48,'BAR',.65),(-.36,'L/M',1.8)]:
 cyl('GaugeCase',(x,-.147,1.584),.037,.029,steel,reg,'Y');cyl('GaugeDial',(x,-.165,1.584),.032,.003,white,reg,'Y')
 for i in range(21):
  a=-math.pi*.8+i*math.pi*1.6/20;z=1.584+math.cos(a)*.026;xx=x+math.sin(a)*.026;o=box('GaugeTick',(xx,-.16660,z),(.0014,.0001,.004 if i%5 else .007),graphite,reg,0);o.rotation_euler.y=-a
 rod('GaugeNeedle',(x,-.169,1.584),(x+math.sin(needle)*.021,-.169,1.584+math.cos(needle)*.021),.0012,orange,reg);cyl('GaugeNeedlePivot',(x,-.170,1.584),.003,.002,graphite,reg,'Y')
 text('GaugeUnits',label,(x-.012,-.170,1.568),.008,graphite,reg,'Y');cyl('GaugeGlass',(x,-.172,1.584),.032,.0015,glass,reg,'Y')
cyl('GasOutletBarb',(-.38,-.117,1.49),.010,.037,brass,reg)
tube('ShieldGasHose',[(-.38,-.117,1.47),(-.55,-.14,1.33),(-.57,-.18,.66),(-.27,-.15,.58)],.008,rubber,leads)
# New ergonomic MIG torch, exact original grip/support/tip contract.
box('ErgonomicTorchHandle',(-.003,0,-.006),(.043,.046,.133),graphite,gun,.010,rot=(0,-.18,0))
for side in [-1,1]:
 box('OvermouldGrip',(-.007,side*.024,-.01),(.031,.008,.092),rubber,gun,.006,rot=(0,-.18,0))
 for z in [-.042,-.024,-.006,.012,.030]:box('GripTraction',(-.006,side*.029,z),(.027,.002,.003),graphite,gun,.0007)
 for z in [-.045,.040]:cyl('TorchShellScrew',(-.003,side*.030,z),.0028,.003,steel,gun,'Y',6)
box('MIGTrigger',(.025,0,.012),(.009,.028,.051),orange,gun,.003);cyl('TriggerHinge',(.025,0,.043),.004,.036,steel,gun,'Y')
tube('TriggerGuard',[(.022,-.019,.046),(.039,-.020,.028),(.039,-.020,-.032),(.010,-.019,-.056)],.0032,graphite,gun)
cyl('TorchBarrel',(.029,0,.079),.024,.087,orange,gun,'X');cyl('BarrelRearCap',(-.018,0,.079),.025,.008,graphite,gun,'X')
tube('ChromedSwanNeck',[(.064,0,.079),(.090,0,.090),(.121,0,.123),(.152,0,.135)],.0075,steel,gun);cyl('NeckHex',(.068,0,.081),.015,.016,graphite,gun,'X',6)
def hollow_x(n,start,end,ro,ri,m):
 v=[];f=[];N=64
 for x,r in [(start,ro),(end,ro),(end,ri),(start,ri)]:
  for i in range(N):a=i*math.tau/N;v.append((x,r*math.cos(a),.135+r*math.sin(a)))
 for j in range(4):
  for i in range(N):f.append((j*N+i,j*N+(i+1)%N,((j+1)%4)*N+(i+1)%N,((j+1)%4)*N+i))
 mesh=bpy.data.meshes.new(n);mesh.from_pydata(v,[],f);mesh.update();o=bpy.data.objects.new(n,mesh);S.collection.objects.link(o);finish(o,n,m,gun)
 for face in mesh.polygons:face.use_smooth=True
 return o
cyl('NozzleInsulator',(.153,0,.135),.014,.016,graphite,gun,'X');hollow_x('CopperNozzle_OpenBore',.161,.225,.011,.0082,copper);hollow_x('ContactTip_HollowWireGuide',.20,.221,.0032,.0009,copper);cyl('GasDiffuser',(.185,0,.135),.0058,.030,brass,gun,'X');rod('ExposedWeldingWire',(.219,0,.135),(.239,0,.135),.0006,steel,gun)
for x in [.167,.185,.218]:ring('NozzleTurnedGroove',(x,0,.135),.011,.00045,copper,gun,'X')
rod('TorchStrainRelief',(0,0,-.065),(-.016,0,-.12),.015,rubber,gun)
for z in [-.077,-.088,-.099,-.110]:ring('StrainReliefFlexRib',(-.005,0,z),.016,.0018,rubber,gun)
text('TorchModel','FL MIG',(.005,-.025,.075),.008,white,gun,'Y')
for n,p in [('Grip',(0,0,0)),('Support',(.03,-.03,.08)),('WeldTip',(.239,0,.135)),('TriggerPivot',(.025,0,.043))]:o=bpy.data.objects.new('SOCKET_'+n,None);S.collection.objects.link(o);o.parent=gun;o.location=p
# Clamped copper jaws and insulated handles, with editable pivot datum.
box('ClampLowerJaw',(.015,0,0),(.15,.027,.021),copper,clamp,.003);box('ClampUpperJaw',(.015,0,.026),(.14,.027,.018),copper,clamp,.003,rot=(0,-.10,0));cyl('ClampPivot',(.002,0,.015),.010,.040,steel,clamp,'Y');box('ClampLowerGrip',(-.045,0,-.003),(.065,.035,.029),rubber,clamp,.006);box('ClampUpperGrip',(-.038,0,.038),(.063,.035,.027),rubber,clamp,.006)
for i in range(6):box('ClampJawSerration',(.045+i*.007,0,.014),(.003,.023,.004),steel,clamp,.0004)
# Stage torch/clamp at their original idle attachment transforms after local modelling.
for child in gun.children:child.matrix_parent_inverse.identity()
gun.location=(.17,-.38,.87);clamp.location=(.05,.44,.08)
tube('MIGCoaxLead',[(.573,-.13,.56),(.64,-.13,.56),(.76,-.20,.30),(.86,-.49,.04),(.50,-.82,.035),(-.03,-.74,.035),(-.20,-.48,.12),(.154,-.38,.75)],.013,rubber,leads)
tube('GroundLead',[(.573,.13,.56),(.64,.13,.56),(.77,.21,.10),(.65,.48,.035),(.22,.56,.035),(.00,.44,.078)],.010,rubber,leads)
for y in [-.13,.13]:
 for i in range(5):ring('PlugStrainRelief',(.585+i*.009,y,.56),.022-i*.001,.002,rubber,leads,'X')
rod('HolsterArm',(.16,-.235,.86),(.16,-.35,.86),.010,graphite,cart);ring('TorchHolster',(.16,-.36,.86),.030,.005,steel,cart)
bpy.context.view_layer.update()
def desc(g):return [g,*g.children_recursive]
def bounds(obs):
 pts=[];dg=bpy.context.evaluated_depsgraph_get()
 for o in obs:
  if o.type not in {'MESH','CURVE','FONT'}:continue
  e=o.evaluated_get(dg);m=e.to_mesh();pts.extend(e.matrix_world@v.co for v in m.vertices);e.to_mesh_clear()
 return {'min_cm':[min(p[i] for p in pts)*100 for i in range(3)],'max_cm':[max(p[i] for p in pts)*100 for i in range(3)]}
def export(n,obs,local=False):
 saved={o:o.matrix_world.copy() for o in obs};offset=gun.location.copy() if n=='Welder_MIGGun' else clamp.location.copy() if n=='Welder_WorkClamp' else Vector((0,0,0))
 if local:
  from mathutils import Matrix
  for o in obs:o.matrix_world=Matrix.Translation(-offset)@saved[o]
 bpy.ops.object.select_all(action='DESELECT')
 for o in obs:o.select_set(True)
 bpy.context.view_layer.objects.active=next(o for o in obs if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(O/(n+'.fbx')),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 for o,M in saved.items():o.matrix_world=M
entries=[]
for g in groups.values():entries.append({'name':g.name,'pivot_cm':list(g.location*100),'bounds':bounds(desc(g))});export(g.name,desc(g))
export('Welder_Machine',[o for g in [cart,wheels,power,bottle,reg] for o in desc(g)])
export('Welder_MIGGun',desc(gun),True);export('Welder_WorkClamp',desc(clamp),True);export('Welder_Leads',desc(leads));export('Welder_Complete_Review',[o for g in groups.values() for o in desc(g)])
checks={'gun_grip_cm':[0,0,0],'gun_support_cm':[3,-3,8],'gun_weldtip_cm':[23.9,0,13.5],'nozzle_outer_diameter_mm':22,'nozzle_bore_diameter_mm':16.4,'wire_diameter_mm':1.2,'cylinder_on_tray_cm':38,'independent_groups':8,'original_authored_no_vendor_geometry':True,'game_files_unchanged':True,'actual_unreal_import':False}
assert (bpy.data.objects['SOCKET_WeldTip'].location-Vector((.239,0,.135))).length<1e-6
assert not any(o.type=='FONT' for o in bpy.data.objects)
(O/'label_surface_qa.json').write_text(json.dumps({'labels':label_qa,'all_label_geometry_surface_applied':True,'remaining_font_objects':0,'gauge_tick_gap_mm':[.05,.15]},indent=2))
(O/'manifest.json').write_text(json.dumps({'assets':entries,'qa':checks,'labels_surface_applied':True,'units':'Blender metres; Unreal centimetres. Display feet/inches.','compatible_idle_gun_cm':[17,-38,87],'compatible_idle_clamp_cm':[5,44,8]},indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(O/'Foundryline_MIGWelder_QualityReview.blend'))
S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=16;S.cycles.use_denoising=True;S.render.threads_mode='FIXED';S.render.threads=2;S.render.resolution_x=1200;S.render.resolution_y=1000;S.render.resolution_percentage=100
world=bpy.data.worlds.new('ReviewWorld');S.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.14,.17,.21,1);world.node_tree.nodes['Background'].inputs[1].default_value=.45
floor=mat('ReviewGround',(.10,.12,.15),0,.7);box('REVIEW_Floor',(0,0,-.020),(15,15,.02),floor,None)
def area(p,power,size,target):
 bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
area((2,-3,4),600,3,(0,0,.8));area((-2,1,3),450,2,(0,0,.8))
def render(n,p,target,scale):
 bpy.ops.object.camera_add(location=p);o=bpy.context.object;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();o.data.type='ORTHO';o.data.ortho_scale=scale;S.camera=o;S.render.filepath=str(O/(n+'.png'));bpy.ops.render.render(write_still=True)
render('Welder_Updated_Overview',(2.6,-3.4,2.2),(0,-.07,.8),2.20)
if '--overview-only' not in sys.argv:render('Welder_Bottle_Regulator',(-1.4,-1.7,2.05),(-.42,-.04,1.30),.85)
for g in [cart,wheels,power,bottle,reg,leads,clamp]:
 for o in desc(g):o.hide_render=True
if '--overview-only' not in sys.argv:render('Welder_MIGGun_Closeup',(.65,-.94,1.18),(.275,-.38,.91),.43)
print('WELDER_QUALITY_REBUILD_READY',len(parts),'original parts; eight independent groups; compatible torch anchors')
