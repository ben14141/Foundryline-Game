"""Check authored scale, component hierarchy and review animation in the saved blend."""
import bpy, json
from pathlib import Path
root=Path(r'C:/Users/curly/Documents/Foundryline Game')
folder=root/'SourceAssets/Production/Equipment/PressBrake'
bpy.ops.wm.open_mainfile(filepath=str(folder/'Foundryline_PressBrake.blend'))
manifest=json.loads((folder/'manifest.json').read_text())
assert len(manifest['assets'])==12
assert all((folder/(entry['name']+'.fbx')).is_file() for entry in manifest['assets'])
ram=bpy.data.objects['PressBrake_Ram']
punch=bpy.data.objects['PressBrake_UpperPunch']
rods=bpy.data.objects['PressBrake_PistonRods']
assert punch.parent==ram and rods.parent==ram
scene=bpy.context.scene
scene.frame_set(1);bpy.context.view_layer.update()
initial=[o.matrix_world.translation.z for o in [ram,punch,rods]]
assert abs(bpy.data.objects['PressBrake_SampleFlat'].dimensions.z-.003175)<.0001
assert abs(punch.dimensions.y-3.048)<.003
scene.frame_set(100);bpy.context.view_layer.update()
closed=[o.matrix_world.translation.z for o in [ram,punch,rods]]
assert all(abs((a-b)-.315)<.00001 for a,b in zip(initial,closed))
assert bpy.data.objects['PressBrake_SampleFlat'].hide_render
assert not bpy.data.objects['PressBrake_SampleBent90'].hide_render
report={'assets':12,'tooling_length_nominal_cm':304.8,'actual_segmented_length_cm':punch.dimensions.y*100,'ram_and_tooling_travel_cm':[(a-b)*100 for a,b in zip(initial,closed)],'flat_stock_thickness_mm':3.175,'result':'PASS'}
(root/'Saved/Logs/PressBrakeModelQA.json').write_text(json.dumps(report,indent=2))
print('PRESS_BRAKE_MODEL_QA_PASS',json.dumps(report))
