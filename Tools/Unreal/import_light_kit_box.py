"""Import original marked carton and its product thumbnail without touching vendor content."""
import unreal,json
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();folder='/Game/Foundryline/Products/Components/LightKit'
t=unreal.AssetImportTask();t.filename=str(r/'SourceAssets/Products/Components/LightKit/SM_LightKitBox.fbx');t.destination_path=folder;t.destination_name='SM_LightKitBox';t.automated=True;t.replace_existing=True;t.save=True
o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=True;o.import_textures=False;o.import_as_skeletal=False;o.static_mesh_import_data.combine_meshes=True;o.static_mesh_import_data.auto_generate_collision=False;o.static_mesh_import_data.convert_scene=True;o.static_mesh_import_data.convert_scene_unit=True;t.options=o
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);mesh=unreal.load_asset(folder+'/SM_LightKitBox')
if not mesh:raise RuntimeError('Light-kit mesh did not import')
extent=mesh.get_bounds().box_extent
if not (20<extent.x<23 and 13<extent.y<16 and 13<extent.z<15):raise RuntimeError('Light-kit scale check failed: '+str(extent))
if any(not m.material_interface for m in mesh.get_editor_property('static_materials')):raise RuntimeError('Missing kit material')
tex=unreal.AssetImportTask();tex.filename=str(r/'SourceAssets/UI/BuildCatalogue/T_Catalog_Part15.png');tex.destination_path='/Game/Foundryline/UI/BuildCatalogue';tex.destination_name='T_Catalog_Part15';tex.automated=True;tex.replace_existing=True;tex.save=True;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([tex])
thumbnail=unreal.load_asset('/Game/Foundryline/UI/BuildCatalogue/T_Catalog_Part15');thumbnail.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);thumbnail.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);unreal.EditorAssetLibrary.save_loaded_asset(thumbnail);unreal.EditorAssetLibrary.save_directory(folder,False,True)
(r/'Saved/Logs/LightKitImportQA.json').write_text(json.dumps({'mesh':mesh.get_path_name(),'extent_cm':[extent.x,extent.y,extent.z],'material_slots':len(mesh.get_editor_property('static_materials')),'thumbnail':bool(thumbnail),'passed':True},indent=2));unreal.EditorPythonScripting.set_keep_python_script_alive(True)
import time
_quit_at=time.monotonic()+5
def _finish(dt):
 if time.monotonic()>_quit_at:
  unreal.unregister_slate_post_tick_callback(_handle);unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(_finish)
