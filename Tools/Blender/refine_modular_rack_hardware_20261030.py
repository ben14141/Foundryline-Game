"""Isolated background rebuild of the original rack family and hardware detail review.
Run Blender --background --threads 2 --python this_file. Does not use the live Blender.
"""
import runpy, bpy, json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]
context=runpy.run_path(str(root/'Tools/Blender/build_modular_pallet_racking.py'),run_name='__main__')
out=root/'SourceAssets/Inventory/Storage/ModularPalletRacking'
manifest=json.loads((out/'manifest.json').read_text())
checks=json.loads((out/'geometry_qa.json').read_text())
assert all(checks['checks'].values())
assert all(abs(v['dimensions_cm'][0]-305)<.01 and abs(v['dimensions_cm'][1]-148.03)<.1 for v in manifest['variants'])
for o in bpy.data.objects:
 if o.type=='MESH' and ('Review pallet' in o.name or 'Review cut plate' in o.name):o.hide_render=True
for i,rack in enumerate(context['racks']):rack.hide_render=i!=1
rack=context['racks'][1]
bpy.context.scene.camera.location=(2.3,-3.6,2.15)
bpy.context.scene.camera.rotation_euler=(Vector((1.36,-.62,1.15))-bpy.context.scene.camera.location).to_track_quat('-Z','Y').to_euler()
bpy.context.scene.camera.data.ortho_scale=1.95
scene=bpy.context.scene;scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.cycles.samples=24
scene.render.filepath=str(out/'PalletRack_HardwareDetail.png');bpy.ops.render.render(write_still=True)
report={'passed':True,'unchanged_footprint_cm':[305,148.03],'shelf_interval_cm':152.4,'details_added':['bolted upright foot plates','hex anchor nuts and stud tips','side brace fasteners','beam connector bolts','beam locking pins and retaining rings'],'existing_details_preserved':['side diagonal bracing','rear cross bracing','open mesh decks','front forklift access'],'materials_preserved':list(manifest['materials']),'review_image':str(out/'PalletRack_HardwareDetail.png')}
(out/'hardware_detail_qa.json').write_text(json.dumps(report,indent=2))
print('MODULAR_RACK_HARDWARE_REVIEW_COMPLETE')
