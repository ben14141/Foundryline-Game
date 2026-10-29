import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\Users\curly\Documents\Foundryline Game')
bpy.ops.wm.read_factory_settings(use_empty=True)
for name in ['SM_Pallet_01','SM_Rack_01','SM_Rack_400']:
 bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/MaterialHandling/FactorySource'/(name+'.fbx')));o=bpy.data.objects[name];vs=[o.matrix_world@Vector(v) for v in o.bound_box];print(name,[round(min(v[i] for v in vs),4) for i in range(3)],[round(max(v[i] for v in vs),4) for i in range(3)])
