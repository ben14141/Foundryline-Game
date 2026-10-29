import bpy,json
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=r'C:\Users\curly\Documents\Foundryline Game\SourceAssets\MegapolyTrucks\3_axle_Tanker_2.fbx')
for o in list(bpy.data.objects)[:4]:print(o.name,'parent',o.parent,'location',list(o.location),'scale',list(o.scale),'rot',list(o.rotation_euler),'matrix',o.matrix_world)
