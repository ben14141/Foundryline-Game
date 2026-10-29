import bpy,json
from mathutils import Vector
from pathlib import Path
root=Path(r'C:/Users/curly/Documents/Foundryline Game');rows={}
for filename in ['Truck_sidewall.fbx','Truck_van_trailer.fbx']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/MegapolySmallTrucks/Meshes'/filename));bpy.context.view_layer.update();result=[]
 for o in bpy.data.objects:
  if o.type!='MESH':continue
  vs=[o.matrix_world@Vector(c) for c in o.bound_box];lo=[min(v[i] for v in vs) for i in range(3)];hi=[max(v[i] for v in vs) for i in range(3)];result.append({'name':o.name,'min':lo,'max':hi,'size':[hi[i]-lo[i] for i in range(3)]})
 rows[filename]=result
(root/'Saved/Logs/SmallTruckSourceParts.json').write_text(json.dumps(rows,indent=2))
