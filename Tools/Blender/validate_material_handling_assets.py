"""Read-only checks against the dedicated authored review blend (Blender background)."""
import bpy, json
from pathlib import Path
root = Path(__file__).resolve().parents[2]
manifest = json.loads((root/'SourceAssets/MaterialHandling/manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(root/'SourceAssets/MaterialHandling/Authored/Foundryline_MaterialHandling.blend'))
checks = {}
checks['seventeen_authored_meshes'] = len(manifest['assets']) == 17 and all(bpy.data.objects.get(a['name']) for a in manifest['assets'])
sheet = bpy.data.objects['SteelSheet_60x120']
checks['sheet_exact_inches_to_meters'] = all(abs(a-b)<.0001 for a,b in zip(sorted(sheet.dimensions), sorted([1.524,3.048,.003175])))
checks['four_separate_wheels'] = len([a for a in manifest['assets'] if a['kind']=='wheel']) == 4
checks['rear_wheel_steering'] = sum(bool(a.get('steers')) for a in manifest['assets'] if a['kind']=='wheel') == 2
checks['three_independent_rollers'] = len([a for a in manifest['assets'] if a['kind']=='roller']) == 3
checks['rollers_doubled_to_352cm'] = all(abs(bpy.data.objects[a['name']].dimensions.y-3.52)<.001 for a in manifest['assets'] if a['kind']=='roller')
bpy.context.scene.frame_set(1)
low = bpy.data.objects['Forklift_Carriage'].location.z
bpy.context.scene.frame_set(120)
checks['carriage_lift_animation_280cm'] = abs(bpy.data.objects['Forklift_Carriage'].location.z-low-2.8)<.001
checks['all_fbx_files_exist'] = all((root/'SourceAssets'/a['source_file']).is_file() for a in manifest['assets'])
checks['all_meshes_have_materials'] = all(len(bpy.data.objects[a['name']].data.materials)>0 for a in manifest['assets'])
report = {'checks':checks,'passed':all(checks.values()),'sheet_dimensions_m':list(sheet.dimensions)}
(root/'Saved/Logs/MaterialHandlingBlenderQA.json').write_text(json.dumps(report,indent=2))
print('MATERIAL_HANDLING_BLENDER_QA',json.dumps(report))
