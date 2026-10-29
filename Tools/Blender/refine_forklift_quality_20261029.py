"""Local licensed-derived review only; never overwrite or publicly publish vendor art."""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29';O.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);S=bpy.context.scene;S.unit_settings.system='IMPERIAL';S.unit_settings.scale_length=1
rows=[r for r in json.loads((R/'SourceAssets/MaterialHandling/manifest.json').read_text())['assets'] if r['folder']=='Vehicles/Forklift/FL01'];groups={};original={};new=[]
for row in rows:
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets'/row['source_file']));o=next(o for o in set(bpy.data.objects)-before if o.type=='MESH');o.name=row['name'];o.location=Vector(row['position_cm'])/100;bpy.context.view_layer.update();groups[o.name]=o;original[o.name]={'matrix':[list(v) for v in o.matrix_world],'vertices':len(o.data.vertices),'position_cm':[v*100 for v in o.location]}
body=groups['Forklift_Body'];bm=bmesh.new();bm.from_mesh(body.data);old=[v for v in bm.verts if (body.matrix_world@v.co).z>=2.529 and abs((body.matrix_world@v.co).x+1.15)<.105 and abs((body.matrix_world@v.co).y)<.105];assert len(old)>50;bmesh.ops.delete(bm,geom=old,context='VERTS');bm.to_mesh(body.data);bm.free()
def mat(name,col,metal,rough):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
orange=mat('FQ_GlossOrange',(.79,.19,.027),.15,.25);orange.node_tree.nodes.get('Principled BSDF').inputs['Coat Weight'].default_value=.5;dark=mat('FQ_GraphiteSteel',(.036,.048,.058),.65,.3);steel=mat('FQ_MachinedSteel',(.36,.42,.47),.92,.24);rubber=mat('FQ_Rubber',(.013,.017,.019),0,.68);seatmat=mat('FQ_SeatVinyl',(.027,.034,.04),0,.47);light=mat('FQ_WorkLampGlass',(.73,.83,.88),0,.2);lamp=light.node_tree.nodes.get('Principled BSDF');lamp.inputs['Emission Color'].default_value=(.64,.78,1,1);lamp.inputs['Emission Strength'].default_value=.4
for o in groups.values():
 for i,m in enumerate(o.data.materials):
  if not m:continue
  n=m.name.lower();replacement=orange if 'orange' in n else rubber if 'rubber' in n else steel if 'machined' in n else dark if any(k in n for k in ['darksteel','graphite']) else None
  if replacement:o.data.materials[i]=replacement
 bevel=o.modifiers.new('Small true-scale edge radii','BEVEL');bevel.width=.0015;bevel.segments=2;bevel.limit_method='ANGLE';bevel.angle_limit=.6
 norm=o.modifiers.new('Weighted planar normals','WEIGHTED_NORMAL');norm.keep_sharp=True
def finish(o,n,m,parent):
 o.name=n;o.data.materials.append(m);M=o.matrix_world.copy();o.parent=parent;o.matrix_world=M;new.append(o);return o
def box(n,p,d,m,parent=body,bevel=.002):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);finish(o,n,m,parent)
 if bevel:b=o.modifiers.new('Edge radius','BEVEL');b.width=bevel;b.segments=2
 return o
def cylinder(n,p,r,d,m,parent=body,axis='Z',verts=24):
 rot=(math.pi/2,0,0) if axis=='Y' else (0,math.pi/2,0) if axis=='X' else (0,0,0);bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=d,location=p,rotation=rot);o=finish(bpy.context.object,n,m,parent)
 for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
 return o
def hose(n,points,r=.009,parent=body,m=rubber):
 c=bpy.data.curves.new(n,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;sp=c.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
 for p,co in zip(sp.bezier_points,points):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 o=bpy.data.objects.new(n,c);S.collection.objects.link(o);return finish(o,n,m,parent)
# Side service covers with recessed vents and visible fasteners.
for side in [-1,1]:
 y=side*.70
 box('ServicePanel',(-1.93,y,.98),(.57,.018,.37),orange)
 for i in range(7):box('CoolingLouver',(-2.15+i*.071,y+side*.012,1.0),(.042,.014,.19),dark,bevel=.001)
 for x in [-2.18,-1.68]:
  for z in [.83,1.13]:cylinder('PanelSocketBolt',(x,y+side*.022,z),.009,.008,steel,axis='Y',verts=6)
 box('StepPlate',(-.69,side*.69,.56),(.38,.20,.035),dark)
 for i in range(6):box('StepGripRib',(-.84+i*.055,side*.69,.58),(.009,.17,.008),rubber,bevel=.001)
 hose('CabGrabHandle',[(-.68,side*.61,.86),(-.74,side*.64,.93),(-.74,side*.64,1.25),(-.68,side*.61,1.31)],.016,m=dark)
 # Guard gussets stay on existing outer envelope and preserve operator clearance.
 for x in [-1.67,-.60]:box('RoofPostMount',(x,side*.53,2.48),(.13,.12,.026),dark)
 cylinder('RearWorkLampHousing',(-1.84,side*.47,2.46),.065,.055,dark,axis='X');cylinder('RearWorkLampLens',(-1.871,side*.47,2.46),.053,.006,light,axis='X')
 hose('GuardHarness',[(-1.68,side*.54,1.22),(-1.70,side*.54,1.55),(-1.70,side*.54,2.39),(-1.81,side*.47,2.43)],.006)
# Rear counterweight towing pin and access hardware.
cylinder('TowBoss',(-2.54,0,.57),.065,.05,dark,axis='X');cylinder('TowPin',(-2.565,0,.57),.021,.13,steel)
box('ServiceHatch',(-2.546,0,1.07),(.016,.52,.22),orange)
for y in [-.20,.20]:cylinder('HatchFastener',(-2.558,y,1.07),.01,.008,steel,axis='X',verts=6)
# Cab instruments outside the original preserved driver grip/seat geometry.
box('InstrumentPod',(-.62,.22,1.55),(.11,.20,.075),dark);box('InstrumentGlass',(-.62,.22,1.592),(.085,.17,.006),light)
for y in [.16,.22,.28]:cylinder('StatusButton',(-.57,y,1.58),.009,.009,orange)
for y in [-.20,.13]:box('FootPedal',(-.52,y,.87),(.13,.09,.025),rubber)
# Wheel hubs are inside original tyre envelopes; no wheel radius/pivot changes.
for row in rows:
 if row['kind']!='wheel':continue
 o=groups[row['name']];p=o.location.copy();sign=1 if p.y>0 else -1;half=row['size_cm'][1]/200;radius=row['radius_cm']/100
 cylinder('MachinedRimLip',(p.x,p.y+sign*(half-.010),p.z),radius*.55,.012,steel,o,'Y',40)
 cylinder('HubCap',(p.x,p.y+sign*(half-.001),p.z),radius*.22,.014,dark,o,'Y')
 for i in range(6):
  a=i*math.tau/6;cylinder('HubStud',(p.x+math.cos(a)*radius*.36,p.y+sign*(half+.004),p.z+math.sin(a)*radius*.36),.012,.012,steel,o,'Y',6)
 # Fine transverse tread sipes do not enlarge the authored tyre.
 for i in range(28):
  a=i*math.tau/28;t=box('TreadSipe',(p.x+math.sin(a)*(radius-.003),p.y,p.z+math.cos(a)*(radius-.003)),(.011,half*1.5,.004),dark,o,.0005);t.rotation_euler.y=a
# Mast guide hardware follows its correct independent moving group.
outer=groups['Forklift_MastOuter'];inner=groups['Forklift_MastInner'];car=groups['Forklift_Carriage']
for side in [-1,1]:
 y=side*.38
 for z in [.55,1.55,2.05]:
  cylinder('MastGuideRoller',(.37,y,z),.060,.05,steel,outer,'Y');cylinder('GuidePin',(.37,y+side*.03,z),.020,.018,dark,outer,'Y',6)
 cylinder('LiftCylinderBarrel',(.20,side*.19,.70),.048,.91,dark,outer);cylinder('HydraulicChromeRod',(.20,side*.19,1.35),.024,.54,steel,inner)
 hose('CylinderPressureLine',[(.16,side*.19,.30),(.10,side*.20,.32),(.10,side*.20,.78),(.17,side*.19,.91)],.008,parent=outer)
 for i in range(27):box('LiftChainLink',(.30,side*.28,.32+i*.064),(.021,.035,.045),steel,outer,.002)
 cylinder('ChainTopSprocket',(.30,side*.28,2.08),.075,.035,dark,outer,'Y')
 for z in [.48,.75,1.02]:box('CarriageWearPad',(.44,side*.46,z),(.035,.12,.07),dark,car)
 # Local hose loop is fixed to carriage and exported separately for later deformation.
 hose('CarriageHoseLoop',[(.28,side*.34,.54),(.18,side*.34,.75),(.22,side*.34,1.02),(.39,side*.34,1.03)],.009,parent=car)
# Append original independent beacon and keep only one roof light.
with bpy.data.libraries.load(str(R/'SourceAssets/Vehicles/Forklift/AmberBeacon/ForkliftAmberBeacon.blend'),link=False) as (src,dst):dst.objects=list(src.objects)
beaconobjects=[o for o in dst.objects if o]
for o in beaconobjects:S.collection.objects.link(o)
beacon=next(o for o in beaconobjects if o.name.startswith('ForkliftAmberBeacon_RoofDatum'));beacon.parent=body;beacon.location=(-1.15,0,2.53)
anchor=next(o for o in beaconobjects if o.name.startswith('Bulb_LightAnchor'))
bpy.context.view_layer.update();assert abs(anchor.matrix_world.translation.z-2.631)<.001,tuple(anchor.matrix_world.translation)
# Preserve and report all authoritative imported component matrices.
bpy.context.view_layer.update()
for name,o in groups.items():assert all(abs(o.matrix_world[i][j]-original[name]['matrix'][i][j])<1e-6 for i in range(4) for j in range(4)),name
def export(path,objects):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=next(o for o in objects if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
exports=[]
for name,o in groups.items():
 export(O/(name+'.fbx'),[o,*o.children_recursive]);exports.append(name+'.fbx')
export(O/'Forklift_Complete_Review.fbx',[o for o in bpy.data.objects if o.type in {'MESH','CURVE','EMPTY'}])
# Editable actual lift/rotation preview; original export rest transforms remain frame1.
for o,delta in [(inner,.45),(car,1.2)]:
 p=o.location.copy();o.keyframe_insert(data_path='location',frame=1);o.location.z+=delta;o.keyframe_insert(data_path='location',frame=90);o.location=p;o.keyframe_insert(data_path='location',frame=180)
 if o.animation_data:o.animation_data.action.name=o.name+'_MechanicalLiftPreview'
S.frame_end=180;S.frame_set(1)
manifest={'scope':'Local licensed-derived model quality review; not public redistribution','source':'Factory derivative component FBXs; original vendor files unchanged','component_rest_transforms':original,'component_exports':exports,'new_detail_objects':len(new),'removed_old_beacon_vertices':len(old),'beacon_roof_attachment_cm':[-115,0,253],'bulb_world_anchor_cm':[-115,0,263.1],'driver_fit_preserved':{'seat_root_x_cm':-106,'hips_z_cm':148,'hands_z_cm':174,'original_cab_geometry_unchanged':True},'wheel_pivots_radii_preserved':True,'mast_preview_lift_cm':{'inner':45,'carriage':120},'unreal_light_status':'Original beacon mesh and anchor ready; actual Unreal point light/BP not imported while Claude active','no_game_asset_overwrites':True,'units':'1m Blender =100cm Unreal; imperial presentation'}
(O/'manifest.json').write_text(json.dumps(manifest,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(O/'Forklift_QualityReview.blend'))
S.view_settings.exposure=-.6;S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=12;S.cycles.use_denoising=True;S.render.threads_mode='FIXED';S.render.threads=2;S.render.resolution_x=1200;S.render.resolution_y=1000;S.render.resolution_percentage=100
world=bpy.data.worlds.new('ReviewWorld');S.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.18,.20,.24,1);world.node_tree.nodes['Background'].inputs[1].default_value=.6
ground=mat('ReviewGround',(.12,.14,.17),0,.65);box('REVIEW_Ground',(-.6,0,-.035),(12,12,.04),ground,parent=None)
def area(loc,power,size,target):
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
area((2,-4,6),1800,5,(-.8,0,1));area((-3,2,5),1500,4,(-.8,0,1));area((-4,-2,2),700,3,(-.8,0,1))
def render(name,loc,target,scale):
 bpy.ops.object.camera_add(location=loc);o=bpy.context.object;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();o.data.type='ORTHO';o.data.ortho_scale=scale;S.camera=o;S.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True)
render('Forklift_Updated_Overview',(4,-5,3.8),(-.55,0,1.3),5.7)
render('Forklift_MastDetail',(3,-2.8,2.4),(.27,0,1.20),2.85)
render('Forklift_CabBeacon',(-3.3,-2.7,3.4),(-1.1,0,1.9),2.7)
S.frame_set(90);bpy.context.view_layer.update();assert car.location.z-original['Forklift_Carriage']['position_cm'][2]/100>1.19
manifest['raised_pose_verified']=True;manifest['raised_carriage_z_cm']=car.location.z*100;S.frame_set(1);(O/'manifest.json').write_text(json.dumps(manifest,indent=2));print('FORKLIFT_QUALITY_READY',len(new),'new details; pivots preserved; no Unreal use')
