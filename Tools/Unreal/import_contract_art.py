"""Import original company symbols and actual-model product cutouts into game-owned UI content."""
import unreal,json
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve()
exec((r/'Tools/Unreal/import_build_catalogue.py').read_text().replace('unreal.SystemLibrary.quit_editor()',''),{})
folder='/Game/Foundryline/UI/Companies';tasks=[]
for p in (r/'SourceAssets/UI/Companies').glob('*.png'):
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path=folder;t.destination_name=p.stem;t.automated=True;t.replace_existing=True;t.save=True;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
 a=unreal.load_asset(folder+'/'+t.destination_name)
 if not a:raise RuntimeError('Missing company symbol '+t.destination_name)
 a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);a.set_editor_property('srgb',True);unreal.EditorAssetLibrary.save_loaded_asset(a)
(r/'Saved/Logs/ContractArtImport.json').write_text(json.dumps({'company_marks':len(tasks),'product_previews':[p.stem for p in (r/'SourceAssets/UI/BuildCatalogue').glob('T_Catalog_Part[0-9]*.png')]},indent=2));unreal.SystemLibrary.quit_editor()
