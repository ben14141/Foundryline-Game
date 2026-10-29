"""Verify dimensions, clearances and UVs in the actual saved equipment blends."""
import bpy,json,math
from pathlib import Path
root=Path(r'C:/Users/curly/Documents/Foundryline Game');base=root/'SourceAssets/Production/Equipment';checks={}
for folder,file,prefix in [('PressBrake','Foundryline_PressBrake.blend','PB_'),('PlasmaTable','Foundryline_PlasmaTable.blend','PT_'),('SheetRoller','Foundryline_SheetRoller.blend','SR_')]:
 bpy.ops.wm.open_mainfile(filepath=str(base/folder/file));scene=bpy.context.scene;scene.frame_set(1);bpy.context.view_layer.update()
 manifest=json.loads((base/folder/'manifest.json').read_text())
 assert all((base/folder/(a['name']+'.fbx')).is_file() for a in manifest['assets'])
 screenfaces=0
 for o in scene.objects:
  if o.type!='MESH':continue
  assert all(math.isfinite(c) for v in o.data.vertices for c in v.co)
  for p in o.data.polygons:
   if o.data.materials and o.data.materials[p.material_index] and o.data.materials[p.material_index].name.startswith(prefix+'CNCDisplay'):
    screenfaces+=1
    for loop in p.loop_indices:
     assert all(-.001<=c<=1.001 for c in o.data.uv_layers.active.data[loop].uv),'Invalid screen UV '+folder
 assert screenfaces>0
 checks[folder]={'finite_geometry':True,'display_uvs':True,'exported_meshes':len(manifest['assets'])}
 if folder=='PressBrake':
  ram=bpy.data.objects['PressBrake_Ram'];punch=bpy.data.objects['PressBrake_UpperPunch'];z=punch.matrix_world.translation.z
  assert abs(punch.dimensions.y-3.048)<.003
  scene.frame_set(100);bpy.context.view_layer.update();assert abs(z-punch.matrix_world.translation.z-.315)<.00001
  checks[folder]['tooling_and_stroke']=True
 elif folder=='PlasmaTable':
  assert manifest['work_envelope_cm']==[304.8,426.72]
  torch=bpy.data.objects['PlasmaTable_Torch'];stock=bpy.data.objects['PlasmaTable_StockSheet']
  tip=min((torch.matrix_world@v.co).z for v in torch.data.vertices)
  top=max((stock.matrix_world@v.co).z for v in stock.data.vertices)
  gap=(tip-top)*1000
  assert 1.4<gap<2.2,('Nozzle clearance',gap)
  assert abs(stock.dimensions.x-1.524)<.0001 and abs(stock.dimensions.y-3.048)<.0001
  checks[folder]['nozzle_clearance_mm']=gap;checks[folder]['work_envelope_ft']=[10,14]
 else:
  upper=bpy.data.objects['SheetRoller_Upper'];lower=bpy.data.objects['SheetRoller_LowerFront']
  gap=((upper.location-lower.location).length-upper.dimensions.x/2-lower.dimensions.x/2)*1000
  assert 3.175<gap<3.7,('Roller surface gap',gap)
  assert abs(upper.dimensions.y-3.52)<.001
  checks[folder]['roller_surface_gap_mm']=gap;checks[folder]['shaft_length_cm']=352
(root/'Saved/Logs/EquipmentModelQA.json').write_text(json.dumps({'result':'PASS','checks':checks},indent=2));print('EQUIPMENT_MODEL_QA_PASS')
