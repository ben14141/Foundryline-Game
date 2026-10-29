"""Original hinged welding hood; Handyman references are loaded only AFTER original save/export."""
import bpy,bmesh,math,json,sys
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Employees/Equipment/WeldingHood';O=O/'V2Draft' if '--draft' in sys.argv else O;O.mkdir(parents=True,exist_ok=True);P=R/'SourceAssets/Employees/Handyman/Imported'
bpy.ops.wm.read_factory_settings(use_empty=True);S=bpy.context.scene;S.unit_settings.system='IMPERIAL';S.unit_settings.scale_length=1
def mat(n,c,metal=0,rough=.4):
 m=bpy.data.materials.new(n);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
graphite=mat('Hood_GraphiteShell',(.025,.037,.049),.14,.34);orange=mat('Hood_OrangeTrim',(.72,.13,.017),.1,.3);rubber=mat('Hood_RubberPad',(.012,.017,.022),0,.7);steel=mat('Hood_PivotSteel',(.31,.37,.42),.9,.25);lensmat=mat('Hood_DarkFilterLens',(.004,.015,.011),.22,.18);covermat=mat('Hood_LensCover',(.016,.033,.035),.15,.15)
harness=bpy.data.objects.new('WeldingHood_Harness',None);S.collection.objects.link(harness);visor=bpy.data.objects.new('WeldingHood_Visor',None);S.collection.objects.link(visor);parts=[]
def finish(o,n,m,parent):
 o.name=n;o.data.materials.append(m);M=o.matrix_world.copy();o.parent=parent;o.matrix_world=M;parts.append(o);return o
def box(n,p,d,m,parent=visor,bevel=.002):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);finish(o,n,m,parent)
 if bevel:b=o.modifiers.new('Soft shell edge','BEVEL');b.width=bevel;b.segments=3;o.modifiers.new('Weighted planar normals','WEIGHTED_NORMAL')
 return o
def cyl(n,p,r,d,m,parent=harness):
 bpy.ops.mesh.primitive_cylinder_add(vertices=40,radius=r,depth=d,location=p,rotation=(0,math.pi/2,0));o=finish(bpy.context.object,n,m,parent)
 for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
 return o
def surface(n,v,f,m,parent=visor,thickness=.0025):
 mesh=bpy.data.meshes.new(n);mesh.from_pydata(v,[],f);mesh.update();o=bpy.data.objects.new(n,mesh);S.collection.objects.link(o);finish(o,n,m,parent)
 if thickness:s=o.modifiers.new('Real shell wall','SOLIDIFY');s.thickness=thickness;s.offset=0
 b=o.modifiers.new('Moulded edge radius','BEVEL');b.width=.0012;b.segments=2;return o
# V2: compound-curved moulded shell with tapered cheek/chin profiles.
profile=[(-.165,.103,.174,.018),(-.12,.126,.187,.009),(-.07,.143,.193,.005),(-.035,.149,.195,.005),(.035,.149,.195,.005),(.075,.144,.180,0),(.115,.128,.143,-.005),(.14,.095,.095,0)]
def params(z):
 for a,b in zip(profile,profile[1:]):
  if a[0]<=z<=b[0]:
   t=(z-a[0])/(b[0]-a[0]);return [a[i]*(1-t)+b[i]*t for i in range(1,4)]
 return profile[0][1:] if z<profile[0][0] else profile[-1][1:]
def curved(theta,z,offset=0):
 rx,ry,cy=params(z);a=math.radians(theta);return ((rx+offset)*math.sin(a),cy-(ry+offset)*math.cos(a),z)
angles=sorted(set([-105+210*i/56 for i in range(57)]+[-32,32]));zs=[-.165,-.15,-.135,-.12,-.10,-.08,-.06,-.045,-.035,.035,.05,.065,.075,.09,.105,.115,.13,.14];v=[curved(a,z) for z in zs for a in angles];f=[];N=len(angles)
for i in range(len(zs)-1):
 for j in range(N-1):
  if zs[i]==-.035 and zs[i+1]==.035 and angles[j]>=-32 and angles[j+1]<=32:continue
  f.append((i*N+j,i*N+j+1,(i+1)*N+j+1,(i+1)*N+j))
shell=surface('CompoundCurvedErgonomicShell',v,f,graphite)
for face in shell.data.polygons:face.use_smooth=True
def curved_panel(name,a0,a1,z0,z1,offset,material,thickness=.001,N=24):
 zrows=sorted(set([z0,z1]+[z for z in zs if z0<z<z1]));v=[curved(a0+(a1-a0)*i/N,z,offset) for z in zrows for i in range(N+1)];f=[(j*(N+1)+i,j*(N+1)+i+1,(j+1)*(N+1)+i+1,(j+1)*(N+1)+i) for j in range(len(zrows)-1) for i in range(N)];o=surface(name,v,f,material,thickness=thickness)
 for face in o.data.polygons:face.use_smooth=True
 return o
def rectloop(a,z,N=16):
 return [(-a+2*a*i/N,-z) for i in range(N)]+[(a,-z+2*z*i/N) for i in range(N)]+[(a-2*a*i/N,z) for i in range(N)]+[(-a,z-2*z*i/N) for i in range(N)]
def bezel(name,outer,inner,offset,material,thickness):
 rings=[rectloop(*outer),rectloop(*inner)];v=[curved(a,z,offset) for ring in rings for a,z in ring];N=len(rings[0]);f=[(i,(i+1)%N,N+(i+1)%N,N+i) for i in range(N)];o=surface(name,v,f,material,thickness=thickness)
 for face in o.data.polygons:face.use_smooth=True
 return o
bezel('RecessedFilterCartridge',(37,.050),(30.5,.0305),.005,graphite,.004)
bezel('FineOrangeLensAccent',(37.4,.051),(36.7,.049),.0072,orange,.0008)
bezel('ReplaceableFilterGasket',(32,.034),(30.2,.030),.0035,rubber,.0012)
curved_panel('AutoDarkFilter',-31,31,-.032,.032,.0015,lensmat,.001)
curved_panel('ClearOuterLensCover',-30.2,30.2,-.030,.030,.003,covermat,.0007)
curved_panel('SolarStrip',-17,17,.063,.077,.0035,lensmat,.001)
for i in range(1,8):
 a=-17+34*i/8;curved_panel('SolarCellDivider',a-.16,a+.16,.064,.076,.0043,steel,.0002,N=1)
for theta in [-25,25]:
 p=curved(theta,.068,.004);o=box('ArcSensorWindow',p,(.008,.003,.008),lensmat,bevel=.002);o.rotation_euler.z=math.radians(theta)
curved_panel('BrowOrangePinstripe',-43,43,.088,.092,.002,orange,.0008)
for side in [-1,1]:
 curved_panel('MouldedTempleInset',side*72,side*78,-.07,.055,.002,orange,.001,N=4)
 cyl('VisorPivotEar',(side*.151,0,0),.024,.008,graphite,visor)
 cyl('DetentedHingeKnob',(side*.163,0,0),.026,.018,graphite,harness);cyl('PivotScrew',(side*.174,0,0),.007,.005,steel,harness)
 for i in range(18):
  a=i*math.tau/18;o=box('HingeKnurl',(side*.164,.025*math.sin(a),.025*math.cos(a)),(.013,.0025,.004),rubber,harness,.0008);o.rotation_euler.x=-a
 cyl('ExternalShadeDial',(side*.151,-.025,.065),.013,.011,graphite,visor)
 for i in range(8):
  a=i*math.tau/8;o=box('ShadeDialKnurl',(side*.157,-.025+.012*math.sin(a),.065+.012*math.cos(a)),(.007,.002,.003),steel,visor,.0005);o.rotation_euler.x=-a
# A continuous rolled/rubber lip softens the open edges and protects the neck flare.
def rim(name,points):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=.0028;c.bevel_resolution=2;sp=c.splines.new('POLY');sp.points.add(len(points)-1)
 for p,co in zip(sp.points,points):p.co=(*co,1)
 o=bpy.data.objects.new(name,c);S.collection.objects.link(o);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return finish(bpy.context.object,name,rubber,visor)
rim('RoundedChinLip',[curved(a,-.165,.0004) for a in angles]);rim('CrownLip',[curved(a,.14,.0004) for a in angles])
for side in [-1,1]:rim('SoftSideRim',[curved(side*105,z,.0004) for z in zs])
# Original suspended oval headband and rear ratchet. Sized to bare heads, not caps.
N=64;v=[];f=[]
for z in [.016,.041]:
 for rx,ry in [(.139,.143),(.132,.136)]:
  for i in range(N):a=i*math.tau/N;v.append((rx*math.cos(a),-.010+ry*math.sin(a),z))
for i in range(N):
 j=(i+1)%N;f.extend([(i,j,N+j,N+i),(2*N+i,3*N+i,3*N+j,2*N+j),(i,2*N+i,2*N+j,j),(N+i,N+j,3*N+j,3*N+i)])
surface('AdjustableHeadband',v,f,graphite,harness,0)
box('ForeheadComfortPad',(0,-.155,.029),(.12,.008,.025),rubber,harness,.005)
box('RearRatchetHousing',(0,.137,.027),(.060,.027,.032),graphite,harness,.006)
bpy.ops.mesh.primitive_cylinder_add(vertices=32,radius=.021,depth=.012,location=(0,.158,.027),rotation=(math.pi/2,0,0));finish(bpy.context.object,'RearRatchetDial',rubber,harness)
for side in [-1,1]:box('HarnessPivotBridge',(side*.144,0,.012),(.030,.036,.025),graphite,harness,.003)
# Crown support strap, sliding buckle and rear-pad detail complete the suspension.
v=[];N=28
for x in [-.012,.012]:
 for i in range(N+1):
  a=i*math.pi/N;v.append((x,-.010+.144*math.cos(a),.035+.085*math.sin(a)))
f=[(i,i+1,N+2+i,N+1+i) for i in range(N)];strap=surface('AdjustableCrownStrap',v,f,graphite,harness,.003)
for face in strap.data.polygons:face.use_smooth=True
box('CrownStrapSlider',(0,-.010,.123),(.039,.025,.009),graphite,harness,.003)
box('RearComfortPad',(0,.123,.027),(.065,.008,.025),rubber,harness,.004)
for n in ['EarHinge','HeadAttachmentDatum']:
 e=bpy.data.objects.new('SOCKET_'+n,None);S.collection.objects.link(e);e.parent=harness
def export(n,objects):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=next(o for o in objects if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(O/(n+'.fbx')),use_selection=True,object_types={'MESH','EMPTY'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
for g in [harness,visor]:export(g.name,[g,*g.children_recursive])
export('WeldingHood_Complete',[harness,visor,*harness.children_recursive,*visor.children_recursive])
visor.rotation_euler.x=0;visor.keyframe_insert(data_path='rotation_euler',frame=1);visor.rotation_euler.x=math.radians(-105);visor.keyframe_insert(data_path='rotation_euler',frame=30);visor.rotation_euler.x=0;visor.keyframe_insert(data_path='rotation_euler',frame=60);visor.animation_data.action.name='WeldingHood_Lift_Closed_Open';S.frame_end=60;S.frame_set(1)
assert not bpy.data.libraries and not any(i.source=='FILE' for i in bpy.data.images)
bpy.ops.wm.save_as_mainfile(filepath=str(O/'WeldingHood_Original.blend'))
manifest={'version':2,'original_authorship':True,'source_blend_contains_no_vendor_reference':True,'units':'Blender metres / Unreal centimetres; imperial display','meshes':['WeldingHood_Harness.fbx','WeldingHood_Visor.fbx'],'native_paths':['/Game/Foundryline/Employees/Equipment/WeldingHood/SM_WeldingHood_Harness','/Game/Foundryline/Employees/Equipment/WeldingHood/SM_WeldingHood_Visor'],'neutral_root':'ear hinge, axes match native Handyman component: front+Y/up+Z','head_bone':'head','head_relative_location_cm':[14.558063928,-1.856296644,.001122955],'head_relative_quaternion_xyzw':[-.025120771857,-.706634613379,.025167143659,-.706684572406],'visor_relative_location_cm':[0,0,0],'hinge_axis_native':[1,0,0],'closed_degrees':0,'open_degrees':105,'blender_open_degrees':-105,'bare_head_rule':'Filter SK_Hat_* only for welding-hood outfits; preserve original head/beard/clothes and stable variant index. Caps cannot remain inside the merged mesh.','world_reference_ear_cm':[0,0,170],'materials':{m.name:{'base_color':list(m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value[:3]),'metallic':m.node_tree.nodes['Principled BSDF'].inputs['Metallic'].default_value,'roughness':m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value} for m in [graphite,orange,rubber,steel,lensmat,covermat]},'native_import_or_runtime_validation':False}
(O/'manifest.json').write_text(json.dumps(manifest,indent=2))
# Load actual heads only for private clearance checks and rendered fitting reference.
def bvh_objects(objects,extra=Matrix.Identity(4)):
 verts=[];faces=[];dg=bpy.context.evaluated_depsgraph_get()
 for o in objects:
  if o.type!='MESH':continue
  e=o.evaluated_get(dg);mesh=e.to_mesh();start=len(verts);verts.extend(extra@e.matrix_world@v.co for v in mesh.vertices);faces.extend([[start+j for j in f.vertices] for f in mesh.polygons]);e.to_mesh_clear()
 return BVHTree.FromPolygons(verts,faces),verts
headrows=[];refs=[]
for index,names in enumerate([['SK_Head_1'],['SK_Head_2','SK_Beard_1_2'],['SK_Head_3','SK_Beard_2_3']]):
 obs=[]
 for name in names:
  before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(P/(name+'.fbx')),use_anim=False);new=list(set(bpy.data.objects)-before);obs.extend(new)
 refs.append(obs);headtree,_=bvh_objects(obs);row={'variant':index,'head_parts':names,'caps_removed':True,'pose_checks':[]}
 for degree in range(0,106,5):
  S.frame_set(1);T=Matrix.Translation((0,0,1.70));hoodtree,points=bvh_objects(visor.children_recursive,T@Matrix.Rotation(math.radians(-degree),4,'X'));bandtree,bandpoints=bvh_objects(harness.children_recursive,T);overlap=len(headtree.overlap(hoodtree))+len(headtree.overlap(bandtree));clearance=min(headtree.find_nearest(p)[3] for p in points+bandpoints)*1000;row['pose_checks'].append({'open_degrees':degree,'triangle_intersections':overlap,'minimum_vertex_clearance_mm':clearance});
  if overlap:
   print('HOOD_CLIP_OBJECTS',index,degree,[(o.name,len(headtree.overlap(bvh_objects([o],T@(Matrix.Rotation(math.radians(-degree),4,'X') if o.parent==visor else Matrix.Identity(4)))[0]))) for o in [*visor.children_recursive,*harness.children_recursive] if o.type=='MESH'])
  assert overlap==0,(index,degree,overlap,clearance)
 headrows.append(row)
 for o in obs:o.hide_render=True
(O/'head_fit_qa.json').write_text(json.dumps({'variants':headrows,'source_heads_inspected':3,'hat_extents_checked':{'Hat1_front_cm':-23.39835,'Hat2_back_cm':18.98490},'no_vendor_geometry_in_original_source':True,'passed':True},indent=2));print('HOOD_ALL3_HEADS_POSE_CLEARANCE_PASS')
# Two modest CPU renders; vendor heads exist only in these fitting images.
S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=16;S.cycles.use_denoising=True;S.render.threads_mode='FIXED';S.render.threads=2;S.render.resolution_x=1200;S.render.resolution_y=900;S.render.resolution_percentage=100
world=bpy.data.worlds.new('HoodReview');S.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.15,.18,.22,1);world.node_tree.nodes['Background'].inputs[1].default_value=.55
def area(loc,power,size,target):
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
area((1,-2,3),170,2,(0,0,1.7));area((-1,1,2.5),140,2,(0,0,1.7))
def render(n,loc,target,scale):
 bpy.ops.object.camera_add(location=loc);o=bpy.context.object;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();o.data.type='ORTHO';o.data.ortho_scale=scale;S.camera=o;S.render.filepath=str(O/(n+'.png'));bpy.ops.render.render(write_still=True)
S.frame_set(1);harness.location=(0,0,1.7);visor.location=(0,0,1.7);
if '--fit-only' not in sys.argv:render('WeldingHood_Standalone',(1,-2,2.05),(0,-.04,1.71),.52)
# Assign vendor palette only to private reference meshes after original source is safely saved.
palette=bpy.data.materials.new('PRIVATE_HandymanPalette');palette.use_nodes=True;tex=palette.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(P/'T_Main_colors.png'));tex.interpolation='Closest';palette.node_tree.links.new(tex.outputs['Color'],palette.node_tree.nodes['Principled BSDF'].inputs['Base Color']);palette.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.65
for index,x in [(0,-.24),(2,.24)]:
 for o in refs[index]:
  o.hide_render=False
  if o.type=='MESH':o.data.materials.clear();o.data.materials.append(palette)
  if o.parent is None:o.location.x+=x
 if index==0:harness.location.x=x;visor.location.x=x
 else:
  copies={}
  for parent in [harness,visor]:
   root=parent.copy();root.animation_data_clear();S.collection.objects.link(root);root.location=(x,0,1.7);copies[parent]=root
   for child in parent.children_recursive:
    o=child.copy();o.data=child.data.copy() if child.data else None;S.collection.objects.link(o);o.parent=root;o.matrix_parent_inverse=child.matrix_parent_inverse.copy()
  copies[visor].rotation_euler.x=math.radians(-105)
render('WeldingHood_Worn_Closed_Open',(.6,-3,2.05),(0,-.015,1.72),.96)
print('ORIGINAL_WELDING_HOOD_READY source/FBX only; no engine use')
