import bpy,math,json,bmesh
from pathlib import Path
from mathutils import Matrix,Vector
ROOT=Path(r'C:\Users\curly\Documents\Foundryline Game');OUT=ROOT/'SourceAssets/AssemblyReviews/TankerTractor';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;scene.render.fps=30;scene.frame_start=1;scene.frame_end=360
palette=bpy.data.materials.new('Foundryline_MegapolyPalette');palette.use_nodes=True
bs=next(n for n in palette.node_tree.nodes if n.type=='BSDF_PRINCIPLED');tex=palette.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(ROOT/'SourceAssets/MegapolyTrucks/Tex_256Gradient.png'));palette.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color']);bs.inputs['Roughness'].default_value=.65;tex.image.pack()
parts=[];roots={};wheels={};legs=[]
def origin_mesh(o,center):
 for v in o.data.vertices:v.co-=center
 o.location=center
 bpy.context.view_layer.update()
def import_vehicle(src,label,factor,origin):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceAssets/MegapolyTrucks'/src));objs=[o for o in set(bpy.data.objects)-before if o.type=='MESH'];bpy.context.view_layer.update()
 matrices={o:o.matrix_world.copy() for o in objs};parents={o:bool(o.parent) for o in objs}
 for o in objs:
  M=matrices[o]
  if parents[o]:M=Matrix.Rotation(-math.pi/2,4,'X')@Matrix.Scale(1/factor,4)@M
  o.parent=None;o.data.transform(M);o.matrix_world=Matrix.Identity(4)
  for v in o.data.vertices:v.co-=Vector(origin)
  if o.data.uv_layers:
   active=next((u for u in o.data.uv_layers if u.active_render),o.data.uv_layers[0])
   for layer in list(o.data.uv_layers):
    if layer!=active:o.data.uv_layers.remove(layer)
   active.name='UVMap';active.active_render=True
  o.data.materials.clear();o.data.materials.append(palette)
  for poly in o.data.polygons:poly.material_index=0
 # Use authored wheel objects; spare wheels remain on the body.
 moving=[];body=[]
 for o in objs:
  if o.name.startswith('Circle'):
   vs=[v.co for v in o.data.vertices];low=Vector([min(v[i] for v in vs) for i in range(3)]);high=Vector([max(v[i] for v in vs) for i in range(3)]);size=high-low;center=(low+high)/2
   if abs(size.x-size.z)<.1 and size.x>.65 and size.y<size.x*.85:
    origin_mesh(o,center);o.name=label+'_Wheel_'+str(len(moving));moving.append(o);continue
  body.append(o)
 if label=='Tanker':
  chassis=next(o for o in body if o.name.startswith('Dump_'))
  # Source support legs are loose connected components. Separate only the two
  # lower sliding legs; leave their sleeves and cross brace on the chassis.
  adj=[[] for _ in chassis.data.vertices]
  for e in chassis.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
  seen=set();groups=[]
  for i in range(len(adj)):
   if i in seen:continue
   stack=[i];seen.add(i);ids=[]
   while stack:
    v=stack.pop();ids.append(v)
    for n in adj[v]:
     if n not in seen:seen.add(n);stack.append(n)
   coords=[chassis.data.vertices[j].co for j in ids];lo=Vector([min(v[k] for v in coords) for k in range(3)]);hi=Vector([max(v[k] for v in coords) for k in range(3)])
   if len(ids)==24 and .15<lo.z<.25 and .8<hi.z-lo.z<1.0 and hi.x-lo.x<.25:groups.append(ids)
  assert len(groups)==2,('gear groups',len(groups))
  allids=set(j for g in groups for j in g)
  for idx,g in enumerate(groups):
   ids=set(g);remap={old:new for new,old in enumerate(g)};verts=[chassis.data.vertices[j].co.copy() for j in g];faces=[[remap[j] for j in p.vertices] for p in chassis.data.polygons if all(j in ids for j in p.vertices)]
   mesh=bpy.data.meshes.new('Tanker_LegMesh');mesh.from_pydata(verts,[],faces);o=bpy.data.objects.new('Tanker_Leg_'+str(idx),mesh);scene.collection.objects.link(o);o.data.materials.append(palette)
   lo=Vector([min(v[k] for v in verts) for k in range(3)]);hi=Vector([max(v[k] for v in verts) for k in range(3)]);center=(lo+hi)/2;origin_mesh(o,center)
   # Add a proper 32 x 26 cm footplate to the telescoping lower leg.
   bpy.ops.mesh.primitive_cube_add(size=1,location=(center.x,center.y,lo.z+.015));foot=bpy.context.object;foot.dimensions=(.32,.26,.03);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);foot.data.materials.append(palette)
   bpy.ops.object.select_all(action='DESELECT');foot.select_set(True);o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.join();o.location.z+=.55;legs.append(o)
  bm=bmesh.new();bm.from_mesh(chassis.data);bm.verts.ensure_lookup_table();bmesh.ops.delete(bm,geom=[bm.verts[j] for j in allids],context='VERTS');bm.to_mesh(chassis.data);bm.free()
 bpy.ops.object.select_all(action='DESELECT')
 for o in body:o.select_set(True)
 bpy.context.view_layer.objects.active=body[0];bpy.ops.object.join();bodyobj=bpy.context.object;bodyobj.name=label+'_Body'
 root=bpy.data.objects.new(label+'_ROOT',None);scene.collection.objects.link(root);roots[label]=root
 for o in [bodyobj]+moving+(legs if label=='Tanker' else []):o.parent=root
 wheels[label]=moving
 for o in [bodyobj]+moving+(legs if label=='Tanker' else []):
  part={'name':o.name,'vehicle':label,'kind':'wheel' if o in moving else 'leg' if o in legs else 'body','position_cm':[round(v*100,5) for v in o.location]}
  if o in moving:part['radius_cm']=round(max(v.co.z for v in o.data.vertices)*100,5)
  parts.append(part)
 return root
trailer=import_vehicle('3_axle_Tanker_2.fbx','Tanker',100,(5.1,0,0))
tractor=import_vehicle('Semi_Truck_2.fbx','Tractor',254,(-2.07,0,0))
# Actor origin: tractor rear axle; trailer kingpin. Both are on the road plane.
# Fifth wheel lies 42 cm ahead of the tractor rear axle, 128 cm above ground.
for name,parent,loc in [('Kingpin',trailer,(0,0,1.28)),('FifthWheel',tractor,(.42,0,1.28))]:
 e=bpy.data.objects.new(name,None);scene.collection.objects.link(e);e.parent=parent;e.location=loc;e.empty_display_size=.25
# Authored demonstration: reverse, couple, raise gear, pull away, lower gear,
# uncouple, and move the tractor away. Gameplay will drive these transforms.
for frame,x in [(1,7),(100,-.42),(150,-.42),(230,7.58),(290,7.58),(360,13.58)]:tractor.location.x=x;tractor.keyframe_insert('location',frame=frame)
for frame,x in [(1,0),(150,0),(230,8),(360,8)]:trailer.location.x=x;trailer.keyframe_insert('location',frame=frame)
for leg in legs:
 base=leg.location.copy();leg['retracted_z']=base.z;leg['extension_metres']=.735
 for frame,ext in [(1,1),(105,1),(145,0),(240,0),(285,1),(360,1)]:leg.location.z=base.z-.735*ext;leg.keyframe_insert('location',frame=frame)
for label,items in wheels.items():
 for o in items:
  radius=max(v.co.z for v in o.data.vertices);r=roots[label]
  for frame in [1,100,150,230,290,360]:scene.frame_set(frame);o.rotation_euler.y=r.location.x/radius;o.keyframe_insert('rotation_euler',frame=frame)
scene.frame_set(1)
# Export each independently pivoted part at rest; wheel and gear animation is
# distance/state driven in Unreal, so it does not need baked animation assets.
for part in parts:
 sub='Products/Tankers/StraightBarrel' if part['vehicle']=='Tanker' else 'Vehicles/Semi/Megapoly'
 part['source_file']=sub+'/'+part['name']+'.fbx';part['asset_path']='/Game/Foundryline/'+sub+'/SM_'+part['name']
 (ROOT/'SourceAssets'/sub).mkdir(parents=True,exist_ok=True)
 o=bpy.data.objects[part['name']];saved=o.matrix_world.copy();o.parent=None;o.animation_data_clear();o.location=(0,0,0);o.rotation_euler=(0,0,0);o.scale=(1,1,1)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(ROOT/'SourceAssets'/part['source_file']),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
 o.parent=roots[part['vehicle']];o.location=[v/100 for v in part['position_cm']]
# Restore preview animation after export.
for label,items in wheels.items():
 for o in items:
  radius=max(v.co.z for v in o.data.vertices)
  for frame in [1,100,150,230,290,360]:scene.frame_set(frame);o.rotation_euler.y=roots[label].location.x/radius;o.keyframe_insert('rotation_euler',frame=frame)
for leg in legs:
 base=next(p['position_cm'][2]/100 for p in parts if p['name']==leg.name)
 for frame,ext in [(1,1),(105,1),(145,0),(240,0),(285,1),(360,1)]:leg.location.z=base-.735*ext;leg.keyframe_insert('location',frame=frame)
# Ground and lighting for an isolated asset review; no game map is changed.
bpy.ops.mesh.primitive_plane_add(size=70,location=(4,0,-.025));ground=bpy.context.object;ground.name='ReviewGround'
gray=bpy.data.materials.new('ReviewAsphalt');gray.diffuse_color=(.08,.09,.10,1);ground.data.materials.append(gray)
bpy.ops.object.camera_add(location=(23,-22,14));camera=bpy.context.object;camera.name='ReviewCamera';camera.rotation_euler=(Vector((3,0,1.6))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=28;scene.camera=camera
bpy.ops.object.light_add(type='AREA',location=(2,-6,14));bpy.context.object.data.energy=3200;bpy.context.object.data.size=12
scene.world=bpy.data.worlds.new('ReviewWorld');scene.world.color=(.25,.25,.25);scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100
for frame,label in [(1,'Detached'),(150,'Coupled'),(285,'Supported')]:
 scene.frame_set(frame);scene.render.filepath=str(OUT/('Review_'+label+'.png'));bpy.ops.render.render(write_still=True)
scene.frame_set(1)
for area in bpy.context.screen.areas:
 if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_Tanker_Tractor.blend'))
(OUT/'parts.json').write_text(json.dumps({'parts':parts,'tractor_hitch_cm':[42,0,128],'trailer_hitch_cm':[0,0,128],'landing_extension_cm':73.5,'animation_frames':360},indent=2))
print('RIG_EXPORT_COMPLETE',len(parts),'wheels',len(wheels['Tanker']),len(wheels['Tractor']))
