import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\Users\curly\Documents\Foundryline Game')
result={}
for filename in ['3_axle_Tanker_1.fbx','3_axle_Tanker_2.fbx','Semi_Truck_2.fbx']:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/MegapolyTrucks'/filename))
 bpy.context.view_layer.update()
 rows=[]
 for o in bpy.data.objects:
  if o.type!='MESH':continue
  coords=[o.matrix_world@Vector(c) for c in o.bound_box];lo=Vector([min(v[i] for v in coords) for i in range(3)]);hi=Vector([max(v[i] for v in coords) for i in range(3)])
  rows.append({'name':o.name,'min':list(lo),'max':list(hi),'center':list((lo+hi)/2),'size':list(hi-lo),'vertices':len(o.data.vertices)})
 result[filename]=rows
(root/'Saved/Logs/TruckBlenderParts.json').write_text(json.dumps(result,indent=2))
print('INSPECT_DONE')
