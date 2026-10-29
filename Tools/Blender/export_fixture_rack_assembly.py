"""Export the removable fixture kit in its rack-on-bench pose, sharing the bench floor datum.

blender -b SourceAssets/Production/Equipment/WeldingBench/Foundryline_RackWeldingSetup.blend --python Tools/Blender/export_fixture_rack_assembly.py -- <out.fbx>
The standalone kit FBX lays the risers out compactly; this pose is what the station places on its bench.
"""
import bpy,sys
out=sys.argv[sys.argv.index('--')+1]
bpy.ops.object.select_all(action='DESELECT')
objs=[o for o in bpy.data.collections['BENCH / Removable fixture'].all_objects if o.type=='MESH']
for o in objs:o.select_set(True)
bpy.context.view_layer.objects.active=objs[0]
bpy.ops.export_scene.fbx(filepath=out,use_selection=True,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',object_types={'MESH'},use_mesh_modifiers=True)
print('EXPORTED',len(objs),out)
