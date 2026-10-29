"""Read original saved geometry, verify contacts/mesh budget, tidy isolated views."""
import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[2]
folder=root/'SourceAssets/Inventory/Storage/ModularPalletRacking'
manifest=json.loads((folder/'manifest.json').read_text());qa=json.loads((folder/'geometry_qa.json').read_text())
for v in manifest['variants']:
 bpy.ops.wm.open_mainfile(filepath=str(folder/(v['name']+'.blend')))
 rack=bpy.data.objects[v['name']]
 vertices=len(rack.data.vertices);triangles=sum(len(p.vertices)-2 for p in rack.data.polygons)
 v['vertices']=vertices;v['triangles']=triangles;v['material_slots']=len(rack.data.materials)
 qa['checks'][v['name']+'_bounded_triangle_budget']=triangles<35000
 qa['checks'][v['name']+'_identity_export_transform']=all(abs(rack.location[i])<1e-6 and abs(rack.rotation_euler[i])<1e-6 and abs(rack.scale[i]-1)<1e-6 for i in range(3))
 for o in bpy.data.objects:
  if o.name.startswith('PalletRack_') and o.type=='MESH':o.hide_set(o!=rack)
 rack.hide_set(False);bpy.context.view_layer.objects.active=rack
 bpy.ops.wm.save_as_mainfile(filepath=str(folder/(v['name']+'.blend')))
bpy.ops.wm.open_mainfile(filepath=str(folder/'PalletRack_AllLevels_Review.blend'))
supports=[o for o in bpy.data.objects if o.name.startswith('Review pallet support')]
errors=[]
for o in supports:
 lowest=min((o.matrix_world@vert.co).z for vert in o.data.vertices)
 errors.append(min(abs(lowest-z) for z in [0,1.524,3.048])*100)
qa['review_support_count']=len(supports);qa['maximum_pallet_deck_contact_error_cm']=max(errors)
qa['checks']['review_all12_pallets_present']=len(supports)==36
qa['checks']['review_pallets_contact_floor_or_deck']=max(errors)<.001
assert all(qa['checks'].values()),qa
(folder/'manifest.json').write_text(json.dumps(manifest,indent=2));(folder/'geometry_qa.json').write_text(json.dumps(qa,indent=2))
print('MODULAR_RACK_VERIFIED',len(qa['checks']),[(v['name'],v['triangles']) for v in manifest['variants']],max(errors))
