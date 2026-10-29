"""Split the authored tanker into assembly groups, preserving mesh positions and UVs."""
import bpy,json,math,sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).parent))
from tank_head_geometry import make_head
root=Path(r'C:/Users/curly/Documents/Foundryline Game');out=root/'SourceAssets/Products/Tankers/StraightBarrel/ProductionStages';out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root/'SourceAssets/AssemblyReviews/TankerTractor/Foundryline_Tanker_Tractor.blend'))
body=bpy.data.objects['Tanker_Body'];me=body.data
adj=[[] for _ in me.vertices]
for e in me.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();group={};sizes={};num=0
for i in range(len(adj)):
 if i in seen:continue
 stack=[i];seen.add(i);ids=[]
 while stack:
  a=stack.pop();ids.append(a)
  for b in adj[a]:
   if b not in seen:seen.add(b);stack.append(b)
 pts=[me.vertices[j].co for j in ids];lo=Vector([min(p[k] for p in pts) for k in range(3)]);hi=Vector([max(p[k] for p in pts) for k in range(3)]);d=hi-lo
 stage=1 if d.x>2 and d.z>1.8 and d.y>1.8 else 2 if d.x<.7 and d.y>1.3 and d.z>1 else 0 if hi.z<1.8 or (d.x>8 and d.z<1.2) else 3
 for j in ids:group[j]=(stage,lo,hi)
 num+=1
faces={i:[] for i in range(4)}
for p in me.polygons:
 stage,lo,hi=group[p.vertices[0]]
 if stage==1 and abs(p.normal.x)>.9:stage=2
 faces[stage].append(p)
steel=bpy.data.materials.get('MH_CarbonSteel') or bpy.data.materials.new('MH_CarbonSteel');steel.diffuse_color=(.4,.42,.44,1)
entries=[];stages=[]
for stage,name in enumerate(['ProductFrame','ProductShell','ProductEndCaps','ProductFittings']):
 polygons=faces[stage];assert polygons,('Empty production group',name)
 used=sorted({i for p in polygons for i in p.vertices});remap={old:new for new,old in enumerate(used)};mesh=bpy.data.meshes.new(name);mesh.from_pydata([me.vertices[j].co for j in used],[],[tuple(remap[j] for j in p.vertices) for p in polygons])
 for m in me.materials:mesh.materials.append(m)
 uv=mesh.uv_layers.new(name='UVMap')
 for dst,src in zip(mesh.polygons,polygons):
  dst.material_index=src.material_index;dst.use_smooth=src.use_smooth
  for dl,sl in zip(dst.loop_indices,src.loop_indices):uv.data[dl].uv=me.uv_layers.active.data[sl].uv
 obj=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(obj)
 if stage==1:
  # Real cylindrical shell; head weld tangencies match the complete F&D heads.
  verts=[(x,1.175*math.cos(i*2*math.pi/96),2.44904+1.175*math.sin(i*2*math.pi/96)) for x in [-10.2025,.9138] for i in range(96)]
  shell=bpy.data.meshes.new('OpenTankShell');shell.from_pydata(verts,[],[(i,(i+1)%96,(i+1)%96+96,i+96) for i in range(96)]);shell.materials.append(steel);obj.data=shell
  for p in shell.polygons:p.use_smooth=True
  bpy.context.view_layer.objects.active=obj;solid=obj.modifiers.new('Steel gauge','SOLIDIFY');solid.thickness=.003175;solid.offset=-1;bpy.ops.object.modifier_apply(modifier=solid.name)
 if stage==2:
  bpy.data.objects.remove(obj,do_unlink=True);heads=[]
  for x,angle in [(.8638,math.pi/2),(-10.1525,-math.pi/2)]:
   head,_=make_head('WholeHead',steel);head.rotation_euler.y=angle;head.location=(x,0,2.44904);heads.append(head)
  bpy.ops.object.select_all(action='DESELECT')
  for h in heads:h.select_set(True)
  bpy.context.view_layer.objects.active=heads[0];bpy.ops.object.join();obj=bpy.context.object;obj.name=name
  bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 if not obj.data.uv_layers:
  uv=obj.data.uv_layers.new(name='UVMap')
  for p in obj.data.polygons:
   for loop in p.loop_indices:
    v=obj.data.vertices[obj.data.loops[loop].vertex_index].co;uv.data[loop].uv=(v.x*.1,v.z*.25)
 bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
 entries.append({'name':name,'stage':stage,'polygons':len(polygons)})
 stages.append(obj)
copies=[]
for obj in stages:
 copy=obj.copy();copy.data=obj.data.copy();bpy.context.scene.collection.objects.link(copy);copies.append(copy)
bpy.ops.object.select_all(action='DESELECT')
for copy in copies:copy.select_set(True)
bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();complete=bpy.context.object;complete.name='Tanker_Body'
bpy.ops.export_scene.fbx(filepath=str(out/'Tanker_Body.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
entries.append({'name':'Tanker_Body','stage':-1,'polygons':len(complete.data.polygons)})
(out/'manifest.json').write_text(json.dumps({'assets':entries,'source_components':num,'notes':'Independent frame, shell, end-cap and fitting groups. Wheel and landing-gear assets remain independent.'},indent=2));print('TANKER_PRODUCTION_STAGES_COMPLETE')
