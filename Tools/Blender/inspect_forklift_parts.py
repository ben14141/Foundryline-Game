import bpy,json,tempfile,bmesh
from pathlib import Path
from mathutils import Vector
src=Path(tempfile.gettempdir())/'FoundrylineAssetInspection/SM_Forklift_01.fbx'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(src));o=bpy.data.objects['SM_Forklift_01'];M=o.matrix_world.copy();o.parent=None;o.data.transform(M);o.matrix_world.identity()
bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.to_mesh(o.data);bm.free()
adj=[[] for _ in o.data.vertices]
for e in o.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();rows=[]
for i in range(len(adj)):
 if i in seen:continue
 stack=[i];seen.add(i);ids=[]
 while stack:
  n=stack.pop();ids.append(n)
  for j in adj[n]:
   if j not in seen:seen.add(j);stack.append(j)
 vs=[o.data.vertices[j].co for j in ids];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)])
 rows.append({'n':len(ids),'min':[round(v,3) for v in lo],'max':[round(v,3) for v in hi],'center':[round(v,3) for v in (lo+hi)/2]})
print('MATERIALS',[m.name if m else None for m in o.data.materials]);print(json.dumps(sorted(rows,key=lambda x:-x['n']),indent=2))
Path(r'C:\Users\curly\Documents\Foundryline Game\Saved\Logs\ForkliftSourceParts.json').write_text(json.dumps(rows,indent=2))
