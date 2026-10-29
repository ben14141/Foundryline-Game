import bpy
bpy.ops.wm.open_mainfile(filepath=r'C:\Users\curly\Documents\Foundryline Game\SourceAssets\Vehicles\TankerRig\Foundryline_Tanker_Tractor.blend')
for name in ['Tractor_Body','Tanker_Body','Tractor_Wheel_0']:
 o=bpy.data.objects[name];print(name,[(u.name,u.active_render) for u in o.data.uv_layers], 'active',o.data.uv_layers.active_index)
