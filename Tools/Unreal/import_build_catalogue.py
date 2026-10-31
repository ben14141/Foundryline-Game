"""Import transparent build catalogue cutouts into game-owned UI assets."""
import unreal,json
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();folder='/Game/Foundryline/UI/BuildCatalogue';tasks=[]
for p in (r/'SourceAssets/UI/BuildCatalogue').glob('*.png'):
 dest=r/'Content/Foundryline/UI/BuildCatalogue'/(p.stem+'.uasset')
 if dest.exists() and dest.stat().st_mtime>=p.stat().st_mtime:continue
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path=folder;t.destination_name=p.stem;t.automated=True;t.replace_existing=True;t.save=True;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
report={}
for t in tasks:
 a=unreal.load_asset(folder+'/'+t.destination_name)
 if not a:raise RuntimeError('Missing '+t.destination_name)
 a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);a.set_editor_property('srgb',True);unreal.EditorAssetLibrary.save_loaded_asset(a);report[t.destination_name]=True
(r/'Saved/Logs/CatalogueTextureImport.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
