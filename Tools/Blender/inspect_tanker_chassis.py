import bpy,math,json
from pathlib import Path
from mathutils import Matrix,Vector
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=r'C:\Users\curly\Documents\Foundryline Game\SourceAssets\MegapolyTrucks\3_axle_Tanker_2.fbx')
o=bpy.data.objects['Dump_1_Trailer.002'];mat=Matrix.Rotation(-math.pi/2,4,'X')@Matrix.Scale(.01,4)@o.matrix_world
verts=o.data.vertices;adj=[[] for v in verts]
for e in o.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();rows=[]
for i in range(len(verts)):
 if i in seen:continue
 stack=[i];seen.add(i);ids=[]
 while stack:
  v=stack.pop();ids.append(v)
  for n in adj[v]:
   if n not in seen:seen.add(n);stack.append(n)
 vs=[mat@verts[v].co for v in ids];lo=[min(v[j] for v in vs) for j in range(3)];hi=[max(v[j] for v in vs) for j in range(3)]
 if hi[0]>1 and lo[2]<1.8:rows.append({'n':len(ids),'min':[round(v,3) for v in lo],'max':[round(v,3) for v in hi]})
print(json.dumps(rows))
