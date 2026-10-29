"""Detailed game-owned 10-foot press brake, with independent tooling and moving parts."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
exec((Path(__file__).parent/'build_material_handling_assets.py').read_text().split('def import_factory')[0])
OUT=R/'SourceAssets/Production/Equipment/PressBrake';OUT.mkdir(parents=True,exist_ok=True)
groups={k:[] for k in ['Body','Ram','UpperPunch','PistonRods','LowerDie','BackGauge','OperatorConsole','FootPedal','ControlCables']}
def put(group,o):groups[group].append(o);return o
def rod(name,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,m,rot=(0,0,0));o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def hose(name,points,r,m=rubber):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2
 s=c.splines.new('BEZIER');s.bezier_points.add(len(points)-1)
 for p,co in zip(s.bezier_points,points):p.co=co;p.handle_left_type=p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,c);scene.collection.objects.link(o);o.data.materials.append(m)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return bpy.context.object
def prism(name,profile,y0,y1,m):
 vs=[(x,y,z) for y in [y0,y1] for x,z in profile];n=len(profile)
 faces=[tuple(reversed(range(n))),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(vs,[],faces);mesh.materials.append(m);o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o)
 mod=o.modifiers.new('Machined edge bevel','BEVEL');mod.width=.003;mod.segments=2;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name);return o
def bolt(group,name,loc,r=.014,axis='X',m=steel):
 rot=(0,math.pi/2,0) if axis=='X' else (math.pi/2,0,0) if axis=='Y' else (0,0,0)
 put(group,cyl(name,loc,r,.018,m,rot=rot,verts=6))
# Fixed frame: genuine C-throat side plates rather than a filled rectangular box.
outline=[(-.72,.12),(.70,.12),(.70,.78),(.04,.78),(.04,1.52),(.70,1.52),(.70,2.75),(-.72,2.75)]
for side in [-1,1]:
 y=side*1.78
 put('Body',prism('CFrame',outline,y-.14,y+.14,orange))
 put('Body',box('FrameOuterServicePlate',(-.28,side*1.935,1.62),(.67,.025,1.67),graphite,bevel=.018))
 for x in [-.55,.03]:
  for z in [.91,1.51,2.34]:bolt('Body','FrameCoverBolt',(x,side*1.958,z),axis='Y')
 for x in [-.47,.47]:
  put('Body',box('AnchoringFoot',(x,y,.07),(.62,.55,.14),dark,bevel=.016))
  for dx in [-.22,.22]:
   for dy in [-.18,.18]:bolt('Body','FloorAnchor',(x+dx,y+dy,.151),.021,'Z')
 put('Body',box('RamGuideRail',(.69,side*1.59,1.92),(.055,.09,.83),steel,bevel=.008))
 for z in [1.62,1.87,2.14,2.30]:bolt('Body','GuideRailBolt',(.728,side*1.59,z),.012)
put('Body',box('RearCrossFrame',(-.47,0,1.59),(.42,3.45,2.20),graphite,bevel=.028))
put('Body',box('TopBridge',(.02,0,2.55),(1.25,3.55,.42),orange,bevel=.025))
put('Body',box('LowerBed',(.39,0,.61),(.70,3.55,.77),orange,bevel=.022))
put('Body',box('BedFrontSkin',(.755,0,.65),(.025,3.36,.48),graphite,bevel=.008))
put('Body',box('DieHolder',(.52,0,1.00),(.21,3.36,.08),dark,bevel=.008))
# Segmented lower V-die, 3,048 mm usable tooling length.
for i in range(12):
 a=-1.524+i*.254+.001;b=a+.252
 put('LowerDie',prism('LeftVDie',[(.43,1.04),(.48,1.04),(.52,.985),(.52,.96),(.43,.96)],a,b,steel))
 put('LowerDie',prism('RightVDie',[(.52,.96),(.52,.985),(.56,1.04),(.61,1.04),(.61,.96)],a,b,steel))
 bolt('Body','DieLockScrew',(.643,(a+b)/2,1.0),.009)
# Ram and clamp rail. The punch is a separate changeable tool attached to the ram.
put('Ram',box('MovingRam',(.45,0,1.84),(.49,3.35,.64),orange,bevel=.018))
put('Ram',box('RamFrontPanel',(.71,0,1.94),(.026,3.18,.32),graphite,bevel=.01))
put('Ram',label('FoundrylineBrand','FOUNDRYLINE',(.732,-.83,1.91),(math.pi/2,0,math.pi/2),.145,white))
put('Ram',label('ModelMark','PB-10 / HYDRAULIC',(.733,-.72,1.82),(math.pi/2,0,math.pi/2),.047,orange))
put('Ram',box('ToolClampRail',(.52,0,1.505),(.17,3.18,.09),dark,bevel=.006))
for i in range(12):
 y=-1.397+i*.254
 put('Ram',box('QuickClamp',(.625,y,1.49),(.055,.205,.085),steel,bevel=.005))
 put('Ram',cyl('ClampKnob',(.67,y,1.49),.021,.035,dark,rot=(0,math.pi/2,0)))
 put('UpperPunch',prism('UpperPunchSegment',[(.49,1.475),(.55,1.475),(.55,1.40),(.52,1.34),(.49,1.40)],y-.126,y+.126,steel))
for y in [-1.48,1.48]:
 for z in [1.82,2.07]:bolt('Ram','RamPanelBolt',(.736,y,z),.012)
# Two hydraulic cylinders, exposed chrome rods, bolted end plates and pipe fittings.
for side in [-1,1]:
 y=side*1.43
 put('Body',cyl('CylinderBarrel',(.35,y,2.80),.14,.70,dark,rot=(0,0,0),verts=32))
 for z in [2.45,3.15]:
  put('Body',box('CylinderEndCap',(.35,y,z),(.34,.34,.065),orange,bevel=.012))
  for dx in [-.12,.12]:
   for dy in [-.12,.12]:bolt('Body','CylinderTieBolt',(.35+dx,y+dy,z+.04),.015,'Z')
 put('PistonRods',cyl('ChromePistonRod',(.35,y,2.495),.049,.95,steel,rot=(0,0,0),verts=32))
 put('PistonRods',box('RodClevis',(.35,y,2.065),(.18,.18,.13),dark,bevel=.012))
 put('PistonRods',cyl('ClevisPin',(.35,y,2.065),.035,.22,steel))
 for z in [2.57,3.02]:
  put('Body',cyl('HydraulicPort',(.35,y-.18,z),.027,.06,steel))
  put('Body',hose('CylinderHydraulicHose',[(.35,y-.21,z),(.02,y-.30,z+.08),(-.55,y-.24,2.62),(-.79,-1.25,1.43)],.016))
 for x in [.24,.46]:put('Body',rod('CylinderTieRod',(x,y-.11,2.47),(x,y-.11,3.13),.012,steel))
# Rear hydraulic power unit and electrical cabinet, with service details.
put('Body',box('OilReservoir',(-.86,-.95,.67),(.61,.76,1.03),graphite,bevel=.025))
put('Body',box('TankLid',(-.86,-.95,1.20),(.66,.80,.055),dark,bevel=.012))
put('Body',cyl('HydraulicPump',(-.86,-.96,1.40),.16,.32,dark,rot=(0,0,0),verts=32))
for z in [1.26,1.30,1.34,1.38,1.42,1.46,1.50]:put('Body',cyl('MotorCoolingFin',(-.86,-.96,z),.174,.013,graphite,rot=(0,0,0)))
put('Body',cyl('OilFiller',(-.86,-.69,1.27),.046,.075,red,rot=(0,0,0)))
put('Body',box('SightGlass',(-.541,-.95,.82),(.018,.09,.25),steel,bevel=.005))
put('Body',box('SightGlassWindow',(-.53,-.95,.82),(.008,.055,.20),glass))
put('Body',box('ElectricalCabinet',(-.85,.95,.91),(.60,.83,1.54),graphite,bevel=.02))
put('Body',box('CabinetDoor',(-1.163,.95,.91),(.024,.73,1.42),dark,bevel=.008))
for z in [.46,.53,.60,.67,.74]:put('Body',box('CabinetVent',(-1.181,.95,z),(.012,.48,.018),graphite))
put('Body',box('CabinetDoorHandle',(-1.195,.67,1.17),(.04,.028,.18),steel,bevel=.005))
put('Body',box('ElectricalWarning',(-1.185,1.04,1.37),(.01,.17,.16),yellow,bevel=.004))
put('Body',label('ElectricalWarningMark','!',(-1.195,1.08,1.32),(math.pi/2,0,-math.pi/2),.12,graphite))
# Back gauge crossbar, fingers and screw-drive; separate for later X/R adjustment.
for y in [-1.05,1.05]:
 put('Body',rod('BackGaugeGuide',(-.70,y,.96),(.30,y,.96),.025,steel))
 put('Body',box('GuideSupport',(-.65,y,.87),(.14,.14,.22),dark,bevel=.008))
put('Body',rod('GaugeBallScrew',(-.72,0,.94),(.27,0,.94),.018,steel))
for x in [-.65,-.57,-.49,-.41,-.33,-.25,-.17,-.09,-.01,.07,.15]:
 put('Body',cyl('BallScrewThread',(x,0,.94),.022,.018,dark,rot=(0,math.pi/2,0)))
put('BackGauge',box('GaugeCrossbar',(-.10,0,1.00),(.16,2.78,.14),steel,bevel=.008))
for y in [-.85,.85]:
 put('BackGauge',box('FingerCarriage',(-.08,y,1.07),(.22,.20,.12),dark,bevel=.008))
 put('BackGauge',box('GaugeFinger',(.17,y,1.10),(.50,.065,.05),steel,bevel=.005))
 put('BackGauge',box('FingerStop',(.408,y,1.14),(.025,.095,.09),steel,bevel=.004))
 put('BackGauge',cyl('FingerAdjustKnob',(-.08,y,1.155),.031,.035,graphite,rot=(0,0,0)))
# Front sliding sheet supports and manual adjustments.
put('Body',box('SupportSlideRail',(.87,0,.84),(.12,3.32,.12),dark,bevel=.008))
for y in [-.91,.91]:
 put('Body',box('SupportCarriage',(.92,y,.85),(.22,.25,.22),steel,bevel=.008))
 put('Body',box('SupportArm',(1.25,y,.97),(.73,.15,.09),graphite,bevel=.008))
 put('Body',box('SupportWearStrip',(1.25,y,1.021),(.70,.12,.012),steel,bevel=.003))
 put('Body',cyl('SupportLockKnob',(1.00,y-.17,.85),.041,.06,orange))
 for x in [1.02,1.27,1.51]:
  put('Body',cyl('SheetSupportRoller',(x,y,1.022),.018,.14,steel))
# Light-curtain housings, warning bands and emergency stops.
for side in [-1,1]:
 y=side*1.68
 put('Body',box('LightCurtainPost',(.94,y,1.23),(.085,.085,.76),dark,bevel=.008))
 put('Body',box('LightCurtainWindow',(.943,y-side*.047,1.23),(.052,.01,.65),glass))
 for z in [.98,1.13,1.28,1.43]:put('Body',box('SensorDiode',(.944,y-side*.055,z),(.022,.006,.013),red))
 put('Body',box('SafetyPostFoot',(.94,y,.84),(.17,.16,.08),yellow,bevel=.008))
 put('Body',cyl('MachineEmergencyStop',(.76,side*1.52,.75),.035,.04,red,rot=(0,math.pi/2,0)))
for i in range(30):
 put('Body',box('HazardStripe',(.774,-1.55+i*.107,.47),(.012,.057,.12),yellow if i%2==0 else dark,rot=(.35,0,0)))
put('Body',label('KeepClear','KEEP HANDS CLEAR',(.777,-.57,.62),(math.pi/2,0,math.pi/2),.062,white))
# Separate CNC computer stand, tactile keyboard, status graphics and e-stop.
put('OperatorConsole',box('ConsoleFoot',(1.80,-2.15,.06),(.70,.77,.12),dark,bevel=.02))
put('OperatorConsole',box('ConsolePost',(1.78,-2.15,.63),(.15,.15,1.06),graphite,bevel=.012))
put('OperatorConsole',box('ConsoleDesk',(1.92,-2.15,1.11),(.62,.78,.09),orange,bevel=.016))
put('OperatorConsole',box('MonitorSupport',(1.70,-2.15,1.31),(.075,.075,.41),dark,bevel=.007))
put('OperatorConsole',box('CNCMonitor',(1.75,-2.15,1.52),(.07,.65,.41),graphite,bevel=.014))
displaymat=mat('PB_CNCDisplay',(.015,.025,.035),.48)
bs=next(n for n in displaymat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
tex=displaymat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(OUT/'T_PressBrake_CNC.png'));tex.image.pack()
displaymat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
displaymat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Emission Color']);bs.inputs['Emission Strength'].default_value=.8
mesh=bpy.data.meshes.new('CNCDisplaySurface')
mesh.from_pydata([(1.796,-2.4425,1.355),(1.796,-1.8575,1.355),(1.796,-1.8575,1.685),(1.796,-2.4425,1.685)],[],[(0,1,2,3)])
mesh.materials.append(displaymat);screen=bpy.data.objects.new('CNCDisplay',mesh);scene.collection.objects.link(screen);put('OperatorConsole',screen)
put('OperatorConsole',box('Keyboard',(2.00,-2.18,1.17),(.19,.44,.035),dark,bevel=.006))
for row in range(4):
 for col in range(12):put('OperatorConsole',box('KeyboardKey',(1.938+row*.038,-2.37+col*.034,1.192),(.026,.026,.009),steel,bevel=.0015))
put('OperatorConsole',box('Mouse',(2.02,-1.86,1.18),(.105,.063,.043),graphite,bevel=.02))
put('OperatorConsole',cyl('ConsoleEStop',(2.10,-2.44,1.185),.038,.04,red,rot=(0,0,0)))
put('OperatorConsole',box('ComputerCPU',(1.80,-2.15,.85),(.30,.30,.30),graphite,bevel=.012))
put('ControlCables',hose('ComputerControlCable',[(1.80,-2.0,.85),(1.59,-2.0,.5),(1.45,-2.1,.04),(.80,-2.10,.035),(-.60,-2.12,.035),(-.90,-1.40,.50)],.015))
put('ControlCables',hose('MonitorCable',[(1.69,-2.15,1.49),(1.64,-2.13,1.13),(1.77,-2.05,.92)],.007))
# Guarded, cabled floor pedal.
put('FootPedal',box('PedalBase',(1.90,-.85,.04),(.37,.32,.08),dark,bevel=.012))
for side in [-1,1]:put('FootPedal',box('PedalSideGuard',(1.90,-.85+side*.15,.15),(.35,.035,.23),yellow,bevel=.009))
put('FootPedal',box('PedalHood',(1.84,-.85,.27),(.24,.33,.035),yellow,rot=(0,-.14,0),bevel=.008))
put('FootPedal',box('PedalTread',(1.95,-.85,.105),(.22,.20,.035),rubber,rot=(0,.14,0),bevel=.006))
for x in [1.88,1.92,1.96,2.0]:put('FootPedal',box('PedalGripRib',(x,-.85,.13),(.012,.18,.01),graphite))
put('ControlCables',hose('FootPedalCable',[(1.74,-.85,.08),(1.48,-1.02,.035),(1.20,-1.4,.035),(.90,-1.58,.08),(.78,-1.53,.55)],.012))
# Extra service hardware shares the existing mechanical groups and materials.
exec((Path(__file__).parent/'equipment_detail_parts.py').read_text());add_details('PressBrake',globals())
# Join by mechanical function; all tiny details share the owning mesh.
objects={}
for name,items in groups.items():
 pivot=(1.80,-2.15,0) if name=='OperatorConsole' else (1.90,-.85,0) if name=='FootPedal' else (0,0,0)
 o=join('PressBrake_'+name,items,pivot);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);uv_planar(o);objects[name]=o
 if name=='OperatorConsole':
  for poly in o.data.polygons:
   if o.data.materials[poly.material_index]==displaymat:
    for loop in poly.loop_indices:
     world=o.matrix_world@o.data.vertices[o.data.loops[loop].vertex_index].co
     o.data.uv_layers.active.data[loop].uv=((world.y+2.4425)/.585,(world.z-1.355)/.33)
# Flat and formed sample part, separate product components, 3.175 mm carbon steel.
flat=box('PressBrake_SampleFlat',(0,0,.0015875),(.4,.8,.003175),sheetmat,bevel=.0004);bpy.context.scene.cursor.location=(0,0,0);bpy.context.view_layer.objects.active=flat;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');uv_planar(flat)
cross=[(-.141421,.141421)]+[(.008*math.cos(math.radians(225+i*90/8)),.0113137+.008*math.sin(math.radians(225+i*90/8))) for i in range(9)]+[(.141421,.141421)]
zmin=min(z for x,z in cross);vs=[(x,y,z-zmin) for y in [-.4,.4] for x,z in cross];n=len(cross);faces=[(i,i+1,n+i+1,n+i) for i in range(n-1)]
mesh=bpy.data.meshes.new('BentCoupon');mesh.from_pydata(vs,[],faces);mesh.materials.append(sheetmat);bent=bpy.data.objects.new('PressBrake_SampleBent90',mesh);scene.collection.objects.link(bent);bpy.context.view_layer.objects.active=bent
mod=bent.modifiers.new('Actual stock thickness','SOLIDIFY');mod.thickness=.003175;bpy.ops.object.modifier_apply(modifier=mod.name)
lo=min(v.co.z for v in bent.data.vertices)
for v in bent.data.vertices:v.co.z-=lo
uv_planar(bent)
# Combined ready-to-place display mesh, plus the independent assembly components.
copies=[]
for o in objects.values():
 c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);copies.append(c)
complete=join('PressBrake_Complete',copies);complete.hide_render=True
assets=list(objects.values())+[flat,bent,complete];entries=[]
for o in assets:
 bpy.context.view_layer.update();entries.append({'name':o.name,'position_cm':[round(v*100,5) for v in o.location],'dimensions_cm':[round(v*100,5) for v in o.dimensions],'domain':'Products/Components/Steel' if o in [flat,bent] else 'Production/Equipment/PressBrake'})
 c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);c.hide_render=False;c.animation_data_clear();c.parent=None;c.location=(0,0,0)
 bpy.ops.object.select_all(action='DESELECT');c.select_set(True);bpy.context.view_layer.objects.active=c
 bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z');bpy.data.objects.remove(c,do_unlink=True)
# Animated review: a ram stroke and back-gauge adjustment. No force/production simulation.
for name in ['UpperPunch','PistonRods']:
 o=objects[name];o.parent=objects['Ram'];o.matrix_parent_inverse=Matrix.Identity(4)
for frame,z in [(1,0),(50,0),(100,-.315),(130,-.315),(180,0),(240,0)]:objects['Ram'].location.z=z;objects['Ram'].keyframe_insert('location',frame=frame)
for frame,x in [(1,0),(45,-.18),(160,-.18),(210,0),(240,0)]:objects['BackGauge'].location.x=x;objects['BackGauge'].keyframe_insert('location',frame=frame)
flat.location=(.52,0,1.04);bent.location=(.52,0,1.025)
for frame,closed in [(1,False),(85,False),(100,True),(150,True),(180,False),(240,False)]:
 flat.hide_render=closed;bent.hide_render=not closed;flat.keyframe_insert('hide_render',frame=frame);bent.keyframe_insert('hide_render',frame=frame)
scene.frame_set(1)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));bpy.context.object.name='ReviewFloor';bpy.context.object.data.materials.append(mat('ReviewFloor',(.085,.105,.13),.85))
bpy.ops.object.camera_add(location=(7,-7,4.5));camera=bpy.context.object;camera.rotation_euler=(Vector((.25,-.35,1.25))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=6.8;scene.camera=camera
for loc,power,size in [((4,-4,7),1800,5),((-4,0,5),1600,4),((1,5,6),1100,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.size=size;light.rotation_euler=(Vector((0,0,1.3))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('PressBrakeReviewWorld');scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=48;scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
scene.render.filepath=str(OUT/'PressBrake_Overview.png');bpy.ops.render.render(write_still=True)
oldloc=camera.location.copy();oldrot=camera.rotation_euler.copy();oldscale=camera.data.ortho_scale
camera.location=(3.0,-2.4,1.85);camera.rotation_euler=(Vector((.52,-.5,1.18))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=2.0;scene.frame_set(100)
scene.render.filepath=str(OUT/'PressBrake_Tooling.png');bpy.ops.render.render(write_still=True)
camera.location=(3.1,-2.30,1.71);camera.rotation_euler=(Vector((1.79,-2.15,1.50))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=.89;scene.frame_set(1)
scene.render.filepath=str(OUT/'PressBrake_CNCDetail.png')
camera.location=(-5.5,5,3.8);camera.rotation_euler=(Vector((-.2,0,1.3))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=5.3;scene.frame_set(1)
scene.render.filepath=str(OUT/'PressBrake_RearDetail.png')
camera.location=oldloc;camera.rotation_euler=oldrot;camera.data.ortho_scale=oldscale;scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_PressBrake.blend'))
(OUT/'manifest.json').write_text(json.dumps({'tooling_length_cm':304.8,'ram_stroke_cm':31.5,'sample_thickness_cm':.3175,'units':'Metres in Blender, centimetres in Unreal','assets':entries,'status':'Authored model and Blender review animation; no production gameplay.'},indent=2))
print('PRESS_BRAKE_ASSETS_COMPLETE',len(entries))
