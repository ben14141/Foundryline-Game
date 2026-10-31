"""Reimport game-owned open racks without changing footprint, bearings or assigned materials."""
import unreal,json,re,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
report={'passed':False,'assets':[],'errors':[]}
clean=lambda value:re.sub('[^a-z0-9]','',str(value).lower())
try:
 unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
 for family in ['ModularPalletRacking','WideBayPalletRacking','OpenSheetShelf']:
  folder=root/'SourceAssets/Inventory/Storage'/family
  manifest=json.loads((folder/'manifest.json').read_text());assert manifest['original_authorship']
  rows=manifest.get('assets',[{'name':'Rack_Shelf400','file':'Rack_Shelf400.fbx','destination':manifest.get('destination','')}])
  if family!='OpenSheetShelf':
   for stem in ['PalletRack_ShelfDeck']+['PalletRack_UprightFrame_'+str(i)+'Level' for i in [1,2,3]]:
    dest='/Game/Foundryline/Inventory/Storage/'+family+'/SM_'+stem
    if unreal.EditorAssetLibrary.does_asset_exist(dest):rows.append({'name':stem,'file':stem+'.fbx','destination':dest})
  for row in rows:
   dest=row['destination'];old=unreal.load_asset(dest)
   if not old:raise RuntimeError('Existing game rack required: '+dest)
   old_slots=old.get_editor_property('static_materials');preserved={clean(s.material_slot_name):s.material_interface for s in old_slots}
   before=old.get_bounds().box_extent*2
   task=unreal.AssetImportTask();task.filename=str(folder/row['file']);task.destination_path=dest.rsplit('/',1)[0];task.destination_name=dest.rsplit('/',1)[1];task.automated=True;task.replace_existing=True;task.save=False
   options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False
   options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.convert_scene=True;options.static_mesh_import_data.convert_scene_unit=True;options.static_mesh_import_data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
   task.options=options;task.factory=unreal.FbxFactory();unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);mesh=unreal.load_asset(dest)
   for i,slot in enumerate(mesh.get_editor_property('static_materials')):
    key=clean(slot.material_slot_name);material=preserved.get(key)
    if not material:
     matches=[m for k,m in preserved.items() if k in key or key in k]
     if len(matches)==1:material=matches[0]
    if family=='OpenSheetShelf':material=old_slots[0].material_interface
    if not material:raise RuntimeError('Cannot preserve '+str(slot.material_slot_name)+' on '+dest)
    mesh.set_material(i,material)
   after=mesh.get_bounds().box_extent*2
   if family!='OpenSheetShelf' and (after-before).length()>.3:raise RuntimeError('Rack dimensions changed: '+dest+' '+str(before)+' -> '+str(after))
   if family=='OpenSheetShelf' and (abs(after.x-120)>.2 or abs(after.y-416)>.2):raise RuntimeError('Sheet shelf scale mismatch '+str(after))
   unreal.EditorAssetLibrary.save_loaded_asset(mesh)
   report['assets'].append({'asset':dest,'dimensions_cm':[after.x,after.y,after.z]})
 report['passed']=True
except Exception:
 report['errors'].append(traceback.format_exc());unreal.log_error(report['errors'][-1])
finally:
 (root/'Saved/Logs/OpenRacksImportQA.json').write_text(json.dumps(report,indent=2))
 unreal.SystemLibrary.quit_editor()
