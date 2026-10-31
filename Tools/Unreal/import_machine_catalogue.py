"""Import the original machine and forklift-area catalogue cutouts; no map/editor close."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();folder='/Game/Foundryline/UI/BuildCatalogue'
tasks=[];report={}
for name in ['Plasma','Brake','Roller','ToolForklift']:
 source=root/'SourceAssets/UI/BuildCatalogue'/('T_Catalog_'+name+'.png')
 if not source.is_file():raise RuntimeError('Missing original model cutout '+str(source))
 task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=folder;task.destination_name=source.stem
 task.automated=True;task.replace_existing=True;task.save=True;tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
 texture=unreal.load_asset(folder+'/'+task.destination_name)
 if not texture:raise RuntimeError('Missing imported texture '+task.destination_name)
 texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
 texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
 texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
 texture.set_editor_property('srgb',True);unreal.EditorAssetLibrary.save_loaded_asset(texture)
 report[task.destination_name]=texture.get_path_name()
(root/'Saved/Logs/MachineCatalogueImportQA.json').write_text(json.dumps({'passed':len(report)==4,'textures':report},indent=2))
