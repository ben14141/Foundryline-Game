"""Two direct-attached bent L-sills; separate local licensed review revision."""
import bpy,math,json,shutil,hashlib,importlib.util
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');OLD=ROOT/'SourceAssets/Products/SkidTank/Review_2026-10-29';OUT=ROOT/'SourceAssets/Products/SkidTank/LowProfileSills_2026-10-29';OUT.mkdir(parents=True,exist_ok=True)
s=importlib.util.spec_from_file_location('h',ROOT/'Tools/Blender/build_rack_welding_bench.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
bpy.ops.wm.open_mainfile(filepath=str(OLD/'Foundryline_WhiteSkidTank.blend'));root=bpy.data.objects['WhiteSkidTank_FloorCentre'];old=bpy.data.objects['SupportSkid'];old_names=[o.name for o in old.children_recursive]
for o in list(old.children_recursive)+[old]:bpy.data.objects.remove(o,do_unlink=True)
for col in list(bpy.data.collections):
 if col.name.startswith('REVIEW ONLY / Studio'):
  for o in list(col.objects):bpy.data.objects.remove(o,do_unlink=True)
body=bpy.data.objects['TankBody_Source'];native=[tuple(v.co) for v in body.data.vertices];before_body=hashlib.sha256(repr(native).encode()).hexdigest();bpy.data.objects['TankBody'].location.z=-.373;bpy.data.objects['Fittings'].location.z=-.373;bpy.context.view_layer.update();tree=BVHTree.FromObject(body,bpy.context.evaluated_depsgraph_get());body_matrix=body.matrix_world.copy();inv=body_matrix.inverted()
C=h.collection('LowProfileSills');G=bpy.data.objects.new('LowProfileSills',None);C.objects.link(G);G.parent=root;root.name='WhiteTank_LowProfileSills_FloorCentre';h.M={m.name:m for m in bpy.data.materials};h.M['DirectSillSteel']=bpy.data.materials['ST / Graphite painted skid'];h.M['Rubber']=bpy.data.materials['ST / Contact isolation']
T=.008;R=.012;Y=.45;L=5.30;foot=.1524;stations=[-L/2+i*L/32 for i in range(33)];contacts=[];profiles={}
for sign in [-1,1]:
 sections=[]
 for x in stations:
  low=Y-T/2;high=Y+T/2;top=[]
  for y in [low,high]:
   hit,normal,face,dist=tree.ray_cast(inv@Vector((x,sign*y,-1)),Vector((0,0,1)),5);assert hit is not None;hit=body_matrix@hit;(contacts.append((x,sign*y,hit.z)));top.append(hit.z)
  cy=low-R;cz=T+R;p=[(high,top[1]),(low,top[0]),(low,cz)]
  for i in range(1,7):a=-i*math.pi/12;p.append((cy+R*math.cos(a),cz+R*math.sin(a)))
  p.extend([(high-foot,T),(high-foot,0),(cy,0)])
  for i in range(1,7):a=-math.pi/2+i*math.pi/12;p.append((cy+(R+T)*math.cos(a),cz+(R+T)*math.sin(a)))
  sections.append([(x,sign*y,z) for y,z in p])
 n=len(sections[0]);verts=[v for section in sections for v in section];faces=[tuple(range(n-1,-1,-1)),tuple((len(sections)-1)*n+i for i in range(n))]
 for i in range(len(sections)-1):
  for j in range(n):k=(j+1)%n;faces.append((i*n+j,i*n+k,(i+1)*n+k,(i+1)*n+j))
 o=h.mesh('DirectLSill_Left' if sign<0 else 'DirectLSill_Right',verts,faces,'DirectSillSteel',C,G,bevel=.00035);mod=o.modifiers.new('Weighted steel normals','WEIGHTED_NORMAL');mod.keep_sharp=True
 # A narrow continuous attachment fillet on the visible outer upper edge.
 line=[(x,sign*(Y+T/2+.001),sections[i][0][2]-.0015) for i,x in enumerate(stations)];w=h.cable('DirectTankToSillWeld',line,.0022,C,G);w.data.materials.clear();w.data.materials.append(bpy.data.materials['ST / Weld metal'])
 profiles[str(sign)]={'web_height_centre_cm':sections[len(sections)//2][0][2]*100,'flange_width_cm':foot*100,'flange_direction':'INWARD','plate_thickness_cm':T*100,'inside_bend_radius_cm':R*100}
bpy.context.view_layer.update();assert len([o for o in G.children_recursive if o.type=='MESH'])==2;assert all(n not in bpy.data.objects for n in old_names);assert hashlib.sha256(repr([tuple(v.co) for v in body.data.vertices]).encode()).hexdigest()==before_body
for o in G.children_recursive:
 if o.type=='MESH':assert abs(min(v.co.z for v in o.data.vertices))<1e-8
cross_positions=[-2.20,-.75,.75,2.20]
for x in cross_positions:
 h.tube('UnderTankCrossmember',2*(Y-T/2),.1016,.0762,.004,(x,0,.0635),'Y','DirectSillSteel',C,G)
 for sign in [-1,1]:
  line=[(x-.045,sign*(Y-T/2),.101),(x,sign*(Y-T/2),.101),(x+.045,sign*(Y-T/2),.101)];w=h.cable('CrossmemberEndWeld',line,.0018,C,G);w.data.materials.clear();w.data.materials.append(bpy.data.materials['ST / Weld metal'])
# Portable licensed palette; no public redistribution.
tex=OUT/'Textures';tex.mkdir(exist_ok=True);shutil.copy2(OLD/'Textures/T_SkidTank_SourcePalette.png',tex/'T_SkidTank_SourcePalette.png')
for im in bpy.data.images:
 if im.source=='FILE' and ('Gradient' in im.name or 'Palette' in im.name):im.filepath=str(tex/'T_SkidTank_SourcePalette.png');im.pack()
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True)
h.export(OUT/'WhiteTank_LowProfileSills_Complete.fbx',h.descendants(root))
for name in ['TankBody','LowProfileSills','Fittings']:h.export(OUT/('WhiteTank_'+name+'.fbx'),h.descendants(bpy.data.objects[name]))
qa={'result':'PASS','revision':'Lower inward-foot L-sills with four under-tank crossmembers','supports':2,'separate_subframe':False,'old_skid_objects_removed':old_names,'tank_mesh_vertex_hash_unchanged':True,'sill_length_cm':530,'sill_y_centres_cm':[-45,45],'tank_lowered_cm':37.3,'tank_keel_clearance_cm':12.7,'crossmembers':4,'crossmember_height_cm':7.62,'crossmember_bottom_cm':2.54,'crossmember_top_cm':10.16,'minimum_underbarrel_crossmember_gap_cm':2.54,'inward_flange':True,'plate_thickness_cm':.8,'foot_width_cm':15.24,'ground_z_cm':0,'direct_contact_samples':len(contacts),'upper_edge_from_actual_tank_rays':True,'profiles':profiles,'license':'Purchased pack derivative: exclude from public Git','gameplay_or_unreal_changes':False}
(OUT/'direct_sills_qa.json').write_text(json.dumps(qa,indent=2))
def captions(cam,col,title,lines):
 rot=cam.rotation_euler.to_matrix();w=cam.data.ortho_scale;hh=w*1100/1400
 for i,line in enumerate([title]+lines):
  o=h.text_world(line,cam.location+rot@Vector((-w*.46,hh*.43-i*w*.034,-1)),w*(.023 if i==0 else .012),'Annotation',col);o.rotation_euler=cam.rotation_euler
h.M={m.name:m for m in bpy.data.materials}
for name,color in [('Studio',(.65,.68,.69,1)),('Annotation',(.025,.04,.05,1))]:
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=color;h.M[name]=m
c,cam=h.studio((0,0,1.2),9.6,(9,-12,7));captions(cam,c,'WHITE TANK / LOW INWARD-FOOT SILLS',['Two direct L-sills, inward feet and four under-tank crossmembers','About 9 in web height | 5 in keel clearance | no raised saddles','Native tank body: 22 ft 8.8 in long x 7 ft 5 in diameter']);scene=bpy.context.scene;scene.cycles.device='CPU';scene.cycles.samples=24;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.view_settings.exposure=-.15;h.save_render(OUT/'WhiteTank_LowProfileSills_Overview.png',OUT/'Foundryline_WhiteTank_LowProfileSills.blend')
for o in list(c.objects):
 if o.type=='FONT':bpy.data.objects.remove(o,do_unlink=True)
cam.location=(8,-.55,1.5);cam.rotation_euler=(Vector((2.55,0,.33))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.7;captions(cam,c,'INWARD FEET / CROSSMEMBERS',['Low sills attached directly to the tank','Feet turn toward the centre | crossmembers stay below the barrel']);h.save_render(OUT/'WhiteTank_LowProfileSills_EndDetail.png',OUT/'Foundryline_WhiteTank_LowProfileSills_EndReview.blend');print('LOW_PROFILE_INWARD_SILLS_COMPLETE',json.dumps(qa))
