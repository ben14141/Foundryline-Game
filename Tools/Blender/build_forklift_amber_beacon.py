"""Original independent roof beacon. Licensed forklift appears only in review images."""
import bpy,math,json,bmesh
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Vehicles/Forklift/AmberBeacon';O.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);S=bpy.context.scene;S.unit_settings.system='IMPERIAL';S.unit_settings.scale_length=1
def mat(name,color,metal=0,rough=.3):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
base=mat('Beacon_GraphiteHousing',(.025,.033,.043),.6,.26);steel=mat('Beacon_StainlessHardware',(.42,.47,.51),.95,.24);rubber=mat('Beacon_RubberGasket',(.012,.016,.018),0,.7);amber=mat('Beacon_AmberLens',(1,.20,.008),0,.16);p=amber.node_tree.nodes.get('Principled BSDF');p.inputs['Transmission Weight'].default_value=.72;p.inputs['IOR'].default_value=1.46;p.inputs['Alpha'].default_value=.40
bulb=mat('Beacon_BulbEmitter',(1,.65,.18),0,.22);p=bulb.node_tree.nodes.get('Principled BSDF');p.inputs['Emission Color'].default_value=(1,.22,.025,1);p.inputs['Emission Strength'].default_value=3
ceramic=mat('Beacon_CeramicSocket',(.65,.66,.62),0,.46)
root=bpy.data.objects.new('ForkliftAmberBeacon_RoofDatum',None);S.collection.objects.link(root);parts=[]
def finish(o,name,m):
 o.name=name;o.data.materials.append(m);o.parent=root;parts.append(o)
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return o
def cyl(name,r,d,z,m,xy=(0,0),verts=48):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=d,location=(*xy,z));return finish(bpy.context.object,name,m)
def box(name,loc,size,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);finish(o,name,m);b=o.modifiers.new('Manufactured edge','BEVEL');b.width=.001;b.segments=2;return o
def ring(name,major,minor,z,m):
 bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=64,minor_segments=8,location=(0,0,z));return finish(bpy.context.object,name,m)
cyl('MountGasket',.10,.006,.003,rubber);cyl('CastBase',.097,.018,.015,base);ring('LensSeal',.079,.003,.026,rubber);cyl('LensRetainingCollar',.086,.012,.03,base)
for i in range(3):
 a=i*math.tau/3+.3;xy=(math.cos(a)*.087,math.sin(a)*.087);cyl('MountWasher_%d'%i,.008,.0015,.025,steel,xy);cyl('SocketCap_%d'%i,.005,.005,.028,steel,xy,6)
cyl('ReflectorFoot',.047,.005,.037,steel);cyl('CeramicLampSocket',.017,.031,.055,ceramic);cyl('BulbMetalCollar',.014,.01,.073,steel)
for z in [.079,.087,.095,.103,.111]:ring('LEDHeatSinkFin',.022,.002,z,steel)
cyl('FrostedLEDLightEngine',.025,.045,.098,bulb);cyl('LightEngineCap',.027,.004,.122,ceramic)
for i in range(8):
 a=i*math.tau/8;o=box('LEDPanel_%02d'%i,(math.cos(a)*.025,math.sin(a)*.025,.101),(.003,.011,.022),bulb);o.rotation_euler.z=a
# Hollow lens, with an open bottom and a real 2 mm wall.
profile=[(.080,.034),(.080,.135),(.077,.150),(.065,.166),(.045,.177),(.020,.183),(0,.184),(0,.182),(.020,.181),(.044,.175),(.063,.164),(.075,.149),(.078,.135),(.078,.034)]
verts=[];faces=[];N=64
for radius,z in profile:
 for j in range(N):a=j*math.tau/N;verts.append((radius*math.cos(a),radius*math.sin(a),z))
for i in range(len(profile)-1):
 for j in range(N):k=i*N+j;faces.append((k,i*N+(j+1)%N,(i+1)*N+(j+1)%N,k+N))
mesh=bpy.data.meshes.new('HollowAmberCover');mesh.from_pydata(verts,[],faces);mesh.update();lens=bpy.data.objects.new('HollowAmberCover_2mmWall',mesh);S.collection.objects.link(lens);finish(lens,lens.name,amber)
for z in [.055,.072,.089,.106,.123]:ring('OpticalLensRib',.0805,.0008,z,amber)
box('CableGland',(.091,0,.016),(.022,.016,.013),rubber)
curve=bpy.data.curves.new('ShortRoofLead','CURVE');curve.dimensions='3D';curve.bevel_depth=.003;curve.bevel_resolution=2;sp=curve.splines.new('BEZIER');sp.bezier_points.add(3)
for p,co in zip(sp.bezier_points,[(.10,0,.014),(.125,0,.012),(.145,.018,.008),(.155,.025,.003)]):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
o=bpy.data.objects.new('ShortRoofLead',curve);S.collection.objects.link(o);o.parent=root;o.data.materials.append(rubber);parts.append(o)
anchor=bpy.data.objects.new('Bulb_LightAnchor',None);S.collection.objects.link(anchor);anchor.parent=root;anchor.location=(0,0,.101)
for o in parts:o.select_set(False)
root.select_set(True)
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.export_scene.fbx(filepath=str(O/'ForkliftAmberBeacon.fbx'),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
bpy.ops.wm.save_as_mainfile(filepath=str(O/'ForkliftAmberBeacon.blend'))
manifest={'original_authorship':True,'asset':'ForkliftAmberBeacon','units':'metres source; Unreal centimetres','roof_attachment_cm':[-115,0,253],'bulb_anchor_local_cm':[0,0,10.1],'housing_diameter_cm':20,'height_cm':18.4,'lens_wall_mm':2,'clearance_bulb_to_inner_lens_cm':5.3,'light':{'color_linear':[1,.22,.025],'intensity_lumens':45,'attenuation_radius_cm':120,'cast_shadows':False,'stationary_flashing':False},'materials':{m.name:{'color':list(m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value[:3]),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value} for m in [base,steel,rubber,amber,bulb,ceramic]},'old_beacon_removal':'Existing joined base and lens centred(-115,0)cm, Z253..269cm; remove only in a separate licensed-derived body variant, not vendor original.'}
(O/'manifest.json').write_text(json.dumps(manifest,indent=2))
# Review renderer is limited to two CPU threads; no engine use or production edits.
S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=16;S.cycles.use_denoising=True;S.render.threads_mode='FIXED';S.render.threads=2;S.render.resolution_x=1000;S.render.resolution_y=900;S.render.resolution_percentage=100
world=bpy.data.worlds.new('ReviewWorld');S.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.17,.19,.23,1);world.node_tree.nodes['Background'].inputs[1].default_value=.5
def camera(loc,target,scale):
 bpy.ops.object.camera_add(location=loc);cam=bpy.context.object;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=scale;S.camera=cam
def area(loc,power,size,target):
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector(target)-l.location).to_track_quat('-Z','Y').to_euler()
area((-.1,-.4,.65),65,.6,(0,0,.1));area((.4,.2,.4),40,.4,(0,0,.1));camera((.35,-.42,.33),(0,0,.085),.47)
lens.hide_render=True
for o in parts:
 if o.name.startswith('OpticalLensRib'):o.hide_render=True
S.render.filepath=str(O/'Beacon_BulbCutaway.png');bpy.ops.render.render(write_still=True)
lens.hide_render=False
for o in parts:o.hide_render=False
before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/MaterialHandling/Vehicles/Forklift/FL01/Forklift_Body.fbx'));body=next(o for o in set(bpy.data.objects)-before if o.type=='MESH')
bm=bmesh.new();bm.from_mesh(body.data);remove=[v for v in bm.verts if (body.matrix_world@v.co).z>=2.529 and abs((body.matrix_world@v.co).x+1.15)<.105 and abs((body.matrix_world@v.co).y)<.105];assert len(remove)>50,len(remove);bmesh.ops.delete(bm,geom=remove,context='VERTS');bm.to_mesh(body.data);bm.free();body.name='REVIEW_LicensedForkBody_OldBeaconRemoved';root.location=(-1.15,0,2.53)
camera((-1.8,-.95,3.25),(-1.15,0,2.62),1.2);area((-1.8,-.8,3.8),400,2,(-1.15,0,2.6));area((-.3,.7,3.3),300,1.5,(-1.15,0,2.6));S.render.filepath=str(O/'Beacon_RoofCloseup.png');bpy.ops.render.render(write_still=True)
print('BEACON_SOURCE_READY original standalone; review-only old beacon vertices removed',len(remove))
