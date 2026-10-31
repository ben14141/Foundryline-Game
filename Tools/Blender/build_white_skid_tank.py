"""Licensed tank derivative for local review only. Never import or publish automatically."""
import bpy,bmesh,math,json,hashlib,importlib.util
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');OUT=ROOT/'SourceAssets/Products/SkidTank/Review_2026-10-29';OUT.mkdir(parents=True,exist_ok=True)
s=importlib.util.spec_from_file_location('source',ROOT/'Tools/Blender/inspect_white_skid_tank.py');src=importlib.util.module_from_spec(s);s.loader.exec_module(src)
s=importlib.util.spec_from_file_location('h',ROOT/'Tools/Blender/build_rack_welding_bench.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
SOURCE_FILES=[src.SRC/'Meshes/Truck_Tanker_1.fbx',src.SRC/'Prefabs/Truck_Tanker_1.prefab'];before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in SOURCE_FILES}

def material(name,color,metal,rough,coat=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough;bs.inputs['Coat Weight'].default_value=coat;bs.inputs['Coat Roughness'].default_value=.18;return m

def group(name):
 c=h.collection(name);g=bpy.data.objects.new(name,None);c.objects.link(g);g.parent=root;return c,g

def rod(name,a,b,r,material,c,parent):
 a,b=Vector(a),Vector(b);o=h.cylinder(name,r,(b-a).length,(a+b)/2,material,c,parent);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o

def profile(name,points,x,width,material,c,parent):
 n=len(points);v=[(x+dx,y,z) for dx in [-width/2,width/2] for y,z in points];f=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)];return h.mesh(name,v,f,material,c,parent,bevel=.0006)

def bounds(objects):
 vs=[o.matrix_world@v.co for o in objects if o.type=='MESH' for v in o.data.vertices];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)]);return {'min_cm':list(lo*100),'max_cm':list(hi*100),'size_cm':list((hi-lo)*100)}

def feet(m):
 inches=m/.0254;ft=int(inches//12);return str(ft)+' ft '+format(inches-ft*12,'.1f')+' in'

objects=src.reconstructed('Truck_Tanker_1');keep={'Roundcube.002':'TankBody_Source','Cube.269':'TopWalkwayAndManways_Source','Cube.267':'RearTankLadder_Source'}
for o in list(bpy.data.objects):
 if o.type!='MESH' or o.name not in keep:bpy.data.objects.remove(o,do_unlink=True)
# Other FBX armature/null objects are irrelevant; remove only after mesh parents were baked by reconstruction.
for o in list(bpy.data.objects):
 if o.type!='MESH':bpy.data.objects.remove(o,do_unlink=True)
body=bpy.data.objects['Roundcube.002'];native=src.bounds([body]);shift=Vector(((native['min'][0]+native['max'][0])/2,0,native['min'][2]-.50))
root=bpy.data.objects.new('WhiteSkidTank_FloorCentre',None);bpy.context.scene.collection.objects.link(root);root['license_notice']='Derivative of purchased Megapoly.Art Trucks; exclude from public repositories.'
Cbody,Gbody=group('TankBody');Cfittings,Gfit=group('Fittings');Cskid,Gskid=group('SupportSkid')
for o in list(bpy.data.objects):
 if o.type=='MESH':
  for v in o.data.vertices:v.co-=shift
  name=o.name;o.name=keep[name];h.move(o,Cbody if name=='Roundcube.002' else Cfittings);o.parent=Gbody if name=='Roundcube.002' else Gfit
white=material('ST / White coated steel',(.78,.79,.77),.12,.31,.3);steel=material('ST / Machined steel',(.26,.30,.33),.9,.28);graphite=material('ST / Graphite painted skid',(.025,.034,.043),.12,.35,.2);rubber=material('ST / Contact isolation',(.018,.023,.028),0,.76);weld=material('ST / Weld metal',(.085,.105,.12),.8,.47);yellow=material('ST / Service yellow',(.63,.36,.015),0,.4)
body.data.materials.clear();body.data.materials.append(white)
for p in body.data.polygons:p.material_index=0
for o in [body,bpy.data.objects['TopWalkwayAndManways_Source'],bpy.data.objects['RearTankLadder_Source']]:
 if o.data.has_custom_normals:
  bpy.context.view_layer.objects.active=o;bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.ops.mesh.customdata_custom_splitnormals_clear()
 for p in o.data.polygons:p.use_smooth=o==body
 n=o.modifiers.new('Manufactured surface normals','WEIGHTED_NORMAL');n.keep_sharp=True;n.weight=40
h.M={m.name:m for m in bpy.data.materials};h.M.update({'Skid':graphite,'Steel':steel,'White':white,'RubberPad':rubber,'Rubber':rubber,'Weld':weld,'YellowService':yellow})
# Four pairs of floor pads carry the two long structural sills.
rail_y=.72;rail_length=7.8
for y in [-rail_y,rail_y]:
 h.tube('LongitudinalGroundSill',rail_length,.1016,.2032,.00635,(0,y,.1176),'X','Skid',Cskid,Gskid)
 for x in [-3.25,-1.05,1.05,3.25]:
  p=h.box('GroundBearingPad',(.38,.30,.016),(x,y,.008),'Skid',Cskid,Gskid,.003)
  for dx in [-.125,.125]:h.hole(p,(x+dx,y,.008),.014,.06)
for x in [-3.55,-2.10,0,2.10,3.55]:h.tube('SkidCrossMember',2.20,.12,.15,.006,(x,0,.2942),'Y','Skid',Cskid,Gskid)
# Actual mesh-derived underside profiles, not a guessed whole-tank cradle radius.
bpy.context.view_layer.update();tree=BVHTree.FromObject(body,bpy.context.evaluated_depsgraph_get());ys=sorted({round(v.co.y,6) for v in body.data.vertices if abs(v.co.y)<1.025});ys=sorted(set(ys+[-1.02,0,1.02]));contacts=[]
for station in [-2.10,2.10]:
 pts=[]
 for y in ys:
  hit,normal,face,distance=tree.ray_cast(Vector((station,y,-1)),Vector((0,0,1)),5);assert hit is not None,(station,y);pts.append((y,hit.z));contacts.append((station,y,hit.z))
 profile('TankContourSaddleWeb',[(pts[0][0],.3692),(pts[-1][0],.3692)]+[(y,z-.00635) for y,z in reversed(pts)],station,.014,'Skid',Cskid,Gskid)
 profile('ReplaceableTankContactStrip',pts+[(y,z-.00635) for y,z in reversed(pts)],station,.14,'RubberPad',Cskid,Gskid)
 for y in [-.72,.72]:
  # Braced saddle foot, on the crossmember; never blocks a ground support.
  for side in [-1,1]:
   n=Vector((station+side*.15,y,.37));t=Vector((station,y,.57));b=Vector((station,y,.37));v=[tuple(q+Vector((0,d,0))) for d in [-.004,.004] for q in [b,n,t]];h.mesh('SaddleFootGusset',v,[(0,2,1),(3,4,5),(0,1,4,3),(1,2,5,4),(2,0,3,5)],'Skid',Cskid,Gskid,bevel=.0007)
 for y in [-.88,.88]:
  h.box('SaddleMountFoot',(.36,.22,.018),(station,y,.3782),'Skid',Cskid,Gskid,.002)
  for dx in [-.125,.125]:h.cylinder('SaddleHoldDownBolt',.013,.018,(station+dx,y,.395),'Steel',Cskid,Gskid,verts=6)
 seam=h.cable('SaddleBaseWeld',[(station-.008,-.98,.373),(station-.008,0,.373),(station-.008,.98,.373)],.0025,Cskid,Gskid);seam.data.materials.clear();seam.data.materials.append(weld)
# Four provisioned lifting eyes (no invented load rating, no forklift pockets).
for x in [-2.85,2.85]:
 for y in [-.72,.72]:
  lug=h.box('ProvisionedLiftingEye',(.020,.18,.18),(x,y,.3092),'Skid',Cskid,Gskid,.008);h.hole(lug,(x,y,.337),.028,.08,'X')
# Three original manways are retained. Add their actual small top fastener clusters.
top=bpy.data.objects['TopWalkwayAndManways_Source'];bm=bmesh.new();bm.from_mesh(top.data);adj=[[] for v in top.data.vertices]
for e in top.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();candidates=[]
for i in range(len(adj)):
 if i in seen:continue
 stack=[i];seen.add(i);ids=[]
 while stack:
  j=stack.pop();ids.append(j)
  for k in adj[j]:
   if k not in seen:seen.add(k);stack.append(k)
 vs=[top.data.vertices[j].co for j in ids];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)]);d=hi-lo
 if .28<d.x<.75 and .28<d.y<.75 and d.z<.20:candidates.append(((lo+hi)/2,max(d.x,d.y)/2,hi.z))
bm.free();centres=[]
for p,r,z in sorted(candidates,key=lambda a:-a[1]):
 if all(abs(p.x-q[0])>.35 for q in centres):centres.append((p.x,p.y,z,min(r,.27)))
for x,y,z,r in centres[:3]:
 for i in range(8):a=i*math.tau/8;h.cylinder('ManwayClampFastener',.009,.018,(x+math.cos(a)*r*.88,y+math.sin(a)*r*.88,z+.005),'Steel',Cfittings,Gfit,verts=6)
# A source-surface-mounted service outlet with flanges, shutoff body, bolted blind and handwheel.
hit,normal,face,dist=tree.ray_cast(Vector((-10,-.55,.95)),Vector((1,0,0)),20);assert hit is not None;axis=normal.normalized();end=hit+axis*.28;rod('WeldedOutletNeck',hit-axis*.035,end,.065,'White',Cfittings,Gfit)
for distance in [.24,.39]:
 centre=hit+axis*distance;o=rod('BoltedOutletFlange',centre-axis*.018,centre+axis*.018,.115,'Steel',Cfittings,Gfit)
 tangent=axis.cross(Vector((0,0,1))).normalized();side=axis.cross(tangent).normalized()
 for j in range(8):a=j*math.tau/8;p=centre+tangent*(.092*math.cos(a))+side*(.092*math.sin(a));rod('OutletFlangeBolt',p-axis*.025,p+axis*.025,.009,'Steel',Cfittings,Gfit)
valve=h.box('ServiceShutoffValve',(.19,.18,.17),hit+axis*.32,'Skid',Cfittings,Gfit,.018);wheelcentre=hit+axis*.32+Vector((0,0,.20));rod('ValveStem',hit+axis*.32,wheelcentre,.014,'Steel',Cfittings,Gfit)
for i in range(16):
 a=i*math.tau/16;b=(i+1)*math.tau/16;rod('ValveHandwheelRim',wheelcentre+Vector((.12*math.cos(a),.12*math.sin(a),0)),wheelcentre+Vector((.12*math.cos(b),.12*math.sin(b),0)),.011,'YellowService',Cfittings,Gfit)
for a in [0,math.tau/3,2*math.tau/3]:rod('ValveHandwheelSpoke',wheelcentre,wheelcentre+Vector((.12*math.cos(a),.12*math.sin(a),0)),.007,'Steel',Cfittings,Gfit)
# Rear ladder entry step connects to the skid, while retaining the native upper ladder.
h.box('RearAccessStep',(.36,.55,.035),(-3.60,0,.35),'Skid',Cskid,Gskid,.004)
for y in [-.20,.20]:rod('AccessStepSupport',(-3.55,y,.22),(-3.60,y,.335),.025,'Skid',Cskid,Gskid)
for y in [-.20,-.10,0,.10,.20]:h.box('AntiSlipStepRib',(.29,.012,.008),(-3.60,y,.3715),'Steel',Cskid,Gskid,.001)
bpy.context.view_layer.update();allmesh=[o for o in root.children_recursive if o.type=='MESH'];bb=bounds(allmesh);assert bb['min_cm'][2]>=-.01,bb;assert len([o for o in allmesh if o.name.endswith('_Source')])==3
# Independent, separable assembly exports share a floor-centre datum. Licensed derivatives stay local.
h.export(OUT/'WhiteSkidTank_Complete.fbx',h.descendants(root))
for name,g in [('TankBody',Gbody),('SupportSkid',Gskid),('Fittings',Gfit)]:h.export(OUT/('WhiteSkidTank_'+name+'.fbx'),h.descendants(g))
qa={'source_identity':'Megapoly.Art Truck_Tanker_1 / white middle tanker','source_body_object':'Roundcube.002','native_unit_basis':'FBX GeometricScaling254 with centimetre units =2.54 metre mesh basis, combined with Unity prefab transforms; no subjective scale','source_body_size_cm':[v*100 for v in native['size']],'final_bounds':bb,'source_nodes_retained':list(keep),'truck_nodes_retained':0,'floor_origin_cm':[0,0,0],'actual_contour_contact_samples':len(contacts),'max_contact_error_cm':0,'ground_pad_bottom_cm':0,'saddle_stations_cm':[-210,210],'handling':'Provisioned lifting eyes; no forklift compatibility or load rating claimed','manway_fastener_clusters':len(centres[:3]),'license':'Purchased-pack derivative. DO NOT commit these files to public Git.','source_files_unchanged':before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in SOURCE_FILES},'review_only':True}
# Camera and modest CPU renders only; no Unreal process interaction.
h.M={m.name:m for m in bpy.data.materials};h.M['Annotation']=material('Annotation',(.025,.04,.05),0,.8);h.M['Studio']=material('Studio',(.65,.68,.69),0,.75)
bpy.context.scene.world=bpy.data.worlds.new('SkidTank review world')
c,cam=h.studio((0,0,1.4),10.5,(9,-12,7));h.overlay(cam,c,'FOUNDRYLINE / WHITE TANK ON SKIDS',['Original white middle tank - truck removed','Tank body '+feet(native['size'][0])+' long x '+feet(native['size'][1])+' diameter','Steel ground sills, shaped saddles, service outlet and retained top fittings'])
scene=bpy.context.scene;scene.cycles.device='CPU';scene.cycles.samples=24;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.view_settings.exposure=-.15
h.save_render(OUT/'WhiteSkidTank_Overview.png',OUT/'Foundryline_WhiteSkidTank.blend')
# Lower angle shows actual tank/pad/saddle/sill load paths and unobstructed ground contact.
for o in list(c.objects):
 if o.type=='FONT':bpy.data.objects.remove(o,do_unlink=True)
cam.location=(6,-8,4);cam.rotation_euler=(Vector((0,0,.75))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=9.4;h.overlay(cam,c,'FOUNDRYLINE / GROUND SUPPORT DETAIL',['Independent tank + support skid + fittings','Two mesh-matched saddles transfer load into crossmembers and floor pads','Provisioned lifting eyes - no invented load rating or forklift assumption']);h.save_render(OUT/'WhiteSkidTank_SupportDetail.png',OUT/'Foundryline_WhiteSkidTank_SupportReview.blend')
(OUT/'manifest_and_qa.json').write_text(json.dumps(qa,indent=2));print('WHITE_SKID_TANK_REVIEW_COMPLETE',json.dumps(qa))
