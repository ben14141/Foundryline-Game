"""Focused reimport of original 1/2/3-level rack meshes, preserving assigned materials.
Run in an otherwise idle Foundryline editor; this script neither launches nor exits it.
No asset or material is deleted, and vendor content is never touched.
"""
import unreal, json, re
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
source=root/'SourceAssets/Inventory/Storage/ModularPalletRacking'
manifest=json.loads((source/'manifest.json').read_text())
assert manifest['original_authorship']
report={'assets':[],'errors':[],'passed':False}
clean=lambda value:re.sub('[^a-z0-9]','',str(value).lower())
rows=manifest['assets']
# Shared originals are updated only if an existing runtime asset uses them.
for stem in ['PalletRack_ShelfDeck','PalletRack_UprightFrame_1Level','PalletRack_UprightFrame_2Level','PalletRack_UprightFrame_3Level']:
 path='/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_'+stem
 if unreal.EditorAssetLibrary.does_asset_exist(path):rows.append({'name':stem,'file':stem+'.fbx','destination':path})
unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
for row in rows:
 path=row['destination'];old=unreal.load_asset(path)
 if not old:raise RuntimeError('Existing rack required so its materials can be preserved: '+path)
 preserved={clean(slot.material_slot_name):slot.material_interface for slot in old.get_editor_property('static_materials')}
 if not preserved or any(not m for m in preserved.values()):raise RuntimeError('Existing rack has a missing material: '+path)
 original_bounds=old.get_bounds();before=[original_bounds.box_extent.x*2,original_bounds.box_extent.y*2,original_bounds.box_extent.z*2]
 task=unreal.AssetImportTask();task.filename=str(source/row['file']);task.destination_path=path.rsplit('/',1)[0];task.destination_name=path.rsplit('/',1)[1];task.automated=True;task.replace_existing=True;task.replace_existing_settings=False;task.save=False
 options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.convert_scene=True;options.static_mesh_import_data.convert_scene_unit=True;options.static_mesh_import_data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;task.options=options;task.factory=unreal.FbxFactory()
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);mesh=unreal.load_asset(path)
 if not mesh:raise RuntimeError('Rack reimport failed: '+path)
 materials=[]
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=clean(slot.material_slot_name);material=preserved.get(key)
  if not material:
   matches=[m for old_key,m in preserved.items() if old_key in key or key in old_key]
   if len(matches)==1:material=matches[0]
  if not material:raise RuntimeError('Cannot preserve material slot '+str(slot.material_slot_name)+' on '+path)
  mesh.set_material(i,material);materials.append({'slot':str(slot.material_slot_name),'material':material.get_path_name()})
 bounds=mesh.get_bounds();after=[bounds.box_extent.x*2,bounds.box_extent.y*2,bounds.box_extent.z*2]
 variant=next((v for v in manifest['variants'] if v['name']==row['name']),None)
 if variant and any(abs(a-b)>.15 for a,b in zip(after,variant['dimensions_cm'])):raise RuntimeError('Rack scale differs from authored dimensions: '+path+' '+str(after))
 if variant and any(abs(a-b)>.15 for a,b in zip(after,before)):raise RuntimeError('Hardware changed the runtime rack footprint or height: '+path)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 report['assets'].append({'path':path,'before_cm':before,'after_cm':after,'materials':materials})
report['passed']=True
(root/'Saved/Logs/ModularRackHardwareImportQA.json').write_text(json.dumps(report,indent=2))
unreal.log('MODULAR_RACK_HARDWARE_IMPORT_COMPLETE '+str(len(report['assets'])))
