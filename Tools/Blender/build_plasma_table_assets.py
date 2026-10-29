"""Detailed true-scale 14 x 10 ft plasma table; independent moving components."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
exec((Path(__file__).parent/'build_material_handling_assets.py').read_text().split('def import_factory')[0])
OUT=R/'SourceAssets/Production/Equipment/PlasmaTable';OUT.mkdir(parents=True,exist_ok=True)
groups={k:[] for k in ['Body','SlatBed','Gantry','TorchCarriage','Torch','CableChain','Umbilical','ServiceLead','OperatorConsole','PowerUnit','Extraction','StockSheet']}
def put(k,o):groups[k].append(o);return o
def bolt(k,loc,r=.012):return put(k,cyl('SocketBolt',loc,r,.012,steel,rot=(0,0,0),verts=6))
def rod(k,name,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,m,rot=(0,0,0));o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return put(k,o)
def hose(k,name,pts,r,m=rubber):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;s=c.splines.new('BEZIER');s.bezier_points.add(len(pts)-1)
 for p,co in zip(s.bezier_points,pts):p.co=co;p.handle_left_type=p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,c);scene.collection.objects.link(o);c.materials.append(m);bpy.context.view_layer.objects.active=o;bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.ops.object.convert(target='MESH');return put(k,bpy.context.object)
# Bed clear opening 165 x 317 cm; work envelope 152.4 x 304.8 cm.
for x in [-.97,.97]:
 put('Body',box('LongBoxSection',(x,0,.72),(.18,3.65,.24),graphite,bevel=.012))
 put('Body',box('OrangeSideSkirt',(x*1.064,0,.53),(.035,3.45,.32),orange,bevel=.009))
 put('Body',box('LinearRail',(x,0,1.08),(.07,3.62,.035),steel,bevel=.004))
 put('Body',box('RackDrive',(x+.045,0,1.025),(.022,3.54,.035),dark))
 for i in range(96):put('Body',box('RackTooth',(x+.045,-1.72+i*.036,1.047),(.026,.015,.012),steel))
 for y in [-1.5,0,1.5]:
  put('Body',box('TableLeg',(x,y,.37),(.14,.14,.63),graphite,bevel=.009));put('Body',box('LegGusset',(x,y,.60),(.24,.24,.09),dark,bevel=.006))
  put('Body',cyl('LevelingScrew',(x,y,.08),.022,.12,steel,rot=(0,0,0)))
  put('Body',cyl('LevelingFoot',(x,y,.025),.085,.035,rubber,rot=(0,0,0)))
 for y in [-1.7,-1.25,-.8,-.35,.1,.55,1.,1.45]:bolt('Body',(x,y,1.107),.009)
for y in [-1.73,1.73]:
 put('Body',box('EndFrame',(0,y,.72),(2.10,.17,.24),graphite,bevel=.012))
 put('Body',box('EndPanel',(0,y,.49),(1.81,.045,.26),orange,bevel=.006))
 for x in [-.74,.74]:bolt('Body',(x,y,.86))
put('Body',box('DowndraftPlenum',(0,0,.56),(1.80,3.30,.38),dark,bevel=.01))
for y in [-1.1,0,1.1]:put('Body',box('SlagDrawer',(0,y,.36),(1.72,.99,.10),graphite,bevel=.008))
for y in [-1.1,0,1.1]:
 put('Body',box('DrawerFront',(1.075,y,.39),(.035,.94,.18),dark,bevel=.006));put('Body',box('DrawerHandle',(1.115,y,.41),(.055,.24,.025),steel,bevel=.009))
# Removable crosswise slats with serrated supports; one joined mesh.
for i in range(53):
 y=-1.56+i*.06
 profile=[(-.825,.85),(.825,.85),(.825,.98)]
 for j in range(33,-1,-1):profile.append((-.825+j*.05,.98 if j%2 else 1.005))
 n=len(profile);vs=[(x,y+dy,z) for dy in [-.0015,.0015] for x,z in profile]
 faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(j,(j+1)%n,(j+1)%n+n,j+n) for j in range(n)]
 mesh=bpy.data.meshes.new('SacrificialSlat');mesh.from_pydata(vs,[],faces);mesh.materials.append(dark);o=bpy.data.objects.new('SerratedSlat',mesh);scene.collection.objects.link(o);put('SlatBed',o)
# Bridge at y=.55; moves longitudinally, carriage traverses x.
for x in [-.97,.97]:
 put('Gantry',box('BridgeSaddle',(x,.55,1.15),(.23,.38,.13),orange,bevel=.015))
 put('Gantry',box('BridgeUpright',(x,.55,1.36),(.16,.20,.38),graphite,bevel=.012))
 for yy in [.44,.66]:bolt('Gantry',(x,yy,1.225),.014)
 put('Gantry',cyl('YDriveMotor',(x,.55,1.45),.075,.18,dark,rot=(0,0,0)))
 put('Gantry',box('MotorCap',(x,.55,1.55),(.135,.135,.025),steel,bevel=.007))
put('Gantry',box('GantryBeam',(0,.55,1.49),(2.20,.22,.21),orange,bevel=.012))
put('Gantry',box('XLinearGuide',(0,.42,1.50),(1.98,.028,.045),steel,bevel=.003))
put('Gantry',box('GantryBrandPlate',(0,.427,1.58),(.64,.016,.08),graphite,bevel=.004))
put('Gantry',label('FoundrylineBrand','FOUNDRYLINE',(-.275,.416,1.56),(math.pi/2,0,0),.062,white))
for x in [-.86,-.60,-.34,-.08,.18,.44,.70,.86]:bolt('Gantry',(x,.55,1.606),.008)
put('TorchCarriage',box('XCarriage',(-.30,.37,1.48),(.22,.16,.31),graphite,bevel=.009))
for x in [-.365,-.235]:put('TorchCarriage',cyl('ZGuideRod',(x,.265,1.40),.014,.35,steel,rot=(0,0,0)))
put('TorchCarriage',cyl('ZLeadScrew',(-.30,.29,1.43),.012,.32,dark,rot=(0,0,0)))
put('TorchCarriage',cyl('ZStepper',(-.30,.32,1.705),.053,.11,graphite,rot=(0,0,0)))
put('TorchCarriage',box('TorchMount',(-.30,.205,1.28),(.17,.12,.13),orange,bevel=.008))
for z in [1.21,1.31]:put('Torch',cyl('TorchClamp',(-.30,.175,z),.050,.025,steel,rot=(0,0,0)))
put('Torch',cyl('TorchBody',(-.30,.175,1.21),.033,.26,graphite,rot=(0,0,0)))
put('Torch',cyl('CeramicShield',(-.30,.175,1.09),.039,.045,white,rot=(0,0,0)))
put('Torch',cyl('Nozzle',(-.30,.175,1.06),.023,.024,steel,rot=(0,0,0)))
put('Torch',cyl('NozzleAperture',(-.30,.175,1.0495),.008,.003,dark,rot=(0,0,0)))
for i in range(6):put('Torch',cyl('HoseStrainRelief',(-.30,.175,1.36+i*.014),.036-i*.002,.009,rubber,rot=(0,0,0)))
hose('Umbilical','TorchUmbilical',[(-.30,.175,1.42),(-.30,.20,1.86),(-.65,.5,1.92),(-1.03,.65,1.72)],.019)
# Segmented energy chain across bridge; static display, can be replaced by deforming chain later.
for i in range(32):
 x=-.98+i*.053
 put('CableChain',box('EnergyChainLink',(x,.70,1.67),(.047,.075,.055),rubber,bevel=.005))
 for side in [-1,1]:put('CableChain',box('ChainSidePlate',(x,.70+side*.043,1.67),(.040,.014,.066),dark,bevel=.003))
hose('ServiceLead','CableFeed',[(-1.02,.7,1.68),(-1.18,.80,1.55),(-1.18,1.50,.25),(-1.32,2.0,.12)],.023)
# Power supply, vents, fittings, cables and extraction filter/blower.
put('PowerUnit',box('PlasmaPowerUnit',(-1.42,1.0,.39),(.53,.80,.66),orange,bevel=.025))
put('PowerUnit',box('PowerFront',(-1.42,.59,.39),(.46,.025,.53),graphite,bevel=.01))
for z in [.23,.27,.31,.35,.39]:put('PowerUnit',box('PowerVent',(-1.42,.571,z),(.33,.008,.012),dark))
put('PowerUnit',box('SupplyDisplay',(-1.49,.572,.54),(.18,.009,.07),glass,bevel=.004))
put('PowerUnit',label('SupplyAmps','65 A',(-1.565,.564,.52),(math.pi/2,0,0),.043,white))
put('PowerUnit',cyl('CurrentDial',(-1.28,.56,.54),.032,.028,dark))
for x in [-1.59,-1.25]:put('PowerUnit',box('CarryHandle',(x,1.0,.75),(.035,.32,.045),dark,bevel=.008))
hose('PowerUnit','GroundLead',[(-1.32,.58,.15),(-1.25,.3,.05),(-.95,-.3,.05),(-.90,-.5,.65)],.012)
put('PowerUnit',box('GroundClamp',(-.90,-.50,.74),(.07,.13,.045),steel,rot=(0,.2,0),bevel=.004))
put('Extraction',box('FilterCabinet',(-1.50,-1.10,.75),(.68,.82,1.40),graphite,bevel=.025))
put('Extraction',box('FilterDoor',(-1.50,-1.525,.79),(.57,.035,1.11),orange,bevel=.01))
put('Extraction',box('FilterLatch',(-1.27,-1.55,.85),(.04,.04,.17),steel,bevel=.005))
put('Extraction',cyl('BlowerHousing',(-1.50,-1.10,1.56),.28,.27,dark,rot=(0,0,0)))
for i in range(9):put('Extraction',box('ExhaustGrille',(-1.50,-1.30+i*.05,1.705),(.42,.016,.016),steel))
hose('Extraction','DowndraftDuct',[(-.80,-1.1,.55),(-1.0,-1.10,.55),(-1.18,-1.10,.55)],.145,dark)
for i in range(9):rod('Extraction','DuctRib',(-.84-i*.04,-1.1,.55),(-.85-i*.04,-1.1,.55),.153,graphite)
# Standalone console, facing +X for access from front aisle.
for name,loc,size,m in [('ConsoleFoot',(1.58,-1.36,.045),(.60,.65,.09),dark),('ConsolePost',(1.58,-1.36,.59),(.13,.13,1.06),graphite),('ConsoleDesk',(1.67,-1.36,1.11),(.52,.72,.075),orange),('Monitor',(1.57,-1.36,1.48),(.075,.65,.41),graphite)]:put('OperatorConsole',box(name,loc,size,m,bevel=.012))
displaymat=mat('PT_CNCDisplay',(.015,.025,.035),.48);bs=next(n for n in displaymat.node_tree.nodes if n.type=='BSDF_PRINCIPLED');tex=displaymat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(OUT/'T_PlasmaTable_CNC.png'));tex.image.pack()
for socket in ['Base Color','Emission Color']:displaymat.node_tree.links.new(tex.outputs['Color'],bs.inputs[socket])
bs.inputs['Emission Strength'].default_value=.8
mesh=bpy.data.meshes.new('CNCDisplay');mesh.from_pydata([(1.611,-1.6525,1.315),(1.611,-1.0675,1.315),(1.611,-1.0675,1.645),(1.611,-1.6525,1.645)],[],[(0,1,2,3)]);mesh.materials.append(displaymat);screen=bpy.data.objects.new('CNCDisplay',mesh);scene.collection.objects.link(screen);put('OperatorConsole',screen)
put('OperatorConsole',box('Keyboard',(1.76,-1.40,1.16),(.16,.42,.028),dark,bevel=.006))
for row in range(4):
 for col in range(12):put('OperatorConsole',box('Key',(1.704+row*.033,-1.585+col*.032,1.179),(.023,.023,.007),steel,bevel=.001))
put('OperatorConsole',cyl('EmergencyStop',(1.79,-1.065,1.18),.032,.035,red,rot=(0,0,0)))
hose('OperatorConsole','ComputerCable',[(1.53,-1.36,1.40),(1.46,-1.38,.8),(1.30,-1.55,.04),(1.00,-1.63,.12)],.012)
# Real 60 x 120 in stock sheet, partially covering the cutting bed.
put('StockSheet',box('SteelSheet',(0,0,1.0065875),(1.524,3.048,.003175),sheetmat,bevel=.0003))
exec((Path(__file__).parent/'equipment_detail_parts.py').read_text());add_details('PlasmaTable',globals())
objects={}
for name,items in groups.items():
 o=join('PlasmaTable_'+name,items);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);uv_planar(o);objects[name]=o
 if name=='Torch':
  # Nozzle face is 1.825 mm above the 3.175 mm stock in its cutting pose.
  for v in o.data.vertices:v.co.z-=.038
 def stretch(value,half,extra):return value+max(-1,min(1,value/half))*extra
 if name in ['Body','SlatBed','Gantry','CableChain','ServiceLead']:
  for v in o.data.vertices:
   v.co.x=stretch(v.co.x,.825,.762)
   v.co.y=stretch(v.co.y,1.585,.6096) if name in ['Body','SlatBed'] else v.co.y+.55/1.585*.6096
 elif name in ['TorchCarriage','Torch','Umbilical']:
  for v in o.data.vertices:v.co.x-=.30/.825*.762;v.co.y+=.55/1.585*.6096
 elif name in ['PowerUnit','Extraction','OperatorConsole']:
  shift=Vector((.762 if name=='OperatorConsole' else -.762,-.6096 if name!='PowerUnit' else .6096,0))
  for v in o.data.vertices:v.co+=shift
 if name=='OperatorConsole':
  for p in o.data.polygons:
   if o.data.materials[p.material_index]==displaymat:
    for loop in p.loop_indices:
     v=o.matrix_world@o.data.vertices[o.data.loops[loop].vertex_index].co;o.data.uv_layers.active.data[loop].uv=((v.y+2.2621)/.585,(v.z-1.315)/.33)
copies=[]
for name,o in objects.items():
 if name=='StockSheet':continue
 c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);copies.append(c)
complete=join('PlasmaTable_Complete',copies);complete.hide_render=True
entries=[]
bpy.context.view_layer.update()
for o in list(objects.values())+[complete]:
 entries.append({'name':o.name,'position_cm':list(o.location*100),'dimensions_cm':list(o.dimensions*100),'domain':'Products/Components/Steel' if o==objects['StockSheet'] else 'Production/Equipment/PlasmaTable'})
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
# Review animation: gantry Y and carriage X travel; torch retracts vertically.
for name in ['TorchCarriage','CableChain']:objects[name].parent=objects['Gantry'];objects[name].matrix_parent_inverse=Matrix.Identity(4)
objects['Torch'].parent=objects['TorchCarriage'];objects['Torch'].matrix_parent_inverse=Matrix.Identity(4)
for frame,y,x,z in [(1,0,0,0),(60,.65,.70,.04),(120,-1.1,.70,0),(180,-1.1,-.30,.04),(240,0,0,0)]:
 objects['Gantry'].location.y=y;objects['Gantry'].keyframe_insert('location',frame=frame)
 objects['TorchCarriage'].location.x=x;objects['TorchCarriage'].keyframe_insert('location',frame=frame)
 objects['Torch'].location.z=z;objects['Torch'].keyframe_insert('location',frame=frame)
scene.frame_set(1)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));bpy.context.object.data.materials.append(mat('ReviewFloor',(.085,.105,.13),.85))
bpy.ops.object.camera_add(location=(6,-6,5.2));camera=bpy.context.object;camera.data.type='ORTHO';scene.camera=camera
def view(loc,target,scale,path):
 camera.location=loc;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale;scene.render.filepath=str(OUT/path);bpy.ops.render.render(write_still=True)
for loc,power,size in [((3,-4,7),1700,5),((-4,0,5),1400,4),((1,5,6),1200,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.size=size;light.rotation_euler=(Vector((0,0,.8))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('PlasmaReviewWorld');scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=40;scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
view((7,-7,6.4),(0,-.1,.8),8.0,'PlasmaTable_Overview.png')
view((2.5,-1.4,2.5),(-.57,.46,1.24),2.1,'PlasmaTable_TorchDetail.png')
view((3.662,-2.1896,1.75),(2.372,-1.9696,1.48),.90,'PlasmaTable_CNCDetail.png')
objects['StockSheet'].hide_render=True
view((5,-5,6),(0,0,.85),6.7,'PlasmaTable_SlatBed.png');objects['StockSheet'].hide_render=False
camera.location=(7,-7,6.4);camera.rotation_euler=(Vector((0,-.1,.8))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=8.0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_PlasmaTable.blend'))
(OUT/'manifest.json').write_text(json.dumps({'work_envelope_cm':[304.8,426.72],'standard_sheet_cm':[152.4,304.8],'stock_thickness_cm':.3175,'torch_tip_cm':[-(.30+.30/.825*.762)*100,(.175+.55/1.585*.6096)*100,100.95],'units':'Metres in Blender; centimetres in Unreal','assets':entries,'status':'Independent components for the runtime cutting demonstration'},indent=2))
print('PLASMA_TABLE_ASSETS_COMPLETE',len(entries))
