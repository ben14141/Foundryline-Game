import bpy,json,bmesh
from pathlib import Path
from mathutils import Vector
R=Path(r'C:/Users/curly/Documents/Foundryline Game');result={}
for name,path in [('shell',R/'SourceAssets/Products/Components/Fabrication/ShellSection.fbx'),('legacy_output',R/'SourceAssets/MaterialHandling/Products/Components/Steel/SteelSheet_RolledBlank.fbx'),('forks',R/'SourceAssets/MaterialHandling/Vehicles/Forklift/FL01/Forklift_Carriage.fbx')]:
 if not path.exists():result[name]={'missing':str(path)};continue
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path));points=[o.matrix_world@v.co for o in bpy.data.objects if o.type=='MESH' for v in o.data.vertices];lo=Vector([min(v[k] for v in points) for k in range(3)]);hi=Vector([max(v[k] for v in points) for k in range(3)]);result[name]={'min_cm':list(lo*100),'max_cm':list(hi*100),'size_cm':list((hi-lo)*100)}
 if name=='forks':
  for o in bpy.data.objects:
   if o.type!='MESH':continue
   adj=[[] for v in o.data.vertices]
   for e in o.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
   seen=set();rows=[]
   for i in range(len(adj)):
    if i in seen:continue
    stack=[i];seen.add(i);ids=[]
    while stack:
     a=stack.pop();ids.append(a)
     for j in adj[a]:
      if j not in seen:seen.add(j);stack.append(j)
    vs=[o.matrix_world@o.data.vertices[j].co for j in ids];mi=[min(v[k] for v in vs)*100 for k in range(3)];ma=[max(v[k] for v in vs)*100 for k in range(3)]
    if ma[0]-mi[0]>70 and ma[1]-mi[1]<25:rows.append({'min_cm':mi,'max_cm':ma})
   result[name]['tine_candidates']=rows
   front=[o.matrix_world@v.co for v in o.data.vertices if (o.matrix_world@v.co).x>.80 and abs((o.matrix_world@v.co).y)>.40]
   result[name]['blade_front_z_cm']=[min(v.z for v in front)*100,max(v.z for v in front)*100]
(R/'Saved/Logs/CurvedCradleSourceMeasure.json').write_text(json.dumps(result,indent=2));print('CRADLE_SOURCE_MEASURE',json.dumps(result))
